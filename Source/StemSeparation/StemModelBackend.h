#pragma once

#include "StemSeparationTypes.h"
#include <atomic>
#include <functional>

namespace LoopernicoStems
{
class StemModelBackend
{
public:
    using ProgressReporter = std::function<void(float, const juce::String&)>;

    virtual ~StemModelBackend() = default;
    virtual juce::String getName() const = 0;
    virtual std::vector<StemType> getSupportedStems() const = 0;
    virtual bool prepare(juce::String& errorMessage) = 0;
    virtual StemSeparationResult process(const StemSeparationOptions& options,
                                         ProgressReporter reportProgress,
                                         std::atomic_bool& shouldCancel) = 0;
};
}
