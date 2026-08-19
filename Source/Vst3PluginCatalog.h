#pragma once

#include <JuceHeader.h>

class Vst3PluginCatalog final : private juce::Thread,
                                public juce::ChangeBroadcaster
{
public:
    Vst3PluginCatalog();
    ~Vst3PluginCatalog() override;

    juce::StringArray getSearchPaths() const;
    juce::Array<juce::File> getPlugins() const;
    juce::String getStatus() const;
    bool isScanning() const noexcept { return scanning.load(std::memory_order_acquire); }

    void addSearchPath(const juce::File& directory);
    void removeSearchPath(int index);
    void restoreDefaultPaths();
    void startScan();

    static juce::StringArray defaultSearchPaths();

private:
    void run() override;
    void load();
    void save() const;
    static juce::File settingsFile();

    mutable juce::CriticalSection stateLock;
    juce::StringArray searchPaths;
    juce::Array<juce::File> plugins;
    juce::String status;
    std::atomic<bool> scanning { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vst3PluginCatalog)
};
