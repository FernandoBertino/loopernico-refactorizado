#pragma once

#include "StemModelBackend.h"

namespace LoopernicoStems
{
class DemucsCppStemModelBackend final : public StemModelBackend
{
public:
    struct Configuration
    {
        juce::File executable;
        juce::File modelFile;
    };

    explicit DemucsCppStemModelBackend(Configuration configurationToUse);

    juce::String getName() const override { return "Demucs 6 stems (CPU externo)"; }
    std::vector<StemType> getSupportedStems() const override;
    bool prepare(juce::String& errorMessage) override;
    StemSeparationResult process(const StemSeparationOptions& options,
                                 ProgressReporter reportProgress,
                                 std::atomic_bool& shouldCancel) override;

private:
    bool createCanonicalInput(const juce::File& inputFile,
                              const juce::File& outputFile,
                              ProgressReporter reportProgress,
                              std::atomic_bool& shouldCancel,
                              juce::String& errorMessage) const;
    bool validateOutputs(const StemSeparationOptions& options,
                         StemSeparationResult& result,
                         juce::String& errorMessage) const;
    bool normaliseFile(const juce::File& file,
                       std::atomic_bool& shouldCancel,
                       juce::String& errorMessage) const;

    Configuration configuration;
};
}
