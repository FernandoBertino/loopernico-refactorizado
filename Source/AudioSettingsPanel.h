#pragma once

#include <JuceHeader.h>
#include "AudioDeviceController.h"
#include "Vst3PluginCatalog.h"

class AudioSettingsPanel final : public juce::Component,
                                 private juce::ChangeListener
{
public:
    AudioSettingsPanel(AudioDeviceController& controllerToUse,
                       Vst3PluginCatalog& pluginCatalogToUse);
    ~AudioSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void refresh(int activeEffectsLatencySamples);
    void showPluginSettings();

    std::function<void()> onClose;
    std::function<void()> onConfigurationChanged;

private:
    class SearchPathListModel final : public juce::ListBoxModel
    {
    public:
        explicit SearchPathListModel(AudioSettingsPanel& ownerToUse) : owner(ownerToUse) {}
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height,
                              bool rowIsSelected) override;
    private:
        AudioSettingsPanel& owner;
    };

    void updateCalibrationReadout();
    void styleButton(juce::TextButton& button, juce::Colour accent);
    void setPluginView(bool shouldShowPlugins);
    void refreshPluginView();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    AudioDeviceController& controller;
    Vst3PluginCatalog& pluginCatalog;
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
    juce::TextButton pluginViewButton { "PLUGINS VST3" };
    juce::TextButton audioViewButton { "VOLVER A AUDIO" };
    juce::TextButton addPluginPathButton { "AGREGAR CARPETA" };
    juce::TextButton removePluginPathButton { "QUITAR CARPETA" };
    juce::TextButton defaultPluginPathsButton { "RUTAS HABITUALES" };
    juce::TextButton scanPluginsButton { "ESCANEAR" };
    juce::Label pluginInstructions;
    juce::Label pluginPathsTitle;
    juce::Label pluginResultsTitle;
    juce::Label pluginScanStatus;
    juce::ComboBox pluginResults;
    SearchPathListModel searchPathListModel { *this };
    juce::ListBox pluginPathList { "Carpetas VST3", &searchPathListModel };
    juce::StringArray visibleSearchPaths;
    std::unique_ptr<juce::FileChooser> pluginFolderChooser;
    juce::Rectangle<int> cardBounds;
    double currentSampleRate{ 48000.0 };
    bool pluginView = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsPanel)
};
