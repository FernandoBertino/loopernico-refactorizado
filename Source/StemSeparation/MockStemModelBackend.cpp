#include "MockStemModelBackend.h"
#include "StemAudioPreprocessor.h"
#include "StemAudioPostprocessor.h"
#include <array>

namespace LoopernicoStems
{
std::vector<StemType> MockStemModelBackend::getSupportedStems() const
{
    return standardSixStems();
}

bool MockStemModelBackend::prepare(juce::String& errorMessage)
{
    errorMessage.clear();
    return true;
}

StemSeparationResult MockStemModelBackend::process(const StemSeparationOptions& options,
                                                   ProgressReporter reportProgress,
                                                   std::atomic_bool& shouldCancel)
{
    StemSeparationResult result;
    result.jobDirectory = options.outputDirectory;
    const auto fail = [&result](const juce::String& message)
    {
        result.errorMessage = message;
        juce::Logger::writeToLog("Stem mock error: " + message);
        return result;
    };
    const auto cancel = [&result]()
    {
        result.cancelled = true;
        result.errorMessage = "Separacion cancelada.";
        return result;
    };

    reportProgress(0.02f, "Preparando audio");
    StemAudioPreprocessor preprocessor;
    juce::String error;
    auto reader = preprocessor.open(options.inputFile, error);
    if (reader == nullptr)
        return fail(error);

    const auto info = reader->getInfo();
    juce::Logger::writeToLog("Stem mock input: " + options.inputFile.getFullPathName());
    juce::Logger::writeToLog("Stem mock audio: " + juce::String(info.sampleRate, 1)
                             + " Hz, " + juce::String(info.sourceChannels) + " canales, "
                             + juce::String(info.durationSeconds, 2) + " s");

    auto requested = options.requestedStems.empty() ? standardSixStems() : options.requestedStems;
    std::vector<StemType> stems;
    for (const auto type : standardSixStems())
        if (std::find(requested.begin(), requested.end(), type) != requested.end())
            stems.push_back(type);
    if (stems.empty())
        return fail("Selecciona al menos un stem compatible con el modo de prueba.");

    std::vector<std::unique_ptr<StemAudioPostprocessor::WavWriter>> writers;
    writers.reserve(stems.size());
    result.stems.reserve(stems.size());
    for (const auto type : stems)
    {
        const auto output = options.outputDirectory.getChildFile(stemTypeFileName(type) + ".wav");
        auto writer = StemAudioPostprocessor::WavWriter::create(output, info.sampleRate, 2, error);
        if (writer == nullptr)
            return fail(error);
        writers.push_back(std::move(writer));
        result.stems.push_back({ type, stemTypeName(type), output });
    }

    // Ganancias distintas permiten comprobar visual y auditivamente el flujo.
    // Este backend NO realiza separacion real: cada WAV contiene la mezcla original.
    const std::array<float, 6> mockGains { 0.95f, 0.82f, 0.70f, 0.58f, 0.46f, 0.34f };
    const auto supported = standardSixStems();
    juce::AudioBuffer<float> sourceBlock;
    juce::AudioBuffer<float> outputBlock;
    const auto totalSamples = info.lengthInSamples;
    const auto blockCount = juce::jmax<juce::int64>(1,
        (totalSamples + StemAudioPreprocessor::defaultBlockSize - 1)
            / StemAudioPreprocessor::defaultBlockSize);

    float normalisationGain = 1.0f;
    if (options.normalizeOutput)
    {
        float sourcePeak = 0.0f;
        for (juce::int64 position = 0; position < totalSamples;)
        {
            if (shouldCancel.load(std::memory_order_acquire))
                return cancel();
            const int samples = (int) juce::jmin<juce::int64>(
                StemAudioPreprocessor::defaultBlockSize, totalSamples - position);
            if (!reader->readStereoBlock(position, samples, sourceBlock, error))
                return fail(error);
            sourcePeak = juce::jmax(sourcePeak,
                sourceBlock.getMagnitude(0, sourceBlock.getNumSamples()));
            position += samples;
        }
        if (sourcePeak > 0.000001f)
            normalisationGain = 0.99f / sourcePeak;
    }

    reportProgress(0.06f, "Leyendo archivo");
    for (juce::int64 block = 0, position = 0; position < totalSamples; ++block)
    {
        if (shouldCancel.load(std::memory_order_acquire))
            return cancel();

        const int samples = (int) juce::jmin<juce::int64>(
            StemAudioPreprocessor::defaultBlockSize, totalSamples - position);
        if (!reader->readStereoBlock(position, samples, sourceBlock, error))
            return fail(error);

        for (size_t stemIndex = 0; stemIndex < writers.size(); ++stemIndex)
        {
            if (shouldCancel.load(std::memory_order_acquire))
                return cancel();
            outputBlock.makeCopyOf(sourceBlock, true);
            const auto found = std::find(supported.begin(), supported.end(), stems[stemIndex]);
            const auto gainIndex = found != supported.end()
                ? (size_t) std::distance(supported.begin(), found) : size_t{};
            float gain = mockGains[gainIndex];
            gain *= normalisationGain;
            StemAudioPostprocessor::applyGainAndLimit(outputBlock, gain);
            if (!writers[stemIndex]->write(outputBlock, samples, error))
                return fail(error);
        }

        position += samples;
        const float fraction = (float) (block + 1) / (float) blockCount;
        reportProgress(0.08f + fraction * 0.86f,
                       "Procesando bloque " + juce::String(block + 1)
                       + " de " + juce::String(blockCount));
        juce::Thread::sleep(8); // Solo simula latencia y deja visible la cancelacion.
    }

    writers.clear(); // Fuerza el cierre/flush de todos los WAV antes de informar exito.
    if (shouldCancel.load(std::memory_order_acquire))
        return cancel();

    reportProgress(0.97f, "Escribiendo archivos");
    for (const auto& stem : result.stems)
        if (!stem.outputFile.existsAsFile() || stem.outputFile.getSize() <= 44)
            return fail("No se pudo completar " + stem.outputFile.getFileName() + ".");

    result.success = true;
    for (const auto& stem : result.stems)
        juce::Logger::writeToLog("Stem mock output: " + stem.outputFile.getFullPathName());
    reportProgress(1.0f, "Finalizado (modo de prueba; no es separacion real)");
    return result;
}
}
