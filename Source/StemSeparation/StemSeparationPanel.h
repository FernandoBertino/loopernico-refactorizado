#pragma once

#include "StemSeparationEngine.h"
#include "StemImportController.h"

namespace LoopernicoStems
{
class StemSeparationPanel final : public juce::Component
{
public:
    StemSeparationPanel(StemSeparationEngine& engineToUse,
                        StemImportController& importerToUse);
    ~StemSeparationPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void showDemucsModeForSnapshot();

    std::function<void(const StemImportController::ImportResult&)> onImportFinished;
    std::function<int()> getFreeTrackCount;

private:
    void chooseInputFile();
    void chooseDemucsExecutable();
    void chooseDemucsModel();
    void updateModeControls();
    void startSeparation();
    void cancelWork();
    void setBusy(bool busy);
    std::vector<StemType> selectedStemTypes() const;
    void handleSeparationComplete(StemSeparationResult result);
    void handleImportComplete(StemImportController::ImportResult result);

    StemSeparationEngine& engine;
    StemImportController& importer;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File selectedFile;

    juce::Label titleLabel;
    juce::TextButton chooseButton { "SELECCIONAR ARCHIVO" };
    juce::Label fileLabel;
    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    juce::TextButton demucsExecutableButton { "EJECUTABLE DEMUCS" };
    juce::Label demucsExecutableLabel;
    juce::TextButton demucsModelButton { "PESOS 6 STEMS" };
    juce::Label demucsModelLabel;
    std::array<juce::ToggleButton, 6> stemButtons;
    juce::ToggleButton automaticImport { "Importar automaticamente al terminar" };
    juce::ToggleButton normalizeOutput { "Normalizar resultados" };
    juce::TextButton separateButton { "SEPARAR" };
    juce::TextButton cancelButton { "CANCELAR" };
    double progressValue = 0.0;
    juce::ProgressBar progressBar { progressValue };
    juce::Label statusLabel;
    juce::Label errorLabel;
    juce::File demucsExecutable;
    juce::File demucsModel;
    bool completedWasMock = true;
};

class StemSeparationWindow final : public juce::DocumentWindow
{
public:
    explicit StemSeparationWindow(juce::Component& content);
    void closeButtonPressed() override { setVisible(false); }
};
}
