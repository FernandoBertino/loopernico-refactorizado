#include "StemAudioPreprocessor.h"

namespace LoopernicoStems
{
StemAudioPreprocessor::Reader::Reader(std::unique_ptr<juce::AudioFormatReader> sourceReader,
                                     AudioInfo sourceInfo)
    : reader(std::move(sourceReader)), info(sourceInfo)
{
}

bool StemAudioPreprocessor::Reader::readStereoBlock(juce::int64 sourceSample, int samples,
                                                     juce::AudioBuffer<float>& destination,
                                                     juce::String& errorMessage)
{
    if (reader == nullptr || samples <= 0 || sourceSample < 0)
    {
        errorMessage = "Solicitud de lectura de audio invalida.";
        return false;
    }

    try
    {
        destination.setSize(2, samples, false, false, true);
        destination.clear();
        if (!reader->read(&destination, 0, samples, sourceSample, true, true))
        {
            errorMessage = "No se pudo leer el archivo de audio. Puede estar danado.";
            return false;
        }

        // AudioFormatReader duplica mono cuando fillLeftoverChannelsWithCopies=true.
        return true;
    }
    catch (const std::bad_alloc&)
    {
        errorMessage = "No hay memoria suficiente para leer el bloque de audio.";
    }
    catch (const std::exception& e)
    {
        errorMessage = "Error al leer audio: " + juce::String(e.what());
    }
    return false;
}

StemAudioPreprocessor::StemAudioPreprocessor()
{
    formatManager.registerBasicFormats();
}

bool StemAudioPreprocessor::isSupportedExtension(const juce::File& file)
{
    const auto extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".flac" || extension == ".mp3";
}

std::unique_ptr<StemAudioPreprocessor::Reader> StemAudioPreprocessor::open(
    const juce::File& inputFile, juce::String& errorMessage) const
{
    if (!inputFile.existsAsFile())
    {
        errorMessage = "El archivo seleccionado no existe.";
        return {};
    }
    if (!isSupportedExtension(inputFile))
    {
        errorMessage = "Formato no compatible. Selecciona un archivo WAV, FLAC o MP3.";
        return {};
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputFile));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0
        || reader->numChannels == 0)
    {
        errorMessage = "No se pudo abrir el audio. El archivo puede estar danado o usar un codec no disponible.";
        return {};
    }

    AudioInfo info;
    info.sampleRate = reader->sampleRate;
    info.sourceChannels = reader->numChannels;
    info.lengthInSamples = reader->lengthInSamples;
    info.durationSeconds = (double) reader->lengthInSamples / reader->sampleRate;
    return std::make_unique<Reader>(std::move(reader), info);
}
}
