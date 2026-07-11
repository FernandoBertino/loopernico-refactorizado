#include "LooperEngine.h"

LooperEngine::LooperEngine()
{
    tapTimes.fill(0.0);
}

// ==============================================================================
// CICLO DE VIDA
// ==============================================================================
void LooperEngine::prepare(double sr, int maxBlockSize,
    juce::AudioFormatManager& fmt)
{
    formatManager_ = &fmt;
    sampleRate_ = (sr > 0.0) ? sr : 44100.0;
    maxBlockSize_ = maxBlockSize;

    maxLoopLengthSamples = (int)(sampleRate_ * kMaxLoopSecs);

    for (int i = 0; i < kMaxTracks; ++i) {
        tracks[i].buffer.setSize(2, maxLoopLengthSamples);
        tracks[i].buffer.clear();
        tracks[i].currentGain = 1.0f;
        thumbnails[i] = std::make_unique<juce::AudioThumbnail>(512, fmt, thumbnailCache);
    }

    recalcLoopLength();
}

void LooperEngine::releaseResources()
{
    for (int i = 0; i < kMaxTracks; ++i)
        tracks[i].buffer.setSize(0, 0);
}

LooperEngine::SessionSnapshot LooperEngine::createSessionSnapshot() const
{
    SessionSnapshot snapshot;
    snapshot.state = currentState.load();
    snapshot.totalTracks = totalTracks.load();
    snapshot.baseLoopLengthSamples = baseLoopLengthSamples.load();
    snapshot.loopReadPosition = loopReadPosition.load();
    snapshot.recordWritePosition = recordWritePosition.load();
    snapshot.recordStartPosition = recordStartPosition.load();
    snapshot.editableLoopSourceSamples = editableLoopSourceSamples.load();
    snapshot.globalMute = globalMute.load();
    snapshot.globalPaused = globalPaused.load();
    snapshot.silenceThresholdDb = silenceThresholdDb.load();
    snapshot.bpm = bpmParam.load();
    snapshot.beatsPerBar = beatsPerBar.load();
    snapshot.barsPerLoop = barsPerLoop.load();
    snapshot.quantizeEnabled = quantizeEnabled.load();
    snapshot.redoCount = redoCount;
    snapshot.sampleRate = sampleRate_;

    for (int i = 0; i < kMaxTracks; ++i)
    {
        auto& track = snapshot.tracks[(size_t) i];
        track.lengthSamples = tracks[i].lengthSamples.load();
        track.redoLengthSamples = tracks[i].redoLengthSamples.load();
        track.volume = tracks[i].volume.load();
        track.isMuted = tracks[i].isMuted.load();
        track.isPaused = tracks[i].isPaused.load();
        track.pausedReadPos = tracks[i].pausedReadPos.load();
        track.frippDecay = tracks[i].frippDecay.load();
        track.frippEnabled = tracks[i].frippEnabled.load();
        track.currentGain = tracks[i].currentGain;

        int audioLength = juce::jmax(track.lengthSamples, track.redoLengthSamples);
        if (i < snapshot.totalTracks)
            audioLength = juce::jmax(audioLength,
                juce::jmax(snapshot.baseLoopLengthSamples, snapshot.editableLoopSourceSamples));
        if (snapshot.state == State::RecordingBase && i == 0)
            audioLength = juce::jmax(audioLength, snapshot.recordWritePosition);
        if (snapshot.state == State::RecordingLayer && i == snapshot.totalTracks - 1)
            audioLength = juce::jmax(audioLength,
                juce::jmax(snapshot.baseLoopLengthSamples, snapshot.recordWritePosition));
        audioLength = juce::jlimit(0, maxLoopLengthSamples, audioLength);

        if (audioLength > 0)
        {
            track.audio.setSize(2, audioLength);
            track.audio.copyFrom(0, 0, tracks[i].buffer, 0, 0, audioLength);
            track.audio.copyFrom(1, 0, tracks[i].buffer, 1, 0, audioLength);
        }
    }

    return snapshot;
}

void LooperEngine::restoreSessionSnapshot(const SessionSnapshot& snapshot)
{
    sampleRate_ = snapshot.sampleRate > 0.0 ? snapshot.sampleRate : sampleRate_;
    maxLoopLengthSamples = (int) (sampleRate_ * kMaxLoopSecs);

    currentState.store(snapshot.state);
    totalTracks.store(juce::jlimit(0, kMaxTracks, snapshot.totalTracks));
    baseLoopLengthSamples.store(juce::jlimit(0, maxLoopLengthSamples, snapshot.baseLoopLengthSamples));
    loopReadPosition.store(juce::jlimit(0, juce::jmax(0, maxLoopLengthSamples - 1), snapshot.loopReadPosition));
    recordWritePosition.store(juce::jlimit(0, maxLoopLengthSamples, snapshot.recordWritePosition));
    recordStartPosition.store(juce::jlimit(0, maxLoopLengthSamples, snapshot.recordStartPosition));
    editableLoopSourceSamples.store(juce::jlimit(0, maxLoopLengthSamples, snapshot.editableLoopSourceSamples));
    globalMute.store(snapshot.globalMute);
    globalPaused.store(snapshot.globalPaused);
    silenceThresholdDb.store(snapshot.silenceThresholdDb);
    bpmParam.store(snapshot.bpm);
    beatsPerBar.store(snapshot.beatsPerBar);
    barsPerLoop.store(snapshot.barsPerLoop);
    quantizeEnabled.store(snapshot.quantizeEnabled);
    redoCount = juce::jmax(0, snapshot.redoCount);
    tapCount = 0;
    tapTimes.fill(0.0);

    for (int i = 0; i < kMaxTracks; ++i)
    {
        const auto& track = snapshot.tracks[(size_t) i];
        tracks[i].buffer.clear();

        const int audioLength = juce::jmin(maxLoopLengthSamples, track.audio.getNumSamples());
        if (audioLength > 0)
        {
            const int audioChannels = juce::jmin(2, track.audio.getNumChannels());
            for (int ch = 0; ch < audioChannels; ++ch)
                tracks[i].buffer.copyFrom(ch, 0, track.audio, ch, 0, audioLength);
            if (audioChannels == 1)
                tracks[i].buffer.copyFrom(1, 0, track.audio, 0, 0, audioLength);
        }

        tracks[i].lengthSamples.store(juce::jlimit(0, maxLoopLengthSamples, track.lengthSamples));
        tracks[i].redoLengthSamples.store(juce::jlimit(0, maxLoopLengthSamples, track.redoLengthSamples));
        tracks[i].volume.store(juce::jlimit(0.0f, 1.0f, track.volume));
        tracks[i].isMuted.store(track.isMuted);
        tracks[i].isPaused.store(track.isPaused);
        tracks[i].pausedReadPos.store(juce::jlimit(0, juce::jmax(0, maxLoopLengthSamples - 1), track.pausedReadPos));
        tracks[i].frippDecay.store(juce::jlimit(0.0f, 1.0f, track.frippDecay));
        tracks[i].frippEnabled.store(track.frippEnabled);
        tracks[i].currentGain = track.currentGain;
    }
}

// ==============================================================================
// PROCESS BLOCK — Unico metodo del audio thread
// ==============================================================================
void LooperEngine::processBlock(juce::AudioBuffer<float>& buffer,
    int startSample, int numSamples,
    float masterVolume)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh     = juce::jmin(buffer.getNumChannels(), 2);
    State     state     = currentState.load();
    int       baseSmp   = baseLoopLengthSamples.load();
    int       totalTr   = totalTracks.load();
    int       fixBars   = barsPerLoop.load();
    int       localRead = loopReadPosition.load();
    int       localWrite= recordWritePosition.load();
    bool      gPaused   = globalPaused.load();
    bool      gMuted    = globalMute.load();
    const float triggerThresholdLinear = std::pow(10.0f, silenceThresholdDb.load() / 20.0f);

    // Hoist de parametros por track — una sola lectura atomica por bloque.
    struct TrackCache {
        int   len;
        float vol;
        float gain;
        bool  muted;
        bool  paused;
        int   pausedPos;
        bool  frippOn;
        float frippDecay;
    };
    TrackCache tc[kMaxTracks];
    for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
    {
        tc[t].len       = tracks[t].lengthSamples.load();
        tc[t].vol       = tracks[t].volume.load();
        tc[t].gain      = tracks[t].currentGain;
        tc[t].muted     = tracks[t].isMuted.load();
        tc[t].paused    = tracks[t].isPaused.load();
        tc[t].pausedPos = tracks[t].pausedReadPos.load();
        tc[t].frippOn   = tracks[t].frippEnabled.load();
        tc[t].frippDecay= tracks[t].frippDecay.load();
    }

    // Pre-fetch punteros crudos para acceso sin bounds-checking.
    const float* trackRdPtr[kMaxTracks][2] = {};
    for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
        for (int ch = 0; ch < numCh; ++ch)
            trackRdPtr[t][ch] = tracks[t].buffer.getReadPointer(ch);

    float* outPtr[2] = {};
    for (int ch = 0; ch < numCh; ++ch)
        outPtr[ch] = buffer.getWritePointer(ch) + startSample;

    const bool canPlay = !gMuted &&
                         (state == State::Playing || state == State::RecordingLayer);

    // Loop principal — libre de allocaciones, atomic loads y FX internos.
    // La señal de entrada ya viene procesada por effectsChain (MainComponent).
    // Lo que se graba = lo que se escucha en vivo. Una sola cadena, sin duplicacion.
    for (int s = 0; s < numSamples; ++s)
    {
        if (state == State::ArmedBase)
        {
            bool triggerDetected = false;
            for (int ch = 0; ch < numCh && !triggerDetected; ++ch)
                triggerDetected = std::abs(outPtr[ch][s]) >= triggerThresholdLinear;

            if (triggerDetected)
            {
                state = State::RecordingBase;
                currentState.store(State::RecordingBase);
                localWrite = 0;
                recordWritePosition.store(0);
                recordStartPosition.store(0);
            }
        }

        // 1. Grabacion ANTES de agregar el playback — para que los overdubs
        //    capturen solo la entrada nueva, no la mezcla de todos los tracks.
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float inSamp = outPtr[ch][s];

            if (state == State::RecordingBase && localWrite < maxLoopLengthSamples)
            {
                tracks[0].buffer.setSample(ch, localWrite, inSamp);
            }
            else if (state == State::RecordingLayer && baseSmp > 0 && localWrite < maxLoopLengthSamples)
            {
                const int active = totalTr - 1;
                if (active >= 0 && active < kMaxTracks)
                    tracks[active].buffer.setSample(ch, localWrite, inSamp);
            }
        }

        // 2. Sumar playback de tracks grabados encima de la entrada.
        if (canPlay)
        {
            for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
            {
                if (tc[t].muted || tc[t].len <= 0) continue;

                const bool trackPaused = gPaused || tc[t].paused;
                const int  rp = trackPaused
                    ? (tc[t].pausedPos % tc[t].len)
                    : (localRead % tc[t].len);
                if (rp >= maxLoopLengthSamples) continue;

                const float scaledVol = tc[t].vol * tc[t].gain;
                for (int ch = 0; ch < numCh; ++ch)
                    outPtr[ch][s] += trackRdPtr[t][ch][rp] * scaledVol;
            }
        }

        // 3. Aplicar master volume y soft clip.
        for (int ch = 0; ch < numCh; ++ch)
            outPtr[ch][s] = juce::jlimit(-1.0f, 1.0f, outPtr[ch][s]) * masterVolume;

        // 4. Avanzar posicion de lectura.
        if (!gPaused && (state == State::Playing || state == State::RecordingLayer))
        {
            ++localRead;
            if (baseSmp > 0 && localRead >= baseSmp)
            {
                localRead = 0;
                for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
                {
                    if (tc[t].frippOn)
                    {
                        tc[t].gain *= tc[t].frippDecay;
                        tracks[t].currentGain = tc[t].gain;
                    }
                }
            }
        }

        // 5. Avanzar posicion de escritura.
        if (state == State::RecordingBase)
        {
            if (localWrite < maxLoopLengthSamples) ++localWrite;
            if (fixBars > 0 && baseSmp > 0 && localWrite >= baseSmp)
                if (onFixedLoopComplete)
                    juce::MessageManager::callAsync(onFixedLoopComplete);
        }
        else if (state == State::RecordingLayer)
        {
            if (++localWrite >= baseSmp && baseSmp > 0) localWrite = 0;
        }
    }

    // Anti race-condition: si la UI cambio el estado a Playing mientras
    // procesabamos este bloque, forzamos reset inmediato de fase.
    const State finalState = currentState.load();
    if (finalState == State::Playing && (state == State::Stopped || state == State::ArmedBase || state == State::RecordingBase))
        loopReadPosition.store(0);
    else
        loopReadPosition.store(localRead);

    recordWritePosition.store(localWrite);

    // FX post-mix: Se aplica el FX del track 0 sobre el bus de salida completo.
    // NOTA ARQUITECTURAL: En el diseño actual esto actua como un "Insert FX master".
    // Si se desea aplicar el FX de cada track a su propia señal (pre-mix),
}

// ==============================================================================
// CONTROL DE GRABACION
// ==============================================================================
void LooperEngine::recConfirm()
{
    State s = currentState.load();

    if (s == State::Stopped)
    {
        // --- FIX PRO 3: Limpieza e inicializacion absoluta de la pista base ---
        tracks[0].buffer.clear();
        tracks[0].lengthSamples.store(0);
        loopReadPosition.store(0);

        totalTracks.store(1);
        recordWritePosition.store(0);
        recordStartPosition.store(0);
        editableLoopSourceSamples.store(0);
        tracks[0].currentGain = 1.0f;
        currentState.store(State::ArmedBase);
    }
    else if (s == State::ArmedBase)
    {
        tracks[0].buffer.clear();
        totalTracks.store(1);
        recordWritePosition.store(0);
        recordStartPosition.store(0);
        editableLoopSourceSamples.store(0);
        tracks[0].currentGain = 1.0f;
        currentState.store(State::RecordingBase);
    }
    else if (s == State::RecordingBase)
    {
        int recorded = recordWritePosition.load();
        if (recorded > maxLoopLengthSamples) recorded = maxLoopLengthSamples;
        if (recorded <= 0) recorded = (int)sampleRate_;

        if (barsPerLoop.load() == 0 && quantizeEnabled.load())
            recorded = quantizeToNearestBar(recorded);

        // Detectar y eliminar silencio de inicio y final.
        // Se hace ANTES de fijar baseLoopLengthSamples para que el loop
        // empiece y termine exactamente en el contenido real.
        recorded = trimSilence(0, recorded);

        baseLoopLengthSamples.store(recorded);
        editableLoopSourceSamples.store(recorded);
        tracks[0].lengthSamples.store(recorded);

        loopReadPosition.store(0);
        currentState.store(State::Playing);
    }
    else if (s == State::Playing)
    {
        int n = totalTracks.load();
        if (n < kMaxTracks) {
            tracks[n].buffer.clear();
            tracks[n].lengthSamples.store(0);
            tracks[n].redoLengthSamples.store(0); // nueva grabacion invalida el redo
            redoCount = 0;

            const int writeStart = loopReadPosition.load();
            editableLoopSourceSamples.store(baseLoopLengthSamples.load());
            recordWritePosition.store(writeStart);
            recordStartPosition.store(writeStart);
            totalTracks.store(n + 1);
            tracks[n].currentGain = 1.0f;
            currentState.store(State::RecordingLayer);
        }
    }
    else if (s == State::RecordingLayer)
    {
        int active = totalTracks.load() - 1;
        if (active >= 0 && active < kMaxTracks)
            tracks[active].lengthSamples.store(baseLoopLengthSamples.load());
        currentState.store(State::Playing);
    }
}

void LooperEngine::cancelRec()
{
    State s = currentState.load();
    if (s == State::ArmedBase || s == State::RecordingBase) {
        currentState.store(State::Stopped);
        tracks[0].buffer.clear();
        totalTracks.store(0);
        baseLoopLengthSamples.store(0);
        editableLoopSourceSamples.store(0);
        recordWritePosition.store(0);
        recordStartPosition.store(0);
    }
    else if (s == State::RecordingLayer) {
        currentState.store(State::Playing);
        int n = totalTracks.load() - 1;
        totalTracks.store(n);
        if (n >= 0 && n < kMaxTracks) tracks[n].buffer.clear();
    }
}

void LooperEngine::undoLastTrack()
{
    int n = totalTracks.load();
    if (n <= 0) return;
    n--;
    totalTracks.store(n);

    if (n >= 0 && n < kMaxTracks)
    {
        // Guardar la longitud para poder hacer redo — NO limpiar el buffer,
        // los datos quedan en su lugar. El track deja de sonar porque
        // totalTracks decrementó y el audio thread ya no lo lee.
        tracks[n].redoLengthSamples.store(tracks[n].lengthSamples.load());
        tracks[n].lengthSamples.store(0);
        tracks[n].currentGain = 1.0f;
        redoCount++;
    }

    if (n == 0)
    {
        currentState.store(State::Stopped);
        baseLoopLengthSamples.store(0);
        // El base loop también es redoable — la longitud queda en redoLengthSamples[0]
    }
}

void LooperEngine::redoLastTrack()
{
    if (redoCount <= 0) return;

    int n = totalTracks.load();
    if (n >= kMaxTracks) return;

    int redoLen = tracks[n].redoLengthSamples.load();
    if (redoLen <= 0) return;

    // Restaurar la longitud — el buffer ya tiene los datos, nunca se borró.
    tracks[n].lengthSamples.store(redoLen);
    tracks[n].currentGain = 1.0f;
    totalTracks.store(n + 1);
    redoCount--;

    // Si redeshacemos el track base, restaurar el estado del loop
    if (n == 0)
    {
        baseLoopLengthSamples.store(redoLen);
        editableLoopSourceSamples.store(redoLen);
        loopReadPosition.store(0);
        currentState.store(State::Playing);
    }
}

void LooperEngine::stopPlay()
{
    State s = currentState.load();
    if (s == State::Playing || s == State::RecordingLayer)
        currentState.store(State::Stopped);
    else if (s == State::Stopped && baseLoopLengthSamples.load() > 0) {
        currentState.store(State::Playing);
        loopReadPosition.store(0);
    }
}

void LooperEngine::goToStart()
{
    const State s = currentState.load();
    if (s == State::ArmedBase || s == State::RecordingBase || s == State::RecordingLayer)
        return;

    loopReadPosition.store(0);

    if (globalPaused.load())
    {
        for (int t = 0; t < kMaxTracks; ++t)
            tracks[t].pausedReadPos.store(0);
    }
}

void LooperEngine::resetAll()
{
    currentState.store(State::Stopped);
    totalTracks.store(0);
    baseLoopLengthSamples.store(0);
    loopReadPosition.store(0);
    recordWritePosition.store(0);
    recordStartPosition.store(0);
    editableLoopSourceSamples.store(0);
    globalMute.store(false);

    for (int i = 0; i < kMaxTracks; ++i) {
        tracks[i].buffer.clear();
        tracks[i].lengthSamples.store(0);
        tracks[i].redoLengthSamples.store(0);
        tracks[i].volume.store(1.0f);
        tracks[i].isMuted.store(false);
        tracks[i].isPaused.store(false);
        tracks[i].pausedReadPos.store(0);
        tracks[i].currentGain = 1.0f;
        tracks[i].frippEnabled.store(false);
        tracks[i].frippDecay.store(1.0f);
    }

    tapCount = 0; tapTimes.fill(0.0);
    redoCount = 0;
}

// ==============================================================================
// MIXER POR TRACK
// ==============================================================================
void  LooperEngine::setTrackVolume(int t, float v) { if (t >= 0 && t < kMaxTracks) tracks[t].volume.store(juce::jlimit(0.0f, 1.0f, v)); }
void  LooperEngine::setTrackMuted(int t, bool m) { if (t >= 0 && t < kMaxTracks) tracks[t].isMuted.store(m); }
float LooperEngine::getTrackVolume(int t) const { return (t >= 0 && t < kMaxTracks) ? tracks[t].volume.load() : 1.0f; }
bool  LooperEngine::isTrackMuted(int t) const { return (t >= 0 && t < kMaxTracks) && tracks[t].isMuted.load(); }

// ==============================================================================
// PAUSA POR TRACK Y GLOBAL
// ==============================================================================
void LooperEngine::pauseTrack(int t)
{
    if (t < 0 || t >= kMaxTracks) return;
    tracks[t].pausedReadPos.store(loopReadPosition.load());
    tracks[t].isPaused.store(true);
}

void LooperEngine::resumeTrack(int t)
{
    if (t < 0 || t >= kMaxTracks) return;
    tracks[t].isPaused.store(false);
}

void LooperEngine::togglePauseTrack(int t)
{
    if (t < 0 || t >= kMaxTracks) return;
    if (tracks[t].isPaused.load()) resumeTrack(t);
    else                           pauseTrack(t);
}

bool LooperEngine::isTrackPaused(int t) const
{
    return (t >= 0 && t < kMaxTracks) && tracks[t].isPaused.load();
}

void LooperEngine::pauseAll()
{
    int pos = loopReadPosition.load();
    for (int t = 0; t < kMaxTracks; ++t)
        tracks[t].pausedReadPos.store(pos);
    globalPaused.store(true);
}

void LooperEngine::resumeAll()
{
    int n = totalTracks.load();
    if (n > 0 && n <= kMaxTracks)
        loopReadPosition.store(tracks[0].pausedReadPos.load());
    globalPaused.store(false);
    for (int t = 0; t < kMaxTracks; ++t)
        tracks[t].isPaused.store(false);
}

void LooperEngine::togglePauseAll()
{
    if (globalPaused.load()) resumeAll();
    else                     pauseAll();
}

bool LooperEngine::isAnyTrackPaused() const
{
    if (globalPaused.load()) return true;
    for (int t = 0; t < kMaxTracks; ++t)
        if (tracks[t].isPaused.load()) return true;
    return false;
}

// ==============================================================================
// FRIPP INFINITE DECAY
// ==============================================================================
void  LooperEngine::setFrippDecay(int t, float f) { if (t >= 0 && t < kMaxTracks) tracks[t].frippDecay.store(juce::jlimit(0.0f, 1.0f, f)); }
void  LooperEngine::setFrippEnabled(int t, bool e) { if (t >= 0 && t < kMaxTracks) { tracks[t].frippEnabled.store(e); if (!e) tracks[t].currentGain = 1.0f; } }
float LooperEngine::getFrippDecay(int t) const { return (t >= 0 && t < kMaxTracks) ? tracks[t].frippDecay.load() : 1.0f; }
bool  LooperEngine::isFrippEnabled(int t) const { return (t >= 0 && t < kMaxTracks) && tracks[t].frippEnabled.load(); }

// ==============================================================================
// TRIM DE SILENCIO
// ==============================================================================
int LooperEngine::trimSilence(int track, int recordedSamples)
{
    if (track < 0 || track >= kMaxTracks) return recordedSamples;
    if (recordedSamples <= 0)             return recordedSamples;

    const int numCh = tracks[track].buffer.getNumChannels();
    if (numCh == 0) return recordedSamples;

    // Convertir umbral de dBFS a valor lineal.
    // -60 dBFS → 0.001f, -40 dBFS → 0.01f
    const float threshLinear = std::pow(10.0f, silenceThresholdDb.load() / 20.0f);

    // --- Buscar inicio: primer sample que supera el umbral en cualquier canal ---
    int startSample = 0;
    bool found = false;
    for (int s = 0; s < recordedSamples && !found; ++s)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            if (std::abs(tracks[track].buffer.getSample(ch, s)) >= threshLinear)
            {
                startSample = s;
                found = true;
                break;
            }
        }
    }

    // Si todo es silencio, no modificar nada y devolver la longitud original.
    if (!found) return recordedSamples;

    // --- Buscar final: ultimo sample que supera el umbral en cualquier canal ---
    int endSample = recordedSamples - 1;
    found = false;
    for (int s = recordedSamples - 1; s >= startSample && !found; --s)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            if (std::abs(tracks[track].buffer.getSample(ch, s)) >= threshLinear)
            {
                endSample = s;
                found = true;
                break;
            }
        }
    }

    // Si inicio y final son el mismo que el rango completo, no hay nada que recortar.
    if (startSample == 0 && endSample == recordedSamples - 1)
        return recordedSamples;

    const int trimmedLength = endSample - startSample + 1;
    if (trimmedLength <= 0) return recordedSamples;

    // --- Copiar la region activa al inicio del buffer ---
    // Si startSample == 0 solo hay trim de cola, no hace falta copiar.
    if (startSample > 0)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* dest      = tracks[track].buffer.getWritePointer(ch);
            const float* src = tracks[track].buffer.getReadPointer(ch) + startSample;
            std::memmove(dest, src, (size_t)trimmedLength * sizeof(float));
        }
    }

    // Silenciar el resto del buffer que quedo mas alla del contenido util.
    const int remaining = recordedSamples - trimmedLength;
    if (remaining > 0)
    {
        for (int ch = 0; ch < numCh; ++ch)
            tracks[track].buffer.clear(ch, trimmedLength, remaining);
    }

    return trimmedLength;
}

void LooperEngine::rotateTrackRegion(int track, int regionLengthSamples, int deltaSamples)
{
    if (track < 0 || track >= kMaxTracks) return;
    if (regionLengthSamples <= 1) return;

    const int numCh = tracks[track].buffer.getNumChannels();
    if (numCh <= 0) return;

    int shift = deltaSamples % regionLengthSamples;
    if (shift < 0)
        shift += regionLengthSamples;
    if (shift == 0)
        return;

    juce::HeapBlock<float> temp((size_t) regionLengthSamples);
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = tracks[track].buffer.getWritePointer(ch);
        std::memcpy(temp.get(), data, (size_t) regionLengthSamples * sizeof(float));

        const int tailLen = regionLengthSamples - shift;
        std::memcpy(data, temp.get() + shift, (size_t) tailLen * sizeof(float));
        std::memcpy(data + tailLen, temp.get(), (size_t) shift * sizeof(float));
    }
}

void LooperEngine::applyLoopSeamCrossfade(int activeLengthSamples)
{
    const int totalTr = totalTracks.load();
    if (activeLengthSamples <= 1 || totalTr <= 0)
        return;

    const int numCh = tracks[0].buffer.getNumChannels();
    if (numCh <= 0)
        return;

    const int crossfadeSamples = juce::jlimit(16, 2048,
        juce::jmin(activeLengthSamples / 6, juce::roundToInt((float) sampleRate_ * 0.008f)));
    if (crossfadeSamples <= 1 || crossfadeSamples >= activeLengthSamples)
        return;

    const int fadeStart = activeLengthSamples - crossfadeSamples;
    for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
    {
        if (tracks[t].lengthSamples.load() <= 0)
            continue;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = tracks[t].buffer.getWritePointer(ch);
            for (int i = 0; i < crossfadeSamples; ++i)
            {
                const float a = data[fadeStart + i];
                const float b = data[i];
                const float mix = (float) i / (float) (crossfadeSamples - 1);
                data[fadeStart + i] = a * (1.0f - mix) + b * mix;
                data[i] = b * (1.0f - mix) + a * mix;
            }
        }
    }
}

bool LooperEngine::nudgeLoopStart(int deltaSamples)
{
    const int activeLength = baseLoopLengthSamples.load();
    const int sourceLength = editableLoopSourceSamples.load();
    const int totalTr = totalTracks.load();

    if (deltaSamples == 0 || activeLength <= 1 || sourceLength <= 1 || totalTr <= 0)
        return false;

    const int regionLength = juce::jmax(activeLength, sourceLength);
    if (regionLength <= 1)
        return false;

    for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
        rotateTrackRegion(t, regionLength, deltaSamples);

    applyLoopSeamCrossfade(activeLength);
    loopReadPosition.store(0);
    recordWritePosition.store(0);
    recordStartPosition.store(0);
    return true;
}

bool LooperEngine::adjustLoopTail(int deltaSamples)
{
    const int currentLength = baseLoopLengthSamples.load();
    const int sourceLength = editableLoopSourceSamples.load();
    const int totalTr = totalTracks.load();

    if (deltaSamples == 0 || currentLength <= 1 || sourceLength <= 1 || totalTr <= 0)
        return false;

    int newLength = juce::jlimit(64, sourceLength, currentLength + deltaSamples);
    if (newLength == currentLength)
        return false;

    baseLoopLengthSamples.store(newLength);
    for (int t = 0; t < totalTr && t < kMaxTracks; ++t)
        tracks[t].lengthSamples.store(newLength);

    applyLoopSeamCrossfade(newLength);
    loopReadPosition.store(0);
    recordWritePosition.store(0);
    recordStartPosition.store(0);
    return true;
}

// ==============================================================================
// TEMPO / CUANTIZACION
// ==============================================================================
void LooperEngine::tapTempo()
{
    double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    if (tapCount > 0 && (now - tapTimes[(tapCount - 1) % kMaxTaps]) > kTapTimeout)
        tapCount = 0;

    tapTimes[tapCount % kMaxTaps] = now;
    ++tapCount;
    if (tapCount < 2) return;

    int    valid = juce::jmin(tapCount, kMaxTaps);
    double total = 0.0;
    for (int i = 0; i < valid - 1; ++i) {
        int a = (tapCount - 1 - i) % kMaxTaps;
        int b = (tapCount - 2 - i + kMaxTaps) % kMaxTaps;
        total += tapTimes[a] - tapTimes[b];
    }
    double avg = total / (double)(valid - 1);
    if (avg <= 0.0) return;

    setBpm(juce::jlimit(20.0f, 300.0f, (float)(60.0 / avg)));
}

void LooperEngine::setBpm(float bpm)
{
    bpmParam.store(juce::jlimit(20.0f, 300.0f, bpm));
    tapCount = 0;
    recalcLoopLength();
}

void LooperEngine::setBeatsPerBar(int beats)
{
    beatsPerBar.store(juce::jmax(1, beats));
    recalcLoopLength();
}

void LooperEngine::setBarsPerLoop(int bars)
{
    barsPerLoop.store(juce::jmax(0, bars));
    recalcLoopLength();
}

void LooperEngine::recalcLoopLength()
{
    float sr    = (float)sampleRate_;
    float bpm   = bpmParam.load();
    int   beats = beatsPerBar.load();
    int   bars  = barsPerLoop.load();
    if (bars == 0 || bpm <= 0.0f || beats <= 0 || sr <= 0.0f) return;

    int newLen = juce::roundToInt((60.0f / bpm) * sr * (float)beats * (float)bars);
    newLen = juce::jlimit(1, maxLoopLengthSamples, newLen);
    baseLoopLengthSamples.store(newLen);

    int n = totalTracks.load();
    for (int i = 0; i < n && i < kMaxTracks; ++i)
        tracks[i].lengthSamples.store(newLen);
}

int LooperEngine::quantizeToNearestBar(int recorded) const
{
    float sr    = (float)sampleRate_;
    float bpm   = bpmParam.load();
    int   beats = beatsPerBar.load();
    if (bpm <= 0.0f || beats <= 0 || sr <= 0.0f) return recorded;

    float spb = (60.0f / bpm) * sr * (float)beats;
    if (spb <= 0.0f) return recorded;

    int nearestBars = juce::jmax(1, juce::roundToInt((float)recorded / spb));
    return juce::jlimit(1, maxLoopLengthSamples, juce::roundToInt(spb * (float)nearestBars));
}

// ==============================================================================
// IMPORTAR / EXPORTAR
// ==============================================================================
void LooperEngine::importAudioToBase(juce::AudioFormatManager& fmt, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(file));
    if (!reader) return;

    int len = juce::jmin((int)reader->lengthInSamples, maxLoopLengthSamples);
    if (len <= 0) len = (int)sampleRate_;

    tracks[0].buffer.clear();
    reader->read(&tracks[0].buffer, 0, len, 0, true, true);
    baseLoopLengthSamples.store(len);
    editableLoopSourceSamples.store(len);
    tracks[0].lengthSamples.store(len);
    totalTracks.store(1);
    currentState.store(State::Playing);
    loopReadPosition.store(0);
}

void LooperEngine::exportFullMix(juce::AudioFormatManager& fmt, const juce::File& dest)
{
    juce::ignoreUnused(fmt);
    int len = baseLoopLengthSamples.load();
    if (len <= 0) return;

    juce::WavAudioFormat wav;
    auto stream = dest.createOutputStream();
    if (!stream) return;

    JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
    auto* raw = stream.get();
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(raw, sampleRate_, 2, 24, {}, 0));
    JUCE_END_IGNORE_DEPRECATION_WARNINGS

    if (!writer) return;
    stream.release();

    juce::AudioBuffer<float> mix(2, len); mix.clear();
    int n = totalTracks.load();
    for (int t = 0; t < n && t < kMaxTracks; ++t)
    {
        // La mezcla exportada debe reflejar el mezclador que escucha el usuario.
        if (tracks[t].isMuted.load())
            continue;

        const float gain = tracks[t].volume.load();
        for (int ch = 0; ch < 2; ++ch)
            mix.addFrom(ch, 0, tracks[t].buffer.getReadPointer(ch), len, gain);
    }

    writer->writeFromAudioSampleBuffer(mix, 0, len);
}

void LooperEngine::exportSeparateTracks(juce::AudioFormatManager& fmt, const juce::File& folder)
{
    juce::ignoreUnused(fmt);
    int len = baseLoopLengthSamples.load();
    if (len <= 0) return;

    juce::WavAudioFormat wav;
    int n = totalTracks.load();
    for (int t = 0; t < n && t < kMaxTracks; ++t)
    {
        auto file = folder.getChildFile("Pista_" + juce::String(t + 1) + ".wav");
        auto stream = file.createOutputStream();
        if (!stream) continue;

        JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
        auto* raw = stream.get();
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wav.createWriterFor(raw, sampleRate_, 2, 24, {}, 0));
        JUCE_END_IGNORE_DEPRECATION_WARNINGS

        if (!writer) continue;
        stream.release();
        writer->writeFromAudioSampleBuffer(tracks[t].buffer, 0, len);
    }
}

// ==============================================================================
// VISUALIZACION DE FORMA DE ONDA
// ==============================================================================
juce::AudioThumbnail* LooperEngine::getTrackThumbnail(int track)
{
    if (track < 0 || track >= kMaxTracks) return nullptr;
    return thumbnails[track].get();
}

void LooperEngine::refreshThumbnail(int track, juce::AudioFormatManager& fmt)
{
    juce::ignoreUnused(fmt);
    if (track < 0 || track >= kMaxTracks) return;
    if (!thumbnails[track]) return;

    const auto state = currentState.load();

    if ((state == State::ArmedBase || state == State::RecordingBase) && track == 0)
    {
        const int len = recordWritePosition.load();
        if (len <= 0) { thumbnails[track]->clear(); return; }

        thumbnails[track]->reset(2, sampleRate_, len);
        thumbnails[track]->addBlock(0, tracks[track].buffer, 0, len);
        return;
    }
    else if (state == State::RecordingLayer && track == totalTracks.load() - 1)
    {
        const int loopLen = baseLoopLengthSamples.load();
        const int start   = juce::jlimit(0, juce::jmax(0, loopLen - 1), recordStartPosition.load());
        const int write   = juce::jlimit(0, juce::jmax(0, loopLen - 1), recordWritePosition.load());

        if (loopLen <= 0) { thumbnails[track]->clear(); return; }

        thumbnails[track]->reset(2, sampleRate_, loopLen);

        if (write == start)
            return;

        if (write > start)
        {
            thumbnails[track]->addBlock(start, tracks[track].buffer, start, write - start);
        }
        else
        {
            thumbnails[track]->addBlock(start, tracks[track].buffer, start, loopLen - start);
            if (write > 0)
                thumbnails[track]->addBlock(0, tracks[track].buffer, 0, write);
        }

        return;
    }

    int len = tracks[track].lengthSamples.load();

    if (len <= 0) { thumbnails[track]->clear(); return; }

    thumbnails[track]->reset(2, sampleRate_, len);
    thumbnails[track]->addBlock(0, tracks[track].buffer, 0, len);
}
