#pragma once

#include <JuceHeader.h>

namespace LoopernicoLatency
{
    struct Breakdown
    {
        int inputSamples{ 0 };
        int outputSamples{ 0 };
        int pluginSamples{ 0 };
        int calibrationSamples{ 0 };

        int driverRoundTripSamples() const noexcept;
        int automaticSamples() const noexcept;
        int effectiveSamples() const noexcept;
    };

    // Positive compensation writes earlier in the loop. Negative calibration
    // values deliberately move the recorded sample later. The int64 arithmetic
    // keeps extreme calibration values from overflowing before wrap-around.
    int compensatedLoopPosition(int transportPosition,
                                int loopLengthSamples,
                                int compensationSamples) noexcept;

    float monitoredInputSample(float inputSample, bool softwareMonitoringEnabled) noexcept;
    void addRhythmToRecordableBuffer(juce::AudioBuffer<float>& recordableInput,
                                     const juce::AudioBuffer<float>& rhythm,
                                     int startSample,
                                     int numSamples) noexcept;
    float separateRhythmMonitorSample(float rhythmSample,
                                      bool recordableInputIsMonitored) noexcept;
    float mixOutputSample(float looperSample, float rhythmSample, float masterVolume) noexcept;

    bool runDeterministicTests(juce::String& report);
}
