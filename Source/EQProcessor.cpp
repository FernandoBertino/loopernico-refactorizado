#include "EQProcessor.h"


EQProcessor::EQProcessor()
{
    for (int i = 0; i < kBands; ++i) {
        gains[i].store(0.0f);
        dirty[i].store(true); // forzar calculo inicial al primer bloque
    }
}

void EQProcessor::prepare(double sr)
{
    sampleRate = (sr > 0.0) ? sr : 44100.0;
    for (int i = 0; i < kBands; ++i) dirty[i].store(true);
    reset();
}

void EQProcessor::setGain(int band, float gainDb)
{
    if (band < 0 || band >= kBands) return;
    gains[band].store(gainDb);
    dirty[band].store(true);
}

float EQProcessor::getGain(int band) const
{
    if (band < 0 || band >= kBands) return 0.0f;
    return gains[band].load();
}

void EQProcessor::updateCoefficientsIfNeeded()
{
    for (int i = 0; i < kBands; ++i)
    {
        // exchange(false) lee el flag y lo pone en false atomicamente
        if (dirty[i].exchange(false))
        {
            float g = gains[i].load();
            filters[0][i].setPeakingEQ(sampleRate, getFrequency(i), 1.414f, g);
            filters[1][i].setPeakingEQ(sampleRate, getFrequency(i), 1.414f, g);
        }
    }
}

float EQProcessor::processSample(int channel, float in)
{
    if (!enabled.load()) return in;
    if (channel < 0 || channel >= 2) return in;

    float out = in;
    for (int b = 0; b < kBands; ++b)
        out = filters[channel][b].process(out);
    return out;
}

void EQProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
        for (int b = 0; b < kBands; ++b)
            filters[ch][b].clear();
}

// ==============================================================================
// Biquad
// ==============================================================================
void EQProcessor::Biquad::setPeakingEQ(double sr, float freq, float Q, float gainDb)
{
    float A     = std::pow(10.0f, gainDb / 40.0f);
    float omega = 2.0f * juce::MathConstants<float>::pi * freq / (float)sr;
    float alpha = std::sin(omega) / (2.0f * Q);
    float cosw  = std::cos(omega);
    float a0    = 1.0f + alpha / A;

    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosw)     / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cosw)     / a0;
    a2 = (1.0f - alpha / A) / a0;
}

float EQProcessor::Biquad::process(float in)
{
    float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = in;
    y2 = y1; y1 = out;
    return out;
}

void EQProcessor::Biquad::clear()
{
    x1 = x2 = y1 = y2 = 0.0f;
}
