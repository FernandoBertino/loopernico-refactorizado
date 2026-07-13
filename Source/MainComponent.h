#pragma once

#include <JuceHeader.h>
#include "LoopernicoLookAndFeel.h"
#include "LoopernicoWidgets.h"
#include "LooperEngine.h"
#include "MidiController.h"
#include "EffectsChain.h"
#include "SessionManager.h"

class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer,
                      public juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void updateMuteSoloMatrix();
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    static constexpr int kNumFxPedals = 13;

    class SectionDragHandle final : public juce::Component
    {
    public:
        SectionDragHandle()
        {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            setRepaintsOnMouseActivity(true);
        }

        std::function<void(int)> onDrag;
        std::function<void()> onDragBegin;

        void mouseDown(const juce::MouseEvent& e) override
        {
            dragOriginY = e.getScreenY();
            if (onDragBegin != nullptr)
                onDragBegin();
        }
        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (onDrag != nullptr)
                onDrag(e.getScreenY() - dragOriginY);
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            const bool hot = isMouseOverOrDragging();
            g.setColour(juce::Colours::black.withAlpha(0.72f));
            g.fillRoundedRectangle(b.reduced(3.0f, 2.0f), 3.0f);
            g.setColour((hot ? LnfC::Accent : LnfC::Border).withAlpha(hot ? 0.92f : 0.48f));
            g.drawHorizontalLine(getHeight() / 2, 18.0f, (float) getWidth() - 18.0f);
            g.setColour(juce::Colours::white.withAlpha(hot ? 0.32f : 0.10f));
            g.drawHorizontalLine(getHeight() / 2 - 1, 18.0f, (float) getWidth() - 18.0f);
        }

    private:
        int dragOriginY = 0;
    };

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

    LoopernicoLAF lnf;
    juce::Image rackSurfaceTexture;

    LooperEngine             looper;
    MidiController           midiController;
    juce::AudioFormatManager formatManager;
    EffectsChain             effectsChain;

    juce::TextButton      btnRecConfirm, btnStopPlay, btnUndo, btnRedo;
    juce::TextButton      btnCancel, btnMuteAll, btnPauseAll, btnReset;
    juce::TextButton      btnSaveSession{"GUARDAR"};
    juce::TextButton      btnLoadSession{"ABRIR"};
    juce::TextButton      btnExportMix{"EXPORTAR MIX"};
    juce::TextButton      btnExportTracks{"EXPORTAR PISTAS"};
    juce::Viewport   trackViewport;
    juce::Component  trackContainer;
    std::vector<std::unique_ptr<TrackComponent>> visualTracks;

    juce::Viewport    fxViewport;
    FXChainCanvas     fxContainer;
    std::unique_ptr<FXPedalComponent> fxPedals[kNumFxPedals];
    std::array<int, kNumFxPedals> fxOrder{};
    int draggingFxPedal = -1;
    int dragInsertSlot = -1;
    juce::ComboBox    rhythmStyleSelector;
    juce::ComboBox    rhythmPatternSelector;
    juce::ComboBox    rhythmModeSelector;
    juce::Label       lblRhythmStyle;
    juce::Label       lblRhythmPattern;
    juce::Label       lblRhythmMode;
    juce::Label       lblRhythmBpmValue;
    juce::Label       lblRhythmColorValue;
    juce::Label       lblRhythmVerseBars;
    juce::Label       lblRhythmChorusBars;
    juce::Label       lblRhythmSectionState;
    juce::Label       lblRhythmCountIn;
    juce::Label       lblRhythmCountSound;
    juce::ComboBox    rhythmVerseBarsSelector;
    juce::ComboBox    rhythmChorusBarsSelector;
    juce::ComboBox    rhythmCountInBarsSelector;
    juce::ComboBox    rhythmCountInSoundSelector;
    juce::TextButton  btnRhythmNextSection{"SIGUIENTE PARTE"};
    juce::TextButton  btnRhythmCountInToggle{"PRECONTEO"};
    juce::int64 lastRhythmTapMs = 0;

    juce::ComboBox   midiDeviceSelector, midiActionSelector, midiMessageTypeSelector;
    juce::Slider     midiNumberSelector;
    juce::TextButton btnMidiLearn{"MAP MIDI"};
    juce::TextButton btnMidiClear{"BORRAR"};
    juce::Label      lblMidiStatus;
    juce::Label      lblMidiMapping;
    juce::Label      lblLoopEdit;
    juce::TextButton btnLoopInEarlier{"IN -"}, btnLoopInLater{"IN +"};
    juce::TextButton btnLoopOutShorter{"OUT -"}, btnLoopOutLonger{"OUT +"};
    juce::Label      lblSilenceThreshold;
    juce::Slider     sldSilenceThreshold;

    juce::Slider   masterKnob;
    juce::TextButton btnOutputMode{"MONO"};

    void changeMidiInput(int index);
    void refreshMidiInputSelector();
    void updateMidiMappingLabel();
    void updateMidiManualControls();
    void applyMidiManualMapping();
    void loadMidiConfiguration();
    void saveMidiConfiguration() const;
    juce::File getMidiConfigurationFile() const;
    int  lastMidiInputIndex = -1;
    bool midiListenAllInputs = true;
    std::atomic<juce::int64> lastMidiMs { 0 };
    std::atomic<float> masterVolume { 0.85f };
    std::atomic<bool> outputMono { true };

    SectionDragHandle looperFxDivider;
    SectionDragHandle fxControlDivider;
    int fxSectionHeight = 332;
    int controlSectionHeight = 110;
    int dividerStartFxHeight = 332;
    int dividerStartControlHeight = 110;

    std::atomic<float> inputVU   { 0.0f };
    std::atomic<float> outputVUL { 0.0f };
    std::atomic<float> outputVUR { 0.0f };
    float inputVUSmooth   = 0.0f;
    float outputVUSmoothL = 0.0f;
    float outputVUSmoothR = 0.0f;

    MasterVUMeter masterVUMeter;

    void optimiseAudioDeviceForLowLatency();
    void setupFXPedals();
    void beginFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    void updateFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    void endFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e);
    int findFxPedalIndex(const FXPedalComponent& pedal) const;
    int fxPedalBodyWidth(int pedalIndex) const;
    int fxPedalPitch(int pedalIndex) const;
    int fxSlotStartX(int slot) const;
    int fxSlotForX(int x) const;
    void moveFxPedalToSlot(int pedalIndex, int targetSlot);
    void updateOutputModeButton();
    void updateLoopEditButtons();
    void updateRhythmBoxReadouts();
    void styleVstButton(juce::TextButton& button, juce::Colour accent);
    void loadExternalVst3Plugin();
    void unloadExternalVst3Plugin();
    void openExternalVst3Editor();
    void closeExternalVst3Editor();
    void updateExternalVstUi();
    void saveSessionToFile();
    void loadSessionFromFile();
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

    juce::TextButton btnLoadVst3{"CARGAR VST3"};
    juce::TextButton btnUnloadVst3{"QUITAR"};
    juce::TextButton btnEditVst3{"ABRIR"};
    juce::TextButton btnBypassVst3{"VST3 ON"};
    juce::Label lblVst3Status;
    std::unique_ptr<juce::FileChooser> vst3FileChooser;
    std::unique_ptr<juce::FileChooser> sessionFileChooser;
    std::unique_ptr<juce::FileChooser> exportFileChooser;
    std::unique_ptr<juce::DocumentWindow> vst3EditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
