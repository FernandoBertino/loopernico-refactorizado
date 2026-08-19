#include "LatencyCompensation.h"
#include "DrumMidiMap.h"
#include "DrumMidiPattern.h"
#include "EffectsChain.h"

#include <cmath>
#include <limits>
#include <vector>

namespace LoopernicoLatency
{
    int Breakdown::driverRoundTripSamples() const noexcept
    {
        return juce::jmax(0, inputSamples) + juce::jmax(0, outputSamples);
    }

    int Breakdown::automaticSamples() const noexcept
    {
        return driverRoundTripSamples() + juce::jmax(0, pluginSamples);
    }

    int Breakdown::effectiveSamples() const noexcept
    {
        const auto total = (juce::int64) automaticSamples() + calibrationSamples;
        return (int) juce::jlimit((juce::int64) std::numeric_limits<int>::min(),
                                  (juce::int64) std::numeric_limits<int>::max(), total);
    }

    int compensatedLoopPosition(int transportPosition,
                                int loopLengthSamples,
                                int compensationSamples) noexcept
    {
        if (loopLengthSamples <= 0)
            return 0;

        const auto length = (juce::int64) loopLengthSamples;
        auto wrapped = ((juce::int64) transportPosition - compensationSamples) % length;
        if (wrapped < 0)
            wrapped += length;
        return (int) wrapped;
    }

    float monitoredInputSample(float inputSample, bool softwareMonitoringEnabled) noexcept
    {
        return softwareMonitoringEnabled ? inputSample : 0.0f;
    }

    void addRhythmToRecordableBuffer(juce::AudioBuffer<float>& recordableInput,
                                     const juce::AudioBuffer<float>& rhythm,
                                     int startSample,
                                     int numSamples) noexcept
    {
        const int safeStart = juce::jmax(0, startSample);
        const int channels = juce::jmin(recordableInput.getNumChannels(), rhythm.getNumChannels());
        const int available = juce::jmin(recordableInput.getNumSamples(), rhythm.getNumSamples())
                            - safeStart;
        const int samplesToMix = juce::jmin(juce::jmax(0, numSamples), juce::jmax(0, available));
        for (int ch = 0; ch < channels; ++ch)
            recordableInput.addFrom(ch, safeStart, rhythm, ch, safeStart, samplesToMix);
    }

    float separateRhythmMonitorSample(float rhythmSample,
                                      bool recordableInputIsMonitored) noexcept
    {
        return recordableInputIsMonitored ? 0.0f : rhythmSample;
    }

    float mixOutputSample(float looperSample, float rhythmSample, float masterVolume) noexcept
    {
        return juce::jlimit(-1.0f, 1.0f, looperSample + rhythmSample)
             * juce::jlimit(0.0f, 1.0f, masterVolume);
    }

    bool runDeterministicTests(juce::String& report)
    {
        struct Failure
        {
            juce::String name;
            int expected;
            int actual;
        };

        std::vector<Failure> failures;
        const auto expect = [&failures](const juce::String& name, int expected, int actual)
        {
            if (expected != actual)
                failures.push_back({ name, expected, actual });
        };

        // An impulse performed at loop sample 7 arrives nine samples later.
        expect("impulso lineal", 7, compensatedLoopPosition(16, 32, 9));

        // The same calculation must wrap safely when the input arrives after
        // the transport has crossed the loop boundary.
        expect("impulso con wrap-around", 29, compensatedLoopPosition(3, 32, 6));

        // A negative physical-loopback calibration is valid and moves the
        // destination later without ever producing a negative index.
        expect("offset negativo", 8, compensatedLoopPosition(4, 32, -4));
        expect("compensacion mayor al loop", 5, compensatedLoopPosition(1, 8, 20));
        expect("transporte negativo seguro", 15, compensatedLoopPosition(-1, 16, 0));

        Breakdown breakdown;
        breakdown.inputSamples = 205;
        breakdown.outputSamples = 117;
        breakdown.pluginSamples = 64;
        breakdown.calibrationSamples = -10;
        expect("ida y vuelta", 322, breakdown.driverRoundTripSamples());
        expect("automatico con VST3", 386, breakdown.automaticSamples());
        expect("total efectivo", 376, breakdown.effectiveSamples());
        expect("monitoreo de entrada activo", 250,
               juce::roundToInt(monitoredInputSample(0.25f, true) * 1000.0f));

        juce::AudioBuffer<float> recordable(2, 4);
        juce::AudioBuffer<float> rhythm(2, 4);
        recordable.clear();
        rhythm.clear();
        recordable.setSample(0, 1, 0.50f);
        rhythm.setSample(0, 1, 0.25f);
        addRhythmToRecordableBuffer(recordable, rhythm, 0, 4);
        expect("rhythm entra al bus grabable", 750,
               juce::roundToInt(recordable.getSample(0, 1) * 1000.0f));
        expect("rhythm no se duplica con monitoreo software", 0,
               juce::roundToInt(separateRhythmMonitorSample(0.25f, true) * 1000.0f));
        expect("rhythm se conserva con monitoreo directo", 250,
               juce::roundToInt(separateRhythmMonitorSample(0.25f, false) * 1000.0f));
        expect("Direct conserva bus de ritmo", 250,
               juce::roundToInt(mixOutputSample(monitoredInputSample(0.75f, false),
                                                0.25f, 1.0f) * 1000.0f));
        expect("mezcla final limita antes del master", 500,
               juce::roundToInt(mixOutputSample(0.8f, 0.6f, 0.5f) * 1000.0f));

        LoopernicoMidi::DrumNoteMap drumMap;
        expect("mapa GM kick", 36, drumMap.getNote(LoopernicoMidi::DrumLane::Kick));
        expect("mapa GM canal", 10, drumMap.getChannel(LoopernicoMidi::DrumLane::Snare));
        expect("mapa acepta nota personalizada", 1,
               drumMap.setNote(LoopernicoMidi::DrumLane::Kick, 35) ? 1 : 0);
        expect("mapa conserva nota personalizada", 35,
               drumMap.getNote(LoopernicoMidi::DrumLane::Kick));
        expect("mapa rechaza nota fuera de rango", 0,
               drumMap.setNote(LoopernicoMidi::DrumLane::Kick, 128) ? 1 : 0);
        expect("mapa rechaza canal fuera de rango", 0,
               drumMap.setChannel(LoopernicoMidi::DrumLane::Kick, 17) ? 1 : 0);

        LoopernicoMidi::DrumMidiPattern pattern;
        expect("patron inicia en un compas", 1, pattern.getBars());
        expect("patron inicia vacio", 0, pattern.countActiveSteps());
        expect("patron acepta velocidad", 1,
               pattern.setVelocity(LoopernicoMidi::DrumLane::Kick, 0, 110) ? 1 : 0);
        expect("patron conserva velocidad", 110,
               pattern.getVelocity(LoopernicoMidi::DrumLane::Kick, 0));
        pattern.setBars(4);
        expect("patron permite cuatro compases", 4, pattern.getBars());
        pattern.setVelocity(LoopernicoMidi::DrumLane::Kick, 32, 90);
        expect("patron rechaza paso fuera de rango", 0,
               pattern.setVelocity(LoopernicoMidi::DrumLane::Snare, 64, 100) ? 1 : 0);
        pattern.setBars(1);
        expect("patron limpia pasos fuera al reducir", 0,
               pattern.getVelocity(LoopernicoMidi::DrumLane::Kick, 32));

        EffectsChain cooltron;
        cooltron.prepare(48000.0, 256);
        cooltron.setCooltronEnabled(true);
        cooltron.setCooltronGain1(0.5f);
        cooltron.setCooltronVolume1(0.5f);
        cooltron.setCooltronTone1(0.5f);
        cooltron.setCooltronGain2(0.5f);
        cooltron.setCooltronVolume2(0.5f);
        cooltron.setCooltronTone2(0.5f);
        auto measureCooltronLowFrequency = [&cooltron](bool od2Selected, int bassBoost)
        {
            constexpr int blockSize = 256;
            constexpr int blockCount = 16;
            constexpr double sampleRate = 48000.0;
            constexpr double frequency = 100.0;
            juce::AudioBuffer<float> audio(2, blockSize);
            juce::AudioBuffer<float> rhythm(2, blockSize);
            cooltron.reset();
            cooltron.setCooltronOd2Selected(od2Selected);
            cooltron.setCooltronBassBoost(bassBoost);
            double sumSquares = 0.0;
            int measuredSamples = 0;
            for (int block = 0; block < blockCount; ++block)
            {
                for (int sample = 0; sample < blockSize; ++sample)
                {
                    const int absoluteSample = block * blockSize + sample;
                    const float input = 0.035f * std::sin(
                        juce::MathConstants<double>::twoPi * frequency
                        * (double) absoluteSample / sampleRate);
                    audio.setSample(0, sample, input);
                    audio.setSample(1, sample, input);
                }
                rhythm.clear();
                cooltron.processBlock(audio, rhythm, 0, blockSize);
                if (block >= blockCount / 2)
                    for (int sample = 0; sample < blockSize; ++sample)
                    {
                        const double value = audio.getSample(0, sample);
                        sumSquares += value * value;
                        ++measuredSamples;
                    }
            }
            return std::sqrt(sumSquares / (double) juce::jmax(1, measuredSamples));
        };

        const double od1Off = measureCooltronLowFrequency(false, 0);
        const double od1Od2Only = measureCooltronLowFrequency(false, 1);
        const double od1Both = measureCooltronLowFrequency(false, 2);
        const double od2Off = measureCooltronLowFrequency(true, 0);
        const double od2Only = measureCooltronLowFrequency(true, 1);
        const double od2Both = measureCooltronLowFrequency(true, 2);
        expect("BASS BOOST OD2 no altera OD1", 1,
               std::abs(od1Off - od1Od2Only) < 1.0e-6 ? 1 : 0);
        expect("BASS BOOST BOTH refuerza OD1", 1, od1Both > od1Off * 1.05 ? 1 : 0);
        expect("BASS BOOST OD2 refuerza OD2", 1, od2Only > od2Off * 1.05 ? 1 : 0);
        expect("BASS BOOST BOTH conserva refuerzo OD2", 1,
               std::abs(od2Only - od2Both) < 1.0e-6 ? 1 : 0);

        if (failures.empty())
        {
            report = "OK: 31 pruebas deterministas pasaron, incluidas las tres posiciones "
                     "DSP de BASS BOOST en OD1 y OD2.";
            return true;
        }

        report = "FALLARON " + juce::String((int) failures.size()) + " pruebas:\n";
        for (const auto& failure : failures)
            report << failure.name << ": esperado " << failure.expected
                   << ", obtenido " << failure.actual << "\n";
        return false;
    }
}
