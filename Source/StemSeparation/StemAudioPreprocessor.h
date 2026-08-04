#pragma once

#include <JuceHeader.h>
#include <memory>

namespace LoopernicoStems
{
class StemAudioPreprocessor
{
public:
    struct AudioInfo
    {
        double sampleRate = 0.0;
        unsigned int sourceChannels = 0;
        juce::int64 lengthInSamples = 0;
        double durationSeconds = 0.0;
    };

    class Reader
    {
    public:
        Reader(std::unique_ptr<juce::AudioFormatReader> sourceReader, AudioInfo sourceInfo);

        const AudioInfo& getInfo() const noexcept { return info; }
        bool readStereoBlock(juce::int64 sourceSample, int samples,
                             juce::AudioBuffer<float>& destination,
                             juce::String& errorMessage);

    private:
        std::unique_ptr<juce::AudioFormatReader> reader;
        AudioInfo info;
    };

    StemAudioPreprocessor();
    std::unique_ptr<Reader> open(const juce::File& inputFile,
                                 juce::String& errorMessage) const;

    static bool isSupportedExtension(const juce::File& file);
    static constexpr int defaultBlockSize = 32768;
    static constexpr int defaultOverlapSamples = 2048;

private:
    mutable juce::AudioFormatManager formatManager;
};
}
