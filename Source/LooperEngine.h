#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include "LatencyCompensation.h"

// ==============================================================================
// LooperEngine — Motor central del looper.
//
// Responsabilidades:
//   - Estado de grabacion/reproduccion (Stopped, RecordingBase, RecordingLayer, Playing)
//   - Buffers de audio de cada track (hasta kMaxTracks)
//   - Tempo: BPM, firma de tiempo, compases por loop, cuantizacion, tap tempo
//   - Fripp infinite decay por track.
//
// Los efectos (FX Pedalboard) viven en MainComponent::effectsChain y se aplican
// ANTES de que la señal entre al looper. El looper graba y reproduce señal pura,
// sin cadenas de efectos internas. La entrada procesada se recibe separada del
// bus final para poder grabarla aunque el monitoreo por software este apagado.
//
// Thread safety:
//   - Todos los metodos publicos que cambian estado son seguros desde el
//     message thread. El audio thread SOLO llama a processBlock().
//   - Los parametros compartidos usan std::atomic.
// ==============================================================================
class LooperEngine
{
public:
    static constexpr int   kMaxTracks  = 12;
    static constexpr float kMaxLoopSecs = 600.0f;
    static constexpr int   kMaxTaps    = 8;
    static constexpr double kTapTimeout = 3.0;

    enum class State { Stopped, ArmedBase, RecordingBase, RecordingLayer, Playing };

    struct TrackSnapshot
    {
        juce::AudioBuffer<float> audio;
        int lengthSamples{ 0 };
        int redoLengthSamples{ 0 };
        float volume{ 1.0f };
        bool isMuted{ false };
        bool isPaused{ false };
        int pausedReadPos{ 0 };
        float frippDecay{ 1.0f };
        bool frippEnabled{ false };
        float currentGain{ 1.0f };
    };

    struct SessionSnapshot
    {
        State state{ State::Stopped };
        int totalTracks{ 0 };
        int baseLoopLengthSamples{ 0 };
        int loopReadPosition{ 0 };
        int recordWritePosition{ 0 };
        int recordStartPosition{ 0 };
        int editableLoopSourceSamples{ 0 };
        bool globalMute{ false };
        bool globalPaused{ false };
        float silenceThresholdDb{ -60.0f };
        float bpm{ 120.0f };
        int beatsPerBar{ 4 };
        int barsPerLoop{ 0 };
        bool quantizeEnabled{ true };
        int redoCount{ 0 };
        double sampleRate{ 44100.0 };
        std::array<TrackSnapshot, kMaxTracks> tracks;
    };

    // Incremental snapshot capture.  The owner must hold the same callback
    // lock used around processBlock() while calling begin/copy.  Allocation is
    // deliberately performed by prepareIncrementalSnapshot() without that
    // lock.  Captures are rejected while recording; buffer revisions detect
    // any loop-memory mutation between chunks and force a coherent retry.
    enum class SnapshotCaptureResult { inProgress, complete, retryRequired };

    struct IncrementalSnapshot
    {
        SessionSnapshot snapshot;
        std::array<int, kMaxTracks> audioLengths {};
        std::array<uint64_t, kMaxTracks> bufferRevisions {};
        int nextTrack{ 0 };
        int nextSample{ 0 };
        bool prepared{ false };
    };

    // A session restore is deliberately split in two phases.  Building this
    // object may allocate and resample, so it must happen without the audio
    // callback lock.  commitPreparedSessionRestore only swaps buffers and
    // publishes scalar state; after the commit this object owns the old engine
    // buffers, allowing their destruction to happen after the lock is released.
    struct PreparedTrackRestore
    {
        juce::AudioBuffer<float> audio;
        int lengthSamples{ 0 };
        int redoLengthSamples{ 0 };
        float volume{ 1.0f };
        bool isMuted{ false };
        bool isPaused{ false };
        int pausedReadPos{ 0 };
        float frippDecay{ 1.0f };
        bool frippEnabled{ false };
        float currentGain{ 1.0f };
    };

    struct PreparedSessionRestore
    {
        State state{ State::Stopped };
        int totalTracks{ 0 };
        int baseLoopLengthSamples{ 0 };
        int loopReadPosition{ 0 };
        int recordWritePosition{ 0 };
        int recordStartPosition{ 0 };
        int editableLoopSourceSamples{ 0 };
        bool globalMute{ false };
        bool globalPaused{ false };
        float silenceThresholdDb{ -60.0f };
        float bpm{ 120.0f };
        int beatsPerBar{ 4 };
        int barsPerLoop{ 0 };
        bool quantizeEnabled{ true };
        int redoCount{ 0 };
        int maxLoopLengthSamples{ 0 };
        double targetSampleRate{ 0.0 };
        std::array<PreparedTrackRestore, kMaxTracks> tracks;
    };

    struct PreparedTrackImport
    {
        juce::AudioBuffer<float> audio;
        juce::String displayName;
        juce::File sourceFile;
    };

    struct PreparedTrackBatchImport
    {
        int expectedTotalTracks{ 0 };
        int lengthSamples{ 0 };
        double targetSampleRate{ 0.0 };
        std::vector<PreparedTrackImport> tracks;
    };

    struct TrackBatchImportResult
    {
        bool success{ false };
        int firstTrack{ -1 };
        int importedTracks{ 0 };
        juce::String errorMessage;
    };

    LooperEngine();
    ~LooperEngine() = default;

    // ---- Ciclo de vida (message thread) -------------------------------------
    void prepare(double sampleRate, int maxBlockSize,
                 juce::AudioFormatManager& fmt);
    void releaseResources();

    // ---- Procesamiento (audio thread) ---------------------------------------
    void processBlock(const juce::AudioBuffer<float>& recordableInput,
                      int inputStartSample,
                      juce::AudioBuffer<float>& output,
                      int outputStartSample,
                      int numSamples,
                      bool monitorInput,
                      bool forceDualMonoInput = false);

    // ---- Control de grabacion (message thread) ------------------------------
    void recConfirm();
    void cancelRec();
    void undoLastTrack();
    void redoLastTrack();
    void stopPlay();
    void goToStart();
    void resetAll();

    // ---- Getters de estado (seguros desde cualquier thread) -----------------
    State getState()           const { return currentState.load(); }
    int   getTotalTracks()     const { return totalTracks.load(); }
    int   getBaseLoopSamples() const { return baseLoopLengthSamples.load(); }
    int   getReadPosition()    const { return loopReadPosition.load(); }
    int   getRecordWritePosition() const { return recordWritePosition.load(); }
    float getSampleRate()      const { return (float)sampleRate_; }
    bool  canRedo()            const { return redoCount > 0; }

    void setRecordingCompensationSamples(int samples)
    {
        recordingCompensationSamples.store(samples, std::memory_order_relaxed);
    }
    int getRecordingCompensationSamples() const
    {
        return recordingCompensationSamples.load(std::memory_order_relaxed);
    }
    void setBaseRecordingUsesTimingReference(bool shouldCompensate)
    {
        baseRecordingUsesTimingReference.store(shouldCompensate, std::memory_order_relaxed);
    }
    bool consumeFixedLoopCompletionNotification()
    {
        return fixedLoopCompletionPending.exchange(false, std::memory_order_acq_rel);
    }

    std::function<void()> onFixedLoopComplete;

    // ---- Tempo (message thread) ---------------------------------------------
    void tapTempo();
    void setBpm(float bpm);
    void setBeatsPerBar(int beats);
    void setBarsPerLoop(int bars);
    void setQuantizeEnabled(bool e) { quantizeEnabled.store(e); }

    float getBpm()         const { return bpmParam.load(); }
    int   getBeatsPerBar() const { return beatsPerBar.load(); }
    int   getBarsPerLoop() const { return barsPerLoop.load(); }
    bool  isQuantized()    const { return quantizeEnabled.load(); }

    // ---- Mixer por track (message thread) -----------------------------------
    void  setTrackVolume(int track, float vol);
    void  setTrackMuted(int track, bool muted);
    float getTrackVolume(int track) const;
    bool  isTrackMuted(int track)   const;
    void  setGlobalMute(bool m)     { globalMute.store(m); }
    bool  isGlobalMuted()           const { return globalMute.load(); }

    // ---- Pausa por track y global (message thread) --------------------------
    void pauseTrack (int track);
    void resumeTrack(int track);
    void togglePauseTrack(int track);
    bool isTrackPaused(int track) const;

    void pauseAll();
    void resumeAll();
    void togglePauseAll();
    bool isAnyTrackPaused() const;
    bool isGlobalPaused()   const { return globalPaused.load(); }

    // ---- Fripp infinite decay (message thread) ------------------------------
    void  setFrippDecay(int track, float factor);
    void  setFrippEnabled(int track, bool enabled);
    float getFrippDecay(int track) const;
    bool  isFrippEnabled(int track) const;

    // ---- Import / export (message thread) -----------------------------------
    void importAudioToBase(juce::AudioFormatManager& fmt, const juce::File& file);
    TrackBatchImportResult commitPreparedTrackBatchImport(PreparedTrackBatchImport&& prepared);
    void exportFullMix(juce::AudioFormatManager& fmt, const juce::File& dest,
                       bool outputMono = false);
    void exportSeparateTracks(juce::AudioFormatManager& fmt, const juce::File& folder,
                              bool outputMono = false);
    static void exportFullMix(const SessionSnapshot& snapshot, const juce::File& dest,
                              bool outputMono = false);
    static void exportSeparateTracks(const SessionSnapshot& snapshot, const juce::File& folder,
                                     bool outputMono = false);

    // ---- Edicion rapida del loop base (message thread) ----------------------
    bool nudgeLoopStart(int deltaSamples);
    bool adjustLoopTail(int deltaSamples);
    int  getEditableLoopSourceSamples() const { return editableLoopSourceSamples.load(); }

    // ---- Waveform display (message thread) ----------------------------------
    juce::AudioThumbnail* getTrackThumbnail(int track);
    void refreshThumbnail(int track, juce::AudioFormatManager& fmt);
    SessionSnapshot createSessionSnapshot() const;
    SnapshotCaptureResult beginIncrementalSnapshot(IncrementalSnapshot& capture) const;
    static void prepareIncrementalSnapshot(IncrementalSnapshot& capture);
    SnapshotCaptureResult copyIncrementalSnapshotChunk(IncrementalSnapshot& capture,
                                                       int maxSamplesPerChannel) const;
    PreparedSessionRestore prepareSessionRestore(const SessionSnapshot& snapshot) const;
    bool commitPreparedSessionRestore(PreparedSessionRestore&& prepared);
    void restoreSessionSnapshot(const SessionSnapshot& snapshot);

    // ---- Trim de silencio al confirmar grabacion (message thread) -----------
    // Umbral en dBFS bajo el cual una muestra se considera silencio.
    // Valor tipico: -60 dBFS. Rango util: -80 a 0.
    void  setSilenceThresholdDb(float db) { silenceThresholdDb.store(db); }
    float getSilenceThresholdDb() const   { return silenceThresholdDb.load(); }

private:
    // ---- Track interno -------------------------------------------------------
    struct LoopTrack {
        juce::AudioBuffer<float> buffer;
        std::atomic<int>   lengthSamples { 0     };
        std::atomic<int>   redoLengthSamples { 0 }; // longitud guardada al hacer undo, para poder hacer redo
        std::atomic<float> volume        { 1.0f  };
        std::atomic<bool>  isMuted       { false };
        std::atomic<bool>  isPaused      { false };
        std::atomic<int>   pausedReadPos { 0     };
        std::atomic<float> frippDecay    { 1.0f  };
        std::atomic<bool>  frippEnabled  { false };
        std::atomic<float> currentGain   { 1.0f };
        std::atomic<uint64_t> bufferRevision { 0 };
    };

    // ---- Helpers de tempo y trim (message thread) ---------------------------
    void recalcLoopLength();
    int  quantizeToNearestBar(int recordedSamples) const;
    void ensureTrackBufferCapacity(int track, int samples);

    // Detecta el primer y ultimo sample con contenido sobre el umbral,
    // copia la region activa al inicio del buffer y devuelve su longitud.
    // Si toda la grabacion es silencio devuelve recordedSamples sin tocar nada.
    int  trimSilence(int track, int recordedSamples);
    void applyLoopSeamCrossfade(int activeLengthSamples);
    void rotateTrackRegion(int track, int regionLengthSamples, int deltaSamples);

    // ---- Estado (atomico, shared entre threads) -----------------------------
    std::atomic<State> currentState          { State::Stopped };
    std::atomic<int>   totalTracks           { 0 };
    std::atomic<int>   baseLoopLengthSamples { 0 };
    std::atomic<int>   loopReadPosition      { 0 };
    std::atomic<int>   recordWritePosition   { 0 };
    std::atomic<int>   recordStartPosition   { 0 };
    std::atomic<int>   editableLoopSourceSamples { 0 };
    std::atomic<bool>  fixedLoopCompletionRequested { false };
    std::atomic<bool>  fixedLoopCompletionPending { false };
    std::atomic<int>   recordingCompensationSamples { 0 };
    std::atomic<bool>  baseRecordingUsesTimingReference { false };
    std::atomic<bool>  globalMute            { false };
    std::atomic<bool>  globalPaused          { false };
    std::atomic<float> silenceThresholdDb    { -60.0f }; // umbral para trimSilence

    // ---- Tempo --------------------------------------------------------------
    std::atomic<float> bpmParam        { 120.0f };
    std::atomic<int>   beatsPerBar     { 4      };
    std::atomic<int>   barsPerLoop     { 0      };
    std::atomic<bool>  quantizeEnabled { true   };

    std::array<double, kMaxTaps> tapTimes {};
    int tapCount  = 0;
    int redoCount = 0; // cuantas operaciones de undo se pueden rehacer (message thread only)

    // ---- Tracks -------------------------------------------------------------
    LoopTrack tracks[kMaxTracks];
    int       maxLoopLengthSamples = 0;

    // ---- Waveform thumbnails ------------------------------------------------
    juce::AudioFormatManager*              formatManager_ = nullptr;
    juce::AudioThumbnailCache              thumbnailCache { kMaxTracks + 1 };
    std::unique_ptr<juce::AudioThumbnail>  thumbnails[kMaxTracks];

    double sampleRate_    = 44100.0;
    int    maxBlockSize_  = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperEngine)
};
