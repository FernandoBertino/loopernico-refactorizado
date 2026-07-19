#pragma once

#include <JuceHeader.h>
#include "LatencyCompensation.h"

class AudioDeviceController final
{
public:
    struct InitialisationResult
    {
        bool audioAvailable{ false };
        bool restoredSavedConfiguration{ false };
        bool usingFallback{ false };
        juce::String message;
    };

    struct Status
    {
        bool audioAvailable{ false };
        bool usingFallback{ false };
        bool xRunsAvailable{ false };
        bool manualCalibrationEnabled{ false };
        bool softwareInputMonitoringEnabled{ true };
        bool aggressiveBuffer{ false };
        juce::String driverType;
        juce::String inputDevice;
        juce::String outputDevice;
        juce::String lastError;
        double sampleRate{ 0.0 };
        int bufferSize{ 0 };
        int inputLatencySamples{ 0 };
        int outputLatencySamples{ 0 };
        int xRuns{ -1 };
        int manualCalibrationSamples{ 0 };
    };

    explicit AudioDeviceController(juce::AudioDeviceManager& managerToUse);
    ~AudioDeviceController();

    InitialisationResult initialise(bool forceFallbackForDiagnostics = false);
    juce::String restoreRecommendedConfiguration();
    void handleDeviceManagerChange();
    void poll();
    void flushPendingSettings();

    Status getStatus() const;
    juce::AudioDeviceManager& getDeviceManager() noexcept { return manager; }
    juce::File getSettingsFile() const { return settingsFile; }
    juce::File getLogFile() const;
    const InitialisationResult& getInitialisationResult() const noexcept { return initialisationResult; }

    void setManualCalibrationSamples(int samples);
    void resetManualCalibration();
    int getManualCalibrationSamples() const;
    bool isManualCalibrationEnabled() const;
    void setSoftwareInputMonitoringEnabled(bool enabled);
    bool isSoftwareInputMonitoringEnabled() const noexcept
    {
        return softwareInputMonitoringEnabled.load(std::memory_order_relaxed);
    }

    bool showCurrentDeviceControlPanel();
    void log(const juce::String& message);

    static constexpr int preferredStartupBufferSamples = 256;
    static constexpr int minimumCalibrationSamples = -4096;
    static constexpr int maximumCalibrationSamples = 4096;

private:
    juce::AudioIODeviceType* findDeviceType(const juce::String& typeName) const;
    bool validateSavedState(const juce::XmlElement& state, juce::String& reason) const;
    bool tryOpenState(const juce::XmlElement& state,
                      const juce::String& description,
                      juce::String& errorMessage);
    std::unique_ptr<juce::XmlElement> makeDeviceState(const juce::String& typeName,
                                                       const juce::String& inputName,
                                                       const juce::String& outputName,
                                                       double sampleRate,
                                                       int bufferSize) const;
    std::unique_ptr<juce::XmlElement> makeRecommendedAsioState() const;
    std::unique_ptr<juce::XmlElement> makeWindowsFallbackState(const juce::String& typeName,
                                                               double sampleRate,
                                                               int bufferSize) const;
    juce::String chooseSafeEndpoint(const juce::StringArray& names, bool input) const;
    juce::String currentDeviceKey() const;
    juce::String currentDeviceSignature() const;
    juce::XmlElement* findCalibrationElement(const juce::String& key) const;
    juce::XmlElement* findMonitoringElement(const juce::String& key) const;
    void refreshCachedDevicePreferences();
    void loadSettings();
    void saveSettingsNow();
    void persistCurrentDevicePreference();
    void markSettingsDirty();
    void logCurrentDevice(const juce::String& prefix);
    bool applyPreferredStartupBuffer(juce::String& errorMessage);

    juce::AudioDeviceManager& manager;
    juce::File settingsFile;
    std::unique_ptr<juce::XmlElement> settings;
    std::unique_ptr<juce::FileLogger> fileLogger;
    InitialisationResult initialisationResult;
    juce::String automaticFallbackSignature;
    juce::String lastObservedSignature;
    juce::String lastObservedError;
    juce::int64 settingsDirtySinceMs{ 0 };
    bool settingsDirty{ false };
    std::atomic<bool> softwareInputMonitoringEnabled{ true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceController)
};
