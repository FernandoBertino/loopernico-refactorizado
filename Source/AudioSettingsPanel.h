#pragma once

#include <JuceHeader.h>
#include "AudioDeviceController.h"

class AudioSettingsPanel final : public juce::Component
{
public:
    explicit AudioSettingsPanel(AudioDeviceController& controllerToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void refresh(int activeEffectsLatencySamples);

    std::function<void()> onClose;
    std::function<void()> onConfigurationChanged;

private:
    void updateCalibrationReadout();
    void styleButton(juce::TextButton& button, juce::Colour accent);

    AudioDeviceController& controller;
    juce::AudioDeviceSelectorComponent selector;
    juce::Label title;
    juce::Label summary;
    juce::Label calibrationTitle;
    juce::Label calibrationReadout;
    juce::Label directMonitoringNote;
    juce::Label physicalLoopbackNote;
    juce::Label actionStatus;
    juce::Slider calibrationSlider;
    juce::TextButton automaticButton { "VOLVER A AUTOMATICO" };
    juce::TextButton softwareMonitoringButton { "MONITOREO ENTRADA: SOFTWARE" };
    juce::TextButton recommendedButton { "RESTAURAR RECOMENDADO" };
    juce::TextButton controlPanelButton { "PANEL DEL DRIVER" };
    juce::TextButton closeButton { "CERRAR  ESC" };
    juce::Rectangle<int> cardBounds;
    double currentSampleRate{ 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsPanel)
};
