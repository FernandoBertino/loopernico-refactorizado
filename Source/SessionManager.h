#pragma once

#include <JuceHeader.h>
#include "LooperEngine.h"

// Owns the on-disk .loopernico container format. Building and applying the
// UI-specific ValueTree remains the responsibility of MainComponent.
class SessionManager final
{
public:
    static constexpr int currentVersion = 1;

    static bool writeSessionFile(const juce::File& file,
                                 const juce::ValueTree& sessionTree,
                                 const LooperEngine::SessionSnapshot& looperSnapshot,
                                 const juce::MemoryBlock& vstState,
                                 juce::String& errorMessage);

    static bool readSessionFile(const juce::File& file,
                                juce::ValueTree& sessionTree,
                                LooperEngine::SessionSnapshot& looperSnapshot,
                                juce::MemoryBlock& vstState,
                                juce::String& errorMessage);

private:
    SessionManager() = delete;
};
