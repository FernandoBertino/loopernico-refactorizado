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
    void setMonoDisplay(bool mono) { if (monoDisplay != mono) { monoDisplay = mono; repaint(); } }
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
    bool monoDisplay = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};

class FXPedalComponent : public juce::Component
{
public:
    enum class JackSide { input, output };

    FXPedalComponent(const juce::String& name, juce::Colour col,
                     const juce::String& l1, const juce::String& l2, const juce::String& l3);
    ~FXPedalComponent() override = default;

    void configureExtendedKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                                const juce::String& l4, const juce::String& l5, const juce::String& l6);
    void configureNineKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                            const juce::String& l4, const juce::String& l5, const juce::String& l6,
                            const juce::String& l7, const juce::String& l8, const juce::String& l9);
    void configureTenBandGraphicEq();
    void setKnobValueFormatter(int knobIndex, std::function<juce::String(double)> formatter);
    void setInfoText(const juce::String& mainText, const juce::String& subText = {});
    void setDragHintVisible(bool visible);
    const juce::String& getDisplayName() const noexcept { return pedalName; }
    /** Centre of the physical side jack, expressed in this component's coordinates. */
    juce::Point<float> getJackCentre(JackSide side) const noexcept;
    juce::ToggleButton onBtn{"ON"};
    juce::Slider knob1, knob2, knob3, knob4, knob5, knob6, knob7, knob8, knob9;
    juce::Slider eqVolumeSlider, eqGainSlider;
    std::array<juce::Slider, 10> eqBandSliders;
    std::function<void()> onSecondaryFootswitch;
    std::function<void()> onSecondaryFootswitchRelease;
    std::function<void()> onTertiaryFootswitch;
    std::function<void()> onTap;
    bool isTertiaryFootswitchLatched() const noexcept { return tertiaryFootswitchLatched; }
    void setTertiaryFootswitchLatched(bool latched) noexcept
    {
        tertiaryFootswitchLatched = latched;
        repaint();
    }
    int getSunsetBrightMode() const noexcept { return sunsetBrightMode; }
    void setSunsetBrightMode(int mode)
    {
        const int next = juce::jlimit(0, 2, mode);
        if (sunsetBrightMode == next)
            return;
        sunsetBrightMode = next;
        if (onSunsetBrightChange)
            onSunsetBrightChange(next);
        repaint();
    }
    std::function<void(int)> onSunsetBrightChange;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragStart;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragMove;
    std::function<void(FXPedalComponent&, const juce::MouseEvent&)> onDragEnd;
    std::function<void(FXPedalComponent&)> onZoomRequest;
    std::function<void(FXPedalComponent&)> onResized;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (pedalName == "SUNSET")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth());
            const float bodyWidth = juce::jmin(body.getWidth(), bodyHeight);
            body = { body.getCentreX() - bodyWidth * 0.5f,
                     body.getCentreY() - bodyHeight * 0.5f,
                     bodyWidth, bodyHeight };
            const auto mouse = e.position;
            const auto inside = [mouse](juce::Point<float> centre, float radius)
            {
                const float dx = mouse.x - centre.x;
                const float dy = mouse.y - centre.y;
                return dx * dx + dy * dy <= radius * radius;
            };
            const float footY = body.getY() + body.getHeight() * 0.837f;
            if (inside({ body.getX() + body.getWidth() * 0.208f, footY }, 28.0f))
            {
                onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                repaint();
                return;
            }
            if (inside({ body.getX() + body.getWidth() * 0.805f, footY }, 28.0f))
            {
                tertiaryFootswitchLatched = !tertiaryFootswitchLatched;
                if (onTertiaryFootswitch)
                    onTertiaryFootswitch();
                repaint();
                return;
            }

            // The physical CONFIG and BRIGHT switches live on the rear panel.
            // Their slim hit areas sit on the rolled top lip in the virtual unit.
            if (mouse.y <= body.getY() + body.getHeight() * 0.075f)
            {
                if (mouse.x < body.getCentreX())
                    knob9.setValue((juce::roundToInt(knob9.getValue()) + 1) % 3,
                                   juce::sendNotificationSync);
                else
                    setSunsetBrightMode((sunsetBrightMode + 1) % 3);
                repaint();
                return;
            }
        }

        if (pedalName == "TREMOLO")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / (4.0f / 3.0f));
            body = { body.getX(), body.getCentreY() - bodyHeight * 0.5f,
                     body.getWidth(), bodyHeight };
            const auto mouse = e.position;
            const auto inside = [mouse](juce::Point<float> centre, float radius)
            {
                const float dx = mouse.x - centre.x;
                const float dy = mouse.y - centre.y;
                return dx * dx + dy * dy <= radius * radius;
            };
            const float footY = body.getY() + body.getHeight() * 0.835f;
            const auto bypass = juce::Point<float>(body.getX() + body.getWidth() * 0.147f, footY);
            const auto control = juce::Point<float>(body.getX() + body.getWidth() * 0.843f, footY);

            if (inside(bypass, 27.0f))
            {
                onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                repaint();
                return;
            }
            if (inside(control, 27.0f))
            {
                secondaryFootswitchDown = true;
                if (onSecondaryFootswitch)
                    onSecondaryFootswitch();
                repaint();
                return;
            }
        }

        if (pedalName == "HARMONIZER")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            constexpr float aspect = 1578.0f / 1240.0f;
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / aspect);
            const float bodyWidth = juce::jmin(body.getWidth(), bodyHeight * aspect);
            body = { body.getCentreX() - bodyWidth * 0.5f,
                     body.getCentreY() - bodyHeight * 0.5f,
                     bodyWidth, bodyHeight };
            const float footY = body.getY() + body.getHeight() * 0.850f;
            const float footXs[] = { 0.119f, 0.500f, 0.881f };
            for (int i = 0; i < 3; ++i)
            {
                const float dx = e.position.x - (body.getX() + body.getWidth() * footXs[i]);
                const float dy = e.position.y - footY;
                const float hitRadius = body.getWidth() * 0.090f;
                if (dx * dx + dy * dy <= hitRadius * hitRadius)
                {
                    if (i == 0)
                        onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                    else if (i == 1)
                    {
                        secondaryFootswitchDown = true;
                        if (onSecondaryFootswitch)
                            onSecondaryFootswitch();
                    }
                    else if (onTertiaryFootswitch)
                        onTertiaryFootswitch();
                    repaint();
                    return;
                }
            }
        }

        if (pedalName == "VST3 HOST")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / 1.20f);
            body = { body.getX(), body.getCentreY() - bodyHeight * 0.5f,
                     body.getWidth(), bodyHeight };
            const auto foot = juce::Point<float>(body.getCentreX(),
                                                  body.getY() + body.getHeight() * 0.90f);
            const float dx = e.position.x - foot.x;
            const float dy = e.position.y - foot.y;
            if (dx * dx + dy * dy <= 26.0f * 26.0f)
            {
                onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                repaint();
                return;
            }
        }

        if (pedalName == "RHYTHM BOX")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / 0.72f);
            const float bodyWidth = juce::jmin(body.getWidth(), bodyHeight * 0.72f);
            body = { body.getCentreX() - bodyWidth * 0.5f,
                     body.getCentreY() - bodyHeight * 0.5f,
                     bodyWidth, bodyHeight };

            const auto mouse = e.position;
            const auto isInsideCircle = [mouse](juce::Point<float> centre, float radius)
            {
                const float dx = mouse.x - centre.x;
                const float dy = mouse.y - centre.y;
                return dx * dx + dy * dy <= radius * radius;
            };

            const auto leftFoot = juce::Point<float>(body.getX() + body.getWidth() * 0.205f,
                                                      body.getY() + body.getHeight() * 0.805f);
            const auto middleFoot = juce::Point<float>(body.getX() + body.getWidth() * 0.500f,
                                                       body.getY() + body.getHeight() * 0.805f);
            const auto tap = juce::Point<float>(body.getX() + body.getWidth() * 0.795f,
                                                 body.getY() + body.getHeight() * 0.805f);
            const float switchRadius = body.getWidth() * 0.13f;

            if (isInsideCircle(leftFoot, switchRadius))
            {
                onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                repaint();
                return;
            }

            if (isInsideCircle(middleFoot, switchRadius))
            {
                secondaryFootswitchDown = true;
                if (onSecondaryFootswitch)
                    onSecondaryFootswitch();
                repaint();
                return;
            }

            if (isInsideCircle(tap, body.getWidth() * 0.13f))
            {
                if (onTap)
                    onTap();
                repaint();
                return;
            }
        }

        if (pedalName == "ROOM REVERB")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / 1.455f);
            body = { body.getX(), body.getCentreY() - bodyHeight * 0.5f,
                     body.getWidth(), bodyHeight };
            const auto mouse = e.position;
            const float footY = body.getY() + body.getHeight() * 0.830f;
            const float xs[] = { 0.118f, 0.500f, 0.878f };
            for (int i = 0; i < 3; ++i)
            {
                const float dx = mouse.x - (body.getX() + body.getWidth() * xs[i]);
                const float dy = mouse.y - footY;
                if (dx * dx + dy * dy <= 24.0f * 24.0f)
                {
                    if (i == 0)
                        onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                    else if (i == 1 && onSecondaryFootswitch)
                        onSecondaryFootswitch();
                    else if (i == 2)
                    {
                        tertiaryFootswitchLatched = !tertiaryFootswitchLatched;
                        if (onTertiaryFootswitch)
                            onTertiaryFootswitch();
                    }
                    repaint();
                    return;
                }
            }
        }

        if (pedalName == "TAPE DELAY")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / 1.5f);
            body = { body.getX(), body.getCentreY() - bodyHeight * 0.5f,
                     body.getWidth(), bodyHeight };
            const auto mouse = e.position;
            const float footY = body.getY() + body.getHeight() * 0.842f;
            const float xs[] = { 0.115f, 0.500f, 0.880f };
            for (int i = 0; i < 3; ++i)
            {
                const float dx = mouse.x - (body.getX() + body.getWidth() * xs[i]);
                const float dy = mouse.y - footY;
                if (dx * dx + dy * dy <= 25.0f * 25.0f)
                {
                    if (i == 0)
                        onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                    else if (i == 1)
                    {
                        secondaryFootswitchDown = true;
                        if (onSecondaryFootswitch)
                            onSecondaryFootswitch();
                    }
                    else if (i == 2 && onTap)
                        onTap();
                    repaint();
                    return;
                }
            }
        }

        if (pedalName == "CHORUS")
        {
            auto body = getLocalBounds().toFloat().reduced(3.0f);
            constexpr float aspect = 3.38f / 5.25f;
            const float bodyHeight = juce::jmin(body.getHeight(), body.getWidth() / aspect);
            const float bodyWidth = juce::jmin(body.getWidth(), bodyHeight * aspect);
            body = { body.getCentreX() - bodyWidth * 0.5f,
                     body.getCentreY() - bodyHeight * 0.5f,
                     bodyWidth, bodyHeight };
            const auto foot = juce::Point<float>(body.getCentreX(),
                                                  body.getY() + body.getHeight() * 0.722f);
            const float dx = e.position.x - foot.x;
            const float dy = e.position.y - foot.y;
            const float hitRadius = juce::jmax(16.0f, body.getWidth() * 0.13f);
            if (dx * dx + dy * dy <= hitRadius * hitRadius)
            {
                onBtn.setToggleState(!onBtn.getToggleState(), juce::sendNotification);
                repaint();
                return;
            }
        }

        const bool customWideFootswitch = ((pedalName == "FUZZ" || pedalName == "COMPRESSOR"
                                              || pedalName == "SUSTAIN CHORDS" || pedalName == "OUTPUT EQ")
                                               && e.y > juce::roundToInt(getHeight() * 0.66f))
                                          || (pedalName.containsIgnoreCase("NOISE GATE")
                                               && e.y > juce::roundToInt(getHeight() * 0.55f));
        if (e.y > getHeight() - 30 || customWideFootswitch)
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

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onZoomRequest)
            onZoomRequest(*this);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (secondaryFootswitchDown)
        {
            secondaryFootswitchDown = false;
            if (onSecondaryFootswitchRelease)
                onSecondaryFootswitchRelease();
            repaint();
            return;
        }
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
    bool tenBandGraphicEq = false;
    bool dragHintVisible = false;
    bool secondaryFootswitchDown = false;
    bool tertiaryFootswitchLatched = false;
    int sunsetBrightMode = 1;
    juce::String infoMain, infoSub;
    std::function<juce::String(double)> knobValueFormatters[9];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPedalComponent)
};

/** Paints the hardware that belongs behind a row of pedals. Call this from the
    pedalboard/container paint() before painting its child pedals. */
class FXPedalboardWiring
{
public:
    struct CableStyle
    {
        juce::Colour sheath { 0xff17181b };
        float thickness = 7.0f;
        float sag = 18.0f;
    };

    static void drawCable(juce::Graphics&, const juce::Component& coordinateSpace,
                          const FXPedalComponent& from, const FXPedalComponent& to,
                          CableStyle style = {});
    static void drawChain(juce::Graphics&, const juce::Component& coordinateSpace,
                          const juce::Array<FXPedalComponent*>& pedals,
                          CableStyle style = {});
};
