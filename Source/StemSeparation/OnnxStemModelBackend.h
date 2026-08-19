#pragma once

#include "StemModelBackend.h"

namespace LoopernicoStems
{
class OnnxStemModelBackend final : public StemModelBackend
{
public:
    enum class ExecutionProvider { cpu, directML, cuda };

    struct ModelMetadata
    {
        juce::File modelFile;
        double requiredSampleRate = 0.0;
        int requiredChannels = 0;
        int blockLength = 0;
        int overlapSamples = 0;
        juce::String inputName;
        juce::StringArray outputNames;
        std::vector<StemType> outputStemOrder;
        bool requiresNormalisation = false;
        ExecutionProvider provider = ExecutionProvider::cpu;
    };

    explicit OnnxStemModelBackend(ModelMetadata metadataToUse = {});
    juce::String getName() const override { return "ONNX Runtime"; }
    std::vector<StemType> getSupportedStems() const override;
    bool prepare(juce::String& errorMessage) override;
    StemSeparationResult process(const StemSeparationOptions& options,
                                 ProgressReporter reportProgress,
                                 std::atomic_bool& shouldCancel) override;

private:
    ModelMetadata metadata;
};
}
