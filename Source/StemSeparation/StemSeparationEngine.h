#pragma once

#include "StemSeparationJob.h"

namespace LoopernicoStems
{
class StemSeparationEngine
{
public:
    using ProgressCallback = std::function<void(float, const juce::String&)>;
    using CompletionCallback = std::function<void(StemSeparationResult)>;

    StemSeparationEngine();
    ~StemSeparationEngine();

    bool startSeparation(const StemSeparationOptions& options,
                         ProgressCallback progressCallback,
                         CompletionCallback completionCallback);
    void cancel();
    bool isRunning() const noexcept { return running.load(std::memory_order_acquire); }

    static juce::File getTemporaryRoot();
    static void removeIncompleteJob(const juce::File& jobDirectory);

private:
    std::unique_ptr<StemModelBackend> createBackend(const StemSeparationOptions& options) const;
    juce::File createUniqueJobDirectory(juce::String& errorMessage) const;

    std::unique_ptr<StemSeparationJob> job;
    std::atomic_bool running { false };
    std::shared_ptr<int> lifetimeToken = std::make_shared<int>(0);
    mutable juce::CriticalSection jobLock;
};
}
