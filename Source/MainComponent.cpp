#include "MainComponent.h"

using namespace LoopernicoUi;

namespace
{
    constexpr float kLoopTrimStepSeconds = 0.02f;

    constexpr int kUiOuterInset = 10;
    constexpr int kUiSectionGap = 4;
    constexpr int kUiHeaderHeight = 86;
    constexpr int kUiFooterHeight = 40;
    constexpr int kUiFxSectionHeight = 260;
    constexpr int kUiControlSectionHeight = 50;

    constexpr int kControlLabelOffsetY = 6;
    constexpr int kControlRowOffsetY = 19;

    constexpr int kFxViewportTopInset = 8;
    constexpr int kFxViewportBottomInset = 8;
    constexpr int kTrackViewportTopInset = 30;
    constexpr int kTrackViewportBottomInset = 4;

    constexpr int kFooterInset = 18;
    constexpr int kFooterOutputZoneWidth = 430;
    constexpr int kMidiDeviceWidth = 190;
    constexpr int kMidiActionWidth = 172;
    constexpr int kMidiLearnWidth = 96;
    constexpr int kMidiTypeWidth = 84;
    constexpr int kMidiNumberWidth = 72;
    constexpr int kMidiClearWidth = 64;
    constexpr int kMidiSaveWidth = 108;
    constexpr int kMidiControlGap = 8;

    juce::Typeface::Ptr loadTonexDisplayTypeface()
    {
        static const auto typeface = []() -> juce::Typeface::Ptr
        {
            const auto sourceDirectory = juce::File(__FILE__).getParentDirectory();
            const juce::File candidates[] = {
                sourceDirectory.getSiblingFile("Assets/Fonts/Sixteen/Sixteen-Mono-Bold.ttf"),
                juce::File::getCurrentWorkingDirectory()
                    .getChildFile("Assets/Fonts/Sixteen/Sixteen-Mono-Bold.ttf"),
                juce::File::getCurrentWorkingDirectory()
                    .getChildFile("Loopernico Refactorizado/Assets/Fonts/Sixteen/Sixteen-Mono-Bold.ttf")
            };

            for (const auto& file : candidates)
                if (file.existsAsFile())
                {
                    juce::MemoryBlock fontData;
                    if (file.loadFileAsData(fontData))
                        if (auto loaded = juce::Typeface::createSystemTypefaceFor(fontData.getData(),
                                                                                  fontData.getSize());
                            loaded != nullptr)
                            return loaded;
                }

            return {};
        }();

        return typeface;
    }

    class ExternalPluginEditorWindow final : public juce::DocumentWindow
    {
    public:
        ExternalPluginEditorWindow(const juce::String& title,
                                   std::unique_ptr<juce::AudioProcessorEditor> editor)
            : juce::DocumentWindow(title,
                                   juce::Colour(0xff101216),
                                   juce::DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, false);
            setContentOwned(editor.release(), true);
            centreWithSize(getWidth(), getHeight());
        }

        void closeButtonPressed() override
        {
            setVisible(false);
        }
    };

    juce::String formatPercent01(double value)
    {
        return juce::String(juce::roundToInt(value * 100.0)) + "%";
    }

    juce::String formatBlend(double value)
    {
        return juce::String(value, value >= 1.0 ? 1 : 2) + "x";
    }

    juce::String formatMs(double value)
    {
        return juce::String(juce::roundToInt(value)) + " ms";
    }

    juce::String formatDb(double value)
    {
        return juce::String(value, 0) + " dB";
    }

    juce::String formatCrossfade(double value)
    {
        return juce::String(value, 2);
    }

    juce::String noteNameForMidi(int midiNote)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        if (midiNote < 0)
            return "--";

        const int pitchClass = (midiNote % 12 + 12) % 12;
        const int octave = (midiNote / 12) - 1;
        return juce::String(names[pitchClass]) + juce::String(octave);
    }

    juce::String describeMidiMapping(const MidiController::MappingData& mapping)
    {
        if (!mapping.isMapped || mapping.number < 0)
            return "Sin asignacion";

        if (mapping.isProgramChange)
            return "Program Change " + juce::String(mapping.number);

        if (mapping.isNote)
            return "Nota " + noteNameForMidi(mapping.number) + " (" + juce::String(mapping.number) + ")";

        return "CC " + juce::String(mapping.number);
    }

    constexpr const char* kRhythmStyleNames[] = {
        "Rock",
        "Funk",
        "Disco",
        "Latin",
        "Hip Hop",
        "House",
        "Bossa Nova",
        "Techno",
        "Blues"
    };
    constexpr const char* kRhythmPatternNames[] = {
        "Verso",
        "Remate",
        "Estribillo"
    };
    constexpr const char* kRhythmModeNames[] = {
        "Con guitarra",
        "Solo ritmo",
        "Metronomo"
    };
    constexpr int kRhythmBarOptions[] = { 1, 2, 4, 8, 12, 16 };
    constexpr int kRhythmCountInOptions[] = { 0, 1, 2, 4 };
    constexpr const char* kRhythmCountInSoundNames[] = {
        "Palillos",
        "Clicks"
    };

}

MainComponent::FXPedalFocusOverlay::FXPedalFocusOverlay()
{
    setVisible(false);
    setWantsKeyboardFocus(true);
    closeButton.setTooltip("Cerrar vista ampliada");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff171a1f));
    closeButton.setColour(juce::TextButton::buttonOnColourId, LnfC::RecRed.darker(0.2f));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.9f));
    closeButton.onClick = [this]()
    {
        if (onDismiss)
            onDismiss();
    };
    addAndMakeVisible(closeButton);
}

void MainComponent::FXPedalFocusOverlay::showPedal(FXPedalComponent& pedalToShow,
                                                    int compactWidth, int compactHeight)
{
    pedal = &pedalToShow;
    baseWidth = juce::jmax(1, compactWidth);
    baseHeight = juce::jmax(1, compactHeight);
    addAndMakeVisible(pedal);
    setVisible(true);
    resized();
    repaint();
    toFront(true);
    grabKeyboardFocus();
}

void MainComponent::FXPedalFocusOverlay::clearPedal()
{
    pedal = nullptr;
    cardBounds = {};
    setVisible(false);
}

void MainComponent::FXPedalFocusOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.82f));

    if (pedal == nullptr || cardBounds.isEmpty())
        return;

    auto card = cardBounds.toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(card.translated(0.0f, 10.0f).expanded(5.0f), 22.0f);

    juce::ColourGradient stage(juce::Colour(0xff22262c), card.getX(), card.getY(),
                               juce::Colour(0xff08090b), card.getX(), card.getBottom(), false);
    g.setGradientFill(stage);
    g.fillRoundedRectangle(card, 18.0f);
    LoopernicoUi::drawFineRackTexture(g, card, 0.48f);
    g.setColour(LnfC::OvdAmber.withAlpha(0.48f));
    g.drawRoundedRectangle(card.reduced(0.8f), 18.0f, 1.4f);

    const auto titleArea = juce::Rectangle<int>(cardBounds.getX() + 24, cardBounds.getY() + 12,
                                                 cardBounds.getWidth() - 48, 34);
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    g.drawText(pedal->getDisplayName() + "  /  VISTA HD", titleArea,
               juce::Justification::centredLeft, false);

    g.setColour(juce::Colours::white.withAlpha(0.48f));
    g.setFont(juce::Font(juce::FontOptions(11.5f)));
    g.drawText("Parametros activos en tiempo real  |  Clic fuera del pedal o ESC para volver",
               titleArea.translated(0, 29), juce::Justification::centredLeft, false);
}

void MainComponent::FXPedalFocusOverlay::resized()
{
    closeButton.setBounds(getWidth() - 152, 24, 124, 30);
    if (pedal == nullptr)
        return;

    auto available = getLocalBounds().reduced(56, 34);
    available.removeFromTop(64);
    available.removeFromBottom(24);

    const float fitScale = juce::jmin((float)available.getWidth() / (float)baseWidth,
                                      (float)available.getHeight() / (float)baseHeight);
    const float scale = juce::jmin(2.75f, fitScale);
    const int targetWidth = juce::jmax(1, juce::roundToInt((float)baseWidth * scale));
    const int targetHeight = juce::jmax(1, juce::roundToInt((float)baseHeight * scale));
    const int pedalX = available.getCentreX() - targetWidth / 2;
    const int pedalY = available.getCentreY() - targetHeight / 2 + 12;
    pedal->setBounds(pedalX, pedalY, targetWidth, targetHeight);
    pedal->toFront(false);

    cardBounds = pedal->getBounds().expanded(30, 26);
    cardBounds.setTop(cardBounds.getY() - 58);
    cardBounds = cardBounds.getIntersection(getLocalBounds().reduced(18));
    closeButton.toFront(false);
}

void MainComponent::FXPedalFocusOverlay::mouseDown(const juce::MouseEvent& e)
{
    if (!cardBounds.contains(e.getPosition()) && onDismiss)
        onDismiss();
}

bool MainComponent::FXPedalFocusOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onDismiss)
            onDismiss();
        return true;
    }
    return false;
}

MainComponent::MainComponent()
{
    setLookAndFeel(&lnf);
    formatManager.registerBasicFormats();

    const auto workingDirectory = juce::File::getCurrentWorkingDirectory();
    const auto loadUiImage = [&workingDirectory](const juce::String& relativePath)
    {
        juce::Array<juce::File> assetRoots;
        assetRoots.addIfNotAlreadyThere(juce::File(__FILE__).getParentDirectory().getSiblingFile("Assets"));
        assetRoots.addIfNotAlreadyThere(workingDirectory.getChildFile("Assets"));
        assetRoots.addIfNotAlreadyThere(workingDirectory.getChildFile("Loopernico Refactorizado")
                                                        .getChildFile("Assets"));

        auto directory = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                             .getParentDirectory();
        for (int depth = 0; depth < 8; ++depth)
        {
            assetRoots.addIfNotAlreadyThere(directory.getChildFile("Assets"));
            const auto parent = directory.getParentDirectory();
            if (parent == directory)
                break;
            directory = parent;
        }

        for (const auto& root : assetRoots)
        {
            const auto candidate = root.getChildFile(relativePath);
            if (candidate.existsAsFile())
            {
                auto image = juce::ImageFileFormat::loadFrom(candidate);
                if (image.isValid())
                    return image;
            }
        }

        return juce::Image {};
    };

    rackSurfaceTexture = loadUiImage("Textures/dark-tolex-v2.png");
    brandWordmark = loadUiImage("Brand/loopernico-wordmark-mirror-v3.png");
    if (!brandWordmark.isValid())
        brandWordmark = loadUiImage("Brand/loopernico-wordmark-beveled-v2.png");

    trackViewport.setScrollBarsShown(true, false);
    trackViewport.setScrollBarThickness(7);
    trackViewport.setViewedComponent(&trackContainer, false);
    addAndMakeVisible(trackViewport);

    for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
    {
        auto tr = std::make_unique<TrackComponent>(i);
        tr->onVolumeChanged = [this, i](float v) { looper.setTrackVolume(i, v); };
        tr->onMuteChanged   = [this](bool) { updateMuteSoloMatrix(); };
        tr->onSoloChanged   = [this](bool) { updateMuteSoloMatrix(); };
        trackContainer.addAndMakeVisible(tr.get());
        visualTracks.push_back(std::move(tr));
    }

    looper.onFixedLoopComplete = [safeThis = juce::Component::SafePointer<MainComponent>(this)]() {
        if (auto* component = safeThis.getComponent())
        {
            for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
                component->looper.refreshThumbnail(i, component->formatManager);
            component->repaint();
        }
    };

    // Transport buttons
    struct BDef { juce::TextButton* b; const char* txt; juce::Colour on; };
    BDef tdefs[] = {
        { &btnRecConfirm, "REC / CONFIRM",   LnfC::RecRed                  },
        { &btnStopPlay,   "IR ARRIBA",       LnfC::PlayGreen               },
        { &btnUndo,       "UNDO",            juce::Colour(0xff28c7e8)      },
        { &btnRedo,       "REDO",            juce::Colour(0xff4d82ff)      },
        { &btnCancel,     "CANCEL",          juce::Colour(0xffffb21a)      },
        { &btnMuteAll,    "MUTE ALL",        LnfC::RecRed                  },
        { &btnPauseAll,   "PAUSE",           juce::Colour(0xffb965ff)      },
        { &btnReset,      "CLEAR ALL",       juce::Colour(0xffff5a36)      },
    };
    for (auto& d : tdefs) {
        d.b->setButtonText(d.txt);
        d.b->setColour(juce::TextButton::buttonOnColourId, d.on);
        addAndMakeVisible(*d.b);
    }

    auto styleHeaderButton = [](juce::TextButton& button, juce::Colour led)
    {
        button.setComponentID("headerConsoleButton");
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff111419));
        button.setColour(juce::TextButton::buttonOnColourId, led);
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe2e5e9));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    };
    for (auto& d : tdefs)
        if (d.b != &btnMuteAll)
            styleHeaderButton(*d.b, d.on);

    btnRecConfirm.onClick = [this]() {
        auto prev = looper.getState();
        int  tIdx = (prev == LooperEngine::State::RecordingBase) ? 0 : looper.getTotalTracks()-1;
        looper.recConfirm();
        if ((prev == LooperEngine::State::RecordingBase || prev == LooperEngine::State::RecordingLayer)
            && looper.getState() == LooperEngine::State::Playing)
        {
            if (tIdx >= 0 && tIdx < LooperEngine::kMaxTracks) looper.refreshThumbnail(tIdx, formatManager);
            
        }
        repaint();
    };
    btnStopPlay.onClick = [this]() { looper.goToStart(); };
    btnUndo.onClick = [this]() {
        int t = looper.getTotalTracks()-1; looper.undoLastTrack();
        if (t >= 0 && t < LooperEngine::kMaxTracks) looper.refreshThumbnail(t, formatManager);
        repaint();
    };
    btnRedo.onClick = [this]() {
        int t = looper.getTotalTracks(); looper.redoLastTrack();
        if (t >= 0 && t < LooperEngine::kMaxTracks) looper.refreshThumbnail(t, formatManager);
        repaint();
    };
    btnCancel.onClick = [this]() {
        int t = (looper.getState()==LooperEngine::State::RecordingBase) ? 0 : looper.getTotalTracks()-1;
        looper.cancelRec();
        if (t >= 0 && t < LooperEngine::kMaxTracks) looper.refreshThumbnail(t, formatManager);
        repaint();
    };
    btnMuteAll.onClick  = [this]() { looper.setGlobalMute(!looper.isGlobalMuted()); };
    btnPauseAll.onClick = [this]() { looper.togglePauseAll(); };
    btnReset.onClick    = [this]() {
        looper.resetAll();
        for (int i = 0; i < LooperEngine::kMaxTracks; ++i) looper.refreshThumbnail(i, formatManager);
        repaint();
    };

    styleHeaderButton(btnSaveSession,  juce::Colour(0xff42df83));
    styleHeaderButton(btnLoadSession,  juce::Colour(0xff33c3ee));
    styleHeaderButton(btnExportMix,    juce::Colour(0xffff9f2f));
    styleHeaderButton(btnExportTracks, juce::Colour(0xffec62d5));
    btnSaveSession.onClick = [this]() { saveSessionToFile(); };
    btnLoadSession.onClick = [this]() { loadSessionFromFile(); };
    btnExportMix.onClick = [this]() { exportMixToFile(); };
    btnExportTracks.onClick = [this]() { exportTracksToFolder(); };
    addAndMakeVisible(btnSaveSession);
    addAndMakeVisible(btnLoadSession);
    addAndMakeVisible(btnExportMix);
    addAndMakeVisible(btnExportTracks);

    setupFXPedals();
    fxPedalFocusOverlay.onDismiss = [this]() { closeFxPedalFocus(); };
    addChildComponent(fxPedalFocusOverlay);
    fxContainer.onPaint = [this](juce::Graphics& g)
    {
        juce::Array<FXPedalComponent*> chain;
        if (vst3Pedal != nullptr && focusedFxPedalIndex != -1)
            chain.add(vst3Pedal.get());
        for (int slot = 0; slot < kNumFxPedals; ++slot)
        {
            const int pedalIndex = fxOrder[(size_t) slot];
            if (pedalIndex != draggingFxPedal && pedalIndex != focusedFxPedalIndex
                && fxPedals[pedalIndex] != nullptr)
                chain.add(fxPedals[pedalIndex].get());
        }
        FXPedalboardWiring::CableStyle style;
        style.sheath = juce::Colour(0xff111216);
        style.thickness = 7.5f;
        style.sag = 22.0f;
        FXPedalboardWiring::drawChain(g, fxContainer, chain, style);
    };

    // MIDI
    midiDeviceSelector.setTextWhenNoChoicesAvailable("No MIDI Devices");
    refreshMidiInputSelector();
    addAndMakeVisible(midiDeviceSelector);
    midiDeviceSelector.onChange = [this]()
    {
        changeMidiInput(midiDeviceSelector.getSelectedItemIndex());
        markMidiConfigurationDirty();
    };

    for (int i = 0; i < MidiController::kNumActions; ++i)
        midiActionSelector.addItem(MidiController::actionName(i), i+1);
    midiActionSelector.setSelectedId(1);
    midiActionSelector.onChange = [this]()
    {
        updateMidiMappingLabel();
        updateMidiManualControls();
    };
    addAndMakeVisible(midiActionSelector);

    midiMessageTypeSelector.addItem("CC", 1);
    midiMessageTypeSelector.addItem("NOTA", 2);
    midiMessageTypeSelector.addItem("PROGRAM", 3);
    midiMessageTypeSelector.setSelectedId(1);
    midiMessageTypeSelector.onChange = [this]() { applyMidiManualMapping(); };
    addAndMakeVisible(midiMessageTypeSelector);

    midiNumberSelector.setSliderStyle(juce::Slider::IncDecButtons);
    midiNumberSelector.setRange(0, 127, 1);
    midiNumberSelector.setValue(0);
    midiNumberSelector.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 42, 22);
    midiNumberSelector.onValueChange = [this]() { applyMidiManualMapping(); };
    addAndMakeVisible(midiNumberSelector);

    styleVstButton(btnMidiClear, juce::Colour(0xffcc4422));
    btnMidiClear.onClick = [this]()
    {
        const int action = midiActionSelector.getSelectedId() - 1;
        midiController.setMapping(action, {});
        updateMidiMappingLabel();
        updateMidiManualControls();
        markMidiConfigurationDirty();
    };
    addAndMakeVisible(btnMidiClear);

    styleVstButton(btnMidiSave, LnfC::PlayGreen);
    btnMidiSave.onClick = [this]()
    {
        const bool saved = saveMidiConfiguration();
        lblMidiStatus.setText(saved ? "Configuracion MIDI guardada."
                                    : "No se pudo guardar la configuracion MIDI.",
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId,
                                saved ? LnfC::PlayGreen : LnfC::RecRed);
    };
    addAndMakeVisible(btnMidiSave);

    btnMidiLearn.setClickingTogglesState(true);
    btnMidiLearn.setColour(juce::TextButton::buttonOnColourId, LnfC::OvdAmber.darker(0.1f));
    addAndMakeVisible(btnMidiLearn);
    btnMidiLearn.onClick = [this]() {
        if (btnMidiLearn.getToggleState()) midiController.startLearn(midiActionSelector.getSelectedId()-1);
        else
        {
            midiController.stopLearn();
            lblMidiStatus.setText("MIDI listo.", juce::dontSendNotification);
            lblMidiStatus.setColour(juce::Label::textColourId, LnfC::TextDim);
        }
    };

    lblMidiStatus.setText("MIDI Input Ready.", juce::dontSendNotification);
    lblMidiStatus.setFont(juce::Font(juce::FontOptions(10.0f)));
    lblMidiStatus.setColour(juce::Label::textColourId, LnfC::TextDim);

    lblMidiMapping.setFont(juce::Font(juce::FontOptions(10.0f)));
    lblMidiMapping.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.88f));
    changeMidiInput(0);
    loadMidiConfiguration();
    updateMidiMappingLabel();
    updateMidiManualControls();

    lblSilenceThreshold.setText("SIGNAL THRESH.:", juce::dontSendNotification);
    lblSilenceThreshold.setFont(juce::Font(juce::FontOptions(10.0f)));
    lblSilenceThreshold.setColour(juce::Label::textColourId, LnfC::TextDim);
    addAndMakeVisible(lblSilenceThreshold);

    sldSilenceThreshold.setSliderStyle(juce::Slider::LinearHorizontal);
    sldSilenceThreshold.setRange(-80.0, 0.0, 1.0);
    sldSilenceThreshold.setValue(-60.0);
    sldSilenceThreshold.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    sldSilenceThreshold.setTextValueSuffix(" dB");
    sldSilenceThreshold.onValueChange = [this]() { looper.setSilenceThresholdDb((float)sldSilenceThreshold.getValue()); };
    addAndMakeVisible(sldSilenceThreshold);

    masterKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterKnob.setRange(0.0, 1.0, 0.01);
    masterKnob.setValue(0.85);
    masterKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterKnob.setColour(juce::Slider::trackColourId, LnfC::OvdAmber.withAlpha(0.40f));
    masterKnob.setColour(juce::Slider::thumbColourId, LnfC::OvdAmber);
    masterKnob.onValueChange = [this]()
    {
        masterVolume.store((float) masterKnob.getValue(), std::memory_order_relaxed);
    };
    addAndMakeVisible(masterKnob);
    addAndMakeVisible(masterVUMeter);
    masterVolume.store((float) masterKnob.getValue(), std::memory_order_relaxed);

    btnOutputMode.setClickingTogglesState(true);
    btnOutputMode.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    btnOutputMode.setColour(juce::TextButton::buttonOnColourId, LnfC::Accent.withAlpha(0.28f));
    btnOutputMode.onClick = [this]()
    {
        outputMono.store(btnOutputMode.getToggleState(), std::memory_order_relaxed);
        updateOutputModeButton();
    };
    addAndMakeVisible(btnOutputMode);
    updateOutputModeButton();

    styleVstButton(btnLoadVst3, LnfC::OvdAmber);
    styleVstButton(btnUnloadVst3, juce::Colour(0xff9c4630));
    styleVstButton(btnEditVst3, LnfC::Accent);
    styleVstButton(btnBypassVst3, juce::Colour(0xff2f7d53));
    for (auto* button : { &btnLoadVst3, &btnUnloadVst3, &btnEditVst3, &btnBypassVst3 })
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff111820));
        button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
    btnBypassVst3.setClickingTogglesState(true);
    btnLoadVst3.onClick = [this]() { loadExternalVst3Plugin(); };
    btnUnloadVst3.onClick = [this]() { unloadExternalVst3Plugin(); };
    btnEditVst3.onClick = [this]() { openExternalVst3Editor(); };
    btnBypassVst3.onClick = [this]()
    {
        effectsChain.setExternalPluginEnabled(btnBypassVst3.getToggleState());
        updateExternalVstUi();
    };
    if (auto typeface = loadTonexDisplayTypeface(); typeface != nullptr)
        lblVst3Status.setFont(juce::Font(juce::FontOptions(typeface).withHeight(24.0f)));
    else
        lblVst3Status.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                                           15.0f, juce::Font::bold)));
    lblVst3Status.setColour(juce::Label::textColourId, LnfC::TextDim);
    lblVst3Status.setColour(juce::Label::backgroundColourId, juce::Colour(0xff070a0d).withAlpha(0.92f));
    lblVst3Status.setColour(juce::Label::outlineColourId, LnfC::Accent.withAlpha(0.20f));
    lblVst3Status.setJustificationType(juce::Justification::centred);
    if (vst3Pedal != nullptr)
    {
        vst3Pedal->addAndMakeVisible(btnLoadVst3);
        vst3Pedal->addAndMakeVisible(btnUnloadVst3);
        vst3Pedal->addAndMakeVisible(btnEditVst3);
        vst3Pedal->addChildComponent(btnBypassVst3); // State mirror; the hardware footswitch is the live bypass control.
        vst3Pedal->addAndMakeVisible(lblVst3Status);
        vst3Pedal->onResized = [this](FXPedalComponent& pedal)
        {
            layoutVst3PedalControls(pedal);
        };
        layoutVst3PedalControls(*vst3Pedal);
    }
    updateExternalVstUi();

    lblLoopEdit.setText("ARREMANGAR", juce::dontSendNotification);
    lblLoopEdit.setFont(juce::Font(juce::FontOptions(10.0f)));
    lblLoopEdit.setColour(juce::Label::textColourId, LnfC::TextDim);
    addAndMakeVisible(lblLoopEdit);

    auto styleLoopEditButton = [](juce::TextButton& btn, juce::Colour accent)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
        btn.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.22f));
        btn.setColour(juce::TextButton::textColourOffId, LnfC::TextHi);
    };
    styleLoopEditButton(btnLoopInEarlier, LnfC::Accent);
    styleLoopEditButton(btnLoopInLater,   LnfC::Accent);
    styleLoopEditButton(btnLoopOutShorter, LnfC::OvdAmber);
    styleLoopEditButton(btnLoopOutLonger,  LnfC::OvdAmber);
    btnLoopInEarlier.setTriggeredOnMouseDown(true);
    btnLoopInLater.setTriggeredOnMouseDown(true);
    btnLoopOutShorter.setTriggeredOnMouseDown(true);
    btnLoopOutLonger.setTriggeredOnMouseDown(true);
    btnLoopInEarlier.setRepeatSpeed(300, 55, 55);
    btnLoopInLater.setRepeatSpeed(300, 55, 55);
    btnLoopOutShorter.setRepeatSpeed(300, 55, 55);
    btnLoopOutLonger.setRepeatSpeed(300, 55, 55);
    addAndMakeVisible(btnLoopInEarlier);
    addAndMakeVisible(btnLoopInLater);
    addAndMakeVisible(btnLoopOutShorter);
    addAndMakeVisible(btnLoopOutLonger);

    auto refreshAllLoopThumbnails = [this]()
    {
        const int total = looper.getTotalTracks();
        for (int i = 0; i < total && i < LooperEngine::kMaxTracks; ++i)
            looper.refreshThumbnail(i, formatManager);
        repaint();
    };

    btnLoopInEarlier.onClick = [this, refreshAllLoopThumbnails]()
    {
        const int stepSamples = juce::jmax(1, juce::roundToInt(looper.getSampleRate() * kLoopTrimStepSeconds));
        if (looper.nudgeLoopStart(-stepSamples))
            refreshAllLoopThumbnails();
    };
    btnLoopInLater.onClick = [this, refreshAllLoopThumbnails]()
    {
        const int stepSamples = juce::jmax(1, juce::roundToInt(looper.getSampleRate() * kLoopTrimStepSeconds));
        if (looper.nudgeLoopStart(stepSamples))
            refreshAllLoopThumbnails();
    };
    btnLoopOutShorter.onClick = [this, refreshAllLoopThumbnails]()
    {
        const int stepSamples = juce::jmax(1, juce::roundToInt(looper.getSampleRate() * kLoopTrimStepSeconds));
        if (looper.adjustLoopTail(-stepSamples))
            refreshAllLoopThumbnails();
    };
    btnLoopOutLonger.onClick = [this, refreshAllLoopThumbnails]()
    {
        const int stepSamples = juce::jmax(1, juce::roundToInt(looper.getSampleRate() * kLoopTrimStepSeconds));
        if (looper.adjustLoopTail(stepSamples))
            refreshAllLoopThumbnails();
    };
    updateLoopEditButtons();

    auto setupRhythmBarLabel = [this](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
        lbl.setColour(juce::Label::textColourId, LnfC::TextDim);
        addAndMakeVisible(lbl);
    };
    setupRhythmBarLabel(lblRhythmVerseBars, "VERSO");
    setupRhythmBarLabel(lblRhythmChorusBars, "ESTRIBILLO");
    setupRhythmBarLabel(lblRhythmCountIn, "PRECONTEO");
    setupRhythmBarLabel(lblRhythmCountSound, "SONIDO");
    setupRhythmBarLabel(lblRhythmSectionState, "SECCION: VERSO");

    auto setupRhythmBarsSelector = [this](juce::ComboBox& box, int defaultBars)
    {
        for (int i = 0; i < (int) juce::numElementsInArray(kRhythmBarOptions); ++i)
            box.addItem(juce::String(kRhythmBarOptions[i]) + " compases", i + 1);

        int defaultId = 1;
        for (int i = 0; i < (int) juce::numElementsInArray(kRhythmBarOptions); ++i)
            if (kRhythmBarOptions[i] == defaultBars)
                defaultId = i + 1;

        box.setSelectedId(defaultId, juce::dontSendNotification);
        addAndMakeVisible(box);
    };
    setupRhythmBarsSelector(rhythmVerseBarsSelector, 4);
    setupRhythmBarsSelector(rhythmChorusBarsSelector, 8);
    rhythmCountInBarsSelector.addItem("Ninguno", 1);
    rhythmCountInBarsSelector.addItem("1 compas", 2);
    rhythmCountInBarsSelector.addItem("2 compases", 3);
    rhythmCountInBarsSelector.addItem("4 compases", 4);
    rhythmCountInBarsSelector.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(rhythmCountInBarsSelector);
    for (int i = 0; i < (int) juce::numElementsInArray(kRhythmCountInSoundNames); ++i)
        rhythmCountInSoundSelector.addItem(kRhythmCountInSoundNames[i], i + 1);
    rhythmCountInSoundSelector.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(rhythmCountInSoundSelector);
    rhythmVerseBarsSelector.onChange = [this]()
    {
        const int idx = juce::jmax(0, rhythmVerseBarsSelector.getSelectedItemIndex());
        effectsChain.setRhythmVerseBars(kRhythmBarOptions[idx]);
    };
    rhythmChorusBarsSelector.onChange = [this]()
    {
        const int idx = juce::jmax(0, rhythmChorusBarsSelector.getSelectedItemIndex());
        effectsChain.setRhythmChorusBars(kRhythmBarOptions[idx]);
    };
    rhythmCountInBarsSelector.onChange = [this]()
    {
        const int idx = juce::jmax(0, rhythmCountInBarsSelector.getSelectedItemIndex());
        effectsChain.setRhythmCountInBars(kRhythmCountInOptions[idx]);
    };
    rhythmCountInSoundSelector.onChange = [this]()
    {
        const int idx = juce::jmax(0, rhythmCountInSoundSelector.getSelectedItemIndex());
        effectsChain.setRhythmCountInSound((EffectsChain::RhythmCountInSound) idx);
    };
    effectsChain.setRhythmVerseBars(4);
    effectsChain.setRhythmChorusBars(8);
    effectsChain.setRhythmCountInBars(1);
    effectsChain.setRhythmCountInSound(EffectsChain::RhythmCountInSound::Sticks);

    btnRhythmCountInToggle.setClickingTogglesState(true);
    btnRhythmCountInToggle.setToggleState(true, juce::dontSendNotification);
    btnRhythmCountInToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    btnRhythmCountInToggle.setColour(juce::TextButton::buttonOnColourId, LnfC::PlayGreen.withAlpha(0.22f));
    btnRhythmCountInToggle.onClick = [this]()
    {
        effectsChain.setRhythmCountInEnabled(btnRhythmCountInToggle.getToggleState());
    };
    addAndMakeVisible(btnRhythmCountInToggle);
    effectsChain.setRhythmCountInEnabled(true);

    btnRhythmNextSection.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    btnRhythmNextSection.setColour(juce::TextButton::buttonOnColourId, LnfC::Accent.withAlpha(0.24f));
    btnRhythmNextSection.onClick = [this]() { effectsChain.queueRhythmSectionChange(); };
    addAndMakeVisible(btnRhythmNextSection);

    // MIDI callbacks
    midiController.setActionCallback(MidiController::RecConfirm, [this]() { if (btnRecConfirm.onClick) btnRecConfirm.onClick(); });
    midiController.setActionCallback(MidiController::Cancel,     [this]() { if (btnCancel.onClick)     btnCancel.onClick(); });
    midiController.setActionCallback(MidiController::Undo,       [this]() { if (btnUndo.onClick)       btnUndo.onClick(); });
    midiController.setActionCallback(MidiController::Redo,       [this]() { if (btnRedo.onClick)       btnRedo.onClick(); });
    midiController.setActionCallback(MidiController::StopPlay,   [this]() { if (btnStopPlay.onClick)   btnStopPlay.onClick(); });
    midiController.setActionCallback(MidiController::MuteAll,    [this]() { if (btnMuteAll.onClick)    btnMuteAll.onClick(); });
    midiController.setActionCallback(MidiController::ResetAll,   [this]() { if (btnReset.onClick)      btnReset.onClick(); });
    midiController.setActionCallback(MidiController::PauseAll,   [this]() { if (btnPauseAll.onClick)   btnPauseAll.onClick(); });
    midiController.setActionCallback(MidiController::RhythmNextSection, [this]() { effectsChain.queueRhythmSectionChange(); });

    auto registerPedalToggle = [this](MidiController::Action action, int pedalIndex)
    {
        midiController.setActionCallback(action, [this, pedalIndex]()
        {
            toggleFxPedalFromMidi(pedalIndex);
        });
    };

    registerPedalToggle(MidiController::NoiseGateToggle,      0);
    registerPedalToggle(MidiController::RhythmBoxToggle,      1);
    registerPedalToggle(MidiController::CompressorToggle,     2);
    registerPedalToggle(MidiController::HarmonizerToggle,     3);
    registerPedalToggle(MidiController::SustainGranToggle,    4);
    registerPedalToggle(MidiController::SustainChordsToggle,  5);
    registerPedalToggle(MidiController::FuzzToggle,            6);
    registerPedalToggle(MidiController::OverdriveToggle,       7);
    registerPedalToggle(MidiController::ChorusToggle,          8);
    registerPedalToggle(MidiController::TremoloToggle,         9);
    registerPedalToggle(MidiController::TapeDelayToggle,      10);
    registerPedalToggle(MidiController::RoomReverbToggle,     11);
    registerPedalToggle(MidiController::OutputEqToggle,       12);

    midiController.setActionCallback(MidiController::Vst3Toggle, [this]()
    {
        if (vst3Pedal == nullptr)
            return;

        const bool nextState = !vst3Pedal->onBtn.getToggleState();
        vst3Pedal->onBtn.setToggleState(nextState, juce::sendNotificationSync);
        vst3Pedal->repaint();
        fxContainer.repaint();
        lblMidiStatus.setText("MIDI PEDAL VST3: " + juce::String(nextState ? "ON" : "OFF"),
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId,
                                nextState ? LnfC::PlayGreen : LnfC::TextDim);
    });

    setSize(1600, 960);
    setAudioChannels(2, 2);
    optimiseAudioDeviceForLowLatency();
    midiDeviceListConnection = juce::MidiDeviceListConnection::make(
        [safeThis = juce::Component::SafePointer<MainComponent>(this)]()
        {
            if (safeThis != nullptr)
                safeThis->handleMidiDeviceListChanged();
        });
    startTimerHz(24);
}

// ============================================================================
// SETUP - FX PEDALS
// ============================================================================
void MainComponent::setupFXPedals()
{
    struct PDef { const char* name; uint32_t col; const char* l1, *l2, *l3; };
    static const PDef defs[kNumFxPedals] = {
        { "NOISE GATE",     0xffd8d7d1, "THRESH",  "ATTACK",  "REL"   },
        { "RHYTHM BOX",     0xff9a2b1f, "BPM",     "LEVEL",   "SWING" },
        { "COMPRESSOR",     0xffb8880a, "THRESH",  "RATIO",   "LEVEL" },
        { "HARMONIZER",     0xfff1f1f1, "SHIFT A", "SHIFT B", "MIX"   },
        { "SUSTAIN GRAN",   0xff2d5c7d, "MIX",     "SIZE",    "FADE"  },
        { "SUSTAIN CHORDS", 0xff486d89, "MIX",     "SIZE",    "FADE"  },
        { "FUZZ",           0xff0f3a88, "FUZZ",    "TONE",    "LEVEL" },
        { "SUNSET",         0xffc91522, "LEVEL A", "DRIVE A", "TONE A" },
        { "CHORUS",         0xff0f4e88, "MIX",     "RATE",    "DEPTH" },
        { "TREMOLO",        0xff0b7f73, "MIX",     "RATE",    "DEPTH" },
        { "TAPE DELAY",     0xffaa5a08, "MIX",     "TIME",    "FEEDB" },
        { "ROOM REVERB",    0xff365d8c, "MIX",     "ROOM",    "DAMP"  },
        { "OUTPUT EQ",      0xff6d4a8a, "63",      "250",     "500"   },
    };

    fxViewport.setScrollBarsShown(false, true);
    fxViewport.setScrollBarThickness(7);
    fxViewport.setViewedComponent(&fxContainer, false);
    addAndMakeVisible(fxViewport);

    vst3Pedal = std::make_unique<FXPedalComponent>(
        "VST3 HOST", juce::Colour(0xff20262c), "", "", "");
    juce::Slider* vst3HiddenControls[] = {
        &vst3Pedal->knob1, &vst3Pedal->knob2, &vst3Pedal->knob3,
        &vst3Pedal->knob4, &vst3Pedal->knob5, &vst3Pedal->knob6,
        &vst3Pedal->knob7, &vst3Pedal->knob8, &vst3Pedal->knob9
    };
    for (auto* control : vst3HiddenControls)
        control->setVisible(false);
    vst3Pedal->onBtn.onClick = [this]()
    {
        if (!effectsChain.hasExternalPlugin())
        {
            vst3Pedal->onBtn.setToggleState(false, juce::dontSendNotification);
            updateExternalVstUi();
            return;
        }

        effectsChain.setExternalPluginEnabled(vst3Pedal->onBtn.getToggleState());
        updateExternalVstUi();
    };
    vst3Pedal->onZoomRequest = [this](FXPedalComponent& pedal)
    {
        toggleFxPedalFocus(pedal);
    };
    fxContainer.addAndMakeVisible(vst3Pedal.get());

    for (int i = 0; i < kNumFxPedals; ++i)
    {
        auto& d = defs[i];
        fxOrder[(size_t) i] = i;
        fxPedals[i] = std::make_unique<FXPedalComponent>(
            d.name, juce::Colour(d.col), d.l1, d.l2, d.l3);
        fxPedals[i]->onDragStart = [this](FXPedalComponent& pedal, const juce::MouseEvent& e)
        {
            beginFxPedalDrag(pedal, e);
        };
        fxPedals[i]->onDragMove = [this](FXPedalComponent& pedal, const juce::MouseEvent& e)
        {
            updateFxPedalDrag(pedal, e);
        };
        fxPedals[i]->onDragEnd = [this](FXPedalComponent& pedal, const juce::MouseEvent& e)
        {
            endFxPedalDrag(pedal, e);
        };
        fxPedals[i]->onZoomRequest = [this](FXPedalComponent& pedal)
        {
            toggleFxPedalFocus(pedal);
        };
        fxContainer.addAndMakeVisible(fxPedals[i].get());
    }
    effectsChain.setPedalOrder(fxOrder);

    auto cfg = [](juce::Slider& s, double lo, double hi, double val) {
        s.setRange(lo, hi, 0.01); s.setValue(val);
    };

    fxPedals[0]->configureExtendedKnobs("THRESH", "ATTACK", "REL", "", "", "");
    fxPedals[0]->knob1.setComponentID("bossNoiseKnob");
    fxPedals[0]->knob2.setComponentID("bossNoiseKnob");
    fxPedals[0]->knob3.setComponentID("bossNoiseKnob");
    cfg(fxPedals[0]->knob1,-80.0,-20.0,-55.0);
    cfg(fxPedals[0]->knob2,0.5,40.0,4.0);
    cfg(fxPedals[0]->knob3,20.0,600.0,90.0);
    fxPedals[0]->knob4.setVisible(false);
    fxPedals[0]->knob5.setVisible(false);
    fxPedals[0]->knob6.setVisible(false);
    fxPedals[0]->onBtn.onClick=[this](){ effectsChain.setNoiseGateEnabled(fxPedals[0]->onBtn.getToggleState()); };
    fxPedals[0]->knob1.onValueChange=[this](){ effectsChain.setNoiseGateThresholdDb((float)fxPedals[0]->knob1.getValue()); };
    fxPedals[0]->knob2.onValueChange=[this](){ effectsChain.setNoiseGateAttackMs((float)fxPedals[0]->knob2.getValue()); };
    fxPedals[0]->knob3.onValueChange=[this](){ effectsChain.setNoiseGateReleaseMs((float)fxPedals[0]->knob3.getValue()); };
    fxPedals[0]->setKnobValueFormatter(0, formatDb);
    fxPedals[0]->setKnobValueFormatter(1, formatMs);
    fxPedals[0]->setKnobValueFormatter(2, formatMs);

    fxPedals[1]->configureExtendedKnobs("BPM", "MIX", "COLOR", "STYLE", "PATTERN", "MODO");
    fxPedals[1]->knob1.setComponentID("beatBuddyKnob");
    fxPedals[1]->knob2.setComponentID("beatBuddyKnob");
    fxPedals[1]->knob3.setComponentID("beatBuddyKnob");
    cfg(fxPedals[1]->knob1, 60.0, 180.0, 112.0);
    cfg(fxPedals[1]->knob2, 0.0, 1.4, 0.78);
    cfg(fxPedals[1]->knob3, 0.0, 1.0, 0.5);
    fxPedals[1]->knob4.setRange(0, 8, 1); fxPedals[1]->knob4.setValue(0);
    fxPedals[1]->knob5.setRange(0, 2, 1); fxPedals[1]->knob5.setValue(0);
    fxPedals[1]->knob6.setRange(0, 2, 1); fxPedals[1]->knob6.setValue(0);
    fxPedals[1]->knob4.setVisible(false);
    fxPedals[1]->knob5.setVisible(false);
    fxPedals[1]->knob6.setVisible(false);

    auto styleRhythmInfoLabel = [](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.88f));
        lbl.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
    };

    styleRhythmInfoLabel(lblRhythmStyle,   "STYLE");
    styleRhythmInfoLabel(lblRhythmPattern, "PARTE");
    styleRhythmInfoLabel(lblRhythmMode,    "MODO");
    for (auto* lbl : { &lblRhythmStyle, &lblRhythmPattern, &lblRhythmMode })
        lbl->setColour(juce::Label::textColourId, juce::Colour(0xff82e6eb).withAlpha(0.90f));
    for (auto* lbl : { &lblRhythmStyle, &lblRhythmPattern, &lblRhythmMode, &lblRhythmBpmValue, &lblRhythmColorValue })
        fxPedals[1]->addAndMakeVisible(*lbl);

    rhythmStyleSelector.clear(juce::dontSendNotification);
    for (int i = 0; i < (int) juce::numElementsInArray(kRhythmStyleNames); ++i)
        rhythmStyleSelector.addItem(kRhythmStyleNames[i], i + 1);
    rhythmStyleSelector.setSelectedId(1, juce::dontSendNotification);
    rhythmStyleSelector.setJustificationType(juce::Justification::centredLeft);
    rhythmStyleSelector.setTextWhenNothingSelected("STYLE");
    rhythmStyleSelector.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    rhythmStyleSelector.setComponentID("rhythmScreenCombo");
    rhythmStyleSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdffcff));
    fxPedals[1]->addAndMakeVisible(rhythmStyleSelector);

    rhythmPatternSelector.clear(juce::dontSendNotification);
    for (int i = 0; i < (int) juce::numElementsInArray(kRhythmPatternNames); ++i)
        rhythmPatternSelector.addItem(kRhythmPatternNames[i], i + 1);
    rhythmPatternSelector.setSelectedId(1, juce::dontSendNotification);
    rhythmPatternSelector.setJustificationType(juce::Justification::centredLeft);
    rhythmPatternSelector.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    rhythmPatternSelector.setComponentID("rhythmScreenCombo");
    rhythmPatternSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdffcff));
    fxPedals[1]->addAndMakeVisible(rhythmPatternSelector);

    rhythmModeSelector.clear(juce::dontSendNotification);
    for (int i = 0; i < (int) juce::numElementsInArray(kRhythmModeNames); ++i)
        rhythmModeSelector.addItem(kRhythmModeNames[i], i + 1);
    rhythmModeSelector.setSelectedId(1, juce::dontSendNotification);
    rhythmModeSelector.setJustificationType(juce::Justification::centredLeft);
    rhythmModeSelector.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    rhythmModeSelector.setComponentID("rhythmScreenCombo");
    rhythmModeSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdffcff));
    fxPedals[1]->addAndMakeVisible(rhythmModeSelector);

    fxPedals[1]->onBtn.onClick=[this](){ effectsChain.setRhythmEnabled(fxPedals[1]->onBtn.getToggleState()); };
    fxPedals[1]->onSecondaryFootswitch=[this](){ effectsChain.queueRhythmSectionChange(); };
    fxPedals[1]->onTap=[this]()
    {
        const juce::int64 now = juce::Time::currentTimeMillis();
        const juce::int64 elapsed = now - lastRhythmTapMs;
        lastRhythmTapMs = now;
        if (elapsed >= 333 && elapsed <= 1000)
        {
            const double bpm = juce::jlimit(60.0, 180.0, 60000.0 / (double) elapsed);
            fxPedals[1]->knob1.setValue(bpm, juce::sendNotificationSync);
        }
    };
    fxPedals[1]->knob1.onValueChange=[this](){ effectsChain.setRhythmTempo((float)fxPedals[1]->knob1.getValue()); updateRhythmBoxReadouts(); };
    fxPedals[1]->knob2.onValueChange=[this](){ effectsChain.setRhythmLevel((float)fxPedals[1]->knob2.getValue()); updateRhythmBoxReadouts(); };
    fxPedals[1]->knob3.onValueChange=[this](){ effectsChain.setRhythmTone((float)fxPedals[1]->knob3.getValue()); updateRhythmBoxReadouts(); };
    rhythmStyleSelector.onChange=[this]()
    {
        const int styleIndex = juce::jmax(0, rhythmStyleSelector.getSelectedItemIndex());
        fxPedals[1]->knob4.setValue(styleIndex, juce::dontSendNotification);
        effectsChain.setRhythmStyle(styleIndex);
    };
    rhythmPatternSelector.onChange=[this]()
    {
        const int sectionIndex = juce::jmax(0, rhythmPatternSelector.getSelectedItemIndex());
        fxPedals[1]->knob5.setValue(sectionIndex, juce::dontSendNotification);
        effectsChain.setRhythmSection((EffectsChain::RhythmSection) sectionIndex);
    };
    rhythmModeSelector.onChange=[this]()
    {
        const int modeIndex = juce::jmax(0, rhythmModeSelector.getSelectedItemIndex());
        fxPedals[1]->knob6.setValue(modeIndex, juce::dontSendNotification);
        effectsChain.setRhythmMode((EffectsChain::RhythmMode) modeIndex);
    };
    effectsChain.setRhythmTone((float) fxPedals[1]->knob3.getValue());
    updateRhythmBoxReadouts();

    fxPedals[2]->configureExtendedKnobs("BLEND", "GAIN", "COMP", "ATTACK", "", "TONE");
    fxPedals[2]->knob1.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob2.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob3.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob4.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob5.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob6.setComponentID("dedaloJetKnob");
    fxPedals[2]->knob5.setVisible(false); // Release is fixed on the physical JET 2.
    cfg(fxPedals[2]->knob1,0.0,1.0,0.65);
    cfg(fxPedals[2]->knob2,0.5,2.5,1.0);
    cfg(fxPedals[2]->knob3,-36.0,-6.0,-18.0);
    cfg(fxPedals[2]->knob4,0.0,1.0,0.25);
    cfg(fxPedals[2]->knob5,0.0,1.0,0.35);
    fxPedals[2]->knob6.setRange(-1.0, 1.0, 0.01); fxPedals[2]->knob6.setValue(0.0);
    fxPedals[2]->onBtn.onClick=[this](){ effectsChain.setCompEnabled(fxPedals[2]->onBtn.getToggleState()); };
    fxPedals[2]->knob1.onValueChange=[this](){ effectsChain.setCompMix((float)fxPedals[2]->knob1.getValue()); };
    fxPedals[2]->knob2.onValueChange=[this](){ effectsChain.setCompGain((float)fxPedals[2]->knob2.getValue()); };
    fxPedals[2]->knob3.onValueChange=[this](){ effectsChain.setCompThresholdDb((float)fxPedals[2]->knob3.getValue()); };
    fxPedals[2]->knob4.onValueChange=[this](){ effectsChain.setCompAction((float)fxPedals[2]->knob4.getValue()); };
    fxPedals[2]->knob5.onValueChange=[this](){ effectsChain.setCompTail((float)fxPedals[2]->knob5.getValue()); };
    fxPedals[2]->knob6.onValueChange=[this](){ effectsChain.setCompEq((float)fxPedals[2]->knob6.getValue()); };

    fxPedals[3]->configureExtendedKnobs("SHIFT A", "SHIFT B", "MIX", "DELAY", "FEEDBACK", "");
    for (auto* knob : { &fxPedals[3]->knob1, &fxPedals[3]->knob2, &fxPedals[3]->knob3,
                        &fxPedals[3]->knob4, &fxPedals[3]->knob5 })
        knob->setComponentID("eventideH90Knob");
    fxPedals[3]->knob1.setRange(-24.0, 24.0, 1.0); fxPedals[3]->knob1.setValue(7.0);
    fxPedals[3]->knob2.setRange(-24.0, 24.0, 1.0); fxPedals[3]->knob2.setValue(12.0);
    cfg(fxPedals[3]->knob3, 0.0, 1.0, 0.40);
    cfg(fxPedals[3]->knob4, 0.0, 1000.0, 25.0);
    cfg(fxPedals[3]->knob5, 0.0, 0.92, 0.0);
    fxPedals[3]->knob6.setVisible(false);
    fxPedals[3]->knob7.setVisible(false);
    fxPedals[3]->knob8.setVisible(false);
    fxPedals[3]->knob9.setVisible(false);
    fxPedals[3]->onBtn.onClick=[this](){ effectsChain.setHarmonicEnabled(fxPedals[3]->onBtn.getToggleState()); };
    fxPedals[3]->knob1.onValueChange=[this](){ effectsChain.setHarmonicInterval((int)fxPedals[3]->knob1.getValue()); fxPedals[3]->repaint(); };
    fxPedals[3]->knob2.onValueChange=[this](){ effectsChain.setHarmonicIntervalB((int)fxPedals[3]->knob2.getValue()); fxPedals[3]->repaint(); };
    fxPedals[3]->knob3.onValueChange=[this](){ effectsChain.setHarmonicMix((float)fxPedals[3]->knob3.getValue()); fxPedals[3]->repaint(); };
    fxPedals[3]->knob4.onValueChange=[this](){ effectsChain.setHarmonicDelayMs((float)fxPedals[3]->knob4.getValue()); fxPedals[3]->repaint(); };
    fxPedals[3]->knob5.onValueChange=[this](){ effectsChain.setHarmonicFeedback((float)fxPedals[3]->knob5.getValue()); fxPedals[3]->repaint(); };
    fxPedals[3]->onSecondaryFootswitch=[this](){ effectsChain.setHarmonicFlex(true); };
    fxPedals[3]->onSecondaryFootswitchRelease=[this](){ effectsChain.setHarmonicFlex(false); };
    fxPedals[3]->onTertiaryFootswitch=[this]()
    {
        static constexpr int h90Programs[][2] = { { 7, 12 }, { -12, 7 }, { -5, 7 }, { 12, 24 } };
        int currentProgram = 0;
        for (int i = 0; i < (int) juce::numElementsInArray(h90Programs); ++i)
            if (juce::roundToInt(fxPedals[3]->knob1.getValue()) == h90Programs[i][0]
                && juce::roundToInt(fxPedals[3]->knob2.getValue()) == h90Programs[i][1])
                currentProgram = i + 1;
        const auto& next = h90Programs[currentProgram % (int) juce::numElementsInArray(h90Programs)];
        fxPedals[3]->knob1.setValue(next[0], juce::sendNotificationSync);
        fxPedals[3]->knob2.setValue(next[1], juce::sendNotificationSync);
    };
    fxPedals[3]->setKnobValueFormatter(0, [](double v){ return (v > 0.0 ? "+" : "") + juce::String(juce::roundToInt(v)) + " st"; });
    fxPedals[3]->setKnobValueFormatter(1, [](double v){ return (v > 0.0 ? "+" : "") + juce::String(juce::roundToInt(v)) + " st"; });
    fxPedals[3]->setKnobValueFormatter(2, formatPercent01);
    fxPedals[3]->setKnobValueFormatter(3, formatMs);
    fxPedals[3]->setKnobValueFormatter(4, formatPercent01);
    effectsChain.setHarmonicInterval((int) fxPedals[3]->knob1.getValue());
    effectsChain.setHarmonicIntervalB((int) fxPedals[3]->knob2.getValue());
    effectsChain.setHarmonicMix((float) fxPedals[3]->knob3.getValue());
    effectsChain.setHarmonicLevel(1.0f);
    effectsChain.setHarmonicDelayMs((float) fxPedals[3]->knob4.getValue());
    effectsChain.setHarmonicFeedback((float) fxPedals[3]->knob5.getValue());

    fxPedals[4]->configureExtendedKnobs("BLEND", "RISE", "TAIL", "TONE", "SIZE", "DENS");
    cfg(fxPedals[4]->knob1,0.0,2.0,1.0);
    cfg(fxPedals[4]->knob2,0.0,1200.0,110.0);
    cfg(fxPedals[4]->knob3,0.0,2200.0,420.0);
    cfg(fxPedals[4]->knob4,0.0,1.0,1.0);
    cfg(fxPedals[4]->knob5,180.0,900.0,900.0);
    cfg(fxPedals[4]->knob6,0.0,1.0,1.0);
    fxPedals[4]->onBtn.onClick=[this](){ effectsChain.setFreezeEnabled(fxPedals[4]->onBtn.getToggleState()); };
    fxPedals[4]->knob1.onValueChange=[this](){ effectsChain.setFreezeMix((float)fxPedals[4]->knob1.getValue()); };
    fxPedals[4]->knob2.onValueChange=[this](){ effectsChain.setFreezeRiseMs((float)fxPedals[4]->knob2.getValue()); };
    fxPedals[4]->knob3.onValueChange=[this](){ effectsChain.setFreezeTailMs((float)fxPedals[4]->knob3.getValue()); };
    fxPedals[4]->knob4.onValueChange=[this](){ effectsChain.setFreezeTone((float)fxPedals[4]->knob4.getValue()); };
    fxPedals[4]->knob5.onValueChange=[this](){ effectsChain.setFreezeWindowMs((float)fxPedals[4]->knob5.getValue()); };
    fxPedals[4]->knob6.onValueChange=[this](){ effectsChain.setFreezeDensity((float) fxPedals[4]->knob6.getValue()); };
    fxPedals[4]->setKnobValueFormatter(0, formatBlend);
    fxPedals[4]->setKnobValueFormatter(1, formatMs);
    fxPedals[4]->setKnobValueFormatter(2, formatMs);
    fxPedals[4]->setKnobValueFormatter(3, formatPercent01);
    fxPedals[4]->setKnobValueFormatter(4, formatMs);
    fxPedals[4]->setKnobValueFormatter(5, formatPercent01);

    fxPedals[5]->configureExtendedKnobs("BLEND", "RISE", "TAIL", "TONE", "SIZE", "DENS");
    cfg(fxPedals[5]->knob1,0.0,2.0,1.0);
    cfg(fxPedals[5]->knob2,0.0,1200.0,110.0);
    cfg(fxPedals[5]->knob3,0.0,2200.0,420.0);
    cfg(fxPedals[5]->knob4,0.0,1.0,1.0);
    cfg(fxPedals[5]->knob5,180.0,900.0,900.0);
    cfg(fxPedals[5]->knob6,0.0,1.0,1.0);
    fxPedals[5]->onBtn.onClick=[this](){ effectsChain.setChordFreezeEnabled(fxPedals[5]->onBtn.getToggleState()); };
    fxPedals[5]->knob1.onValueChange=[this](){ effectsChain.setChordFreezeMix((float)fxPedals[5]->knob1.getValue()); };
    fxPedals[5]->knob2.onValueChange=[this](){ effectsChain.setChordFreezeRiseMs((float)fxPedals[5]->knob2.getValue()); };
    fxPedals[5]->knob3.onValueChange=[this](){ effectsChain.setChordFreezeTailMs((float)fxPedals[5]->knob3.getValue()); };
    fxPedals[5]->knob4.onValueChange=[this](){ effectsChain.setChordFreezeTone((float)fxPedals[5]->knob4.getValue()); };
    fxPedals[5]->knob5.onValueChange=[this](){ effectsChain.setChordFreezeWindowMs((float)fxPedals[5]->knob5.getValue()); };
    fxPedals[5]->knob6.onValueChange=[this](){ effectsChain.setChordFreezeCrossfade((float)fxPedals[5]->knob6.getValue()); };
    fxPedals[5]->setKnobValueFormatter(0, formatBlend);
    fxPedals[5]->setKnobValueFormatter(1, formatMs);
    fxPedals[5]->setKnobValueFormatter(2, formatMs);
    fxPedals[5]->setKnobValueFormatter(3, formatPercent01);
    fxPedals[5]->setKnobValueFormatter(4, formatMs);
    fxPedals[5]->setKnobValueFormatter(5, formatPercent01);

    cfg(fxPedals[6]->knob1,0,1,0.4); cfg(fxPedals[6]->knob2,0.05,1,0.4); cfg(fxPedals[6]->knob3,0,1,0.7);
    fxPedals[6]->knob1.setComponentID("pampaFuzzKnob");
    fxPedals[6]->knob2.setComponentID("pampaFuzzKnob");
    fxPedals[6]->knob3.setComponentID("pampaFuzzKnob");
    fxPedals[6]->onBtn.onClick=[this](){ effectsChain.setFuzzEnabled(fxPedals[6]->onBtn.getToggleState()); };
    fxPedals[6]->knob1.onValueChange=[this](){ effectsChain.setFuzz((float)fxPedals[6]->knob1.getValue()); };
    fxPedals[6]->knob2.onValueChange=[this](){ effectsChain.setFuzzTone((float)fxPedals[6]->knob2.getValue()); };
    fxPedals[6]->knob3.onValueChange=[this](){ effectsChain.setFuzzLevel((float)fxPedals[6]->knob3.getValue()); };

    fxPedals[7]->configureNineKnobs("LEVEL A", "DRIVE A", "TONE A",
                                    "LEVEL B", "DRIVE B", "TONE B",
                                    "TYPE A", "TYPE B", "CONFIG");
    for (auto* knob : { &fxPedals[7]->knob1, &fxPedals[7]->knob2, &fxPedals[7]->knob3,
                        &fxPedals[7]->knob4, &fxPedals[7]->knob5, &fxPedals[7]->knob6 })
        knob->setComponentID("sunsetKnob");
    fxPedals[7]->knob7.setComponentID("sunsetToggle");
    fxPedals[7]->knob8.setComponentID("sunsetToggle");
    cfg(fxPedals[7]->knob1, 0.0, 1.0, 0.52); // LEVEL A
    cfg(fxPedals[7]->knob2, 0.0, 1.0, 0.42); // DRIVE A
    cfg(fxPedals[7]->knob3, 0.0, 1.0, 0.55); // TONE A
    cfg(fxPedals[7]->knob4, 0.0, 1.0, 0.52); // LEVEL B
    cfg(fxPedals[7]->knob5, 0.0, 1.0, 0.36); // DRIVE B
    cfg(fxPedals[7]->knob6, 0.0, 1.0, 0.55); // TONE B
    fxPedals[7]->knob7.setRange(0.0, 2.0, 1.0); fxPedals[7]->knob7.setValue(1.0); // texas
    fxPedals[7]->knob8.setRange(0.0, 2.0, 1.0); fxPedals[7]->knob8.setValue(0.0); // 2stage
    fxPedals[7]->knob9.setRange(0.0, 2.0, 1.0); fxPedals[7]->knob9.setValue(0.0); // A -> B
    fxPedals[7]->knob9.setVisible(false); // CONFIG is operated on the rear/top lip.
    fxPedals[7]->knob1.setTooltip("LEVEL A");
    fxPedals[7]->knob2.setTooltip("DRIVE A");
    fxPedals[7]->knob3.setTooltip("TONE A");
    fxPedals[7]->knob4.setTooltip("LEVEL B");
    fxPedals[7]->knob5.setTooltip("DRIVE B");
    fxPedals[7]->knob6.setTooltip("TONE B");
    fxPedals[7]->knob7.setTooltip("A: Ge / texas / treble");
    fxPedals[7]->knob8.setTooltip("B: 2stage / hard / JFET");
    fxPedals[7]->onBtn.onClick=[this](){ effectsChain.setDriveAEnabled(fxPedals[7]->onBtn.getToggleState()); };
    fxPedals[7]->onTertiaryFootswitch=[this](){ effectsChain.setDriveBEnabled(fxPedals[7]->isTertiaryFootswitchLatched()); };
    fxPedals[7]->knob1.onValueChange=[this](){ effectsChain.setDriveALevel((float)fxPedals[7]->knob1.getValue()); };
    fxPedals[7]->knob2.onValueChange=[this](){ effectsChain.setDriveA((float)fxPedals[7]->knob2.getValue()); };
    fxPedals[7]->knob3.onValueChange=[this](){ effectsChain.setDriveATone((float)fxPedals[7]->knob3.getValue()); };
    fxPedals[7]->knob4.onValueChange=[this](){ effectsChain.setDriveBLevel((float)fxPedals[7]->knob4.getValue()); };
    fxPedals[7]->knob5.onValueChange=[this](){ effectsChain.setDriveB((float)fxPedals[7]->knob5.getValue()); };
    fxPedals[7]->knob6.onValueChange=[this](){ effectsChain.setDriveBTone((float)fxPedals[7]->knob6.getValue()); };
    fxPedals[7]->knob7.onValueChange=[this](){ effectsChain.setDriveAType(juce::roundToInt(fxPedals[7]->knob7.getValue())); fxPedals[7]->repaint(); };
    fxPedals[7]->knob8.onValueChange=[this](){ effectsChain.setDriveBType(juce::roundToInt(fxPedals[7]->knob8.getValue())); fxPedals[7]->repaint(); };
    fxPedals[7]->knob9.onValueChange=[this](){ effectsChain.setDriveRouting(juce::roundToInt(fxPedals[7]->knob9.getValue())); fxPedals[7]->repaint(); };
    fxPedals[7]->onSunsetBrightChange=[this](int mode){ effectsChain.setDriveBright(mode); };
    effectsChain.setDriveALevel((float)fxPedals[7]->knob1.getValue());
    effectsChain.setDriveA((float)fxPedals[7]->knob2.getValue());
    effectsChain.setDriveATone((float)fxPedals[7]->knob3.getValue());
    effectsChain.setDriveBLevel((float)fxPedals[7]->knob4.getValue());
    effectsChain.setDriveB((float)fxPedals[7]->knob5.getValue());
    effectsChain.setDriveBTone((float)fxPedals[7]->knob6.getValue());
    effectsChain.setDriveAType(juce::roundToInt(fxPedals[7]->knob7.getValue()));
    effectsChain.setDriveBType(juce::roundToInt(fxPedals[7]->knob8.getValue()));
    effectsChain.setDriveRouting(juce::roundToInt(fxPedals[7]->knob9.getValue()));
    effectsChain.setDriveBright(fxPedals[7]->getSunsetBrightMode());

    fxPedals[8]->knob1.setComponentID("smallCloneMix");
    fxPedals[8]->knob2.setComponentID("smallCloneRateKnob");
    fxPedals[8]->knob3.setComponentID("smallCloneDepthSwitch");
    cfg(fxPedals[8]->knob1,0,1,0.5); cfg(fxPedals[8]->knob2,0.1,8,1.2);
    fxPedals[8]->knob3.setRange(0.0, 1.0, 1.0); fxPedals[8]->knob3.setValue(0.0);
    fxPedals[8]->knob1.setTooltip("MIX - arrastrar sobre la placa SMALL CLONE");
    fxPedals[8]->knob2.setTooltip("RATE");
    fxPedals[8]->knob3.setTooltip("DEPTH");
    fxPedals[8]->onBtn.onClick=[this](){ effectsChain.setChorusEnabled(fxPedals[8]->onBtn.getToggleState()); };
    fxPedals[8]->knob1.onValueChange=[this](){ effectsChain.setChorusMix((float)fxPedals[8]->knob1.getValue()); };
    fxPedals[8]->knob2.onValueChange=[this](){ effectsChain.setChorusRate((float)fxPedals[8]->knob2.getValue()); };
    fxPedals[8]->knob3.onValueChange=[this](){ effectsChain.setChorusDepth((float)fxPedals[8]->knob3.getValue()); };
    fxPedals[8]->setKnobValueFormatter(0, formatPercent01);
    fxPedals[8]->setKnobValueFormatter(1, [](double v){ return juce::String(v, 2) + " Hz"; });
    fxPedals[8]->setKnobValueFormatter(2, [](double v){ return v >= 0.5 ? "HIGH" : "LOW"; });
    effectsChain.setChorusMix((float) fxPedals[8]->knob1.getValue());
    effectsChain.setChorusRate((float) fxPedals[8]->knob2.getValue());
    effectsChain.setChorusDepth((float) fxPedals[8]->knob3.getValue());

    fxPedals[9]->configureExtendedKnobs("DEPTH", "MODE", "RATE / f IN", "BEATS", "SYM / f OUT", "");
    for (auto* knob : { &fxPedals[9]->knob1, &fxPedals[9]->knob2, &fxPedals[9]->knob3,
                        &fxPedals[9]->knob4, &fxPedals[9]->knob5 })
        knob->setComponentID("dedaloJetKnob");
    fxPedals[9]->knob6.setVisible(false);
    cfg(fxPedals[9]->knob1, 0.0, 1.0, 0.72);
    fxPedals[9]->knob2.setRange(0.0, 4.0, 1.0); fxPedals[9]->knob2.setValue(3.0);
    cfg(fxPedals[9]->knob3, 0.4, 14.0, 3.2);
    fxPedals[9]->knob4.setRange(0.0, 6.0, 1.0); fxPedals[9]->knob4.setValue(2.0);
    cfg(fxPedals[9]->knob5, 0.0, 1.0, 0.5);
    fxPedals[9]->onBtn.onClick=[this](){ effectsChain.setTremoloEnabled(fxPedals[9]->onBtn.getToggleState()); };
    fxPedals[9]->knob1.onValueChange=[this](){ effectsChain.setTremoloDepth((float)fxPedals[9]->knob1.getValue()); };
    fxPedals[9]->knob2.onValueChange=[this](){ effectsChain.setTremoloMode(juce::roundToInt(fxPedals[9]->knob2.getValue())); };
    fxPedals[9]->knob3.onValueChange=[this](){ effectsChain.setTremoloRate((float)fxPedals[9]->knob3.getValue()); };
    fxPedals[9]->knob4.onValueChange=[this](){ effectsChain.setTremoloBeats(juce::roundToInt(fxPedals[9]->knob4.getValue())); };
    fxPedals[9]->knob5.onValueChange=[this](){ effectsChain.setTremoloSymmetry((float)fxPedals[9]->knob5.getValue()); };
    fxPedals[9]->onSecondaryFootswitch=[this]()
    {
        effectsChain.setTremoloControlPressed(true);
        if (juce::roundToInt(fxPedals[9]->knob2.getValue()) == 4)
            return;
        effectsChain.syncTremolo();
        const auto now = juce::Time::currentTimeMillis();
        const auto elapsed = now - lastTremoloTapMs;
        lastTremoloTapMs = now;
        if (elapsed >= 70 && elapsed <= 2500)
            fxPedals[9]->knob3.setValue(juce::jlimit(0.4, 14.0, 1000.0 / (double)elapsed),
                                        juce::sendNotificationSync);
    };
    fxPedals[9]->onSecondaryFootswitchRelease=[this]()
    {
        effectsChain.setTremoloControlPressed(false);
    };

    fxPedals[10]->configureNineKnobs("TIME", "REPEATS", "FILTER", "GRIT", "SPEED",
                                     "DEPTH", "MIX", "TYPE", "VALUE");
    for (auto* knob : { &fxPedals[10]->knob1, &fxPedals[10]->knob2, &fxPedals[10]->knob3,
                        &fxPedals[10]->knob4, &fxPedals[10]->knob5, &fxPedals[10]->knob6,
                        &fxPedals[10]->knob7, &fxPedals[10]->knob8, &fxPedals[10]->knob9 })
        knob->setComponentID("timeLineKnob");
    cfg(fxPedals[10]->knob1, 50.0, 1500.0, 400.0); // TIME
    cfg(fxPedals[10]->knob2, 0.0, 0.95, 0.40);     // REPEATS
    cfg(fxPedals[10]->knob3, 0.0, 1.0, 0.62);      // FILTER
    cfg(fxPedals[10]->knob4, 0.0, 1.0, 0.12);      // GRIT
    cfg(fxPedals[10]->knob5, 0.02, 12.0, 0.35);    // SPEED
    cfg(fxPedals[10]->knob6, 0.0, 1.0, 0.10);      // DEPTH
    cfg(fxPedals[10]->knob7, 0.0, 1.0, 0.30);      // MIX
    fxPedals[10]->knob8.setRange(0.0, 11.0, 1.0); fxPedals[10]->knob8.setValue(6.0); // TYPE
    cfg(fxPedals[10]->knob9, 0.0, 1.0, 0.50);      // VALUE
    fxPedals[10]->onBtn.onClick=[this](){ effectsChain.setDelayEnabled(fxPedals[10]->onBtn.getToggleState()); };
    fxPedals[10]->knob1.onValueChange=[this](){ effectsChain.setDelayTimeMs((float)fxPedals[10]->knob1.getValue()); fxPedals[10]->repaint(); };
    fxPedals[10]->knob2.onValueChange=[this](){ effectsChain.setDelayFeedback((float)fxPedals[10]->knob2.getValue()); };
    fxPedals[10]->knob3.onValueChange=[this](){ effectsChain.setDelayFilter((float)fxPedals[10]->knob3.getValue()); };
    fxPedals[10]->knob4.onValueChange=[this](){ effectsChain.setDelayGrit((float)fxPedals[10]->knob4.getValue()); };
    fxPedals[10]->knob5.onValueChange=[this](){ effectsChain.setDelayModRate((float)fxPedals[10]->knob5.getValue()); };
    fxPedals[10]->knob6.onValueChange=[this](){ effectsChain.setDelayModDepth((float)fxPedals[10]->knob6.getValue()); };
    fxPedals[10]->knob7.onValueChange=[this](){ effectsChain.setDelayMix((float)fxPedals[10]->knob7.getValue()); };
    fxPedals[10]->knob8.onValueChange=[this](){ effectsChain.setDelayType(juce::roundToInt(fxPedals[10]->knob8.getValue())); fxPedals[10]->repaint(); };
    fxPedals[10]->knob9.onValueChange=[this](){ effectsChain.setDelayValue((float)fxPedals[10]->knob9.getValue()); fxPedals[10]->repaint(); };
    fxPedals[10]->onSecondaryFootswitch=[this]()
    {
        const double nextValue = fxPedals[10]->knob9.getValue() < 0.5 ? 0.75 : 0.25;
        fxPedals[10]->knob9.setValue(nextValue, juce::sendNotificationSync);
    };
    fxPedals[10]->onTap=[this]()
    {
        const juce::int64 now = juce::Time::currentTimeMillis();
        const juce::int64 elapsed = now - lastDelayTapMs;
        lastDelayTapMs = now;
        if (elapsed >= 50 && elapsed <= 1500)
            fxPedals[10]->knob1.setValue((double) elapsed, juce::sendNotificationSync);
    };
    fxPedals[10]->setKnobValueFormatter(0, formatMs);
    fxPedals[10]->setKnobValueFormatter(1, formatPercent01);
    fxPedals[10]->setKnobValueFormatter(2, formatPercent01);
    fxPedals[10]->setKnobValueFormatter(3, formatPercent01);
    fxPedals[10]->setKnobValueFormatter(4, [](double v){ return juce::String(v, 2) + " Hz"; });
    fxPedals[10]->setKnobValueFormatter(5, formatPercent01);
    fxPedals[10]->setKnobValueFormatter(6, formatPercent01);
    effectsChain.setDelayTimeMs((float) fxPedals[10]->knob1.getValue());
    effectsChain.setDelayFeedback((float) fxPedals[10]->knob2.getValue());
    effectsChain.setDelayFilter((float) fxPedals[10]->knob3.getValue());
    effectsChain.setDelayGrit((float) fxPedals[10]->knob4.getValue());
    effectsChain.setDelayModRate((float) fxPedals[10]->knob5.getValue());
    effectsChain.setDelayModDepth((float) fxPedals[10]->knob6.getValue());
    effectsChain.setDelayMix((float) fxPedals[10]->knob7.getValue());
    effectsChain.setDelayType(juce::roundToInt(fxPedals[10]->knob8.getValue()));
    effectsChain.setDelayValue((float) fxPedals[10]->knob9.getValue());

    fxPedals[11]->configureNineKnobs("DECAY", "PRE-DELAY", "MIX", "TONE", "PARAM 1",
                                        "PARAM 2", "MOD", "TYPE", "VALUE");
    for (auto* knob : { &fxPedals[11]->knob1, &fxPedals[11]->knob2, &fxPedals[11]->knob3,
                        &fxPedals[11]->knob4, &fxPedals[11]->knob5, &fxPedals[11]->knob6,
                        &fxPedals[11]->knob7, &fxPedals[11]->knob8, &fxPedals[11]->knob9 })
        knob->setComponentID("bigSkyKnob");
    cfg(fxPedals[11]->knob1, 0.05, 1.0, 0.55);       // DECAY
    cfg(fxPedals[11]->knob2, 0.0, 1500.0, 80.0);    // PRE-DELAY
    cfg(fxPedals[11]->knob3, 0.0, 1.0, 0.28);       // MIX
    cfg(fxPedals[11]->knob4, 0.0, 1.0, 0.58);       // TONE
    cfg(fxPedals[11]->knob5, 0.0, 1.0, 0.85);       // PARAM 1 / WIDTH
    cfg(fxPedals[11]->knob6, 0.0, 1.0, 0.65);       // PARAM 2 / DIFFUSION
    cfg(fxPedals[11]->knob7, 0.0, 1.0, 0.18);       // MOD
    fxPedals[11]->knob8.setRange(0.0, 11.0, 1.0); fxPedals[11]->knob8.setValue(1.0); // TYPE
    cfg(fxPedals[11]->knob9, 0.0, 1.0, 0.5);        // VALUE
    fxPedals[11]->onBtn.onClick=[this](){ effectsChain.setReverbEnabled(fxPedals[11]->onBtn.getToggleState()); };
    fxPedals[11]->knob1.onValueChange=[this](){ effectsChain.setReverbRoomSize((float)fxPedals[11]->knob1.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob2.onValueChange=[this](){ effectsChain.setReverbPreDelayMs((float)fxPedals[11]->knob2.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob3.onValueChange=[this](){ effectsChain.setReverbMix((float)fxPedals[11]->knob3.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob4.onValueChange=[this](){ const float tone=(float)fxPedals[11]->knob4.getValue(); effectsChain.setReverbTone(tone); effectsChain.setReverbDamping(1.0f-tone); fxPedals[11]->repaint(); };
    fxPedals[11]->knob5.onValueChange=[this](){ effectsChain.setReverbWidth((float)fxPedals[11]->knob5.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob6.onValueChange=[this](){ effectsChain.setReverbDiffusion((float)fxPedals[11]->knob6.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob7.onValueChange=[this](){ effectsChain.setReverbMod((float)fxPedals[11]->knob7.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->knob8.onValueChange=[this](){ effectsChain.setReverbType(juce::roundToInt(fxPedals[11]->knob8.getValue())); fxPedals[11]->repaint(); };
    fxPedals[11]->knob9.onValueChange=[this](){ effectsChain.setReverbValue((float)fxPedals[11]->knob9.getValue()); fxPedals[11]->repaint(); };
    fxPedals[11]->onSecondaryFootswitch=[this]()
    {
        const int type = juce::roundToInt(fxPedals[11]->knob8.getValue());
        fxPedals[11]->knob8.setValue((type + 11) % 12, juce::sendNotificationSync);
    };
    fxPedals[11]->onTertiaryFootswitch=[this]()
    {
        effectsChain.setReverbInfinite(fxPedals[11]->isTertiaryFootswitchLatched());
    };
    effectsChain.setReverbRoomSize((float)fxPedals[11]->knob1.getValue());
    effectsChain.setReverbPreDelayMs((float)fxPedals[11]->knob2.getValue());
    effectsChain.setReverbMix((float)fxPedals[11]->knob3.getValue());
    effectsChain.setReverbTone((float)fxPedals[11]->knob4.getValue());
    effectsChain.setReverbDamping(1.0f - (float)fxPedals[11]->knob4.getValue());
    effectsChain.setReverbWidth((float)fxPedals[11]->knob5.getValue());
    effectsChain.setReverbDiffusion((float)fxPedals[11]->knob6.getValue());
    effectsChain.setReverbMod((float)fxPedals[11]->knob7.getValue());
    effectsChain.setReverbType(juce::roundToInt(fxPedals[11]->knob8.getValue()));
    effectsChain.setReverbValue((float)fxPedals[11]->knob9.getValue());

    fxPedals[12]->configureTenBandGraphicEq();
    cfg(fxPedals[12]->eqVolumeSlider, -12.0, 12.0, 0.0);
    cfg(fxPedals[12]->eqGainSlider, -12.0, 12.0, 0.0);
    fxPedals[12]->eqVolumeSlider.onValueChange=[this]()
    {
        effectsChain.setEqOutputVolume((float)fxPedals[12]->eqVolumeSlider.getValue());
        fxPedals[12]->repaint();
    };
    fxPedals[12]->eqGainSlider.onValueChange=[this]()
    {
        effectsChain.setEqInputGain((float)fxPedals[12]->eqGainSlider.getValue());
        fxPedals[12]->repaint();
    };
    fxPedals[12]->onBtn.onClick=[this]()
    {
        const bool active = fxPedals[12]->onBtn.getToggleState();
        effectsChain.setEqEnabled(active);
        for (auto& slider : fxPedals[12]->eqBandSliders)
            slider.setColour(juce::Slider::trackColourId,
                             active ? juce::Colour(0xff29b7ff) : juce::Colour(0xff18506b));
        fxPedals[12]->eqVolumeSlider.setColour(juce::Slider::trackColourId,
                                               active ? juce::Colour(0xff29b7ff) : juce::Colour(0xff18506b));
        fxPedals[12]->eqGainSlider.setColour(juce::Slider::trackColourId,
                                             active ? juce::Colour(0xff29b7ff) : juce::Colour(0xff18506b));
        fxPedals[12]->repaint();
    };
    for (int band = 0; band < 10; ++band)
    {
        auto& slider = fxPedals[12]->eqBandSliders[(size_t)band];
        cfg(slider, -12.0, 12.0, 0.0);
        slider.onValueChange = [this, band]()
        {
            effectsChain.setEqBandGain(band, (float)fxPedals[12]->eqBandSliders[(size_t)band].getValue());
            fxPedals[12]->repaint();
        };
    }

}

int MainComponent::findFxPedalIndex(const FXPedalComponent& pedal) const
{
    for (int i = 0; i < kNumFxPedals; ++i)
        if (fxPedals[i].get() == &pedal)
            return i;

    return -1;
}

int MainComponent::fxPedalBodyWidth(int pedalIndex) const
{
    if (pedalIndex == 1) return 180;
    if (pedalIndex == 2) return 245;
    if (pedalIndex == 9) return 245;
    if (pedalIndex == 3) return 310;
    if (pedalIndex == 6) return 270;
    if (pedalIndex == 7) return 270;
    if (pedalIndex == 8) return 158;
    if (pedalIndex == 12) return 270;
    if (pedalIndex == 11) return 320;
    if (pedalIndex == 10) return 330;
    return 156;
}

int MainComponent::fxPedalPitch(int pedalIndex) const
{
    return fxPedalBodyWidth(pedalIndex) + 28;
}

void MainComponent::revealFxPedal(int pedalIndex)
{
    if (pedalIndex < 0 || pedalIndex >= kNumFxPedals || fxPedals[pedalIndex] == nullptr)
        return;

    const int centredX = fxPedals[pedalIndex]->getBounds().getCentreX() - fxViewport.getWidth() / 2;
    fxViewport.setViewPosition(juce::jlimit(0, juce::jmax(0, fxContainer.getWidth() - fxViewport.getWidth()), centredX), 0);
}

void MainComponent::focusFxPedalForSnapshot(int pedalIndex)
{
    if (pedalIndex == -1 && vst3Pedal != nullptr)
    {
        toggleFxPedalFocus(*vst3Pedal);
        return;
    }

    if (pedalIndex >= 0 && pedalIndex < kNumFxPedals && fxPedals[pedalIndex] != nullptr)
        toggleFxPedalFocus(*fxPedals[pedalIndex]);
}

void MainComponent::setVst3DisplayTextForSnapshot(const juce::String& text)
{
    lblVst3Status.setDisplayText(text);
}

void MainComponent::toggleFxPedalFocus(FXPedalComponent& pedal)
{
    if (fxPedalFocusOverlay.getPedal() == &pedal)
    {
        closeFxPedalFocus();
        return;
    }

    if (fxPedalFocusOverlay.getPedal() != nullptr)
        closeFxPedalFocus();

    int pedalIndex = findFxPedalIndex(pedal);
    if (&pedal == vst3Pedal.get())
        pedalIndex = -1;
    else if (pedalIndex < 0)
        return;

    if (draggingFxPedal >= 0)
    {
        fxPedals[draggingFxPedal]->setDragHintVisible(false);
        draggingFxPedal = -1;
        dragInsertSlot = -1;
    }

    focusedFxPedalIndex = pedalIndex;
    const int compactWidth = pedalIndex < 0 ? kVst3PedalBodyWidth : fxPedalBodyWidth(pedalIndex);
    const int compactHeight = juce::jmax(1, fxContainer.getHeight() - 8);
    fxPedalFocusOverlay.setBounds(getLocalBounds());
    fxPedalFocusOverlay.showPedal(pedal, compactWidth, compactHeight);
    resized();
    fxPedalFocusOverlay.toFront(true);
    fxContainer.repaint();
}

void MainComponent::closeFxPedalFocus()
{
    auto* pedal = fxPedalFocusOverlay.getPedal();
    if (pedal == nullptr)
        return;

    const int previousIndex = focusedFxPedalIndex;
    fxContainer.addAndMakeVisible(pedal);
    fxPedalFocusOverlay.clearPedal();
    focusedFxPedalIndex = -2;
    resized();
    fxContainer.repaint();

    if (previousIndex >= 0)
        revealFxPedal(previousIndex);
}

int MainComponent::fxSlotStartX(int slot) const
{
    int x = kVst3PedalPitch;
    for (int i = 0; i < juce::jlimit(0, kNumFxPedals, slot); ++i)
        x += fxPedalPitch(fxOrder[(size_t) i]);
    return x;
}

int MainComponent::fxSlotForX(int x) const
{
    for (int slot = 0; slot < kNumFxPedals; ++slot)
    {
        const int start = fxSlotStartX(slot);
        const int end = start + fxPedalPitch(fxOrder[(size_t) slot]);
        if (x < (start + end) / 2)
            return slot;
    }
    return kNumFxPedals - 1;
}

void MainComponent::moveFxPedalToSlot(int pedalIndex, int targetSlot)
{
    targetSlot = juce::jlimit(0, kNumFxPedals - 1, targetSlot);

    int currentSlot = -1;
    for (int i = 0; i < kNumFxPedals; ++i)
    {
        if (fxOrder[(size_t) i] == pedalIndex)
        {
            currentSlot = i;
            break;
        }
    }

    if (currentSlot < 0 || currentSlot == targetSlot)
        return;

    if (currentSlot < targetSlot)
    {
        for (int i = currentSlot; i < targetSlot; ++i)
            fxOrder[(size_t) i] = fxOrder[(size_t) (i + 1)];
    }
    else
    {
        for (int i = currentSlot; i > targetSlot; --i)
            fxOrder[(size_t) i] = fxOrder[(size_t) (i - 1)];
    }

    fxOrder[(size_t) targetSlot] = pedalIndex;
    effectsChain.setPedalOrder(fxOrder);
}

void MainComponent::beginFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e)
{
    draggingFxPedal = findFxPedalIndex(pedal);
    if (draggingFxPedal < 0)
        return;

    auto relativeEvent = e.getEventRelativeTo(&fxContainer);
    dragInsertSlot = fxSlotForX((int) relativeEvent.position.x);
    pedal.setDragHintVisible(true);
    pedal.toFront(false);
}

void MainComponent::updateFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e)
{
    if (draggingFxPedal != findFxPedalIndex(pedal))
        return;

    auto relativeEvent = e.getEventRelativeTo(&fxContainer);
    dragInsertSlot = fxSlotForX((int) relativeEvent.position.x);

    auto bounds = pedal.getBounds();
    bounds.setCentre((int) relativeEvent.position.x, bounds.getCentreY());
    bounds.setX(juce::jlimit(kVst3PedalPitch + 5,
                             juce::jmax(kVst3PedalPitch + 5,
                                        fxContainer.getWidth() - bounds.getWidth() - 5),
                             bounds.getX()));
    pedal.setBounds(bounds);
}

void MainComponent::endFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e)
{
    const int pedalIndex = findFxPedalIndex(pedal);
    if (draggingFxPedal == pedalIndex && pedalIndex >= 0)
    {
        auto relativeEvent = e.getEventRelativeTo(&fxContainer);
        dragInsertSlot = fxSlotForX((int) relativeEvent.position.x);
        moveFxPedalToSlot(pedalIndex, dragInsertSlot);
    }

    pedal.setDragHintVisible(false);
    draggingFxPedal = -1;
    dragInsertSlot = -1;
    resized();
}

void MainComponent::layoutVst3PedalControls(FXPedalComponent& pedal)
{
    auto body = pedal.getLocalBounds().toFloat().reduced(3.0f);
    const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / 1.20f);
    body = { body.getX(), body.getCentreY() - bodyHeight * 0.5f,
             body.getWidth(), bodyHeight };

    // These proportions match the cropped model-host-face-v2 artwork. Keeping every
    // coordinate relative to the same body rectangle prevents the live display from
    // drifting away from its recess when the focus overlay enlarges the pedal.
    const auto screen = juce::Rectangle<float>(body.getX() + body.getWidth() * 0.168f,
                                                body.getY() + body.getHeight() * 0.155f,
                                                body.getWidth() * 0.664f,
                                                body.getHeight() * 0.334f);
    lblVst3Status.setBounds(screen.toNearestInt());

    const int buttonGap = juce::jmax(3, juce::roundToInt(body.getWidth() * 0.020f));
    const int contentX = juce::roundToInt(body.getX() + body.getWidth() * 0.090f);
    const int contentW = juce::roundToInt(body.getWidth() * 0.820f);
    const int buttonY = juce::roundToInt(body.getY() + body.getHeight() * 0.025f);
    const int buttonH = juce::jmax(18, juce::roundToInt(body.getHeight() * 0.090f));
    const int buttonW = (contentW - buttonGap * 2) / 3;
    btnLoadVst3.setBounds(contentX, buttonY, buttonW, buttonH);
    btnEditVst3.setBounds(contentX + buttonW + buttonGap, buttonY, buttonW, buttonH);
    btnUnloadVst3.setBounds(contentX + (buttonW + buttonGap) * 2, buttonY,
                            contentW - buttonW * 2 - buttonGap * 2, buttonH);
    btnBypassVst3.setBounds(0, 0, 0, 0);
}

void MainComponent::styleVstButton(juce::TextButton& button, juce::Colour accent)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.24f));
    button.setColour(juce::TextButton::textColourOffId, LnfC::TextHi);
}

void MainComponent::loadExternalVst3Plugin()
{
    juce::File defaultFolder = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
        .getChildFile("Common Files")
        .getChildFile("VST3");

    vst3FileChooser = std::make_unique<juce::FileChooser>("Seleccionar plugin VST3",
                                                          defaultFolder.exists() ? defaultFolder : juce::File(),
                                                          "*.vst3");

    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;

    vst3FileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto selectedFile = chooser.getResult();
        vst3FileChooser.reset();

        if (selectedFile == juce::File())
            return;

        EffectsChain::PreparedExternalPlugin candidate;
        juce::String errorMessage;
        if (!effectsChain.prepareExternalVst3(selectedFile, candidate, errorMessage))
        {
            updateExternalVstUi();
            lblVst3Status.setDisplayText(errorMessage);
            return;
        }

        closeExternalVst3Editor();
        EffectsChain::PreparedExternalPlugin retired;
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            retired = effectsChain.commitExternalPlugin(std::move(candidate));
        }

        updateExternalVstUi();
    });
}

void MainComponent::unloadExternalVst3Plugin()
{
    closeExternalVst3Editor();

    EffectsChain::PreparedExternalPlugin retired;
    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        retired = effectsChain.detachExternalPlugin();
    }

    updateExternalVstUi();
}

void MainComponent::openExternalVst3Editor()
{
    auto* plugin = effectsChain.getExternalPluginInstance();
    if (plugin == nullptr || !plugin->hasEditor())
        return;

    if (vst3EditorWindow != nullptr)
    {
        vst3EditorWindow->toFront(true);
        vst3EditorWindow->setVisible(true);
        return;
    }

    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorAndMakeActive());
    if (editor == nullptr)
        return;

    vst3EditorWindow = std::make_unique<ExternalPluginEditorWindow>("VST3 - " + effectsChain.getExternalPluginName(),
                                                                    std::move(editor));
    vst3EditorWindow->setVisible(true);
}

void MainComponent::closeExternalVst3Editor()
{
    vst3EditorWindow.reset();
}

void MainComponent::updateExternalVstUi()
{
    const bool hasPlugin = effectsChain.hasExternalPlugin();
    const bool pluginEnabled = effectsChain.isExternalPluginEnabled();
    const auto pluginName = effectsChain.getExternalPluginName();

    btnUnloadVst3.setEnabled(hasPlugin);
    btnEditVst3.setEnabled(hasPlugin);
    btnBypassVst3.setEnabled(hasPlugin);
    btnBypassVst3.setToggleState(pluginEnabled, juce::dontSendNotification);
    btnBypassVst3.setButtonText(pluginEnabled ? "ENGINE ACTIVE" : "ENGINE BYPASS");
    if (vst3Pedal != nullptr)
    {
        vst3Pedal->onBtn.setToggleState(hasPlugin && pluginEnabled, juce::dontSendNotification);
        vst3Pedal->repaint();
    }

    if (!hasPlugin)
        lblVst3Status.setDisplayText("SIN VST");
    else if (pluginEnabled)
        lblVst3Status.setDisplayText(pluginName);
    else
        lblVst3Status.setDisplayText("BYPASS - " + pluginName);

    lblVst3Status.setColour(juce::Label::textColourId,
                            !hasPlugin ? LnfC::TextDim
                                       : (pluginEnabled ? juce::Colour(0xff87edf2)
                                                        : LnfC::OvdAmber));
    lblVst3Status.setColour(juce::Label::outlineColourId,
                            (pluginEnabled ? juce::Colour(0xff55dce5)
                                           : LnfC::OvdAmber).withAlpha(hasPlugin ? 0.52f : 0.20f));
}

bool MainComponent::captureLooperSnapshotIncrementally(LooperEngine::SessionSnapshot& snapshot,
                                                       juce::String& errorMessage)
{
    constexpr int kChunkSamplesPerChannel = 4096;
    constexpr int kMaxAttempts = 3;

    for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
    {
        LooperEngine::IncrementalSnapshot capture;
        LooperEngine::SnapshotCaptureResult result;
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            result = looper.beginIncrementalSnapshot(capture);
        }

        if (result == LooperEngine::SnapshotCaptureResult::retryRequired)
        {
            const auto state = looper.getState();
            if (state == LooperEngine::State::RecordingBase
                || state == LooperEngine::State::RecordingLayer)
            {
                errorMessage = "Finaliza o cancela la grabacion antes de guardar o exportar.";
                return false;
            }
            continue;
        }

        LooperEngine::prepareIncrementalSnapshot(capture);
        do
        {
            {
                const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
                result = looper.copyIncrementalSnapshotChunk(capture,
                                                              kChunkSamplesPerChannel);
            }
            if (result == LooperEngine::SnapshotCaptureResult::inProgress)
                juce::Thread::yield();
        }
        while (result == LooperEngine::SnapshotCaptureResult::inProgress);

        if (result == LooperEngine::SnapshotCaptureResult::complete)
        {
            snapshot = std::move(capture.snapshot);
            errorMessage.clear();
            return true;
        }
    }

    errorMessage = "El loop cambio durante la captura. Intenta nuevamente cuando este estable.";
    return false;
}

void MainComponent::saveSessionToFile()
{
    sessionFileChooser = std::make_unique<juce::FileChooser>("Guardar sesion",
                                                             juce::File(),
                                                             "*.loopernico");

    const auto chooserFlags = juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting;

    sessionFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        juce::File selectedFile = chooser.getResult();
        sessionFileChooser.reset();

        if (selectedFile == juce::File())
            return;

        if (!selectedFile.hasFileExtension("loopernico"))
            selectedFile = selectedFile.withFileExtension(".loopernico");

        LooperEngine::SessionSnapshot looperSnapshot;
        juce::MemoryBlock vstState;
        juce::String snapshotError;
        if (!captureLooperSnapshotIncrementally(looperSnapshot, snapshotError))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "No se pudo capturar la sesion",
                                                   snapshotError);
            return;
        }

        juce::AudioPluginInstance* suspendedPlugin = nullptr;
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            suspendedPlugin = effectsChain.getExternalPluginInstance();
            if (suspendedPlugin != nullptr)
                suspendedPlugin->suspendProcessing(true);
        }

        effectsChain.copyExternalPluginState(vstState);

        if (suspendedPlugin != nullptr)
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (effectsChain.getExternalPluginInstance() == suspendedPlugin)
                suspendedPlugin->suspendProcessing(false);
        }

        const auto sessionTree = createSessionTree(looperSnapshot, vstState);
        juce::String errorMessage;
        if (!SessionManager::writeSessionFile(selectedFile, sessionTree, looperSnapshot, vstState, errorMessage))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "No se pudo guardar",
                                                   errorMessage);
            return;
        }

        lblMidiStatus.setText("Sesion guardada: " + selectedFile.getFileName(),
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    });
}

void MainComponent::loadSessionFromFile()
{
    sessionFileChooser = std::make_unique<juce::FileChooser>("Abrir sesion",
                                                             juce::File(),
                                                             "*.loopernico");

    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;

    sessionFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto selectedFile = chooser.getResult();
        sessionFileChooser.reset();

        if (selectedFile == juce::File())
            return;

        openSessionFile(selectedFile);
    });
}

bool MainComponent::openSessionFile(const juce::File& sessionFile)
{
    if (!sessionFile.existsAsFile() || !sessionFile.hasFileExtension("loopernico"))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "No se pudo abrir",
                                               "El archivo de sesion no existe o no tiene la extension .loopernico.");
        return false;
    }

    juce::ValueTree sessionTree;
    LooperEngine::SessionSnapshot looperSnapshot;
    juce::MemoryBlock vstState;
    juce::String errorMessage;

    if (!SessionManager::readSessionFile(sessionFile, sessionTree, looperSnapshot, vstState, errorMessage))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "No se pudo abrir",
                                               errorMessage);
        return false;
    }

    if (!applySessionTree(sessionTree, looperSnapshot, vstState, errorMessage))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "No se pudo restaurar",
                                               errorMessage);
        return false;
    }

    lblMidiStatus.setText("Sesion cargada: " + sessionFile.getFileName(),
                          juce::dontSendNotification);
    lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    return true;
}

void MainComponent::exportMixToFile()
{
    if (looper.getBaseLoopSamples() <= 0)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "No hay audio para exportar",
                                               "Graba o carga un loop antes de exportar la mezcla.");
        return;
    }

    exportFileChooser = std::make_unique<juce::FileChooser>("Exportar mezcla WAV", juce::File(), "*.wav");
    const auto chooserFlags = juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting;
    exportFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        exportFileChooser.reset();
        if (file == juce::File()) return;
        if (!file.hasFileExtension("wav")) file = file.withFileExtension(".wav");

        LooperEngine::SessionSnapshot snapshot;
        juce::String snapshotError;
        if (!captureLooperSnapshotIncrementally(snapshot, snapshotError))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "No se pudo exportar",
                                                   snapshotError);
            return;
        }
        LooperEngine::exportFullMix(snapshot, file,
                                    outputMono.load(std::memory_order_relaxed));
        lblMidiStatus.setText("Mezcla WAV exportada: " + file.getFileName(), juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    });
}

void MainComponent::exportTracksToFolder()
{
    if (looper.getTotalTracks() <= 0 || looper.getBaseLoopSamples() <= 0)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "No hay pistas para exportar",
                                               "Graba o carga un loop antes de exportar las pistas.");
        return;
    }

    exportFileChooser = std::make_unique<juce::FileChooser>("Elegir carpeta para las pistas WAV");
    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectDirectories;
    exportFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto folder = chooser.getResult();
        exportFileChooser.reset();
        if (folder == juce::File()) return;

        LooperEngine::SessionSnapshot snapshot;
        juce::String snapshotError;
        if (!captureLooperSnapshotIncrementally(snapshot, snapshotError))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "No se pudo exportar",
                                                   snapshotError);
            return;
        }
        LooperEngine::exportSeparateTracks(snapshot, folder,
                                           outputMono.load(std::memory_order_relaxed));
        lblMidiStatus.setText("Pistas WAV exportadas en: " + folder.getFileName(), juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    });
}

juce::ValueTree MainComponent::createSessionTree(const LooperEngine::SessionSnapshot& looperSnapshot,
                                                 const juce::MemoryBlock& vstState) const
{
    juce::ValueTree root("LOOPERNICO_SESSION");
    root.setProperty("version", SessionManager::currentVersion, nullptr);
    root.setProperty("savedAt", juce::Time::getCurrentTime().toISO8601(true), nullptr);

    juce::ValueTree global("GLOBAL");
    global.setProperty("masterVolume", masterKnob.getValue(), nullptr);
    global.setProperty("outputMono", outputMono.load(std::memory_order_relaxed), nullptr);
    root.addChild(global, -1, nullptr);

    juce::ValueTree looperState("LOOPER");
    looperState.setProperty("state", (int) looperSnapshot.state, nullptr);
    looperState.setProperty("totalTracks", looperSnapshot.totalTracks, nullptr);
    looperState.setProperty("baseLoopLengthSamples", looperSnapshot.baseLoopLengthSamples, nullptr);
    looperState.setProperty("loopReadPosition", looperSnapshot.loopReadPosition, nullptr);
    looperState.setProperty("recordWritePosition", looperSnapshot.recordWritePosition, nullptr);
    looperState.setProperty("recordStartPosition", looperSnapshot.recordStartPosition, nullptr);
    looperState.setProperty("editableLoopSourceSamples", looperSnapshot.editableLoopSourceSamples, nullptr);
    looperState.setProperty("globalMute", looperSnapshot.globalMute, nullptr);
    looperState.setProperty("globalPaused", looperSnapshot.globalPaused, nullptr);
    looperState.setProperty("silenceThresholdDb", looperSnapshot.silenceThresholdDb, nullptr);
    looperState.setProperty("bpm", looperSnapshot.bpm, nullptr);
    looperState.setProperty("beatsPerBar", looperSnapshot.beatsPerBar, nullptr);
    looperState.setProperty("barsPerLoop", looperSnapshot.barsPerLoop, nullptr);
    looperState.setProperty("quantizeEnabled", looperSnapshot.quantizeEnabled, nullptr);
    looperState.setProperty("redoCount", looperSnapshot.redoCount, nullptr);
    looperState.setProperty("sampleRate", looperSnapshot.sampleRate, nullptr);

    for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
    {
        const auto& track = looperSnapshot.tracks[(size_t) i];
        juce::ValueTree trackNode("TRACK");
        trackNode.setProperty("index", i, nullptr);
        trackNode.setProperty("lengthSamples", track.lengthSamples, nullptr);
        trackNode.setProperty("redoLengthSamples", track.redoLengthSamples, nullptr);
        trackNode.setProperty("volume", track.volume, nullptr);
        trackNode.setProperty("isMuted", track.isMuted, nullptr);
        trackNode.setProperty("isPaused", track.isPaused, nullptr);
        trackNode.setProperty("pausedReadPos", track.pausedReadPos, nullptr);
        trackNode.setProperty("frippDecay", track.frippDecay, nullptr);
        trackNode.setProperty("frippEnabled", track.frippEnabled, nullptr);
        trackNode.setProperty("currentGain", track.currentGain, nullptr);
        trackNode.setProperty("audioSamples", track.audio.getNumSamples(), nullptr);
        trackNode.setProperty("soloActive",
                              i < (int) visualTracks.size() ? visualTracks[(size_t) i]->isSoloActive() : false,
                              nullptr);
        looperState.addChild(trackNode, -1, nullptr);
    }
    root.addChild(looperState, -1, nullptr);

    juce::ValueTree fx("FX");
    fx.setProperty("outputMono", outputMono.load(std::memory_order_relaxed), nullptr);
    fx.setProperty("rhythmStyle", rhythmStyleSelector.getSelectedItemIndex(), nullptr);
    fx.setProperty("rhythmPattern", rhythmPatternSelector.getSelectedItemIndex(), nullptr);
    fx.setProperty("rhythmMode", rhythmModeSelector.getSelectedItemIndex(), nullptr);
    fx.setProperty("rhythmVerseBars", rhythmVerseBarsSelector.getSelectedId(), nullptr);
    fx.setProperty("rhythmChorusBars", rhythmChorusBarsSelector.getSelectedId(), nullptr);
    fx.setProperty("rhythmCountInBars", rhythmCountInBarsSelector.getSelectedId(), nullptr);
    fx.setProperty("rhythmCountInSound", rhythmCountInSoundSelector.getSelectedId(), nullptr);
    fx.setProperty("rhythmCountInToggle", btnRhythmCountInToggle.getToggleState(), nullptr);
    for (int i = 0; i < kNumFxPedals; ++i)
    {
        juce::ValueTree pedal("PEDAL");
        pedal.setProperty("index", i, nullptr);
        pedal.setProperty("slot", (int) (std::find(fxOrder.begin(), fxOrder.end(), i) - fxOrder.begin()), nullptr);
        pedal.setProperty("on", fxPedals[i]->onBtn.getToggleState(), nullptr);
        pedal.setProperty("knob1", fxPedals[i]->knob1.getValue(), nullptr);
        pedal.setProperty("knob2", fxPedals[i]->knob2.getValue(), nullptr);
        pedal.setProperty("knob3", fxPedals[i]->knob3.getValue(), nullptr);
        pedal.setProperty("knob4", fxPedals[i]->knob4.getValue(), nullptr);
        pedal.setProperty("knob5", fxPedals[i]->knob5.getValue(), nullptr);
        pedal.setProperty("knob6", fxPedals[i]->knob6.getValue(), nullptr);
        pedal.setProperty("knob7", fxPedals[i]->knob7.getValue(), nullptr);
        pedal.setProperty("knob8", fxPedals[i]->knob8.getValue(), nullptr);
        pedal.setProperty("knob9", fxPedals[i]->knob9.getValue(), nullptr);
        if (i == 7)
        {
            pedal.setProperty("sunsetVersion", 1, nullptr);
            pedal.setProperty("sunsetBOn", fxPedals[i]->isTertiaryFootswitchLatched(), nullptr);
            pedal.setProperty("sunsetBright", fxPedals[i]->getSunsetBrightMode(), nullptr);
        }
        if (i == 12)
        {
            pedal.setProperty("eqVolume", fxPedals[i]->eqVolumeSlider.getValue(), nullptr);
            pedal.setProperty("eqGain", fxPedals[i]->eqGainSlider.getValue(), nullptr);
            for (int band = 0; band < 10; ++band)
                pedal.setProperty("eqBand" + juce::String(band + 1),
                                  fxPedals[i]->eqBandSliders[(size_t)band].getValue(), nullptr);
        }
        fx.addChild(pedal, -1, nullptr);
    }

    juce::ValueTree vst("VST");
    vst.setProperty("path", effectsChain.getExternalPluginPath(), nullptr);
    vst.setProperty("enabled", effectsChain.isExternalPluginEnabled(), nullptr);
    vst.setProperty("hasState", vstState.getSize() > 0, nullptr);
    fx.addChild(vst, -1, nullptr);
    root.addChild(fx, -1, nullptr);

    return root;
}


bool MainComponent::applySessionTree(const juce::ValueTree& sessionTree,
                                     const LooperEngine::SessionSnapshot& looperSnapshot,
                                     const juce::MemoryBlock& vstState,
                                     juce::String& errorMessage)
{
    auto setSlider = [](juce::Slider& slider, double value)
    {
        slider.setValue(value, juce::dontSendNotification);
        if (slider.onValueChange)
            slider.onValueChange();
    };

    auto setCombo = [](juce::ComboBox& box, int selectedId)
    {
        box.setSelectedId(selectedId, juce::dontSendNotification);
        if (box.onChange)
            box.onChange();
    };

    auto setToggle = [](auto& button, bool state)
    {
        button.setToggleState(state, juce::dontSendNotification);
        if (button.onClick)
            button.onClick();
    };

    const auto global = sessionTree.getChildWithName("GLOBAL");
    const auto fx = sessionTree.getChildWithName("FX");
    const auto looperNode = sessionTree.getChildWithName("LOOPER");
    const auto vst = fx.getChildWithName("VST");

    const juce::String pluginPath = vst.getProperty("path").toString();
    const bool pluginEnabled = (bool) vst.getProperty("enabled", true);
    EffectsChain::PreparedExternalPlugin candidate;
    if (pluginPath.isNotEmpty())
    {
        juce::String pluginError;
        if (!effectsChain.prepareExternalVst3(juce::File(pluginPath), candidate,
                                              pluginError, &vstState))
        {
            errorMessage = pluginError;
            return false;
        }
    }
    auto preparedLooper = looper.prepareSessionRestore(looperSnapshot);

    setSlider(masterKnob, (double) global.getProperty("masterVolume", 0.85));
    outputMono.store((bool) global.getProperty("outputMono", true), std::memory_order_relaxed);
    updateOutputModeButton();
    std::array<int, kNumFxPedals> loadedOrder{};
    loadedOrder.fill(0);
    std::array<bool, kNumFxPedals> seen{};
    for (int i = 0; i < fx.getNumChildren(); ++i)
    {
        const auto pedal = fx.getChild(i);
        if (!pedal.hasType("PEDAL"))
            continue;

        const int pedalIndex = juce::jlimit(0, kNumFxPedals - 1, (int) pedal.getProperty("index", 0));
        int slot = (int) pedal.getProperty("slot", pedalIndex);
        slot = juce::jlimit(0, kNumFxPedals - 1, slot);
        while (seen[(size_t) slot])
            slot = (slot + 1) % kNumFxPedals;
        loadedOrder[(size_t) slot] = pedalIndex;
        seen[(size_t) slot] = true;
    }
    for (int slot = 0, nextPedal = 0; slot < kNumFxPedals; ++slot)
    {
        if (seen[(size_t) slot])
            continue;
        while (nextPedal < kNumFxPedals && std::find(loadedOrder.begin(), loadedOrder.end(), nextPedal) != loadedOrder.end())
            ++nextPedal;
        loadedOrder[(size_t) slot] = juce::jlimit(0, kNumFxPedals - 1, nextPedal);
    }
    fxOrder = loadedOrder;
    refreshFxUiOrder();

    for (int i = 0; i < fx.getNumChildren(); ++i)
    {
        const auto pedal = fx.getChild(i);
        if (!pedal.hasType("PEDAL"))
            continue;

        const int pedalIndex = juce::jlimit(0, kNumFxPedals - 1, (int) pedal.getProperty("index", 0));
        auto* pedalUi = fxPedals[pedalIndex].get();
        if (pedalUi == nullptr)
            continue;

        setSlider(pedalUi->knob1, (double) pedal.getProperty("knob1", pedalUi->knob1.getValue()));
        setSlider(pedalUi->knob2, (double) pedal.getProperty("knob2", pedalUi->knob2.getValue()));
        setSlider(pedalUi->knob3, (double) pedal.getProperty("knob3", pedalUi->knob3.getValue()));
        setSlider(pedalUi->knob4, (double) pedal.getProperty("knob4", pedalUi->knob4.getValue()));
        setSlider(pedalUi->knob5, (double) pedal.getProperty("knob5", pedalUi->knob5.getValue()));
        setSlider(pedalUi->knob6, (double) pedal.getProperty("knob6", pedalUi->knob6.getValue()));
        setSlider(pedalUi->knob7, (double) pedal.getProperty("knob7", pedalUi->knob7.getValue()));
        setSlider(pedalUi->knob8, (double) pedal.getProperty("knob8", pedalUi->knob8.getValue()));
        setSlider(pedalUi->knob9, (double) pedal.getProperty("knob9", pedalUi->knob9.getValue()));
        if (pedalIndex == 7)
        {
            if (!pedal.hasProperty("sunsetVersion"))
            {
                // Mirror Drive sessions used knob1=drive, knob2=tone and
                // knob3=level.  Translate them into Sunset channel A once.
                setSlider(pedalUi->knob1, (double) pedal.getProperty("knob3", 0.52));
                setSlider(pedalUi->knob2, (double) pedal.getProperty("knob1", 0.42));
                setSlider(pedalUi->knob3, (double) pedal.getProperty("knob2", 0.55));
                setSlider(pedalUi->knob4, 0.52);
                setSlider(pedalUi->knob5, 0.36);
                setSlider(pedalUi->knob6, 0.55);
                setSlider(pedalUi->knob7, 1.0);
                setSlider(pedalUi->knob8, 0.0);
                setSlider(pedalUi->knob9, 0.0);
            }
            pedalUi->setSunsetBrightMode((int) pedal.getProperty("sunsetBright", 1));
            pedalUi->setTertiaryFootswitchLatched((bool) pedal.getProperty("sunsetBOn", false));
            effectsChain.setDriveBEnabled(pedalUi->isTertiaryFootswitchLatched());
        }
        if (pedalIndex == 12)
        {
            setSlider(pedalUi->eqVolumeSlider,
                      (double)pedal.getProperty("eqVolume", pedalUi->eqVolumeSlider.getValue()));
            setSlider(pedalUi->eqGainSlider,
                      (double)pedal.getProperty("eqGain", pedalUi->eqGainSlider.getValue()));
            for (int band = 0; band < 10; ++band)
                setSlider(pedalUi->eqBandSliders[(size_t)band],
                          (double)pedal.getProperty("eqBand" + juce::String(band + 1),
                                                    pedalUi->eqBandSliders[(size_t)band].getValue()));
        }
        setToggle(pedalUi->onBtn, (bool) pedal.getProperty("on", false));
    }

    setCombo(rhythmStyleSelector, (int) fx.getProperty("rhythmStyle", 0) + 1);
    setCombo(rhythmPatternSelector, (int) fx.getProperty("rhythmPattern", 0) + 1);
    setCombo(rhythmModeSelector, (int) fx.getProperty("rhythmMode", 0) + 1);
    setCombo(rhythmVerseBarsSelector, (int) fx.getProperty("rhythmVerseBars", 3));
    setCombo(rhythmChorusBarsSelector, (int) fx.getProperty("rhythmChorusBars", 4));
    setCombo(rhythmCountInBarsSelector, (int) fx.getProperty("rhythmCountInBars", 2));
    setCombo(rhythmCountInSoundSelector, (int) fx.getProperty("rhythmCountInSound", 1));
    setToggle(btnRhythmCountInToggle, (bool) fx.getProperty("rhythmCountInToggle", true));

    closeExternalVst3Editor();
    EffectsChain::PreparedExternalPlugin retired;

    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        if (!looper.commitPreparedSessionRestore(std::move(preparedLooper)))
        {
            errorMessage = "El dispositivo de audio cambio mientras se preparaba la sesion. Intenta abrirla nuevamente.";
            return false;
        }

        if (pluginPath.isNotEmpty())
        {
            retired = effectsChain.commitExternalPlugin(std::move(candidate));
            effectsChain.setExternalPluginEnabled(pluginEnabled);
        }
        else
        {
            retired = effectsChain.detachExternalPlugin();
        }

    }

    updateExternalVstUi();

    refreshTrackUiFromLooper();
    for (int i = 0; i < looperNode.getNumChildren() && i < (int) visualTracks.size(); ++i)
    {
        const auto trackNode = looperNode.getChild(i);
        visualTracks[(size_t) i]->setSoloActive((bool) trackNode.getProperty("soloActive", false));
    }
    updateMuteSoloMatrix();
    refreshAllThumbnails();
    updateRhythmBoxReadouts();
    updateLoopEditButtons();
    repaint();
    return true;
}

void MainComponent::refreshTrackUiFromLooper()
{
    const int totalTracks = looper.getTotalTracks();
    for (int i = 0; i < (int) visualTracks.size(); ++i)
    {
        visualTracks[(size_t) i]->setThumbnailPointer(looper.getTrackThumbnail(i));
        visualTracks[(size_t) i]->setVolumeValue(looper.getTrackVolume(i));
        visualTracks[(size_t) i]->setMuteActive(looper.isTrackMuted(i));
        if (i >= totalTracks)
            visualTracks[(size_t) i]->setSoloActive(false);
    }

    sldSilenceThreshold.setValue(looper.getSilenceThresholdDb(), juce::dontSendNotification);
}

void MainComponent::refreshFxUiOrder()
{
    effectsChain.setPedalOrder(fxOrder);
    resized();
    repaint();
}

void MainComponent::refreshAllThumbnails()
{
    for (int i = 0; i < LooperEngine::kMaxTracks; ++i)
        looper.refreshThumbnail(i, formatManager);
}

int MainComponent::findMidiDeviceIndexByIdentifier(const juce::String& identifier) const
{
    const auto devices = juce::MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        if (devices.getReference(i).identifier == identifier)
            return i;

    return -1;
}

void MainComponent::refreshMidiInputSelector()
{
    midiDeviceSelector.clear(juce::dontSendNotification);
    midiDeviceSelector.addItem("TODAS LAS ENTRADAS MIDI", 1);

    const auto devices = juce::MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        midiDeviceSelector.addItem(devices.getReference(i).name, i + 2);

    midiDeviceSelector.setSelectedItemIndex(0, juce::dontSendNotification);
}

void MainComponent::updateMidiMappingLabel()
{
    const int actionIndex = midiActionSelector.getSelectedId() - 1;
    const juce::String actionName = actionIndex >= 0
        ? juce::String(MidiController::actionName(actionIndex))
        : juce::String("ACCION");
    const auto mapping = midiController.getMapping(actionIndex);
    lblMidiMapping.setText(actionName + ": " + describeMidiMapping(mapping), juce::dontSendNotification);
}

void MainComponent::updateMidiManualControls()
{
    const int action = midiActionSelector.getSelectedId() - 1;
    const auto mapping = midiController.getMapping(action);
    const int type = !mapping.isMapped ? 3 : (mapping.isProgramChange ? 3 : (mapping.isNote ? 2 : 1));
    const int number = !mapping.isMapped ? 0 : mapping.number;
    midiMessageTypeSelector.setSelectedId(type, juce::dontSendNotification);
    midiNumberSelector.setValue(juce::jlimit(0, 127, number < 0 ? 0 : number),
                                juce::dontSendNotification);
}

void MainComponent::toggleFxPedalFromMidi(int pedalIndex)
{
    if (pedalIndex < 0 || pedalIndex >= kNumFxPedals || fxPedals[pedalIndex] == nullptr)
        return;

    static constexpr const char* pedalNames[kNumFxPedals] = {
        "NOISE GATE", "RHYTHM BOX", "COMPRESSOR", "HARMONIZER",
        "SUSTAIN GRAN", "SUSTAIN CHORDS", "FUZZ", "SUNSET", "CHORUS",
        "TREMOLO", "TAPE DELAY", "ROOM REVERB", "OUTPUT EQ"
    };

    auto& pedal = *fxPedals[pedalIndex];
    const bool nextState = !pedal.onBtn.getToggleState();

    // Replica el camino del footswitch con mouse: la notificacion sincronica
    // ejecuta onClick y actualiza el DSP; el repaint del padre actualiza LED y
    // carcasa porque el ToggleButton real permanece oculto.
    pedal.onBtn.setToggleState(nextState, juce::sendNotificationSync);
    pedal.repaint();
    fxContainer.repaint();

    lblMidiStatus.setText("MIDI " + juce::String(pedalNames[pedalIndex]) + ": "
                              + (nextState ? "ON" : "OFF"),
                          juce::dontSendNotification);
    lblMidiStatus.setColour(juce::Label::textColourId,
                            nextState ? LnfC::PlayGreen : LnfC::TextDim);
}

void MainComponent::applyMidiManualMapping()
{
    const int action = midiActionSelector.getSelectedId() - 1;
    if (action < 0 || action >= MidiController::kNumActions)
        return;

    MidiController::MappingData mapping;
    mapping.isMapped = true;
    mapping.number = juce::roundToInt(midiNumberSelector.getValue());
    mapping.isNote = midiMessageTypeSelector.getSelectedId() == 2;
    mapping.isProgramChange = midiMessageTypeSelector.getSelectedId() == 3;
    midiController.setMapping(action, mapping);
    updateMidiMappingLabel();
    markMidiConfigurationDirty();
}

juce::File MainComponent::getMidiConfigurationFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Loopernico").getChildFile("midi-config.xml");
}

bool MainComponent::saveMidiConfiguration() const
{
    juce::ValueTree root("LOOPERNICO_MIDI_CONFIG");
    root.setProperty("version", 3, nullptr);
    root.setProperty("listenAllInputs", midiListenAllInputs, nullptr);
    root.setProperty("inputIdentifier",
                     midiListenAllInputs ? juce::String() : selectedMidiInputIdentifier,
                     nullptr);
    root.setProperty("selectedAction", midiActionSelector.getSelectedId(), nullptr);

    for (int i = 0; i < MidiController::kNumActions; ++i)
    {
        const auto mapping = midiController.getMapping(i);
        juce::ValueTree node("MAPPING");
        node.setProperty("action", i, nullptr);
        node.setProperty("isMapped", mapping.isMapped, nullptr);
        node.setProperty("isProgramChange", mapping.isProgramChange, nullptr);
        node.setProperty("isNote", mapping.isNote, nullptr);
        node.setProperty("number", mapping.number, nullptr);
        root.addChild(node, -1, nullptr);
    }

    const auto file = getMidiConfigurationFile();
    if (!file.getParentDirectory().createDirectory())
        return false;

    // El lector usa XmlDocument, por lo tanto este archivo debe ser XML real.
    // Antes se usaba ValueTree::writeToStream (binario), y el mapping nunca
    // podia recuperarse en el siguiente arranque.
    if (const auto xml = root.createXml())
        return xml->writeTo(file);

    return false;
}

void MainComponent::loadMidiConfiguration()
{
    const auto file = getMidiConfigurationFile();
    if (!file.existsAsFile())
    {
        lblMidiStatus.setText("Perfil MIDI nuevo: sin asignaciones.", juce::dontSendNotification);
        return;
    }

    juce::ValueTree root;
    if (const auto xml = juce::XmlDocument::parse(file))
    {
        root = juce::ValueTree::fromXml(*xml);
    }
    else if (const auto stream = file.createInputStream())
    {
        // Compatibilidad con las versiones que escribian ValueTree binario en
        // el archivo .xml. Al siguiente guardado se migra automaticamente.
        root = juce::ValueTree::readFromStream(*stream);
    }

    if (!root.isValid() || !root.hasType("LOOPERNICO_MIDI_CONFIG"))
        return;

    // Version 3 inaugura el perfil explicito. Las configuraciones anteriores
    // se ignoran para comenzar sin asignaciones y evitar mappings conflictivos.
    if ((int) root.getProperty("version", 0) != 3)
    {
        lblMidiStatus.setText("Perfil MIDI nuevo: sin asignaciones.", juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        const auto node = root.getChild(i);
        if (!node.hasType("MAPPING")) continue;
        const int action = (int) node.getProperty("action", -1);
        const int number = (int) node.getProperty("number", -1);
        const bool isMapped = (bool) node.getProperty("isMapped", false);
        const bool isProgramChange = (bool) node.getProperty("isProgramChange", false);
        const bool isNote = (bool) node.getProperty("isNote", false);
        if (action < 0 || action >= MidiController::kNumActions
            || (isMapped && (number < 0 || number > 127))
            || (isProgramChange && isNote))
            continue;

        MidiController::MappingData mapping;
        mapping.isMapped = isMapped;
        mapping.isProgramChange = isProgramChange;
        mapping.isNote = isNote;
        mapping.number = isMapped ? number : -1;
        midiController.setMapping(action, mapping);
    }

    midiActionSelector.setSelectedId((int) root.getProperty("selectedAction", 1), juce::dontSendNotification);
    const auto identifier = root.getProperty("inputIdentifier").toString();
    const bool listenAll = (bool) root.getProperty("listenAllInputs", true);
    const int device = identifier.isNotEmpty() ? findMidiDeviceIndexByIdentifier(identifier) : -1;
    if (listenAll || identifier.isEmpty())
    {
        changeMidiInput(0);
        lblMidiStatus.setText("Configuracion MIDI cargada.", juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    }
    else if (device >= 0)
    {
        changeMidiInput(device + 1);
        lblMidiStatus.setText("Configuracion MIDI cargada.", juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
    }
    else
    {
        lastMidiInputIndex = -1;
        midiListenAllInputs = false;
        selectedMidiInputIdentifier = identifier;
        midiDeviceSelector.setSelectedItemIndex(-1, juce::dontSendNotification);
        lblMidiStatus.setText("Esperando el dispositivo MIDI guardado...", juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::OvdAmber);
    }
}

void MainComponent::markMidiConfigurationDirty()
{
    lblMidiStatus.setText("Cambios MIDI sin guardar.", juce::dontSendNotification);
    lblMidiStatus.setColour(juce::Label::textColourId, LnfC::OvdAmber);
}

// ============================================================================
// DESTRUCTOR
// ============================================================================
MainComponent::~MainComponent()
{
    stopTimer();
    midiDeviceListConnection.reset();
    closeExternalVst3Editor();
    setLookAndFeel(nullptr);
    auto devices = juce::MidiInput::getAvailableDevices();
    for (auto& d : devices) deviceManager.removeMidiInputDeviceCallback(d.identifier, this);
    shutdownAudio();
}

// ============================================================================
// AUDIO
// ============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    looper.prepare(sampleRate, samplesPerBlockExpected, formatManager);
        effectsChain.prepare(sampleRate, samplesPerBlockExpected);
    for (int i = 0; i < (int)visualTracks.size(); ++i)
        visualTracks[i]->setThumbnailPointer(looper.getTrackThumbnail(i));
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& buf)
{
    if (!buf.buffer) return;
    const bool monoOutput = outputMono.load(std::memory_order_relaxed);
    for (int ch = 2; ch < buf.buffer->getNumChannels(); ++ch)
        buf.buffer->clear(ch, buf.startSample, buf.numSamples);

    {
        float peak = 0.0f;
        for (int ch = 0; ch < juce::jmin(2, buf.buffer->getNumChannels()); ++ch) {
            auto* d = buf.buffer->getReadPointer(ch, buf.startSample);
            for (int i = 0; i < buf.numSamples; ++i) peak = std::max(peak, std::abs(d[i]));
        }
        float prev = inputVU.load(std::memory_order_relaxed);
        if (peak > prev) inputVU.store(peak, std::memory_order_relaxed);
    }

    effectsChain.processBlock(*buf.buffer, buf.startSample, buf.numSamples);

    if (monoOutput && buf.buffer->getNumChannels() > 1)
    {
        auto* left  = buf.buffer->getWritePointer(0, buf.startSample);
        auto* right = buf.buffer->getWritePointer(1, buf.startSample);
        for (int i = 0; i < buf.numSamples; ++i)
        {
            const float mono = 0.5f * (left[i] + right[i]);
            left[i] = mono;
            right[i] = mono;
        }
    }

    looper.processBlock(*buf.buffer, buf.startSample, buf.numSamples,
                        masterVolume.load(std::memory_order_relaxed), monoOutput);

    if (monoOutput && buf.buffer->getNumChannels() > 1)
    {
        auto* left  = buf.buffer->getWritePointer(0, buf.startSample);
        auto* right = buf.buffer->getWritePointer(1, buf.startSample);
        for (int i = 0; i < buf.numSamples; ++i)
        {
            const float mono = 0.5f * (left[i] + right[i]);
            left[i] = mono;
            right[i] = mono;
        }
    }

    {
        float peakL = 0.0f, peakR = 0.0f;
        int ch = buf.buffer->getNumChannels();
        if (ch > 0) {
            auto* d = buf.buffer->getReadPointer(0, buf.startSample);
            for (int i = 0; i < buf.numSamples; ++i) peakL = std::max(peakL, std::abs(d[i]));
        }
        if (ch > 1) {
            auto* d = buf.buffer->getReadPointer(1, buf.startSample);
            for (int i = 0; i < buf.numSamples; ++i) peakR = std::max(peakR, std::abs(d[i]));
        } else { peakR = peakL; }

        float prevL = outputVUL.load(std::memory_order_relaxed);
        float prevR = outputVUR.load(std::memory_order_relaxed);
        if (peakL > prevL) outputVUL.store(peakL, std::memory_order_relaxed);
        if (peakR > prevR) outputVUR.store(peakR, std::memory_order_relaxed);
    }
}

void MainComponent::releaseResources() { looper.releaseResources(); effectsChain.reset(); }

void MainComponent::optimiseAudioDeviceForLowLatency()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
        return;

    auto availableBufferSizes = device->getAvailableBufferSizes();
    if (availableBufferSizes.isEmpty())
        return;

    int preferredBufferSize = availableBufferSizes.getLast();
    for (int candidate : { 64, 128, 256 })
    {
        if (availableBufferSizes.contains(candidate))
        {
            preferredBufferSize = candidate;
            break;
        }
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    if (setup.bufferSize == preferredBufferSize)
        return;

    setup.bufferSize = preferredBufferSize;
    deviceManager.setAudioDeviceSetup(setup, true);
}

void MainComponent::updateRhythmBoxReadouts()
{
    const int bpm = juce::roundToInt((float) fxPedals[1]->knob1.getValue());
    lblRhythmBpmValue.setText(juce::String(bpm) + " BPM", juce::dontSendNotification);
    lblRhythmBpmValue.setJustificationType(juce::Justification::centred);
    lblRhythmBpmValue.setColour(juce::Label::textColourId, juce::Colour(0xfffff4cf));
    lblRhythmBpmValue.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));

    const float tone = (float) fxPedals[1]->knob3.getValue();
    juce::String toneText = "Natural";
    if (tone < 0.34f) toneText = "Calido";
    else if (tone > 0.66f) toneText = "Brillante";

    lblRhythmColorValue.setText(toneText, juce::dontSendNotification);
    lblRhythmColorValue.setJustificationType(juce::Justification::centred);
    lblRhythmColorValue.setColour(juce::Label::textColourId, juce::Colour(0xffffd77a).withAlpha(0.92f));
    lblRhythmColorValue.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
}

// ============================================================================
// MIDI
// ============================================================================
void MainComponent::changeMidiInput(int index)
{
    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& d : devices) deviceManager.removeMidiInputDeviceCallback(d.identifier, this);

    if (index <= 0)
    {
        lastMidiInputIndex = -1;
        midiListenAllInputs = true;
        selectedMidiInputIdentifier.clear();

        for (const auto& dev : devices)
        {
            if (!deviceManager.isMidiInputDeviceEnabled(dev.identifier))
                deviceManager.setMidiInputDeviceEnabled(dev.identifier, true);
            deviceManager.addMidiInputDeviceCallback(dev.identifier, this);
        }

        midiDeviceSelector.setSelectedItemIndex(devices.isEmpty() ? -1 : 0, juce::dontSendNotification);
        return;
    }

    const int deviceIndex = index - 1;
    if (deviceIndex >= 0 && deviceIndex < (int) devices.size())
    {
        const auto& dev = devices[deviceIndex];
        if (!deviceManager.isMidiInputDeviceEnabled(dev.identifier))
            deviceManager.setMidiInputDeviceEnabled(dev.identifier, true);
        deviceManager.addMidiInputDeviceCallback(dev.identifier, this);
        lastMidiInputIndex = deviceIndex;
        midiListenAllInputs = false;
        selectedMidiInputIdentifier = dev.identifier;
        midiDeviceSelector.setSelectedItemIndex(index, juce::dontSendNotification);
    }
    else
    {
        lastMidiInputIndex = -1;
        midiListenAllInputs = true;
        selectedMidiInputIdentifier.clear();
        midiDeviceSelector.setSelectedItemIndex(devices.isEmpty() ? -1 : 0, juce::dontSendNotification);
    }
}

void MainComponent::handleMidiDeviceListChanged()
{
    const auto wantedIdentifier = selectedMidiInputIdentifier;
    const bool wasListeningToAllInputs = midiListenAllInputs;

    refreshMidiInputSelector();

    if (wasListeningToAllInputs)
    {
        changeMidiInput(0);
        return;
    }

    const int deviceIndex = findMidiDeviceIndexByIdentifier(wantedIdentifier);
    if (deviceIndex >= 0)
    {
        changeMidiInput(deviceIndex + 1);
        lblMidiStatus.setText("MIDI conectado: "
                                  + juce::MidiInput::getAvailableDevices().getReference(deviceIndex).name,
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
        return;
    }

    const auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices)
        deviceManager.removeMidiInputDeviceCallback(device.identifier, this);

    lastMidiInputIndex = -1;
    midiListenAllInputs = false;
    selectedMidiInputIdentifier = wantedIdentifier;
    midiDeviceSelector.setSelectedItemIndex(-1, juce::dontSendNotification);
    lblMidiStatus.setText("Esperando el dispositivo MIDI guardado...", juce::dontSendNotification);
    lblMidiStatus.setColour(juce::Label::textColourId, LnfC::OvdAmber);
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& msg)
{
    lastMidiMs.store(juce::Time::currentTimeMillis());
    midiController.handleMessage(msg);
}

// ============================================================================
// TIMER
// ============================================================================
void MainComponent::timerCallback()
{
    updateLoopEditButtons();
    static const char* kSectionNames[] = { "SECCION: VERSO", "SECCION: REMATE", "SECCION: ESTRIBILLO" };
    const int currentRhythmSection = juce::jlimit(0, 2, effectsChain.getRhythmCurrentSection());
    const int pendingRhythmSection = effectsChain.getRhythmPendingSection();
    const int countInBeatsRemaining = effectsChain.getRhythmCountInBeatsRemaining();
    juce::String sectionText;
    if (countInBeatsRemaining >= 0)
    {
        const int currentBeat = 4 - (countInBeatsRemaining % 4);
        const int displayBeat = currentBeat <= 0 ? 4 : currentBeat;
        sectionText = "PRECONTEO: " + juce::String(displayBeat);
    }
    else
    {
        sectionText = kSectionNames[currentRhythmSection];
        if (pendingRhythmSection >= 0 && pendingRhythmSection <= 2)
            sectionText << "  >  SIGUE: " << juce::String(kSectionNames[pendingRhythmSection]).fromFirstOccurrenceOf(": ", false, false);
    }
    lblRhythmSectionState.setText(sectionText, juce::dontSendNotification);
    if (countInBeatsRemaining < 0)
        rhythmPatternSelector.setSelectedId(currentRhythmSection + 1, juce::dontSendNotification);

    auto  state      = looper.getState();
    int   baseSmp    = looper.getBaseLoopSamples();
    int   readPos    = looper.getReadPosition();
    int   writePos   = looper.getRecordWritePosition();
    float sr         = looper.getSampleRate();
    float globalProg = -1.0f;

    if (baseSmp > 0 && (state == LooperEngine::State::Playing ||
                         state == LooperEngine::State::RecordingLayer))
        globalProg = juce::jlimit(0.0f, 1.0f, (float)readPos / (float)baseSmp);

    float loopSecs    = (sr > 0.0f && baseSmp > 0) ? (float)baseSmp / sr : 0.0f;
    float currentSecs = 0.0f;
    int   total       = looper.getTotalTracks();

    bool gArm  = (state == LooperEngine::State::ArmedBase);
    bool gRec  = (state == LooperEngine::State::RecordingBase);
    bool gOvd  = (state == LooperEngine::State::RecordingLayer);
    bool gPlay = (state == LooperEngine::State::Playing);

    btnRecConfirm.setToggleState(gArm || gRec || gOvd, juce::dontSendNotification);
    btnPauseAll.setToggleState(looper.isGlobalPaused(), juce::dontSendNotification);

    if (gRec)
    {
        currentSecs = (sr > 0.0f) ? (float)writePos / sr : 0.0f;
        loopSecs = currentSecs;
    }
    else if (globalProg >= 0.0f)
    {
        currentSecs = globalProg * loopSecs;
    }

    if (gRec)
    {
        looper.refreshThumbnail(0, formatManager);
        visualTracks[0]->repaint();
    }
    else if (gOvd)
    {
        const int activeTrack = total - 1;
        if (activeTrack >= 0 && activeTrack < (int) visualTracks.size())
        {
            looper.refreshThumbnail(activeTrack, formatManager);
            visualTracks[activeTrack]->repaint();
        }
    }

    for (int i = 0; i < (int)visualTracks.size(); ++i)
    {
        bool hasContent = (i < total);
        if (state == LooperEngine::State::Stopped) {
            visualTracks[i]->setPlayheadProgress(-1.0f);
            visualTracks[i]->setStatus(false, false, false);
        } else {
            if (!looper.isTrackPaused(i))
                visualTracks[i]->setPlayheadProgress(globalProg);
            bool rec  = (gArm || gRec) && (i == 0);
            bool ovd  = gOvd && (i == total-1);
            bool play = gPlay && hasContent && !looper.isTrackMuted(i);
            visualTracks[i]->setStatus(rec, play && !rec && !ovd, ovd);
        }
        visualTracks[i]->setLoopSecs(currentSecs, loopSecs);
    }

    const float decay = 0.82f;

    float inRaw = inputVU.load(std::memory_order_relaxed);
    inputVU.store(inRaw * decay, std::memory_order_relaxed);
    inputVUSmooth = std::max(inRaw, inputVUSmooth * decay);

    float outRawL = outputVUL.load(std::memory_order_relaxed);
    float outRawR = outputVUR.load(std::memory_order_relaxed);
    outputVUL.store(outRawL * decay, std::memory_order_relaxed);
    outputVUR.store(outRawR * decay, std::memory_order_relaxed);
    outputVUSmoothL = std::max(outRawL, outputVUSmoothL * decay);
    outputVUSmoothR = std::max(outRawR, outputVUSmoothR * decay);

    masterVUMeter.setLevels(outputVUSmoothL, outputVUSmoothR);

    float outputVUSmooth = std::max(outputVUSmoothL, outputVUSmoothR);
    for (int i = 0; i < (int)visualTracks.size(); ++i)
    {
        float vu = 0.0f;
        if      (gRec && i == 0)       vu = inputVUSmooth;
        else if (gOvd && i == total-1) vu = inputVUSmooth * 0.6f + outputVUSmooth * 0.4f;
        else if (i < total && !gRec)   vu = outputVUSmooth;
        visualTracks[i]->setVULevel(vu);
    }

    static bool lastMidiState = false;
    bool midiNow = (juce::Time::currentTimeMillis() - lastMidiMs.load()) < 200;
    if (midiNow != lastMidiState) { lastMidiState = midiNow; repaint(getWidth()-100, 15, 95, 30); }

    if (midiController.isLearning()) {
        lblMidiStatus.setText("WAITING: " + juce::String(MidiController::actionName(midiController.learningAction())),
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::OvdAmber);
    } else {
        if (btnMidiLearn.getToggleState()) {
            btnMidiLearn.setToggleState(false, juce::dontSendNotification);
            updateMidiMappingLabel();
            updateMidiManualControls();
            markMidiConfigurationDirty();
        }
    }
}

void MainComponent::updateOutputModeButton()
{
    const bool mono = outputMono.load(std::memory_order_relaxed);
    btnOutputMode.setButtonText(mono ? "MONO" : "STEREO");
    btnOutputMode.setToggleState(mono, juce::dontSendNotification);
    for (auto& track : visualTracks)
        track->setMonoDisplay(mono);
}

void MainComponent::updateLoopEditButtons()
{
    const auto state = looper.getState();
    const bool canEditLoop = looper.getBaseLoopSamples() > 0
        && state != LooperEngine::State::ArmedBase
        && state != LooperEngine::State::RecordingBase
        && state != LooperEngine::State::RecordingLayer;

    btnLoopInEarlier.setEnabled(canEditLoop);
    btnLoopInLater.setEnabled(canEditLoop);
    btnLoopOutShorter.setEnabled(canEditLoop);
    btnLoopOutLonger.setEnabled(canEditLoop && looper.getBaseLoopSamples() < looper.getEditableLoopSourceSamples());
}

void MainComponent::updateMuteSoloMatrix()
{
    bool anySolo = false;
    for (auto& t : visualTracks) if (t->isSoloActive()) { anySolo = true; break; }
    for (int i = 0; i < (int)visualTracks.size(); ++i)
        looper.setTrackMuted(i, anySolo ? !visualTracks[i]->isSoloActive()
                                        :  visualTracks[i]->isMuteActive());
}

// ============================================================================
// PAINT
// ============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();
    const int outer = kUiOuterInset;
    const int gap = kUiSectionGap;
    const int hH = kUiHeaderHeight;
    const int sH = kUiFooterHeight;
    const int fxH = kUiFxSectionHeight;
    const int controlH = kUiControlSectionHeight;
    const int bottomPanelH = controlH + sH;
    const int fullX = outer;
    const int fullW = W - outer * 2;
    const int mY = hH + outer;
    const int mH = H - hH - sH - outer * 2;
    const int trackH = mH - fxH - controlH - gap * 2;
    const int fxY = mY + trackH + gap;
    const int controlY = fxY + fxH + gap;

    const auto drawTolexLayer = [this, &g](juce::Rectangle<float> area,
                                           float opacity,
                                           float cornerRadius = 0.0f)
    {
        if (!rackSurfaceTexture.isValid() || area.isEmpty())
            return;

        juce::Graphics::ScopedSaveState state(g);
        g.setTiledImageFill(rackSurfaceTexture, 0, 0, opacity);
        if (cornerRadius > 0.0f)
            g.fillRoundedRectangle(area, cornerRadius);
        else
            g.fillRect(area);
    };

    juce::ColourGradient bgGrad(
        juce::Colour(0xff13161a), 0.0f, 0.0f,
        LnfC::BG.darker(0.12f),   0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(0, 0, W, getHeight());
    if (rackSurfaceTexture.isValid())
        drawTolexLayer({ 0.0f, 0.0f, (float)W, (float)getHeight() }, 0.40f);
    else
        drawFineRackTexture(g, { 0.0f, 0.0f, (float)W, (float)getHeight() }, 0.45f);

    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.drawRoundedRectangle(7.0f, 7.0f, (float)W - 14.0f, (float)getHeight() - 14.0f, 18.0f, 3.0f);
    g.setColour(juce::Colours::white.withAlpha(0.045f));
    g.drawRoundedRectangle(12.0f, 12.0f, (float)W - 24.0f, (float)getHeight() - 24.0f, 14.0f, 1.0f);

    juce::ColourGradient hGrad(
        juce::Colour(0xff171a1f), 0.0f, 0.0f,
        juce::Colour(0xff060708), 0.0f, (float)hH, false);
    g.setGradientFill(hGrad);
    g.fillRect(0, 0, W, hH);
    if (rackSurfaceTexture.isValid())
        drawTolexLayer({ 0.0f, 0.0f, (float)W, (float)hH }, 0.16f);
    else
        drawBrushedMetalHeader(g, { 0.0f, 0.0f, (float)W, (float)hH });

    constexpr int brandWidth = 252;
    const int brandX = (W - brandWidth) / 2;

    g.setColour(juce::Colours::black.withAlpha(0.82f));
    g.drawHorizontalLine(hH + 1, 0.0f, (float)W);
    g.setColour(LnfC::Border.brighter(0.15f).withAlpha(0.80f));
    g.drawHorizontalLine(hH, 0.0f, (float)W);
    g.setColour(LnfC::Accent.withAlpha(0.06f));
    g.drawHorizontalLine(hH - 1, 0.0f, (float)W);

    drawScrew(g, 22.0f, (float)hH * 0.5f, 6.0f);
    drawScrew(g, (float)W - 22.0f, (float)hH * 0.5f, 6.0f);

    if (brandWordmark.isValid())
    {
        constexpr int brandHeight = 55;
        juce::Graphics::ScopedSaveState logoState(g);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setColour(juce::Colours::white);
        g.setOpacity(1.0f);
        g.drawImage(brandWordmark,
                    brandX, (hH - brandHeight) / 2, brandWidth, brandHeight,
                    0, 0, brandWordmark.getWidth(), brandWordmark.getHeight());
    }
    else
    {
        g.setFont(juce::Font(juce::FontOptions(19.0f).withStyle("Bold Italic")));
        g.setColour(LnfC::TextHi);
        g.drawText("LOOPERNICO", brandX, 20, brandWidth, 30, juce::Justification::centred);
    }

    LoopernicoUi::drawPanel(g, { fullX, mY,       fullW, trackH },   {},                  LnfC::Accent);
    LoopernicoUi::drawPanel(g, { fullX, fxY,      fullW, fxH },      {},                  LnfC::OvdAmber, false);
    LoopernicoUi::drawPanel(g, { fullX, controlY, fullW, bottomPanelH }, {},               LnfC::Accent, false);
    drawTolexLayer(juce::Rectangle<float>((float)fullX + 2.0f, (float)mY + 2.0f,
                                          (float)fullW - 4.0f, (float)trackH - 4.0f),
                   0.07f, 6.0f);
    drawTolexLayer(juce::Rectangle<float>((float)fullX + 2.0f, (float)fxY + 2.0f,
                                          (float)fullW - 4.0f, (float)fxH - 4.0f),
                   0.11f, 6.0f);
    drawTolexLayer(juce::Rectangle<float>((float)fullX + 2.0f, (float)controlY + 2.0f,
                                          (float)fullW - 4.0f, (float)bottomPanelH - 4.0f),
                   0.08f, 6.0f);

    const int footerY = H - outer - sH;
    const int outputZoneX = W - kFooterInset - kFooterOutputZoneWidth;

    auto outputCluster = juce::Rectangle<float>((float) outputZoneX + 210.0f,
                                                 (float) footerY + 4.0f, 192.0f, (float) sH - 8.0f);
    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillRoundedRectangle(outputCluster.translated(0.0f, 3.0f), 12.0f);
    juce::ColourGradient outputClusterGrad(
        juce::Colour(0xff17191d), outputCluster.getX(), outputCluster.getY(),
        juce::Colour(0xff0a0c10), outputCluster.getRight(), outputCluster.getBottom(), true);
    g.setGradientFill(outputClusterGrad);
    g.fillRoundedRectangle(outputCluster, 12.0f);
    g.setColour(LnfC::OvdAmber.withAlpha(0.12f));
    g.drawRoundedRectangle(outputCluster.reduced(0.8f), 12.0f, 1.0f);
}

void MainComponent::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    const int outer = kUiOuterInset;
    const int gap = kUiSectionGap;
    const int hH = kUiHeaderHeight;
    const int sH = kUiFooterHeight;
    const int fxH = kUiFxSectionHeight;
    const int controlH = kUiControlSectionHeight;
    const int fullX = outer;
    const int fullW = W - outer * 2;
    const int mY = hH + outer;
    const int mH = H - hH - sH - outer * 2;
    const int trackH = mH - fxH - controlH - gap * 2;
    const int fxY = mY + trackH + gap;
    const int controlY = fxY + fxH + gap;

    constexpr int brandWidth = 252;
    constexpr int headerSideGap = 30;
    constexpr int headerOuterInset = 52;
    constexpr int headerButtonGap = 16;
    constexpr int headerButtonHeight = 24;
    const int brandX = (W - brandWidth) / 2;
    const auto leftHeaderZone = juce::Rectangle<int>(headerOuterInset, 0,
                                                      juce::jmax(0, brandX - headerSideGap - headerOuterInset), hH);
    const auto rightHeaderZone = juce::Rectangle<int>(brandX + brandWidth + headerSideGap, 0,
                                                       juce::jmax(0, W - headerOuterInset - brandX - brandWidth - headerSideGap), hH);

    auto layoutHeaderRow = [headerButtonGap, headerButtonHeight]
                           (juce::Rectangle<int> zone, int y, int maxButtonWidth,
                            std::initializer_list<juce::TextButton*> buttons)
    {
        const int count = (int) buttons.size();
        if (count == 0 || zone.isEmpty())
            return;

        const int gap = headerButtonGap;
        const int height = headerButtonHeight;
        const int availablePerButton = juce::jmax(40, (zone.getWidth() - gap * (count - 1)) / count);
        const int buttonWidth = juce::jmin(maxButtonWidth, availablePerButton);
        const int rowWidth = buttonWidth * count + gap * (count - 1);
        int x = zone.getCentreX() - rowWidth / 2;

        for (auto* button : buttons)
        {
            button->setBounds(x, y, buttonWidth, height);
            x += buttonWidth + gap;
        }
    };

    layoutHeaderRow(leftHeaderZone, 13, 106,
                    { &btnRecConfirm, &btnStopPlay, &btnPauseAll, &btnCancel });
    layoutHeaderRow(leftHeaderZone, 49, 106,
                    { &btnUndo, &btnRedo, &btnReset });
    layoutHeaderRow(rightHeaderZone, 13, 128,
                    { &btnSaveSession, &btnLoadSession });
    layoutHeaderRow(rightHeaderZone, 49, 128,
                    { &btnExportMix, &btnExportTracks });

    auto vpRect = juce::Rectangle<int>(fullX + 4, mY + kTrackViewportTopInset, fullW - 8,
                                       trackH - kTrackViewportTopInset - kTrackViewportBottomInset);
    trackViewport.setBounds(vpRect);

    const int kTH = 116;
    const int nT = (int) visualTracks.size();
    trackContainer.setSize(vpRect.getWidth() - trackViewport.getScrollBarThickness(), nT * kTH);
    int ty = 0;
    for (auto& tr : visualTracks)
    {
        tr->setBounds(0, ty, trackContainer.getWidth(), kTH);
        ty += kTH;
    }

    int fxPanY = fxY + kFxViewportTopInset;
    int fxPanW = fullW - 12;
    fxViewport.setBounds(fullX + 8, fxPanY, fxPanW - 4,
                         juce::jmax(0, fxH - kFxViewportTopInset - kFxViewportBottomInset));

    int totalPedalWidth = 10 + kVst3PedalPitch;
    for (int slot = 0; slot < kNumFxPedals; ++slot)
        totalPedalWidth += fxPedalPitch(fxOrder[(size_t) slot]);
    fxContainer.setSize(totalPedalWidth, fxViewport.getHeight());

    fxPedalFocusOverlay.setBounds(getLocalBounds());
    if (focusedFxPedalIndex != -2)
        fxPedalFocusOverlay.resized();

    if (vst3Pedal != nullptr)
    {
        if (focusedFxPedalIndex != -1)
            vst3Pedal->setBounds(14, 4, kVst3PedalBodyWidth, fxContainer.getHeight() - 8);
        layoutVst3PedalControls(*vst3Pedal);
    }

    for (int slot = 0; slot < kNumFxPedals; ++slot)
    {
        const int pedalIndex = fxOrder[(size_t) slot];
        if (pedalIndex == draggingFxPedal || pedalIndex == focusedFxPedalIndex)
            continue;

        fxPedals[pedalIndex]->setBounds(fxSlotStartX(slot) + 14, 4,
                                        fxPedalBodyWidth(pedalIndex), fxContainer.getHeight() - 8);
    }

    auto pedalArea = fxPedals[1].get();
    auto k1 = pedalArea->knob1.getBounds();
    auto k3 = pedalArea->knob3.getBounds();
    auto styleBox = pedalArea->knob4.getBounds();
    auto patternBox = pedalArea->knob5.getBounds();
    auto modeBox = pedalArea->knob6.getBounds();

    lblRhythmBpmValue.setBounds(styleBox.getX(), styleBox.getY() - 12, styleBox.getWidth(), 12);
    lblRhythmColorValue.setBounds(0, 0, 0, 0);
    lblRhythmStyle.setBounds(0, 0, 0, 0);
    lblRhythmPattern.setBounds(0, 0, 0, 0);
    lblRhythmMode.setBounds(0, 0, 0, 0);
    rhythmStyleSelector.setBounds(styleBox);
    rhythmPatternSelector.setBounds(patternBox);
    rhythmModeSelector.setBounds(modeBox);

    lblRhythmVerseBars.setBounds(fullX + 490, controlY + kControlLabelOffsetY, 66, 12);
    rhythmVerseBarsSelector.setBounds(fullX + 490, controlY + kControlRowOffsetY, 92, 24);
    lblRhythmChorusBars.setBounds(fullX + 594, controlY + kControlLabelOffsetY, 88, 12);
    rhythmChorusBarsSelector.setBounds(fullX + 594, controlY + kControlRowOffsetY, 108, 24);
    lblRhythmCountIn.setBounds(fullX + 716, controlY + kControlLabelOffsetY, 88, 12);
    rhythmCountInBarsSelector.setBounds(fullX + 716, controlY + kControlRowOffsetY, 104, 24);
    lblRhythmCountSound.setBounds(fullX + 832, controlY + kControlLabelOffsetY, 78, 12);
    rhythmCountInSoundSelector.setBounds(fullX + 832, controlY + kControlRowOffsetY, 108, 24);
    btnRhythmCountInToggle.setBounds(fullX + 952, controlY + kControlRowOffsetY, 118, 24);
    btnRhythmNextSection.setBounds(fullX + 1082, controlY + kControlRowOffsetY, 180, 24);
    lblRhythmSectionState.setBounds(fullX + 952, controlY + kControlLabelOffsetY - 1, 310, 12);

    const int loopGroupW = 270;
    const int loopGroupX = fullX + fullW - 18 - loopGroupW;
    lblLoopEdit.setBounds(loopGroupX, controlY + kControlLabelOffsetY, 110, 12);
    btnLoopInEarlier.setBounds(loopGroupX, controlY + kControlRowOffsetY, 58, 24);
    btnLoopInLater.setBounds(loopGroupX + 64, controlY + kControlRowOffsetY, 58, 24);
    btnLoopOutShorter.setBounds(loopGroupX + 132, controlY + kControlRowOffsetY, 66, 24);
    btnLoopOutLonger.setBounds(loopGroupX + 204, controlY + kControlRowOffsetY, 66, 24);

    if (focusedFxPedalIndex != -2)
        fxPedalFocusOverlay.toFront(false);

    lblSilenceThreshold.setBounds(fullX + 24, mY + 5, 100, 18);
    sldSilenceThreshold.setBounds(fullX + 126, mY + 5, 420, 18);

    const int footerY = H - outer - sH;
    const int midiZoneX = kFooterInset;
    const int midiControlsY = footerY + (sH - 24) / 2;
    const int outputZoneX = W - kFooterInset - kFooterOutputZoneWidth;

    btnOutputMode.setBounds(outputZoneX, midiControlsY, 96, 24);
    btnMuteAll.setBounds(outputZoneX + 104, midiControlsY, 96, 24);
    masterKnob.setBounds(outputZoneX + 225, footerY + 3, 34, 34);
    masterVUMeter.setBounds(outputZoneX + 282, midiControlsY, 116, 24);

    midiDeviceSelector.setBounds(midiZoneX, midiControlsY, kMidiDeviceWidth, 24);
    midiActionSelector.setBounds(midiZoneX + kMidiDeviceWidth + kMidiControlGap,
                                 midiControlsY, kMidiActionWidth, 24);
    btnMidiLearn.setBounds(midiZoneX + kMidiDeviceWidth + kMidiControlGap
                                    + kMidiActionWidth + kMidiControlGap,
                           midiControlsY, kMidiLearnWidth, 24);
    const int midiManualX = midiZoneX + kMidiDeviceWidth + kMidiControlGap
                                      + kMidiActionWidth + kMidiControlGap
                                      + kMidiLearnWidth + kMidiControlGap;
    midiMessageTypeSelector.setBounds(midiManualX, midiControlsY, kMidiTypeWidth, 24);
    midiNumberSelector.setBounds(midiManualX + kMidiTypeWidth + kMidiControlGap,
                                 midiControlsY, kMidiNumberWidth, 24);
    btnMidiClear.setBounds(midiManualX + kMidiTypeWidth + kMidiControlGap
                                      + kMidiNumberWidth + kMidiControlGap,
                           midiControlsY, kMidiClearWidth, 24);
    btnMidiSave.setBounds(midiManualX + kMidiTypeWidth + kMidiControlGap
                                     + kMidiNumberWidth + kMidiControlGap
                                     + kMidiClearWidth + kMidiControlGap,
                          midiControlsY, kMidiSaveWidth, 24);
}
