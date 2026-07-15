#pragma once
#include <JuceHeader.h>
#include <atomic>

// ==============================================================================
// EQProcessor — EQ paramétrico de 10 bandas, instanciable por track.
//
// Antes: habia un solo array Biquad[2][10] global en MainComponent.
// Ahora: cada LoopTrack tiene su propio EQProcessor, y el Input EQ
//        (aplicado antes de grabar) es otra instancia separada.
//
// Thread safety:
//   - setGain() / prepare() se llaman desde el message thread.
//   - processSample() se llama desde el audio thread.
//   - Los flags eqDirty[] garantizan que los coeficientes solo se
//     recalculan al inicio del bloque, nunca muestra a muestra.
// ==============================================================================
class EQProcessor
{
public:
    static constexpr int kBands = 10;

    // MSVC no permite constexpr float[] sin inicializador en la declaracion.
    // Usamos una funcion inline para exponer las frecuencias de forma segura.
    static float getFrequency(int band)
    {
        static const float freqs[kBands] = {
            31.25f, 62.5f, 125.0f, 250.0f, 500.0f,
            1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
        };
        return (band >= 0 && band < kBands) ? freqs[band] : 0.0f;
    }

    EQProcessor();

    // Llamar desde prepareToPlay()
    void prepare(double sampleRate);

    // Llamar desde el message thread cuando el usuario mueve un slider
    void setGain(int band, float gainDb);
    float getGain(int band) const;

    // Llamar al inicio de cada bloque de audio (audio thread)
    // Recalcula solo las bandas marcadas como dirty
    void updateCoefficientsIfNeeded();

    // Procesar una muestra en un canal especifico (audio thread)
    float processSample(int channel, float in);

    // Resetear estado interno (limpiar historiales del filtro)
    void reset();

    // Bypass total
    void setEnabled(bool e) { enabled.store(e); }
    bool isEnabled() const  { return enabled.load(); }

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        void setPeakingEQ(double sr, float freq, float Q, float gainDb);
        float process(float in);
        void  clear();
    };

    Biquad           filters[2][kBands];
    std::atomic<float> gains[kBands];
    std::atomic<bool>  dirty[kBands];
    std::atomic<bool>  enabled { true };
    double             sampleRate = 44100.0;
};
