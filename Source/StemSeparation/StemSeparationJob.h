#pragma once

#include "StemModelBackend.h"

namespace LoopernicoStems
{
class StemSeparationJob final : public juce::Thread
{
public:
    using FinishedCallback = std::function<void(StemSeparationResult)>;

    StemSeparationJob(std::unique_ptr<StemModelBackend> backendToUse,
                      StemSeparationOptions optionsToUse,
                      StemModelBackend::ProgressReporter progressToUse,
                      FinishedCallback finishedToUse);
    ~StemSeparationJob() override;

    void run() override;
    void cancel() noexcept;

private:
    std::unique_ptr<StemModelBackend> backend;
    StemSeparationOptions options;
    StemModelBackend::ProgressReporter progress;
    FinishedCallback finished;
    std::atomic_bool shouldCancel { false };
};
}
