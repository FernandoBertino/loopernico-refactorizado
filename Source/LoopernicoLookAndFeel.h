#pragma once

#include <JuceHeader.h>

namespace LnfC {
    static const juce::Colour BG         { 0xff07080a };
    static const juce::Colour Panel      { 0xff0f1114 };
    static const juce::Colour PanelLight { 0xff1a1d22 };
    static const juce::Colour Border     { 0xff2b2f36 };
    static const juce::Colour Accent     { 0xff55f04a };
    static const juce::Colour TextHi     { 0xffffffff };
    static const juce::Colour TextDim    { 0xffa4a7ad };
    static const juce::Colour RecRed     { 0xffff2b24 };
    static const juce::Colour PlayGreen  { 0xff55f04a };
    static const juce::Colour OvdAmber   { 0xffff9a18 };
    static const juce::Colour KnobArc    { 0xffff9a18 };
    static const juce::Colour StepOn     { 0xffff342b };
    static const juce::Colour StepCur    { 0xffff756b };
    static const juce::Colour MetalHi    { 0xff7b8087 };
    static const juce::Colour MetalLo    { 0xff15171a };
    static const juce::Colour Recess     { 0xff030405 };

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
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.0f);
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.46f;
        auto centre = bounds.getCentre();

        if (slider.getComponentID() == "smallCloneDepthSwitch")
        {
            auto slot = bounds.reduced(bounds.getWidth() * 0.28f, bounds.getHeight() * 0.08f);
            g.setColour(juce::Colours::black.withAlpha(0.72f));
            g.fillRoundedRectangle(slot.translated(1.0f, 1.5f), 2.5f);
            juce::ColourGradient slotMetal(juce::Colour(0xff3f3e3b), slot.getX(), slot.getY(),
                                            juce::Colour(0xff080808), slot.getRight(), slot.getBottom(), true);
            g.setGradientFill(slotMetal);
            g.fillRoundedRectangle(slot, 2.5f);
            const float capH = slot.getHeight() * 0.48f;
            const float capY = pos >= 0.5f ? slot.getY() + 1.5f : slot.getBottom() - capH - 1.5f;
            auto cap = juce::Rectangle<float>(slot.getX() - 2.0f, capY, slot.getWidth() + 4.0f, capH);
            juce::ColourGradient plastic(juce::Colour(0xff4e4e4b), cap.getX(), cap.getY(),
                                          juce::Colour(0xff090909), cap.getRight(), cap.getBottom(), true);
            g.setGradientFill(plastic);
            g.fillRoundedRectangle(cap, 2.0f);
            for (int i = 1; i < 4; ++i)
            {
                const float yy = cap.getY() + cap.getHeight() * (float) i / 4.0f;
                g.setColour(juce::Colours::white.withAlpha(0.12f));
                g.drawHorizontalLine(juce::roundToInt(yy), cap.getX() + 2.0f, cap.getRight() - 2.0f);
            }
            return;
        }

        // Broad drop shadow from the physical height of the knob. The darker,
        // tighter ellipses in each knob style remain as contact occlusion.
        const float shadowOffsetX = juce::jlimit(0.8f, 3.2f, radius * 0.10f);
        const float shadowOffsetY = juce::jlimit(1.2f, 4.2f, radius * 0.17f);
        const float shadowRadius = radius * 1.10f;
        const auto shadowCentre = centre.translated(shadowOffsetX, shadowOffsetY);
        juce::ColourGradient knobShadow(juce::Colours::black.withAlpha(0.31f),
                                        shadowCentre.x, shadowCentre.y,
                                        juce::Colours::transparentBlack,
                                        shadowCentre.x + shadowRadius, shadowCentre.y, true);
        knobShadow.addColour(0.66, juce::Colours::black.withAlpha(0.17f));
        g.setGradientFill(knobShadow);
        g.fillEllipse(shadowCentre.x - shadowRadius, shadowCentre.y - shadowRadius,
                      shadowRadius * 2.0f, shadowRadius * 2.0f);

        if (slider.getComponentID() == "smallCloneRateKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.96f;
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.fillEllipse(centre.x - r - 1.5f, centre.y - r + 2.2f,
                          r * 2.0f + 3.0f, r * 2.0f + 3.0f);
            for (int i = 0; i < 36; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.86f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.12f : 0.05f));
                g.drawLine({ p1, p2 }, 0.75f);
            }
            juce::ColourGradient cap(juce::Colour(0xff555451), centre.x - r * 0.55f, centre.y - r * 0.65f,
                                      juce::Colour(0xff070707), centre.x + r * 0.50f, centre.y + r * 0.58f, true);
            cap.addColour(0.44, juce::Colour(0xff242422));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - r * 0.85f, centre.y - r * 0.85f, r * 1.70f, r * 1.70f);
            g.setColour(juce::Colours::white.withAlpha(0.13f));
            g.fillEllipse(centre.x - r * 0.48f, centre.y - r * 0.60f, r * 0.72f, r * 0.21f);
            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.18f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.68f);
            g.setColour(juce::Colours::black.withAlpha(0.76f));
            g.drawLine({ markerStart.translated(0.9f, 1.0f), markerEnd.translated(0.9f, 1.0f) }, 3.0f);
            g.setColour(juce::Colour(0xfff1f0ea));
            g.drawLine({ markerStart, markerEnd }, 2.1f);
            return;
        }

        if (slider.getComponentID() == "eventideH90Knob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.96f;
            const bool mainEncoder = juce::jmin(w, h) >= 50;
            if (mainEncoder)
            {
                g.setColour(juce::Colour(0xffff5c5c).withAlpha(0.24f));
                g.fillEllipse(centre.x - r - 4.0f, centre.y - r - 4.0f,
                              (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
                g.setColour(juce::Colour(0xffff5c5c).withAlpha(0.88f));
                g.drawEllipse(centre.x - r - 2.0f, centre.y - r - 2.0f,
                              (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f, 2.0f);
            }
            g.setColour(juce::Colours::black.withAlpha(0.46f));
            g.fillEllipse(centre.x - r - 1.4f, centre.y - r + 2.0f,
                          r * 2.0f + 2.8f, r * 2.0f + 2.8f);
            for (int i = 0; i < 40; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.86f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.13f : 0.045f));
                g.drawLine({ p1, p2 }, 0.75f);
            }
            juce::ColourGradient cap(juce::Colour(0xff56585c), centre.x - r * 0.55f, centre.y - r * 0.65f,
                                      juce::Colour(0xff050607), centre.x + r * 0.48f, centre.y + r * 0.58f, true);
            cap.addColour(0.42, juce::Colour(0xff222428));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - r * 0.86f, centre.y - r * 0.86f, r * 1.72f, r * 1.72f);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.fillEllipse(centre.x - r * 0.46f, centre.y - r * 0.62f, r * 0.76f, r * 0.23f);
            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.20f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.70f);
            g.setColour(juce::Colours::black.withAlpha(0.80f));
            g.drawLine({ markerStart.translated(0.9f, 1.1f), markerEnd.translated(0.9f, 1.1f) }, 2.8f);
            g.setColour(juce::Colour(0xffeeeeea));
            g.drawLine({ markerStart, markerEnd }, 1.7f);
            return;
        }

        if (slider.getComponentID() == "timeLineKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.95f;
            g.setColour(juce::Colours::black.withAlpha(0.68f));
            g.fillEllipse(centre.x - r - 1.6f, centre.y - r + 2.2f,
                          r * 2.0f + 3.2f, r * 2.0f + 3.2f);
            for (int i = 0; i < 34; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 34.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.84f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.13f : 0.055f));
                g.drawLine({ p1, p2 }, 0.75f);
            }
            juce::ColourGradient cap(juce::Colour(0xff65676a), centre.x - r * 0.55f, centre.y - r * 0.68f,
                                      juce::Colour(0xff050607), centre.x + r * 0.52f, centre.y + r * 0.62f, true);
            cap.addColour(0.43, juce::Colour(0xff25272a));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - r * 0.84f, centre.y - r * 0.84f, r * 1.68f, r * 1.68f);
            g.setColour(juce::Colours::white.withAlpha(0.14f));
            g.fillEllipse(centre.x - r * 0.48f, centre.y - r * 0.60f, r * 0.72f, r * 0.21f);
            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.18f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.71f);
            g.setColour(juce::Colours::black.withAlpha(0.78f));
            g.drawLine({ markerStart.translated(0.9f, 1.0f), markerEnd.translated(0.9f, 1.0f) }, 3.0f);
            g.setColour(juce::Colour(0xfff1f1ed));
            g.drawLine({ markerStart, markerEnd }, 2.1f);
            return;
        }

        if (slider.getComponentID() == "bigSkyKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.94f;
            g.setColour(juce::Colours::black.withAlpha(0.62f));
            g.fillEllipse(centre.x - r - 1.5f, centre.y - r + 2.0f,
                          r * 2.0f + 3.0f, r * 2.0f + 3.0f);
            for (int i = 0; i < 32; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float)i / 32.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.86f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour((i % 2 == 0 ? juce::Colours::white : juce::Colours::black)
                                .withAlpha(i % 2 == 0 ? 0.10f : 0.72f));
                g.drawLine({ p1, p2 }, 0.8f);
            }
            juce::ColourGradient cap(juce::Colour(0xff55575a), centre.x - r * 0.52f, centre.y - r * 0.65f,
                                      juce::Colour(0xff060708), centre.x + r * 0.48f, centre.y + r * 0.58f, true);
            cap.addColour(0.44, juce::Colour(0xff242629));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - r * 0.85f, centre.y - r * 0.85f, r * 1.70f, r * 1.70f);
            g.setColour(juce::Colours::white.withAlpha(0.13f));
            g.fillEllipse(centre.x - r * 0.48f, centre.y - r * 0.60f, r * 0.72f, r * 0.22f);
            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.20f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.70f);
            g.setColour(juce::Colours::black.withAlpha(0.72f));
            g.drawLine({ markerStart.translated(0.9f, 1.0f), markerEnd.translated(0.9f, 1.0f) }, 2.8f);
            g.setColour(juce::Colour(0xffe8e8e4));
            g.drawLine({ markerStart, markerEnd }, 2.0f);
            return;
        }

        if (slider.getComponentID() == "beatBuddyKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.92f;
            const auto gold = juce::Colour(0xffd6a144);

            g.setColour(juce::Colours::black.withAlpha(0.60f));
            g.fillEllipse(centre.x - r - 2.0f, centre.y - r + 2.8f,
                          (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f);
            juce::ColourGradient metal(juce::Colour(0xffffd879), centre.x - r * 0.58f, centre.y - r * 0.68f,
                                       juce::Colour(0xff67400b), centre.x + r * 0.60f, centre.y + r * 0.64f, true);
            metal.addColour(0.48, gold);
            g.setGradientFill(metal);
            g.fillEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::black.withAlpha(0.68f));
            g.fillEllipse(centre.x - r * 0.79f, centre.y - r * 0.79f,
                          r * 1.58f, r * 1.58f);

            for (int i = 0; i < 30; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 30.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.68f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.79f);
                g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.18f : 0.08f));
                g.drawLine({ p1, p2 }, 0.9f);
            }

            juce::ColourGradient cap(juce::Colour(0xff505152), centre.x - r * 0.45f, centre.y - r * 0.55f,
                                     juce::Colour(0xff08090a), centre.x + r * 0.42f, centre.y + r * 0.50f, true);
            cap.addColour(0.46, juce::Colour(0xff252627));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - r * 0.66f, centre.y - r * 0.66f,
                          r * 1.32f, r * 1.32f);
            g.setColour(juce::Colours::white.withAlpha(0.13f));
            g.fillEllipse(centre.x - r * 0.38f, centre.y - r * 0.50f,
                          r * 0.60f, r * 0.18f);

            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.20f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.58f);
            g.setColour(juce::Colours::black.withAlpha(0.72f));
            g.drawLine({ markerStart.translated(1.0f, 1.0f), markerEnd.translated(1.0f, 1.0f) }, 3.0f);
            g.setColour(juce::Colour(0xffffe6ae));
            g.drawLine({ markerStart, markerEnd }, 2.1f);
            return;
        }

        if (slider.getComponentID() == "bossNoiseKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.88f;

            g.setColour(juce::Colours::black.withAlpha(0.30f));
            g.fillEllipse(centre.x - r - 1.8f, centre.y - r + 2.3f,
                          r * 2.0f + 3.6f, r * 2.0f + 3.6f);
            g.setColour(juce::Colour(0xff242424));
            g.fillEllipse(centre.x - r - 1.2f, centre.y - r - 1.2f,
                          r * 2.0f + 2.4f, r * 2.0f + 2.4f);

            for (int i = 0; i < 24; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 24.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.82f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.22f : 0.10f));
                g.drawLine({ p1, p2 }, 0.8f);
            }

            juce::ColourGradient ivory(juce::Colour(0xffffffff), centre.x - r * 0.50f, centre.y - r * 0.62f,
                                       juce::Colour(0xffbbb8ae), centre.x + r * 0.50f, centre.y + r * 0.58f, true);
            ivory.addColour(0.46, juce::Colour(0xffeeeae0));
            g.setGradientFill(ivory);
            g.fillEllipse(centre.x - r * 0.72f, centre.y - r * 0.72f,
                          r * 1.44f, r * 1.44f);
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.drawEllipse(centre.x - r * 0.72f, centre.y - r * 0.72f,
                          r * 1.44f, r * 1.44f, 0.9f);

            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.16f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.62f);
            g.setColour(juce::Colour(0xff181818));
            g.drawLine({ markerStart, markerEnd }, 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.38f));
            g.fillEllipse(centre.x - r * 0.42f, centre.y - r * 0.54f,
                          r * 0.65f, r * 0.18f);
            return;
        }

        if (slider.getComponentID() == "dedaloJetKnob")
        {
            const float angle = startAngle + pos * (endAngle - startAngle);
            const float r = radius * 0.90f;

            g.setColour(juce::Colours::black.withAlpha(0.30f));
            g.fillEllipse(centre.x - r - 1.5f, centre.y - r + 2.8f,
                          r * 2.0f + 3.0f, r * 2.0f + 3.0f);
            g.setColour(juce::Colour(0xffd2cdbf));
            g.fillEllipse(centre.x - r - 0.8f, centre.y - r - 0.8f,
                          r * 2.0f + 1.6f, r * 2.0f + 1.6f);

            juce::ColourGradient ivory(juce::Colour(0xfffffff5), centre.x - r * 0.55f, centre.y - r * 0.72f,
                                        juce::Colour(0xffd8d2c4), centre.x + r * 0.48f, centre.y + r * 0.62f, true);
            ivory.addColour(0.48, juce::Colour(0xfff3f0e7));
            g.setGradientFill(ivory);
            g.fillEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.70f));
            g.drawEllipse(centre.x - r + 0.7f, centre.y - r + 0.7f,
                          r * 2.0f - 1.4f, r * 2.0f - 1.4f, 1.0f);
            g.setColour(juce::Colour(0xff8e8a80).withAlpha(0.48f));
            g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.9f);

            juce::Path raisedGrip;
            raisedGrip.addRoundedRectangle(centre.x - r * 0.23f, centre.y - r * 0.78f,
                                           r * 0.46f, r * 1.56f, r * 0.16f);
            raisedGrip.applyTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));
            g.setColour(juce::Colours::black.withAlpha(0.10f));
            auto gripShadow = raisedGrip;
            gripShadow.applyTransform(juce::AffineTransform::translation(1.0f, 1.3f));
            g.fillPath(gripShadow);
            juce::ColourGradient grip(juce::Colour(0xffffffff), centre.x - r * 0.20f, centre.y - r * 0.65f,
                                      juce::Colour(0xffddd8cb), centre.x + r * 0.20f, centre.y + r * 0.62f, true);
            g.setGradientFill(grip);
            g.fillPath(raisedGrip);
            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.strokePath(raisedGrip, juce::PathStrokeType(0.8f));

            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.34f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.78f);
            g.setColour(juce::Colours::black.withAlpha(0.82f));
            g.drawLine({ markerStart, markerEnd }, juce::jmax(1.4f, r * 0.09f));
            g.setColour(juce::Colours::white.withAlpha(0.28f));
            g.fillEllipse(centre.x - r * 0.50f, centre.y - r * 0.66f,
                          r * 0.78f, r * 0.24f);
            return;
        }

        if (slider.getComponentID() == "sunsetToggle")
        {
            const float travel = radius * 0.72f;
            const float leverY = centre.y + juce::jmap(pos, -travel, travel);
            const float baseR = radius * 0.62f;
            g.setColour(juce::Colours::black.withAlpha(0.50f));
            g.fillEllipse(centre.x - baseR - 1.0f, centre.y - baseR + 1.5f,
                          baseR * 2.0f + 2.0f, baseR * 2.0f + 2.0f);
            juce::ColourGradient metal(juce::Colour(0xfff0eee8), centre.x - baseR, centre.y - baseR,
                                       juce::Colour(0xff565653), centre.x + baseR, centre.y + baseR, true);
            g.setGradientFill(metal);
            g.fillEllipse(centre.x - baseR, centre.y - baseR, baseR * 2.0f, baseR * 2.0f);
            g.setColour(juce::Colour(0xff171717));
            g.fillEllipse(centre.x - baseR * 0.48f, centre.y - baseR * 0.48f,
                          baseR * 0.96f, baseR * 0.96f);
            g.setColour(juce::Colour(0xffc7c4bb));
            g.drawLine(centre.x, centre.y, centre.x, leverY, juce::jmax(2.0f, radius * 0.22f));
            g.setColour(juce::Colour(0xff2b211f));
            g.fillEllipse(centre.x - radius * 0.25f, leverY - radius * 0.25f,
                          radius * 0.50f, radius * 0.50f);
            g.setColour(juce::Colours::white.withAlpha(0.34f));
            g.fillEllipse(centre.x - radius * 0.15f, leverY - radius * 0.18f,
                          radius * 0.22f, radius * 0.13f);
            return;
        }

        if (slider.getComponentID() == "sunsetKnob")
        {
            const float r = radius * 0.92f;
            const float angle = startAngle + pos * (endAngle - startAngle);
            g.setColour(juce::Colours::black.withAlpha(0.48f));
            g.fillEllipse(centre.x - r - 1.5f, centre.y - r + 2.5f,
                          r * 2.0f + 3.0f, r * 2.0f + 3.0f);
            g.setColour(juce::Colour(0xff090909));
            g.fillEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
            for (int i = 0; i < 20; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float)i / 20.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.82f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * r;
                g.setColour(juce::Colours::white.withAlpha(0.10f));
                g.drawLine({ p1, p2 }, 1.0f);
            }
            const float capR = r * 0.67f;
            juce::ColourGradient cap(juce::Colour(0xfff2f1ed), centre.x - capR * 0.55f, centre.y - capR * 0.65f,
                                     juce::Colour(0xff5d5d5b), centre.x + capR * 0.60f, centre.y + capR * 0.68f, true);
            cap.addColour(0.48, juce::Colour(0xffc8c8c5));
            g.setGradientFill(cap);
            g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.30f));
            g.drawEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f, 0.9f);
            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.22f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.78f);
            g.setColour(juce::Colours::white.withAlpha(0.96f));
            g.drawLine({ markerStart, markerEnd }, 2.2f);
            return;
        }

        if (slider.getComponentID() == "pampaFuzzKnob")
        {
            const float r = radius * 0.84f;
            const float angle = startAngle + pos * (endAngle - startAngle);
            g.setColour(juce::Colours::black.withAlpha(0.40f));
            g.fillEllipse(centre.x - r - 1.5f, centre.y - r + 2.2f, r * 2.0f + 3.0f, r * 2.0f + 3.0f);
            g.setColour(juce::Colour(0xff050505));
            g.fillEllipse(centre.x - r - 1.0f, centre.y - r - 1.0f, r * 2.0f + 2.0f, r * 2.0f + 2.0f);

            for (int i = 0; i < 36; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
                const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 0.87f);
                const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (r * 1.05f);
                g.setColour((i % 2 == 0 ? juce::Colours::white : juce::Colours::black)
                                .withAlpha(i % 2 == 0 ? 0.13f : 0.72f));
                g.drawLine({ p1, p2 }, 1.25f);
            }

            juce::ColourGradient vintageBlack(juce::Colour(0xff55534d), centre.x - r * 0.48f, centre.y - r * 0.58f,
                                               juce::Colour(0xff090909), centre.x + r * 0.42f, centre.y + r * 0.52f, true);
            vintageBlack.addColour(0.45, juce::Colour(0xff292927));
            g.setGradientFill(vintageBlack);
            g.fillEllipse(centre.x - r * 0.86f, centre.y - r * 0.86f, r * 1.72f, r * 1.72f);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawEllipse(centre.x - r * 0.84f, centre.y - r * 0.84f, r * 1.68f, r * 1.68f, 1.0f);

            const auto markerStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.18f);
            const auto markerEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (r * 0.69f);
            g.setColour(juce::Colours::black.withAlpha(0.70f));
            g.drawLine({ markerStart.translated(1.0f, 1.2f), markerEnd.translated(1.0f, 1.2f) }, 3.0f);
            g.setColour(juce::Colour(0xffe8d8ad));
            g.drawLine({ markerStart, markerEnd }, 2.1f);
            g.setColour(juce::Colours::white.withAlpha(0.16f));
            g.fillEllipse(centre.x - r * 0.44f, centre.y - r * 0.68f, r * 0.68f, r * 0.24f);
            return;
        }

        juce::Path bgArc;
        bgArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillEllipse(centre.x - radius - 2.5f, centre.y - radius - 0.5f,
                      (radius + 2.5f) * 2.0f, (radius + 2.5f) * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.88f));
        g.fillEllipse(centre.x - radius - 0.8f, centre.y - radius - 0.8f,
                      (radius + 0.8f) * 2.0f, (radius + 0.8f) * 2.0f);

        g.setColour(LnfC::Panel.darker(0.55f));
        g.strokePath(bgArc, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        for (int tick = 0; tick <= 10; ++tick)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) tick / 10.0f;
            const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (radius + 1.5f);
            const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (radius + (tick % 5 == 0 ? 5.0f : 3.5f));
            g.setColour(juce::Colours::white.withAlpha(tick % 5 == 0 ? 0.30f : 0.13f));
            g.drawLine({ p1, p2 }, tick % 5 == 0 ? 1.0f : 0.7f);
        }

        if (pos > 0.001f)
        {
            float angle = startAngle + pos * (endAngle - startAngle);
            juce::Path valArc;
            valArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
            g.setColour(LnfC::KnobArc.withAlpha(0.09f));
            g.strokePath(valArc, juce::PathStrokeType(5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(LnfC::KnobArc.withAlpha(0.82f));
            g.strokePath(valArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        float kr = radius * 0.68f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(centre.x - kr - 1.0f, centre.y - kr + 1.0f, (kr + 1.0f) * 2.0f, (kr + 1.0f) * 2.0f);

        juce::ColourGradient kGrad(
            LnfC::MetalHi, centre.x - kr * 0.55f, centre.y - kr * 0.65f,
            LnfC::MetalLo, centre.x + kr * 0.45f, centre.y + kr * 0.55f, true);
        kGrad.addColour(0.42, juce::Colour(0xff42464b));
        kGrad.addColour(0.72, juce::Colour(0xff202226));
        g.setGradientFill(kGrad);
        g.fillEllipse(centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);

        g.setColour(juce::Colour(0xff9a9ca2).withAlpha(0.72f));
        g.drawEllipse(centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f, 1.2f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawEllipse(centre.x - kr + 2.0f, centre.y - kr + 2.0f, kr * 2.0f - 4.0f, kr * 2.0f - 4.0f, 0.9f);

        // Fine radial knurling gives the control a machined edge at any scale.
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        for (int i = 0; i < 24; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float)i / 24.0f;
            const auto p1 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (kr * 0.84f);
            const auto p2 = centre + juce::Point<float>(std::sin(a), -std::cos(a)) * (kr * 0.96f);
            g.drawLine({ p1, p2 }, 0.65f);
        }

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.fillEllipse(centre.x - kr * 0.48f, centre.y - kr * 0.80f, kr * 0.96f, kr * 0.42f);

        float angle = startAngle + pos * (endAngle - startAngle);
        float dotR = kr * 0.075f;
        float dotD = kr * 0.64f;
        float dotX = centre.x + std::sin(angle) * dotD;
        float dotY = centre.y - std::cos(angle) * dotD;

        const auto lineStart = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (kr * 0.22f);
        const auto lineEnd = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (kr * 0.72f);
        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.drawLine({ lineStart.translated(0.8f, 1.0f), lineEnd.translated(0.8f, 1.0f) }, 2.4f);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.drawLine({ lineStart, lineEnd }, 1.5f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bgCol, bool hl, bool dn) override
    {
        if (btn.getComponentID() == "headerConsoleButton")
        {
            drawHeaderConsoleButton(g, btn, hl, dn);
            return;
        }

        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        bool on = btn.getToggleState();
        auto col = on ? bgCol.brighter(0.12f) : dn ? bgCol.darker(0.08f)
                                                    : hl ? bgCol.brighter(0.06f) : bgCol;

        if (on)
        {
            const auto glow = btn.findColour(juce::TextButton::buttonOnColourId).withMultipliedAlpha(0.65f);
            g.setColour(glow.withAlpha(0.08f));
            g.fillRoundedRectangle(b.expanded(5.0f, 4.0f), 7.0f);
            g.setColour(glow.withAlpha(0.16f));
            g.fillRoundedRectangle(b.expanded(2.0f, 1.5f), 5.0f);
        }

        g.setColour(juce::Colours::black.withAlpha(0.78f));
        g.fillRoundedRectangle(b.translated(0.0f, 2.0f).expanded(1.0f), 4.0f);
        g.setColour(LnfC::Recess);
        g.fillRoundedRectangle(b.expanded(1.0f), 4.5f);

        juce::ColourGradient bGrad(
            col.brighter(dn ? 0.02f : 0.18f), b.getX(), b.getY(),
            col.darker(dn ? 0.30f : 0.18f),   b.getX(), b.getBottom(), false);
        g.setGradientFill(bGrad);
        g.fillRoundedRectangle(on ? b.translated(0.0f, 1.0f) : b, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(hl ? 0.055f : 0.028f));
        for (int yy = (int)b.getY() + 2; yy < (int)b.getBottom() - 2; yy += 3)
            g.drawHorizontalLine(yy, b.getX() + 3.0f, b.getRight() - 3.0f);

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

        if (on)
        {
            const auto led = on && txt.contains("CONFIRM") ? LnfC::RecRed : LnfC::Accent;
            const float lr = 2.4f;
            const float lx = b.getRight() - 8.0f;
            const float ly = b.getY() + 7.0f;
            g.setColour(led.withAlpha(0.12f));
            g.fillEllipse(lx - lr * 2.8f, ly - lr * 2.8f, lr * 5.6f, lr * 5.6f);
            g.setColour(led);
            g.fillEllipse(lx - lr, ly - lr, lr * 2.0f, lr * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.75f));
            g.fillEllipse(lx - lr * 0.45f, ly - lr * 0.65f, lr * 0.65f, lr * 0.65f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool isMouseOverButton, bool isButtonDown) override
    {
        if (btn.getComponentID() != "headerConsoleButton")
        {
            juce::LookAndFeel_V4::drawButtonText(g, btn, isMouseOverButton, isButtonDown);
            return;
        }

        const bool illuminated = btn.getToggleState() || isButtonDown;
        const auto led = btn.findColour(juce::TextButton::buttonOnColourId);
        auto textArea = btn.getLocalBounds().reduced(7, 1);
        if (isButtonDown)
            textArea.translate(0, 1);

        g.setFont(juce::Font(juce::FontOptions(9.6f).withStyle("Bold"))
                      .withExtraKerningFactor(0.045f));
        g.setColour(juce::Colours::black.withAlpha(illuminated ? 0.48f : 0.78f));
        g.drawFittedText(btn.getButtonText(), textArea.translated(0, 1),
                         juce::Justification::centred, 1, 0.82f);
        g.setColour(illuminated ? juce::Colours::white
                                : isMouseOverButton ? led.brighter(0.75f)
                                                    : juce::Colour(0xffe2e5e9));
        g.drawFittedText(btn.getButtonText(), textArea,
                         juce::Justification::centred, 1, 0.82f);
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool,
                      int bx, int by, int bw, int bh, juce::ComboBox& box) override
    {
        juce::Rectangle<float> b(0.0f, 0.0f, (float)w, (float)h);
        if (box.getComponentID() == "rhythmScreenCombo")
        {
            juce::ColourGradient screen(juce::Colour(0xff183b40), b.getX(), b.getY(),
                                         juce::Colour(0xff071719), b.getX(), b.getBottom(), false);
            g.setGradientFill(screen);
            g.fillRoundedRectangle(b, 2.5f);
            g.setColour(juce::Colour(0xff83e6eb).withAlpha(0.34f));
            g.drawRoundedRectangle(b.reduced(0.5f), 2.5f, 0.8f);
            auto ac = juce::Rectangle<float>((float)bx, (float)by, (float)bw, (float)bh).getCentre();
            const float as = juce::jmax(2.5f, (float)bh * 0.24f);
            juce::Path arr;
            arr.addTriangle({ ac.x - as, ac.y - as * 0.45f },
                            { ac.x + as, ac.y - as * 0.45f },
                            { ac.x, ac.y + as * 0.55f });
            g.setColour(juce::Colour(0xffffc85d));
            g.fillPath(arr);
            return;
        }

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
        if (cb.getComponentID() == "rhythmScreenCombo")
        {
            lbl.setBounds(3, 0, cb.getWidth() - 13, cb.getHeight());
            lbl.setFont(getComboBoxFont(cb));
            return;
        }
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

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        if (box.getComponentID() == "rhythmScreenCombo")
            return juce::Font(juce::FontOptions(7.2f).withStyle("Bold"));
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

        if (slider.getComponentID() == "mxrTenBandSlider")
        {
            const float cx = b.getCentreX();
            const float trackTop = b.getY() + 4.0f;
            const float trackBottom = b.getBottom() - 4.0f;
            g.setColour(juce::Colours::black.withAlpha(0.86f));
            g.fillRoundedRectangle(cx - 2.3f, trackTop, 4.6f, trackBottom - trackTop, 1.4f);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawVerticalLine(juce::roundToInt(cx - 0.5f), trackTop + 1.0f, trackBottom - 1.0f);

            const auto blue = slider.findColour(juce::Slider::trackColourId);
            g.setColour(blue.withAlpha(0.20f));
            g.fillEllipse(cx - 8.5f, pos - 8.5f, 17.0f, 17.0f);
            g.setColour(juce::Colours::black.withAlpha(0.58f));
            g.fillRoundedRectangle(cx - 7.2f, pos - 4.6f, 14.4f, 10.8f, 2.5f);
            juce::ColourGradient cap(juce::Colour(0xfff5f6f6), cx - 5.5f, pos - 4.0f,
                                     juce::Colour(0xff6e7478), cx + 5.0f, pos + 5.0f, true);
            g.setGradientFill(cap);
            g.fillRoundedRectangle(cx - 6.2f, pos - 5.0f, 12.4f, 10.0f, 2.0f);
            g.setColour(blue.withAlpha(0.96f));
            g.fillRoundedRectangle(cx - 2.2f, pos - 3.6f, 4.4f, 7.2f, 1.2f);
            g.setColour(juce::Colours::white.withAlpha(0.72f));
            g.drawHorizontalLine(juce::roundToInt(pos - 3.0f), cx - 1.5f, cx + 1.5f);
            return;
        }

        float trackThick = 3.0f;
        juce::Rectangle<float> track;
        if (horiz) track = { b.getX(), b.getCentreY() - trackThick * 0.5f, b.getWidth(), trackThick };
        else       track = { b.getCentreX() - trackThick * 0.5f, b.getY(), trackThick, b.getHeight() };

        g.setColour(juce::Colours::black.withAlpha(0.80f));
        g.fillRoundedRectangle(track.expanded(horiz ? 1.5f : 0.8f, horiz ? 0.8f : 1.5f), 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.035f));
        g.fillRoundedRectangle(track, trackThick * 0.5f);

        juce::Rectangle<float> filled;
        if (horiz) filled = { track.getX(), track.getY(), pos - (float)x, trackThick };
        else       filled = { track.getX(), pos, trackThick, track.getBottom() - pos };

        const auto fillColour = slider.findColour(juce::Slider::trackColourId).withMultipliedSaturation(1.25f);
        g.setColour(fillColour.withAlpha(0.18f));
        g.fillRoundedRectangle(filled.expanded(0.0f, 2.5f), 3.0f);
        g.setColour(fillColour.withAlpha(0.92f));
        g.fillRoundedRectangle(filled, trackThick * 0.5f);

        float thumbR = 6.0f;
        float tx = horiz ? pos : b.getCentreX();
        float ty = horiz ? b.getCentreY() : pos;

        g.setColour(fillColour.withAlpha(0.20f));
        g.fillEllipse(tx - thumbR * 1.8f, ty - thumbR * 1.8f, thumbR * 3.6f, thumbR * 3.6f);
        juce::ColourGradient thumbGrad(LnfC::MetalHi, tx - thumbR, ty - thumbR,
                                       LnfC::MetalLo, tx + thumbR, ty + thumbR, false);
        g.setGradientFill(thumbGrad);
        g.fillEllipse(tx - thumbR, ty - thumbR, thumbR * 2.0f, thumbR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawEllipse(tx - thumbR, ty - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.0f);

        juce::ignoreUnused(slider);
    }

private:
    static void drawHeaderConsoleButton(juce::Graphics& g, juce::Button& btn,
                                        bool isMouseOverButton, bool isButtonDown)
    {
        auto bounds = btn.getLocalBounds().toFloat().reduced(1.0f, 0.8f);
        const bool illuminated = btn.getToggleState() || isButtonDown;
        const auto led = btn.findColour(juce::TextButton::buttonOnColourId);
        const float glowStrength = illuminated ? 1.0f : isMouseOverButton ? 0.52f : 0.22f;

        // Recessed socket first: everything that follows is seen through the clear cap.
        g.setColour(juce::Colours::black.withAlpha(0.78f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.6f), 3.4f);
        juce::ColourGradient socket(juce::Colour(0xff111419), bounds.getX(), bounds.getY(),
                                    juce::Colour(0xff010203), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(socket);
        g.fillRoundedRectangle(bounds, 3.4f);
        g.setColour(juce::Colours::black.withAlpha(0.82f));
        g.drawRoundedRectangle(bounds.reduced(0.4f), 3.2f, 0.8f);

        auto cap = bounds.reduced(2.1f, 1.8f);
        if (isButtonDown)
            cap.translate(0.0f, 1.0f);

        // A small emitter and its bloom are painted underneath, not mixed into the plastic.
        const float emitterWidth = juce::jlimit(22.0f, 48.0f, cap.getWidth() * 0.34f);
        auto emitter = juce::Rectangle<float>(cap.getCentreX() - emitterWidth * 0.5f,
                                               cap.getBottom() - 4.2f,
                                               emitterWidth, 2.3f);
        g.setColour(led.withAlpha(0.055f * glowStrength));
        g.fillRoundedRectangle(cap.expanded(1.4f, 0.9f), 3.5f);
        juce::ColourGradient bloom(led.withAlpha(0.015f), cap.getCentreX(), cap.getY(),
                                   led.withAlpha(0.62f * glowStrength), cap.getCentreX(), cap.getBottom(), false);
        bloom.addColour(0.58, led.withAlpha(0.11f * glowStrength));
        bloom.addColour(0.82, led.withAlpha(0.32f * glowStrength));
        g.setGradientFill(bloom);
        g.fillRoundedRectangle(cap.reduced(0.5f), 2.7f);
        g.setColour(led.withAlpha(illuminated ? 0.94f : isMouseOverButton ? 0.58f : 0.30f));
        g.fillRoundedRectangle(emitter, 1.1f);
        g.setColour(led.brighter(0.85f).withAlpha(illuminated ? 0.76f : 0.20f));
        g.fillRoundedRectangle(emitter.reduced(5.0f, 0.5f), 0.6f);

        // Neutral, genuinely translucent acrylic: the colour visible here comes from the LED below.
        juce::ColourGradient acrylic(juce::Colours::white.withAlpha(isButtonDown ? 0.10f : 0.22f),
                                     cap.getX(), cap.getY(),
                                     juce::Colour(0xff9aa2ac).withAlpha(0.035f),
                                     cap.getX(), cap.getBottom(), false);
        acrylic.addColour(0.30, juce::Colour(0xffe8edf2).withAlpha(isMouseOverButton ? 0.13f : 0.085f));
        acrylic.addColour(0.68, juce::Colour(0xff48505a).withAlpha(0.055f));
        acrylic.addColour(0.88, juce::Colours::white.withAlpha(0.025f));
        g.setGradientFill(acrylic);
        g.fillRoundedRectangle(cap, 2.6f);

        // Bevel and refraction cues define the clear shell without making it opaque.
        g.setColour(juce::Colours::white.withAlpha(isButtonDown ? 0.16f : 0.48f));
        g.drawRoundedRectangle(cap.reduced(0.35f), 2.45f, 0.75f);
        g.setColour(juce::Colours::white.withAlpha(isButtonDown ? 0.04f : 0.19f));
        g.fillRoundedRectangle(cap.reduced(1.3f).withHeight(2.7f), 1.3f);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawVerticalLine(juce::roundToInt(cap.getX() + 1.2f),
                           cap.getY() + 3.0f, cap.getBottom() - 3.0f);
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.drawHorizontalLine(juce::roundToInt(cap.getBottom() - 0.9f),
                             cap.getX() + 2.0f, cap.getRight() - 2.0f);
        g.setColour(led.withAlpha(0.13f + 0.16f * glowStrength));
        g.drawHorizontalLine(juce::roundToInt(cap.getBottom() - 1.8f),
                             cap.getX() + 5.0f, cap.getRight() - 5.0f);
    }
};

namespace LoopernicoUi
{
    static inline void drawScrew(juce::Graphics& g, float cx, float cy, float r)
    {
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillEllipse(cx - r - 1.6f, cy - r + 0.4f, r * 2.0f + 3.2f, r * 2.0f + 3.2f);
        g.setColour(juce::Colours::black.withAlpha(0.82f));
        g.fillEllipse(cx - r - 0.6f, cy - r + 1.2f, r * 2.0f + 1.2f, r * 2.0f + 1.2f);

        juce::ColourGradient grad(
            juce::Colour(0xffd4d5d6), cx - r * 0.48f, cy - r * 0.58f,
            juce::Colour(0xff26282b), cx + r * 0.38f, cy + r * 0.48f, true);
        grad.addColour(0.38, juce::Colour(0xff909397));
        grad.addColour(0.72, juce::Colour(0xff4a4d51));
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.30f));
        g.drawEllipse(cx - r + 0.4f, cy - r + 0.4f, r * 2.0f - 0.8f, r * 2.0f - 0.8f, 0.8f);
        g.setColour(juce::Colours::black.withAlpha(0.62f));
        g.drawEllipse(cx - r + 1.2f, cy - r + 1.2f, r * 2.0f - 2.4f, r * 2.0f - 2.4f, 0.7f);

        const float angle = std::fmod(cx * 0.071f + cy * 0.113f, juce::MathConstants<float>::pi);
        const float sl = r * 0.58f;
        auto endPoint = [=](float a, float distance)
        {
            return juce::Point<float>(cx + std::cos(a) * distance, cy + std::sin(a) * distance);
        };
        const auto a1 = endPoint(angle, sl);
        const auto a2 = endPoint(angle + juce::MathConstants<float>::pi, sl);
        const auto b1 = endPoint(angle + juce::MathConstants<float>::halfPi, sl * 0.88f);
        const auto b2 = endPoint(angle - juce::MathConstants<float>::halfPi, sl * 0.88f);

        g.setColour(juce::Colours::black.withAlpha(0.90f));
        g.drawLine({ a1, a2 }, juce::jmax(1.2f, r * 0.24f));
        g.drawLine({ b1, b2 }, juce::jmax(1.0f, r * 0.20f));
        g.setColour(juce::Colours::white.withAlpha(0.14f));
        g.drawLine({ a1.translated(0.0f, -0.55f), a2.translated(0.0f, -0.55f) }, 0.55f);

        g.setColour(juce::Colours::white.withAlpha(0.42f));
        g.fillEllipse(cx - r * 0.52f, cy - r * 0.72f, r * 0.62f, r * 0.28f);
    }

    static inline void drawLedLens(juce::Graphics& g, float cx, float cy, float r,
                                   juce::Colour colour, bool on)
    {
        if (on)
        {
            for (int layer = 4; layer >= 1; --layer)
            {
                const float expand = r * (0.65f + 0.50f * (float) layer);
                g.setColour(colour.withAlpha(0.028f * (float) (5 - layer)));
                g.fillEllipse(cx - r - expand, cy - r - expand,
                              (r + expand) * 2.0f, (r + expand) * 2.0f);
            }
        }

        g.setColour(juce::Colours::black.withAlpha(0.78f));
        g.fillEllipse(cx - r - 2.2f, cy - r - 1.2f, r * 2.0f + 4.4f, r * 2.0f + 4.4f);
        juce::ColourGradient bezel(juce::Colour(0xff777b7f), cx - r, cy - r,
                                   juce::Colour(0xff17191b), cx + r, cy + r, true);
        g.setGradientFill(bezel);
        g.fillEllipse(cx - r - 1.0f, cy - r - 1.0f, r * 2.0f + 2.0f, r * 2.0f + 2.0f);

        const auto lensHi = on ? colour.brighter(0.65f) : colour.withMultipliedBrightness(0.24f);
        const auto lensLo = on ? colour.darker(0.35f) : colour.withMultipliedBrightness(0.07f);
        juce::ColourGradient lens(lensHi, cx - r * 0.42f, cy - r * 0.55f,
                                  lensLo, cx + r * 0.35f, cy + r * 0.46f, true);
        g.setGradientFill(lens);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.75f);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.76f : 0.28f));
        g.fillEllipse(cx - r * 0.52f, cy - r * 0.68f, r * 0.62f, r * 0.36f);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.24f : 0.06f));
        g.fillEllipse(cx - r * 0.72f, cy - r * 0.78f, r * 1.44f, r * 0.92f);
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
                                 const juce::String& title, juce::Colour accent,
                                 bool showHeader = true)
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
        if (showHeader)
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

        if (!showHeader)
            return;

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

        if (title.isNotEmpty())
        {
            g.setColour(LnfC::TextHi.withAlpha(0.95f));
            g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
            g.drawText(title,
                       hdr.getX() + 26, hdr.getY() + 2,
                       hdr.getWidth() - 52, hdr.getHeight() - 4,
                       juce::Justification::centredLeft);
        }
    }
}
