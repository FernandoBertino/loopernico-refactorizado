#pragma once

#include <JuceHeader.h>
#include <memory>

namespace LoopernicoStems
{
class StemAudioPostprocessor
{
public:
    class WavWriter
    {
    public:
        static std::unique_ptr<WavWriter> create(const juce::File& file,
                                                 double sampleRate,
                                                 int channels,
                                                 juce::String& errorMessage);
        bool write(const juce::AudioBuffer<float>& buffer, int samples,
                   juce::String& errorMessage);

    private:
        explicit WavWriter(std::unique_ptr<juce::AudioFormatWriter> outputWriter);
        std::unique_ptr<juce::AudioFormatWriter> writer;
    };

    static void applyGainAndLimit(juce::AudioBuffer<float>& block, float gain);
    static void applyJoinFades(juce::AudioBuffer<float>& block,
                               bool fadeIn, bool fadeOut, int overlapSamples);
};
}
