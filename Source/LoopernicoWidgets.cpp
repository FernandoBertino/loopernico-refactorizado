#include "LoopernicoWidgets.h"

using namespace LoopernicoUi;

void MasterVUMeter::drawBar(juce::Graphics& g, juce::Rectangle<float> bar, float level)
{
    g.setColour(LnfC::BG.darker(0.3f));
    g.fillRoundedRectangle(bar, 2.0f);

    float db = (level > 0.00001f) ? 20.0f * std::log10(level) : -96.0f;
    float mapped = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 54.0f);

    const int N = 18;
    float segH = bar.getHeight() / (float)N;
    int topSeg = (int)(mapped * N);

    for (int i = 0; i < N; ++i)
    {
        float bot = bar.getBottom() - (float)i * segH;
        float top = bot - segH + 1.5f;
        float thr = (float)i / (float)N;
        bool lit = (1.0f - thr) <= mapped;

        juce::Colour col = (i < 10) ? LnfC::PlayGreen
                         : (i < 14) ? juce::Colour(0xffbbcc00)
                         : (i < 16) ? LnfC::OvdAmber
                                    : LnfC::RecRed;

        if (lit)
        {
            bool isTop = (i == topSeg);
            g.setColour(isTop ? col.brighter(0.5f) : col);
            g.fillRoundedRectangle(bar.getX() + 1.5f, top,
                                   bar.getWidth() - 3.0f, segH - 1.5f, 1.2f);

            if (isTop)
            {
                g.setColour(col.withAlpha(0.30f));
                g.fillRoundedRectangle(bar.getX() - 1.0f, top - 1.0f,
                                       bar.getWidth() + 2.0f, segH + 1.0f, 2.0f);
            }
        }
        else
        {
            g.setColour(col.withAlpha(0.08f));
            g.fillRoundedRectangle(bar.getX() + 1.5f, top,
                                   bar.getWidth() - 3.0f, segH - 1.5f, 1.2f);
        }
    }
}

void MasterVUMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    const bool compact = b.getWidth() < 52;
    const int scaleW = compact ? 0 : 26;
    const int barGap = compact ? 2 : 3;
    const int barW = juce::jmax(8, (b.getWidth() - scaleW - barGap - (compact ? 0 : 4)) / 2);
    const int barH = juce::jmax(18, b.getHeight() - 16);
    int barY = 0;

    juce::Rectangle<float> barL((float)(0), (float)barY, (float)barW, (float)barH);
    juce::Rectangle<float> barR((float)(barW + barGap), (float)barY, (float)barW, (float)barH);
    drawBar(g, barL, levelL);
    drawBar(g, barR, levelR);

    g.setColour(LnfC::TextDim);
    g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
    g.drawText("L", 0, barH + 2, barW, 12, juce::Justification::centred);
    g.drawText("R", barW + barGap, barH + 2, barW, 12, juce::Justification::centred);

    if (compact)
        return;

    int scaleX = barW * 2 + barGap + 3;
    struct { const char* lbl; float db; } marks[] = {
        { "+6", 6.0f }, { "0", 0.0f }, { "-6", -6.0f },
        { "-12", -12.0f }, { "-24", -24.0f }, { "-48", -48.0f },
    };
    g.setFont(juce::Font(juce::FontOptions(7.5f)));
    for (auto& m : marks)
    {
        float frac = 1.0f - (m.db + 48.0f) / 54.0f;
        int y = barY + (int)(frac * (float)barH);
        g.setColour(LnfC::Border.withAlpha(0.6f));
        g.drawHorizontalLine(y, (float)scaleX, (float)(scaleX + 4));
        g.setColour(m.db >= 0 ? LnfC::RecRed.withAlpha(0.8f) : LnfC::TextDim.withAlpha(0.7f));
        g.drawText(m.lbl, scaleX + 5, y - 4, scaleW - 6, 9, juce::Justification::centredLeft);
    }
}

void VUMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(LnfC::BG.darker(0.20f));
    g.fillRoundedRectangle(b, 2.0f);

    const int N = 14;
    float segH = b.getHeight() / (float)N;
    int topSeg = (int)(level * N);

    for (int i = 0; i < N; ++i)
    {
        float bot = b.getBottom() - (float)i * segH;
        float top = bot - segH + 1.8f;
        float thr = (float)i / (float)N;
        bool lit = (1.0f - thr) <= level;

        juce::Colour col = (i < 8) ? LnfC::PlayGreen
                         : (i < 11) ? juce::Colour(0xffaacc00)
                                    : LnfC::RecRed;

        if (lit)
        {
            bool isTop = (i == topSeg);
            g.setColour(isTop ? col.brighter(0.4f) : col);
            g.fillRoundedRectangle(b.getX() + 1.5f, top,
                                   b.getWidth() - 3.0f, segH - 1.8f, 1.5f);

            if (isTop)
            {
                g.setColour(col.withAlpha(0.28f));
                g.fillRoundedRectangle(b.getX() - 0.5f, top - 0.8f,
                                       b.getWidth() + 1.0f, segH, 2.0f);
            }
        }
        else
        {
            g.setColour(col.withAlpha(0.08f));
            g.fillRoundedRectangle(b.getX() + 1.5f, top,
                                   b.getWidth() - 3.0f, segH - 1.8f, 1.5f);
        }
    }

    g.setColour(LnfC::Border.withAlpha(0.35f));
    g.drawRoundedRectangle(b, 2.0f, 0.7f);
}

void StepButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.5f);

    if (active)
    {
        juce::Colour col = isCur ? LnfC::StepCur : LnfC::StepOn;
        g.setColour(col.withAlpha(0.18f));
        g.fillRoundedRectangle(b.expanded(1.5f), 4.0f);

        juce::ColourGradient bodyGrad(
            col.brighter(isCur ? 0.40f : 0.15f), b.getCentreX(), b.getY(),
            col.darker(0.15f),                   b.getCentreX(), b.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(b, 3.0f);

        g.setColour(juce::Colours::white.withAlpha(isCur ? 0.25f : 0.14f));
        g.fillRoundedRectangle(b.withHeight(b.getHeight() * 0.32f).reduced(1.5f, 0.0f), 2.0f);
        g.setColour(col.brighter(isCur ? 0.50f : 0.25f));
        g.drawRoundedRectangle(b, 3.0f, 1.0f);
    }
    else
    {
        juce::Colour base = isCur ? LnfC::Border.brighter(0.25f) : juce::Colour(0xff1c1c2a);
        juce::ColourGradient bodyGrad(
            base.brighter(0.06f), b.getCentreX(), b.getY(),
            base.darker(0.10f),   b.getCentreX(), b.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(b, 3.0f);
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillRoundedRectangle(b.reduced(1.0f), 2.5f);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(b.reduced(2.0f), 2.0f);
        g.setColour(LnfC::Border.withAlpha(isCur ? 0.60f : 0.28f));
        g.drawRoundedRectangle(b, 3.0f, 1.0f);
    }
}

void CircularButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(5.0f);
    float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    auto ctr = b.getCentre();

    juce::Colour ringCol = getBaseColour();

    if (active)
    {
        g.setColour(ringCol.withAlpha(0.06f));
        g.fillEllipse(ctr.x - (r + 14), ctr.y - (r + 14), (r + 14) * 2, (r + 14) * 2);
        g.setColour(ringCol.withAlpha(0.14f));
        g.fillEllipse(ctr.x - (r + 8), ctr.y - (r + 8), (r + 8) * 2, (r + 8) * 2);
        g.setColour(ringCol.withAlpha(0.24f));
        g.fillEllipse(ctr.x - (r + 3), ctr.y - (r + 3), (r + 3) * 2, (r + 3) * 2);
    }

    juce::Colour outerRing = active ? ringCol : ringCol.darker(0.60f);
    g.setColour(outerRing);
    g.fillEllipse(ctr.x - r, ctr.y - r, r * 2, r * 2);

    g.setColour(juce::Colours::white.withAlpha(active ? 0.18f : 0.08f));
    g.fillEllipse(ctr.x - r, ctr.y - r, r * 2, r * 0.5f);

    float ir = r - (active ? 3.5f : 4.0f);
    juce::ColourGradient bodyGrad(
        juce::Colour(0xff52525e).brighter(pressed ? 0.20f : hovered ? 0.10f : 0.0f),
        ctr.x - ir * 0.35f, ctr.y - ir * 0.42f,
        juce::Colour(0xff0e0e18),
        ctr.x + ir * 0.25f, ctr.y + ir * 0.38f,
        true);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(ctr.x - ir, ctr.y - ir, ir * 2, ir * 2);

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawEllipse(ctr.x - ir, ctr.y - ir, ir * 2, ir * 2, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillEllipse(ctr.x - ir * 0.52f, ctr.y - ir * 0.76f, ir * 1.04f, ir * 0.52f);

    juce::Colour iconCol = active ? ringCol.brighter(0.25f) : ringCol.darker(0.30f).withAlpha(0.70f);

    if (type == Rec)
    {
        float dr = ir * 0.36f;
        if (active)
        {
            g.setColour(ringCol.withAlpha(0.30f));
            g.fillEllipse(ctr.x - dr * 1.7f, ctr.y - dr * 1.7f, dr * 3.4f, dr * 3.4f);
            g.setColour(ringCol.withAlpha(0.55f));
            g.fillEllipse(ctr.x - dr * 1.2f, ctr.y - dr * 1.2f, dr * 2.4f, dr * 2.4f);
        }
        g.setColour(iconCol);
        g.fillEllipse(ctr.x - dr, ctr.y - dr, dr * 2, dr * 2);
        if (active)
        {
            g.setColour(juce::Colours::white.withAlpha(0.22f));
            g.fillEllipse(ctr.x - dr * 0.5f, ctr.y - dr * 0.75f, dr * 0.7f, dr * 0.45f);
        }
    }
    else if (type == Play)
    {
        float ts = ir * 0.38f;
        juce::Path tri;
        tri.addTriangle(ctr.x - ts * 0.65f, ctr.y - ts,
                        ctr.x - ts * 0.65f, ctr.y + ts,
                        ctr.x + ts * 0.90f, ctr.y);
        if (active)
        {
            g.setColour(iconCol.withAlpha(0.28f));
            juce::AffineTransform scale = juce::AffineTransform::scale(1.35f, 1.35f, ctr.x, ctr.y);
            g.fillPath(tri, scale);
        }
        g.setColour(iconCol);
        g.fillPath(tri);
    }
    else
    {
        float or1 = ir * 0.40f;
        float or2 = ir * 0.20f;
        g.setColour(iconCol);
        g.drawEllipse(ctr.x - or1, ctr.y - or1, or1 * 2, or1 * 2, 2.5f);
        if (active)
        {
            g.setColour(iconCol.withAlpha(0.35f));
            g.fillEllipse(ctr.x - or2 * 1.6f, ctr.y - or2 * 1.6f, or2 * 3.2f, or2 * 3.2f);
        }
        g.setColour(iconCol);
        g.fillEllipse(ctr.x - or2, ctr.y - or2, or2 * 2, or2 * 2);
    }
}

TrackComponent::TrackComponent(int trackIndex) : index(trackIndex)
{
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff111317));
    muteBtn.setColour(juce::TextButton::buttonOnColourId, LnfC::RecRed.withAlpha(0.70f));
    muteBtn.setColour(juce::TextButton::textColourOffId, LnfC::TextHi.withAlpha(0.70f));
    addAndMakeVisible(muteBtn);
    muteBtn.onClick = [this]() { if (onMuteChanged) onMuteChanged(muteBtn.getToggleState()); };

    soloBtn.setClickingTogglesState(true);
    soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff111317));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffcc9900));
    soloBtn.setColour(juce::TextButton::textColourOffId, LnfC::TextHi.withAlpha(0.70f));
    addAndMakeVisible(soloBtn);
    soloBtn.onClick = [this]() { if (onSoloChanged) onSoloChanged(soloBtn.getToggleState()); };

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.85, juce::dontSendNotification);
    volumeSlider.setColour(juce::Slider::trackColourId, LnfC::trackColor(index).withAlpha(0.70f));
    volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff171a1f));
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.85f));
    volumeSlider.onValueChange = [this]()
    {
        if (onVolumeChanged) onVolumeChanged((float)volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(vuMeter);
}

void TrackComponent::setPlayheadProgress(float p)
{
    if (playheadProg != p) { playheadProg = p; repaint(); }
}

void TrackComponent::setStatus(bool rec, bool play, bool ovd)
{
    isRec = rec;
    isPlay = play;
    isOvd = ovd;
    recBtn.setActive(rec);
    playBtn.setActive(play);
    ovdBtn.setActive(ovd);
    repaint();
}

void TrackComponent::setLoopSecs(float t, float len)
{
    timeSecs = t;
    lenSecs = len;
    repaint();
}

void TrackComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    juce::Colour tColor = LnfC::trackColor(index);
    const float leftW = 182.0f;
    auto leftArea = juce::Rectangle<float>(b.getX(), b.getY(), leftW, b.getHeight());
    auto waveArea = juce::Rectangle<float>(b.getX() + leftW - 2.0f, b.getY(),
                                           b.getWidth() - leftW - 2.0f, b.getHeight());

    juce::Colour stateC = isRec ? LnfC::RecRed
                        : isOvd ? LnfC::OvdAmber
                        : isPlay ? tColor
                        : (lenSecs > 0.0f) ? tColor.withAlpha(0.85f)
                                           : LnfC::TextDim;

    juce::String stateStr = isRec ? "REC"
                          : isOvd ? "DUB"
                          : isPlay ? "PLAY"
                          : (lenSecs > 0.0f) ? "STOP"
                                             : "EMPTY";

    juce::Colour tint = isRec ? LnfC::RecRed.withAlpha(0.05f)
                      : isOvd ? LnfC::OvdAmber.withAlpha(0.04f)
                      : isPlay ? tColor.withAlpha(0.035f)
                               : juce::Colours::transparentBlack;

    const bool hasWaveform = thumbnail != nullptr
        && thumbnail->getNumChannels() > 0
        && thumbnail->getTotalLength() > 0.0;

    g.setColour(juce::Colour(0xff040506));
    g.fillRect((int)waveArea.getX(), (int)waveArea.getY(), (int)waveArea.getWidth(), (int)waveArea.getHeight());
    if (hasWaveform)
    {
        g.setColour(tint);
        g.fillRect((int)waveArea.getX(), (int)waveArea.getY(), (int)waveArea.getWidth(), (int)waveArea.getHeight());
        drawFineRackTexture(g, waveArea, 0.16f);
    }

    juce::Colour stripC = isRec ? LnfC::RecRed : isOvd ? LnfC::OvdAmber : tColor;
    float stripAlpha = (isRec || isOvd || isPlay || lenSecs > 0.0f) ? 0.90f : 0.25f;
    g.setColour(stripC.withAlpha(stripAlpha));
    g.fillRoundedRectangle(b.getX() + 1.5f, b.getY() + 4.0f, 3.0f, b.getHeight() - 8.0f, 1.5f);
    if (isRec || isOvd || isPlay)
    {
        g.setColour(stripC.withAlpha(0.25f));
        g.fillRoundedRectangle(b.getX() + 0.0f, b.getY() + 3.0f, 7.0f, b.getHeight() - 6.0f, 3.0f);
    }

    juce::ColourGradient leftGlow(
        stateC.withAlpha(0.10f), leftArea.getX() + 6.0f, leftArea.getCentreY(),
        juce::Colours::transparentBlack, leftArea.getRight() - 10.0f, leftArea.getCentreY(), false);
    g.setGradientFill(leftGlow);
    g.fillRoundedRectangle(leftArea.reduced(8.0f, 6.0f), 6.0f);

    g.setColour(stateC.withAlpha(0.95f));
    g.setFont(juce::Font(juce::FontOptions(28.0f).withStyle("Bold")));
    g.drawText(juce::String(index + 1), 14, 8, 26, 36, juce::Justification::centredLeft);

    g.setColour(stateC);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    g.drawText(stateStr, 42, 14, 94, 16, juce::Justification::centredLeft);

    int tMin = (int)(timeSecs / 60);
    float tSec = timeSecs - tMin * 60;
    g.setColour(LnfC::TextHi.withAlpha(0.82f));
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText(juce::String::formatted("%02d:%04.1f", tMin, tSec),
               42, 34, 88, 14, juce::Justification::centredLeft);

    if (lenSecs > 0.0f)
    {
        int lMin = (int)(lenSecs / 60);
        float lSec = lenSecs - lMin * 60;
        g.setColour(LnfC::TextDim.withAlpha(0.78f));
        g.setFont(juce::Font(juce::FontOptions(8.5f)));
        g.drawText("L:" + juce::String::formatted("%02d:%04.1f", lMin, lSec),
                   42, 48, 80, 12, juce::Justification::centredLeft);
    }

    g.setColour(LnfC::TextDim.withAlpha(0.72f));
    g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
    g.drawText("VOL", 14, 68, 24, 12, juce::Justification::centredLeft);

    int wx = (int)waveArea.getX();
    int wy = (int)waveArea.getY();
    int ww = (int)waveArea.getWidth();
    int wh = (int)waveArea.getHeight();
    auto waveRect = juce::Rectangle<int>(wx, wy, ww, wh);

    auto waveContent = waveRect;

    if (hasWaveform)
    {
        juce::Colour wC = isRec ? LnfC::RecRed.brighter(0.15f)
                        : isOvd ? LnfC::OvdAmber
                                : tColor.brighter(0.05f);
        g.setColour(wC.withAlpha(isPlay ? 0.88f : 0.55f));
        if (!monoDisplay)
        {
            thumbnail->drawChannels(g, waveContent, 0.0, thumbnail->getTotalLength(), 1.0f);
        }
        else
        {
            const float centreY = waveContent.toFloat().getCentreY();
            const float halfHeight = waveContent.toFloat().getHeight() * 0.44f;
            const double totalLength = thumbnail->getTotalLength();
            const int width = juce::jmax(1, waveContent.getWidth());
            for (int x = 0; x < width; ++x)
            {
                const double t0 = totalLength * (double) x / (double) width;
                const double t1 = totalLength * (double) (x + 1) / (double) width;
                float minValue = 0.0f, maxValue = 0.0f;
                thumbnail->getApproximateMinMax(t0, t1, 0, minValue, maxValue);
                g.drawVerticalLine(waveContent.getX() + x,
                                   centreY + minValue * halfHeight,
                                   centreY + maxValue * halfHeight);
            }
        }

        if (playheadProg >= 0.0f && playheadProg <= 1.0f)
        {
            float phX = (float)waveContent.getX() + (float)waveContent.getWidth() * playheadProg;
            g.setColour(tColor.withAlpha(0.25f));
            g.drawVerticalLine((int)phX - 1, (float)waveContent.getY(), (float)waveContent.getBottom());
            g.drawVerticalLine((int)phX + 1, (float)waveContent.getY(), (float)waveContent.getBottom());
            g.setColour(tColor.withAlpha(0.90f));
            g.drawVerticalLine((int)phX, (float)waveContent.getY(), (float)waveContent.getBottom());
            juce::Path tri;
            tri.addTriangle(phX - 4.0f, (float)waveContent.getY(),
                            phX + 4.0f, (float)waveContent.getY(),
                            phX, (float)waveContent.getY() + 6.0f);
            g.setColour(tColor.brighter(0.25f));
            g.fillPath(tri);
        }
    }
}

void TrackComponent::resized()
{
    const int leftW = 182;
    vuMeter.setBounds(150, 10, 13, getHeight() - 26);
    volumeSlider.setBounds(42, 68, 92, 10);
    muteBtn.setBounds(16, 86, 24, 16);
    soloBtn.setBounds(44, 86, 24, 16);

    recBtn.setBounds(leftW - 88, 12, 22, 22);
    playBtn.setBounds(leftW - 62, 12, 22, 22);
    ovdBtn.setBounds(leftW - 36, 12, 22, 22);
}

FXPedalComponent::FXPedalComponent(const juce::String& name, juce::Colour col,
                                   const juce::String& l1, const juce::String& l2,
                                   const juce::String& l3)
    : pedalName(name), k1(l1), k2(l2), k3(l3), pedalCol(col)
{
    auto setupKnob = [](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, 1.0, 0.01);
        s.setValue(0.5);
    };
    setupKnob(knob1); addAndMakeVisible(knob1);
    setupKnob(knob2); addAndMakeVisible(knob2);
    setupKnob(knob3); addAndMakeVisible(knob3);
    setupKnob(knob4); addAndMakeVisible(knob4); knob4.setVisible(false);
    setupKnob(knob5); addAndMakeVisible(knob5); knob5.setVisible(false);
    setupKnob(knob6); addAndMakeVisible(knob6); knob6.setVisible(false);
    setupKnob(knob7); addAndMakeVisible(knob7); knob7.setVisible(false);
    setupKnob(knob8); addAndMakeVisible(knob8); knob8.setVisible(false);
    setupKnob(knob9); addAndMakeVisible(knob9); knob9.setVisible(false);

    onBtn.setClickingTogglesState(true);
    addAndMakeVisible(onBtn);
}

void FXPedalComponent::configureExtendedKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                                              const juce::String& l4, const juce::String& l5, const juce::String& l6)
{
    extendedKnobs = true;
    nineKnobs = false;
    k1 = l1; k2 = l2; k3 = l3;
    k4 = l4; k5 = l5; k6 = l6;
    knob4.setVisible(true);
    knob5.setVisible(true);
    knob6.setVisible(true);
    knob7.setVisible(false);
    knob8.setVisible(false);
    knob9.setVisible(false);
    resized();
    repaint();
}

void FXPedalComponent::configureNineKnobs(const juce::String& l1, const juce::String& l2, const juce::String& l3,
                                          const juce::String& l4, const juce::String& l5, const juce::String& l6,
                                          const juce::String& l7, const juce::String& l8, const juce::String& l9)
{
    extendedKnobs = false;
    nineKnobs = true;
    k1 = l1; k2 = l2; k3 = l3;
    k4 = l4; k5 = l5; k6 = l6;
    k7 = l7; k8 = l8; k9 = l9;
    knob4.setVisible(true);
    knob5.setVisible(true);
    knob6.setVisible(true);
    knob7.setVisible(true);
    knob8.setVisible(true);
    knob9.setVisible(true);
    resized();
    repaint();
}

void FXPedalComponent::setKnobValueFormatter(int knobIndex, std::function<juce::String(double)> formatter)
{
    if (knobIndex < 0 || knobIndex >= 9)
        return;

    knobValueFormatters[knobIndex] = std::move(formatter);
    repaint();
}

void FXPedalComponent::setInfoText(const juce::String& mainText, const juce::String& subText)
{
    if (infoMain == mainText && infoSub == subText)
        return;

    infoMain = mainText;
    infoSub = subText;
    repaint();
}

void FXPedalComponent::setDragHintVisible(bool visible)
{
    if (dragHintVisible == visible)
        return;

    dragHintVisible = visible;
    repaint();
}

// The pedal rendering remains intentionally detailed because it defines the hardware-like identity of the UI.
void FXPedalComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(3.0f);
    auto originalBody = b;

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(b.translated(0.0f, 2.0f).expanded(1.0f), 10.0f);

    juce::ColourGradient bodyGrad(
        pedalCol.brighter(0.12f), b.getX(), b.getY(),
        pedalCol.darker(0.50f),   b.getX(), b.getBottom(), false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(b, 9.0f);

    int bh = (int)b.getHeight();
    for (int i = 0; i < bh; i += 3)
    {
        float alpha = (i % 6 == 0) ? 0.020f : 0.006f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawHorizontalLine((int)b.getY() + i, b.getX() + 2.0f, b.getRight() - 2.0f);
    }

    for (int i = 0; i < 36; ++i)
    {
        const int seed = i * 37 + pedalName.hashCode();
        const float x = b.getX() + 8.0f + (float)((seed * 17) % juce::jmax(1, (int)b.getWidth() - 16));
        const float y = b.getY() + 12.0f + (float)((seed * 29) % juce::jmax(1, (int)b.getHeight() - 28));
        const float w = 1.0f + (float)((seed >> 3) & 3) * 0.45f;
        const float a = ((seed & 1) == 0) ? 0.075f : 0.040f;
        g.setColour(((seed & 2) == 0 ? juce::Colours::white : juce::Colours::black).withAlpha(a));
        g.fillRoundedRectangle(x, y, w, 0.8f, 0.4f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRoundedRectangle(originalBody.removeFromLeft(5.0f), 7.0f);
    g.fillRoundedRectangle(b.withTrimmedLeft(b.getWidth() - 5.0f), 7.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine(b.getX() + 9.0f, b.getY() + 3.0f, b.getRight() - 9.0f, b.getY() + 3.0f, 1.0f);

    g.setColour(pedalCol.brighter(0.55f).withAlpha(0.88f));
    g.drawRoundedRectangle(b.reduced(0.5f), 9.0f, 1.4f);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(b.reduced(2.5f), 7.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.drawRoundedRectangle(b.reduced(1.5f), 8.0f, 1.5f);

    auto namePlate = b.removeFromTop(28.0f);
    auto plate = namePlate.reduced(4.0f, 3.0f);
    juce::ColourGradient plateGrad(
        juce::Colours::black.withAlpha(0.38f), plate.getX(), plate.getY(),
        juce::Colours::black.withAlpha(0.66f), plate.getX(), plate.getBottom(), false);
    g.setGradientFill(plateGrad);
    g.fillRoundedRectangle(plate, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.09f));
    g.drawRoundedRectangle(plate.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawRoundedRectangle(plate.reduced(1.5f), 3.0f, 1.0f);

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    g.drawText(pedalName, namePlate.translated(1.0f, 1.0f).toNearestInt(), juce::Justification::centred);
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawText(pedalName, namePlate.toNearestInt(), juce::Justification::centred);
    if (dragHintVisible)
    {
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawRoundedRectangle(plate.expanded(1.0f), 5.0f, 1.6f);
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText("DRAG", plate.removeFromRight(32.0f).toNearestInt(), juce::Justification::centredRight);
    }

    float sb = getLocalBounds().toFloat().reduced(3.0f).getY();
    float scrR = 4.2f;
    drawScrew(g, b.getX() + scrR + 2.0f, sb + scrR + 2.5f, scrR);
    drawScrew(g, b.getRight() - scrR - 2.0f, sb + scrR + 2.5f, scrR);

    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
    auto drawKnobLabel = [&](const juce::Slider& knob, const juce::String& label)
    {
        if (!knob.isVisible() || label.isEmpty()) return;
        auto kb = knob.getBounds();
        g.drawText(label, kb.getX(), kb.getBottom() - 2, kb.getWidth(), 12, juce::Justification::centred);
    };
    auto drawKnobValue = [&](const juce::Slider& knob, const std::function<juce::String(double)>& formatter)
    {
        if (!knob.isVisible() || !formatter)
            return;

        const auto text = formatter(knob.getValue());
        if (text.isEmpty())
            return;

        auto kb = knob.getBounds();
        g.setColour(juce::Colours::white.withAlpha(0.92f));
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText(text, kb.getX(), kb.getY() - 1, kb.getWidth(), 11, juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.setColour(juce::Colours::white.withAlpha(0.80f));
    };
    drawKnobValue(knob1, knobValueFormatters[0]);
    drawKnobValue(knob2, knobValueFormatters[1]);
    drawKnobValue(knob3, knobValueFormatters[2]);
    drawKnobValue(knob4, knobValueFormatters[3]);
    drawKnobValue(knob5, knobValueFormatters[4]);
    drawKnobValue(knob6, knobValueFormatters[5]);
    drawKnobValue(knob7, knobValueFormatters[6]);
    drawKnobValue(knob8, knobValueFormatters[7]);
    drawKnobValue(knob9, knobValueFormatters[8]);
    drawKnobLabel(knob1, k1);
    drawKnobLabel(knob2, k2);
    drawKnobLabel(knob3, k3);
    drawKnobLabel(knob4, k4);
    drawKnobLabel(knob5, k5);
    drawKnobLabel(knob6, k6);
    drawKnobLabel(knob7, k7);
    drawKnobLabel(knob8, k8);
    drawKnobLabel(knob9, k9);

    if (infoMain.isNotEmpty() || infoSub.isNotEmpty())
    {
        auto info = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(142).reduced(13, 0);
        g.setColour(juce::Colours::black.withAlpha(0.34f));
        g.fillRoundedRectangle(info.toFloat(), 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawRoundedRectangle(info.toFloat().reduced(0.5f), 5.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.96f));
        g.setFont(juce::Font(juce::FontOptions(19.0f).withStyle("Bold")));
        g.drawText(infoMain, info.removeFromTop(27), juce::Justification::centred);
        g.setColour(juce::Colours::white.withAlpha(0.76f));
        g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
        g.drawText(infoSub, info, juce::Justification::centred);

    }

    bool on = onBtn.getToggleState();
    float ledCX = (float)getWidth() * 0.5f - 27.0f;
    float ledCY = (float)getHeight() - 16.0f;
    float ledR = 5.5f;
    float footCX = (float)getWidth() * 0.5f + 22.0f;
    float footCY = (float)getHeight() - 17.0f;
    float footR = 12.0f;

    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.fillEllipse(footCX - footR - 2.0f, footCY - footR + 1.5f, (footR + 2.0f) * 2.0f, (footR + 2.0f) * 2.0f);
    juce::ColourGradient footGrad(
        juce::Colour(0xffd8d8dc), footCX - footR * 0.45f, footCY - footR * 0.55f,
        juce::Colour(0xff4a4d54), footCX + footR * 0.45f, footCY + footR * 0.55f, true);
    g.setGradientFill(footGrad);
    g.fillEllipse(footCX - footR, footCY - footR, footR * 2.0f, footR * 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawEllipse(footCX - footR, footCY - footR, footR * 2.0f, footR * 2.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.28f));
    g.fillEllipse(footCX - footR * 0.45f, footCY - footR * 0.72f, footR * 0.9f, footR * 0.35f);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f, 0.8f);

    if (on)
    {
        g.setColour(LnfC::PlayGreen.withAlpha(0.08f));
        g.fillEllipse(ledCX - ledR - 7.0f, ledCY - ledR - 7.0f, (ledR + 7.0f) * 2, (ledR + 7.0f) * 2);
        g.setColour(LnfC::PlayGreen.withAlpha(0.20f));
        g.fillEllipse(ledCX - ledR - 3.5f, ledCY - ledR - 3.5f, (ledR + 3.5f) * 2, (ledR + 3.5f) * 2);
        g.setColour(LnfC::PlayGreen.withAlpha(0.45f));
        g.fillEllipse(ledCX - ledR - 1.0f, ledCY - ledR - 1.0f, (ledR + 1.0f) * 2, (ledR + 1.0f) * 2);
        juce::ColourGradient ledGrad(
            LnfC::PlayGreen.brighter(0.5f), ledCX - ledR * 0.3f, ledCY - ledR * 0.5f,
            LnfC::PlayGreen.darker(0.2f),   ledCX + ledR * 0.2f, ledCY + ledR * 0.3f, true);
        g.setGradientFill(ledGrad);
        g.fillEllipse(ledCX - ledR, ledCY - ledR, ledR * 2, ledR * 2);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillEllipse(ledCX - ledR * 0.45f, ledCY - ledR * 0.72f, ledR * 0.6f, ledR * 0.38f);
    }
    else
    {
        g.setColour(juce::Colour(0xff112218));
        g.fillEllipse(ledCX - ledR, ledCY - ledR, ledR * 2, ledR * 2);
        g.setColour(juce::Colour(0xff1a3322));
        g.drawEllipse(ledCX - ledR, ledCY - ledR, ledR * 2, ledR * 2, 0.8f);
    }

    g.setColour(on ? juce::Colours::white : LnfC::TextDim.withAlpha(0.60f));
    g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
    g.drawText("ON", (int)(ledCX + ledR + 3.0f), (int)(ledCY - 6.0f), 22, 13, juce::Justification::centredLeft);

    auto drawJack = [&](float jx, float jy)
    {
        g.setColour(juce::Colours::black.withAlpha(0.80f));
        g.fillEllipse(jx - 8.0f, jy - 8.0f, 16.0f, 16.0f);
        juce::ColourGradient jGrad(
            juce::Colour(0xff666677), jx - 5.0f, jy - 5.0f,
            juce::Colour(0xff2a2a36), jx + 3.0f, jy + 4.0f, true);
        g.setGradientFill(jGrad);
        g.drawEllipse(jx - 7.5f, jy - 7.5f, 15.0f, 15.0f, 2.5f);
        g.setColour(juce::Colour(0xff080810));
        g.fillEllipse(jx - 3.5f, jy - 3.5f, 7.0f, 7.0f);
        g.setColour(juce::Colour(0xff444455));
        g.drawEllipse(jx - 3.5f, jy - 3.5f, 7.0f, 7.0f, 0.8f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillEllipse(jx - 5.5f, jy - 7.0f, 4.5f, 2.5f);
    };
    float jackY = (float)getHeight() * 0.38f;
    drawJack(4.5f, jackY);
    drawJack((float)getWidth() - 4.5f, jackY);
}

void FXPedalComponent::resized()
{
    auto b = getLocalBounds().reduced(5);
    onBtn.setBounds(b.getX(), getHeight() - 24, b.getWidth(), 20);
    onBtn.setVisible(false);

    if (extendedKnobs)
    {
        const int knobZoneTop = 34;
        const int knobZoneBot = getHeight() - 38;
        const int rowGap = 14;
        const int labelPad = 12;
        const int rowH = (knobZoneBot - knobZoneTop - rowGap) / 2;
        const int thirdW = (b.getWidth() - 8) / 3;
        const int secondRowY = knobZoneTop + rowH + rowGap;

        knob1.setBounds(b.getX(), knobZoneTop, thirdW, rowH - labelPad);
        knob2.setBounds(b.getX() + thirdW + 4, knobZoneTop, thirdW, rowH - labelPad);
        knob3.setBounds(b.getX() + (thirdW + 4) * 2, knobZoneTop, thirdW, rowH - labelPad);

        knob4.setBounds(b.getX(), secondRowY, thirdW, rowH - labelPad);
        knob5.setBounds(b.getX() + thirdW + 4, secondRowY, thirdW, rowH - labelPad);
        knob6.setBounds(b.getX() + (thirdW + 4) * 2, secondRowY, thirdW, rowH - labelPad);
        return;
    }

    if (nineKnobs)
    {
        const int knobZoneTop = 34;
        const int knobZoneBot = getHeight() - 34;
        const int rowGap = 5;
        const int labelPad = 10;
        const int availableH = juce::jmax(72, knobZoneBot - knobZoneTop);
        const int rowH = juce::jmax(24, (availableH - rowGap * 2) / 3);
        const int knobH = juce::jmax(18, rowH - labelPad);
        const int thirdW = juce::jmax(24, (b.getWidth() - 8) / 3);
        const int row2Y = knobZoneTop + rowH + rowGap;
        const int row3Y = row2Y + rowH + rowGap;

        knob1.setBounds(b.getX(), knobZoneTop, thirdW, knobH);
        knob2.setBounds(b.getX() + thirdW + 4, knobZoneTop, thirdW, knobH);
        knob3.setBounds(b.getX() + (thirdW + 4) * 2, knobZoneTop, thirdW, knobH);

        knob4.setBounds(b.getX(), row2Y, thirdW, knobH);
        knob5.setBounds(b.getX() + thirdW + 4, row2Y, thirdW, knobH);
        knob6.setBounds(b.getX() + (thirdW + 4) * 2, row2Y, thirdW, knobH);

        knob7.setBounds(b.getX(), row3Y, thirdW, knobH);
        knob8.setBounds(b.getX() + thirdW + 4, row3Y, thirdW, knobH);
        knob9.setBounds(b.getX() + (thirdW + 4) * 2, row3Y, thirdW, knobH);
        return;
    }

    int knobZoneTop = 34;
    int knobZoneBot = getHeight() - 42;
    int knobZoneH = knobZoneBot - knobZoneTop;

    int topH = (int)(knobZoneH * 0.55f);
    int halfW = (b.getWidth() - 4) / 2;
    knob1.setBounds(b.getX(), knobZoneTop, halfW, topH);
    knob2.setBounds(b.getX() + halfW + 2, knobZoneTop, halfW, topH);

    int botY = knobZoneTop + topH + 16;
    int botH = knobZoneBot - botY;
    int botW = (int)(b.getWidth() * 0.62f);
    int botX = b.getX() + (b.getWidth() - botW) / 2;
    knob3.setBounds(botX, botY, botW, botH);
    knob4.setBounds(0, 0, 0, 0);
    knob5.setBounds(0, 0, 0, 0);
    knob6.setBounds(0, 0, 0, 0);
    knob7.setBounds(0, 0, 0, 0);
    knob8.setBounds(0, 0, 0, 0);
    knob9.setBounds(0, 0, 0, 0);
}
