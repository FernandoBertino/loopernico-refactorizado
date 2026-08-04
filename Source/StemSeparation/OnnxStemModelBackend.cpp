#include "OnnxStemModelBackend.h"

#ifndef LOOPERNICO_ENABLE_ONNX
 #define LOOPERNICO_ENABLE_ONNX 0
#endif

namespace LoopernicoStems
{
OnnxStemModelBackend::OnnxStemModelBackend(ModelMetadata metadataToUse)
    : metadata(std::move(metadataToUse))
{
}

std::vector<StemType> OnnxStemModelBackend::getSupportedStems() const
{
    return metadata.outputStemOrder;
}

bool OnnxStemModelBackend::prepare(juce::String& errorMessage)
{
#if LOOPERNICO_ENABLE_ONNX
    if (!metadata.modelFile.existsAsFile())
    {
        errorMessage = "No se encontro el modelo ONNX configurado.";
        return false;
    }
    if (metadata.requiredSampleRate <= 0.0 || metadata.requiredChannels <= 0
        || metadata.blockLength <= 0 || metadata.inputName.isEmpty()
        || metadata.outputNames.isEmpty() || metadata.outputStemOrder.empty())
    {
        errorMessage = "Los metadatos del modelo ONNX estan incompletos o son incompatibles.";
        return false;
    }

    // La creacion de Ort::Env, SessionOptions, proveedor y Session se agregara
    // aqui cuando se seleccione un modelo concreto. No se infieren tensores.
    errorMessage = "ONNX Runtime esta habilitado, pero todavia no se implemento un adaptador para un modelo concreto.";
    return false;
#else
    errorMessage = "ONNX Runtime no fue habilitado en esta compilacion. Usa el modo de prueba o configura LOOPERNICO_ENABLE_ONNX.";
    return false;
#endif
}

StemSeparationResult OnnxStemModelBackend::process(const StemSeparationOptions&,
                                                   ProgressReporter,
                                                   std::atomic_bool&)
{
    StemSeparationResult result;
    result.errorMessage = "No hay un adaptador de modelo ONNX configurado.";
    return result;
}
}
