#pragma once

#include <JuceHeader.h>
#include <vector>

namespace LoopernicoStems
{
enum class StemType
{
    vocals,
    drums,
    bass,
    guitar,
    piano,
    other,
    acousticGuitar,
    electricGuitar,
    synthesizer,
    strings,
    wind,
    leadVocals,
    backingVocals,
    kick,
    snare,
    toms,
    cymbals,
    unknown
};

inline juce::String stemTypeName(StemType type)
{
    switch (type)
    {
        case StemType::vocals:         return "Voz";
        case StemType::drums:          return "Bateria";
        case StemType::bass:           return "Bajo";
        case StemType::guitar:         return "Guitarra";
        case StemType::piano:          return "Piano";
        case StemType::other:          return "Otros";
        case StemType::acousticGuitar: return "Guitarra acustica";
        case StemType::electricGuitar: return "Guitarra electrica";
        case StemType::synthesizer:    return "Sintetizadores";
        case StemType::strings:        return "Cuerdas";
        case StemType::wind:           return "Vientos";
        case StemType::leadVocals:     return "Voz principal";
        case StemType::backingVocals:  return "Coros";
        case StemType::kick:           return "Bombo";
        case StemType::snare:          return "Redoblante";
        case StemType::toms:           return "Toms";
        case StemType::cymbals:        return "Platos";
        case StemType::unknown:        break;
    }
    return "Desconocido";
}

inline juce::String stemTypeFileName(StemType type)
{
    switch (type)
    {
        case StemType::vocals:         return "vocals";
        case StemType::drums:          return "drums";
        case StemType::bass:           return "bass";
        case StemType::guitar:         return "guitar";
        case StemType::piano:          return "piano";
        case StemType::other:          return "other";
        case StemType::acousticGuitar: return "acoustic_guitar";
        case StemType::electricGuitar: return "electric_guitar";
        case StemType::synthesizer:    return "synthesizer";
        case StemType::strings:        return "strings";
        case StemType::wind:           return "wind";
        case StemType::leadVocals:     return "lead_vocals";
        case StemType::backingVocals:  return "backing_vocals";
        case StemType::kick:           return "kick";
        case StemType::snare:          return "snare";
        case StemType::toms:           return "toms";
        case StemType::cymbals:        return "cymbals";
        case StemType::unknown:        break;
    }
    return "unknown";
}

struct StemData
{
    StemType type = StemType::unknown;
    juce::String displayName;
    juce::File outputFile;
};

struct StemSeparationResult
{
    bool success = false;
    bool cancelled = false;
    juce::String errorMessage;
    juce::File jobDirectory;
    std::vector<StemData> stems;
};

enum class StemSeparationMode
{
    mock,
    demucsCppSixStem,
    onnxSixStem
};

struct StemSeparationOptions
{
    juce::File inputFile;
    std::vector<StemType> requestedStems;
    bool normalizeOutput = false;
    bool importAutomatically = true;
    StemSeparationMode mode = StemSeparationMode::mock;

    // Configuracion externa del backend demucs.cpp. Loopernico no distribuye
    // ni descarga automaticamente el ejecutable o los pesos.
    juce::File demucsCppExecutable;
    juce::File demucsCppModel;

    // Asignado por StemSeparationEngine. Los backends nunca comparten carpeta.
    juce::File outputDirectory;
};

inline std::vector<StemType> standardSixStems()
{
    return { StemType::vocals, StemType::drums, StemType::bass,
             StemType::guitar, StemType::piano, StemType::other };
}
}
