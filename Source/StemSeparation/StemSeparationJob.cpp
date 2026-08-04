#include "StemSeparationJob.h"

namespace LoopernicoStems
{
StemSeparationJob::StemSeparationJob(std::unique_ptr<StemModelBackend> backendToUse,
                                     StemSeparationOptions optionsToUse,
                                     StemModelBackend::ProgressReporter progressToUse,
                                     FinishedCallback finishedToUse)
    : juce::Thread("Stem separation worker"),
      backend(std::move(backendToUse)), options(std::move(optionsToUse)),
      progress(std::move(progressToUse)), finished(std::move(finishedToUse))
{
}

StemSeparationJob::~StemSeparationJob()
{
    cancel();
    stopThread(2000);
}

void StemSeparationJob::cancel() noexcept
{
    shouldCancel.store(true, std::memory_order_release);
    signalThreadShouldExit();
}

void StemSeparationJob::run()
{
    const double startedAtMs = juce::Time::getMillisecondCounterHiRes();
    StemSeparationResult result;
    try
    {
        juce::String error;
        progress(0.0f, "Preparando backend: " + backend->getName());
        juce::Logger::writeToLog("Stem backend: " + backend->getName());
        if (!backend->prepare(error))
            result.errorMessage = error;
        else if (shouldCancel.load(std::memory_order_acquire))
        {
            result.cancelled = true;
            result.errorMessage = "Separacion cancelada.";
        }
        else
            result = backend->process(options, progress, shouldCancel);
    }
    catch (const std::bad_alloc&)
    {
        result.errorMessage = "No hay memoria suficiente para completar la separacion.";
    }
    catch (const std::exception& e)
    {
        result.errorMessage = "Error interno de separacion: " + juce::String(e.what());
    }
    catch (...)
    {
        result.errorMessage = "Error interno desconocido durante la separacion.";
    }

    const double elapsedMs = juce::Time::getMillisecondCounterHiRes() - startedAtMs;
    juce::Logger::writeToLog("Stem job: "
        + juce::String(result.success ? "finalizado" : result.cancelled ? "cancelado" : "error")
        + ", tiempo=" + juce::String(elapsedMs / 1000.0, 3) + " s"
        + (result.errorMessage.isNotEmpty() ? ", detalle=" + result.errorMessage : juce::String()));

    if (finished != nullptr)
        finished(std::move(result));
}
}
