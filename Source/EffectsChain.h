#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <vector>
#include "EQProcessor.h"

class EffectsChain : private juce::Thread
{
public:
    struct PreparedExternalPlugin
    {
        PreparedExternalPlugin() = default;
        ~PreparedExternalPlugin();
        PreparedExternalPlugin(PreparedExternalPlugin&&) noexcept;
        PreparedExternalPlugin& operator=(PreparedExternalPlugin&&) noexcept;

        PreparedExternalPlugin(const PreparedExternalPlugin&) = delete;
        PreparedExternalPlugin& operator=(const PreparedExternalPlugin&) = delete;

        bool isValid() const noexcept { return instance != nullptr; }

    private:
        friend class EffectsChain;
        void release() noexcept;

        std::unique_ptr<juce::AudioPluginInstance> instance;
        juce::AudioBuffer<float> scratchBuffer;
        juce::MidiBuffer midiBuffer;
        int inputChannels{ 0 };
        int outputChannels{ 0 };
        juce::String name;
        juce::String path;
        bool enabled{ true };
    };

    static constexpr int kFreezeMaxLayers = 5;
    static constexpr int kPedalCount = 13;

    enum PedalId
    {
        NoiseGatePedal = 0,
        RhythmPedal,
        CompressorPedal,
        HarmonizerPedal,
        SustainGranPedal,
        SustainChordsPedal,
        FuzzPedal,
        OverdrivePedal,
        ChorusPedal,
        TremoloPedal,
        DelayPedal,
        ReverbPedal,
        EqPedal
    };

    enum class DrumTriggerPlacement
    {
        BeforeCompressor = 0,
        AfterCompressor
    };

    enum class RhythmMode
    {
        Blend = 0,
        Solo,
        Metronome
    };

    enum class RhythmCountInSound
    {
        Sticks = 0,
        Click
    };

    enum class RhythmSection
    {
        Verse = 0,
        Fill,
        Chorus
    };

    EffectsChain();
    ~EffectsChain() override;

    void prepare(double sampleRate, int samplesPerBlockExpected);
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void reset();
    bool loadExternalVst3(const juce::File& pluginFile, juce::String& errorMessage);
    void unloadExternalPlugin();
    bool prepareExternalVst3(const juce::File& pluginFile,
                             PreparedExternalPlugin& candidate,
                             juce::String& errorMessage,
                             const juce::MemoryBlock* stateToRestore = nullptr);
    PreparedExternalPlugin commitExternalPlugin(PreparedExternalPlugin&& candidate) noexcept;
    PreparedExternalPlugin detachExternalPlugin() noexcept;
    bool hasExternalPlugin() const { return externalPluginInstance != nullptr; }
    void setExternalPluginEnabled(bool shouldBeEnabled) { externalPluginEnabled.store(shouldBeEnabled); }
    bool isExternalPluginEnabled() const { return externalPluginEnabled.load() && externalPluginInstance != nullptr; }
    juce::String getExternalPluginName() const { return externalPluginName; }
    juce::String getExternalPluginPath() const { return externalPluginPath; }
    juce::AudioPluginInstance* getExternalPluginInstance() const { return externalPluginInstance.get(); }
    bool copyExternalPluginState(juce::MemoryBlock& state) const;
    bool restoreExternalPluginState(const juce::MemoryBlock& state, juce::String& errorMessage);

    void setPedalOrder(const std::array<int, kPedalCount>& order);

    // --- SETTERS ATÓMICOS CONECTADOS A LA INTERFAZ GRÁFICA ---
    void setNoiseGateEnabled(bool b) { noiseGateEnabled.store(b); }
    void setNoiseGateThresholdDb(float db) { noiseGateThresholdDb.store(db); }
    void setNoiseGateAttackMs(float ms) { noiseGateAttackMs.store(ms); }
    void setNoiseGateReleaseMs(float ms) { noiseGateReleaseMs.store(ms); }

    void setCompEnabled(bool b) { compEnabled.store(b); }
    void setCompMix(float m) { compMix.store(m); }
    void setCompThresholdDb(float t) { compThresholdDb.store(t); }
    void setCompGain(float g) { compGain.store(g); }
    void setCompAction(float a) { compAction.store(a); }
    void setCompTail(float t) { compTail.store(t); }
    void setCompEq(float e) { compEq.store(e); }

    void setRhythmEnabled(bool b) { rhythmEnabled.store(b); }
    void setRhythmTempo(float bpm) { rhythmTempoBpm.store(bpm); }
    void setRhythmLevel(float l) { rhythmLevel.store(l); }
    void setRhythmSwing(float s) { rhythmSwing.store(s); }
    void setRhythmTone(float t) { rhythmTone.store(t); }
    void setRhythmStyle(int s) { rhythmStyle.store(s); }
    void setRhythmVariation(int v) { rhythmVariation.store(v); }
    void setRhythmMode(RhythmMode m) { rhythmMode.store((int)m); }
    void setRhythmCountInEnabled(bool isEnabled) { rhythmCountInEnabled.store(isEnabled); }
    void setRhythmCountInBars(int bars) { rhythmCountInBars.store(juce::jlimit(0, 8, bars)); }
    void setRhythmCountInSound(RhythmCountInSound sound) { rhythmCountInSound.store((int) sound); }
    void setRhythmVerseBars(int bars) { rhythmVerseBars.store(juce::jlimit(1, 32, bars)); }
    void setRhythmChorusBars(int bars) { rhythmChorusBars.store(juce::jlimit(1, 32, bars)); }
    void setRhythmSection(RhythmSection s) { rhythmSectionRequest.store((int) s); }
    void queueRhythmSectionChange() { rhythmAdvanceRequest.store(true); }
    int getRhythmCurrentSection() const { return rhythmCurrentSectionDisplay.load(); }
    int getRhythmPendingSection() const { return rhythmPendingSectionDisplay.load(); }
    int getRhythmCountInBeatsRemaining() const { return rhythmCountInBeatsRemainingDisplay.load(); }

    void setDrumTriggerEnabled(bool b) { drumTriggerEnabled.store(b); }
    void setDrumTriggerSensitivity(float s) { drumTriggerSensitivity.store(s); }
    void setDrumTriggerDecay(float d) { drumTriggerDecay.store(d); }
    void setDrumTriggerGate(float g) { drumTriggerGate.store(g); }
    void setDrumTriggerLevel(float l) { drumTriggerLevel.store(l); }
    void setDrumTriggerCooldownMs(float ms) { drumTriggerCooldownMs.store(ms); }
    void setDrumKickLevel(float l) { drumKickLevel.store(l); }
    void setDrumSnareLevel(float l) { drumSnareLevel.store(l); }
    void setDrumHatLevel(float l) { drumHatLevel.store(l); }
    void setDrumPitchToleranceHz(float hz) { drumPitchToleranceHz.store(hz); }
    void setDrumKickPitchHz(float hz) { drumKickPitchHz.store(hz); }
    void setDrumSnarePitchHz(float hz) { drumSnarePitchHz.store(hz); }
    void setDrumHatPitchHz(float hz) { drumHatPitchHz.store(hz); }
    void setDrumTriggerPlacement(DrumTriggerPlacement p) { drumTriggerPlacement.store((int)p); }
    float getDrumDetectorLevel() const { return drumDetectorLevel.load(); }
    float getDrumTransientLevel() const { return drumTransientLevel.load(); }
    float getDrumOutputLevel() const { return drumOutputLevel.load(); }
    float getDrumDetectedPitchHz() const { return drumDetectedPitchHz.load(); }
    int getDrumLastInstrument() const { return drumLastInstrument.load(); }
    int getDrumActiveVoiceCount() const { return drumActiveVoices.load(); }

    void setHarmonicEnabled(bool b) { harmonicEnabled.store(b); }
    void setHarmonicInterval(int i) { harmonicInterval.store(i); }
    void setHarmonicIntervalB(int i) { harmonicIntervalB.store(i); }
    void setHarmonicMix(float m) { harmonicMix.store(m); }
    void setHarmonicLevel(float l) { harmonicLevel.store(l); }
    void setHarmonicDelayMs(float ms) { harmonicDelayMs.store(juce::jlimit(0.0f, 1000.0f, ms)); }
    void setHarmonicFeedback(float f) { harmonicFeedback.store(juce::jlimit(0.0f, 0.92f, f)); }
    void setHarmonicFlex(bool pressed) { harmonicFlex.store(pressed); }

    void setFuzzEnabled(bool b) { fuzzEnabled.store(b); }
    void setFuzz(float f) { fuzz.store(f); } // CORRECCIÓN: Nombre correcto
    void setFuzzTone(float t) { fuzzTone.store(t); }
    void setFuzzLevel(float l) { fuzzLevel.store(l); }

    // Strymon Sunset: two independently switchable gain engines.  The legacy
    // setters remain aliases for channel A so old sessions and integrations do
    // not lose their overdrive settings.
    void setDriveEnabled(bool b) { driveAEnabled.store(b); }
    void setDriveAEnabled(bool b) { driveAEnabled.store(b); }
    void setDriveBEnabled(bool b) { driveBEnabled.store(b); }
    void setDrive(float d) { driveA.store(juce::jlimit(0.0f, 1.0f, d)); }
    void setDriveTone(float t) { driveATone.store(juce::jlimit(0.0f, 1.0f, t)); }
    void setDriveLevel(float l) { driveALevel.store(juce::jlimit(0.0f, 1.0f, l)); }
    void setDriveA(float d) { setDrive(d); }
    void setDriveATone(float t) { setDriveTone(t); }
    void setDriveALevel(float l) { setDriveLevel(l); }
    void setDriveAType(int type) { driveAType.store(juce::jlimit(0, 2, type)); }
    void setDriveB(float d) { driveB.store(juce::jlimit(0.0f, 1.0f, d)); }
    void setDriveBTone(float t) { driveBTone.store(juce::jlimit(0.0f, 1.0f, t)); }
    void setDriveBLevel(float l) { driveBLevel.store(juce::jlimit(0.0f, 1.0f, l)); }
    void setDriveBType(int type) { driveBType.store(juce::jlimit(0, 2, type)); }
    void setDriveRouting(int routing) { driveRouting.store(juce::jlimit(0, 2, routing)); }
    void setDriveBright(int bright) { driveBright.store(juce::jlimit(0, 2, bright)); }

    void setChorusEnabled(bool b) { chorusEnabled.store(b); }
    void setChorusMix(float m) { chorusMix.store(m); }
    void setChorusRate(float r) { chorusRate.store(r); }
    void setChorusDepth(float d) { chorusDepth.store(d); }

    void setTremoloEnabled(bool b) { tremoloEnabled.store(b); }
    void setTremoloMix(float m) { tremoloMix.store(m); }
    void setTremoloRate(float r) { tremoloRate.store(r); }
    void setTremoloDepth(float d) { tremoloDepth.store(d); }
    void setTremoloMode(int mode) { tremoloMode.store(juce::jlimit(0, 4, mode)); }
    void setTremoloBeats(int beats) { tremoloBeats.store(juce::jlimit(0, 6, beats)); }
    void setTremoloSymmetry(float symmetry) { tremoloSymmetry.store(juce::jlimit(0.0f, 1.0f, symmetry)); }
    void setTremoloControlPressed(bool pressed) { tremoloControlPressed.store(pressed); }
    void syncTremolo() { tremoloSyncRequested.store(true); }

    void setDelayEnabled(bool b) { delayEnabled.store(b); }
    void setDelayMix(float m) { delayMix.store(m); }
    void setDelayTimeMs(float t) { delayTimeMs.store(t); }
    void setDelayFeedback(float f) { delayFeedback.store(f); }
    void setDelayFilter(float v) { delayFilter.store(juce::jlimit(0.0f, 1.0f, v)); }
    void setDelayGrit(float v) { delayGrit.store(juce::jlimit(0.0f, 1.0f, v)); }
    void setDelayModRate(float hz) { delayModRate.store(juce::jlimit(0.02f, 12.0f, hz)); }
    void setDelayModDepth(float v) { delayModDepth.store(juce::jlimit(0.0f, 1.0f, v)); }
    void setDelayType(int type) { delayType.store(juce::jlimit(0, 11, type)); }
    void setDelayValue(float v) { delayValue.store(juce::jlimit(0.0f, 1.0f, v)); }

    void setReverbEnabled(bool b) { reverbEnabled.store(b); }
    void setReverbMix(float m) { reverbMix.store(m); }
    void setReverbRoomSize(float s) { reverbRoomSize.store(s); }
    void setReverbSpace(float s) { setReverbRoomSize(s); }
    void setReverbDamping(float d) { reverbDamping.store(d); }
    void setReverbPreDelayMs(float ms) { reverbPreDelayMs.store(juce::jlimit(0.0f, 1500.0f, ms)); }
    void setReverbTone(float t) { reverbTone.store(juce::jlimit(0.0f, 1.0f, t)); }
    void setReverbWidth(float w) { reverbWidth.store(juce::jlimit(0.0f, 1.0f, w)); }
    void setReverbDiffusion(float d) { reverbDiffusion.store(juce::jlimit(0.0f, 1.0f, d)); }
    void setReverbMod(float m) { reverbMod.store(juce::jlimit(0.0f, 1.0f, m)); }
    void setReverbType(int t) { reverbType.store(juce::jlimit(0, 11, t)); }
    void setReverbValue(float v) { reverbValue.store(juce::jlimit(0.0f, 1.0f, v)); }
    void setReverbInfinite(bool enabled) { reverbInfinite.store(enabled); }

    void setEqEnabled(bool b)
    {
        eqEnabled.store(b);
        outputEq.setEnabled(b);
    }
    void setEqBandGain(int band, float gainDb) { outputEq.setGain(band, gainDb); }
    void setEqInputGain(float gainDb) { outputEqInputGainDb.store(juce::jlimit(-12.0f, 12.0f, gainDb)); }
    void setEqOutputVolume(float gainDb) { outputEqVolumeDb.store(juce::jlimit(-12.0f, 12.0f, gainDb)); }

    void setFreezeEnabled(bool b) { freezeEnabled.store(b); }
    void setFreezeMix(float m) { freezeMix.store(m); }
    void setFreezeWindowMs(float w) { freezeWindowMs.store(w); }
    void setFreezeRiseMs(float ms) { freezeRiseMs.store(ms); }
    void setFreezeTailMs(float ms) { freezeTailMs.store(ms); }
    void setFreezeFadeMs(float f) { freezeTailMs.store(f); }
    void setFreezeTone(float t) { freezeTone.store(t); }
    void setFreezeWetOnly(bool b) { freezeWetOnly.store(b); }
    void setFreezeLayerCount(int c) { freezeLayerCount.store(juce::jlimit(1, kFreezeMaxLayers, c)); }
    void setFreezeDensity(float d) { freezeDensity.store(d); }
    void setChordFreezeEnabled(bool b) { chordFreezeEnabled.store(b); }
    void setChordFreezeMix(float m) { chordFreezeMix.store(m); }
    void setChordFreezeWindowMs(float w) { chordFreezeWindowMs.store(w); }
    void setChordFreezeRiseMs(float ms) { chordFreezeRiseMs.store(ms); }
    void setChordFreezeTailMs(float ms) { chordFreezeTailMs.store(ms); }
    void setChordFreezeTone(float t) { chordFreezeTone.store(t); }
    void setChordFreezeCrossfade(float x) { chordFreezeCrossfade.store(x); }

private:
    static constexpr int kChordFreezeGrainCount = 10;

    struct ChordFreezeGrain
    {
        double phase{ 0.0 };
        double rate{ 1.0 };
        int start{ 0 };
        int length{ 0 };
        float gain{ 0.0f };
    };

    struct SustainLayer
    {
        juce::AudioBuffer<float> buffer;
        int capturedLen{ 0 };
        double readPhase{ 0.0 };
        double grainPhase{ 0.0 };
        int grainLen{ 0 };
        int grainStartA{ 0 };
        int grainStartB{ 0 };
        bool active{ false };
        float wetLevel{ 0.0f };
        float brightness{ 1.0f };
        float smooth[2]{ 0.0f, 0.0f };
        float toneState[2]{ 0.0f, 0.0f };
    };

    enum class DrumInstrument
    {
        Kick = 0,
        Snare,
        Hat
    };

    enum class RhythmSampleSlot
    {
        Kick = 0,
        Snare,
        Hat,
        Accent,
        Tick,
        CountInAccent,
        CountInTick,
        Count
    };

    struct DrumVoice
    {
        bool active{ false };
        float env{ 0.0f };
        float phase{ 0.0f };
        float baseFreq{ 0.0f };
        float pitchSweep{ 0.0f };
        float noiseState{ 0.0f };
        float tone{ 0.0f };
        float velocity{ 0.0f };
    };

    struct LoadedSample
    {
        juce::AudioBuffer<float> audio;
        juce::String sourceName;
    };

    struct SamplePool
    {
        std::vector<LoadedSample> entries;
        int nextIndex{ 0 };
    };

    struct SampleVoice
    {
        bool active{ false };
        int position{ 0 };
        float velocity{ 0.0f };
        const LoadedSample* sample{ nullptr };
    };

    struct RhythmSampleBank
    {
        std::array<SamplePool, (size_t) RhythmSampleSlot::Count> pools;
        int styleIndex{ -1 };
        bool metronomeMode{ false };
        bool samplesRootFound{ false };
    };

    void triggerDrumVoice(DrumInstrument instrument, float velocity);
    void triggerRhythmSample(RhythmSampleSlot slot, float velocity);
    std::array<float, 2> renderDrumFrame(DrumInstrument instrument);
    float renderRhythmSample(RhythmSampleSlot slot, int channel);
    void advanceRhythmSampleVoices();
    void registerDrumTrigger(DrumInstrument instrument, float velocity);
    void configureRhythmPattern(int styleIndex, int variationIndex, bool metronomeMode);
    void advanceRhythmSequencer(float tempoBpm, float swingAmount, bool metronomeMode, int styleIndex);
    float estimateRecentPitchHz(int channel, int writePosition) const;
    juce::File locateRhythmSamplesRoot() const;
    juce::File locateProjectRoot() const;
    std::shared_ptr<RhythmSampleBank> loadRhythmSamples(int styleIndex, bool metronomeMode);
    void loadSamplesIntoPool(SamplePool& pool, const juce::File& folder, int maxFiles = 0);
    void loadSampleFileIntoPool(SamplePool& pool, const juce::File& file);
    void requestRhythmSampleLoad(int styleIndex, bool metronomeMode) noexcept;
    void applyPendingRhythmSampleBank(int styleIndex, bool metronomeMode) noexcept;
    void run() override;
    static RhythmSampleSlot sampleSlotForInstrument(DrumInstrument instrument);
    void processExternalPluginBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void prepareExternalPluginScratchBuffer();
    // --- VARIABLES ATÓMICAS DE CONTROL DE ESTADO ---
    std::array<std::atomic<int>, kPedalCount> pedalOrder;
    std::atomic<bool> externalPluginEnabled{ true };

    std::atomic<bool> noiseGateEnabled{ false };
    std::atomic<float> noiseGateThresholdDb{ -55.0f };
    std::atomic<float> noiseGateAttackMs{ 4.0f };
    std::atomic<float> noiseGateReleaseMs{ 90.0f };
    std::atomic<bool> compEnabled{ false };
    std::atomic<float> compMix{ 0.65f };
    std::atomic<float> compThresholdDb{ -10.0f };
    std::atomic<float> compGain{ 1.0f };
    std::atomic<float> compAction{ 0.25f };
    std::atomic<float> compTail{ 0.35f };
    std::atomic<float> compEq{ 0.0f };

    std::atomic<bool> rhythmEnabled{ false };
    std::atomic<float> rhythmTempoBpm{ 112.0f };
    std::atomic<float> rhythmLevel{ 0.78f };
    std::atomic<float> rhythmSwing{ 0.08f };
    std::atomic<float> rhythmTone{ 0.5f };
    std::atomic<int> rhythmStyle{ 0 };
    std::atomic<int> rhythmVariation{ 0 };
    std::atomic<int> rhythmMode{ (int) RhythmMode::Blend };
    std::atomic<bool> rhythmCountInEnabled{ false };
    std::atomic<int> rhythmCountInBars{ 0 };
    std::atomic<int> rhythmCountInSound{ (int) RhythmCountInSound::Sticks };
    std::atomic<int> rhythmVerseBars{ 4 };
    std::atomic<int> rhythmChorusBars{ 8 };
    std::atomic<int> rhythmSectionRequest{ -1 };
    std::atomic<bool> rhythmAdvanceRequest{ false };
    std::atomic<int> rhythmCurrentSectionDisplay{ (int) RhythmSection::Verse };
    std::atomic<int> rhythmPendingSectionDisplay{ -1 };
    std::atomic<int> rhythmCountInBeatsRemainingDisplay{ -1 };

    std::atomic<bool> drumTriggerEnabled{ false };
    std::atomic<float> drumTriggerSensitivity{ 0.55f };
    std::atomic<float> drumTriggerDecay{ 0.35f };
    std::atomic<float> drumTriggerGate{ 0.08f };
    std::atomic<float> drumTriggerLevel{ 1.0f };
    std::atomic<float> drumTriggerCooldownMs{ 42.0f };
    std::atomic<float> drumKickLevel{ 1.0f };
    std::atomic<float> drumSnareLevel{ 0.95f };
    std::atomic<float> drumHatLevel{ 0.72f };
    std::atomic<float> drumPitchToleranceHz{ 42.0f };
    std::atomic<float> drumKickPitchHz{ 110.0f };
    std::atomic<float> drumSnarePitchHz{ 196.0f };
    std::atomic<float> drumHatPitchHz{ 330.0f };
    std::atomic<int> drumTriggerPlacement{ (int) DrumTriggerPlacement::BeforeCompressor };
    std::atomic<float> drumDetectorLevel{ 0.0f };
    std::atomic<float> drumTransientLevel{ 0.0f };
    std::atomic<float> drumOutputLevel{ 0.0f };
    std::atomic<float> drumDetectedPitchHz{ 0.0f };
    std::atomic<int> drumLastInstrument{ -1 };
    std::atomic<int> drumActiveVoices{ 0 };

    std::atomic<bool> harmonicEnabled{ false };
    std::atomic<int>  harmonicInterval{ 7 };
    std::atomic<int>  harmonicIntervalB{ 12 };
    std::atomic<float> harmonicMix{ 0.4f };
    std::atomic<float> harmonicLevel{ 1.0f };
    std::atomic<float> harmonicDelayMs{ 25.0f };
    std::atomic<float> harmonicFeedback{ 0.0f };
    std::atomic<bool> harmonicFlex{ false };

    std::atomic<bool> fuzzEnabled{ false };
    std::atomic<float> fuzz{ 0.4f }; // CORRECCIÓN: Ahora es el atómico oficial
    std::atomic<float> fuzzTone{ 0.4f };
    std::atomic<float> fuzzLevel{ 0.7f };

    std::atomic<bool> driveAEnabled{ false };
    std::atomic<bool> driveBEnabled{ false };
    std::atomic<float> driveA{ 0.42f };
    std::atomic<float> driveATone{ 0.55f };
    std::atomic<float> driveALevel{ 0.52f };
    std::atomic<int> driveAType{ 1 };       // Ge, texas, treble
    std::atomic<float> driveB{ 0.36f };
    std::atomic<float> driveBTone{ 0.55f };
    std::atomic<float> driveBLevel{ 0.52f };
    std::atomic<int> driveBType{ 0 };       // 2stage, hard, JFET
    std::atomic<int> driveRouting{ 0 };     // A->B, B->A, parallel
    std::atomic<int> driveBright{ 1 };      // minus, neutral, plus

    // Four-times oversampled nonlinear state.  Each engine owns its filters so
    // routing changes do not cause the other engine's tone memory to jump.
    float sunsetStateA[6][2]{};
    float sunsetStateB[6][2]{};
    float sunsetPreviousInput[2]{};

    std::atomic<bool> chorusEnabled{ false };
    std::atomic<float> chorusMix{ 0.5f };
    std::atomic<float> chorusRate{ 1.2f };
    std::atomic<float> chorusDepth{ 0.35f };

    std::atomic<bool> tremoloEnabled{ false };
    std::atomic<float> tremoloMix{ 0.75f };
    std::atomic<float> tremoloRate{ 3.2f };
    std::atomic<float> tremoloDepth{ 0.72f };
    std::atomic<int> tremoloMode{ 3 };
    std::atomic<int> tremoloBeats{ 2 };
    std::atomic<float> tremoloSymmetry{ 0.5f };
    std::atomic<bool> tremoloControlPressed{ false };
    std::atomic<bool> tremoloSyncRequested{ false };

    std::atomic<bool> delayEnabled{ false };
    std::atomic<float> delayMix{ 0.3f };
    std::atomic<float> delayTimeMs{ 400.0f };
    std::atomic<float> delayFeedback{ 0.4f };
    std::atomic<float> delayFilter{ 0.62f };
    std::atomic<float> delayGrit{ 0.12f };
    std::atomic<float> delayModRate{ 0.35f };
    std::atomic<float> delayModDepth{ 0.10f };
    std::atomic<int> delayType{ 6 };
    std::atomic<float> delayValue{ 0.50f };

    std::atomic<bool> reverbEnabled{ false };
    std::atomic<float> reverbMix{ 0.22f };
    std::atomic<float> reverbRoomSize{ 0.45f };
    std::atomic<float> reverbDamping{ 0.3f };
    std::atomic<float> reverbPreDelayMs{ 80.0f };
    std::atomic<float> reverbTone{ 0.58f };
    std::atomic<float> reverbWidth{ 0.85f };
    std::atomic<float> reverbDiffusion{ 0.65f };
    std::atomic<float> reverbMod{ 0.18f };
    std::atomic<int> reverbType{ 1 };
    std::atomic<float> reverbValue{ 0.5f };
    std::atomic<bool> reverbInfinite{ false };
    std::atomic<bool> eqEnabled{ false };

    std::atomic<bool> freezeEnabled{ false };
    std::atomic<float> freezeMix{ 1.0f };
    std::atomic<float> freezeWindowMs{ 900.0f };
    std::atomic<float> freezeRiseMs{ 110.0f };
    std::atomic<float> freezeTailMs{ 420.0f };
    std::atomic<float> freezeTone{ 1.0f };
    std::atomic<bool> freezeWetOnly{ false };
    std::atomic<int> freezeLayerCount{ 2 };
    std::atomic<float> freezeDensity{ 1.0f };
    std::atomic<bool> chordFreezeEnabled{ false };
    std::atomic<float> chordFreezeMix{ 1.0f };
    std::atomic<float> chordFreezeWindowMs{ 900.0f };
    std::atomic<float> chordFreezeRiseMs{ 110.0f };
    std::atomic<float> chordFreezeTailMs{ 420.0f };
    std::atomic<float> chordFreezeTone{ 1.0f };
    std::atomic<float> chordFreezeCrossfade{ 1.0f };

    // =========================================================================
    // --- VARIABLES INTERNAS DE DSP RECLAMADAS POR EFFECTSCHAIN.CPP ---
    // =========================================================================
    double sampleRate_{ 44100.0 };
    int maxBlockSize_{ 512 };

    static constexpr int kPitchBufLen = 192000;
    static constexpr int kFreezeBufLen = 192000;

    juce::AudioPluginFormatManager pluginFormatManager;
    std::unique_ptr<juce::AudioPluginInstance> externalPluginInstance;
    juce::AudioBuffer<float> externalPluginScratchBuffer;
    juce::MidiBuffer externalPluginMidiBuffer;
    int externalPluginInputChannels{ 0 };
    int externalPluginOutputChannels{ 0 };
    juce::String externalPluginName;
    juce::String externalPluginPath;
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> chorusBuffer;
    juce::AudioBuffer<float> pitchBuffer;
    juce::AudioBuffer<float> harmonicDelayBufferA;
    juce::AudioBuffer<float> harmonicDelayBufferB;
    juce::AudioBuffer<float> freezeBuffer;
    juce::AudioBuffer<float> chordBodyBuffer;
    juce::AudioBuffer<float> reverbPreDelayBuffer;
    juce::AudioBuffer<float> reverbScratchBuffer;
    juce::Reverb            roomReverb;
    EQProcessor             outputEq;
    std::atomic<float>      outputEqInputGainDb { 0.0f };
    std::atomic<float>      outputEqVolumeDb { 0.0f };

    int delayWritePos{ 0 };
    int chorusWritePos{ 0 };
    int pitchWritePos{ 0 };
    int harmonicDelayWritePos{ 0 };
    int freezeWritePos{ 0 };
    int chordBodyWritePos{ 0 };
    int reverbPreDelayWritePos{ 0 };
    float reverbToneState[2]{};
    double reverbModPhase{ 0.0 };
    bool freezeHoldActive{ false };
    int freezeLayerWriteIndex{ 0 };
    std::array<SustainLayer, kFreezeMaxLayers> sustainLayers;
    std::array<ChordFreezeGrain, kChordFreezeGrainCount> freezeGrains;
    bool chordFreezeHoldActive{ false };
    SustainLayer chordSustainLayer;
    std::array<ChordFreezeGrain, kChordFreezeGrainCount> chordFreezeGrains;

    float lowPassFilters[10][2]{};
    float noiseGateEnv{ 0.0f };
    float noiseGateGain{ 1.0f };
    float chordBodyDamp[2]{ 0.0f, 0.0f };
    float compEnv[2]{ 0.0f, 0.0f };
    float compLowBand[2]{ 0.0f, 0.0f };
    float compMidLowBand[2]{ 0.0f, 0.0f };
    float drumTriggerEnv[2]{ 0.0f, 0.0f };
    float drumTriggerPulse[2]{ 0.0f, 0.0f };
    float drumLowBand{ 0.0f };
    float drumHighBand{ 0.0f };
    float drumPrevPulse{ 0.0f };
    float drumPeakValue{ 0.0f };
    float drumPeakLowBand{ 0.0f };
    float drumPeakHighBand{ 0.0f };
    float drumPeakTransient{ 0.0f };
    float drumLastTriggerLevel{ 0.0f };
    float drumDetectorDisplay{ 0.0f };
    float drumTransientDisplay{ 0.0f };
    float drumOutputDisplay{ 0.0f };
    float rhythmToneState[2]{ 0.0f, 0.0f };
    DrumVoice kickVoice;
    DrumVoice snareVoice;
    DrumVoice hatVoice;
    std::shared_ptr<RhythmSampleBank> rhythmSampleBank;
    std::shared_ptr<RhythmSampleBank> pendingRhythmSampleBank;
    std::shared_ptr<RhythmSampleBank> retiredRhythmSampleBank;
    juce::SpinLock rhythmSampleBankLock;
    std::atomic<int> requestedRhythmSampleStyle{ -1 };
    std::atomic<bool> requestedRhythmSampleMetronome{ false };
    std::atomic<unsigned int> rhythmSampleRequestSerial{ 0 };
    std::array<SampleVoice, (size_t) RhythmSampleSlot::Count> rhythmSampleVoices;
    juce::AudioFormatManager rhythmSampleFormatManager;
    static constexpr int kDrumPatternSteps = 16;
    std::array<float, kDrumPatternSteps> kickPattern{};
    std::array<float, kDrumPatternSteps> snarePattern{};
    std::array<float, kDrumPatternSteps> hatPattern{};

    int drumStepIndex{ 0 };
    int drumStepSamples{ 0 };
    int drumCooldownSamples{ 0 };
    int drumReleaseSamples{ 0 };
    int drumPeakSamples{ 0 };
    int drumNoInputCounter{ 0 };
    int drumRecentIntervalWrite{ 0 };
    int drumRecentIntervalCount{ 0 };
    int drumDensityCounter{ 0 };
    int drumBarCounter{ 0 };
    int drumTriggerRoundRobin{ 0 };
    bool drumTriggerArmed{ true };
    bool drumWasEnabled{ false };
    bool rhythmWasEnabled{ false };
    int rhythmCurrentStyle{ -1 };
    int rhythmCurrentVariation{ -1 };
    bool rhythmCurrentMetronome{ false };
    RhythmSection rhythmSectionState{ RhythmSection::Verse };
    RhythmSection rhythmPendingMainSection{ RhythmSection::Chorus };
    int rhythmBarsElapsedInSection{ 0 };
    bool rhythmTransitionQueued{ false };
    bool rhythmCountInActive{ false };
    int rhythmCountInBeatIndex{ 0 };
    int rhythmCountInTotalBeats{ 0 };
    juce::int64 rhythmCountInNextSample{ 0 };
    int rhythmCountInTailFadeSamplesRemaining{ 0 };
    int rhythmCountInTailFadeLengthSamples{ 0 };
    int rhythmLaunchFadeSamplesRemaining{ 0 };
    int rhythmLaunchFadeLengthSamples{ 0 };
    juce::int64 drumSampleCounter{ 0 };
    juce::int64 drumNextStepSample{ 0 };
    juce::int64 drumLastTriggerSample{ -1 };
    std::array<int, 4> drumRecentIntervals{};

    double pitchPhaseHarm{ 0.0 };
    double pitchPhaseHarmB{ 0.0 };
    float harmonicAutoEqState[2][2]{};
    double chorusPhase{ 0.0 };
    double delayModPhase{ 0.0 };
    double tremoloPhase{ 0.0 };
    float tremoloFootSpeedAmount{ 0.0f };
    float tremoloFadeGain{ 1.0f };

};
