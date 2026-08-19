#include "StemAudioPostprocessor.h"

namespace LoopernicoStems
{
StemAudioPostprocessor::WavWriter::WavWriter(
    std::unique_ptr<juce::AudioFormatWriter> outputWriter)
    : writer(std::move(outputWriter))
{
}

std::unique_ptr<StemAudioPostprocessor::WavWriter> StemAudioPostprocessor::WavWriter::create(
    const juce::File& file, double sampleRate, int channels, juce::String& errorMessage)
{
    auto stream = file.createOutputStream();
    if (stream == nullptr)
    {
        errorMessage = "No se pudo crear " + file.getFileName()
                     + ". Comprueba permisos y espacio disponible.";
        return {};
    }

    juce::WavAudioFormat wav;
    JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
    auto* rawStream = stream.get();
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(rawStream, sampleRate, (unsigned int) channels, 24, {}, 0));
    JUCE_END_IGNORE_DEPRECATION_WARNINGS
    if (writer == nullptr)
    {
        errorMessage = "No se pudo inicializar el escritor WAV para " + file.getFileName() + ".";
        return {};
    }

    stream.release();
    return std::unique_ptr<WavWriter>(new WavWriter(std::move(writer)));
}

bool StemAudioPostprocessor::WavWriter::write(const juce::AudioBuffer<float>& buffer,
                                              int samples, juce::String& errorMessage)
{
    if (writer == nullptr || samples < 0 || samples > buffer.getNumSamples())
    {
        errorMessage = "Bloque WAV invalido.";
        return false;
    }
    if (!writer->writeFromAudioSampleBuffer(buffer, 0, samples))
    {
        errorMessage = "Error al escribir WAV. Comprueba el espacio disponible en disco.";
        return false;
    }
    return true;
}

void StemAudioPostprocessor::applyGainAndLimit(juce::AudioBuffer<float>& block, float gain)
{
    block.applyGain(gain);
    for (int channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* samples = block.getWritePointer(channel);
        for (int i = 0; i < block.getNumSamples(); ++i)
            samples[i] = juce::jlimit(-1.0f, 1.0f, samples[i]);
    }
}

void StemAudioPostprocessor::applyJoinFades(juce::AudioBuffer<float>& block,
                                            bool fadeIn, bool fadeOut,
                                            int overlapSamples)
{
    const int fadeLength = juce::jlimit(0, block.getNumSamples(), overlapSamples);
    if (fadeLength <= 1)
        return;

    for (int channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* samples = block.getWritePointer(channel);
        for (int i = 0; i < fadeLength; ++i)
        {
            const float phase = (float) i / (float) (fadeLength - 1);
            if (fadeIn)
                samples[i] *= std::sin(phase * juce::MathConstants<float>::halfPi);
            if (fadeOut)
                samples[block.getNumSamples() - 1 - i]
                    *= std::sin(phase * juce::MathConstants<float>::halfPi);
        }
    }
}
}
