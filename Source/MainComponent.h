#pragma once

#include <JuceHeader.h>
#include "LoopernicoLookAndFeel.h"
#include "LoopernicoWidgets.h"
#include "LooperEngine.h"
#include "MidiController.h"
#include "EffectsChain.h"
#include "SessionManager.h"
#include "OrbitLoadingIndicator.h"
#include "AudioDeviceController.h"
#include "AudioSettingsPanel.h"
#include "SystemAudioCapture.h"
#include "StemSeparation/StemSeparationEngine.h"
#include "StemSeparation/StemImportController.h"
#include "StemSeparation/StemSeparationPanel.h"

class TonexDisplayLabel final : public juce::Label,
                                private juce::Timer
{
public:
    ~TonexDisplayLabel() override
    {
        stopTimer();
    }

    void setDisplayText(const juce::String& newText)
    {
        const auto normalised = newText.toUpperCase();
        if (normalised == fullText)
            return;

        fullText = normalised;
        scrollIndex = 0;
        phase = Phase::initialPause;
        stopTimer();

        if (fullText.length() > displayCharacters)
            startTimer(initialAndEndPauseMs);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto outer = getLocalBounds().toFloat().reduced(0.5f);
        const auto glass = outer.reduced(3.0f, 3.5f);

        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(outer.translated(0.0f, 2.0f), 5.5f);

        juce::ColourGradient bezel(juce::Colour(0xff28231d), outer.getX(), outer.getY(),
                                    juce::Colour(0xff070605), outer.getX(), outer.getBottom(), false);
        g.setGradientFill(bezel);
        g.fillRoundedRectangle(outer, 5.0f);

        g.setColour(juce::Colour(0xff080704));
        g.fillRoundedRectangle(glass, 3.0f);

        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(glass.toNearestInt());
            for (int y = juce::roundToInt(glass.getY()); y < juce::roundToInt(glass.getBottom()); y += 3)
            {
                g.setColour(juce::Colour(0xffffa51f).withAlpha(0.018f));
                g.drawHorizontalLine(y, glass.getX(), glass.getRight());
            }

            g.setColour(juce::Colours::white.withAlpha(0.035f));
            g.fillRoundedRectangle(glass.reduced(1.0f).withHeight(glass.getHeight() * 0.24f), 2.0f);

            auto textArea = glass.reduced(glass.getWidth() * 0.055f,
                                          glass.getHeight() * 0.18f).toNearestInt();
            const auto displayText = visibleText();
            auto displayFont = getFont();
            displayFont.setHeight((float) textArea.getHeight() * 0.88f);
            const float naturalWidth = juce::GlyphArrangement::getStringWidth(
                displayFont, juce::String::repeatedString("8", displayCharacters));
            if (naturalWidth > (float) textArea.getWidth())
                displayFont.setHeight(displayFont.getHeight()
                                      * (float) textArea.getWidth() / naturalWidth);

            g.setFont(displayFont);
            g.setColour(juce::Colour(0xff6f3105).withAlpha(0.16f));
            g.drawText(juce::String::repeatedString("8", displayCharacters), textArea,
                       juce::Justification::centred, false);

            g.setColour(juce::Colour(0xffff8a12).withAlpha(isEnabled() ? 0.16f : 0.08f));
            g.drawText(displayText, textArea.translated(0, 1), juce::Justification::centred, false);
            g.setColour(juce::Colour(0xffffa51f).withAlpha(isEnabled() ? 0.96f : 0.48f));
            g.drawText(displayText, textArea, juce::Justification::centred, false);
        }

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(glass.reduced(0.5f), 3.0f, 0.8f);
        g.setColour(juce::Colours::black.withAlpha(0.82f));
        g.drawRoundedRectangle(outer, 5.0f, 1.3f);
    }

private:
    static constexpr int displayCharacters = 8;
    static constexpr int separatorCharacters = 3;
    static constexpr int initialAndEndPauseMs = 900;
    static constexpr int scrollStepMs = 300;

    enum class Phase { initialPause, scrolling, endPause, wrapping };

    juce::String visibleText() const
    {
        if (fullText.length() <= displayCharacters)
            return fullText;

        const auto loopText = fullText + juce::String::repeatedString(" ", separatorCharacters)
                            + fullText;
        return loopText.substring(scrollIndex, scrollIndex + displayCharacters);
    }

    void timerCallback() override
    {
        const int lastFullWindow = fullText.length() - displayCharacters;

        if (phase == Phase::initialPause)
        {
            phase = Phase::scrolling;
            startTimer(scrollStepMs);
            return;
        }

        if (phase == Phase::endPause)
        {
            phase = Phase::wrapping;
            startTimer(scrollStepMs);
            return;
        }

        ++scrollIndex;
        repaint();

        if (phase == Phase::scrolling && scrollIndex >= lastFullWindow)
        {
            phase = Phase::endPause;
            startTimer(initialAndEndPauseMs);
        }
        else if (phase == Phase::wrapping
                 && scrollIndex >= fullText.length() + separatorCharacters)
        {
            scrollIndex = 0;
            phase = Phase::initialPause;
            startTimer(initialAndEndPauseMs);
        }
    }

    juce::String fullText;
    int scrollIndex = 0;
    Phase phase = Phase::initialPause;
};

class MainComponent : public juce::Component,
                      public juce::AudioSource,
                      public juce::Timer,
                      public juce::MidiInputCallback,
                      private juce::ChangeListener
{
public:
    explicit MainComponent(bool forceAudioFallbackForDiagnostics = false);
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void updateMuteSoloMatrix();
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void revealFxPedal(int pedalIndex);
    void focusFxPedalForSnapshot(int pedalIndex);
    void setVst3DisplayTextForSnapshot(const juce::String& text);
    void showAudioSettingsForSnapshot();
    void showStemSeparationForSnapshot();
    juce::Image createStemSeparationSnapshot() const;
    bool openSessionFile(const juce::File& sessionFile);
    void openSessionFileAsync(const juce::File& sessionFile);
    bool setAudioBufferForSnapshot(int bufferSamples);
    void setSoftwareInputMonitoringForSnapshot(bool enabled);
    void setSystemAudioInputForSnapshot(bool enabled);
    bool loadExternalVst3ForSnapshot(const juce::File& pluginFile, bool enabled);
    bool saveSessionFileForSnapshot(const juce::File& sessionFile, juce::String& errorMessage);
    bool runSessionPersistenceDiagnostic(const juce::File& sessionFile,
                                         juce::String& report);
    void runStemSeparationDiagnostic(const juce::File& outputDirectory,
                                     std::function<void(bool, juce::String)> completion);
    void runDemucsAdapterDiagnostic(const juce::File& outputDirectory,
                                    const juce::File& executable,
                                    const juce::File& model,
                                    const juce::File& input,
                                    std::function<void(bool, juce::String)> completion);

private:
    class LoopRangeMarker final : public juce::Component
    {
    public:
        std::function<void(float, float)> onRangeChanged;

        void setRange(float newStart, float newEnd);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

    private:
        enum class DragTarget { none, start, end };
        float start = 0.0f;
        float end = 1.0f;
        DragTarget dragTarget = DragTarget::none;
        float dragSpan = 1.0f;
    };

    // Se conserva el espacio historico del antiguo Rhythm Box (indice 1) para
    // no renumerar los pedales al restaurar sesiones anteriores. Ese indice no
    // forma parte del orden activo de la cadena.
    static constexpr int kNumFxPedals = 13;
    static constexpr int kMaxVst3Pedals = EffectsChain::kMaxExternalPlugins;
    // RhythmPedal y el indice historico de Sustain Gran no forman parte de la
    // cadena activa, aunque ambos indices se conservan para compatibilidad.
    static constexpr int kChainItemCount = kNumFxPedals - 2 + kMaxVst3Pedals;
    static constexpr int kChainItemIdLimit = kNumFxPedals + kMaxVst3Pedals;
    static constexpr int kVst3PedalBodyWidth = 254;
    static constexpr int kVst3PedalPitch = kVst3PedalBodyWidth + 28;

    class FXChainCanvas final : public juce::Component
    {
    public:
        std::function<void(juce::Graphics&)> onPaint;
        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            juce::ColourGradient floor(juce::Colour(0xff111315), 0.0f, 0.0f,
                                       juce::Colour(0xff050607), 0.0f, b.getBottom(), false);
            g.setGradientFill(floor);
            g.fillRect(b);
            LoopernicoUi::drawFineRackTexture(g, b, 0.40f);
            if (onPaint != nullptr)
                onPaint(g);
        }
    };

    class FXPedalFocusOverlay final : public juce::Component
    {
    public:
        FXPedalFocusOverlay();
        void showPedal(FXPedalComponent& pedalToShow, int compactWidth, int compactHeight);
        void clearPedal();
        FXPedalComponent* getPedal() const noexcept { return pedal; }

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& e) override;
        bool keyPressed(const juce::KeyPress& key) override;

        std::function<void()> onDismiss;

    private:
        FXPedalComponent* pedal = nullptr;
        juce::TextButton closeButton { "CERRAR   ESC" };
        juce::Rectangle<int> cardBounds;
        int baseWidth = 1;
        int baseHeight = 1;
    };

    class MidiMenuSurface final : public juce::Component
    {
    public:
        std::function<void()> onEscape;

        void paint(juce::Graphics& g) override;
        bool keyPressed(const juce::KeyPress& key) override;
    };

    class LoadingOverlay final : public juce::Component,
                                 private juce::Timer
    {
    public:
        void showLoading(const juce::String& detailText);
        void hideLoading();
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::String detail;
        float animationPhase = 0.15f;
    };

    LoopernicoLAF lnf;
    juce::Image rackSurfaceTexture;
    juce::Image brandWordmark;

    juce::AudioDeviceManager deviceManager;
    AudioDeviceController audioDeviceController { deviceManager };
    juce::AudioSourcePlayer audioSourcePlayer;
    SystemAudioCapture systemAudioCapture;
    juce::AudioBuffer<float> processedInputBuffer;
    juce::AudioBuffer<float> systemInputDryBuffer;
    juce::AudioBuffer<float> rhythmMonitorBuffer;

    LooperEngine             looper;
    MidiController           midiController;
    juce::AudioFormatManager formatManager;
    EffectsChain             effectsChain;
    LoopernicoStems::StemSeparationEngine stemSeparationEngine;
    std::unique_ptr<LoopernicoStems::StemImportController> stemImportController;
    std::unique_ptr<LoopernicoStems::StemSeparationPanel> stemSeparationPanel;
    std::unique_ptr<LoopernicoStems::StemSeparationWindow> stemSeparationWindow;

    juce::TextButton      btnRecConfirm, btnStopPlay, btnUndo, btnRedo;
    juce::TextButton      btnCancel, btnMuteAll, btnPauseAll, btnReset;
    juce::TextButton      btnSaveSession{"GUARDAR"};
    juce::TextButton      btnLoadSession{"ABRIR"};
    juce::TextButton      btnExportMix{"EXPORTAR MIX"};
    juce::TextButton      btnExportTracks{"EXPORTAR PISTAS"};
    juce::TextButton      btnAudioSettings{"AUDIO"};
    juce::TextButton      btnStemSeparation{"SEPARAR CANCION"};
    juce::Label           lblInputSource;
    juce::ComboBox        inputSourceSelector;
    juce::Label           lblAudioStatus;
    juce::Label           lblLooperTempo;
    juce::Slider          sldLooperTempo;
    juce::TextButton      btnLooperTap{"TAP"};
    juce::Viewport   trackViewport;
    juce::Component  trackContainer;
    std::vector<std::unique_ptr<TrackComponent>> visualTracks;

    juce::Viewport    fxViewport;
    FXChainCanvas     fxContainer;
    std::array<std::unique_ptr<FXPedalComponent>, kMaxVst3Pedals> vst3Pedals;
    std::unique_ptr<FXPedalComponent> fxPedals[kNumFxPedals];
    std::array<int, kChainItemCount> fxOrder{};
    int draggingFxPedal = -1; // id unificado: 0..12 interno, 13..16 VST
    int dragInsertSlot = -1;
    FXPedalFocusOverlay fxPedalFocusOverlay;
    int focusedFxPedalIndex = -2; // -2: ninguno, -1: VST3, 0..12: pedal interno
    juce::int64 lastTremoloTapMs = 0;
    juce::int64 lastDelayTapMs = 0;

    juce::ComboBox   midiDeviceSelector, midiActionSelector, midiMessageTypeSelector;
    juce::Slider     midiNumberSelector;
    MidiMenuSurface  midiMenuSurface;
    LoadingOverlay   loadingOverlay;
    juce::TextButton btnMidiMenu{"MIDI +"};
    bool             midiMenuExpanded = false;
    juce::TextButton btnMidiLearn{"MAP MIDI"};
    juce::TextButton btnMidiClear{"BORRAR"};
    juce::TextButton btnMidiSave{"GUARDAR MIDI"};
    juce::Label      lblMidiStatus;
    juce::Label      lblMidiMapping;
    LoopRangeMarker loopRangeMarker;
    float loopMarkerStart = 0.0f;
    float loopMarkerEnd = 1.0f;
    bool loopMarkerInitialised = false;
    juce::Label      lblSilenceThreshold;
    juce::Slider     sldSilenceThreshold;

    juce::Slider   masterKnob;
    juce::TextButton btnOutputMode{"MONO"};

    void changeMidiInput(int index);
    void handleMidiDeviceListChanged();
    void refreshMidiInputSelector();
    void updateMidiMappingLabel();
    void updateMidiManualControls();
    void applyMidiManualMapping();
    void toggleFxPedalFromMidi(int pedalIndex);
    void loadMidiConfiguration();
    bool saveMidiConfiguration() const;
    void markMidiConfigurationDirty();
    juce::File getMidiConfigurationFile() const;
    int  lastMidiInputIndex = -1;
    bool midiListenAllInputs = true;
    juce::String selectedMidiInputIdentifier;
    juce::MidiDeviceListConnection midiDeviceListConnection;
    std::atomic<juce::int64> lastMidiMs { 0 };
    std::atomic<float> masterVolume { 0.85f };
    std::atomic<bool> outputMono { true };
    std::atomic<bool> useSystemAudioInput { false };

    std::atomic<float> inputVU   { 0.0f };
    std::atomic<float> outputVUL { 0.0f };
    std::atomic<float> outputVUR { 0.0f };
    float inputVUSmooth   = 0.0f;
    float outputVUSmoothL = 0.0f;
    float outputVUSmoothR = 0.0f;

    MasterVUMeter masterVUMeter;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void showAudioSettings();
    void showStemSeparationWindow();
    void closeAudioSettings();
    void updateAudioStatusUi();
    void updateRecordingLatencyCompensation();
    void setupFXPedals();
    void beginFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    void updateFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    void endFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    int findFxPedalIndex(const FXPedalComponent& pedal) const;
    int findChainItemId(const FXPedalComponent& pedal) const;
    FXPedalComponent* getChainItemPedal(int chainItem) const;
    int fxPedalBodyWidth(int chainItem) const;
    int fxPedalPitch(int chainItem) const;
    int fxSlotStartX(int slot) const;
    int fxSlotForX(int x) const;
    void moveFxPedalToSlot(int pedalIndex, int targetSlot);
    void toggleFxPedalFocus(FXPedalComponent& pedal);
    void closeFxPedalFocus();
    void updateOutputModeButton();
    void setMidiMenuExpanded(bool expanded);
    void updateLoopEditButtons();
    void layoutVst3PedalControls(FXPedalComponent& pedal);
    void styleVstButton(juce::TextButton& button, juce::Colour accent);
    void loadExternalVst3Plugin(int slot = 0);
    void unloadExternalVst3Plugin(int slot = 0);
    void openExternalVst3Editor(int slot = 0);
    void openExternalVst3DrumMapEditor(int slot = 0);
    void openExternalVst3PatternEditor(int slot = 0);
    void closeExternalVst3Editor(int slot = -1);
    void updateExternalVstUi(int slot = -1);
    void saveSessionToFile();
    bool saveCurrentSessionToFile(const juce::File& sessionFile, juce::String& errorMessage);
    void loadSessionFromFile();
    void finishLoadingSession();
    void exportMixToFile();
    void exportTracksToFolder();
    bool captureLooperSnapshotIncrementally(LooperEngine::SessionSnapshot& snapshot,
                                            juce::String& errorMessage);
    juce::ValueTree createSessionTree(const LooperEngine::SessionSnapshot& looperSnapshot,
                                      const juce::MemoryBlock& vstState) const;
    bool applySessionTree(const juce::ValueTree& sessionTree,
                          const LooperEngine::SessionSnapshot& looperSnapshot,
                          const juce::MemoryBlock& vstState,
                          juce::String& errorMessage);
    void refreshTrackUiFromLooper();
    void refreshFxUiOrder();
    void refreshAllThumbnails();
    int findMidiDeviceIndexByIdentifier(const juce::String& identifier) const;

    void loadPedalPresets();
    void refreshPedalPresetSelector(int pedalIndex);
    void savePedalPreset(int pedalIndex);
    void savePedalPresetWithName(int pedalIndex, const juce::String& name);
    void deletePedalPreset(int pedalIndex);
    void applyPedalPreset(int pedalIndex, const juce::String& presetName);
    juce::File getPedalPresetFile() const;
    void refreshVstPresetSelector(int slot);
    void saveVstPreset(int slot);
    void saveVstPresetWithName(int slot, const juce::String& name);
    void deleteVstPreset(int slot);
    void applyVstPreset(int slot, const juce::String& presetName);

    std::array<juce::TextButton, kMaxVst3Pedals> btnLoadVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnUnloadVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnEditVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnMapVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnPatternVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnPatternPlayVst3;
    std::array<juce::TextButton, kMaxVst3Pedals> btnBypassVst3;
    std::array<TonexDisplayLabel, kMaxVst3Pedals> lblVst3Status;
    std::array<std::unique_ptr<juce::FileChooser>, kMaxVst3Pedals> vst3FileChoosers;
    std::unique_ptr<juce::FileChooser> sessionFileChooser;
    std::unique_ptr<juce::FileChooser> exportFileChooser;
    std::array<std::unique_ptr<juce::DocumentWindow>, kMaxVst3Pedals> vst3EditorWindows;
    std::unique_ptr<AudioSettingsPanel> audioSettingsPanel;
    std::array<juce::ComboBox, kChainItemCount> pedalPresetSelectors;
    std::array<juce::TextButton, kChainItemCount> pedalPresetSaveButtons;
    std::array<juce::TextButton, kChainItemCount> pedalPresetDeleteButtons;
    std::array<juce::ComboBox, kMaxVst3Pedals> vstPresetSelectors;
    std::array<juce::TextButton, kMaxVst3Pedals> vstPresetSaveButtons;
    std::array<juce::TextButton, kMaxVst3Pedals> vstPresetDeleteButtons;
    juce::ValueTree pedalPresets { "LOOPERNICO_PEDAL_PRESETS" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
