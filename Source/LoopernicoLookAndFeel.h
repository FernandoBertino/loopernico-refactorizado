#pragma once

#include <JuceHeader.h>

namespace LnfC {
    static const juce::Colour BG         { 0xff07080a };
    static const juce::Colour Panel      { 0xff0f1114 };
    static const juce::Colour PanelLight { 0xff1a1d22 };
    static const juce::Colour Border     { 0xff2b2f36 };
    static const juce::Colour Accent     { 0xff55f04a };
    static const juce::Colour TextHi     { 0xffffffff };
    static const juce::Colour TextDim    { 0xff8a8d93 };
    static const juce::Colour RecRed     { 0xffff2b24 };
    static const juce::Colour PlayGreen  { 0xff55f04a };
    static const juce::Colour OvdAmber   { 0xffff9a18 };
    static const juce::Colour KnobArc    { 0xffff9a18 };
    static const juce::Colour StepOn     { 0xffff342b };
    static const juce::Colour StepCur    { 0xffff756b };

    static inline juce::Colour trackColor(int index)
    {
        static const juce::Colour palette[12] = {
            juce::Colour(0xffdd2222),
            juce::Colour(0xff22cc44),
            juce::Colour(0xffff8800),
            juce::Colour(0xff2288ee),
            juce::Colour(0xffaa22dd),
            juce::Colour(0xff22ccbb),
            juce::Colour(0xffcccc22),
            juce::Colour(0xffdd2288),
            juce::Colour(0xff22aadd),
            juce::Colour(0xffdd6633),
            juce::Colour(0xff6644dd),
            juce::Colour(0xff33dd88),
        };
        return palette[juce::jlimit(0, 11, index)];
    }
}

class LoopernicoLAF : public juce::LookAndFeel_V4
{
public:
    LoopernicoLAF()
    {
        setColour(juce::ResizableWindow::backgroundColourId,      LnfC::BG);
        setColour(juce::TextButton::buttonColourId,               LnfC::PanelLight);
        setColour(juce::TextButton::buttonOnColourId,             LnfC::Accent.withAlpha(0.22f));
        setColour(juce::TextButton::textColourOffId,              LnfC::TextHi.withAlpha(0.85f));
        setColour(juce::TextButton::textColourOnId,               LnfC::Accent);
        setColour(juce::ComboBox::backgroundColourId,             LnfC::Panel);
        setColour(juce::ComboBox::textColourId,                   LnfC::TextHi);
        setColour(juce::ComboBox::outlineColourId,                LnfC::Border);
        setColour(juce::ComboBox::arrowColourId,                  LnfC::Accent);
        setColour(juce::PopupMenu::backgroundColourId,            LnfC::Panel);
        setColour(juce::PopupMenu::textColourId,                  LnfC::TextHi);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, LnfC::Accent.withAlpha(0.28f));
        setColour(juce::Label::textColourId,                      LnfC::TextHi);
        setColour(juce::Label::backgroundColourId,                juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderFillColourId,         LnfC::KnobArc);
        setColour(juce::Slider::thumbColourId,                    juce::Colours::white);
        setColour(juce::Slider::trackColourId,                    LnfC::Accent.withAlpha(0.4f));
        setColour(juce::Slider::backgroundColourId,               LnfC::Panel.darker(0.2f));
        setColour(juce::ScrollBar::thumbColourId,                 LnfC::Border.brighter(0.3f));
        setColour(juce::ToggleButton::textColourId,               LnfC::TextHi.withAlpha(0.85f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.0f);
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.46f;
        auto centre = bounds.getCentre();

        juce::Path bgArc;
        bgArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.62f));
        g.fillEllipse(centre.x - radius - 2.0f, centre.y - radius + 1.5f,
                      (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f);

        g.setColour(LnfC::Panel.darker(0.55f));
        g.strokePath(bgArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            float angle = startAngle + pos * (endAngle - startAngle);
            juce::Path valArc;
            valArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
            g.setColour(LnfC::KnobArc.withAlpha(0.95f));
            g.strokePath(valArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        float kr = radius * 0.68f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(centre.x - kr - 1.0f, centre.y - kr + 1.0f, (kr + 1.0f) * 2.0f, (kr + 1.0f) * 2.0f);

        juce::ColourGradient kGrad(
            juce::Colour(0xff70747d), centre.x - kr * 0.4f, centre.y - kr * 0.4f,
            juce::Colour(0xff0a0b0d), centre.x + kr * 0.3f, centre.y + kr * 0.4f, true);
        g.setGradientFill(kGrad);
        g.fillEllipse(centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);

        g.setColour(juce::Colour(0xff9a9ca2).withAlpha(0.72f));
        g.drawEllipse(centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f, 1.2f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawEllipse(centre.x - kr + 2.0f, centre.y - kr + 2.0f, kr * 2.0f - 4.0f, kr * 2.0f - 4.0f, 0.9f);

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.fillEllipse(centre.x - kr * 0.48f, centre.y - kr * 0.80f, kr * 0.96f, kr * 0.42f);

        float angle = startAngle + pos * (endAngle - startAngle);
        float dotR = kr * 0.11f;
        float dotD = kr * 0.62f;
        float dotX = centre.x + std::sin(angle) * dotD;
        float dotY = centre.y - std::cos(angle) * dotD;

        g.setColour(juce::Colours::white.withAlpha(0.20f));
        g.fillEllipse(dotX - dotR * 2.2f, dotY - dotR * 2.2f, dotR * 4.4f, dotR * 4.4f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bgCol, bool hl, bool dn) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        bool on = btn.getToggleState();
        auto col = on ? bgCol.brighter(0.12f) : dn ? bgCol.darker(0.08f)
                                                    : hl ? bgCol.brighter(0.06f) : bgCol;

        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillRoundedRectangle(b.translated(0.0f, 1.6f).expanded(1.0f), 4.0f);

        juce::ColourGradient bGrad(
            col.brighter(dn ? 0.02f : 0.18f), b.getX(), b.getY(),
            col.darker(dn ? 0.30f : 0.18f),   b.getX(), b.getBottom(), false);
        g.setGradientFill(bGrad);
        g.fillRoundedRectangle(b, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(dn ? 0.015f : 0.10f));
        g.fillRoundedRectangle(b.withHeight(3.0f).reduced(2.0f, 0.0f), 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.26f));
        g.drawRoundedRectangle(b.reduced(2.0f), 3.0f, 1.0f);

        auto txt = btn.getButtonText();
        if (txt.contains("CONFIRM") || txt.contains("REC / C"))
        {
            g.setColour(LnfC::RecRed.withAlpha(hl ? 1.0f : 0.80f));
            g.drawRoundedRectangle(b, 4.0f, 2.0f);
            if (hl || on)
            {
                g.setColour(LnfC::RecRed.withAlpha(0.12f));
                g.drawRoundedRectangle(b.expanded(1.5f), 5.0f, 2.0f);
            }
        }
        else
        {
            g.setColour(on ? LnfC::Accent.withAlpha(0.65f) : LnfC::Border.withAlpha(0.85f));
            g.drawRoundedRectangle(b, 4.0f, 1.0f);
        }

        if (txt.contains("PLAY"))
        {
            float cy = b.getCentreY();
            float tx = b.getX() + 11.0f;
            float ts = 6.5f;
            juce::Path tri;
            tri.addTriangle(tx, cy - ts, tx, cy + ts, tx + ts * 1.5f, cy);
            g.setColour(LnfC::PlayGreen.withAlpha(0.25f));
            g.fillPath(tri);
            g.setColour(LnfC::PlayGreen.withAlpha(dn ? 0.7f : 1.0f));
            g.fillPath(tri);
        }
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override
    {
        juce::Rectangle<float> b(0.0f, 0.0f, (float)w, (float)h);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(b.translated(0.0f, 1.0f), 3.0f);
        juce::ColourGradient boxGrad(
            juce::Colour(0xff191b20), b.getX(), b.getY(),
            juce::Colour(0xff07080a), b.getX(), b.getBottom(), false);
        g.setGradientFill(boxGrad);
        g.fillRoundedRectangle(b, 3.0f);
        g.setColour(LnfC::Border.withAlpha(0.95f));
        g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);
        auto ac = juce::Rectangle<float>((float)bx, (float)by, (float)bw, (float)bh).getCentre();
        float as = (float)bh * 0.28f;
        juce::Path arr;
        arr.addTriangle({ ac.x - as, ac.y - as * 0.5f }, { ac.x + as, ac.y - as * 0.5f }, { ac.x, ac.y + as * 0.5f });
        g.setColour(LnfC::TextDim.brighter(0.25f));
        g.fillPath(arr);
    }

    void positionComboBoxText(juce::ComboBox& cb, juce::Label& lbl) override
    {
        lbl.setBounds(4, 1, cb.getWidth() - 26, cb.getHeight() - 2);
        lbl.setFont(getComboBoxFont(cb));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn, bool, bool) override
    {
        auto b = btn.getLocalBounds().toFloat();
        bool on = btn.getToggleState();
        float ls = juce::jmin(b.getHeight() * 0.55f, 13.0f);
        float lx = b.getX() + 5.0f;
        float ly = b.getCentreY() - ls * 0.5f;

        if (on)
        {
            g.setColour(LnfC::PlayGreen.withAlpha(0.15f));
            g.fillEllipse(lx - 5.0f, ly - 5.0f, ls + 10.0f, ls + 10.0f);
            g.setColour(LnfC::PlayGreen.withAlpha(0.30f));
            g.fillEllipse(lx - 3.0f, ly - 3.0f, ls + 6.0f, ls + 6.0f);
            g.setColour(LnfC::PlayGreen);
        }
        else
        {
            g.setColour(juce::Colour(0xff1a3322));
        }

        g.fillEllipse(lx, ly, ls, ls);
        g.setColour(LnfC::Border);
        g.drawEllipse(lx, ly, ls, ls, 1.0f);

        g.setColour(on ? LnfC::TextHi : LnfC::TextDim);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText(btn.getButtonText(),
                   (int)(lx + ls + 5.0f), 0,
                   btn.getWidth() - (int)(ls + 12.0f), btn.getHeight(),
                   juce::Justification::centredLeft);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(juce::FontOptions(11.5f));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override
    {
        return juce::Font(juce::FontOptions(11.5f).withStyle("Bold"));
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        auto f = label.getFont();
        return juce::approximatelyEqual(f.getHeight(), 15.0f)
            ? juce::Font(juce::FontOptions(11.0f))
            : f;
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float, float, juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        bool horiz = (style == juce::Slider::LinearHorizontal);
        auto b = juce::Rectangle<int>(x, y, w, h).toFloat();

        float trackThick = 3.0f;
        juce::Rectangle<float> track;
        if (horiz) track = { b.getX(), b.getCentreY() - trackThick * 0.5f, b.getWidth(), trackThick };
        else       track = { b.getCentreX() - trackThick * 0.5f, b.getY(), trackThick, b.getHeight() };

        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.fillRoundedRectangle(track, trackThick * 0.5f);

        juce::Rectangle<float> filled;
        if (horiz) filled = { track.getX(), track.getY(), pos - (float)x, trackThick };
        else       filled = { track.getX(), pos, trackThick, track.getBottom() - pos };

        g.setColour(LnfC::OvdAmber.withAlpha(0.78f));
        g.fillRoundedRectangle(filled, trackThick * 0.5f);

        float thumbR = 6.0f;
        float tx = horiz ? pos : b.getCentreX();
        float ty = horiz ? b.getCentreY() : pos;

        g.setColour(LnfC::OvdAmber.withAlpha(0.20f));
        g.fillEllipse(tx - thumbR * 1.8f, ty - thumbR * 1.8f, thumbR * 3.6f, thumbR * 3.6f);
        g.setColour(LnfC::OvdAmber);
        g.fillEllipse(tx - thumbR, ty - thumbR, thumbR * 2.0f, thumbR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawEllipse(tx - thumbR, ty - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.0f);

        juce::ignoreUnused(slider);
    }
};

namespace LoopernicoUi
{
    static inline void drawScrew(juce::Graphics& g, float cx, float cy, float r)
    {
        g.setColour(juce::Colours::black.withAlpha(0.60f));
        g.fillEllipse(cx - r + 0.5f, cy - r + 1.2f, r * 2.0f, r * 2.0f);

        juce::ColourGradient grad(
            juce::Colour(0xff9898aa), cx - r * 0.35f, cy - r * 0.42f,
            juce::Colour(0xff242432), cx + r * 0.30f, cy + r * 0.40f, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        g.setColour(juce::Colour(0xff4c4c60));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);

        float sl = r * 0.64f;
        float sw = r * 0.20f;
        g.setColour(juce::Colours::black.withAlpha(0.82f));
        g.fillRect(cx - sl, cy - sw * 0.5f, sl * 2.0f, sw);
        g.fillRect(cx - sw * 0.5f, cy - sl, sw, sl * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawLine(cx - sl, cy - sw * 0.5f, cx + sl, cy - sw * 0.5f, 0.6f);
        g.drawLine(cx - sw * 0.5f, cy - sl, cx - sw * 0.5f, cy + sl, 0.6f);

        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.fillEllipse(cx - r * 0.44f, cy - r * 0.72f, r * 0.58f, r * 0.36f);
    }

    static inline void drawGlow(juce::Graphics& g, float cx, float cy, float r,
                                juce::Colour col, int layers = 3)
    {
        for (int i = layers; i >= 1; --i)
        {
            float expand = r * 0.55f * (float)i;
            float alpha = 0.10f / (float)i;
            g.setColour(col.withAlpha(alpha));
            g.fillEllipse(cx - r - expand, cy - r - expand,
                          (r + expand) * 2.0f, (r + expand) * 2.0f);
        }
    }

    static inline void drawBrushedMetalHeader(juce::Graphics& g, juce::Rectangle<float> b)
    {
        int h = (int)b.getHeight();
        for (int i = 0; i < h; ++i)
        {
            float alpha = (i % 3 == 0) ? 0.014f
                        : (i % 3 == 1) ? 0.005f
                                       : 0.002f;
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.drawHorizontalLine((int)(b.getY()) + i, b.getX(), b.getRight());
        }
        g.setColour(juce::Colours::white.withAlpha(0.03f));
        g.drawHorizontalLine((int)b.getBottom() - 1, b.getX(), b.getRight());
    }

    static inline void drawFineRackTexture(juce::Graphics& g, juce::Rectangle<float> b, float alphaScale = 1.0f)
    {
        const int top = (int)b.getY();
        const int bottom = (int)b.getBottom();
        for (int y = top; y < bottom; ++y)
        {
            const float a = ((y % 5) == 0 ? 0.020f : (y % 2) == 0 ? 0.007f : 0.003f) * alphaScale;
            g.setColour(juce::Colours::white.withAlpha(a));
            g.drawHorizontalLine(y, b.getX(), b.getRight());
        }

        g.setColour(juce::Colours::black.withAlpha(0.18f * alphaScale));
        for (int x = (int)b.getX(); x < (int)b.getRight(); x += 18)
            g.drawVerticalLine(x, b.getY(), b.getBottom());
    }

    static inline void drawPanel(juce::Graphics& g, juce::Rectangle<int> r,
                                 const juce::String& title, juce::Colour accent)
    {
        auto rf = r.toFloat();

        if (title == "RITMOS")
        {
            auto pedal = rf.reduced(16.0f, 14.0f);
            auto glass = juce::Rectangle<float>(pedal.getX(), pedal.getY(), pedal.getWidth(), pedal.getHeight() * 0.62f);
            auto screen = juce::Rectangle<float>(pedal.getX() + 20.0f, pedal.getY() + 96.0f, 150.0f, 154.0f);
            auto tapPad = juce::Rectangle<float>(pedal.getRight() - 114.0f, pedal.getY() + 116.0f, 96.0f, 96.0f);

            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.fillRoundedRectangle(pedal.translated(0.0f, 8.0f), 34.0f);

            juce::ColourGradient bodyGrad(
                juce::Colour(0xff38383b), pedal.getX(), pedal.getBottom(),
                juce::Colour(0xff191a1d), pedal.getRight(), pedal.getY(), true);
            g.setGradientFill(bodyGrad);
            g.fillRoundedRectangle(pedal, 34.0f);

            juce::ColourGradient glassGrad(
                juce::Colour(0xff242426), glass.getX(), glass.getBottom(),
                juce::Colour(0xff050608), glass.getRight(), glass.getY(), true);
            g.setGradientFill(glassGrad);
            g.fillRoundedRectangle(glass, 25.0f);

            g.setColour(juce::Colours::white.withAlpha(0.05f));
            juce::Path gloss;
            gloss.startNewSubPath(glass.getX() + 18.0f, glass.getBottom() - 16.0f);
            gloss.quadraticTo(glass.getX() + 72.0f, glass.getY() + 14.0f,
                              glass.getX() + 128.0f, glass.getY() + 16.0f);
            gloss.lineTo(glass.getX() + 74.0f, glass.getBottom() - 16.0f);
            gloss.closeSubPath();
            g.fillPath(gloss);

            g.setColour(juce::Colour(0xff0b0b10));
            g.fillRoundedRectangle(screen, 12.0f);
            g.setColour(juce::Colour(0xff5efc78).withAlpha(0.08f));
            g.fillRoundedRectangle(screen.reduced(3.0f), 10.0f);
            g.setColour(juce::Colour(0xff1f2228));
            g.drawRoundedRectangle(screen, 12.0f, 1.0f);

            g.setColour(juce::Colours::white.withAlpha(0.94f));
            g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle("Bold")));
            g.drawText("SWING",  (int)glass.getX() + 18,  (int)glass.getY() + 80, 64, 18, juce::Justification::centred);
            g.drawText("TEMPO",  (int)glass.getX() + 106, (int)glass.getY() + 80, 70, 18, juce::Justification::centred);
            g.drawText("VOLUME", (int)glass.getX() + 196, (int)glass.getY() + 80, 76, 18, juce::Justification::centred);

            g.setColour(juce::Colours::white.withAlpha(0.82f));
            g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
            g.drawText("DRUM SET", (int)screen.getX() + 12, (int)screen.getY() + 8, 90, 16, juce::Justification::centredLeft);
            g.setColour(juce::Colour(0xff9fffb0).withAlpha(0.88f));
            g.drawText("LIVE", (int)screen.getRight() - 42, (int)screen.getY() + 8, 28, 16, juce::Justification::centredRight);

            auto status = juce::Rectangle<float>(screen.getX() + 10.0f, screen.getY() + 32.0f, screen.getWidth() - 20.0f, 26.0f);
            g.setColour(juce::Colour(0xff0f1512));
            g.fillRoundedRectangle(status, 6.0f);
            g.setColour(juce::Colour(0xff57ff7b).withAlpha(0.18f));
            g.fillRoundedRectangle(status.reduced(1.0f), 5.0f);
            g.setColour(juce::Colours::white.withAlpha(0.90f));
            g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
            g.drawText("BEAT GRID", status.toNearestInt(), juce::Justification::centredLeft);

            auto stepBand = juce::Rectangle<float>(screen.getX() + 10.0f, screen.getY() + 64.0f, screen.getWidth() - 20.0f, 16.0f);
            float stepCellW = stepBand.getWidth() / 16.0f;
            for (int i = 0; i < 16; ++i)
            {
                auto cell = juce::Rectangle<float>(stepBand.getX() + i * stepCellW + 0.8f, stepBand.getY(), stepCellW - 1.6f, stepBand.getHeight());
                bool beatStart = (i % 4 == 0);
                g.setColour(beatStart ? juce::Colour(0xff5efc78).withAlpha(0.30f)
                                      : juce::Colour(0xff5efc78).withAlpha(0.16f));
                g.fillRoundedRectangle(cell, 2.0f);
            }

            g.setColour(juce::Colour(0xff1d1d1f));
            g.fillEllipse(tapPad);
            g.setColour(juce::Colours::black.withAlpha(0.45f));
            g.drawEllipse(tapPad, 1.3f);
            auto tapInner = tapPad.reduced(24.0f);
            g.setColour(juce::Colour(0xff2a2a2d));
            g.fillEllipse(tapInner);
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.drawEllipse(tapInner, 1.1f);

            g.setColour(juce::Colour(0xffdfc36a).withAlpha(0.85f));
            g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
            g.drawText("LOOPERNICO", r.getX(), r.getBottom() - 58, r.getWidth(), 18, juce::Justification::centred);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.setColour(juce::Colour(0xffdbc46e).withAlpha(0.66f));
            g.drawText("RHYTHM MACHINE", r.getX(), r.getBottom() - 40, r.getWidth(), 14, juce::Justification::centred);

            g.setColour(juce::Colour(0xffd4b15a).withAlpha(0.95f));
            g.fillEllipse(pedal.getX() + 24.0f, pedal.getBottom() - 42.0f, 20.0f, 20.0f);
            g.fillEllipse(pedal.getRight() - 44.0f, pedal.getBottom() - 42.0f, 20.0f, 20.0f);
            return;
        }

        if (title == "PARTES")
        {
            auto module = rf.reduced(10.0f, 8.0f);

            g.setColour(juce::Colours::black.withAlpha(0.24f));
            g.fillRoundedRectangle(module.translated(0.0f, 5.0f), 14.0f);

            juce::ColourGradient bodyGrad(
                juce::Colour(0xff17191d), module.getX(), module.getBottom(),
                juce::Colour(0xff0a0c0f), module.getRight(), module.getY(), true);
            g.setGradientFill(bodyGrad);
            g.fillRoundedRectangle(module, 14.0f);
            drawFineRackTexture(g, module.reduced(1.5f), 0.22f);

            auto header = juce::Rectangle<float>(module.getX() + 10.0f, module.getY() + 10.0f, module.getWidth() - 20.0f, 18.0f);
            g.setColour(juce::Colour(0xff050608).withAlpha(0.88f));
            g.fillRoundedRectangle(header, 8.0f);
            g.setColour(juce::Colour(0xff67ff86).withAlpha(0.12f));
            g.fillRoundedRectangle(header.reduced(1.0f), 7.0f);

            g.setColour(juce::Colours::white.withAlpha(0.92f));
            g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
            g.drawText("PART SELECT", header.toNearestInt(), juce::Justification::centredLeft);

            auto queueDot = juce::Rectangle<float>(header.getRight() - 16.0f, header.getY() + 5.0f, 7.0f, 7.0f);
            g.setColour(juce::Colour(0xff67ff86).withAlpha(0.90f));
            g.fillEllipse(queueDot);

            auto buttonBay = juce::Rectangle<float>(module.getX() + 10.0f, module.getY() + 34.0f, module.getWidth() - 20.0f, 48.0f);
            g.setColour(juce::Colours::black.withAlpha(0.44f));
            g.fillRoundedRectangle(buttonBay, 10.0f);
            g.setColour(juce::Colour(0xff5efc78).withAlpha(0.06f));
            g.fillRoundedRectangle(buttonBay.reduced(1.0f), 9.0f);
            g.setColour(juce::Colour(0xff25292f));
            g.drawRoundedRectangle(buttonBay, 10.0f, 1.0f);

            auto footer = juce::Rectangle<float>(module.getX() + 10.0f, module.getBottom() - 22.0f, module.getWidth() - 20.0f, 12.0f);
            g.setColour(juce::Colour(0xff0a0c0f).withAlpha(0.86f));
            g.fillRoundedRectangle(footer, 6.0f);
            g.setColour(juce::Colour(0xff31363d));
            g.drawRoundedRectangle(footer, 6.0f, 0.8f);

            drawScrew(g, module.getX() + 14.0f, module.getY() + 14.0f, 5.2f);
            drawScrew(g, module.getRight() - 14.0f, module.getY() + 14.0f, 5.2f);
            return;
        }

        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(rf.translated(0.0f, 2.0f).expanded(1.8f), 8.5f);

        juce::ColourGradient bodyGrad(
            juce::Colour(0xff181b1f), rf.getX(), rf.getY(),
            juce::Colour(0xff090a0c), rf.getX(), rf.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(rf.reduced(1.0f), 8.0f);
        drawFineRackTexture(g, rf.reduced(2.0f), 0.38f);

        juce::Rectangle<float> grain = rf.reduced(1.5f);
        grain.removeFromTop(28.0f);
        for (int i = 0; i < (int)grain.getHeight(); i += 5)
        {
            float alpha = (i % 10 == 0) ? 0.010f : 0.003f;
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.drawHorizontalLine((int)grain.getY() + i, grain.getX(), grain.getRight());
        }

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawRoundedRectangle(rf.reduced(3.0f), 4.5f, 1.0f);

        g.setColour(accent.withAlpha(0.04f));
        g.drawRoundedRectangle(rf.reduced(1.5f), 7.5f, 1.8f);

        g.setColour(LnfC::Border.brighter(0.10f).withAlpha(0.60f));
        g.drawRoundedRectangle(rf.reduced(0.5f), 8.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.drawRoundedRectangle(rf.reduced(2.0f), 7.0f, 0.8f);

        auto hdr = r.removeFromTop(28);
        auto hdrf = hdr.toFloat();

        juce::ColourGradient hGrad(
            juce::Colour(0xff14171b), hdrf.getX(), hdrf.getY(),
            juce::Colour(0xff090a0c), hdrf.getX(), hdrf.getBottom(), false);
        g.setGradientFill(hGrad);
        g.fillRoundedRectangle(hdrf.reduced(1.0f), 7.0f);

        drawBrushedMetalHeader(g, hdrf.reduced(1.0f));

        g.setColour(LnfC::Border.withAlpha(0.65f));
        g.drawHorizontalLine(hdr.getBottom(),
                             (float)hdr.getX() + 1.0f, (float)hdr.getRight() - 1.0f);

        drawScrew(g, (float)hdr.getX() + 12.0f, (float)hdr.getCentreY(), 5.5f);
        drawScrew(g, (float)hdr.getRight() - 12.0f, (float)hdr.getCentreY(), 5.5f);

        g.setColour(LnfC::TextHi.withAlpha(0.95f));
        g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
        g.drawText(title,
                   hdr.getX() + 26, hdr.getY() + 2,
                   hdr.getWidth() - 52, hdr.getHeight() - 4,
                   juce::Justification::centredLeft);
    }
}
