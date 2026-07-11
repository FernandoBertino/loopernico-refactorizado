#pragma once

#include <JuceHeader.h>
#include "LoopernicoLookAndFeel.h"

class ToggleSwitchComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(on ? LnfC::OvdAmber : LnfC::TextDim);
        g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
        g.drawText("DROP TUNE", 0, 0, 72, (int)b.getHeight(), juce::Justification::centredLeft);

        const float sw = 34.f, sh = 16.f;
        const float sx = 74.f, sy = b.getCentreY() - sh * .5f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(sx - 2.0f, sy - 2.0f, sw + 4.0f, sh + 4.0f, (sh + 4.0f) * 0.5f);
        g.setColour(on ? LnfC::OvdAmber.darker(.25f) : juce::Colour(0xff171a1f));
        g.fillRoundedRectangle(sx, sy, sw, sh, sh * .5f);
        g.setColour(on ? LnfC::OvdAmber.brighter(.2f) : LnfC::Border.withAlpha(0.85f));
        g.drawRoundedRectangle(sx, sy, sw, sh, sh * .5f, 1.f);

        float cr = sh * .38f;
        float cx = on ? sx + sw - cr - 3.f : sx + cr + 3.f;
        float cy = sy + sh * .5f;
        if (on)
        {
            g.setColour(LnfC::OvdAmber.withAlpha(0.22f));
            g.fillEllipse(cx - cr * 2.4f, cy - cr * 2.4f, cr * 4.8f, cr * 4.8f);
        }
        g.setColour(on ? juce::Colours::white : LnfC::TextDim.brighter(.2f));
        g.fillEllipse(cx - cr, cy - cr, cr * 2, cr * 2);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawEllipse(cx - cr, cy - cr, cr * 2, cr * 2, 0.8f);
    }

    void mouseDown(const juce::MouseEvent&) override { on = !on; repaint(); if (onClick) onClick(); }
    bool getToggleState() const { return on; }
    void setToggleState(bool v, juce::NotificationType) { on = v; repaint(); }

    std::function<void()> onClick;

private:
    bool on = false;
};

class VUMeter : public juce::Component
{
public:
    void setLevel(float linear)
    {
        float db = (linear > 0.00001f) ? 20.0f * std::log10(linear) : -96.0f;
        float mapped = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
        if (!juce::approximatelyEqual(mapped, level)) { level = mapped; repaint(); }
    }

    void paint(juce::Graphics& g) override;

private:
    float level = 0.0f;
};

class MasterVUMeter : public juce::Component
{
public:
    void setLevels(float l, float r)
    {
        bool changed = !juce::approximatelyEqual(levelL, l)
                    || !juce::approximatelyEqual(levelR, r);
        levelL = l;
        levelR = r;
        if (changed) repaint();
    }

    void paint(juce::Graphics& g) override;

private:
    float levelL = 0.0f, levelR = 0.0f;
    void drawBar(juce::Graphics& g, juce::Rectangle<float> bar, float level);
};

class StepButton : public juce::Component
{
public:
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override { active = !active; repaint(); if (onChanged) onChanged(active); }
    bool getActive() const { return active; }
    void setActive(bool a) { if (active != a) { active = a; repaint(); } }
    void setCurrent(bool c) { if (isCur != c) { isCur = c; repaint(); } }

    std::function<void(bool)> onChanged;

private:
    bool active = false, isCur = false;
};

class CircularButton : public juce::Component
{
public:
    enum Type { Rec, Play, Ovd };
    explicit CircularButton(Type t) : type(t) {}

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp(const juce::MouseEvent&) override { pressed = false; repaint(); if (onClick) onClick(); }
    void mouseEnter(const juce::MouseEvent&) override { hovered = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hovered = false; repaint(); }

    void setActive(bool a) { if (active != a) { active = a; repaint(); } }
    bool getActive() const { return active; }

    void setCustomColour(juce::Colour c) { customCol = c; hasCustomCol = true; repaint(); }
    void clearCustomColour() { hasCustomCol = false; repaint(); }

    std::function<void()> onClick;

private:
    Type type;
    bool active = false, hovered = false, pressed = false;
    bool hasCustomCol = false;
    juce::Colour customCol;

    juce::Colour getBaseColour() const
    {
        if (hasCustomCol) return customCol;
        switch (type) {
            case Rec:  return LnfC::RecRed;
            case Play: return LnfC::PlayGreen;
            case Ovd:  return LnfC::OvdAmber;
        }
        return LnfC::Border;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircularButton)
};

class TrackComponent : public juce::Component
{
public:
    explicit TrackComponent(int trackIndex);
    ~TrackComponent() override = default;

    void setThumbnailPointer(juce::AudioThumbnail* t) { thumbnail = t; }
    bool isMuteActive() const { return muteBtn.getToggleState(); }
    bool isSoloActive() const { return soloBtn.getToggleState(); }
    void setMuteActive(bool active) { muteBtn.setToggleState(active, juce::dontSendNotification); }
    void setSoloActive(bool active) { soloBtn.setToggleState(active, juce::dontSendNotification); }
    void setVolumeValue(float value) { volumeSlider.setValue(value, juce::dontSendNotification); }
    float getVolumeValue() const { return (float) volumeSlider.getValue(); }
    void setPlayheadProgress(float p);
    void setStatus(bool rec, bool play, bool ovd);
    void setLoopSecs(float t, float len);
    void setVULevel(float l) { vuMeter.setLevel(l); }

    std::function<void(float)> onVolumeChanged;
    std::function<void(bool)> onMuteChanged;
    std::function<void(bool)> onSoloChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int index;
    juce::TextButton muteBtn{"M"}, soloBtn{"S"};
    CircularButton recBtn{ CircularButton::Rec };
    CircularButton playBtn{ CircularButton::Play };
    CircularButton ovdBtn{ CircularButton::Ovd };
    juce::Slider volumeSlider;
    VUMeter vuMeter;

    juce::AudioThumbnail* thumbnail = nullptr;
    float playheadProg = -1.0f;
    bool isRec = false, isPlay = false, isOvd = false;
    float timeSecs = 0.0f, lenSecs = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};

class FXPedalComponent : public juce::Component
{
public:
    FXPedalComponent(const juce::String& name, juce::Colour col,
                     const juce::String& l1, const juce::String& l2, const juce::String& l3);
    ~FXPedalComponent() override = default;

    void configureExtendedKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                                const juce::String& l4, const juce::String& l5, const juce::String& l6);
    void configureNineKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                            const juce::String& l4, const juce::String& l5, const juce::String& l6,
                            const juce::String& l7, const juce::String& l8, const juce::String& l9);
    void setKnobValueFormatter(int knobIndex, std::function<juce::String(double)> formatter);
    void setInfoText(const juce::String& mainText, const juce::String& subText = {});
    void setDragHintVisible(bool visible);
    juce::ToggleButton onBtn{"ON"};
    juce::Slider knob1, knob2, knob3, knob4, knob5, knob6, knob7, knob8, knob9;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragStart;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragMove;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragEnd;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.y > getHeight() - 30)
        {
            onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
            repaint();
        }
        else if (e.y < 34 && onDragStart)
        {
            onDragStart(*this, e);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (onDragMove)
            onDragMove(*this, e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (onDragEnd)
            onDragEnd(*this, e);
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String pedalName, k1, k2, k3, k4, k5, k6, k7, k8, k9;
    juce::Colour pedalCol;
    bool extendedKnobs = false;
    bool nineKnobs = false;
    bool dragHintVisible = false;
    juce::String infoMain, infoSub;
    std::function<juce::String(double)> knobValueFormatters[9];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPedalComponent)
};
