#include "LoopernicoWidgets.h"

using namespace LoopernicoUi;

void MasterVUMeter::drawBar(juce::Graphics& g, juce::Rectangle<float> bar, float level)
{
    g.setColour(LnfC::BG.darker(0.3f));
    g.fillRoundedRectangle(bar, 2.0f);

    const float db = (level > 0.00001f) ? 20.0f * std::log10(level) : -96.0f;
    const float mapped = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 54.0f);

    const int N = 18;
    const float segW = bar.getWidth() / (float)N;
    const int litSegments = juce::jlimit(0, N, juce::roundToInt(mapped * (float)N));

    for (int i = 0; i < N; ++i)
    {
        const float left = bar.getX() + (float)i * segW;
        const bool lit = i < litSegments;

        juce::Colour col = (i < 10) ? LnfC::PlayGreen
                         : (i < 14) ? juce::Colour(0xffbbcc00)
                         : (i < 16) ? LnfC::OvdAmber
                                    : LnfC::RecRed;

        if (lit)
        {
            const bool isHead = i == litSegments - 1;
            g.setColour(col.withAlpha(isHead ? 0.22f : 0.07f));
            g.fillRoundedRectangle(left - 0.6f, bar.getY() - 0.6f,
                                   segW + 0.5f, bar.getHeight() + 1.2f, 1.8f);
            g.setColour(isHead ? col.brighter(0.5f) : col);
            g.fillRoundedRectangle(left + 0.7f, bar.getY() + 1.0f,
                                   juce::jmax(1.0f, segW - 1.5f), bar.getHeight() - 2.0f, 1.0f);

            if (isHead)
            {
                g.setColour(col.withAlpha(0.30f));
                g.fillRoundedRectangle(left - 0.7f, bar.getY() - 0.8f,
                                       segW + 0.7f, bar.getHeight() + 1.6f, 1.8f);
            }
        }
        else
        {
            g.setColour(col.withAlpha(0.08f));
            g.fillRoundedRectangle(left + 0.7f, bar.getY() + 1.0f,
                                   juce::jmax(1.0f, segW - 1.5f), bar.getHeight() - 2.0f, 1.0f);
        }
    }

    const float zeroDbX = bar.getX() + bar.getWidth() * (48.0f / 54.0f);
    g.setColour(juce::Colours::white.withAlpha(0.22f));
    g.drawVerticalLine(juce::roundToInt(zeroDbX), bar.getY(), bar.getBottom());
}

void MasterVUMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    constexpr float labelW = 12.0f;
    constexpr float rowGap = 3.0f;
    const float rowH = juce::jmax(6.0f, (b.getHeight() - rowGap) * 0.5f);
    const auto barL = juce::Rectangle<float>(b.getX() + labelW, b.getY(),
                                             b.getWidth() - labelW, rowH);
    const auto barR = juce::Rectangle<float>(b.getX() + labelW, b.getY() + rowH + rowGap,
                                             b.getWidth() - labelW, rowH);

    drawBar(g, barL, levelL);
    drawBar(g, barR, levelR);
    g.setColour(LnfC::TextDim);
    g.setFont(juce::Font(juce::FontOptions(7.5f).withStyle("Bold")));
    g.drawText("L", juce::Rectangle<float>(b.getX(), b.getY(), labelW - 2.0f, rowH).toNearestInt(),
               juce::Justification::centred);
    g.drawText("R", juce::Rectangle<float>(b.getX(), b.getY() + rowH + rowGap,
                                            labelW - 2.0f, rowH).toNearestInt(),
               juce::Justification::centred);
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
            g.setColour(col.withAlpha(isTop ? 0.22f : 0.065f));
            g.fillRoundedRectangle(b.getX() - 0.5f, top - 0.6f,
                                   b.getWidth() + 1.0f, segH + 0.3f, 2.0f);
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

    const bool hasWaveform = thumbnail != nullptr
        && thumbnail->getNumChannels() > 0
        && thumbnail->getTotalLength() > 0.0;

    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(b.reduced(2.0f, 3.0f).translated(0.0f, 2.0f), 6.0f);
    juce::ColourGradient leftPanel(juce::Colour(0xff17191c), leftArea.getX(), leftArea.getY(),
                                   juce::Colour(0xff090a0c), leftArea.getRight(), leftArea.getBottom(), true);
    g.setGradientFill(leftPanel);
    g.fillRoundedRectangle(leftArea.reduced(3.0f, 4.0f), 5.0f);
    g.setColour(juce::Colours::white.withAlpha(0.045f));
    g.drawRoundedRectangle(leftArea.reduced(3.5f, 4.5f), 5.0f, 1.0f);

    auto wavePanel = waveArea.reduced(3.0f, 5.0f);
    g.setColour(juce::Colour(0xff020304));
    g.fillRoundedRectangle(wavePanel, 4.0f);

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

    const float stateLedX = 132.0f;
    const float stateLedY = 22.0f;
    const bool stateActive = isRec || isOvd || isPlay;
    drawLedLens(g, stateLedX, stateLedY, 3.6f, stateC, stateActive);

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
    auto waveRect = juce::Rectangle<int>(wx, wy, ww, wh).reduced(7, 8);

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
    : pedalName(name), displayName(name), k1(l1), k2(l2), k3(l3), pedalCol(col)
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

    auto setupEqSlider = [this](juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange(-12.0, 12.0, 0.1);
        slider.setValue(0.0);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setComponentID("mxrTenBandSlider");
        slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff18506b));
        addAndMakeVisible(slider);
        slider.setVisible(false);
    };
    setupEqSlider(eqVolumeSlider);
    for (auto& slider : eqBandSliders)
        setupEqSlider(slider);
    setupEqSlider(eqGainSlider);

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

void FXPedalComponent::configureTenBandGraphicEq()
{
    extendedKnobs = false;
    nineKnobs = false;
    tenBandGraphicEq = true;
    knob1.setVisible(false); knob2.setVisible(false); knob3.setVisible(false);
    knob4.setVisible(false); knob5.setVisible(false); knob6.setVisible(false);
    knob7.setVisible(false); knob8.setVisible(false); knob9.setVisible(false);
    eqVolumeSlider.setVisible(true);
    for (auto& slider : eqBandSliders)
        slider.setVisible(true);
    eqGainSlider.setVisible(true);
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

juce::Point<float> FXPedalComponent::getJackCentre(JackSide side) const noexcept
{
    return { side == JackSide::input ? (float)getWidth() - 4.5f : 4.5f,
             (float)getHeight() * 0.38f };
}

namespace
{
constexpr float kCompactPedalHeight = 236.0f;
// The supplied artwork includes a wide black/transparent presentation border.
// Render the actual pedal larger inside its compact lane while keeping the
// source PNG untouched.
constexpr float kGamechangerPlusFaceAspect = 0.45f;
constexpr float kBigSkyFaceAspect = 1536.0f / 1024.0f;
// Source crop and measured OLED bounds inside strymon-bigsky-mx-face-v3.png.
// The screen coordinates must be relative to the crop drawn into the pedal,
// rather than to the complete 1536x1024 photograph.
constexpr float kBigSkyCropX = 103.0f;
constexpr float kBigSkyCropY = 56.0f;
constexpr float kBigSkyCropW = 1330.0f;
constexpr float kBigSkyCropH = 916.0f;
constexpr float kBigSkyScreenX = (659.0f - kBigSkyCropX) / kBigSkyCropW;
constexpr float kBigSkyScreenY = (175.0f - kBigSkyCropY) / kBigSkyCropH;
constexpr float kBigSkyScreenW = 422.0f / kBigSkyCropW;
constexpr float kBigSkyScreenH = 117.0f / kBigSkyCropH;
constexpr float kTimelineFaceAspect = 1536.0f / 1024.0f;
// getPedalTextureCrop() draws this exact rounded 1 % inset from
// strymon-timeline-mx-face-v2.png. All measured hardware coordinates below
// are expressed in full-PNG pixels and converted to the crop's coordinate
// space here, so paint() and resized() cannot mix the two systems.
constexpr float kTimelineCropX = 15.0f;
constexpr float kTimelineCropY = 10.0f;
constexpr float kTimelineCropW = 1505.0f;
constexpr float kTimelineCropH = 1004.0f;
constexpr float timelineX(float pngX) noexcept { return (pngX - kTimelineCropX) / kTimelineCropW; }
constexpr float timelineY(float pngY) noexcept { return (pngY - kTimelineCropY) / kTimelineCropH; }
constexpr float kTimelineScreenX = timelineX(649.0f);
constexpr float kTimelineScreenY = timelineY(133.0f);
constexpr float kTimelineScreenW = 479.0f / kTimelineCropW;
constexpr float kTimelineScreenH = 126.0f / kTimelineCropH;
constexpr float kBeatBuddyScreenX = 0.090f;
constexpr float kBeatBuddyScreenY = 0.285f;
constexpr float kBeatBuddyScreenW = 0.820f;
constexpr float kBeatBuddyScreenH = 0.212f;

float getPedalHardwareScale(const juce::Component& pedal)
{
    return juce::jmax(0.1f, (float) pedal.getHeight() / kCompactPedalHeight);
}

juce::Image loadPedalTexture(const juce::String& fileName)
{
    const auto sourceDirectory = juce::File(__FILE__).getParentDirectory();
    const juce::File candidates[] = {
        sourceDirectory.getSiblingFile("Assets").getChildFile(fileName),
        juce::File::getCurrentWorkingDirectory().getChildFile("Assets").getChildFile(fileName),
        juce::File::getCurrentWorkingDirectory()
            .getChildFile("Loopernico Refactorizado/Assets").getChildFile(fileName)
    };
    for (const auto& file : candidates)
        if (file.existsAsFile())
            if (auto image = juce::ImageFileFormat::loadFrom(file); image.isValid())
                return image;
    return {};
}

const juce::Image& getMetalFootswitchImage()
{
    static const auto footswitch = loadPedalTexture("Hardware/footswitch-metal-v3.png");
    return footswitch;
}

void drawMetalFootswitch(juce::Graphics& g, float cx, float cy, float size, bool pressed)
{
    const auto& footswitch = getMetalFootswitchImage();
    if (!footswitch.isValid())
        return;

    const float pressOffset = pressed ? size * 0.0375f : 0.0f;
    const float renderedSize = pressed ? size * 0.97f : size;
    const auto target = juce::Rectangle<float>(renderedSize, renderedSize)
                            .withCentre({ cx, cy + pressOffset });

    juce::Graphics::ScopedSaveState state(g);
    const float height = pressed ? 0.35f : 1.0f;
    const auto shadowCentre = juce::Point<float>(cx + size * 0.045f * height,
                                                  cy + size * 0.075f * height);
    const float shadowRadius = size * (pressed ? 0.36f : 0.43f);
    juce::ColourGradient shadow(juce::Colours::black.withAlpha(pressed ? 0.20f : 0.42f),
                                shadowCentre.x, shadowCentre.y,
                                juce::Colours::transparentBlack,
                                shadowCentre.x + shadowRadius, shadowCentre.y, true);
    shadow.addColour(0.62, juce::Colours::black.withAlpha(pressed ? 0.12f : 0.27f));
    g.setGradientFill(shadow);
    g.fillEllipse(shadowCentre.x - shadowRadius, shadowCentre.y - shadowRadius,
                  shadowRadius * 2.0f, shadowRadius * 2.0f);

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.setOpacity(pressed ? 0.92f : 1.0f);
    g.drawImage(footswitch, target);
}

void drawCooltronLedLens(juce::Graphics& g, float cx, float cy, float r, bool on)
{
    // The real Cooltron uses a small, deep-red lens. Keep the light contained
    // inside its bezel: no synthetic exterior halo or saturated scarlet disc.
    g.setColour(juce::Colours::black.withAlpha(0.52f));
    g.fillEllipse(cx - r - 1.6f, cy - r - 0.8f, r * 2.0f + 3.2f, r * 2.0f + 3.2f);

    juce::ColourGradient bezel(juce::Colour(0xff887c76), cx - r * 0.72f, cy - r * 0.78f,
                                juce::Colour(0xff211b1a), cx + r * 0.72f, cy + r * 0.78f, true);
    bezel.addColour(0.55, juce::Colour(0xff514744));
    g.setGradientFill(bezel);
    g.fillEllipse(cx - r - 0.9f, cy - r - 0.9f, r * 2.0f + 1.8f, r * 2.0f + 1.8f);

    const auto lensTop = on ? juce::Colour(0xffd05a50) : juce::Colour(0xff50181a);
    const auto lensMid = on ? juce::Colour(0xffa51f24) : juce::Colour(0xff321012);
    const auto lensBottom = on ? juce::Colour(0xff5b080d) : juce::Colour(0xff160506);
    juce::ColourGradient lens(lensTop, cx - r * 0.38f, cy - r * 0.62f,
                              lensBottom, cx + r * 0.42f, cy + r * 0.58f, true);
    lens.addColour(0.48, lensMid);
    g.setGradientFill(lens);
    g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour(juce::Colour(0xff160507).withAlpha(0.72f));
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.72f);
    g.setColour(juce::Colour(0xffffe1d4).withAlpha(on ? 0.34f : 0.13f));
    g.fillEllipse(cx - r * 0.48f, cy - r * 0.62f, r * 0.56f, r * 0.28f);
    g.setColour(juce::Colour(0xffffb2a4).withAlpha(on ? 0.12f : 0.03f));
    g.fillEllipse(cx - r * 0.64f, cy - r * 0.70f, r * 1.28f, r * 0.64f);
}

const juce::Image& getPedalEnamelTexture(const juce::String& name)
{
    static const auto noiseGate = loadPedalTexture("Pedals/boss-ns2-noise-suppressor-face-v8.png");
    static const auto pampaFuzz = loadPedalTexture("Pedals/pampa-fuzz-face-v8.png");
    static const auto voxCooltron = loadPedalTexture("Pedals/vox-cooltron-duel-overdrive-face-v1.png");
    static const auto dedaloJet = loadPedalTexture("Pedals/dedalo-jet-compressor-face-v4.png");
    static const auto dedaloTres = loadPedalTexture("Pedals/dedalo-tres-tremolo-face-v1.png");
    static const auto harmonizer = loadPedalTexture("Pedals/ehx-pico-pitch-fork-face-v4.png");
    static const auto gamechangerPlus = loadPedalTexture("Pedals/gamechanger-plus-pedal-ii-face-v1.png");
    static const auto modelHost = loadPedalTexture("Pedals/model-host-face-v3.png");
    static const auto tenBandEq = loadPedalTexture("Pedals/ten-band-eq-aluminum-face-v1.png");
    static const auto beatBuddy = loadPedalTexture("Pedals/singular-sound-beatbuddy-face-v4.png");
    static const auto smallClone = loadPedalTexture("Pedals/electro-harmonix-small-clone-face-v3.png");
    static const auto bigSky = loadPedalTexture("Pedals/strymon-bigsky-mx-face-v3.png");
    static const auto timeLine = loadPedalTexture("Pedals/strymon-timeline-mx-face-v2.png");
    static const auto neutral = loadPedalTexture("Textures/pedal-enamel-neutral-v1.png");
    static const auto silver  = loadPedalTexture("Textures/pedal-brushed-silver-v1.png");
    static const auto black   = loadPedalTexture("Textures/pedal-black-powder-v1.png");
    static const auto lacquer = loadPedalTexture("Textures/pedal-blue-lacquer-v1.png");

    if (name.containsIgnoreCase("NOISE GATE"))
        return noiseGate;
    if (name == "RHYTHM BOX")
        return beatBuddy;
    if (name == "FUZZ")
        return pampaFuzz;
    if (name == "SUNSET")
        return voxCooltron;
    if (name == "COMPRESSOR")
        return dedaloJet;
    if (name == "TREMOLO")
        return dedaloTres;
    if (name == "HARMONIZER")
        return harmonizer;
    if (name == "SUSTAIN CHORDS")
        return gamechangerPlus;
    if (name == "VST3 HOST")
        return modelHost;
    if (name == "OUTPUT EQ")
        return tenBandEq;
    if (name == "CHORUS")
        return smallClone;
    if (name == "ROOM REVERB")
        return bigSky;
    if (name == "TAPE DELAY")
        return timeLine;
    if (name.containsIgnoreCase("COMPRESSOR") || name.containsIgnoreCase("OUTPUT EQ"))
        return silver;
    if (name.containsIgnoreCase("FUZZ") || name.containsIgnoreCase("DELAY"))
        return black;
    if (name.containsIgnoreCase("CHORUS") || name.containsIgnoreCase("TREMOLO")
        || name.containsIgnoreCase("REVERB") || name.containsIgnoreCase("SUSTAIN"))
        return lacquer;
    return neutral;
}

juce::Rectangle<int> getPedalTextureCrop(const juce::String& name, const juce::Image& image)
{
    const int w = image.getWidth();
    const int h = image.getHeight();
    if (name.containsIgnoreCase("NOISE GATE"))
        return image.getBounds();
    if (name == "RHYTHM BOX")
        return { juce::roundToInt(w * 0.068f), juce::roundToInt(h * 0.052f),
                 juce::roundToInt(w * 0.864f), juce::roundToInt(h * 0.856f) };
    if (name == "FUZZ")
        return image.getBounds();
    if (name == "SUNSET")
        return image.getBounds();
    if (name == "COMPRESSOR")
        return { juce::roundToInt(w * 0.015f), juce::roundToInt(h * 0.015f),
                 juce::roundToInt(w * 0.97f), juce::roundToInt(h * 0.97f) };
    if (name == "TREMOLO")
        return { juce::roundToInt(w * 0.012f), juce::roundToInt(h * 0.012f),
                 juce::roundToInt(w * 0.976f), juce::roundToInt(h * 0.976f) };
    if (name == "HARMONIZER")
        return image.getBounds();
    if (name == "SUSTAIN CHORDS")
        return { juce::roundToInt(w * 0.205f), juce::roundToInt(h * 0.045f),
                 juce::roundToInt(w * 0.610f), juce::roundToInt(h * 0.905f) };
    if (name == "VST3 HOST")
        return { juce::roundToInt(w * 0.035f), juce::roundToInt(h * 0.118f),
                 juce::roundToInt(w * 0.93f), juce::roundToInt(h * 0.74f) };
    if (name == "OUTPUT EQ")
        return image.getBounds();
    if (name == "CHORUS")
        return { juce::roundToInt(w * 0.035f), juce::roundToInt(h * 0.008f),
                 juce::roundToInt(w * 0.930f), juce::roundToInt(h * 0.970f) };
    if (name == "ROOM REVERB")
        return { juce::roundToInt(kBigSkyCropX), juce::roundToInt(kBigSkyCropY),
                 juce::roundToInt(kBigSkyCropW), juce::roundToInt(kBigSkyCropH) };
    if (name == "TAPE DELAY")
        return { juce::roundToInt(kTimelineCropX), juce::roundToInt(kTimelineCropY),
                 juce::roundToInt(kTimelineCropW), juce::roundToInt(kTimelineCropH) };
    if (name.containsIgnoreCase("COMPRESSOR") || name.containsIgnoreCase("OUTPUT EQ"))
        return { juce::roundToInt(w * 0.22f), juce::roundToInt(h * 0.045f),
                 juce::roundToInt(w * 0.56f), juce::roundToInt(h * 0.91f) };
    if (name.containsIgnoreCase("FUZZ") || name.containsIgnoreCase("DELAY"))
        return { juce::roundToInt(w * 0.10f), juce::roundToInt(h * 0.07f),
                 juce::roundToInt(w * 0.80f), juce::roundToInt(h * 0.86f) };
    if (name.containsIgnoreCase("CHORUS") || name.containsIgnoreCase("TREMOLO")
        || name.containsIgnoreCase("REVERB") || name.containsIgnoreCase("SUSTAIN"))
        return { juce::roundToInt(w * 0.13f), juce::roundToInt(h * 0.055f),
                 juce::roundToInt(w * 0.74f), juce::roundToInt(h * 0.89f) };
    return image.getBounds().reduced(8);
}

float getPedalTextureOpacity(const juce::String& name)
{
    if (name == "FUZZ")
        return 0.96f;
    if (name == "SUNSET")
        return 1.0f;
    if (name == "COMPRESSOR")
        return 0.98f;
    if (name == "TREMOLO")
        return 0.99f;
    if (name == "HARMONIZER")
        return 0.99f;
    if (name == "SUSTAIN CHORDS")
        return 1.0f;
    if (name == "VST3 HOST")
        return 0.99f;
    if (name == "OUTPUT EQ")
        return 0.96f;
    if (name == "ROOM REVERB")
        return 0.99f;
    if (name == "TAPE DELAY")
        return 1.0f;
    if (name == "CHORUS")
        return 1.0f;
    if (name == "RHYTHM BOX")
        return 1.0f;
    return name.containsIgnoreCase("NOISE GATE") ? 1.0f : 0.24f;
}

void drawCablePlug(juce::Graphics& g, juce::Point<float> p, float direction)
{
    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.fillRoundedRectangle(p.x - 13.0f * direction - (direction < 0 ? 10.0f : 0.0f),
                           p.y - 5.5f, 23.0f, 11.0f, 4.0f);
    juce::Rectangle<float> sleeve(direction > 0 ? p.x : p.x - 12.0f, p.y - 4.0f, 12.0f, 8.0f);
    juce::ColourGradient metal(juce::Colour(0xffd5d1c6), sleeve.getX(), sleeve.getY(),
                               juce::Colour(0xff494a4e), sleeve.getX(), sleeve.getBottom(), false);
    g.setGradientFill(metal);
    g.fillRoundedRectangle(sleeve, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawLine(sleeve.getX(), sleeve.getY() + 1.0f, sleeve.getRight(), sleeve.getY() + 1.0f, 0.7f);
}
}

void FXPedalboardWiring::drawCable(juce::Graphics& g, const juce::Component& coordinateSpace,
                                   const FXPedalComponent& from, const FXPedalComponent& to,
                                   CableStyle style)
{
    auto a = coordinateSpace.getLocalPoint(&from, from.getJackCentre(FXPedalComponent::JackSide::input));
    auto b = coordinateSpace.getLocalPoint(&to, to.getJackCentre(FXPedalComponent::JackSide::output));
    if (a.x > b.x) std::swap(a, b);

    const float dx = juce::jmax(18.0f, (b.x - a.x) * 0.36f);
    juce::Path cable;
    cable.startNewSubPath(a);
    cable.cubicTo(a.x + dx, a.y + style.sag, b.x - dx, b.y + style.sag, b.x, b.y);
    auto cableShadow = cable;
    cableShadow.applyTransform(juce::AffineTransform::translation(0.0f, 4.0f));
    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.strokePath(cableShadow, juce::PathStrokeType(style.thickness + 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colours::black.withAlpha(0.82f));
    g.strokePath(cable, juce::PathStrokeType(style.thickness + 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(style.sheath);
    g.strokePath(cable, juce::PathStrokeType(style.thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colours::white.withAlpha(0.075f));
    g.strokePath(cable, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawCablePlug(g, a, -1.0f);
    drawCablePlug(g, b, 1.0f);
}

void FXPedalboardWiring::drawChain(juce::Graphics& g, const juce::Component& coordinateSpace,
                                   const juce::Array<FXPedalComponent*>& pedals, CableStyle style)
{
    for (int i = 1; i < pedals.size(); ++i)
        if (pedals[i - 1] != nullptr && pedals[i] != nullptr)
            drawCable(g, coordinateSpace, *pedals[i - 1], *pedals[i], style);
}

// The pedal rendering remains intentionally detailed because it defines the hardware-like identity of the UI.
void FXPedalComponent::paint(juce::Graphics& g)
{
    const float hardwareScale = getPedalHardwareScale(*this);
    auto b = getLocalBounds().toFloat().reduced(3.0f);
    const bool wideFuzzArtwork = pedalName == "FUZZ";
    const bool voxCooltronArtwork = pedalName == "SUNSET"; // Historical session ID.
    const bool dedaloJetArtwork = pedalName == "COMPRESSOR";
    const bool dedaloTresArtwork = pedalName == "TREMOLO";
    const bool harmonizerArtwork = pedalName == "HARMONIZER";
    const bool sustainChordsArtwork = pedalName == "SUSTAIN CHORDS";
    const bool plusPedalArtwork = sustainChordsArtwork;
    const bool beatBuddyArtwork = pedalName == "RHYTHM BOX";
    const bool modelHostArtwork = pedalName == "VST3 HOST";
    const bool tenBandEqArtwork = pedalName == "OUTPUT EQ";
    const bool bigSkyArtwork = pedalName == "ROOM REVERB";
    const bool timeLineArtwork = pedalName == "TAPE DELAY";
    const bool smallCloneArtwork = pedalName == "CHORUS";
    const bool bossNoiseArtwork = pedalName.containsIgnoreCase("NOISE GATE");
    const bool customArtwork = wideFuzzArtwork || voxCooltronArtwork || dedaloJetArtwork || dedaloTresArtwork || harmonizerArtwork
                            || sustainChordsArtwork || bossNoiseArtwork
                            || beatBuddyArtwork || modelHostArtwork || tenBandEqArtwork || bigSkyArtwork || timeLineArtwork
                            || smallCloneArtwork;
    if (customArtwork)
    {
        const float aspect = voxCooltronArtwork ? (2000.0f / 1874.0f)
                           : (smallCloneArtwork ? (3.38f / 5.25f)
                           : (wideFuzzArtwork ? (977.0f / 1610.0f)
                           : (timeLineArtwork ? kTimelineFaceAspect
                           : (bigSkyArtwork ? kBigSkyFaceAspect
                           : (dedaloJetArtwork ? 1.23f
                           : (dedaloTresArtwork ? (4.0f / 3.0f)
                           : (harmonizerArtwork ? (976.0f / 1781.0f)
                            : (plusPedalArtwork ? kGamechangerPlusFaceAspect
                            : (beatBuddyArtwork ? 0.72f
                           : (bossNoiseArtwork ? (976.0f / 1612.0f)
                           : (modelHostArtwork ? 1.20f : (tenBandEqArtwork ? (124.0f / 92.0f) : 1.08f))))))))))));
        const float bodyHeight = juce::jmin(b.getHeight(), b.getWidth() / aspect);
        const float bodyWidth = (beatBuddyArtwork || bossNoiseArtwork || wideFuzzArtwork
                                 || harmonizerArtwork || smallCloneArtwork
                                 || plusPedalArtwork || bigSkyArtwork || timeLineArtwork)
                                    ? juce::jmin(b.getWidth(), bodyHeight * aspect)
                                    : b.getWidth();
        b = juce::Rectangle<float>(b.getCentreX() - bodyWidth * 0.5f,
                                   b.getCentreY() - bodyHeight * 0.5f,
                                   bodyWidth, bodyHeight);
    }
    auto originalBody = b;

    const auto enamel = pedalCol.withMultipliedSaturation(0.68f).darker(0.08f);
    if (!bossNoiseArtwork && !beatBuddyArtwork && !plusPedalArtwork)
    {
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(b.translated(0.0f, 7.0f).expanded(4.0f, 2.0f), 12.0f);
        g.setColour(juce::Colours::black.withAlpha(0.62f));
        g.fillRoundedRectangle(b.translated(0.0f, 3.0f).expanded(1.5f), 10.0f);

        juce::ColourGradient bodyGrad(
            enamel.brighter(0.10f), b.getX(), b.getY(),
            enamel.darker(0.58f),   b.getX(), b.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(b, 9.0f);
    }

    if (const auto& enamelTexture = getPedalEnamelTexture(pedalName); enamelTexture.isValid())
    {
        juce::Graphics::ScopedSaveState state(g);
        if (pedalName == "OUTPUT EQ")
        {
            juce::Path roundedClip;
            roundedClip.addRoundedRectangle(b, 10.0f);
            g.reduceClipRegion(roundedClip);
        }
        else
        {
            g.reduceClipRegion(b.toNearestInt());
        }
        g.setOpacity(getPedalTextureOpacity(pedalName));
        const auto crop = getPedalTextureCrop(pedalName, enamelTexture);
        const auto target = b.toNearestInt();
        g.drawImage(enamelTexture,
                    target.getX(), target.getY(), target.getWidth(), target.getHeight(),
                    crop.getX(), crop.getY(), crop.getWidth(), crop.getHeight());
    }

    if (!bossNoiseArtwork && !beatBuddyArtwork && !plusPedalArtwork)
    {
        // A cast aluminium enclosure: rolled top lip, darker side walls and a subtle centre sheen.
        g.setColour(enamel.brighter(0.42f).withAlpha(0.18f));
        g.fillRoundedRectangle(b.reduced(2.0f).withHeight(7.0f), 5.0f);
        juce::ColourGradient centreSheen(juce::Colours::white.withAlpha(0.10f), b.getCentreX(), b.getY(),
                                         juce::Colours::transparentWhite, b.getRight(), b.getY(), false);
        g.setGradientFill(centreSheen);
        g.fillRoundedRectangle(b.reduced(6.0f, 3.0f), 6.0f);

    juce::ColourGradient directionalLight(juce::Colours::white.withAlpha(0.12f), b.getX(), b.getY(),
                                           juce::Colours::transparentWhite, b.getCentreX(), b.getCentreY(), true);
    g.setGradientFill(directionalLight);
    g.fillRoundedRectangle(b.reduced(3.0f), 7.0f);

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

    g.setColour(enamel.brighter(0.45f).withAlpha(0.64f));
    g.drawRoundedRectangle(b.reduced(0.5f), 9.0f, 1.4f);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(b.reduced(2.5f), 7.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.drawRoundedRectangle(b.reduced(1.5f), 8.0f, 1.5f);
    }

    juce::Rectangle<float> plate;
    if (!customArtwork)
    {
        auto namePlate = b.removeFromTop(28.0f);
        plate = namePlate.reduced(4.0f, 3.0f);
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
    }
    if (modelHostArtwork)
    {
        const auto cyan = juce::Colour(0xff55dce5);
        const auto amber = juce::Colour(0xffe3a24a);

        const float railY = b.getY() + b.getHeight() * 0.505f;
        juce::ColourGradient rail(cyan.withAlpha(0.82f), b.getX() + 28.0f, railY,
                                  amber.withAlpha(0.78f), b.getRight() - 28.0f, railY, false);
        g.setGradientFill(rail);
        g.fillRoundedRectangle(b.getX() + 30.0f, railY, b.getWidth() - 60.0f, 1.2f, 0.6f);

    }

    if (bossNoiseArtwork)
    {
        const auto red = juce::Colour(0xffd9202a);
        const bool active = onBtn.getToggleState();
        if (active)
        {
            drawLedLens(g, b.getX() + b.getWidth() * 0.510f,
                        b.getY() + b.getHeight() * 0.065f, 2.8f * hardwareScale, red, true);
            drawLedLens(g, b.getX() + b.getWidth() * 0.188f,
                        b.getY() + b.getHeight() * 0.170f, 2.5f * hardwareScale, red, true);
        }
    }

    if (voxCooltronArtwork)
    {
        const bool aOn = onBtn.getToggleState();
        const bool bOn = tertiaryFootswitchLatched;
        drawCooltronLedLens(g, b.getX() + b.getWidth() * 0.248f,
                            b.getY() + b.getHeight() * 0.682f, 3.5f * hardwareScale, aOn);
        drawCooltronLedLens(g, b.getX() + b.getWidth() * 0.742f,
                            b.getY() + b.getHeight() * 0.682f, 3.5f * hardwareScale, bOn);

    }

    if (beatBuddyArtwork)
    {
        const bool rhythmOn = onBtn.getToggleState();
        const float screenY = b.getY() + b.getHeight() * kBeatBuddyScreenY;
        const float screenH = b.getHeight() * kBeatBuddyScreenH;
        auto screenOuter = juce::Rectangle<float>(b.getX() + b.getWidth() * kBeatBuddyScreenX, screenY,
                                                   b.getWidth() * kBeatBuddyScreenW, screenH);
        const float screenScale = hardwareScale;
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(screenOuter.translated(0.0f, 1.5f * screenScale).expanded(1.2f * screenScale),
                               3.5f * screenScale);
        g.setColour(juce::Colour(0xff050708));
        g.fillRoundedRectangle(screenOuter, 2.5f * screenScale);
        juce::ColourGradient glass(rhythmOn ? juce::Colour(0xff24482d) : juce::Colour(0xff17221b),
                                   screenOuter.getX(), screenOuter.getY(),
                                   juce::Colour(0xff07110a), screenOuter.getX(), screenOuter.getBottom(), false);
        g.setGradientFill(glass);
        g.fillRoundedRectangle(screenOuter.reduced(1.5f * screenScale), 1.8f * screenScale);
        g.setColour(juce::Colours::white.withAlpha(0.11f));
        g.fillRoundedRectangle(screenOuter.reduced(2.0f * screenScale).withHeight(screenH * 0.16f),
                               1.5f * screenScale);
        g.setColour(juce::Colour(0xff88d785).withAlpha(0.38f));
        g.drawRoundedRectangle(screenOuter.reduced(1.5f * screenScale), 2.0f * screenScale,
                               0.7f * screenScale);

    }

    if (dedaloJetArtwork)
    {
        const float topCentres[] = { 0.143f, 0.497f, 0.851f };
        const juce::String topLabels[] = { "BLEND", "COMP", "ATTACK" };
        const float lowerCentres[] = { 0.323f, 0.675f };
        const juce::String lowerLabels[] = { "GAIN", "TONE" };
        const float labelWidth = b.getWidth() * 0.245f;
        const float topY = b.getY() + b.getHeight() * 0.325f;
        const float lowerY = b.getY() + b.getHeight() * 0.575f;

        g.setColour(juce::Colour(0xff171411).withAlpha(0.94f));
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.2f, b.getHeight() * 0.019f)).withStyle("Bold")));
        for (int i = 0; i < 3; ++i)
        {
            const float cx = b.getX() + b.getWidth() * topCentres[i];
            g.drawFittedText(topLabels[i],
                             juce::Rectangle<float>(cx - labelWidth * 0.5f, topY, labelWidth, 15.0f).toNearestInt(),
                             juce::Justification::centred, 1, 0.80f);
        }
        for (int i = 0; i < 2; ++i)
        {
            const float cx = b.getX() + b.getWidth() * lowerCentres[i];
            g.drawFittedText(lowerLabels[i],
                             juce::Rectangle<float>(cx - labelWidth * 0.5f, lowerY, labelWidth, 15.0f).toNearestInt(),
                             juce::Justification::centred, 1, 0.80f);
        }
    }

    if (harmonizerArtwork)
    {
        const auto ink = juce::Colour(0xff111111);
        const float labelFont = juce::jlimit(7.0f, 29.0f, b.getWidth() * 0.060f);
        auto drawPicoLabel = [&](const juce::String& text, float nx, float ny, float width)
        {
            const auto area = juce::Rectangle<float>(b.getWidth() * width,
                                                      b.getHeight() * 0.042f)
                                  .withCentre({ b.getX() + b.getWidth() * nx,
                                                b.getY() + b.getHeight() * ny });
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(labelFont).withStyle("Bold")));
            g.drawFittedText(text, area.toNearestInt(), juce::Justification::centred, 1, 0.86f);
        };

        drawPicoLabel("MODE",  0.500f, 0.105f, 0.280f);
        drawPicoLabel("VOL",   0.211f, 0.258f, 0.245f);
        drawPicoLabel("BLEND", 0.767f, 0.258f, 0.285f);
        drawPicoLabel("SWEEP", 0.211f, 0.463f, 0.285f);
        drawPicoLabel("SHIFT", 0.767f, 0.463f, 0.265f);

        const float modeX = b.getCentreX();
        const float modeY = b.getY() + b.getHeight() * 0.053f;
        const float modeDiameter = b.getWidth() * 0.140f;
        const float modeRadius = modeDiameter * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(0.44f));
        g.fillEllipse(modeX - modeRadius + modeDiameter * 0.045f,
                      modeY - modeRadius + modeDiameter * 0.075f,
                      modeDiameter, modeDiameter);
        juce::ColourGradient modeBezel(juce::Colour(0xff686865),
                                       modeX - modeRadius, modeY - modeRadius,
                                       juce::Colour(0xff101010),
                                       modeX + modeRadius, modeY + modeRadius, true);
        g.setGradientFill(modeBezel);
        g.fillEllipse(modeX - modeRadius, modeY - modeRadius,
                      modeDiameter, modeDiameter);
        const float capRadius = modeRadius * 0.78f;
        juce::ColourGradient modeCap(juce::Colour(0xff3d3d3a),
                                     modeX - capRadius * 0.55f, modeY - capRadius * 0.70f,
                                     juce::Colour(0xff090909),
                                     modeX + capRadius * 0.55f, modeY + capRadius * 0.65f, true);
        g.setGradientFill(modeCap);
        g.fillEllipse(modeX - capRadius, modeY - capRadius,
                      capRadius * 2.0f, capRadius * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.fillEllipse(modeX - capRadius * 0.45f, modeY - capRadius * 0.58f,
                      capRadius * 0.82f, capRadius * 0.34f);

        const int mode = juce::jlimit(0, 2, juce::roundToInt(knob5.getValue()));
        const auto modeColour = mode == 0 ? juce::Colour(0xffff322f)
                              : (mode == 1 ? juce::Colour(0xff37dc62)
                                           : juce::Colour(0xffff9f24));
        drawLedLens(g, b.getCentreX(), b.getY() + b.getHeight() * 0.262f,
                    juce::jmax(2.2f, b.getWidth() * 0.039f), modeColour,
                    onBtn.getToggleState());

        if (picoModeButtonPressed)
        {
            g.setColour(juce::Colours::white.withAlpha(0.13f));
            const float radius = b.getWidth() * 0.070f;
            g.fillEllipse(b.getCentreX() - radius,
                          b.getY() + b.getHeight() * 0.053f - radius,
                          radius * 2.0f, radius * 2.0f);
        }
    }

    if (bigSkyArtwork)
    {
        static const juce::StringArray typeNames {
            "IMPULSE", "CLOUD", "SHIMMER", "BLOOM", "CHORALE", "MAGNETO",
            "NONLINEAR", "ROOM", "HALL", "CHAMBER", "PLATE", "SPRING"
        };
        const int type = juce::jlimit(0, typeNames.size() - 1, juce::roundToInt(knob8.getValue()));
        auto screen = juce::Rectangle<float>(b.getX() + b.getWidth() * kBigSkyScreenX,
                                              b.getY() + b.getHeight() * kBigSkyScreenY,
                                              b.getWidth() * kBigSkyScreenW,
                                              b.getHeight() * kBigSkyScreenH);
        const float screenPixelScale = juce::jmin(b.getWidth() / kBigSkyCropW,
                                                   b.getHeight() / kBigSkyCropH);
        const float cornerRadius = juce::jmax(1.0f, 5.0f * screenPixelScale);
        const float borderWidth = juce::jlimit(0.55f, 1.1f, screenPixelScale);
        g.setColour(juce::Colour(0xff02090d));
        g.fillRoundedRectangle(screen, cornerRadius);
        g.setColour(juce::Colour(0xff244321));
        g.drawRoundedRectangle(screen.reduced(borderWidth * 0.5f),
                               juce::jmax(0.6f, cornerRadius - borderWidth * 0.5f),
                               borderWidth);
        g.setColour(juce::Colour(0xff70f147));
        const float fontHeight = juce::jlimit(7.0f, 22.0f, screen.getHeight() * 0.34f);
        const float textInsetX = juce::jmax(2.0f, screen.getWidth() * 0.025f);
        const float textInsetY = juce::jmax(1.0f, screen.getHeight() * 0.08f);
        g.setFont(juce::Font(juce::FontOptions(fontHeight).withStyle("Bold")));
        g.drawFittedText(typeNames[type], screen.reduced(textInsetX, textInsetY).toNearestInt(),
                         juce::Justification::centred, 1, 0.72f);

        const float ledY = b.getY() + b.getHeight() * 0.704f;
        const float footY = b.getY() + b.getHeight() * 0.830f;
        const float xs[] = { 0.118f, 0.500f, 0.878f };
        for (int i = 0; i < 3; ++i)
        {
            const bool active = (i == 0 && onBtn.getToggleState())
                             || (i == 2 && tertiaryFootswitchLatched);
            drawLedLens(g, b.getX() + b.getWidth() * xs[i], ledY, 3.0f,
                        juce::Colour(0xff7be1ff), active);
            if (active)
            {
                g.setColour(juce::Colour(0xff6bd6ff).withAlpha(0.15f));
                g.drawEllipse(b.getX() + b.getWidth() * xs[i] - 20.0f, footY - 20.0f,
                              40.0f, 40.0f, 2.0f);
            }
        }
    }

    if (smallCloneArtwork)
    {
        const bool active = onBtn.getToggleState();
        drawLedLens(g, b.getX() + b.getWidth() * 0.758f,
                    b.getY() + b.getHeight() * 0.220f, 4.3f,
                    juce::Colour(0xffff296d), active);
    }

    if (timeLineArtwork)
    {
        static const juce::StringArray typeNames {
            "MULTITAP", "SPECTRAL", "OIL CAN", "REVERSE", "DRUM", "ICE",
            "DIGITAL", "LO FI", "dBUCKET", "FILTER", "dTAPE", "REVERB"
        };
        const int type = juce::jlimit(0, typeNames.size() - 1, juce::roundToInt(knob8.getValue()));
        auto screen = juce::Rectangle<float>(b.getX() + b.getWidth() * kTimelineScreenX,
                                              b.getY() + b.getHeight() * kTimelineScreenY,
                                              b.getWidth() * kTimelineScreenW,
                                              b.getHeight() * kTimelineScreenH);
        const float screenPixelScale = juce::jmin(b.getWidth() / kTimelineCropW,
                                                   b.getHeight() / kTimelineCropH);
        const float cornerRadius = juce::jmax(0.75f, 2.0f * screenPixelScale);
        const float borderWidth = juce::jlimit(0.50f, 1.20f, screenPixelScale);
        g.setColour(juce::Colour(0xff010604));
        g.fillRoundedRectangle(screen, cornerRadius);
        g.setColour(juce::Colour(0xff1f4218));
        g.drawRoundedRectangle(screen.reduced(borderWidth * 0.5f),
                               juce::jmax(0.45f, cornerRadius - borderWidth * 0.5f),
                               borderWidth);

        const float insetX = juce::jmax(1.5f, screen.getWidth() * 0.025f);
        const float insetY = juce::jmax(0.75f, screen.getHeight() * 0.055f);
        const auto textArea = screen.reduced(insetX, insetY);
        const float lineGap = juce::jmax(0.0f, screen.getHeight() * 0.025f);
        const float lineHeight = (textArea.getHeight() - lineGap) * 0.5f;
        const auto topLine = textArea.withHeight(lineHeight);
        const auto bottomLine = textArea.withTrimmedTop(lineHeight + lineGap);

        g.setColour(juce::Colour(0xff4ee82a));
        const float fontHeight = juce::jlimit(5.0f, 18.0f, screen.getHeight() * 0.245f);
        g.setFont(juce::Font(juce::FontOptions(fontHeight).withStyle("Bold")));
        g.drawFittedText("100A  TIMELINE MX", topLine.toNearestInt(),
                         juce::Justification::centred, 1, 0.72f);
        const int bpm = juce::roundToInt(60000.0 / juce::jmax(50.0, knob1.getValue()));
        const juce::String bottom = "1  SER  2  " + typeNames[type].substring(0, 7)
                                  + "  " + juce::String(bpm) + "BPM";
        g.drawFittedText(bottom, bottomLine.toNearestInt(),
                         juce::Justification::centred, 1, 0.66f);

        static constexpr float typeLedX[] = { 0.131f, 0.169f, 0.098f, 0.205f, 0.078f, 0.222f,
                                               0.066f, 0.212f, 0.099f, 0.205f, 0.131f, 0.169f };
        static constexpr float typeLedY[] = { 0.075f, 0.075f, 0.120f, 0.120f, 0.169f, 0.169f,
                                               0.220f, 0.220f, 0.270f, 0.270f, 0.300f, 0.300f };
        for (int i = 0; i < 12; ++i)
            drawLedLens(g, b.getX() + b.getWidth() * typeLedX[i],
                        b.getY() + b.getHeight() * typeLedY[i], 2.0f,
                        juce::Colour(0xff6cff25), i == type);

        const float ledY = b.getY() + b.getHeight() * 0.706f;
        const float xs[] = { 0.115f, 0.500f, 0.880f };
        drawLedLens(g, b.getX() + b.getWidth() * xs[0], ledY, 3.0f,
                    juce::Colour(0xff73ff21), onBtn.getToggleState());
        drawLedLens(g, b.getX() + b.getWidth() * xs[1], ledY, 3.0f,
                    juce::Colour(0xff73ff21), secondaryFootswitchDown);
        drawLedLens(g, b.getX() + b.getWidth() * xs[2], ledY, 3.0f,
                    juce::Colour(0xff73ff21), true);
    }

    if (tenBandEqArtwork)
    {
        // The compact pedal is already rendered at roughly 197 px of artwork
        // height. Use that real miniaturised size as the baseline; using the
        // 92 px design grid here would enlarge the thumbnail a second time.
        // HD typography grows only up to a deliberate ceiling.
        constexpr float compactEqArtworkHeight = 197.0f;
        const float eqScale = juce::jmin(2.0f, b.getHeight() / compactEqArtworkHeight);
        const auto actualEqBounds = b;
        b = { b.getX(), b.getY(), b.getWidth() / eqScale, b.getHeight() / eqScale };
        juce::Graphics::ScopedSaveState eqArtworkState(g);
        g.addTransform(juce::AffineTransform::scale(eqScale, eqScale, b.getX(), b.getY()));

        const auto ink = juce::Colour(0xff17191c);
        const auto blue = juce::Colour(0xff29b7ff);
        const bool active = onBtn.getToggleState();
        const float left = b.getX() + b.getWidth() * 0.055f;
        const float right = b.getRight() - b.getWidth() * 0.055f;
        const float sliderTop = b.getY() + b.getHeight() * 0.285f;
        const float sliderBottom = b.getY() + b.getHeight() * 0.650f;

        g.setColour(ink.withAlpha(0.92f));
        g.setFont(juce::Font(juce::FontOptions(6.6f).withStyle("Bold")));
        g.drawFittedText("OUT 1", juce::Rectangle<float>(b.getX() + b.getWidth() * 0.14f, b.getY() + 5.0f, 35.0f, 10.0f).toNearestInt(), juce::Justification::centred, 1, 0.72f);
        g.drawFittedText("OUT 2", juce::Rectangle<float>(b.getX() + b.getWidth() * 0.32f, b.getY() + 5.0f, 35.0f, 10.0f).toNearestInt(), juce::Justification::centred, 1, 0.72f);
        g.drawFittedText("POWER", juce::Rectangle<float>(b.getX() + b.getWidth() * 0.455f, b.getY() + 5.0f, 42.0f, 10.0f).toNearestInt(), juce::Justification::centred, 1, 0.72f);
        g.drawFittedText("IN", juce::Rectangle<float>(b.getX() + b.getWidth() * 0.75f, b.getY() + 5.0f, 30.0f, 10.0f).toNearestInt(), juce::Justification::centred, 1, 0.72f);

        const juce::String labels[] = { "VOL", "31.25", "62.5", "125", "250", "500", "1K", "2K", "4K", "8K", "16K", "GAIN" };
        juce::Slider* sliders[] = { &eqVolumeSlider,
                                    &eqBandSliders[0], &eqBandSliders[1], &eqBandSliders[2], &eqBandSliders[3], &eqBandSliders[4],
                                    &eqBandSliders[5], &eqBandSliders[6], &eqBandSliders[7], &eqBandSliders[8], &eqBandSliders[9],
                                    &eqGainSlider };
        const float cellW = (right - left) / 12.0f;
        g.setFont(juce::Font(juce::FontOptions(5.9f).withStyle("Bold")));
        for (int i = 0; i < 12; ++i)
        {
            const float cx = left + ((float)i + 0.5f) * cellW;
            g.setColour(ink.withAlpha(0.72f));
            for (int tick = 0; tick < 9; ++tick)
            {
                const float y = juce::jmap((float)tick, 0.0f, 8.0f, sliderTop, sliderBottom);
                const float tickW = tick == 4 ? 7.0f : 4.0f;
                g.drawHorizontalLine(juce::roundToInt(y), cx - tickW, cx + tickW);
            }
            g.setColour(ink.withAlpha(0.96f));
            g.drawFittedText(labels[i], juce::Rectangle<float>(cx - cellW * 0.56f,
                                                               sliderTop - 15.0f,
                                                               cellW * 1.12f, 11.0f).toNearestInt(),
                             juce::Justification::centred, 1, 0.72f);

            if (active)
            {
                const float p = juce::jlimit(0.0f, 1.0f, ((float)sliders[i]->getValue() + 12.0f) / 24.0f);
                const float glowY = sliderTop + (1.0f - p) * (sliderBottom - sliderTop);
                g.setColour(blue.withAlpha(0.13f));
                g.fillEllipse(cx - 8.0f, glowY - 8.0f, 16.0f, 16.0f);
            }
        }

        const float scaleXs[] = { left + cellW * 1.0f, left + cellW * 6.0f, left + cellW * 11.0f };
        const juce::String scale[] = { "+12", "+6", "0", "-6", "-12" };
        g.setFont(juce::Font(juce::FontOptions(5.5f).withStyle("Bold")));
        for (float scaleX : scaleXs)
            for (int tick = 0; tick < 5; ++tick)
            {
                const float y = juce::jmap((float)tick, 0.0f, 4.0f, sliderTop, sliderBottom);
                g.drawText(scale[tick], juce::Rectangle<float>(scaleX - 9.0f, y - 5.0f, 18.0f, 10.0f), juce::Justification::centred);
            }

        auto logo = juce::Rectangle<float>(b.getX() + b.getWidth() * 0.075f,
                                            b.getY() + b.getHeight() * 0.735f,
                                            b.getWidth() * 0.285f, b.getHeight() * 0.145f);
        g.setColour(ink.withAlpha(0.96f));
        g.drawRoundedRectangle(logo, 7.0f, 2.2f);
        g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
        g.drawText("MXR", logo, juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(11.5f)));
        g.drawFittedText("ten band eq",
                         juce::Rectangle<float>(b.getX() + b.getWidth() * 0.625f,
                                                b.getY() + b.getHeight() * 0.765f,
                                                b.getWidth() * 0.32f, 22.0f).toNearestInt(),
                         juce::Justification::centred, 1, 0.88f);

        b = actualEqBounds;
    }

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
    // Custom faceplates already carry their own labels; extra values would
    // obscure the printed hardware artwork in compact view.
    if (!bossNoiseArtwork && !tenBandEqArtwork && !smallCloneArtwork && !beatBuddyArtwork
        && !harmonizerArtwork && !plusPedalArtwork)
    {
        drawKnobValue(knob1, knobValueFormatters[0]);
        drawKnobValue(knob2, knobValueFormatters[1]);
        drawKnobValue(knob3, knobValueFormatters[2]);
        drawKnobValue(knob4, knobValueFormatters[3]);
        drawKnobValue(knob5, knobValueFormatters[4]);
        drawKnobValue(knob6, knobValueFormatters[5]);
        drawKnobValue(knob7, knobValueFormatters[6]);
        drawKnobValue(knob8, knobValueFormatters[7]);
        drawKnobValue(knob9, knobValueFormatters[8]);
    }
    if (!customArtwork)
    {
        drawKnobLabel(knob1, k1);
        drawKnobLabel(knob2, k2);
        drawKnobLabel(knob3, k3);
        drawKnobLabel(knob4, k4);
        drawKnobLabel(knob5, k5);
        drawKnobLabel(knob6, k6);
        drawKnobLabel(knob7, k7);
        drawKnobLabel(knob8, k8);
        drawKnobLabel(knob9, k9);
    }

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
    if (beatBuddyArtwork)
    {
        constexpr float switchCentres[] = { 0.205f, 0.500f, 0.795f };
        constexpr const char* switchLabels[] = { "START", "NEXT", "TAP" };
        const auto brightGold = juce::Colour(0xffffd77a);
        const float switchCY = b.getY() + b.getHeight() * 0.805f;
        g.setColour(brightGold.withAlpha(0.95f));
        g.setFont(juce::Font(juce::FontOptions(6.8f).withStyle("Bold")));
        for (int i = 0; i < 3; ++i)
        {
            const float switchCX = b.getX() + b.getWidth() * switchCentres[i];
            g.drawText(switchLabels[i], juce::roundToInt(switchCX - b.getWidth() * 0.12f),
                       juce::roundToInt(switchCY - b.getWidth() * 0.15f),
                       juce::roundToInt(b.getWidth() * 0.24f), 10, juce::Justification::centred);
        }
    }

    if (dedaloTresArtwork)
    {
        const float leftX = b.getX() + b.getWidth() * 0.147f;
        const float rightX = b.getX() + b.getWidth() * 0.843f;
        const float ledY = b.getY() + b.getHeight() * 0.650f;
        drawLedLens(g, leftX, ledY, 3.4f, juce::Colour(0xfffff8dd), onBtn.getToggleState());
        drawLedLens(g, rightX, ledY, 3.4f, juce::Colour(0xfffff8dd), secondaryFootswitchDown);
    }

    if (!beatBuddyArtwork && !bossNoiseArtwork && !dedaloTresArtwork && !bigSkyArtwork && !smallCloneArtwork
        && !wideFuzzArtwork
        && !plusPedalArtwork
        && !harmonizerArtwork && !voxCooltronArtwork && !timeLineArtwork)
    {
    float ledCX = customArtwork ? (dedaloJetArtwork ? b.getX() + b.getWidth() * 0.156f
                                                     : ((harmonizerArtwork || sustainChordsArtwork) ? b.getCentreX() - 31.0f
                                                     : (modelHostArtwork ? b.getCentreX() + 30.0f
                                                                         : (tenBandEqArtwork ? b.getCentreX() - 34.0f
                                                                                             : (float)getWidth() * 0.5f))))
                                : (float)getWidth() * 0.5f - 27.0f;
    float ledCY = customArtwork ? b.getY() + b.getHeight() * (wideFuzzArtwork ? 0.72f
                                                     : (dedaloJetArtwork ? 0.672f
                                                     : ((harmonizerArtwork || sustainChordsArtwork) ? 0.895f
                                                     : (modelHostArtwork ? 0.90f : (tenBandEqArtwork ? 0.815f : 0.74f))))
                                                     )
                                : (float)getHeight() - 16.0f;
    float ledR = customArtwork ? (dedaloJetArtwork ? 3.4f : (modelHostArtwork ? 4.2f : (tenBandEqArtwork ? 3.6f : 6.5f))) : 5.5f;
    if (!customArtwork)
    {
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f);
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f, 0.8f);
    }

    const auto ledColour = dedaloJetArtwork ? juce::Colour(0xfffff9e8)
                         : (harmonizerArtwork ? juce::Colour(0xff51e0dc)
                         : (sustainChordsArtwork ? juce::Colour(0xff49d5e8)
                         : (modelHostArtwork ? juce::Colour(0xff55dce5)
                         : (tenBandEqArtwork ? juce::Colour(0xff29b7ff) : LnfC::PlayGreen))));
    if (!tenBandEqArtwork)
        drawLedLens(g, ledCX, ledCY, ledR, ledColour, on);

    if (!customArtwork)
    {
        g.setColour(on ? juce::Colours::white : LnfC::TextDim.withAlpha(0.60f));
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.drawText("ON", (int)(ledCX + ledR + 3.0f), (int)(ledCY - 6.0f), 22, 13, juce::Justification::centredLeft);
    }
    }

    // All round stomp switches, including the Pico Pitch Fork, share the same functional
    // rasterized steel hardware without changing hit areas, callbacks or MIDI.
    if (!bossNoiseArtwork && !plusPedalArtwork)
    {
        auto drawAt = [&](float nx, float ny, float size, bool pressed = false)
        {
            drawMetalFootswitch(g, b.getX() + b.getWidth() * nx,
                                b.getY() + b.getHeight() * ny, size, pressed);
        };

        if (harmonizerArtwork)
        {
            // The raster contains transparent padding. A 0.35 target leaves
            // the printed curved line visible around the physical switch.
            const float size = b.getWidth() * 0.35f;
            drawAt(0.500f, 0.878f, size,
                   knob6.getValue() >= 0.5 ? picoMomentaryFootswitchDown : on);
        }
        else if (beatBuddyArtwork)
        {
            const float size = b.getWidth() * 0.18f;
            drawAt(0.205f, 0.805f, size, on);
            drawAt(0.500f, 0.805f, size, secondaryFootswitchDown);
            drawAt(0.795f, 0.805f, size, false);
        }
        else if (voxCooltronArtwork)
        {
            // The raster has transparent padding: 383 of its 512 pixels contain
            // the metal housing. A 12.8% target therefore covers the 9.6%-wide
            // switches printed in the VOX face without covering either label.
            const float size = b.getWidth() * 0.128f;
            drawAt(0.248f, 0.778f, size, on);
            drawAt(0.742f, 0.778f, size, tertiaryFootswitchLatched);
        }
        else if (dedaloTresArtwork)
        {
            drawAt(0.147f, 0.835f, 39.0f * hardwareScale, on);
            drawAt(0.843f, 0.835f, 39.0f * hardwareScale, secondaryFootswitchDown);
        }
        else if (bigSkyArtwork)
        {
            drawAt(0.118f, 0.830f, 38.0f * hardwareScale, on);
            drawAt(0.500f, 0.830f, 38.0f * hardwareScale, false);
            drawAt(0.878f, 0.830f, 38.0f * hardwareScale, tertiaryFootswitchLatched);
        }
        else if (timeLineArtwork)
        {
            drawAt(0.115f, 0.842f, 39.0f * hardwareScale, on);
            drawAt(0.500f, 0.842f, 39.0f * hardwareScale, secondaryFootswitchDown);
            drawAt(0.880f, 0.842f, 39.0f * hardwareScale, false);
        }
        else if (smallCloneArtwork)
        {
            drawAt(0.500f, 0.722f, b.getWidth() * 0.160f, on);
        }
        else
        {
            const float cx = dedaloJetArtwork ? b.getX() + b.getWidth() * 0.156f
                                              : (wideFuzzArtwork ? b.getX() + b.getWidth() * 0.500f
                                                                  : b.getCentreX());
            const float cy = b.getY() + b.getHeight() * (wideFuzzArtwork ? 0.885f
                                                      : (dedaloJetArtwork ? 0.871f
                                                      : (plusPedalArtwork ? 0.90f
                                                      : (modelHostArtwork ? 0.900f
                                                      : (tenBandEqArtwork ? 0.815f : 0.880f)))));

            if (wideFuzzArtwork && on)
            {
                const auto haloColour = juce::Colour(0xffc9f7ff);
                const float haloRadius = 30.0f * hardwareScale;
                juce::ColourGradient halo(haloColour.withAlpha(0.30f), cx, cy,
                                          juce::Colours::transparentBlack,
                                          cx + haloRadius, cy, true);
                g.setGradientFill(halo);
                g.fillEllipse(cx - haloRadius, cy - haloRadius,
                              haloRadius * 2.0f, haloRadius * 2.0f);
                g.setColour(haloColour.withAlpha(0.25f));
                g.drawEllipse(cx - haloRadius * 0.73f, cy - haloRadius * 0.73f,
                              haloRadius * 1.46f, haloRadius * 1.46f,
                              juce::jmax(0.7f, 0.75f * hardwareScale));
            }

            drawMetalFootswitch(g, cx, cy, 40.0f * hardwareScale, on);
        }
    }
}

void FXPedalComponent::resized()
{
    if (onResized)
        onResized(*this);

    const float hardwareScale = getPedalHardwareScale(*this);
    const auto scaled = [hardwareScale](int compactPixels)
    {
        return juce::jmax(1, juce::roundToInt((float) compactPixels * hardwareScale));
    };
    auto b = getLocalBounds().reduced(5);
    onBtn.setBounds(b.getX(), getHeight() - 24, b.getWidth(), 20);
    onBtn.setVisible(false);

    if (pedalName == "CHORUS")
    {
        const float aspect = 3.38f / 5.25f;
        const int bodyHeight = juce::jmin(getHeight() - 6,
                                          juce::roundToInt((float) (getWidth() - 6) / aspect));
        const int bodyWidth = juce::jmin(getWidth() - 6,
                                         juce::roundToInt((float) bodyHeight * aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = (getWidth() - bodyWidth) / 2;

        const int rateSize = juce::roundToInt((float) bodyWidth * 0.315f);
        const int rateX = bodyLeft + juce::roundToInt((float) bodyWidth * 0.500f);
        const int rateY = bodyTop + juce::roundToInt((float) bodyHeight * 0.220f);
        knob2.setBounds(rateX - rateSize / 2, rateY - rateSize / 2, rateSize, rateSize);

        const int switchW = juce::roundToInt((float) bodyWidth * 0.20f);
        const int switchH = juce::roundToInt((float) bodyHeight * 0.13f);
        const int switchX = bodyLeft + juce::roundToInt((float) bodyWidth * 0.255f);
        const int switchY = bodyTop + juce::roundToInt((float) bodyHeight * 0.218f);
        knob3.setBounds(switchX - switchW / 2, switchY - switchH / 2, switchW, switchH);

        // The physical Small Clone has no mix control. Keep the existing DSP control/order
        // as an invisible drag zone over the logo instead of inventing an extra knob.
        knob1.setAlpha(0.0f);
        knob1.setBounds(bodyLeft + juce::roundToInt((float) bodyWidth * 0.12f),
                        bodyTop + juce::roundToInt((float) bodyHeight * 0.40f),
                        juce::roundToInt((float) bodyWidth * 0.76f),
                        juce::roundToInt((float) bodyHeight * 0.22f));
        knob4.setBounds(0, 0, 0, 0); knob5.setBounds(0, 0, 0, 0); knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0); knob8.setBounds(0, 0, 0, 0); knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "RHYTHM BOX")
    {
        const float aspect = 0.72f;
        const int bodyHeight = juce::jmin(getHeight() - 6, juce::roundToInt((float) getWidth() / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyWidth = juce::jmin(getWidth() - 6, juce::roundToInt((float) bodyHeight * aspect));
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        // The raster is intentionally control-free. Place the complete native knobs
        // over the printed labels using the same crop coordinates as paint().
        const int knobSize = juce::roundToInt((float) bodyWidth * 0.150f);
        const int knobCY = bodyTop + juce::roundToInt(bodyHeight * 0.121f);
        const float knobCentres[] = { 0.160f, 0.500f, 0.840f };
        juce::Slider* topKnobs[] = { &knob1, &knob2, &knob3 };
        for (int i = 0; i < 3; ++i)
        {
            const int cx = bodyLeft + juce::roundToInt(bodyWidth * knobCentres[i]);
            topKnobs[i]->setBounds(cx - knobSize / 2, knobCY - knobSize / 2, knobSize, knobSize);
        }

        // Invisible layout anchors used by MainComponent for the interactive screen controls.
        // Keep a small inset from the full-width glass rectangle defined above.
        const int screenX = bodyLeft + juce::roundToInt(bodyWidth * (kBeatBuddyScreenX + 0.012f));
        const int screenW = juce::roundToInt(bodyWidth * (kBeatBuddyScreenW - 0.024f));
        const int screenY = bodyTop + juce::roundToInt(bodyHeight * 0.340f);
        const int controlH = juce::jmax(12, juce::roundToInt(bodyHeight * 0.058f));
        knob4.setBounds(screenX, screenY, screenW, controlH);
        knob5.setBounds(screenX, screenY + controlH + 1, screenW / 2 - 1, controlH);
        knob6.setBounds(screenX + screenW / 2 + 1, screenY + controlH + 1,
                        screenW - screenW / 2 - 1, controlH);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "ROOM REVERB")
    {
        const float aspect = kBigSkyFaceAspect;
        const int availableWidth = getWidth() - 6;
        const int availableHeight = getHeight() - 6;
        const int bodyHeight = juce::jmin(availableHeight,
                                          juce::roundToInt((float) availableWidth / aspect));
        const int bodyWidth = juce::jmin(availableWidth,
                                         juce::roundToInt((float) bodyHeight * aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        auto place = [bodyLeft, bodyTop, bodyWidth, bodyHeight](juce::Slider& knob,
                                                               float nx, float ny, int size)
        {
            const int cx = bodyLeft + juce::roundToInt((float) bodyWidth * nx);
            const int cy = bodyTop + juce::roundToInt((float) bodyHeight * ny);
            knob.setBounds(cx - size / 2, cy - size / 2, size, size);
        };
        place(knob8, 0.140f, 0.172f, scaled(36)); // TYPE
        place(knob9, 0.927f, 0.172f, scaled(34)); // VALUE
        place(knob1, 0.069f, 0.424f, scaled(32)); // DECAY
        place(knob2, 0.214f, 0.424f, scaled(32)); // PRE-DELAY
        place(knob4, 0.356f, 0.424f, scaled(32)); // TONE
        place(knob7, 0.500f, 0.424f, scaled(32)); // MOD
        place(knob5, 0.644f, 0.424f, scaled(32)); // PARAM 1
        place(knob6, 0.786f, 0.424f, scaled(32)); // PARAM 2
        place(knob3, 0.929f, 0.424f, scaled(32)); // MIX
        return;
    }

    if (pedalName == "TAPE DELAY")
    {
        const int availableWidth = getWidth() - 6;
        const int availableHeight = getHeight() - 6;
        const int bodyHeight = juce::jmin(availableHeight,
                                          juce::roundToInt((float) availableWidth / kTimelineFaceAspect));
        const int bodyWidth = juce::jmin(availableWidth,
                                         juce::roundToInt((float) bodyHeight * kTimelineFaceAspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        auto place = [bodyLeft, bodyTop, bodyWidth, bodyHeight](juce::Slider& knob,
                                                               float nx, float ny, int size)
        {
            const int cx = bodyLeft + juce::roundToInt((float) bodyWidth * nx);
            const int cy = bodyTop + juce::roundToInt((float) bodyHeight * ny);
            knob.setBounds(cx - size / 2, cy - size / 2, size, size);
        };
        place(knob8, timelineX(221.0f),  timelineY(190.0f), scaled(38)); // TYPE
        place(knob9, timelineX(1414.0f), timelineY(190.0f), scaled(36)); // VALUE
        place(knob1, timelineX(115.0f),  timelineY(445.0f), scaled(34)); // TIME
        place(knob2, timelineX(336.0f),  timelineY(445.0f), scaled(34)); // REPEATS
        place(knob3, timelineX(552.0f),  timelineY(445.0f), scaled(34)); // FILTER
        place(knob4, timelineX(768.0f),  timelineY(445.0f), scaled(34)); // GRIT
        place(knob5, timelineX(984.0f),  timelineY(445.0f), scaled(34)); // SPEED
        place(knob6, timelineX(1204.0f), timelineY(445.0f), scaled(34)); // DEPTH
        place(knob7, timelineX(1420.0f), timelineY(445.0f), scaled(34)); // MIX
        return;
    }

    if (pedalName.containsIgnoreCase("NOISE GATE"))
    {
        constexpr float aspect = 976.0f / 1612.0f;
        const int availableWidth = getWidth() - 6;
        const int availableHeight = getHeight() - 6;
        const int bodyHeight = juce::jmin(availableHeight,
                                          juce::roundToInt((float) availableWidth / aspect));
        const int bodyWidth = juce::jmin(availableWidth,
                                         juce::roundToInt((float) bodyHeight * aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        const int knobSize = juce::jmax(22, juce::roundToInt(bodyWidth * 0.200f));
        const int knobCY = bodyTop + juce::roundToInt(bodyHeight * 0.163f);
        const float centres[] = { 0.399f, 0.606f, 0.810f };
        juce::Slider* knobs[] = { &knob1, &knob2, &knob3 };
        for (int i = 0; i < 3; ++i)
        {
            const int cx = bodyLeft + juce::roundToInt(bodyWidth * centres[i]);
            knobs[i]->setBounds(cx - knobSize / 2, knobCY - knobSize / 2,
                                knobSize, knobSize);
        }
        knob4.setBounds(0, 0, 0, 0);
        knob5.setBounds(0, 0, 0, 0);
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "FUZZ")
    {
        // The face artwork is control-free: place the interactive knobs
        // inside the printed QUILOMBO / TONO / VOLUMEN ornaments using the
        // same normalized coordinates as the visible crop.
        const int knobSize = juce::jmax(16, juce::roundToInt((float) b.getWidth() * 0.300f));
        const int knobCY = b.getY() + juce::roundToInt((float) b.getHeight() * 0.271f);
        const float knobCentres[] = { 0.200f, 0.500f, 0.800f };
        juce::Slider* knobs[] = { &knob1, &knob2, &knob3 };
        for (int i = 0; i < 3; ++i)
        {
            const int cx = b.getX() + juce::roundToInt((float) b.getWidth() * knobCentres[i]);
            knobs[i]->setBounds(cx - knobSize / 2, knobCY - knobSize / 2,
                                knobSize, knobSize);
        }
        knob4.setBounds(0, 0, 0, 0);
        knob5.setBounds(0, 0, 0, 0);
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "SUNSET")
    {
        constexpr float voxReferenceAspect = 2000.0f / 1874.0f;
        const int bodyHeight = juce::jmin(getHeight() - 6,
                                          juce::roundToInt((float) (getWidth() - 6) / voxReferenceAspect));
        const int bodyWidth = juce::jmin(getWidth() - 6,
                                         juce::roundToInt((float) bodyHeight * voxReferenceAspect));
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        auto place = [bodyLeft, bodyTop, bodyWidth, bodyHeight](juce::Slider& knob,
                                                               float x, float y, int size)
        {
            const int cx = bodyLeft + juce::roundToInt((float) bodyWidth * x);
            const int cy = bodyTop + juce::roundToInt((float) bodyHeight * y);
            knob.setBounds(cx - size / 2, cy - size / 2, size, size);
        };

        const int largeKnobSize = juce::roundToInt((float) bodyWidth * 0.165f);
        const int smallKnobSize = juce::roundToInt((float) bodyWidth * 0.154f);
        place(knob2, 0.164f, 0.315f, largeKnobSize); // GAIN 1
        place(knob1, 0.384f, 0.315f, largeKnobSize); // VOLUME 1
        place(knob5, 0.614f, 0.315f, largeKnobSize); // GAIN 2
        place(knob4, 0.837f, 0.315f, largeKnobSize); // VOLUME 2
        place(knob3, 0.272f, 0.492f, smallKnobSize); // TONE 1
        place(knob7, 0.500f, 0.492f, smallKnobSize); // BASS BOOST
        place(knob6, 0.729f, 0.492f, smallKnobSize); // TONE 2

        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "COMPRESSOR")
    {
        const float aspect = 1.23f;
        const int bodyHeight = juce::jmin(getHeight() - 6, juce::roundToInt((float) getWidth() / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = 3;
        const int bodyWidth = getWidth() - 6;
        const int knobSize = scaled(50);
        const int topCY = bodyTop + juce::roundToInt(bodyHeight * 0.195f);
        const int lowerCY = bodyTop + juce::roundToInt(bodyHeight * 0.435f);
        auto placeKnob = [knobSize](juce::Slider& knob, int cx, int cy)
        {
            knob.setBounds(cx - knobSize / 2, cy - knobSize / 2, knobSize, knobSize);
        };
        auto xAt = [bodyLeft, bodyWidth](float proportion)
        {
            return bodyLeft + juce::roundToInt(bodyWidth * proportion);
        };
        placeKnob(knob1, xAt(0.143f), topCY);   // BLEND
        placeKnob(knob3, xAt(0.497f), topCY);   // COMP
        placeKnob(knob4, xAt(0.851f), topCY);   // ATTACK
        placeKnob(knob2, xAt(0.323f), lowerCY); // GAIN
        placeKnob(knob6, xAt(0.675f), lowerCY); // TONE
        knob5.setBounds(0, 0, 0, 0);            // JET 2 has a fixed release time.
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "TREMOLO")
    {
        const float aspect = 4.0f / 3.0f;
        const int bodyHeight = juce::jmin(getHeight() - 6,
                                          juce::roundToInt((float)getWidth() / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = 3;
        const int bodyWidth = getWidth() - 6;
        const int knobSize = scaled(44);
        const int topCY = bodyTop + juce::roundToInt(bodyHeight * 0.198f);
        const int lowerCY = bodyTop + juce::roundToInt(bodyHeight * 0.433f);
        const int topX[] = {
            bodyLeft + juce::roundToInt(bodyWidth * 0.147f),
            bodyLeft + juce::roundToInt(bodyWidth * 0.500f),
            bodyLeft + juce::roundToInt(bodyWidth * 0.843f)
        };
        const int lowerX[] = {
            bodyLeft + juce::roundToInt(bodyWidth * 0.326f),
            bodyLeft + juce::roundToInt(bodyWidth * 0.674f)
        };
        auto place = [knobSize](juce::Slider& knob, int cx, int cy)
        {
            knob.setBounds(cx - knobSize / 2, cy - knobSize / 2, knobSize, knobSize);
        };
        place(knob1, topX[0], topCY);   // DEPTH
        place(knob2, topX[1], topCY);   // MODE
        place(knob3, topX[2], topCY);   // RATE / f IN
        place(knob4, lowerX[0], lowerCY); // BEATS
        place(knob5, lowerX[1], lowerCY); // SYM / f OUT
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "HARMONIZER")
    {
        const float aspect = 976.0f / 1781.0f;
        const int bodyHeight = juce::jmin(getHeight() - 6, juce::roundToInt((float) (getWidth() - 6) / aspect));
        const int bodyWidth = juce::jmin(getWidth() - 6, juce::roundToInt((float) bodyHeight * aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = (getWidth() - bodyWidth) / 2;
        const int knobSize = juce::jmax(32, juce::roundToInt(bodyWidth * 0.330f));
        const int leftX = bodyLeft + juce::roundToInt(bodyWidth * 0.211f);
        const int rightX = bodyLeft + juce::roundToInt(bodyWidth * 0.767f);
        const int topY = bodyTop + juce::roundToInt(bodyHeight * 0.157f);
        const int lowerY = bodyTop + juce::roundToInt(bodyHeight * 0.370f);
        knob1.setBounds(leftX - knobSize / 2, topY - knobSize / 2, knobSize, knobSize);
        knob2.setBounds(rightX - knobSize / 2, topY - knobSize / 2, knobSize, knobSize);
        knob3.setBounds(leftX - knobSize / 2, lowerY - knobSize / 2, knobSize, knobSize);
        knob4.setBounds(rightX - knobSize / 2, lowerY - knobSize / 2, knobSize, knobSize);
        knob5.setBounds(0, 0, 0, 0);
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "SUSTAIN CHORDS")
    {
        const float aspect = kGamechangerPlusFaceAspect;
        const int availableWidth = getWidth() - 6;
        const int availableHeight = getHeight() - 6;
        const int bodyHeight = juce::jmin(availableHeight,
                                          juce::roundToInt((float) availableWidth / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyWidth = juce::jmin(availableWidth,
                                         juce::roundToInt((float) bodyHeight * aspect));
        const int bodyLeft = (getWidth() - bodyWidth) / 2;

        // The six controls are raster overlays for the printed hardware. Their
        // component bounds include the 4 px LookAndFeel inset on each side;
        // these proportions therefore come from the measured 1024x1536 face
        // and the non-transparent extents of the two knob rasters.
        const int largeKnobSize = juce::jmax(26, juce::roundToInt((float) bodyWidth * 0.182f));
        const int miniKnobSize = juce::jmax(22, juce::roundToInt((float) bodyWidth * 0.141f));
        auto place = [bodyLeft, bodyTop, bodyWidth, bodyHeight](juce::Slider& knob,
                                                                 float nx, float ny, int size)
        {
            const int cx = bodyLeft + juce::roundToInt((float) bodyWidth * nx);
            const int cy = bodyTop + juce::roundToInt((float) bodyHeight * ny);
            knob.setBounds(cx - size / 2, cy - size / 2, size, size);
        };
        // Keep the historical knob indices: this preserves old session values.
        place(knob1, 0.174f, 0.087f, largeKnobSize); // LEVEL -> old mix
        place(knob4, 0.200f, 0.178f, miniKnobSize);  // TONE -> old tone
        place(knob2, 0.174f, 0.274f, largeKnobSize); // RISE -> old fade-in
        place(knob6, 0.803f, 0.086f, largeKnobSize); // LAYERS -> old crossfade/density
        place(knob5, 0.780f, 0.178f, miniKnobSize);  // RESONANCE -> old window
        place(knob3, 0.805f, 0.273f, largeKnobSize); // DECAY -> old fade-out
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (tenBandGraphicEq)
    {
        const float aspect = 124.0f / 92.0f;
        const int bodyHeight = juce::jmin(getHeight() - 6, juce::roundToInt((float)getWidth() / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = 3;
        const int bodyWidth = getWidth() - 6;
        const float left = (float)bodyLeft + (float)bodyWidth * 0.055f;
        const float right = (float)(bodyLeft + bodyWidth) - (float)bodyWidth * 0.055f;
        const int top = bodyTop + juce::roundToInt((float)bodyHeight * 0.285f);
        const int bottom = bodyTop + juce::roundToInt((float)bodyHeight * 0.650f);
        const float cellW = (right - left) / 12.0f;
        juce::Slider* sliders[] = { &eqVolumeSlider,
                                    &eqBandSliders[0], &eqBandSliders[1], &eqBandSliders[2], &eqBandSliders[3], &eqBandSliders[4],
                                    &eqBandSliders[5], &eqBandSliders[6], &eqBandSliders[7], &eqBandSliders[8], &eqBandSliders[9],
                                    &eqGainSlider };
        for (int i = 0; i < 12; ++i)
        {
            const int cx = juce::roundToInt(left + ((float)i + 0.5f) * cellW);
            const int sliderW = juce::jmax(12, juce::roundToInt(cellW * 0.78f));
            sliders[i]->setBounds(cx - sliderW / 2, top, sliderW, bottom - top);
        }
        return;
    }

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
