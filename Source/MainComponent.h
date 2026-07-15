#pragma once

#include <JuceHeader.h>
#include "LoopernicoLookAndFeel.h"
#include "LoopernicoWidgets.h"
#include "LooperEngine.h"
#include "MidiController.h"
#include "EffectsChain.h"
#include "SessionManager.h"

class TonexDisplayLabel final : public juce::Label
{
public:
    void paint(juce::Graphics& g) override
    {
        const auto outer = getLocalBounds().toFloat().reduced(0.5f);
        const auto glass = outer.reduced(3.0f, 3.5f);

        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(outer.translated(0.0f, 2.0f), 5.5f);

        juce::ColourGradient bezel(juce::Colour(0xff30200d), outer.getX(), outer.getY(),
                                    juce::Colour(0xff090704), outer.getX(), outer.getBottom(), false);
        g.setGradientFill(bezel);
        g.fillRoundedRectangle(outer, 5.0f);

        juce::ColourGradient amber(juce::Colour(0xffffbd3f), glass.getX(), glass.getY(),
                                    juce::Colour(0xffd97708), glass.getX(), glass.getBottom(), false);
        g.setGradientFill(amber);
        g.fillRoundedRectangle(glass, 3.0f);

        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(glass.toNearestInt());
            for (int y = juce::roundToInt(glass.getY()); y < juce::roundToInt(glass.getBottom()); y += 3)
            {
                g.setColour(juce::Colours::black.withAlpha(0.035f));
                g.drawHorizontalLine(y, glass.getX(), glass.getRight());
            }

            g.setColour(juce::Colours::white.withAlpha(0.14f));
            g.fillRoundedRectangle(glass.reduced(1.0f).withHeight(glass.getHeight() * 0.24f), 2.0f);

            auto textArea = glass.reduced(8.0f, 8.0f).toNearestInt();
            g.setFont(getFont());
            g.setColour(juce::Colour(0xff3a1d02).withAlpha(0.09f));
            g.drawFittedText(juce::String::repeatedString("8", juce::jmax(1, getText().length())),
                             textArea, juce::Justification::centred, 1, 0.45f);
            g.setColour(juce::Colour(0xff160b01).withAlpha(isEnabled() ? 0.96f : 0.48f));
            g.drawFittedText(getText().toUpperCase(), textArea,
                             juce::Justification::centred, 1, 0.45f);
        }

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(glass.reduced(0.5f), 3.0f, 0.8f);
        g.setColour(juce::Colours::black.withAlpha(0.82f));
        g.drawRoundedRectangle(outer, 5.0f, 1.3f);
    }
};

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
    void revealFxPedal(int pedalIndex);

private:
    static constexpr int kNumFxPedals = 13;
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

    LoopernicoLAF lnf;
    juce::Image rackSurfaceTexture;
    juce::Image brandWordmark;

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
    std::unique_ptr<FXPedalComponent> vst3Pedal;
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
    juce::int64 lastTremoloTapMs = 0;

    juce::ComboBox   midiDeviceSelector, midiActionSelector, midiMessageTypeSelector;
    juce::Slider     midiNumberSelector;
    juce::TextButton btnMidiLearn{"MAP MIDI"};
    juce::TextButton btnMidiClear{"BORRAR"};
    juce::TextButton btnMidiSave{"GUARDAR MIDI"};
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
    void toggleFxPedalFromMidi(int pedalIndex);
    void loadMidiConfiguration();
    bool saveMidiConfiguration() const;
    void markMidiConfigurationDirty();
    juce::File getMidiConfigurationFile() const;
    int  lastMidiInputIndex = -1;
    bool midiListenAllInputs = true;
    std::atomic<juce::int64> lastMidiMs { 0 };
    std::atomic<float> masterVolume { 0.85f };
    std::atomic<bool> outputMono { true };

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

    juce::TextButton btnLoadVst3{"CARGAR"};
    juce::TextButton btnUnloadVst3{"QUITAR"};
    juce::TextButton btnEditVst3{"ABRIR"};
    juce::TextButton btnBypassVst3{"VST3 ON"};
    TonexDisplayLabel lblVst3Status;
    std::unique_ptr<juce::FileChooser> vst3FileChooser;
    std::unique_ptr<juce::FileChooser> sessionFileChooser;
    std::unique_ptr<juce::FileChooser> exportFileChooser;
    std::unique_ptr<juce::DocumentWindow> vst3EditorWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
