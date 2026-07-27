#include "MainComponent.h"

using namespace LoopernicoUi;

namespace
{
    constexpr int kUiOuterInset = 10;
    constexpr int kUiSectionGap = 4;
    constexpr int kUiHeaderHeight = 86;
    constexpr int kUiFooterHeight = 40;
    constexpr int kUiFxSectionHeight = 310;
    constexpr int kUiControlSectionHeight = 0;

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
    constexpr int kMidiMenuWidth = 910;
    constexpr int kMidiMenuHeight = 94;
    constexpr int kMultiVstStateMagic = 0x56535434; // VST4

    juce::String serializeDrumPattern(const LoopernicoMidi::DrumMidiPattern& pattern)
    {
        juce::String serialized;
        for (int laneIndex = 0; laneIndex < (int) LoopernicoMidi::DrumLane::Count; ++laneIndex)
        {
            const auto lane = (LoopernicoMidi::DrumLane) laneIndex;
            for (int step = 0; step < LoopernicoMidi::DrumMidiPattern::maxSteps; ++step)
            {
                if (serialized.isNotEmpty())
                    serialized << ',';
                serialized << pattern.getVelocity(lane, step);
            }
        }
        return serialized;
    }

    bool deserializeDrumPattern(const juce::String& serialized,
                                int storedBars,
                                LoopernicoMidi::DrumMidiPattern& pattern)
    {
        juce::StringArray tokens;
        tokens.addTokens(serialized, ",", "");
        const int expected = (int) LoopernicoMidi::DrumLane::Count
                           * LoopernicoMidi::DrumMidiPattern::maxSteps;
        if (tokens.size() != expected || storedBars < 1
            || storedBars > LoopernicoMidi::DrumMidiPattern::maxBars)
            return false;

        LoopernicoMidi::DrumMidiPattern decoded;
        decoded.setBars(storedBars);
        for (int laneIndex = 0; laneIndex < (int) LoopernicoMidi::DrumLane::Count; ++laneIndex)
        {
            const auto lane = (LoopernicoMidi::DrumLane) laneIndex;
            for (int step = 0; step < LoopernicoMidi::DrumMidiPattern::maxSteps; ++step)
            {
                const int velocity = tokens[laneIndex * LoopernicoMidi::DrumMidiPattern::maxSteps + step].getIntValue();
                if (velocity < 0 || velocity > 127)
                    return false;
                if (step < storedBars * LoopernicoMidi::DrumMidiPattern::stepsPerBar
                    && !decoded.setVelocity(lane, step, velocity))
                    return false;
                if (step >= storedBars * LoopernicoMidi::DrumMidiPattern::stepsPerBar
                    && velocity != 0)
                    return false;
            }
        }
        pattern = decoded;
        return true;
    }

    juce::MemoryBlock packVstStates(const std::array<juce::MemoryBlock, EffectsChain::kMaxExternalPlugins>& states)
    {
        juce::MemoryOutputStream out;
        out.writeInt(kMultiVstStateMagic);
        out.writeInt(EffectsChain::kMaxExternalPlugins);
        for (const auto& state : states)
        {
            out.writeInt((int) state.getSize());
            if (state.getSize() > 0)
                out.write(state.getData(), state.getSize());
        }
        return out.getMemoryBlock();
    }

    std::array<juce::MemoryBlock, EffectsChain::kMaxExternalPlugins>
    unpackVstStates(const juce::MemoryBlock& packed)
    {
        std::array<juce::MemoryBlock, EffectsChain::kMaxExternalPlugins> states;
        if (packed.getSize() == 0)
            return states;
        juce::MemoryInputStream in(packed, false);
        if (in.readInt() != kMultiVstStateMagic)
        {
            states[0] = packed; // Sesiones v1: el bloque completo pertenecia al unico VST.
            return states;
        }
        const int count = juce::jlimit(0, EffectsChain::kMaxExternalPlugins, in.readInt());
        for (int slot = 0; slot < count; ++slot)
        {
            const int size = in.readInt();
            if (size < 0 || (juce::int64) size > in.getNumBytesRemaining())
                break;
            states[(size_t) slot].setSize((size_t) size);
            if (size > 0)
                in.read(states[(size_t) slot].getData(), size);
        }
        return states;
    }

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

    class DrumPatternEditorComponent final : public juce::Component
    {
    public:
        explicit DrumPatternEditorComponent(const LoopernicoMidi::DrumMidiPattern& initialPattern)
            : pattern(initialPattern)
        {
            // La grilla completa necesita margen para sus 16 columnas y las
            // etiquetas laterales. El componente tambien recalcula el ancho
            // de las celdas cuando la ventana cambia de tamano.
            setSize(820, 285);

            barsLabel.setText("COMPASES", juce::dontSendNotification);
            barsLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
            addAndMakeVisible(barsLabel);

            for (int bars = 1; bars <= LoopernicoMidi::DrumMidiPattern::maxBars; ++bars)
                barsSelector.addItem(juce::String(bars), bars);
            barsSelector.setSelectedId(pattern.getBars(), juce::dontSendNotification);
            barsSelector.onChange = [this]()
            {
                    pattern.setBars(juce::jmax(1, barsSelector.getSelectedId()));
                    refreshPageSelector();
                    refreshGrid();
                    notifyPatternChanged();
            };
            addAndMakeVisible(barsSelector);

            pageLabel.setText("PAGINA", juce::dontSendNotification);
            pageLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
            addAndMakeVisible(pageLabel);

            pageSelector.onChange = [this]()
            {
                currentPage = juce::jmax(0, pageSelector.getSelectedId() - 1);
                refreshGrid();
            };
            addAndMakeVisible(pageSelector);

            for (std::size_t row = 0; row < laneNames.size(); ++row)
            {
                laneLabels[row].setText(laneNames[row], juce::dontSendNotification);
                laneLabels[row].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
                laneLabels[row].setJustificationType(juce::Justification::centredRight);
                addAndMakeVisible(laneLabels[row]);
            }

            for (int col = 0; col < LoopernicoMidi::DrumMidiPattern::stepsPerBar; ++col)
            {
                stepLabels[(size_t) col].setText(juce::String(col + 1), juce::dontSendNotification);
                stepLabels[(size_t) col].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
                stepLabels[(size_t) col].setJustificationType(juce::Justification::centred);
                addAndMakeVisible(stepLabels[(size_t) col]);
            }

            for (std::size_t row = 0; row < gridButtons.size(); ++row)
            {
                for (std::size_t col = 0; col < gridButtons[row].size(); ++col)
                {
                    auto& button = gridButtons[row][col];
                    button.setClickingTogglesState(true);
                    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff232731));
                    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4fcf9d));
                    button.onClick = [this, row, col]()
                    {
                        const int absoluteStep = currentPage * LoopernicoMidi::DrumMidiPattern::stepsPerBar + (int) col;
                        const int velocity = gridButtons[row][col].getToggleState() ? 100 : 0;
                        pattern.setVelocity(lanes[row], absoluteStep, velocity);
                        notifyPatternChanged();
                    };
                    addAndMakeVisible(button);
                }
            }

            refreshPageSelector();
            refreshGrid();
        }

        const LoopernicoMidi::DrumMidiPattern& getPattern() const noexcept
        {
            return pattern;
        }

        void clearPattern() noexcept
        {
            pattern.clear();
            refreshGrid();
            notifyPatternChanged();
        }

        std::function<void(const LoopernicoMidi::DrumMidiPattern&)> onPatternChanged;

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff161a20));
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 8.0f, 1.0f);
        }

        void resized() override
        {
            const int top = 14;
            barsLabel.setBounds(16, top, 80, 22);
            barsSelector.setBounds(102, top, 72, 24);
            pageLabel.setBounds(194, top, 66, 22);
            pageSelector.setBounds(266, top, 92, 24);

            const int gridTop = 56;
            const int laneW = 118;
            const int cellH = 26;
            const int gap = 6;
            const int gridX = laneW + 18;
            const int availableGridWidth = juce::jmax(16, getWidth() - gridX - 12);
            const int cellW = juce::jmax(1,
                                         (availableGridWidth - (LoopernicoMidi::DrumMidiPattern::stepsPerBar - 1) * gap)
                                             / LoopernicoMidi::DrumMidiPattern::stepsPerBar);
            const int labelY = gridTop - 20;
            for (int col = 0; col < LoopernicoMidi::DrumMidiPattern::stepsPerBar; ++col)
                stepLabels[(size_t) col].setBounds(gridX + col * (cellW + gap), labelY, cellW, 16);

            for (std::size_t row = 0; row < gridButtons.size(); ++row)
            {
                const int y = gridTop + (int) row * (cellH + gap);
                laneLabels[row].setBounds(8, y + 2, laneW - 10, 22);
                for (std::size_t col = 0; col < gridButtons[row].size(); ++col)
                    gridButtons[row][col].setBounds(gridX + (int) col * (cellW + gap), y, cellW, cellH);
            }
        }

    private:
        void notifyPatternChanged()
        {
            if (onPatternChanged != nullptr)
                onPatternChanged(pattern);
        }

        void refreshPageSelector()
        {
            pageSelector.clear(juce::dontSendNotification);
            for (int page = 0; page < pattern.getBars(); ++page)
                pageSelector.addItem("COMPAS " + juce::String(page + 1), page + 1);
            currentPage = juce::jlimit(0, pattern.getBars() - 1, currentPage);
            pageSelector.setSelectedId(currentPage + 1, juce::dontSendNotification);
        }

        void refreshGrid()
        {
            const int baseStep = currentPage * LoopernicoMidi::DrumMidiPattern::stepsPerBar;
            for (std::size_t row = 0; row < gridButtons.size(); ++row)
                for (std::size_t col = 0; col < gridButtons[row].size(); ++col)
                    gridButtons[row][col].setToggleState(pattern.isActive(lanes[row], baseStep + (int) col),
                                                         juce::dontSendNotification);
        }

        static constexpr std::array<LoopernicoMidi::DrumLane, LoopernicoMidi::DrumNoteMap::laneCount> lanes = {
            LoopernicoMidi::DrumLane::Kick,
            LoopernicoMidi::DrumLane::Snare,
            LoopernicoMidi::DrumLane::ClosedHiHat,
            LoopernicoMidi::DrumLane::OpenHiHat,
            LoopernicoMidi::DrumLane::Crash,
            LoopernicoMidi::DrumLane::Ride
        };
        static constexpr std::array<const char*, LoopernicoMidi::DrumNoteMap::laneCount> laneNames = {
            "KICK", "SNARE", "HH CERRADO", "HH ABIERTO", "CRASH", "RIDE"
        };

        LoopernicoMidi::DrumMidiPattern pattern;
        int currentPage{ 0 };
        juce::Label barsLabel, pageLabel;
        juce::ComboBox barsSelector, pageSelector;
        std::array<juce::Label, LoopernicoMidi::DrumNoteMap::laneCount> laneLabels;
        std::array<juce::Label, LoopernicoMidi::DrumMidiPattern::stepsPerBar> stepLabels;
        std::array<std::array<juce::TextButton, LoopernicoMidi::DrumMidiPattern::stepsPerBar>,
                   LoopernicoMidi::DrumNoteMap::laneCount> gridButtons;
    };

    class MidiPopupContent : public juce::Component
    {
    public:
        std::function<void(int)> onResult;

        void finish(int result)
        {
            if (onResult != nullptr)
                onResult(result);
        }
    };

    class MidiMapEditorContent final : public MidiPopupContent
    {
    public:
        MidiMapEditorContent(const LoopernicoMidi::DrumNoteMap& initialMap)
        {
            static constexpr const char* names[] = {
                "KICK", "SNARE", "HI-HAT CERRADO", "HI-HAT ABIERTO", "CRASH", "RIDE", "CANAL MIDI"
            };
            juce::StringArray noteItems;
            for (int note = 0; note <= 127; ++note)
                noteItems.add(juce::String(note));
            for (int index = 0; index < 6; ++index)
            {
                labels[(size_t) index].setText(names[index], juce::dontSendNotification);
                labels[(size_t) index].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
                addAndMakeVisible(labels[(size_t) index]);
                for (int note = 0; note <= 127; ++note)
                    combos[(size_t) index].addItem(juce::String(note), note + 1);
                combos[(size_t) index].setSelectedId(initialMap.getNote(lanes[(size_t) index]) + 1,
                                                      juce::dontSendNotification);
                addAndMakeVisible(combos[(size_t) index]);
            }

            labels[6].setText(names[6], juce::dontSendNotification);
            labels[6].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
            addAndMakeVisible(labels[6]);
            for (int channel = 1; channel <= 16; ++channel)
                combos[6].addItem(juce::String(channel), channel);
            combos[6].setSelectedId(initialMap.getChannel(LoopernicoMidi::DrumLane::Kick),
                                    juce::dontSendNotification);
            addAndMakeVisible(combos[6]);

            saveButton.setButtonText("GUARDAR");
            resetButton.setButtonText("RESTABLECER GM");
            cancelButton.setButtonText("CANCELAR");
            saveButton.onClick = [this]() { finish(1); };
            resetButton.onClick = [this]() { finish(2); };
            cancelButton.onClick = [this]() { finish(0); };
            addAndMakeVisible(saveButton);
            addAndMakeVisible(resetButton);
            addAndMakeVisible(cancelButton);
            setSize(560, 560);
        }

        int getNote(int index) const { return combos[(size_t) index].getSelectedId() - 1; }
        int getChannel() const { return combos[6].getSelectedId(); }

        void resized() override
        {
            const int margin = 18;
            const int columnGap = 18;
            const int labelWidth = 150;
            const int comboWidth = juce::jmax(120, getWidth() - margin * 2 - labelWidth - columnGap);
            for (int index = 0; index < 7; ++index)
            {
                const int y = 18 + index * 42;
                labels[(size_t) index].setBounds(margin, y, labelWidth, 28);
                combos[(size_t) index].setBounds(margin + labelWidth + columnGap, y, comboWidth, 28);
            }
            const int buttonY = getHeight() - 52;
            saveButton.setBounds(margin, buttonY, 110, 30);
            resetButton.setBounds(margin + 120, buttonY, 170, 30);
            cancelButton.setBounds(getWidth() - margin - 110, buttonY, 110, 30);
        }

    private:
        static constexpr std::array<LoopernicoMidi::DrumLane, 6> lanes = {
            LoopernicoMidi::DrumLane::Kick, LoopernicoMidi::DrumLane::Snare,
            LoopernicoMidi::DrumLane::ClosedHiHat, LoopernicoMidi::DrumLane::OpenHiHat,
            LoopernicoMidi::DrumLane::Crash, LoopernicoMidi::DrumLane::Ride
        };
        std::array<juce::Label, 7> labels;
        std::array<juce::ComboBox, 7> combos;
        juce::TextButton saveButton, resetButton, cancelButton;
    };

    class MidiPatternEditorContent final : public MidiPopupContent
    {
    public:
        explicit MidiPatternEditorContent(const LoopernicoMidi::DrumMidiPattern& pattern)
            : editor(pattern), initialPattern(pattern)
        {
            addAndMakeVisible(editor);
            saveButton.setButtonText("GUARDAR");
            clearButton.setButtonText("LIMPIAR");
            cancelButton.setButtonText("CANCELAR");
            saveButton.onClick = [this]() { finish(1); };
            clearButton.onClick = [this]() { editor.clearPattern(); };
            cancelButton.onClick = [this]() { finish(0); };
            addAndMakeVisible(saveButton);
            addAndMakeVisible(clearButton);
            addAndMakeVisible(cancelButton);
            setSize(1000, 500);
        }

        const LoopernicoMidi::DrumMidiPattern& getPattern() const { return editor.getPattern(); }
        const LoopernicoMidi::DrumMidiPattern& getInitialPattern() const { return initialPattern; }

        void setPatternChangedCallback(std::function<void(const LoopernicoMidi::DrumMidiPattern&)> callback)
        {
            editor.onPatternChanged = std::move(callback);
        }

        void resized() override
        {
            const int margin = 8;
            const int buttonY = getHeight() - 42;
            editor.setBounds(margin, margin, getWidth() - margin * 2, buttonY - margin * 2);
            saveButton.setBounds(margin, buttonY, 110, 30);
            clearButton.setBounds(margin + 120, buttonY, 100, 30);
            cancelButton.setBounds(getWidth() - margin - 110, buttonY, 110, 30);
        }

    private:
        DrumPatternEditorComponent editor;
        LoopernicoMidi::DrumMidiPattern initialPattern;
        juce::TextButton saveButton, clearButton, cancelButton;
    };

    class MidiEditorWindow final : public juce::DocumentWindow
    {
    public:
        MidiEditorWindow(const juce::String& title,
                         std::unique_ptr<MidiPopupContent> content,
                         int width, int height,
                         int minimumWidth, int minimumHeight,
                         int maximumWidth, int maximumHeight)
            : juce::DocumentWindow(title, juce::Colour(0xff101216), juce::DocumentWindow::closeButton),
              contentComponent(content.release())
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizeLimits(minimumWidth, minimumHeight, maximumWidth, maximumHeight);
            setContentOwned(contentComponent, true);
            setSize(width, height);
        }

        ~MidiEditorWindow() override
        {
            juce::Desktop::getInstance().removeGlobalMouseListener(this);
        }

        void finish(int result)
        {
            if (!closing)
            {
                closing = true;
                exitModalState(result);
            }
        }

        void startDismissOnOutsideClick()
        {
            juce::Desktop::getInstance().addGlobalMouseListener(this);
        }

    private:
        void mouseDown(const juce::MouseEvent& event) override
        {
            if (!getScreenBounds().contains(event.getScreenPosition().roundToInt()))
                finish(0);
        }

        MidiPopupContent* contentComponent = nullptr;
        bool closing = false;
    };

}

void MainComponent::LoopRangeMarker::setRange(float newStart, float newEnd)
{
    start = juce::jlimit(0.0f, 1.0f, newStart);
    end = juce::jlimit(start, 1.0f, newEnd);
    repaint();
}

void MainComponent::LoopRangeMarker::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const float left = bounds.getX() + 10.0f;
    const float width = juce::jmax(1.0f, bounds.getWidth() - 20.0f);
    const float startX = left + start * width;
    const float endX = left + end * width;

    g.setColour(juce::Colour(0xff10151d).withAlpha(0.96f));
    g.fillRoundedRectangle(bounds, 6.0f);
    for (int i = 0; i < 52; ++i)
    {
        const float x = left + width * (float) i / 51.0f;
        const float bar = 4.0f + 12.0f * (0.5f + 0.5f * std::sin((float) i * 1.83f));
        g.setColour(juce::Colour(0xff6a86a8).withAlpha(0.34f));
        g.fillRoundedRectangle(x - 1.0f, bounds.getCentreY() - bar * 0.5f, 2.0f, bar, 1.0f);
    }
    g.setColour(juce::Colour(0xff9d5cff).withAlpha(0.72f));
    g.fillRoundedRectangle(left, bounds.getY() + 2.0f, juce::jmax(0.0f, startX - left),
                           bounds.getHeight() - 4.0f, 5.0f);
    g.setColour(juce::Colour(0xff2e9eea).withAlpha(0.70f));
    g.fillRoundedRectangle(startX, bounds.getY() + 2.0f, juce::jmax(0.0f, endX - startX),
                           bounds.getHeight() - 4.0f, 5.0f);

    auto drawHandle = [&g, &bounds](float x, juce::Colour colour, const char* label)
    {
        g.setColour(colour.withAlpha(0.22f));
        g.fillRoundedRectangle(x - 10.0f, bounds.getY(), 20.0f, bounds.getHeight(), 5.0f);
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(x - 2.0f, bounds.getY() + 2.0f, 4.0f, bounds.getHeight() - 4.0f, 2.0f);
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText(label, (int) x - 18, (int) bounds.getY() - 1, 36, 10, juce::Justification::centred);
    };
    drawHandle(startX, juce::Colour(0xffb46cff), "IN");
    drawHandle(endX, juce::Colour(0xff52c9ff), "OUT");
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void MainComponent::LoopRangeMarker::mouseDown(const juce::MouseEvent& e)
{
    const float left = 11.0f;
    const float width = juce::jmax(1.0f, (float) getWidth() - 22.0f);
    const float x = (float) e.position.x;
    const float startX = left + start * width;
    const float endX = left + end * width;
    dragTarget = std::abs(x - startX) <= std::abs(x - endX) ? DragTarget::start : DragTarget::end;
    dragSpan = juce::jmax(0.01f, end - start);
}

void MainComponent::LoopRangeMarker::mouseDrag(const juce::MouseEvent& e)
{
    const float left = 11.0f;
    const float width = juce::jmax(1.0f, (float) getWidth() - 22.0f);
    const float value = juce::jlimit(0.0f, 1.0f, ((float) e.position.x - left) / width);
    if (dragTarget == DragTarget::start)
    {
        start = juce::jlimit(0.0f, juce::jmax(0.0f, 1.0f - dragSpan), value);
        end = juce::jlimit(start + 0.01f, 1.0f, start + dragSpan);
    }
    else if (dragTarget == DragTarget::end)
        end = juce::jlimit(juce::jmin(1.0f, start + 0.01f), 1.0f, value);
    repaint();
    if (onRangeChanged != nullptr)
        onRangeChanged(start, end);
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

void MainComponent::MidiMenuSurface::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(juce::Colour(0xff141a21).withAlpha(0.98f));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(LnfC::Accent.withAlpha(0.42f));
    g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
    g.setColour(LnfC::TextDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    g.drawText("CONFIGURACION MIDI", 14, 7, 180, 16, juce::Justification::centredLeft, false);
    g.setColour(juce::Colours::white.withAlpha(0.42f));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("ESC para cerrar", getWidth() - 110, 7, 94, 16, juce::Justification::centredRight, false);
}

bool MainComponent::MidiMenuSurface::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onEscape != nullptr)
            onEscape();
        return true;
    }

    return false;
}

MainComponent::MainComponent(bool forceAudioFallbackForDiagnostics)
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
        if (prev == LooperEngine::State::Stopped || prev == LooperEngine::State::ArmedBase)
            looper.setBaseRecordingUsesTimingReference(false);
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
    btnAudioSettings.setButtonText("CONFIGURACION");
    btnAudioSettings.setTooltip("Abrir configuracion de audio");
    btnAudioSettings.onClick = [this]() { showAudioSettings(); };
    styleHeaderButton(btnAudioSettings, LnfC::Accent);
    addAndMakeVisible(btnSaveSession);
    addAndMakeVisible(btnLoadSession);
    addAndMakeVisible(btnExportMix);
    addAndMakeVisible(btnExportTracks);
    addAndMakeVisible(btnAudioSettings);

    lblAudioStatus.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
    lblAudioStatus.setColour(juce::Label::textColourId, LnfC::TextDim);
    lblAudioStatus.setJustificationType(juce::Justification::centredRight);
    // El diagnostico detallado de driver/latencia se consulta desde
    // CONFIGURACION; no ocupa espacio visual permanente en la pantalla.

    setupFXPedals();
    fxPedalFocusOverlay.onDismiss = [this]() { closeFxPedalFocus(); };
    addChildComponent(fxPedalFocusOverlay);

    // MIDI
    btnMidiMenu.setTooltip("Mostrar u ocultar la configuracion MIDI");
    styleVstButton(btnMidiMenu, LnfC::Accent);
    btnMidiMenu.onClick = [this]() { setMidiMenuExpanded(!midiMenuExpanded); };
    addAndMakeVisible(btnMidiMenu);

    midiMenuSurface.onEscape = [this]() { setMidiMenuExpanded(false); };
    midiMenuSurface.setWantsKeyboardFocus(true);
    addAndMakeVisible(midiMenuSurface);

    midiDeviceSelector.setTextWhenNoChoicesAvailable("No MIDI Devices");
    refreshMidiInputSelector();
    midiMenuSurface.addAndMakeVisible(midiDeviceSelector);
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
    midiMenuSurface.addAndMakeVisible(midiActionSelector);

    midiMessageTypeSelector.addItem("CC", 1);
    midiMessageTypeSelector.addItem("NOTA", 2);
    midiMessageTypeSelector.addItem("PROGRAM", 3);
    midiMessageTypeSelector.setSelectedId(1);
    midiMessageTypeSelector.onChange = [this]() { applyMidiManualMapping(); };
    midiMenuSurface.addAndMakeVisible(midiMessageTypeSelector);

    midiNumberSelector.setSliderStyle(juce::Slider::IncDecButtons);
    midiNumberSelector.setRange(0, 127, 1);
    midiNumberSelector.setValue(0);
    midiNumberSelector.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 42, 22);
    midiNumberSelector.onValueChange = [this]() { applyMidiManualMapping(); };
    midiMenuSurface.addAndMakeVisible(midiNumberSelector);

    styleVstButton(btnMidiClear, juce::Colour(0xffcc4422));
    btnMidiClear.onClick = [this]()
    {
        const int action = midiActionSelector.getSelectedId() - 1;
        midiController.setMapping(action, {});
        updateMidiMappingLabel();
        updateMidiManualControls();
        markMidiConfigurationDirty();
    };
    midiMenuSurface.addAndMakeVisible(btnMidiClear);

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
    midiMenuSurface.addAndMakeVisible(btnMidiSave);

    btnMidiLearn.setClickingTogglesState(true);
    btnMidiLearn.setColour(juce::TextButton::buttonOnColourId, LnfC::OvdAmber.darker(0.1f));
    midiMenuSurface.addAndMakeVisible(btnMidiLearn);
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
    midiMenuSurface.addAndMakeVisible(lblMidiStatus);

    lblMidiMapping.setFont(juce::Font(juce::FontOptions(10.0f)));
    lblMidiMapping.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.88f));
    midiMenuSurface.addAndMakeVisible(lblMidiMapping);
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
    sldSilenceThreshold.setComponentID("signalThresholdSlider");
    sldSilenceThreshold.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    sldSilenceThreshold.setTextValueSuffix(" dB");
    sldSilenceThreshold.onValueChange = [this]() { looper.setSilenceThresholdDb((float)sldSilenceThreshold.getValue()); };
    addAndMakeVisible(sldSilenceThreshold);

    lblLooperTempo.setText("TEMPO:", juce::dontSendNotification);
    lblLooperTempo.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    lblLooperTempo.setColour(juce::Label::textColourId, LnfC::TextDim);
    lblLooperTempo.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(lblLooperTempo);

    sldLooperTempo.setSliderStyle(juce::Slider::IncDecButtons);
    sldLooperTempo.setRange(20.0, 300.0, 1.0);
    sldLooperTempo.setValue(looper.getBpm(), juce::dontSendNotification);
    sldLooperTempo.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 72, 18);
    sldLooperTempo.setTextValueSuffix(" BPM");
    sldLooperTempo.onValueChange = [this]() { looper.setBpm((float) sldLooperTempo.getValue()); };
    addAndMakeVisible(sldLooperTempo);

    styleHeaderButton(btnLooperTap, LnfC::OvdAmber);
    btnLooperTap.onClick = [this]()
    {
        looper.tapTempo();
        sldLooperTempo.setValue(looper.getBpm(), juce::dontSendNotification);
    };
    addAndMakeVisible(btnLooperTap);

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
    btnOutputMode.setComponentID("outputModeButton");
    btnOutputMode.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    btnOutputMode.setColour(juce::TextButton::buttonOnColourId, LnfC::Accent.withAlpha(0.28f));
    btnOutputMode.onClick = [this]()
    {
        outputMono.store(btnOutputMode.getToggleState(), std::memory_order_relaxed);
        updateOutputModeButton();
    };
    addAndMakeVisible(btnOutputMode);
    updateOutputModeButton();

    const auto tonexTypeface = loadTonexDisplayTypeface();
    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
    {
        btnLoadVst3[(size_t) slot].setButtonText("CARGAR");
        btnUnloadVst3[(size_t) slot].setButtonText("QUITAR");
        btnEditVst3[(size_t) slot].setButtonText("ABRIR");
        btnMapVst3[(size_t) slot].setButtonText("MAPA");
        btnPatternVst3[(size_t) slot].setButtonText("PATRON");
        btnPatternPlayVst3[(size_t) slot].setButtonText("PLAY");
        btnBypassVst3[(size_t) slot].setButtonText("VST3 ON");
        styleVstButton(btnLoadVst3[(size_t) slot], LnfC::OvdAmber);
        styleVstButton(btnUnloadVst3[(size_t) slot], juce::Colour(0xff9c4630));
        styleVstButton(btnEditVst3[(size_t) slot], LnfC::Accent);
        styleVstButton(btnMapVst3[(size_t) slot], juce::Colour(0xffb36bff));
        styleVstButton(btnPatternVst3[(size_t) slot], juce::Colour(0xff4fcf9d));
        styleVstButton(btnPatternPlayVst3[(size_t) slot], LnfC::PlayGreen);
        btnPatternPlayVst3[(size_t) slot].setComponentID("vstPatternTransportButton");
        btnPatternPlayVst3[(size_t) slot].setTooltip("Reproducir o detener el patron");
        styleVstButton(btnBypassVst3[(size_t) slot], juce::Colour(0xff2f7d53));
        for (auto* button : { &btnLoadVst3[(size_t) slot], &btnUnloadVst3[(size_t) slot],
                              &btnEditVst3[(size_t) slot], &btnMapVst3[(size_t) slot],
                              &btnPatternVst3[(size_t) slot], &btnPatternPlayVst3[(size_t) slot],
                              &btnBypassVst3[(size_t) slot] })
        {
            button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff111820));
            button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        }
        btnBypassVst3[(size_t) slot].setClickingTogglesState(true);
        btnLoadVst3[(size_t) slot].onClick = [this, slot]() { loadExternalVst3Plugin(slot); };
        btnUnloadVst3[(size_t) slot].onClick = [this, slot]() { unloadExternalVst3Plugin(slot); };
        btnEditVst3[(size_t) slot].onClick = [this, slot]() { openExternalVst3Editor(slot); };
        btnMapVst3[(size_t) slot].onClick = [this, slot]() { openExternalVst3DrumMapEditor(slot); };
        btnPatternVst3[(size_t) slot].onClick = [this, slot]() { openExternalVst3PatternEditor(slot); };
        btnPatternPlayVst3[(size_t) slot].setClickingTogglesState(true);
        btnPatternPlayVst3[(size_t) slot].onClick = [this, slot]()
        {
            effectsChain.setExternalPluginPatternPlaying(
                btnPatternPlayVst3[(size_t) slot].getToggleState(), slot);
            updateExternalVstUi(slot);
        };
        btnBypassVst3[(size_t) slot].onClick = [this, slot]()
        {
            effectsChain.setExternalPluginEnabled(btnBypassVst3[(size_t) slot].getToggleState(), slot);
            updateExternalVstUi(slot);
        };
        auto& status = lblVst3Status[(size_t) slot];
        if (tonexTypeface != nullptr)
            status.setFont(juce::Font(juce::FontOptions(tonexTypeface).withHeight(24.0f)));
        else
            status.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 15.0f,
                                                        juce::Font::bold)));
        status.setColour(juce::Label::textColourId, LnfC::TextDim);
        status.setColour(juce::Label::backgroundColourId, juce::Colour(0xff070a0d).withAlpha(0.92f));
        status.setColour(juce::Label::outlineColourId, LnfC::Accent.withAlpha(0.20f));
        status.setJustificationType(juce::Justification::centred);
        auto* pedal = vst3Pedals[(size_t) slot].get();
        if (pedal != nullptr)
        {
            pedal->addAndMakeVisible(btnLoadVst3[(size_t) slot]);
            pedal->addAndMakeVisible(btnUnloadVst3[(size_t) slot]);
            pedal->addAndMakeVisible(btnEditVst3[(size_t) slot]);
            pedal->addAndMakeVisible(btnMapVst3[(size_t) slot]);
            pedal->addAndMakeVisible(btnPatternVst3[(size_t) slot]);
            pedal->addAndMakeVisible(btnPatternPlayVst3[(size_t) slot]);
            pedal->addChildComponent(btnBypassVst3[(size_t) slot]);
            pedal->addAndMakeVisible(status);
            pedal->onResized = [this](FXPedalComponent& resizedPedal) { layoutVst3PedalControls(resizedPedal); };
            layoutVst3PedalControls(*pedal);
        }
    }
    updateExternalVstUi();

    loopRangeMarker.onRangeChanged = [this](float newStart, float newEnd)
    {
        const int sourceSamples = looper.getEditableLoopSourceSamples();
        if (sourceSamples <= 1)
            return;

        const int startDelta = juce::roundToInt((newStart - loopMarkerStart) * (float) sourceSamples);
        const int endDelta = juce::roundToInt((newEnd - loopMarkerEnd) * (float) sourceSamples);
        if (startDelta != 0)
            looper.nudgeLoopStart(startDelta);
        if (endDelta != 0)
            looper.adjustLoopTail(endDelta);
        loopMarkerStart = newStart;
        loopMarkerEnd = newEnd;
        updateLoopEditButtons();
        refreshAllThumbnails();
    };
    addAndMakeVisible(loopRangeMarker);

    updateLoopEditButtons();

    // MIDI callbacks
    midiController.setActionCallback(MidiController::RecConfirm, [this]() { if (btnRecConfirm.onClick) btnRecConfirm.onClick(); });
    midiController.setActionCallback(MidiController::Cancel,     [this]() { if (btnCancel.onClick)     btnCancel.onClick(); });
    midiController.setActionCallback(MidiController::Undo,       [this]() { if (btnUndo.onClick)       btnUndo.onClick(); });
    midiController.setActionCallback(MidiController::Redo,       [this]() { if (btnRedo.onClick)       btnRedo.onClick(); });
    midiController.setActionCallback(MidiController::StopPlay,   [this]() { if (btnStopPlay.onClick)   btnStopPlay.onClick(); });
    midiController.setActionCallback(MidiController::MuteAll,    [this]() { if (btnMuteAll.onClick)    btnMuteAll.onClick(); });
    midiController.setActionCallback(MidiController::ResetAll,   [this]() { if (btnReset.onClick)      btnReset.onClick(); });
    midiController.setActionCallback(MidiController::PauseAll,   [this]() { if (btnPauseAll.onClick)   btnPauseAll.onClick(); });

    auto registerPedalToggle = [this](MidiController::Action action, int pedalIndex)
    {
        midiController.setActionCallback(action, [this, pedalIndex]()
        {
            toggleFxPedalFromMidi(pedalIndex);
        });
    };

    registerPedalToggle(MidiController::NoiseGateToggle,      0);
    registerPedalToggle(MidiController::CompressorToggle,     2);
    registerPedalToggle(MidiController::HarmonizerToggle,     3);
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
        if (vst3Pedals[0] == nullptr)
            return;

        const bool nextState = !vst3Pedals[0]->onBtn.getToggleState();
        vst3Pedals[0]->onBtn.setToggleState(nextState, juce::sendNotificationSync);
        vst3Pedals[0]->repaint();
        fxContainer.repaint();
        lblMidiStatus.setText("MIDI PEDAL VST3: " + juce::String(nextState ? "ON" : "OFF"),
                              juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId,
                                nextState ? LnfC::PlayGreen : LnfC::TextDim);
    });

    setSize(1600, 960);
    const auto audioInitialisation = audioDeviceController.initialise(forceAudioFallbackForDiagnostics);
    deviceManager.addChangeListener(this);
    audioSourcePlayer.setSource(this);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    audioSettingsPanel = std::make_unique<AudioSettingsPanel>(audioDeviceController);
    audioSettingsPanel->onClose = [this]() { closeAudioSettings(); };
    audioSettingsPanel->onConfigurationChanged = [this]()
    {
        updateRecordingLatencyCompensation();
        updateAudioStatusUi();
    };
    addChildComponent(audioSettingsPanel.get());
    updateRecordingLatencyCompensation();
    updateAudioStatusUi();

    if (audioInitialisation.usingFallback || !audioInitialisation.audioAvailable)
    {
        lblMidiStatus.setText(audioInitialisation.message, juce::dontSendNotification);
        lblMidiStatus.setColour(juce::Label::textColourId, LnfC::OvdAmber);
    }
    midiDeviceListConnection = juce::MidiDeviceListConnection::make(
        [safeThis = juce::Component::SafePointer<MainComponent>(this)]()
        {
            if (safeThis != nullptr)
                safeThis->handleMidiDeviceListChanged();
        });
    setMidiMenuExpanded(false);
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
        { "REMOVED",        0xff9a2b1f, "",        "",        ""      },
        { "COMPRESSOR",     0xffb8880a, "THRESH",  "RATIO",   "LEVEL" },
        { "HARMONIZER",     0xfff1f1f1, "SHIFT A", "SHIFT B", "MIX"   },
        { "REMOVED",        0xff9a2b1f, "",        "",        ""      },
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

    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
    {
        auto& vstPedal = vst3Pedals[(size_t) slot];
        vstPedal = std::make_unique<FXPedalComponent>(
            "VST3 HOST", juce::Colour(0xff20262c), "", "", "");
        for (auto* control : { &vstPedal->knob1, &vstPedal->knob2, &vstPedal->knob3,
                               &vstPedal->knob4, &vstPedal->knob5, &vstPedal->knob6,
                               &vstPedal->knob7, &vstPedal->knob8, &vstPedal->knob9 })
            control->setVisible(false);
        vstPedal->onBtn.onClick = [this, slot]()
        {
            if (!effectsChain.hasExternalPlugin(slot))
            {
                vst3Pedals[(size_t) slot]->onBtn.setToggleState(false, juce::dontSendNotification);
                updateExternalVstUi(slot);
                return;
            }
            effectsChain.setExternalPluginEnabled(vst3Pedals[(size_t) slot]->onBtn.getToggleState(), slot);
            updateExternalVstUi(slot);
        };
        vstPedal->onDragStart = [this](FXPedalComponent& pedal, const juce::MouseEvent& e) { beginFxPedalDrag(pedal, e); };
        vstPedal->onDragMove = [this](FXPedalComponent& pedal, const juce::MouseEvent& e) { updateFxPedalDrag(pedal, e); };
        vstPedal->onDragEnd = [this](FXPedalComponent& pedal, const juce::MouseEvent& e) { endFxPedalDrag(pedal, e); };
        vstPedal->onZoomRequest = [this](FXPedalComponent& pedal) { toggleFxPedalFocus(pedal); };
        fxContainer.addAndMakeVisible(vstPedal.get());
    }

    for (int i = 0; i < kNumFxPedals; ++i)
    {
        if (i == EffectsChain::RhythmPedal || i == EffectsChain::LegacySustainGranPedal)
        {
            // El hueco conserva el indice de sesiones antiguas, pero el
            // BeatBuddy ya no se construye ni entra en la cadena activa.
            fxPedals[i].reset();
            continue;
        }

        auto& d = defs[i];
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
    fxOrder[0] = kNumFxPedals;
    int nextOrderSlot = 1;
    for (int i = 0; i < kNumFxPedals; ++i)
        if (i != EffectsChain::RhythmPedal && i != EffectsChain::LegacySustainGranPedal)
            fxOrder[(size_t) nextOrderSlot++] = i;
    for (int slot = 1; slot < kMaxVst3Pedals; ++slot)
        fxOrder[(size_t) nextOrderSlot++] = kNumFxPedals + slot;
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

int MainComponent::findChainItemId(const FXPedalComponent& pedal) const
{
    const int internal = findFxPedalIndex(pedal);
    if (internal >= 0)
        return internal;
    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
        if (vst3Pedals[(size_t) slot].get() == &pedal)
            return kNumFxPedals + slot;
    return -1;
}

FXPedalComponent* MainComponent::getChainItemPedal(int chainItem) const
{
    if (juce::isPositiveAndBelow(chainItem, kNumFxPedals))
        return fxPedals[chainItem].get();
    const int slot = chainItem - kNumFxPedals;
    return juce::isPositiveAndBelow(slot, kMaxVst3Pedals) ? vst3Pedals[(size_t) slot].get() : nullptr;
}

int MainComponent::fxPedalBodyWidth(int pedalIndex) const
{
    if (pedalIndex >= kNumFxPedals) return kVst3PedalBodyWidth;
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
    if (pedalIndex == -1 && vst3Pedals[0] != nullptr)
    {
        toggleFxPedalFocus(*vst3Pedals[0]);
        return;
    }

    if (pedalIndex >= 0 && pedalIndex < kNumFxPedals && fxPedals[pedalIndex] != nullptr)
        toggleFxPedalFocus(*fxPedals[pedalIndex]);
}

void MainComponent::setVst3DisplayTextForSnapshot(const juce::String& text)
{
    lblVst3Status[0].setDisplayText(text);
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

    const int pedalIndex = findChainItemId(pedal);
    if (pedalIndex < 0)
        return;

    if (draggingFxPedal >= 0)
    {
        if (auto* dragged = getChainItemPedal(draggingFxPedal))
            dragged->setDragHintVisible(false);
        draggingFxPedal = -1;
        dragInsertSlot = -1;
    }

    focusedFxPedalIndex = pedalIndex;
    const int compactWidth = fxPedalBodyWidth(pedalIndex);
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
    int x = 10;
    for (int i = 0; i < juce::jlimit(0, kChainItemCount, slot); ++i)
        x += fxPedalPitch(fxOrder[(size_t) i]);
    return x;
}

int MainComponent::fxSlotForX(int x) const
{
    for (int slot = 0; slot < kChainItemCount; ++slot)
    {
        const int start = fxSlotStartX(slot);
        const int end = start + fxPedalPitch(fxOrder[(size_t) slot]);
        if (x < (start + end) / 2)
            return slot;
    }
    return kChainItemCount - 1;
}

void MainComponent::moveFxPedalToSlot(int pedalIndex, int targetSlot)
{
    targetSlot = juce::jlimit(0, kChainItemCount - 1, targetSlot);

    int currentSlot = -1;
    for (int i = 0; i < kChainItemCount; ++i)
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
    draggingFxPedal = findChainItemId(pedal);
    if (draggingFxPedal < 0)
        return;

    auto relativeEvent = e.getEventRelativeTo(&fxContainer);
    dragInsertSlot = fxSlotForX((int) relativeEvent.position.x);
    pedal.setDragHintVisible(true);
    pedal.toFront(false);
}

void MainComponent::updateFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e)
{
    if (draggingFxPedal != findChainItemId(pedal))
        return;

    auto relativeEvent = e.getEventRelativeTo(&fxContainer);
    dragInsertSlot = fxSlotForX((int) relativeEvent.position.x);

    auto bounds = pedal.getBounds();
    bounds.setCentre((int) relativeEvent.position.x, bounds.getCentreY());
    bounds.setX(juce::jlimit(5,
                             juce::jmax(5,
                                        fxContainer.getWidth() - bounds.getWidth() - 5),
                             bounds.getX()));
    pedal.setBounds(bounds);
}

void MainComponent::endFxPedalDrag(FXPedalComponent& pedal, const juce::MouseEvent& e)
{
    const int pedalIndex = findChainItemId(pedal);
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
    const int slot = findChainItemId(pedal) - kNumFxPedals;
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;

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
    lblVst3Status[(size_t) slot].setBounds(screen.toNearestInt());

    const int buttonGap = juce::jmax(5, juce::roundToInt(body.getWidth() * 0.025f));
    const int contentX = juce::roundToInt(body.getX() + body.getWidth() * 0.090f);
    const int contentW = juce::roundToInt(body.getWidth() * 0.820f);
    const int buttonH = juce::jlimit(18, 26, juce::roundToInt(body.getHeight() * 0.095f));
    const int controlsTop = juce::roundToInt(body.getY() + body.getHeight() * 0.040f);
    const int secondRowY = juce::roundToInt(body.getY() + body.getHeight() * 0.515f);
    const int buttonW = (contentW - buttonGap * 2) / 3;

    // Keep one row above the display and the second in the gap below it, above
    // the TONEX artwork. The proportions scale with the normal and focused size.
    btnLoadVst3[(size_t) slot].setBounds(contentX, controlsTop, buttonW, buttonH);
    btnUnloadVst3[(size_t) slot].setBounds(contentX + buttonW + buttonGap,
                                           controlsTop, buttonW, buttonH);
    btnEditVst3[(size_t) slot].setBounds(contentX + (buttonW + buttonGap) * 2,
                                         controlsTop, buttonW, buttonH);
    btnMapVst3[(size_t) slot].setBounds(contentX, secondRowY, buttonW, buttonH);
    btnPatternVst3[(size_t) slot].setBounds(contentX + buttonW + buttonGap,
                                            secondRowY, buttonW, buttonH);
    btnPatternPlayVst3[(size_t) slot].setBounds(contentX + (buttonW + buttonGap) * 2,
                                                secondRowY, buttonW, buttonH);
    btnBypassVst3[(size_t) slot].setBounds(0, 0, 0, 0);
}

void MainComponent::styleVstButton(juce::TextButton& button, juce::Colour accent)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.24f));
    button.setColour(juce::TextButton::textColourOffId, LnfC::TextHi);
}

void MainComponent::loadExternalVst3Plugin(int slot)
{
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;
    juce::File defaultFolder = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
        .getChildFile("Common Files")
        .getChildFile("VST3");

    auto& fileChooser = vst3FileChoosers[(size_t) slot];
    fileChooser = std::make_unique<juce::FileChooser>("Seleccionar plugin VST3",
                                                          defaultFolder.exists() ? defaultFolder : juce::File(),
                                                          "*.vst3");

    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this, slot](const juce::FileChooser& chooser)
    {
        const auto selectedFile = chooser.getResult();
        vst3FileChoosers[(size_t) slot].reset();

        if (selectedFile == juce::File())
            return;

        EffectsChain::PreparedExternalPlugin candidate;
        juce::String errorMessage;
        if (!effectsChain.prepareExternalVst3(selectedFile, candidate, errorMessage))
        {
            updateExternalVstUi(slot);
            lblVst3Status[(size_t) slot].setDisplayText(errorMessage);
            return;
        }

        closeExternalVst3Editor(slot);
        EffectsChain::PreparedExternalPlugin retired;
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            retired = effectsChain.commitExternalPlugin(std::move(candidate), slot);
        }

        updateExternalVstUi(slot);
    });
}

void MainComponent::unloadExternalVst3Plugin(int slot)
{
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;
    closeExternalVst3Editor(slot);

    EffectsChain::PreparedExternalPlugin retired;
    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        retired = effectsChain.detachExternalPlugin(slot);
    }

    updateExternalVstUi(slot);
}

void MainComponent::openExternalVst3Editor(int slot)
{
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;
    auto* plugin = effectsChain.getExternalPluginInstance(slot);
    if (plugin == nullptr || !plugin->hasEditor())
        return;

    auto& editorWindow = vst3EditorWindows[(size_t) slot];
    if (editorWindow != nullptr)
    {
        editorWindow->toFront(true);
        editorWindow->setVisible(true);
        return;
    }

    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorAndMakeActive());
    if (editor == nullptr)
        return;

    editorWindow = std::make_unique<ExternalPluginEditorWindow>("VST3 - " + effectsChain.getExternalPluginName(slot),
                                                                    std::move(editor));
    editorWindow->setVisible(true);
}

void MainComponent::openExternalVst3DrumMapEditor(int slot)
{
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;

    static constexpr LoopernicoMidi::DrumLane mapLanes[] = {
        LoopernicoMidi::DrumLane::Kick, LoopernicoMidi::DrumLane::Snare,
        LoopernicoMidi::DrumLane::ClosedHiHat, LoopernicoMidi::DrumLane::OpenHiHat,
        LoopernicoMidi::DrumLane::Crash, LoopernicoMidi::DrumLane::Ride
    };
    auto content = std::make_unique<MidiMapEditorContent>(effectsChain.getExternalPluginDrumMap(slot));
    auto* contentPtr = content.get();
    const auto pluginName = effectsChain.getExternalPluginName(slot);
    const auto title = "MAPA MIDI - " + (pluginName.isNotEmpty() ? pluginName : "SLOT " + juce::String(slot + 1));
    auto* window = new MidiEditorWindow(title, std::move(content), 560, 560, 520, 420, 1200, 900);
    contentPtr->onResult = [window](int result) { window->finish(result); };
    window->centreAroundComponent(vst3Pedals[(size_t) slot].get(), 560, 560);
    window->enterModalState(true,
                           juce::ModalCallbackFunction::create([this, window, contentPtr, slot](int result)
    {
        if (result == 1)
        {
            for (int i = 0; i < 6; ++i)
                effectsChain.setExternalPluginDrumNote(slot, mapLanes[i], contentPtr->getNote(i));
            for (const auto lane : mapLanes)
                effectsChain.setExternalPluginDrumChannel(slot, lane, contentPtr->getChannel());
        }
        else if (result == 2)
            effectsChain.resetExternalPluginDrumMap(slot);
        delete window;
    }), false);
    window->startDismissOnOutsideClick();
    return;

#if 0

    static constexpr LoopernicoMidi::DrumLane lanes[] = {
        LoopernicoMidi::DrumLane::Kick,
        LoopernicoMidi::DrumLane::Snare,
        LoopernicoMidi::DrumLane::ClosedHiHat,
        LoopernicoMidi::DrumLane::OpenHiHat,
        LoopernicoMidi::DrumLane::Crash,
        LoopernicoMidi::DrumLane::Ride
    };
    static constexpr const char* names[] = {
        "kick", "snare", "closed_hihat", "open_hihat", "crash", "ride"
    };
    static constexpr const char* labels[] = {
        "KICK", "SNARE", "HI-HAT CERRADO", "HI-HAT ABIERTO", "CRASH", "RIDE"
    };

    auto* alert = new ClickOutsideDismissableAlertWindow("MAPA MIDI - " + effectsChain.getExternalPluginName(slot),
                                        "Elegí la nota MIDI para cada instrumento.",
                                        juce::AlertWindow::NoIcon,
                                        this);
    juce::StringArray noteItems;
    for (int note = 0; note <= 127; ++note)
        noteItems.add(juce::String(note));

    const auto& map = effectsChain.getExternalPluginDrumMap(slot);
    for (int i = 0; i < 6; ++i)
    {
        alert->addComboBox(names[i], noteItems, labels[i]);
        if (auto* combo = alert->getComboBoxComponent(names[i]))
            combo->setSelectedId(map.getNote(lanes[i]) + 1, juce::dontSendNotification);
    }

    juce::StringArray channelItems;
    for (int channel = 1; channel <= 16; ++channel)
        channelItems.add(juce::String(channel));
    alert->addComboBox("channel", channelItems, "CANAL MIDI");
    if (auto* combo = alert->getComboBoxComponent("channel"))
        combo->setSelectedId(map.getChannel(LoopernicoMidi::DrumLane::Kick), juce::dontSendNotification);

    alert->addButton("GUARDAR", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("RESTABLECER GM", 2);
    alert->addButton("CANCELAR", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    alert->enableResizing(520, 520, 1200, 900);
    alert->centreAroundComponent(vst3Pedals[(size_t) slot].get(), 560, 560);
    alert->enterModalState(true,
                           juce::ModalCallbackFunction::create([this, alert, slot](int result)
    {
        if (result == 1 || result == 2)
        {
            if (result == 2)
                effectsChain.resetExternalPluginDrumMap(slot);
            else
            {
                for (int i = 0; i < 6; ++i)
                    if (auto* combo = alert->getComboBoxComponent(names[i]))
                        effectsChain.setExternalPluginDrumNote(slot, lanes[i], combo->getSelectedId() - 1);

                if (auto* combo = alert->getComboBoxComponent("channel"))
                    for (const auto lane : lanes)
                        effectsChain.setExternalPluginDrumChannel(slot, lane, combo->getSelectedId());
            }
        }
        delete alert;
    }), false);
    alert->startDismissOnOutsideClick();
#endif
}

void MainComponent::openExternalVst3PatternEditor(int slot)
{
    if (!juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        return;

    auto content = std::make_unique<MidiPatternEditorContent>(effectsChain.getExternalPluginDrumPattern(slot));
    auto* contentPtr = content.get();
    const auto pluginName = effectsChain.getExternalPluginName(slot);
    const auto title = "PATRON MIDI - " + (pluginName.isNotEmpty() ? pluginName : "SLOT " + juce::String(slot + 1));
    auto* window = new MidiEditorWindow(title, std::move(content), 1000, 500, 980, 430, 1500, 900);
    contentPtr->onResult = [window](int result) { window->finish(result); };
    contentPtr->setPatternChangedCallback([this, slot](const LoopernicoMidi::DrumMidiPattern& pattern)
    {
        effectsChain.setExternalPluginDrumPattern(slot, pattern);
    });
    window->centreAroundComponent(vst3Pedals[(size_t) slot].get(), 1000, 500);
    window->enterModalState(true,
                           juce::ModalCallbackFunction::create([this, window, contentPtr, slot](int result)
    {
            if (result == 1)
                effectsChain.setExternalPluginDrumPattern(slot, contentPtr->getPattern());
            else if (result == 0)
                effectsChain.setExternalPluginDrumPattern(slot, contentPtr->getInitialPattern());
        delete window;
    }), false);
    window->startDismissOnOutsideClick();
    return;

#if 0

    auto* editor = new DrumPatternEditorComponent(effectsChain.getExternalPluginDrumPattern(slot));
    auto* alert = new ClickOutsideDismissableAlertWindow("PATRON MIDI - " + effectsChain.getExternalPluginName(slot),
                                        "Edita la grilla sin reproducirla todavia.",
                                        juce::AlertWindow::NoIcon,
                                        this);
    alert->addCustomComponent(editor);
    alert->addButton("GUARDAR", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("LIMPIAR", 2);
    alert->addButton("CANCELAR", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    alert->enableResizing(980, 500, 1500, 900);
    alert->centreAroundComponent(vst3Pedals[(size_t) slot].get(), 1000, 500);
    alert->enterModalState(true,
                           juce::ModalCallbackFunction::create([this, alert, editor, slot](int result)
    {
        if (result == 1)
        {
            effectsChain.setExternalPluginDrumPattern(slot, editor->getPattern());
        }
        else if (result == 2)
        {
            editor->clearPattern();
            effectsChain.setExternalPluginDrumPattern(slot, editor->getPattern());
        }

        delete alert;
    }), false);
    alert->startDismissOnOutsideClick();
#endif
}

void MainComponent::closeExternalVst3Editor(int slot)
{
    if (slot < 0)
    {
        for (auto& editor : vst3EditorWindows)
            editor.reset();
    }
    else if (juce::isPositiveAndBelow(slot, kMaxVst3Pedals))
        vst3EditorWindows[(size_t) slot].reset();
}

void MainComponent::updateExternalVstUi(int requestedSlot)
{
    const int first = requestedSlot < 0 ? 0 : requestedSlot;
    const int last = requestedSlot < 0 ? kMaxVst3Pedals : requestedSlot + 1;
    for (int slot = first; slot < last; ++slot)
    {
        const bool hasPlugin = effectsChain.hasExternalPlugin(slot);
        const bool pluginEnabled = effectsChain.isExternalPluginEnabled(slot);
        const auto pluginName = effectsChain.getExternalPluginName(slot);
        btnUnloadVst3[(size_t) slot].setEnabled(hasPlugin);
        btnEditVst3[(size_t) slot].setEnabled(hasPlugin);
        // El mapa y el patron pertenecen al slot, no a la instancia cargada.
        // Se pueden preparar antes de elegir o cargar el VST3.
        btnMapVst3[(size_t) slot].setEnabled(true);
        btnPatternVst3[(size_t) slot].setEnabled(true);
        btnPatternPlayVst3[(size_t) slot].setEnabled(hasPlugin);
        btnPatternPlayVst3[(size_t) slot].setToggleState(
            hasPlugin && effectsChain.isExternalPluginPatternPlaying(slot),
            juce::dontSendNotification);
        btnPatternPlayVst3[(size_t) slot].setButtonText(
            effectsChain.isExternalPluginPatternPlaying(slot) ? "STOP" : "PLAY");
        btnBypassVst3[(size_t) slot].setEnabled(hasPlugin);
        btnBypassVst3[(size_t) slot].setToggleState(pluginEnabled, juce::dontSendNotification);
        btnBypassVst3[(size_t) slot].setButtonText(pluginEnabled ? "ENGINE ACTIVE" : "ENGINE BYPASS");
        if (auto* pedal = vst3Pedals[(size_t) slot].get())
        {
            pedal->onBtn.setToggleState(hasPlugin && pluginEnabled, juce::dontSendNotification);
            pedal->repaint();
        }
        auto& status = lblVst3Status[(size_t) slot];
        if (!hasPlugin)
            status.setDisplayText("SIN VST");
        else if (pluginEnabled)
            status.setDisplayText(pluginName + " / LAT "
                                  + juce::String(effectsChain.getExternalPluginReportedLatencySamples(slot))
                                  + " SMP");
        else
            status.setDisplayText("BYPASS - " + pluginName);
        status.setColour(juce::Label::textColourId,
                         !hasPlugin ? LnfC::TextDim : (pluginEnabled ? juce::Colour(0xff87edf2) : LnfC::OvdAmber));
        status.setColour(juce::Label::outlineColourId,
                         (pluginEnabled ? juce::Colour(0xff55dce5) : LnfC::OvdAmber)
                            .withAlpha(hasPlugin ? 0.52f : 0.20f));
    }
    updateRecordingLatencyCompensation();
}

bool MainComponent::captureLooperSnapshotIncrementally(LooperEngine::SessionSnapshot& snapshot,
                                                       juce::String& errorMessage)
{
    // Keep each callback-lock hold comfortably below a 64/128-sample period.
    constexpr int kChunkSamplesPerChannel = 1024;
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

        juce::String errorMessage;
        if (!saveCurrentSessionToFile(selectedFile, errorMessage))
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

bool MainComponent::saveCurrentSessionToFile(const juce::File& sessionFile,
                                             juce::String& errorMessage)
{
    LooperEngine::SessionSnapshot looperSnapshot;
    juce::MemoryBlock vstState;
    if (!captureLooperSnapshotIncrementally(looperSnapshot, errorMessage))
        return false;

    std::array<juce::AudioPluginInstance*, kMaxVst3Pedals> suspendedPlugins{};
    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
            if ((suspendedPlugins[(size_t) slot] = effectsChain.getExternalPluginInstance(slot)) != nullptr)
                suspendedPlugins[(size_t) slot]->suspendProcessing(true);
    }

    std::array<juce::MemoryBlock, kMaxVst3Pedals> states;
    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
        effectsChain.copyExternalPluginState(states[(size_t) slot], slot);
    vstState = packVstStates(states);

    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
            if (suspendedPlugins[(size_t) slot] != nullptr
                && effectsChain.getExternalPluginInstance(slot) == suspendedPlugins[(size_t) slot])
                suspendedPlugins[(size_t) slot]->suspendProcessing(false);
    }

    const auto sessionTree = createSessionTree(looperSnapshot, vstState);
    return SessionManager::writeSessionFile(sessionFile, sessionTree,
                                            looperSnapshot, vstState, errorMessage);
}

bool MainComponent::saveSessionFileForSnapshot(const juce::File& sessionFile,
                                               juce::String& errorMessage)
{
    const bool saved = saveCurrentSessionToFile(sessionFile, errorMessage);
    audioDeviceController.log(saved
        ? "Diagnostico de guardado durante reproduccion correcto: " + sessionFile.getFullPathName()
        : "Diagnostico de guardado fallo: " + errorMessage);
    return saved;
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
    for (int i = 0; i < kNumFxPedals; ++i)
    {
        if (fxPedals[i] == nullptr)
            continue;
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

    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
    {
        juce::ValueTree vst("VST");
        vst.setProperty("index", slot, nullptr);
        vst.setProperty("slot", (int) (std::find(fxOrder.begin(), fxOrder.end(), kNumFxPedals + slot)
                                        - fxOrder.begin()), nullptr);
        vst.setProperty("path", effectsChain.getExternalPluginPath(slot), nullptr);
        vst.setProperty("enabled", effectsChain.isExternalPluginEnabled(slot), nullptr);
        vst.setProperty("hasState", vstState.getSize() > 0, nullptr);
        const auto& pattern = effectsChain.getExternalPluginDrumPattern(slot);
        vst.setProperty("patternBars", pattern.getBars(), nullptr);
        vst.setProperty("patternData", serializeDrumPattern(pattern), nullptr);
        fx.addChild(vst, -1, nullptr);
    }
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
    const auto vstStates = unpackVstStates(vstState);
    std::array<juce::String, kMaxVst3Pedals> pluginPaths;
    std::array<bool, kMaxVst3Pedals> pluginEnabled{};
    std::array<int, kMaxVst3Pedals> patternBars{};
    std::array<juce::String, kMaxVst3Pedals> patternData;
    std::array<EffectsChain::PreparedExternalPlugin, kMaxVst3Pedals> candidates;
    int vstNodeCount = 0;
    for (int i = 0; i < fx.getNumChildren(); ++i)
    {
        const auto node = fx.getChild(i);
        if (!node.hasType("VST"))
            continue;
        const int slot = juce::jlimit(0, kMaxVst3Pedals - 1,
                                      (int) node.getProperty("index", vstNodeCount));
        ++vstNodeCount;
        pluginPaths[(size_t) slot] = node.getProperty("path").toString();
        pluginEnabled[(size_t) slot] = (bool) node.getProperty("enabled", true);
        patternBars[(size_t) slot] = (int) node.getProperty("patternBars", 1);
        patternData[(size_t) slot] = node.getProperty("patternData", "").toString();
        if (pluginPaths[(size_t) slot].isNotEmpty())
        {
            juce::String pluginError;
            if (!effectsChain.prepareExternalVst3(juce::File(pluginPaths[(size_t) slot]),
                                                  candidates[(size_t) slot], pluginError,
                                                  &vstStates[(size_t) slot]))
            {
                errorMessage = "VST " + juce::String(slot + 1) + ": " + pluginError;
                return false;
            }
        }
    }
    auto preparedLooper = looper.prepareSessionRestore(looperSnapshot);

    setSlider(masterKnob, (double) global.getProperty("masterVolume", 0.85));
    outputMono.store((bool) global.getProperty("outputMono", true), std::memory_order_relaxed);
    updateOutputModeButton();
    const bool legacySingleVst = vstNodeCount <= 1;
    std::array<int, kChainItemCount> loadedOrder{};
    loadedOrder.fill(-1);
    std::array<bool, kChainItemCount> seenSlots{};
    std::array<bool, kChainItemIdLimit> seenItems{};
    for (int i = 0; i < fx.getNumChildren(); ++i)
    {
        const auto pedal = fx.getChild(i);
        int item = -1;
        int desiredSlot = -1;
        if (pedal.hasType("PEDAL"))
        {
            item = juce::jlimit(0, kNumFxPedals - 1, (int) pedal.getProperty("index", 0));
            if (item == EffectsChain::RhythmPedal || item == EffectsChain::LegacySustainGranPedal)
                continue;
            desiredSlot = (int) pedal.getProperty("slot", item) + (legacySingleVst ? 1 : 0);
        }
        else if (pedal.hasType("VST"))
        {
            const int vstIndex = juce::jlimit(0, kMaxVst3Pedals - 1,
                                               (int) pedal.getProperty("index", 0));
            item = kNumFxPedals + vstIndex;
            desiredSlot = legacySingleVst ? 0 : (int) pedal.getProperty("slot", item);
        }
        if (item < 0 || item >= kChainItemIdLimit || seenItems[(size_t) item])
            continue;
        desiredSlot = juce::jlimit(0, kChainItemCount - 1, desiredSlot);
        while (seenSlots[(size_t) desiredSlot])
            desiredSlot = (desiredSlot + 1) % kChainItemCount;
        loadedOrder[(size_t) desiredSlot] = item;
        seenSlots[(size_t) desiredSlot] = true;
        seenItems[(size_t) item] = true;
    }
    for (int slot = 0, nextItem = 0; slot < kChainItemCount; ++slot)
    {
        if (seenSlots[(size_t) slot])
            continue;
        while (nextItem < kChainItemIdLimit
               && (nextItem == EffectsChain::RhythmPedal || seenItems[(size_t) nextItem]))
            ++nextItem;
        if (nextItem >= kChainItemIdLimit)
            break;
        loadedOrder[(size_t) slot] = nextItem;
        seenItems[(size_t) loadedOrder[(size_t) slot]] = true;
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

    closeExternalVst3Editor();
    std::array<EffectsChain::PreparedExternalPlugin, kMaxVst3Pedals> retired;

    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        if (!looper.commitPreparedSessionRestore(std::move(preparedLooper)))
        {
            errorMessage = "El dispositivo de audio cambio mientras se preparaba la sesion. Intenta abrirla nuevamente.";
            return false;
        }

        for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
        {
            if (pluginPaths[(size_t) slot].isNotEmpty())
            {
                retired[(size_t) slot] = effectsChain.commitExternalPlugin(
                    std::move(candidates[(size_t) slot]), slot);
                effectsChain.setExternalPluginEnabled(pluginEnabled[(size_t) slot], slot);
            }
            else
                retired[(size_t) slot] = effectsChain.detachExternalPlugin(slot);

            LoopernicoMidi::DrumMidiPattern pattern;
            if (deserializeDrumPattern(patternData[(size_t) slot], patternBars[(size_t) slot], pattern))
            {
                effectsChain.setExternalPluginDrumPattern(slot, pattern);
            }
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
    sldLooperTempo.setValue(looper.getBpm(), juce::dontSendNotification);
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

    if (pedalIndex == EffectsChain::RhythmPedal)
        return;

    static constexpr const char* pedalNames[kNumFxPedals] = {
        "NOISE GATE", "REMOVED", "COMPRESSOR", "HARMONIZER",
        "REMOVED", "SUSTAIN CHORDS", "FUZZ", "SUNSET", "CHORUS",
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
    deviceManager.removeChangeListener(this);
    midiDeviceListConnection.reset();
    closeExternalVst3Editor();
    setLookAndFeel(nullptr);
    auto devices = juce::MidiInput::getAvailableDevices();
    for (auto& d : devices) deviceManager.removeMidiInputDeviceCallback(d.identifier, this);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    deviceManager.closeAudioDevice();
    audioDeviceController.flushPendingSettings();
}

// ============================================================================
// AUDIO
// ============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    looper.prepare(sampleRate, samplesPerBlockExpected, formatManager);
    effectsChain.prepare(sampleRate, samplesPerBlockExpected);
    processedInputBuffer.setSize(2, juce::jmax(16, samplesPerBlockExpected), false, true, false);
    rhythmMonitorBuffer.setSize(2, juce::jmax(16, samplesPerBlockExpected), false, true, false);
    processedInputBuffer.clear();
    rhythmMonitorBuffer.clear();
    for (int i = 0; i < (int)visualTracks.size(); ++i)
        visualTracks[i]->setThumbnailPointer(looper.getTrackThumbnail(i));
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& buf)
{
    if (!buf.buffer) return;
    const bool monoOutput = outputMono.load(std::memory_order_relaxed);
    const int outputChannels = juce::jmin(2, buf.buffer->getNumChannels());
    if (buf.numSamples <= 0 || outputChannels <= 0
        || processedInputBuffer.getNumChannels() < 2
        || rhythmMonitorBuffer.getNumChannels() < 2
        || processedInputBuffer.getNumSamples() < buf.numSamples
        || rhythmMonitorBuffer.getNumSamples() < buf.numSamples)
    {
        buf.clearActiveBufferRegion();
        return;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        if (ch < buf.buffer->getNumChannels())
            processedInputBuffer.copyFrom(ch, 0, *buf.buffer, ch, buf.startSample, buf.numSamples);
        else
            processedInputBuffer.clear(ch, 0, buf.numSamples);
    }

    // La entrada de instrumento puede estar conectada a cualquiera de los dos
    // canales fisicos. Algunos VST3 procesan ambos, pero sus afinadores solo
    // inspeccionan la entrada 1. Conservamos el nivel del canal claramente
    // dominante (el caso habitual de una guitarra mono) y, si hay una fuente
    // realmente estereo, hacemos un downmix convencional. Desde aqui la misma
    // senal alimenta ambos canales, antes de los efectos y del looper.
    const float leftMagnitude = processedInputBuffer.getMagnitude(0, 0, buf.numSamples);
    const float rightMagnitude = processedInputBuffer.getMagnitude(1, 0, buf.numSamples);
    const bool useLeftOnly = leftMagnitude > rightMagnitude * 2.0f;
    const bool useRightOnly = rightMagnitude > leftMagnitude * 2.0f;
    auto* rawInputLeft = processedInputBuffer.getWritePointer(0);
    auto* rawInputRight = processedInputBuffer.getWritePointer(1);
    for (int i = 0; i < buf.numSamples; ++i)
    {
        const float monoInput = useLeftOnly ? rawInputLeft[i]
                              : useRightOnly ? rawInputRight[i]
                                             : 0.5f * (rawInputLeft[i] + rawInputRight[i]);
        rawInputLeft[i] = rawInputRight[i] = monoInput;
    }

    rhythmMonitorBuffer.clear(0, buf.numSamples);

    {
        float peak = 0.0f;
        for (int ch = 0; ch < outputChannels; ++ch) {
            auto* d = processedInputBuffer.getReadPointer(ch);
            for (int i = 0; i < buf.numSamples; ++i) peak = std::max(peak, std::abs(d[i]));
        }
        float prev = inputVU.load(std::memory_order_relaxed);
        if (peak > prev) inputVU.store(peak, std::memory_order_relaxed);
    }

    const auto looperTransportState = looper.getState();
    const bool looperIsRecording = looperTransportState == LooperEngine::State::RecordingBase
                                || looperTransportState == LooperEngine::State::RecordingLayer;
    const bool looperIsPlaying = looperIsRecording
                              || looperTransportState == LooperEngine::State::Playing;
    effectsChain.setExternalPluginTransport(looper.getBpm(), looper.getBeatsPerBar(),
                                            looperIsPlaying
                                                || effectsChain.isAnyExternalPluginPatternPlaying(),
                                            looperIsRecording, looperIsPlaying);
    effectsChain.processBlock(processedInputBuffer, rhythmMonitorBuffer, 0, buf.numSamples);

    if (monoOutput)
    {
        auto* inputLeft = processedInputBuffer.getWritePointer(0);
        auto* inputRight = processedInputBuffer.getWritePointer(1);
        auto* rhythmLeft = rhythmMonitorBuffer.getWritePointer(0);
        auto* rhythmRight = rhythmMonitorBuffer.getWritePointer(1);
        for (int i = 0; i < buf.numSamples; ++i)
        {
            const float monoInput = 0.5f * (inputLeft[i] + inputRight[i]);
            inputLeft[i] = inputRight[i] = monoInput;
            const float monoRhythm = 0.5f * (rhythmLeft[i] + rhythmRight[i]);
            rhythmLeft[i] = rhythmRight[i] = monoRhythm;
        }
    }

    buf.clearActiveBufferRegion();
    const bool monitorInput = audioDeviceController.isSoftwareInputMonitoringEnabled()
                           && !effectsChain.isRhythmSoloMode();
    looper.processBlock(processedInputBuffer, 0, *buf.buffer, buf.startSample,
                        buf.numSamples, monitorInput, monoOutput);

    const float master = masterVolume.load(std::memory_order_relaxed);
    for (int ch = 0; ch < outputChannels; ++ch)
    {
        auto* output = buf.buffer->getWritePointer(ch, buf.startSample);
        const auto* rhythm = rhythmMonitorBuffer.getReadPointer(ch);
        for (int i = 0; i < buf.numSamples; ++i)
            output[i] = LoopernicoLatency::mixOutputSample(
                output[i],
                LoopernicoLatency::separateRhythmMonitorSample(rhythm[i], monitorInput),
                master);
    }

    if (monoOutput && outputChannels > 1)
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

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source != &deviceManager)
        return;

    audioDeviceController.handleDeviceManagerChange();
    updateRecordingLatencyCompensation();
    updateAudioStatusUi();
}

void MainComponent::showAudioSettings()
{
    if (audioSettingsPanel == nullptr)
        return;

    closeFxPedalFocus();
    audioSettingsPanel->setBounds(getLocalBounds());
    audioSettingsPanel->refresh(effectsChain.getActiveExternalPluginLatencySamples());
    audioSettingsPanel->setVisible(true);
    audioSettingsPanel->toFront(true);
    audioSettingsPanel->grabKeyboardFocus();
}

void MainComponent::showAudioSettingsForSnapshot()
{
    showAudioSettings();
}

bool MainComponent::setAudioBufferForSnapshot(int bufferSamples)
{
    if (bufferSamples <= 0 || deviceManager.getCurrentAudioDevice() == nullptr)
        return false;

    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = bufferSamples;
    const auto error = deviceManager.setAudioDeviceSetup(setup, true);
    audioDeviceController.handleDeviceManagerChange();
    updateRecordingLatencyCompensation();
    updateAudioStatusUi();
    if (error.isNotEmpty())
    {
        audioDeviceController.log("Diagnostico de buffer " + juce::String(bufferSamples)
                                  + " fallo: " + error);
        return false;
    }

    deviceManager.sendSynchronousChangeMessage();
    audioDeviceController.log("Diagnostico de buffer solicitado: " + juce::String(bufferSamples));
    return true;
}

void MainComponent::setSoftwareInputMonitoringForSnapshot(bool enabled)
{
    audioDeviceController.setSoftwareInputMonitoringEnabled(enabled);
    updateAudioStatusUi();
}

bool MainComponent::loadExternalVst3ForSnapshot(const juce::File& pluginFile, bool enabled)
{
    EffectsChain::PreparedExternalPlugin candidate;
    juce::String error;
    if (!effectsChain.prepareExternalVst3(pluginFile, candidate, error))
    {
        lblVst3Status[0].setDisplayText(error);
        audioDeviceController.log("Diagnostico VST3 fallo: " + error);
        return false;
    }

    EffectsChain::PreparedExternalPlugin retired;
    {
        const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
        retired = effectsChain.commitExternalPlugin(std::move(candidate));
        effectsChain.setExternalPluginEnabled(enabled);
    }
    updateExternalVstUi();
    audioDeviceController.log("Diagnostico VST3 activo: " + effectsChain.getExternalPluginName()
                              + ", latencia="
                              + juce::String(effectsChain.getActiveExternalPluginLatencySamples()));
    return true;
}

void MainComponent::closeAudioSettings()
{
    if (audioSettingsPanel != nullptr)
        audioSettingsPanel->setVisible(false);
}

void MainComponent::updateRecordingLatencyCompensation()
{
    const auto status = audioDeviceController.getStatus();
    LoopernicoLatency::Breakdown latency;
    latency.inputSamples = status.inputLatencySamples;
    latency.outputSamples = status.outputLatencySamples;
    latency.pluginSamples = effectsChain.getActiveExternalPluginLatencySamples();
    latency.calibrationSamples = status.manualCalibrationSamples;
    looper.setRecordingCompensationSamples(latency.effectiveSamples());
}

void MainComponent::updateAudioStatusUi()
{
    const auto status = audioDeviceController.getStatus();
    if (!status.audioAvailable)
    {
        lblAudioStatus.setText("AUDIO NO DISPONIBLE - ABRIR AUDIO", juce::dontSendNotification);
        lblAudioStatus.setColour(juce::Label::textColourId, LnfC::RecRed);
        btnAudioSettings.setButtonText("CONFIGURACION !");
        return;
    }

    LoopernicoLatency::Breakdown latency;
    latency.inputSamples = status.inputLatencySamples;
    latency.outputSamples = status.outputLatencySamples;
    latency.pluginSamples = effectsChain.getActiveExternalPluginLatencySamples();
    latency.calibrationSamples = status.manualCalibrationSamples;
    const double totalMs = status.sampleRate > 0.0
        ? 1000.0 * latency.driverRoundTripSamples() / status.sampleRate : 0.0;

    juce::String deviceName = status.inputDevice == status.outputDevice
        ? status.inputDevice : status.inputDevice + " / " + status.outputDevice;
    juce::String text;
    text << status.driverType << "  |  " << deviceName
         << "  |  " << juce::String(status.sampleRate / 1000.0, 1) << " kHz"
         << "  |  " << status.bufferSize << " smp"
         << "  |  IN " << status.inputLatencySamples
         << " + OUT " << status.outputLatencySamples
         << " = " << juce::String(totalMs, 3) << " ms"
         << "  |  VST " << latency.pluginSamples
         << "  |  COMP " << latency.effectiveSamples()
         << "  |  MON " << (status.softwareInputMonitoringEnabled ? "SW" : "DIRECT")
         << "  |  XRUN "
         << (status.xRunsAvailable ? juce::String(status.xRuns) : "N/D");
    lblAudioStatus.setText(text, juce::dontSendNotification);

    const bool warning = status.usingFallback || status.aggressiveBuffer
                      || (status.xRunsAvailable && status.xRuns > 0);
    lblAudioStatus.setColour(juce::Label::textColourId, warning ? LnfC::OvdAmber : LnfC::PlayGreen);
    btnAudioSettings.setButtonText(warning ? "CONFIGURACION !" : "CONFIGURACION");

    if (audioSettingsPanel != nullptr && audioSettingsPanel->isVisible())
        audioSettingsPanel->refresh(latency.pluginSamples);
}

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
    audioDeviceController.poll();

    if (effectsChain.consumeExternalPluginLatencyRefreshRequest())
    {
        {
            const juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            effectsChain.refreshExternalPluginLatency();
        }
        updateRecordingLatencyCompensation();
        updateExternalVstUi();
    }

    if (looper.consumeFixedLoopCompletionNotification() && looper.onFixedLoopComplete)
        looper.onFixedLoopComplete();

    static int audioStatusRefreshDivider = 0;
    if (++audioStatusRefreshDivider >= 6)
    {
        audioStatusRefreshDivider = 0;
        updateRecordingLatencyCompensation();
        updateAudioStatusUi();
    }

    updateLoopEditButtons();
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

void MainComponent::setMidiMenuExpanded(bool expanded)
{
    midiMenuExpanded = expanded;
    midiMenuSurface.setVisible(expanded);
    btnMidiMenu.setButtonText(expanded ? "MIDI -" : "MIDI +");
    btnMidiMenu.setToggleState(expanded, juce::dontSendNotification);

    if (expanded)
        midiMenuSurface.grabKeyboardFocus();

    resized();
}

void MainComponent::updateLoopEditButtons()
{
    const auto state = looper.getState();
    const bool canEditLoop = looper.getBaseLoopSamples() > 0
        && state != LooperEngine::State::ArmedBase
        && state != LooperEngine::State::RecordingBase
        && state != LooperEngine::State::RecordingLayer;

    const int sourceSamples = looper.getEditableLoopSourceSamples();
    const int baseSamples = looper.getBaseLoopSamples();
    if (sourceSamples <= 1 || baseSamples <= 1)
    {
        loopMarkerInitialised = false;
        loopMarkerStart = 0.0f;
        loopMarkerEnd = 1.0f;
        loopRangeMarker.setRange(loopMarkerStart, loopMarkerEnd);
    }
    else if (!loopMarkerInitialised)
    {
        loopMarkerStart = 0.0f;
        loopMarkerEnd = juce::jlimit(0.01f, 1.0f, (float) baseSamples / (float) sourceSamples);
        loopRangeMarker.setRange(loopMarkerStart, loopMarkerEnd);
        loopMarkerInitialised = true;
    }
    loopRangeMarker.setVisible(canEditLoop);
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
    layoutHeaderRow(rightHeaderZone, 13, 144,
                    { &btnSaveSession, &btnLoadSession, &btnAudioSettings });
    layoutHeaderRow(rightHeaderZone, 49, 144,
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

    int totalPedalWidth = 10;
    for (int slot = 0; slot < kChainItemCount; ++slot)
        totalPedalWidth += fxPedalPitch(fxOrder[(size_t) slot]);
    fxContainer.setSize(totalPedalWidth, fxViewport.getHeight());

    fxPedalFocusOverlay.setBounds(getLocalBounds());
    if (focusedFxPedalIndex != -2)
        fxPedalFocusOverlay.resized();

    for (int slot = 0; slot < kMaxVst3Pedals; ++slot)
    {
        const int chainItem = kNumFxPedals + slot;
        auto* pedal = vst3Pedals[(size_t) slot].get();
        const auto found = std::find(fxOrder.begin(), fxOrder.end(), chainItem);
        if (pedal != nullptr && found != fxOrder.end())
        {
            const int orderSlot = (int) std::distance(fxOrder.begin(), found);
            if (focusedFxPedalIndex != chainItem && draggingFxPedal != chainItem)
                pedal->setBounds(fxSlotStartX(orderSlot) + 14, 4, kVst3PedalBodyWidth, fxContainer.getHeight() - 8);
            layoutVst3PedalControls(*pedal);
        }
    }

    for (int slot = 0; slot < kChainItemCount; ++slot)
    {
        const int pedalIndex = fxOrder[(size_t) slot];
        if (pedalIndex >= kNumFxPedals)
            continue;
        if (pedalIndex == draggingFxPedal || pedalIndex == focusedFxPedalIndex)
            continue;

        fxPedals[pedalIndex]->setBounds(fxSlotStartX(slot) + 14, 4,
                                        fxPedalBodyWidth(pedalIndex), fxContainer.getHeight() - 8);
    }

    if (focusedFxPedalIndex != -2)
        fxPedalFocusOverlay.toFront(false);

    lblSilenceThreshold.setBounds(fullX + 24, mY + 5, 100, 18);
    sldSilenceThreshold.setBounds(fullX + 126, mY + 5, 300, 18);
    lblLooperTempo.setBounds(fullX + 438, mY + 5, 58, 18);
    sldLooperTempo.setBounds(fullX + 502, mY + 5, 126, 18);
    btnLooperTap.setBounds(fullX + 638, mY + 5, 50, 18);
    lblAudioStatus.setBounds(fullX + 700, mY + 5, fullW - 720, 18);

    if (audioSettingsPanel != nullptr)
    {
        audioSettingsPanel->setBounds(getLocalBounds());
        if (audioSettingsPanel->isVisible())
            audioSettingsPanel->toFront(false);
    }

    const int footerY = H - outer - sH;
    const int midiControlsY = footerY + (sH - 24) / 2;
    const int outputZoneX = W - kFooterInset - kFooterOutputZoneWidth;

    btnOutputMode.setBounds(outputZoneX, midiControlsY, 96, 24);
    btnMuteAll.setBounds(outputZoneX + 104, midiControlsY, 96, 24);
    masterKnob.setBounds(outputZoneX + 225, footerY + 3, 34, 34);
    masterVUMeter.setBounds(outputZoneX + 282, midiControlsY, 116, 24);

    const int midiButtonWidth = 108;
    btnMidiMenu.setBounds(kFooterInset, midiControlsY, midiButtonWidth, 24);

    const int midiMenuW = juce::jmin(kMidiMenuWidth, juce::jmax(620, W - kFooterInset * 2));
    const int midiMenuH = kMidiMenuHeight;
    midiMenuSurface.setBounds(kFooterInset, juce::jmax(outer, footerY - midiMenuH - 6),
                              midiMenuW, midiMenuH);
    if (midiMenuExpanded)
    {
        midiDeviceSelector.setBounds(14, 28, kMidiDeviceWidth, 24);
        midiActionSelector.setBounds(14 + kMidiDeviceWidth + kMidiControlGap, 28,
                                     kMidiActionWidth, 24);
        btnMidiLearn.setBounds(14 + kMidiDeviceWidth + kMidiControlGap
                                   + kMidiActionWidth + kMidiControlGap,
                               28, kMidiLearnWidth, 24);

        const int mappingX = 14 + kMidiDeviceWidth + kMidiControlGap
                           + kMidiActionWidth + kMidiControlGap + kMidiLearnWidth + kMidiControlGap;
        lblMidiMapping.setBounds(mappingX, 29, juce::jmax(120, midiMenuW - mappingX - 14), 22);

        midiMessageTypeSelector.setBounds(14, 60, kMidiTypeWidth, 24);
        midiNumberSelector.setBounds(14 + kMidiTypeWidth + kMidiControlGap, 60,
                                     kMidiNumberWidth, 24);
        const int midiClearX = 14 + kMidiTypeWidth + kMidiControlGap
                             + kMidiNumberWidth + kMidiControlGap;
        const int midiSaveX = midiClearX + kMidiClearWidth + kMidiControlGap;
        btnMidiClear.setBounds(midiClearX, 60, kMidiClearWidth, 24);
        btnMidiSave.setBounds(midiSaveX, 60, kMidiSaveWidth, 24);
        lblMidiStatus.setBounds(midiSaveX + kMidiSaveWidth + kMidiControlGap, 63,
                                juce::jmax(100, midiMenuW - midiSaveX - kMidiSaveWidth - 28), 18);
    }

    // El marcador es una pista maestra visual: vive sobre la forma de onda de
    // la primera pista y no ocupa la hilera superior de controles.
    const int firstTrackWaveX = fullX + 4 + 180;
    loopRangeMarker.setBounds(firstTrackWaveX, mY + kTrackViewportTopInset + 5,
                              juce::jmax(40, fullW - 8 - 180), 30);
}
