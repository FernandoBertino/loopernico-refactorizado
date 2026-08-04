#include "StemImportController.h"
#include <cmath>

namespace LoopernicoStems
{
class StemImportController::ImportJob final : public juce::Thread
{
public:
    using Finished = std::function<void(std::shared_ptr<LooperEngine::PreparedTrackBatchImport>,
                                        juce::String, bool)>;

    ImportJob(std::vector<StemData> stemsToImport,
              int expectedTracks, int targetLength, double targetRate,
              ProgressCallback progressToUse, Finished finishedToUse)
        : juce::Thread("Stem import worker"), stems(std::move(stemsToImport)),
          expectedTotalTracks(expectedTracks), requiredLength(targetLength),
          targetSampleRate(targetRate), progress(std::move(progressToUse)),
          finished(std::move(finishedToUse))
    {
    }

    ~ImportJob() override
    {
        cancel();
        stopThread(2000);
    }

    void cancel()
    {
        shouldCancel.store(true, std::memory_order_release);
        signalThreadShouldExit();
    }

    void run() override
    {
        auto prepared = std::make_shared<LooperEngine::PreparedTrackBatchImport>();
        prepared->expectedTotalTracks = expectedTotalTracks;
        prepared->targetSampleRate = targetSampleRate;
        juce::String error;

        try
        {
            juce::AudioFormatManager manager;
            manager.registerBasicFormats();
            juce::int64 commonNaturalLength = 0;

            for (size_t index = 0; index < stems.size(); ++index)
            {
                if (shouldCancel.load(std::memory_order_acquire))
                    break;

                std::unique_ptr<juce::AudioFormatReader> reader(
                    manager.createReaderFor(stems[index].outputFile));
                if (reader == nullptr || reader->sampleRate <= 0.0
                    || reader->lengthInSamples <= 0)
                {
                    error = "No se pudo leer " + stems[index].outputFile.getFileName() + ".";
                    break;
                }

                const auto naturalLength = (juce::int64) std::llround(
                    (double) reader->lengthInSamples * targetSampleRate / reader->sampleRate);
                if (commonNaturalLength == 0)
                    commonNaturalLength = naturalLength;
                else if (std::abs(commonNaturalLength - naturalLength) > 1)
                {
                    error = "Los stems no tienen exactamente la misma duracion.";
                    break;
                }

                const int maxSamples = juce::roundToInt(targetSampleRate * LooperEngine::kMaxLoopSecs);
                const int outputLength = requiredLength > 0
                    ? requiredLength
                    : (int) juce::jlimit<juce::int64>(1, maxSamples, commonNaturalLength);
                if (prepared->lengthSamples == 0)
                    prepared->lengthSamples = outputLength;

                LooperEngine::PreparedTrackImport track;
                track.displayName = stems[index].displayName;
                track.sourceFile = stems[index].outputFile;
                track.audio.setSize(2, outputLength, false, true, false);
                track.audio.clear();

                auto* rawReader = reader.release();
                juce::AudioFormatReaderSource source(rawReader, true);
                juce::ResamplingAudioSource resampler(&source, false, 2);
                resampler.setResamplingRatio(rawReader->sampleRate / targetSampleRate);
                resampler.prepareToPlay(32768, targetSampleRate);
                source.setNextReadPosition(0);

                for (int position = 0; position < outputLength; position += 32768)
                {
                    if (shouldCancel.load(std::memory_order_acquire))
                        break;
                    const int count = juce::jmin(32768, outputLength - position);
                    juce::AudioSourceChannelInfo destination(&track.audio, position, count);
                    resampler.getNextAudioBlock(destination);
                }
                resampler.releaseResources();
                if (shouldCancel.load(std::memory_order_acquire))
                    break;

                prepared->tracks.push_back(std::move(track));
                if (progress != nullptr)
                    progress((float) (index + 1) / (float) stems.size(),
                             "Preparando " + stems[index].displayName + " para el looper");
            }
        }
        catch (const std::bad_alloc&)
        {
            error = "No hay memoria suficiente para importar los stems.";
        }
        catch (const std::exception& e)
        {
            error = "Error al preparar stems: " + juce::String(e.what());
        }

        const bool wasCancelled = shouldCancel.load(std::memory_order_acquire);
        if (wasCancelled || error.isNotEmpty())
            prepared.reset();
        if (finished != nullptr)
            finished(std::move(prepared), error, wasCancelled);
    }

private:
    std::vector<StemData> stems;
    int expectedTotalTracks = 0;
    int requiredLength = 0;
    double targetSampleRate = 0.0;
    ProgressCallback progress;
    Finished finished;
    std::atomic_bool shouldCancel { false };
};

StemImportController::StemImportController(LooperEngine& looperToUse,
                                           juce::CriticalSection& audioCallbackLock)
    : looper(looperToUse), audioLock(audioCallbackLock)
{
}

StemImportController::~StemImportController()
{
    lifetimeToken.reset();
    cancel();
    const juce::ScopedLock lock(jobLock);
    job.reset();
}

bool StemImportController::importAsync(const std::vector<StemData>& stems,
                                       ProgressCallback progressCallback,
                                       CompletionCallback completionCallback)
{
    const juce::ScopedLock lock(jobLock);
    if (running.load(std::memory_order_acquire))
        return false;

    const int existing = looper.getTotalTracks();
    const int freeTracks = LooperEngine::kMaxTracks - existing;
    if (stems.empty() || freeTracks <= 0)
    {
        ImportResult result;
        result.errorMessage = freeTracks <= 0
            ? "No hay pistas libres. Loopernico tiene 12 pistas ocupadas."
            : "No hay stems para importar.";
        if (completionCallback != nullptr)
            completionCallback(std::move(result));
        return false;
    }

    const int selectedCount = juce::jmin(freeTracks, (int) stems.size());
    std::vector<StemData> selected(stems.begin(), stems.begin() + selectedCount);
    const bool partial = selectedCount < (int) stems.size();
    const int requiredLength = existing > 0 ? looper.getBaseLoopSamples() : 0;
    const double targetRate = looper.getSampleRate();
    auto weakToken = std::weak_ptr<int>(lifetimeToken);
    auto safeProgress = [weakToken, callback = std::move(progressCallback)]
                        (float value, const juce::String& text)
    {
        juce::MessageManager::callAsync([weakToken, callback, value, text]()
        {
            if (!weakToken.expired() && callback != nullptr)
                callback(value, text);
        });
    };
    auto finished = [this, weakToken, selected, partial,
                     callback = std::move(completionCallback)]
                    (std::shared_ptr<LooperEngine::PreparedTrackBatchImport> prepared,
                     juce::String error, bool cancelled) mutable
    {
        juce::MessageManager::callAsync(
            [this, weakToken, prepared = std::move(prepared), selected, partial,
             error = std::move(error), cancelled, callback = std::move(callback)]() mutable
        {
            if (!weakToken.expired())
                finishPrepared(std::move(prepared), std::move(selected), partial,
                               std::move(error), cancelled, std::move(callback));
        });
    };

    running.store(true, std::memory_order_release);
    job = std::make_unique<ImportJob>(selected, existing, requiredLength, targetRate,
                                      std::move(safeProgress), std::move(finished));
    if (!job->startThread(juce::Thread::Priority::low))
    {
        running.store(false, std::memory_order_release);
        job.reset();
        return false;
    }
    return true;
}

void StemImportController::finishPrepared(
    std::shared_ptr<LooperEngine::PreparedTrackBatchImport> prepared,
    std::vector<StemData> selectedStems, bool partial,
    juce::String preparationError, bool cancelled,
    CompletionCallback completionCallback)
{
    ImportResult result;
    result.cancelled = cancelled;
    result.partial = partial;
    result.errorMessage = std::move(preparationError);

    if (prepared != nullptr && !cancelled && result.errorMessage.isEmpty())
    {
        LooperEngine::TrackBatchImportResult committed;
        {
            const juce::ScopedLock callbackLock(audioLock);
            committed = looper.commitPreparedTrackBatchImport(std::move(*prepared));
        }
        result.success = committed.success;
        result.errorMessage = committed.errorMessage;
        if (committed.success)
        {
            for (int i = 0; i < committed.importedTracks; ++i)
                result.imported.push_back({ selectedStems[(size_t) i], committed.firstTrack + i });
            if (partial)
                result.errorMessage = "Solo se importaron " + juce::String(committed.importedTracks)
                    + " stems porque no habia mas pistas libres.";
        }
    }
    else if (cancelled)
        result.errorMessage = "Importacion cancelada.";

    {
        const juce::ScopedLock lock(jobLock);
        running.store(false, std::memory_order_release);
        job.reset();
    }
    if (completionCallback != nullptr)
        completionCallback(std::move(result));
}

void StemImportController::cancel()
{
    const juce::ScopedLock lock(jobLock);
    if (job != nullptr)
        job->cancel();
}
}
