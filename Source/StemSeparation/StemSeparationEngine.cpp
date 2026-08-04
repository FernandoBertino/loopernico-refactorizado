#include "StemSeparationEngine.h"
#include "DemucsCppStemModelBackend.h"
#include "MockStemModelBackend.h"
#include "OnnxStemModelBackend.h"

namespace LoopernicoStems
{
StemSeparationEngine::StemSeparationEngine() = default;

StemSeparationEngine::~StemSeparationEngine()
{
    lifetimeToken.reset();
    cancel();
    const juce::ScopedLock lock(jobLock);
    job.reset();
}

juce::File StemSeparationEngine::getTemporaryRoot()
{
    return juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("Loopernico").getChildFile("StemSeparation");
}

juce::File StemSeparationEngine::createUniqueJobDirectory(juce::String& errorMessage) const
{
    const auto root = getTemporaryRoot();
    if (!root.createDirectory())
    {
        errorMessage = "No se pudo crear la carpeta temporal de separacion.";
        return {};
    }

    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const auto id = juce::Uuid().toString();
        const auto directory = root.getChildFile(id);
        if (!directory.exists() && directory.createDirectory())
            return directory;
    }
    errorMessage = "No se pudo crear un directorio temporal unico.";
    return {};
}

void StemSeparationEngine::removeIncompleteJob(const juce::File& jobDirectory)
{
    if (jobDirectory == juce::File() || !jobDirectory.isAChildOf(getTemporaryRoot()))
        return;
    if (jobDirectory.exists() && !jobDirectory.deleteRecursively())
        juce::Logger::writeToLog("No se pudo limpiar el trabajo stem incompleto: "
                                 + jobDirectory.getFullPathName());
}

std::unique_ptr<StemModelBackend> StemSeparationEngine::createBackend(
    const StemSeparationOptions& options) const
{
    if (options.mode == StemSeparationMode::demucsCppSixStem)
        return std::make_unique<DemucsCppStemModelBackend>(
            DemucsCppStemModelBackend::Configuration {
                options.demucsCppExecutable, options.demucsCppModel });
    if (options.mode == StemSeparationMode::onnxSixStem)
        return std::make_unique<OnnxStemModelBackend>();
    return std::make_unique<MockStemModelBackend>();
}

bool StemSeparationEngine::startSeparation(const StemSeparationOptions& requestedOptions,
                                           ProgressCallback progressCallback,
                                           CompletionCallback completionCallback)
{
    const juce::ScopedLock lock(jobLock);
    if (running.load(std::memory_order_acquire))
    {
        if (completionCallback != nullptr)
        {
            StemSeparationResult result;
            result.errorMessage = "Ya hay una separacion en curso.";
            completionCallback(std::move(result));
        }
        return false;
    }

    StemSeparationOptions options = requestedOptions;
    juce::String error;
    options.outputDirectory = createUniqueJobDirectory(error);
    if (options.outputDirectory == juce::File())
    {
        if (completionCallback != nullptr)
        {
            StemSeparationResult result;
            result.errorMessage = error;
            completionCallback(std::move(result));
        }
        return false;
    }

    auto backend = createBackend(options);
    auto weakToken = std::weak_ptr<int>(lifetimeToken);
    const auto jobDirectory = options.outputDirectory;
    auto safeProgress = [weakToken, callback = std::move(progressCallback)]
                        (float value, const juce::String& status)
    {
        juce::MessageManager::callAsync([weakToken, callback, value, status]()
        {
            if (!weakToken.expired() && callback != nullptr)
                callback(juce::jlimit(0.0f, 1.0f, value), status);
        });
    };
    auto safeFinished = [this, weakToken, callback = std::move(completionCallback), jobDirectory]
                        (StemSeparationResult result) mutable
    {
        juce::MessageManager::callAsync(
            [this, weakToken, callback, jobDirectory, result = std::move(result)]() mutable
        {
            if (weakToken.expired())
                return;
            {
                const juce::ScopedLock callbackLock(jobLock);
                running.store(false, std::memory_order_release);
                job.reset();
            }
            if (!result.success)
                removeIncompleteJob(jobDirectory);
            if (callback != nullptr)
                callback(std::move(result));
        });
    };

    running.store(true, std::memory_order_release);
    job = std::make_unique<StemSeparationJob>(std::move(backend), std::move(options),
                                              std::move(safeProgress), std::move(safeFinished));
    if (!job->startThread(juce::Thread::Priority::low))
    {
        running.store(false, std::memory_order_release);
        job.reset();
        removeIncompleteJob(jobDirectory);
        if (completionCallback != nullptr)
        {
            StemSeparationResult result;
            result.errorMessage = "No se pudo iniciar el hilo de separacion.";
            completionCallback(std::move(result));
        }
        return false;
    }
    return true;
}

void StemSeparationEngine::cancel()
{
    const juce::ScopedLock lock(jobLock);
    if (job != nullptr)
        job->cancel();
}
}
