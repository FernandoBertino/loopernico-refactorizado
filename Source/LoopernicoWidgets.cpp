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
            g.setColour(col.withAlpha(isTop ? 0.22f : 0.07f));
            g.fillRoundedRectangle(bar.getX() - 1.0f, top - 0.8f,
                                   bar.getWidth() + 2.0f, segH + 0.5f, 2.0f);
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

    juce::Colour tint = isRec ? LnfC::RecRed.withAlpha(0.05f)
                      : isOvd ? LnfC::OvdAmber.withAlpha(0.04f)
                      : isPlay ? tColor.withAlpha(0.035f)
                               : juce::Colours::transparentBlack;

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
    juce::ColourGradient waveWell(juce::Colour(0xff080a0b), wavePanel.getX(), wavePanel.getY(),
                                  juce::Colour(0xff020304), wavePanel.getX(), wavePanel.getBottom(), false);
    g.setGradientFill(waveWell);
    g.fillRoundedRectangle(wavePanel, 4.0f);
    g.setColour(tColor.withAlpha(hasWaveform ? 0.22f : 0.11f));
    g.drawRoundedRectangle(wavePanel.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.035f));
    g.drawHorizontalLine((int) wavePanel.getCentreY(), wavePanel.getX() + 6.0f, wavePanel.getRight() - 6.0f);
    for (int tick = 1; tick < 8; ++tick)
    {
        const float x = wavePanel.getX() + wavePanel.getWidth() * (float) tick / 8.0f;
        g.setColour(juce::Colours::white.withAlpha(tick % 2 == 0 ? 0.055f : 0.028f));
        g.drawVerticalLine((int) x, wavePanel.getY() + 5.0f, wavePanel.getBottom() - 5.0f);
    }
    if (hasWaveform)
    {
        g.setColour(tint);
        g.fillRoundedRectangle(wavePanel, 4.0f);
        drawFineRackTexture(g, wavePanel, 0.16f);
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

juce::Point<float> FXPedalComponent::getJackCentre(JackSide side) const noexcept
{
    return { side == JackSide::input ? (float)getWidth() - 4.5f : 4.5f,
             (float)getHeight() * 0.38f };
}

namespace
{
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

const juce::Image& getPedalEnamelTexture(const juce::String& name)
{
    static const auto noiseGate = loadPedalTexture("Pedals/noise-gate-face-v1.png");
    static const auto pampaFuzz = loadPedalTexture("Pedals/pampa-fuzz-face-v3.png");
    static const auto mirrorDrive = loadPedalTexture("Pedals/mirror-drive-face-v1.png");
    static const auto dedaloJet = loadPedalTexture("Pedals/dedalo-jet-compressor-face-v3.png");
    static const auto neutral = loadPedalTexture("Textures/pedal-enamel-neutral-v1.png");
    static const auto silver  = loadPedalTexture("Textures/pedal-brushed-silver-v1.png");
    static const auto black   = loadPedalTexture("Textures/pedal-black-powder-v1.png");
    static const auto lacquer = loadPedalTexture("Textures/pedal-blue-lacquer-v1.png");

    if (name.containsIgnoreCase("NOISE GATE"))
        return noiseGate;
    if (name == "RHYTHM BOX")
        return black;
    if (name == "FUZZ")
        return pampaFuzz;
    if (name == "OVERDRIVE")
        return mirrorDrive;
    if (name == "COMPRESSOR")
        return dedaloJet;
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
        return { juce::roundToInt(w * 0.25f), juce::roundToInt(h * 0.075f),
                 juce::roundToInt(w * 0.50f), juce::roundToInt(h * 0.85f) };
    if (name == "RHYTHM BOX")
        return { juce::roundToInt(w * 0.08f), juce::roundToInt(h * 0.04f),
                 juce::roundToInt(w * 0.84f), juce::roundToInt(h * 0.92f) };
    if (name == "FUZZ")
        return { juce::roundToInt(w * 0.065f), juce::roundToInt(h * 0.195f),
                 juce::roundToInt(w * 0.87f), juce::roundToInt(h * 0.665f) };
    if (name == "OVERDRIVE")
        return { juce::roundToInt(w * 0.035f), juce::roundToInt(h * 0.02f),
                 juce::roundToInt(w * 0.93f), juce::roundToInt(h * 0.95f) };
    if (name == "COMPRESSOR")
        return { juce::roundToInt(w * 0.015f), juce::roundToInt(h * 0.015f),
                 juce::roundToInt(w * 0.97f), juce::roundToInt(h * 0.97f) };
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
    if (name == "OVERDRIVE")
        return 0.97f;
    if (name == "COMPRESSOR")
        return 0.98f;
    if (name == "RHYTHM BOX")
        return 0.82f;
    return name.containsIgnoreCase("NOISE GATE") ? 0.82f : 0.24f;
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
    auto b = getLocalBounds().toFloat().reduced(3.0f);
    const bool wideFuzzArtwork = pedalName == "FUZZ";
    const bool mirrorDriveArtwork = pedalName == "OVERDRIVE";
    const bool dedaloJetArtwork = pedalName == "COMPRESSOR";
    const bool beatBuddyArtwork = pedalName == "RHYTHM BOX";
    const bool customArtwork = wideFuzzArtwork || mirrorDriveArtwork || dedaloJetArtwork || beatBuddyArtwork;
    if (customArtwork)
    {
        const float aspect = wideFuzzArtwork ? 1.30f : (dedaloJetArtwork ? 1.23f : (beatBuddyArtwork ? 0.88f : 1.08f));
        const float bodyHeight = juce::jmin(b.getHeight(), b.getWidth() / aspect);
        b = juce::Rectangle<float>(b.getX(), b.getCentreY() - bodyHeight * 0.5f,
                                   b.getWidth(), bodyHeight);
    }
    auto originalBody = b;

    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(b.translated(0.0f, 7.0f).expanded(4.0f, 2.0f), 12.0f);
    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.fillRoundedRectangle(b.translated(0.0f, 3.0f).expanded(1.5f), 10.0f);

    const auto enamel = pedalCol.withMultipliedSaturation(0.68f).darker(0.08f);
    juce::ColourGradient bodyGrad(
        enamel.brighter(0.10f), b.getX(), b.getY(),
        enamel.darker(0.58f),   b.getX(), b.getBottom(), false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(b, 9.0f);

    if (const auto& enamelTexture = getPedalEnamelTexture(pedalName); enamelTexture.isValid())
    {
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(b.toNearestInt());
        g.setOpacity(getPedalTextureOpacity(pedalName));
        const auto crop = getPedalTextureCrop(pedalName, enamelTexture);
        const auto target = b.toNearestInt();
        g.drawImage(enamelTexture,
                    target.getX(), target.getY(), target.getWidth(), target.getHeight(),
                    crop.getX(), crop.getY(), crop.getWidth(), crop.getHeight());
    }

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
    if (dragHintVisible)
    {
        const auto dragPlate = customArtwork ? b.reduced(8.0f).withHeight(18.0f) : plate;
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawRoundedRectangle(dragPlate.expanded(1.0f), 5.0f, 1.6f);
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText("DRAG", dragPlate.toNearestInt(), juce::Justification::centredRight);
    }

    float sb = customArtwork ? b.getY() : getLocalBounds().toFloat().reduced(3.0f).getY();
    const float screwBottom = customArtwork ? b.getBottom() : (float) getHeight();
    float scrR = 4.2f;
    drawScrew(g, b.getX() + scrR + 2.0f, sb + scrR + 2.5f, scrR);
    drawScrew(g, b.getRight() - scrR - 2.0f, sb + scrR + 2.5f, scrR);
    drawScrew(g, b.getX() + scrR + 2.0f, screwBottom - scrR - 5.5f, scrR);
    drawScrew(g, b.getRight() - scrR - 2.0f, screwBottom - scrR - 5.5f, scrR);

    if (beatBuddyArtwork)
    {
        const auto gold = juce::Colour(0xffd8a84a);
        const auto warmGold = juce::Colour(0xfff1c66d);

        juce::ColourGradient topRail(gold.brighter(0.30f), b.getX(), b.getY(),
                                     gold.darker(0.58f), b.getRight(), b.getY(), false);
        g.setGradientFill(topRail);
        g.fillRoundedRectangle(b.getX() + 11.0f, b.getY() + 4.0f, b.getWidth() - 22.0f, 3.0f, 1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.66f));
        g.drawLine(b.getX() + 12.0f, b.getY() + 8.0f, b.getRight() - 12.0f, b.getY() + 8.0f, 1.0f);

        const bool rhythmOn = onBtn.getToggleState();
        const auto magenta = juce::Colour(0xffff2bd6);
        const auto beatsArea = juce::Rectangle<float>(b.getX() + 13.0f, b.getY() + 7.0f,
                                                       b.getWidth() - 26.0f, 20.0f);
        g.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
        if (rhythmOn)
        {
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx)
                    if (dx != 0 || dy != 0)
                    {
                        g.setColour(magenta.withAlpha((std::abs(dx) + std::abs(dy) <= 2) ? 0.12f : 0.045f));
                        g.drawText("BEATS", beatsArea.translated((float) dx, (float) dy),
                                   juce::Justification::centred);
                    }
        }
        g.setColour(rhythmOn ? magenta : magenta.darker(0.72f).withAlpha(0.72f));
        g.drawText("BEATS", beatsArea, juce::Justification::centred);

        const float screenY = b.getY() + b.getHeight() * 0.370f;
        const float screenH = b.getHeight() * 0.270f;
        auto screenOuter = juce::Rectangle<float>(b.getX() + b.getWidth() * 0.095f, screenY,
                                                   b.getWidth() * 0.81f, screenH);
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(screenOuter.translated(0.0f, 2.5f).expanded(2.0f), 7.0f);
        juce::ColourGradient bezel(juce::Colour(0xffd6a64d), screenOuter.getX(), screenOuter.getY(),
                                   juce::Colour(0xff5b3a0b), screenOuter.getRight(), screenOuter.getBottom(), true);
        g.setGradientFill(bezel);
        g.fillRoundedRectangle(screenOuter.expanded(2.0f), 6.0f);
        g.setColour(juce::Colour(0xff050708));
        g.fillRoundedRectangle(screenOuter, 4.5f);
        juce::ColourGradient glass(juce::Colour(0xff17353a), screenOuter.getX(), screenOuter.getY(),
                                   juce::Colour(0xff071416), screenOuter.getX(), screenOuter.getBottom(), false);
        g.setGradientFill(glass);
        g.fillRoundedRectangle(screenOuter.reduced(3.0f), 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.11f));
        g.fillRoundedRectangle(screenOuter.reduced(4.0f).withHeight(screenH * 0.18f), 2.5f);
        g.setColour(juce::Colour(0xff8eeaf0).withAlpha(0.35f));
        g.drawRoundedRectangle(screenOuter.reduced(3.5f), 3.0f, 0.8f);

        const juce::String topLabels[] = { "", "VOLUME", "" };
        const float centres[] = { 0.205f, 0.500f, 0.795f };
        g.setColour(warmGold.withAlpha(0.92f));
        g.setFont(juce::Font(juce::FontOptions(7.0f).withStyle("Bold")));
        for (int i = 0; i < 3; ++i)
        {
            const float cx = b.getX() + b.getWidth() * centres[i];
            if (topLabels[i].isNotEmpty())
                g.drawText(topLabels[i], juce::roundToInt(cx - b.getWidth() * 0.12f),
                           juce::roundToInt(b.getY() + b.getHeight() * 0.295f),
                           juce::roundToInt(b.getWidth() * 0.24f), 10, juce::Justification::centred);
        }

    }

    if (dedaloJetArtwork)
    {
        const float centres[] = { 0.218f, 0.502f, 0.786f };
        const juce::String topLabels[] = { "BLEND", "GAIN", "COMP" };
        const juce::String bottomLabels[] = { "ATTACK", "RELEASE", "TONE" };
        const float labelWidth = b.getWidth() * 0.245f;
        const float topY = b.getY() + b.getHeight() * 0.300f;
        const float bottomY = b.getY() + b.getHeight() * 0.592f;

        g.setColour(juce::Colour(0xff171411).withAlpha(0.94f));
        g.setFont(juce::Font(juce::FontOptions(7.2f).withStyle("Bold")));
        for (int i = 0; i < 3; ++i)
        {
            const float cx = b.getX() + b.getWidth() * centres[i];
            g.drawFittedText(topLabels[i],
                             juce::Rectangle<float>(cx - labelWidth * 0.5f, topY, labelWidth, 11.0f).toNearestInt(),
                             juce::Justification::centred, 1, 0.80f);
            g.drawFittedText(bottomLabels[i],
                             juce::Rectangle<float>(cx - labelWidth * 0.5f, bottomY, labelWidth, 11.0f).toNearestInt(),
                             juce::Justification::centred, 1, 0.80f);
        }
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
    drawKnobValue(knob1, knobValueFormatters[0]);
    drawKnobValue(knob2, knobValueFormatters[1]);
    drawKnobValue(knob3, knobValueFormatters[2]);
    drawKnobValue(knob4, knobValueFormatters[3]);
    drawKnobValue(knob5, knobValueFormatters[4]);
    drawKnobValue(knob6, knobValueFormatters[5]);
    drawKnobValue(knob7, knobValueFormatters[6]);
    drawKnobValue(knob8, knobValueFormatters[7]);
    drawKnobValue(knob9, knobValueFormatters[8]);
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
        const auto gold = juce::Colour(0xffd5a243);
        const auto brightGold = juce::Colour(0xffffd77a);
        auto drawGoldSwitch = [&](float cx, float cy, bool active)
        {
            const float outerR = 17.0f;
            const float innerR = 11.8f;
            g.setColour(juce::Colours::black.withAlpha(0.72f));
            g.fillEllipse(cx - outerR - 2.0f, cy - outerR + 3.0f,
                          (outerR + 2.0f) * 2.0f, (outerR + 2.0f) * 2.0f);
            juce::ColourGradient ring(brightGold, cx - outerR * 0.58f, cy - outerR * 0.72f,
                                      juce::Colour(0xff6f430b), cx + outerR * 0.62f, cy + outerR * 0.70f, true);
            g.setGradientFill(ring);
            g.fillEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f);
            g.setColour(juce::Colours::black.withAlpha(0.76f));
            g.fillEllipse(cx - innerR - 1.0f, cy - innerR + 1.0f,
                          (innerR + 1.0f) * 2.0f, (innerR + 1.0f) * 2.0f);
            juce::ColourGradient cap(juce::Colour(0xfff2ede1), cx - innerR * 0.55f, cy - innerR * 0.70f,
                                     juce::Colour(0xff5a584f), cx + innerR * 0.50f, cy + innerR * 0.62f, true);
            g.setGradientFill(cap);
            g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.30f));
            g.fillEllipse(cx - innerR * 0.48f, cy - innerR * 0.68f,
                          innerR * 0.92f, innerR * 0.30f);
            if (active)
            {
                g.setColour(juce::Colour(0xff80ffb0).withAlpha(0.22f));
                g.drawEllipse(cx - outerR - 3.0f, cy - outerR - 3.0f,
                              (outerR + 3.0f) * 2.0f, (outerR + 3.0f) * 2.0f, 3.0f);
            }
        };

        const float tapCX = b.getCentreX();
        const float tapCY = b.getY() + b.getHeight() * 0.820f;
        const float tapR = 15.0f;
        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillEllipse(tapCX - tapR - 2.0f, tapCY - tapR + 2.0f,
                      (tapR + 2.0f) * 2.0f, (tapR + 2.0f) * 2.0f);
        juce::ColourGradient tapRing(brightGold, tapCX - tapR * 0.65f, tapCY - tapR * 0.65f,
                                     juce::Colour(0xff65400c), tapCX + tapR * 0.62f, tapCY + tapR * 0.65f, true);
        g.setGradientFill(tapRing);
        g.fillEllipse(tapCX - tapR, tapCY - tapR, tapR * 2.0f, tapR * 2.0f);
        g.setColour(juce::Colour(0xff151719));
        g.fillEllipse(tapCX - tapR + 4.0f, tapCY - tapR + 4.0f,
                      (tapR - 4.0f) * 2.0f, (tapR - 4.0f) * 2.0f);
        g.setColour(brightGold.withAlpha(0.95f));
        g.setFont(juce::Font(juce::FontOptions(7.4f).withStyle("Bold")));
        g.drawText("TAP", juce::roundToInt(tapCX - tapR), juce::roundToInt(tapCY - 5.0f),
                   juce::roundToInt(tapR * 2.0f), 10, juce::Justification::centred);

        const float footY = b.getY() + b.getHeight() * 0.885f;
        drawGoldSwitch(b.getX() + b.getWidth() * 0.255f, footY, on);
        drawGoldSwitch(b.getX() + b.getWidth() * 0.745f, footY, false);

        drawLedLens(g, b.getCentreX(),
                    b.getY() + b.getHeight() * 0.700f, 3.2f,
                    juce::Colour(0xffff2bd6), on);
    }

    if (!beatBuddyArtwork)
    {
    float ledCX = customArtwork ? (dedaloJetArtwork ? b.getX() + b.getWidth() * 0.156f : (float)getWidth() * 0.5f)
                                : (float)getWidth() * 0.5f - 27.0f;
    float ledCY = customArtwork ? b.getY() + b.getHeight() * (wideFuzzArtwork ? 0.72f : (dedaloJetArtwork ? 0.672f : 0.74f))
                                : (float)getHeight() - 16.0f;
    float ledR = customArtwork ? (dedaloJetArtwork ? 3.4f : 6.5f) : 5.5f;
    float footCX = customArtwork ? (dedaloJetArtwork ? b.getX() + b.getWidth() * 0.156f : (float)getWidth() * 0.5f)
                                 : (float)getWidth() * 0.5f + 22.0f;
    float footCY = customArtwork ? b.getY() + b.getHeight() * (wideFuzzArtwork ? 0.84f : (dedaloJetArtwork ? 0.871f : 0.88f))
                                 : (float)getHeight() - 17.0f;
    float footR = customArtwork ? 14.0f : 12.0f;

    if (customArtwork)
    {
        const float nutR = footR + 4.5f;
        juce::Path hexNut;
        for (int i = 0; i < 6; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 6.0f - juce::MathConstants<float>::halfPi;
            const auto p = juce::Point<float>(footCX + std::cos(a) * nutR, footCY + std::sin(a) * nutR);
            if (i == 0) hexNut.startNewSubPath(p); else hexNut.lineTo(p);
        }
        hexNut.closeSubPath();
        auto nutShadow = hexNut;
        nutShadow.applyTransform(juce::AffineTransform::translation(1.8f, 3.0f));
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillPath(nutShadow);
        juce::ColourGradient nutMetal(juce::Colour(0xffeee8d9), footCX - nutR * 0.5f, footCY - nutR * 0.65f,
                                      juce::Colour(0xff4a4945), footCX + nutR * 0.5f, footCY + nutR * 0.65f, true);
        g.setGradientFill(nutMetal);
        g.fillPath(hexNut);
        g.setColour(juce::Colours::black.withAlpha(0.48f));
        g.strokePath(hexNut, juce::PathStrokeType(1.2f));

        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillEllipse(footCX - footR - 1.2f, footCY - footR + 1.5f, footR * 2.0f + 2.4f, footR * 2.0f + 2.4f);
        juce::ColourGradient switchMetal(juce::Colour(0xffd9d5c9), footCX - footR * 0.45f, footCY - footR * 0.60f,
                                         juce::Colour(0xff5a5955), footCX + footR * 0.42f, footCY + footR * 0.55f, true);
        g.setGradientFill(switchMetal);
        g.fillEllipse(footCX - footR, footCY - footR, footR * 2.0f, footR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.34f));
        g.fillEllipse(footCX - footR * 0.48f, footCY - footR * 0.70f, footR * 0.90f, footR * 0.30f);
        g.setColour(juce::Colours::black.withAlpha(0.44f));
        g.drawEllipse(footCX - footR, footCY - footR, footR * 2.0f, footR * 2.0f, 1.0f);
    }
    else
    {
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
    }

    if (!customArtwork)
    {
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f);
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawRoundedRectangle(ledCX - ledR - 7.0f, ledCY - ledR - 3.0f, 40.0f, 18.0f, 5.0f, 0.8f);
    }

    const auto ledColour = dedaloJetArtwork ? juce::Colour(0xfffff9e8) : LnfC::PlayGreen;
    drawLedLens(g, ledCX, ledCY, ledR, ledColour, on);

    if (!customArtwork)
    {
        g.setColour(on ? juce::Colours::white : LnfC::TextDim.withAlpha(0.60f));
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.drawText("ON", (int)(ledCX + ledR + 3.0f), (int)(ledCY - 6.0f), 22, 13, juce::Justification::centredLeft);
    }
    }

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
    auto outJack = getJackCentre(JackSide::output);
    auto inJack = getJackCentre(JackSide::input);
    drawJack(outJack.x, outJack.y);
    drawJack(inJack.x, inJack.y);

    g.setColour(juce::Colours::white.withAlpha(0.46f));
    g.setFont(juce::Font(juce::FontOptions(6.5f).withStyle("Bold")));
    g.drawText("OUT", 7, (int)outJack.y - 15, 22, 9, juce::Justification::centredLeft);
    g.drawText("IN", getWidth() - 29, (int)inJack.y - 15, 22, 9, juce::Justification::centredRight);
}

void FXPedalComponent::resized()
{
    auto b = getLocalBounds().reduced(5);
    onBtn.setBounds(b.getX(), getHeight() - 24, b.getWidth(), 20);
    onBtn.setVisible(false);

    if (pedalName == "RHYTHM BOX")
    {
        const float aspect = 0.88f;
        const int bodyHeight = juce::jmin(getHeight() - 6, juce::roundToInt((float) getWidth() / aspect));
        const int bodyTop = (getHeight() - bodyHeight) / 2;
        const int bodyLeft = 3;
        const int bodyWidth = getWidth() - 6;
        const int knobSize = 45;
        const int knobCY = bodyTop + juce::roundToInt(bodyHeight * 0.205f);
        const float knobCentres[] = { 0.205f, 0.500f, 0.795f };
        juce::Slider* topKnobs[] = { &knob1, &knob2, &knob3 };
        for (int i = 0; i < 3; ++i)
        {
            const int cx = bodyLeft + juce::roundToInt(bodyWidth * knobCentres[i]);
            topKnobs[i]->setBounds(cx - knobSize / 2, knobCY - knobSize / 2, knobSize, knobSize);
        }

        // Invisible layout anchors used by MainComponent for the interactive screen controls.
        const int screenX = bodyLeft + juce::roundToInt(bodyWidth * 0.13f);
        const int screenW = juce::roundToInt(bodyWidth * 0.74f);
        const int screenY = bodyTop + juce::roundToInt(bodyHeight * 0.395f);
        const int controlH = 23;
        knob4.setBounds(screenX, screenY, screenW, controlH);
        knob5.setBounds(screenX, screenY + 32, screenW / 2 - 3, controlH);
        knob6.setBounds(screenX + screenW / 2 + 3, screenY + 32, screenW / 2 - 3, controlH);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "FUZZ")
    {
        const int top = juce::roundToInt((float) getHeight() * 0.18f);
        const int knobH = juce::roundToInt((float) getHeight() * 0.21f);
        const int thirdW = (b.getWidth() - 18) / 3;
        knob1.setBounds(b.getX() + 3, top, thirdW, knobH);
        knob2.setBounds(b.getX() + thirdW + 9, top, thirdW, knobH);
        knob3.setBounds(b.getX() + (thirdW + 6) * 2 + 3, top, thirdW, knobH);
        knob4.setBounds(0, 0, 0, 0);
        knob5.setBounds(0, 0, 0, 0);
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
        return;
    }

    if (pedalName == "OVERDRIVE")
    {
        const int top = juce::roundToInt((float) getHeight() * 0.20f);
        const int knobH = juce::roundToInt((float) getHeight() * 0.22f);
        const int thirdW = (b.getWidth() - 18) / 3;
        // Artwork order is VOLUME, TONE, GAIN; DSP order is DRIVE, TONE, LEVEL.
        knob3.setBounds(b.getX() + 3, top, thirdW, knobH);
        knob2.setBounds(b.getX() + thirdW + 9, top, thirdW, knobH);
        knob1.setBounds(b.getX() + (thirdW + 6) * 2 + 3, top, thirdW, knobH);
        knob4.setBounds(0, 0, 0, 0);
        knob5.setBounds(0, 0, 0, 0);
        knob6.setBounds(0, 0, 0, 0);
        knob7.setBounds(0, 0, 0, 0);
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
        // Keep the Jet knob size stable even when the enclosure width changes.
        const int knobSize = 40;
        const int topCY = bodyTop + juce::roundToInt(bodyHeight * 0.200f);
        const int bottomCY = bodyTop + juce::roundToInt(bodyHeight * 0.486f);
        const int leftCX = bodyLeft + juce::roundToInt(bodyWidth * 0.218f);
        const int centreCX = bodyLeft + juce::roundToInt(bodyWidth * 0.502f);
        const int rightCX = bodyLeft + juce::roundToInt(bodyWidth * 0.786f);
        auto placeKnob = [knobSize](juce::Slider& knob, int cx, int cy)
        {
            knob.setBounds(cx - knobSize / 2, cy - knobSize / 2, knobSize, knobSize);
        };
        placeKnob(knob1, leftCX, topCY);
        placeKnob(knob2, centreCX, topCY);
        placeKnob(knob3, rightCX, topCY);
        placeKnob(knob4, leftCX, bottomCY);
        placeKnob(knob5, centreCX, bottomCY);
        placeKnob(knob6, rightCX, bottomCY);
        knob7.setBounds(0, 0, 0, 0);
        knob8.setBounds(0, 0, 0, 0);
        knob9.setBounds(0, 0, 0, 0);
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
