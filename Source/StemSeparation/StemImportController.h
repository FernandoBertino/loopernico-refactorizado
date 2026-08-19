#pragma once

#include "StemSeparationTypes.h"
#include "../LooperEngine.h"

namespace LoopernicoStems
{
class StemImportController
{
public:
    struct ImportedStem
    {
        StemData stem;
        int trackIndex = -1;
    };

    struct ImportResult
    {
        bool success = false;
        bool cancelled = false;
        bool partial = false;
        juce::String errorMessage;
        std::vector<ImportedStem> imported;
    };

    using ProgressCallback = std::function<void(float, const juce::String&)>;
    using CompletionCallback = std::function<void(ImportResult)>;

    StemImportController(LooperEngine& looperToUse, juce::CriticalSection& audioCallbackLock);
    ~StemImportController();

    bool importAsync(const std::vector<StemData>& stems,
                     ProgressCallback progressCallback,
                     CompletionCallback completionCallback);
    void cancel();
    bool isRunning() const noexcept { return running.load(std::memory_order_acquire); }

private:
    class ImportJob;
    void finishPrepared(std::shared_ptr<LooperEngine::PreparedTrackBatchImport> prepared,
                        std::vector<StemData> selectedStems,
                        bool partial,
                        juce::String preparationError,
                        bool cancelled,
                        CompletionCallback completionCallback);

    LooperEngine& looper;
    juce::CriticalSection& audioLock;
    std::unique_ptr<ImportJob> job;
    std::atomic_bool running { false };
    std::shared_ptr<int> lifetimeToken = std::make_shared<int>(0);
    juce::CriticalSection jobLock;
};
}
