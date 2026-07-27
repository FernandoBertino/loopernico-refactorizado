#pragma once

#include "DrumMidiMap.h"

#include <array>
#include <cstdint>

namespace LoopernicoMidi
{
    class DrumMidiPattern
    {
    public:
        static constexpr int stepsPerBar = 16;
        static constexpr int maxBars = 4;
        static constexpr int maxSteps = stepsPerBar * maxBars;

        DrumMidiPattern() noexcept = default;

        int getBars() const noexcept { return bars; }

        void setBars(int newBars) noexcept
        {
            bars = newBars < 1 ? 1 : (newBars > maxBars ? maxBars : newBars);
            clearStepsOutsidePattern();
        }

        void clear() noexcept
        {
            velocities = {};
        }

        bool setVelocity(DrumLane lane, int step, int velocity) noexcept
        {
            if (!isValidStep(step) || velocity < 0 || velocity > 127)
                return false;

            velocities[indexFor(lane)][static_cast<std::size_t>(step)] =
                static_cast<std::uint8_t>(velocity);
            return true;
        }

        int getVelocity(DrumLane lane, int step) const noexcept
        {
            return isValidStep(step)
                ? velocities[indexFor(lane)][static_cast<std::size_t>(step)] : 0;
        }

        bool isActive(DrumLane lane, int step) const noexcept
        {
            return getVelocity(lane, step) > 0;
        }

        int countActiveSteps() const noexcept
        {
            int count = 0;
            for (const auto& lane : velocities)
                for (int step = 0; step < bars * stepsPerBar; ++step)
                    count += lane[static_cast<std::size_t>(step)] > 0 ? 1 : 0;
            return count;
        }

    private:
        bool isValidStep(int step) const noexcept
        {
            return step >= 0 && step < bars * stepsPerBar;
        }

        static constexpr std::size_t indexFor(DrumLane lane) noexcept
        {
            const auto raw = static_cast<std::size_t>(lane);
            return raw < DrumNoteMap::laneCount ? raw : 0;
        }

        void clearStepsOutsidePattern() noexcept
        {
            const int firstUnusedStep = bars * stepsPerBar;
            for (auto& lane : velocities)
                for (int step = firstUnusedStep; step < maxSteps; ++step)
                    lane[static_cast<std::size_t>(step)] = 0;
        }

        int bars{ 1 };
        std::array<std::array<std::uint8_t, maxSteps>, DrumNoteMap::laneCount> velocities{};
    };
}
