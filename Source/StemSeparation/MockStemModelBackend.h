#pragma once

#include "StemModelBackend.h"

namespace LoopernicoStems
{
class MockStemModelBackend final : public StemModelBackend
{
public:
    juce::String getName() const override { return "Mock (no separa instrumentos)"; }
    std::vector<StemType> getSupportedStems() const override;
    bool prepare(juce::String& errorMessage) override;
    StemSeparationResult process(const StemSeparationOptions& options,
                                 ProgressReporter reportProgress,
                                 std::atomic_bool& shouldCancel) override;
};
}
