#include "LatencyCompensation.h"

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

        if (failures.empty())
        {
            report = "OK: 14 pruebas deterministas de compensacion y buses pasaron; "
                     "el rhythm entra al looper sin duplicarse en monitoreo.";
            return true;
        }

        report = "FALLARON " + juce::String((int) failures.size()) + " pruebas:\n";
        for (const auto& failure : failures)
            report << failure.name << ": esperado " << failure.expected
                   << ", obtenido " << failure.actual << "\n";
        return false;
    }
}
