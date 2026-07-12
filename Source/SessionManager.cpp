#include "SessionManager.h"

#include <cmath>
#include <limits>

namespace
{
    constexpr int kSessionMagic = 0x4c4e5331; // LNS1
    constexpr int kMaxSessionXmlBytes = 4 * 1024 * 1024;
    constexpr int kMaxVstStateBytes = 64 * 1024 * 1024;
    constexpr double kMinSessionSampleRate = 8000.0;
    constexpr double kMaxSessionSampleRate = 192000.0;

    bool writeAudioBufferToStream(juce::OutputStream& stream, const juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        stream.writeCompressedInt(numChannels);
        stream.writeCompressedInt(numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto bytes = (size_t) numSamples * sizeof(float);
            if (bytes > 0 && !stream.write(buffer.getReadPointer(ch), bytes))
                return false;
        }

        return true;
    }

    bool readAudioBufferFromStream(juce::InputStream& stream,
                                   juce::AudioBuffer<float>& buffer,
                                   int maxSamples,
                                   juce::String& errorMessage,
                                   int trackNumber)
    {
        if (stream.isExhausted())
        {
            errorMessage = "Falta el audio de la pista " + juce::String(trackNumber) + ".";
            return false;
        }

        const int numChannels = stream.readCompressedInt();
        const int numSamples = stream.readCompressedInt();

        if (numChannels < 0 || numChannels > 2)
        {
            errorMessage = "La pista " + juce::String(trackNumber)
                         + " declara una cantidad de canales no valida.";
            return false;
        }

        if (numSamples < 0 || numSamples > maxSamples || (numChannels == 0 && numSamples != 0))
        {
            errorMessage = "La pista " + juce::String(trackNumber)
                         + " supera la duracion maxima permitida.";
            return false;
        }

        const auto bytes = (size_t) numSamples * sizeof(float);
        if (numSamples > 0 && bytes / sizeof(float) != (size_t) numSamples)
        {
            errorMessage = "El tamaño de audio de la pista " + juce::String(trackNumber)
                         + " no es valido.";
            return false;
        }

        buffer.setSize(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (bytes > 0 && stream.read(buffer.getWritePointer(ch), bytes) != (int) bytes)
            {
                errorMessage = "El audio de la pista " + juce::String(trackNumber)
                             + " esta truncado.";
                return false;
            }
        }

        return true;
    }
}

bool SessionManager::writeSessionFile(const juce::File& file,
                                      const juce::ValueTree& sessionTree,
                                      const LooperEngine::SessionSnapshot& looperSnapshot,
                                      const juce::MemoryBlock& vstState,
                                      juce::String& errorMessage)
{
    auto xml = sessionTree.createXml();
    if (xml == nullptr)
    {
        errorMessage = "No se pudo crear la metadata de la sesion.";
        return false;
    }

    juce::FileOutputStream fileStream(file);
    if (!fileStream.openedOk())
    {
        errorMessage = "No se pudo abrir el archivo destino.";
        return false;
    }

    juce::GZIPCompressorOutputStream gzipStream(&fileStream, 9, false);
    const juce::String xmlText = xml->toString();
    const juce::MemoryBlock xmlBytes(xmlText.toRawUTF8(), xmlText.getNumBytesAsUTF8());

    gzipStream.writeInt(kSessionMagic);
    gzipStream.writeInt(SessionManager::currentVersion);
    gzipStream.writeCompressedInt((int) xmlBytes.getSize());
    if (!gzipStream.write(xmlBytes.getData(), xmlBytes.getSize()))
    {
        errorMessage = "No se pudo escribir la cabecera de la sesion.";
        return false;
    }

    for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
        if (!writeAudioBufferToStream(gzipStream, looperSnapshot.tracks[(size_t) i].audio))
        {
            errorMessage = "No se pudo escribir el audio de la sesion.";
            return false;
        }

    gzipStream.writeCompressedInt((int) vstState.getSize());

    if (vstState.getSize() > 0 && !gzipStream.write(vstState.getData(), vstState.getSize()))
    {
        errorMessage = "No se pudo escribir el estado interno del VST3.";
        return false;
    }

    gzipStream.flush();
    return true;
}

bool SessionManager::readSessionFile(const juce::File& file,
                                     juce::ValueTree& sessionTree,
                                     LooperEngine::SessionSnapshot& looperSnapshot,
                                     juce::MemoryBlock& vstState,
                                     juce::String& errorMessage)
{
    juce::FileInputStream fileStream(file);
    if (!fileStream.openedOk())
    {
        errorMessage = "No se pudo abrir el archivo de sesion.";
        return false;
    }

    juce::GZIPDecompressorInputStream gzipStream(&fileStream, false);
    if (gzipStream.readInt() != kSessionMagic)
    {
        errorMessage = "El archivo no es una sesion valida de Loopernico.";
        return false;
    }

    const int version = gzipStream.readInt();
    if (version < 1 || version > SessionManager::currentVersion)
    {
        errorMessage = "La sesion fue creada con una version mas nueva del programa.";
        return false;
    }

    const int xmlSize = gzipStream.readCompressedInt();
    if (xmlSize <= 0 || xmlSize > kMaxSessionXmlBytes)
    {
        errorMessage = "La metadata de la sesion esta dañada.";
        return false;
    }

    juce::MemoryBlock xmlBytes((size_t) xmlSize);
    if (gzipStream.read(xmlBytes.getData(), xmlSize) != xmlSize)
    {
        errorMessage = "No se pudo leer la metadata de la sesion.";
        return false;
    }

    auto xml = juce::parseXML(juce::String::fromUTF8((const char*) xmlBytes.getData(), xmlSize));
    if (xml == nullptr)
    {
        errorMessage = "La metadata XML de la sesion no es valida.";
        return false;
    }

    sessionTree = juce::ValueTree::fromXml(*xml);
    if (!sessionTree.isValid() || !sessionTree.hasType("LOOPERNICO_SESSION"))
    {
        errorMessage = "No se pudo reconstruir la sesion guardada.";
        return false;
    }

    const auto looperNode = sessionTree.getChildWithName("LOOPER");
    if (!looperNode.isValid() || looperNode.getNumChildren() != LooperEngine::kMaxTracks)
    {
        errorMessage = "La sesion no contiene la estructura de pistas esperada.";
        return false;
    }

    const int storedState = (int) looperNode.getProperty("state", (int) LooperEngine::State::Stopped);
    if (storedState < (int) LooperEngine::State::Stopped
        || storedState > (int) LooperEngine::State::Playing)
    {
        errorMessage = "El estado del looper almacenado no es valido.";
        return false;
    }
    looperSnapshot.state = (LooperEngine::State) storedState;
    looperSnapshot.totalTracks = (int) looperNode.getProperty("totalTracks", 0);
    looperSnapshot.baseLoopLengthSamples = (int) looperNode.getProperty("baseLoopLengthSamples", 0);
    looperSnapshot.loopReadPosition = (int) looperNode.getProperty("loopReadPosition", 0);
    looperSnapshot.recordWritePosition = (int) looperNode.getProperty("recordWritePosition", 0);
    looperSnapshot.recordStartPosition = (int) looperNode.getProperty("recordStartPosition", 0);
    looperSnapshot.editableLoopSourceSamples = (int) looperNode.getProperty("editableLoopSourceSamples", 0);
    looperSnapshot.globalMute = (bool) looperNode.getProperty("globalMute", false);
    looperSnapshot.globalPaused = (bool) looperNode.getProperty("globalPaused", false);
    looperSnapshot.silenceThresholdDb = (float) (double) looperNode.getProperty("silenceThresholdDb", -60.0);
    looperSnapshot.bpm = (float) (double) looperNode.getProperty("bpm", 120.0);
    looperSnapshot.beatsPerBar = (int) looperNode.getProperty("beatsPerBar", 4);
    looperSnapshot.barsPerLoop = (int) looperNode.getProperty("barsPerLoop", 0);
    looperSnapshot.quantizeEnabled = (bool) looperNode.getProperty("quantizeEnabled", true);
    looperSnapshot.redoCount = (int) looperNode.getProperty("redoCount", 0);
    looperSnapshot.sampleRate = (double) looperNode.getProperty("sampleRate", 44100.0);

    if (!std::isfinite(looperSnapshot.sampleRate)
        || looperSnapshot.sampleRate < kMinSessionSampleRate
        || looperSnapshot.sampleRate > kMaxSessionSampleRate)
    {
        errorMessage = "La frecuencia de muestreo de la sesion no es valida.";
        return false;
    }

    const double maxSamplesAsDouble = std::ceil(looperSnapshot.sampleRate
                                                 * (double) LooperEngine::kMaxLoopSecs);
    if (maxSamplesAsDouble > (double) std::numeric_limits<int>::max())
    {
        errorMessage = "La duracion de audio declarada por la sesion no es valida.";
        return false;
    }
    const int maxTrackSamples = (int) maxSamplesAsDouble;

    if (looperSnapshot.totalTracks < 0 || looperSnapshot.totalTracks > LooperEngine::kMaxTracks
        || looperSnapshot.baseLoopLengthSamples < 0 || looperSnapshot.baseLoopLengthSamples > maxTrackSamples
        || looperSnapshot.editableLoopSourceSamples < 0 || looperSnapshot.editableLoopSourceSamples > maxTrackSamples
        || looperSnapshot.loopReadPosition < 0 || looperSnapshot.loopReadPosition > maxTrackSamples
        || looperSnapshot.recordWritePosition < 0 || looperSnapshot.recordWritePosition > maxTrackSamples
        || looperSnapshot.recordStartPosition < 0 || looperSnapshot.recordStartPosition > maxTrackSamples
        || looperSnapshot.redoCount < 0 || looperSnapshot.redoCount > LooperEngine::kMaxTracks)
    {
        errorMessage = "La metadata del looper contiene longitudes o cantidades no validas.";
        return false;
    }

    for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
    {
        const auto trackNode = looperNode.getChild(i);
        if (!trackNode.hasType("TRACK") || (int) trackNode.getProperty("index", i) != i)
        {
            errorMessage = "La metadata de la pista " + juce::String(i + 1) + " no es valida.";
            return false;
        }
        auto& track = looperSnapshot.tracks[(size_t) i];
        track.lengthSamples = (int) trackNode.getProperty("lengthSamples", 0);
        track.redoLengthSamples = (int) trackNode.getProperty("redoLengthSamples", 0);
        track.volume = (float) (double) trackNode.getProperty("volume", 1.0);
        track.isMuted = (bool) trackNode.getProperty("isMuted", false);
        track.isPaused = (bool) trackNode.getProperty("isPaused", false);
        track.pausedReadPos = (int) trackNode.getProperty("pausedReadPos", 0);
        track.frippDecay = (float) (double) trackNode.getProperty("frippDecay", 1.0);
        track.frippEnabled = (bool) trackNode.getProperty("frippEnabled", false);
        track.currentGain = (float) (double) trackNode.getProperty("currentGain", 1.0);

        if (!readAudioBufferFromStream(gzipStream, track.audio, maxTrackSamples,
                                       errorMessage, i + 1))
            return false;

        const int storedAudioSamples = (int) trackNode.getProperty("audioSamples",
                                                                   track.audio.getNumSamples());
        if (storedAudioSamples != track.audio.getNumSamples()
            || track.lengthSamples < 0 || track.lengthSamples > track.audio.getNumSamples()
            || track.redoLengthSamples < 0 || track.redoLengthSamples > track.audio.getNumSamples()
            || track.pausedReadPos < 0
            || (track.audio.getNumSamples() == 0
                ? track.pausedReadPos != 0
                : track.pausedReadPos >= track.audio.getNumSamples()))
        {
            errorMessage = "Las longitudes de la pista " + juce::String(i + 1)
                         + " no coinciden con su audio.";
            return false;
        }
    }

    if (gzipStream.isExhausted())
    {
        errorMessage = "Falta el estado del VST3 en el archivo de sesion.";
        return false;
    }

    const int vstStateSize = gzipStream.readCompressedInt();
    if (vstStateSize < 0 || vstStateSize > kMaxVstStateBytes)
    {
        errorMessage = "El estado del VST3 esta dañado.";
        return false;
    }

    vstState.setSize((size_t) vstStateSize);
    if (vstStateSize > 0 && gzipStream.read(vstState.getData(), vstStateSize) != vstStateSize)
    {
        errorMessage = "No se pudo leer el estado interno del VST3.";
        return false;
    }

    return true;
}

