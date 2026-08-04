#include "DemucsCppStemModelBackend.h"
#include "StemAudioPostprocessor.h"
#include "StemAudioPreprocessor.h"
#include <array>

namespace LoopernicoStems
{
namespace
{
constexpr double demucsSampleRate = 44100.0;
constexpr int conversionBlockSize = 32768;

struct DemucsOutput
{
    StemType type;
    const char* fileName;
};

constexpr std::array<DemucsOutput, 6> demucsOutputs {{
    { StemType::drums,  "target_0_drums.wav" },
    { StemType::bass,   "target_1_bass.wav" },
    { StemType::other,  "target_2_other.wav" },
    { StemType::vocals, "target_3_vocals.wav" },
    { StemType::guitar, "target_4_guitar.wav" },
    { StemType::piano,  "target_5_piano.wav" }
}};

bool isCancelled(const std::atomic_bool& shouldCancel)
{
    return shouldCancel.load(std::memory_order_acquire);
}
}

DemucsCppStemModelBackend::DemucsCppStemModelBackend(Configuration configurationToUse)
    : configuration(std::move(configurationToUse))
{
}

std::vector<StemType> DemucsCppStemModelBackend::getSupportedStems() const
{
    return standardSixStems();
}

bool DemucsCppStemModelBackend::prepare(juce::String& errorMessage)
{
    if (!configuration.executable.existsAsFile())
    {
        errorMessage = "No se encontro el ejecutable demucs.cpp seleccionado.";
        return false;
    }
    if (!configuration.modelFile.existsAsFile())
    {
        errorMessage = "No se encontro el archivo de pesos de seis stems seleccionado.";
        return false;
    }
    if (configuration.modelFile.getSize() <= 0)
    {
        errorMessage = "El archivo de pesos esta vacio o no se puede leer.";
        return false;
    }

    juce::Logger::writeToLog("Stem Demucs executable: "
                             + configuration.executable.getFullPathName());
    juce::Logger::writeToLog("Stem Demucs model: "
                             + configuration.modelFile.getFileName()
                             + ", bytes=" + juce::String(configuration.modelFile.getSize()));
    errorMessage.clear();
    return true;
}

bool DemucsCppStemModelBackend::createCanonicalInput(
    const juce::File& inputFile, const juce::File& outputFile,
    ProgressReporter reportProgress, std::atomic_bool& shouldCancel,
    juce::String& errorMessage) const
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(inputFile));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0
        || reader->numChannels == 0)
    {
        errorMessage = "No se pudo decodificar la cancion para preparar Demucs.";
        return false;
    }

    const auto sourceRate = reader->sampleRate;
    const auto sourceChannels = reader->numChannels;
    const auto expectedSamples = juce::jmax<juce::int64>(1,
        (juce::int64) std::llround((double) reader->lengthInSamples
                                   * demucsSampleRate / sourceRate));
    juce::AudioFormatReaderSource readerSource(reader.release(), true);
    juce::ResamplingAudioSource resampler(&readerSource, false, 2);
    resampler.setResamplingRatio(sourceRate / demucsSampleRate);
    resampler.prepareToPlay(conversionBlockSize, demucsSampleRate);

    auto writer = StemAudioPostprocessor::WavWriter::create(
        outputFile, demucsSampleRate, 2, errorMessage);
    if (writer == nullptr)
        return false;

    juce::AudioBuffer<float> block(2, conversionBlockSize);
    for (juce::int64 position = 0; position < expectedSamples;)
    {
        if (isCancelled(shouldCancel))
        {
            errorMessage = "Separacion cancelada.";
            return false;
        }

        const int samples = (int) juce::jmin<juce::int64>(
            conversionBlockSize, expectedSamples - position);
        block.clear();
        juce::AudioSourceChannelInfo info(&block, 0, samples);
        resampler.getNextAudioBlock(info);
        if (sourceChannels == 1)
            block.copyFrom(1, 0, block, 0, 0, samples);
        if (!writer->write(block, samples, errorMessage))
            return false;

        position += samples;
        const float fraction = (float) ((double) position / (double) expectedSamples);
        reportProgress(0.03f + 0.17f * fraction, "Preparando audio para Demucs");
    }
    resampler.releaseResources();
    return true;
}

bool DemucsCppStemModelBackend::normaliseFile(
    const juce::File& file, std::atomic_bool& shouldCancel,
    juce::String& errorMessage) const
{
    StemAudioPreprocessor preprocessor;
    auto reader = preprocessor.open(file, errorMessage);
    if (reader == nullptr)
        return false;

    const auto info = reader->getInfo();
    juce::AudioBuffer<float> block;
    float peak = 0.0f;
    for (juce::int64 position = 0; position < info.lengthInSamples;)
    {
        if (isCancelled(shouldCancel))
        {
            errorMessage = "Separacion cancelada.";
            return false;
        }
        const int samples = (int) juce::jmin<juce::int64>(
            conversionBlockSize, info.lengthInSamples - position);
        if (!reader->readStereoBlock(position, samples, block, errorMessage))
            return false;
        peak = juce::jmax(peak, block.getMagnitude(0, block.getNumSamples()),
                          block.getMagnitude(1, block.getNumSamples()));
        position += samples;
    }

    if (peak <= 0.000001f)
        return true;

    reader = preprocessor.open(file, errorMessage);
    if (reader == nullptr)
        return false;
    const auto temporary = file.getSiblingFile(file.getFileNameWithoutExtension()
                                                + ".normalised.wav");
    auto writer = StemAudioPostprocessor::WavWriter::create(
        temporary, info.sampleRate, 2, errorMessage);
    if (writer == nullptr)
        return false;

    const float gain = 0.99f / peak;
    for (juce::int64 position = 0; position < info.lengthInSamples;)
    {
        if (isCancelled(shouldCancel))
        {
            errorMessage = "Separacion cancelada.";
            return false;
        }
        const int samples = (int) juce::jmin<juce::int64>(
            conversionBlockSize, info.lengthInSamples - position);
        if (!reader->readStereoBlock(position, samples, block, errorMessage))
            return false;
        StemAudioPostprocessor::applyGainAndLimit(block, gain);
        if (!writer->write(block, samples, errorMessage))
            return false;
        position += samples;
    }
    writer.reset();
    reader.reset();

    if (!temporary.replaceFileIn(file))
    {
        errorMessage = "No se pudo reemplazar un stem normalizado.";
        return false;
    }
    return true;
}

bool DemucsCppStemModelBackend::validateOutputs(
    const StemSeparationOptions& options, StemSeparationResult& result,
    juce::String& errorMessage) const
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    juce::int64 commonLength = -1;

    const auto requested = options.requestedStems.empty()
        ? standardSixStems() : options.requestedStems;
    for (const auto& output : demucsOutputs)
    {
        const auto file = options.outputDirectory.getChildFile(output.fileName);
        if (!file.existsAsFile() || file.getSize() <= 44)
        {
            errorMessage = "Demucs no genero " + juce::String(output.fileName) + ".";
            return false;
        }
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if (reader == nullptr || std::abs(reader->sampleRate - demucsSampleRate) > 0.5
            || reader->numChannels != 2 || reader->lengthInSamples <= 0)
        {
            errorMessage = "Salida Demucs invalida: " + file.getFileName() + ".";
            return false;
        }
        if (commonLength < 0)
            commonLength = reader->lengthInSamples;
        else if (reader->lengthInSamples != commonLength)
        {
            errorMessage = "Los stems de Demucs no tienen exactamente la misma longitud.";
            return false;
        }
        if (std::find(requested.begin(), requested.end(), output.type) != requested.end())
            result.stems.push_back({ output.type, stemTypeName(output.type), file });
    }
    return !result.stems.empty();
}

StemSeparationResult DemucsCppStemModelBackend::process(
    const StemSeparationOptions& options, ProgressReporter reportProgress,
    std::atomic_bool& shouldCancel)
{
    StemSeparationResult result;
    result.jobDirectory = options.outputDirectory;
    const auto cancel = [&result]()
    {
        result.cancelled = true;
        result.errorMessage = "Separacion cancelada.";
        return result;
    };
    const auto fail = [&result](const juce::String& message)
    {
        result.errorMessage = message;
        juce::Logger::writeToLog("Stem Demucs error: " + message);
        return result;
    };

    reportProgress(0.02f, "Preparando audio");
    juce::String error;
    const auto canonicalInput = options.outputDirectory.getChildFile("demucs_input.wav");
    if (!createCanonicalInput(options.inputFile, canonicalInput,
                              reportProgress, shouldCancel, error))
        return isCancelled(shouldCancel) ? cancel() : fail(error);

    if (isCancelled(shouldCancel))
        return cancel();

    juce::StringArray arguments;
    arguments.add(configuration.executable.getFullPathName());
    arguments.add(configuration.modelFile.getFullPathName());
    arguments.add(canonicalInput.getFullPathName());
    arguments.add(options.outputDirectory.getFullPathName());

    juce::ChildProcess process;
    reportProgress(0.22f, "Iniciando Demucs por CPU");
    // No se redirigen stdout/stderr: ChildProcess::readProcessOutput puede
    // bloquear si el CLI no escribe y demoraria la observacion de cancelacion.
    // El codigo de salida y los errores de contrato se registran igualmente.
    if (!process.start(arguments, 0))
        return fail("No se pudo iniciar el proceso demucs.cpp.");

    const double startedAt = juce::Time::getMillisecondCounterHiRes();
    while (process.isRunning())
    {
        if (isCancelled(shouldCancel))
        {
            process.kill();
            process.waitForProcessToFinish(2000);
            return cancel();
        }

        const int seconds = (int) ((juce::Time::getMillisecondCounterHiRes() - startedAt) / 1000.0);
        reportProgress(0.25f, "Separando por CPU (" + juce::String(seconds) + " s)");
        juce::Thread::sleep(100);
    }
    process.waitForProcessToFinish(1000);

    const auto exitCode = process.getExitCode();
    if (exitCode != 0)
        return fail("demucs.cpp termino con codigo " + juce::String(exitCode)
                    + ". Revisa la compatibilidad del ejecutable y los pesos.");

    reportProgress(0.88f, "Validando stems");
    if (!validateOutputs(options, result, error))
        return fail(error.isNotEmpty() ? error : "Demucs no produjo stems seleccionados.");

    if (options.normalizeOutput)
    {
        for (size_t index = 0; index < result.stems.size(); ++index)
        {
            reportProgress(0.90f + 0.07f * (float) index / (float) result.stems.size(),
                           "Normalizando " + result.stems[index].displayName);
            if (!normaliseFile(result.stems[index].outputFile, shouldCancel, error))
                return isCancelled(shouldCancel) ? cancel() : fail(error);
        }
    }

    if (!canonicalInput.deleteFile())
        juce::Logger::writeToLog("Stem Demucs: no se pudo borrar el WAV de entrada temporal.");
    result.success = true;
    for (const auto& stem : result.stems)
        juce::Logger::writeToLog("Stem Demucs output: " + stem.outputFile.getFullPathName());
    reportProgress(1.0f, "Finalizado (separacion real por CPU)");
    return result;
}
}
