#pragma once

#include <algorithm>
#include <cmath>
#if defined(_MSC_VER)
 #pragma warning(push)
 #pragma warning(disable: 4244 4305 4456)
#endif
#include "../ThirdParty/Signalsmith/signalsmith-stretch.h"

// Streaming wrapper used by the Pico Pitch Fork model. Configuration and reset
// happen outside the audio callback. processSample() only uses storage reserved
// by SignalsmithStretch::configure().
class PicoPitchShifter
{
public:
    struct WetVoices
    {
        float voiceA{ 0.0f };
        float voiceB{ 0.0f };
        float engagement{ 0.0f };
    };

    void prepare(double sampleRate)
    {
        sampleRate_ = std::max(8000.0, sampleRate);

        // About 32 ms of analysis gives the low guitar register enough spectral
        // resolution without the latency of the library's time-stretch preset.
        // Split computation spreads FFT work over the interval and avoids a
        // periodic real-time spike.
        const int blockSamples = std::max(512, (int) std::lround(sampleRate_ * 0.032));
        const int intervalSamples = std::max(64, (int) std::lround(sampleRate_ * 0.004));
        voiceA_.configure(1, blockSamples, intervalSamples, true);
        voiceB_.configure(1, blockSamples, intervalSamples, true);
        latencySamples_ = voiceA_.inputLatency() + voiceA_.outputLatency();
        prepared_ = true;
        reset();
    }

    void reset()
    {
        if (prepared_)
        {
            voiceA_.reset();
            voiceB_.reset();
            voiceA_.setTransposeSemitones(0.0f);
            voiceB_.setTransposeSemitones(0.0f);
        }

        targetA_ = targetB_ = 0.0f;
        appliedA_ = appliedB_ = 0.0f;
        targetEngagement_ = currentEngagement_ = 0.0f;
        sweepSeconds_ = 0.0f;
        dual_ = false;
    }

    void setParameters(float semitonesA, float semitonesB, bool dual,
                       bool engaged, float sweepSeconds) noexcept
    {
        targetA_ = std::clamp(semitonesA, -36.0f, 36.0f);
        targetB_ = std::clamp(semitonesB, -36.0f, 36.0f);
        dual_ = dual;
        targetEngagement_ = engaged ? 1.0f : 0.0f;
        sweepSeconds_ = std::clamp(sweepSeconds, 0.0f, 4.0f);
    }

    WetVoices processSample(float input) noexcept
    {
        if (!prepared_)
            return { input, input, targetEngagement_ };

        if (sweepSeconds_ <= 0.0001f)
        {
            currentEngagement_ = targetEngagement_;
        }
        else
        {
            const float step = 1.0f / (sweepSeconds_ * (float) sampleRate_);
            if (currentEngagement_ < targetEngagement_)
                currentEngagement_ = std::min(targetEngagement_, currentEngagement_ + step);
            else if (currentEngagement_ > targetEngagement_)
                currentEngagement_ = std::max(targetEngagement_, currentEngagement_ - step);
        }

        const float nextA = targetA_ * currentEngagement_;
        const float nextB = (dual_ ? targetB_ : 0.0f) * currentEngagement_;
        constexpr float updateThreshold = 0.00005f;
        if (std::abs(nextA - appliedA_) >= updateThreshold)
        {
            appliedA_ = nextA;
            voiceA_.setTransposeSemitones(appliedA_, (float) (7600.0 / sampleRate_));
        }
        if (std::abs(nextB - appliedB_) >= updateThreshold)
        {
            appliedB_ = nextB;
            voiceB_.setTransposeSemitones(appliedB_, (float) (7600.0 / sampleRate_));
        }

        float outputA = 0.0f;
        float outputB = 0.0f;
        float* inputs[] = { &input };
        float* outputsA[] = { &outputA };
        float* outputsB[] = { &outputB };
        voiceA_.process(inputs, 1, outputsA, 1);
        voiceB_.process(inputs, 1, outputsB, 1);
        return { outputA, outputB, currentEngagement_ };
    }

    int latencySamples() const noexcept { return latencySamples_; }

private:
    signalsmith::stretch::SignalsmithStretch<float> voiceA_{ 0x5049434fL };
    signalsmith::stretch::SignalsmithStretch<float> voiceB_{ 0x464f524bL };
    double sampleRate_{ 44100.0 };
    float targetA_{ 0.0f };
    float targetB_{ 0.0f };
    float appliedA_{ 0.0f };
    float appliedB_{ 0.0f };
    float targetEngagement_{ 0.0f };
    float currentEngagement_{ 0.0f };
    float sweepSeconds_{ 0.0f };
    bool dual_{ false };
    bool prepared_{ false };
    int latencySamples_{ 0 };
};

#if defined(_MSC_VER)
 #pragma warning(pop)
#endif
