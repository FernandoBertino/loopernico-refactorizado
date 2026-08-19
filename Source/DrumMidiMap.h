#pragma once

#include <array>
#include <cstdint>

namespace LoopernicoMidi
{
    enum class DrumLane : std::uint8_t
    {
        Kick = 0,
        Snare,
        ClosedHiHat,
        OpenHiHat,
        Crash,
        Ride,
        Count
    };

    struct DrumNoteMap
    {
        static constexpr std::size_t laneCount = static_cast<std::size_t>(DrumLane::Count);

        DrumNoteMap() noexcept
        {
            resetGeneralMidi();
        }

        void resetGeneralMidi() noexcept
        {
            notes = { 36, 38, 42, 46, 49, 51 };
            channels.fill(10);
        }

        int getNote(DrumLane lane) const noexcept
        {
            return notes[indexFor(lane)];
        }

        int getChannel(DrumLane lane) const noexcept
        {
            return channels[indexFor(lane)];
        }

        bool setNote(DrumLane lane, int midiNote) noexcept
        {
            if (midiNote < 0 || midiNote > 127)
                return false;

            notes[indexFor(lane)] = midiNote;
            return true;
        }

        bool setChannel(DrumLane lane, int midiChannel) noexcept
        {
            if (midiChannel < 1 || midiChannel > 16)
                return false;

            channels[indexFor(lane)] = midiChannel;
            return true;
        }

    private:
        static constexpr std::size_t indexFor(DrumLane lane) noexcept
        {
            const auto raw = static_cast<std::size_t>(lane);
            return raw < laneCount ? raw : 0;
        }

        std::array<int, laneCount> notes{};
        std::array<int, laneCount> channels{};
    };
}
