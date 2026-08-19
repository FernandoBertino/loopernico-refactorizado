#include "StemSeparationPanel.h"

namespace LoopernicoStems
{
namespace
{
juce::File installedStemComponent(const juce::String& relativePath)
{
    const auto bundled = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getChildFile("StemSeparation").getChildFile(relativePath);
    if (bundled.existsAsFile())
        return bundled;

    auto localAppData = juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", {});
    if (localAppData.isEmpty())
        localAppData = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory).getFullPathName();
    return juce::File(localAppData).getChildFile("Loopernico")
        .getChildFile("StemSeparation").getChildFile(relativePath);
}
}

StemSeparationPanel::StemSeparationPanel(StemSeparationEngine& engineToUse,
                                         StemImportController& importerToUse)
    : engine(engineToUse), importer(importerToUse)
{
    titleLabel.setText("SEPARAR CANCION", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff65d9ff));
    addAndMakeVisible(titleLabel);

    chooseButton.onClick = [this]() { chooseInputFile(); };
    addAndMakeVisible(chooseButton);
    fileLabel.setText("Ningun archivo seleccionado", juce::dontSendNotification);
    fileLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    fileLabel.setMinimumHorizontalScale(0.65f);
    addAndMakeVisible(fileLabel);

    modeLabel.setText("MODO", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
    addAndMakeVisible(modeLabel);
    modeSelector.addItem("Modo de prueba (copia la mezcla; NO separa)", 1);
    modeSelector.addItem("Modo 6 stems - Demucs CPU externo", 2);
    juce::PopupMenu::Item onnxItem("Modo 6 stems (requiere modelo ONNX)");
    onnxItem.itemID = 3;
    onnxItem.isEnabled = false;
    modeSelector.getRootMenu()->addItem(std::move(onnxItem));
    modeSelector.setSelectedId(1, juce::dontSendNotification);
    modeSelector.onChange = [this]() { updateModeControls(); };
    addAndMakeVisible(modeSelector);

    demucsExecutableButton.onClick = [this]() { chooseDemucsExecutable(); };
    demucsModelButton.onClick = [this]() { chooseDemucsModel(); };
    demucsExecutableLabel.setText("No configurado", juce::dontSendNotification);
    demucsModelLabel.setText("No configurado", juce::dontSendNotification);
    for (auto* label : { &demucsExecutableLabel, &demucsModelLabel })
    {
        label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.70f));
        label->setMinimumHorizontalScale(0.55f);
        addAndMakeVisible(label);
    }
    addAndMakeVisible(demucsExecutableButton);
    addAndMakeVisible(demucsModelButton);

    demucsExecutable = installedStemComponent("Runtime/demucs.cpp.main.exe");
    demucsModel = installedStemComponent("Models/ggml-model-htdemucs-6s-f16.bin");
    if (demucsExecutable.existsAsFile())
        demucsExecutableLabel.setText(demucsExecutable.getFileName(),
                                      juce::dontSendNotification);
    if (demucsModel.existsAsFile())
        demucsModelLabel.setText(demucsModel.getFileName(), juce::dontSendNotification);
    if (demucsExecutable.existsAsFile() && demucsModel.existsAsFile())
        modeSelector.setSelectedId(2, juce::dontSendNotification);

    const auto standard = standardSixStems();
    for (size_t i = 0; i < stemButtons.size(); ++i)
    {
        stemButtons[i].setButtonText(stemTypeName(standard[i]));
        stemButtons[i].setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(stemButtons[i]);
    }

    automaticImport.setToggleState(true, juce::dontSendNotification);
    normalizeOutput.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(automaticImport);
    addAndMakeVisible(normalizeOutput);

    separateButton.onClick = [this]() { startSeparation(); };
    cancelButton.onClick = [this]() { cancelWork(); };
    cancelButton.setEnabled(false);
    addAndMakeVisible(separateButton);
    addAndMakeVisible(cancelButton);
    addAndMakeVisible(progressBar);

    statusLabel.setText("Selecciona una cancion WAV, FLAC o MP3.", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    addAndMakeVisible(statusLabel);
    errorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffa146));
    errorLabel.setJustificationType(juce::Justification::topLeft);
    errorLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(errorLabel);
    updateModeControls();
}

StemSeparationPanel::~StemSeparationPanel()
{
    fileChooser.reset();
}

void StemSeparationPanel::showDemucsModeForSnapshot()
{
    modeSelector.setSelectedId(2, juce::sendNotificationSync);
}

void StemSeparationPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0e12));
    auto card = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(0xff151b22));
    g.fillRoundedRectangle(card, 9.0f);
    g.setColour(juce::Colour(0xff65d9ff).withAlpha(0.25f));
    g.drawRoundedRectangle(card, 9.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(253, 28.0f, (float) getWidth() - 28.0f);
}

void StemSeparationPanel::resized()
{
    auto area = getLocalBounds().reduced(28);
    titleLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(8);

    auto fileRow = area.removeFromTop(32);
    chooseButton.setBounds(fileRow.removeFromLeft(190));
    fileRow.removeFromLeft(10);
    fileLabel.setBounds(fileRow);
    area.removeFromTop(10);

    auto modeRow = area.removeFromTop(30);
    modeLabel.setBounds(modeRow.removeFromLeft(58));
    modeSelector.setBounds(modeRow.removeFromLeft(390));
    area.removeFromTop(8);

    auto executableRow = area.removeFromTop(30);
    demucsExecutableButton.setBounds(executableRow.removeFromLeft(190));
    executableRow.removeFromLeft(10);
    demucsExecutableLabel.setBounds(executableRow);
    area.removeFromTop(6);
    auto modelRow = area.removeFromTop(30);
    demucsModelButton.setBounds(modelRow.removeFromLeft(190));
    modelRow.removeFromLeft(10);
    demucsModelLabel.setBounds(modelRow);
    area.removeFromTop(12);

    auto stemsArea = area.removeFromTop(82);
    const int columnWidth = stemsArea.getWidth() / 3;
    for (size_t i = 0; i < stemButtons.size(); ++i)
    {
        const int row = (int) i / 3;
        const int column = (int) i % 3;
        stemButtons[i].setBounds(stemsArea.getX() + column * columnWidth,
                                 stemsArea.getY() + row * 35,
                                 columnWidth - 8, 28);
    }

    automaticImport.setBounds(area.removeFromTop(28));
    normalizeOutput.setBounds(area.removeFromTop(28));
    area.removeFromTop(12);

    auto actions = area.removeFromTop(34);
    separateButton.setBounds(actions.removeFromLeft(140));
    actions.removeFromLeft(10);
    cancelButton.setBounds(actions.removeFromLeft(120));
    area.removeFromTop(12);
    progressBar.setBounds(area.removeFromTop(22));
    statusLabel.setBounds(area.removeFromTop(30));
    errorLabel.setBounds(area);
}

void StemSeparationPanel::updateModeControls()
{
    const bool demucsMode = modeSelector.getSelectedId() == 2;
    demucsExecutableButton.setVisible(demucsMode);
    demucsExecutableLabel.setVisible(demucsMode);
    demucsModelButton.setVisible(demucsMode);
    demucsModelLabel.setVisible(demucsMode);
    if (demucsMode)
        statusLabel.setText(demucsExecutable.existsAsFile() && demucsModel.existsAsFile()
                                ? "Demucs CPU listo para separacion real."
                                : "Separacion real por CPU; configura ejecutable y pesos.",
                            juce::dontSendNotification);
    else if (!engine.isRunning())
        statusLabel.setText("Selecciona una cancion WAV, FLAC o MP3.",
                            juce::dontSendNotification);
}

void StemSeparationPanel::chooseInputFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Seleccionar cancion", selectedFile.getParentDirectory(), "*.wav;*.flac;*.mp3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (const juce::FileChooser& chooser)
        {
            if (auto* panel = safeThis.getComponent())
            {
                panel->selectedFile = chooser.getResult();
                panel->fileChooser.reset();
                if (panel->selectedFile.existsAsFile())
                {
                    panel->fileLabel.setText(panel->selectedFile.getFileName(), juce::dontSendNotification);
                    panel->errorLabel.setText({}, juce::dontSendNotification);
                }
            }
        });
}

void StemSeparationPanel::chooseDemucsExecutable()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Seleccionar ejecutable demucs.cpp", demucsExecutable.getParentDirectory(), "*.exe");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (const juce::FileChooser& chooser)
        {
            if (auto* panel = safeThis.getComponent())
            {
                panel->demucsExecutable = chooser.getResult();
                panel->fileChooser.reset();
                if (panel->demucsExecutable.existsAsFile())
                    panel->demucsExecutableLabel.setText(
                        panel->demucsExecutable.getFileName(), juce::dontSendNotification);
            }
        });
}

void StemSeparationPanel::chooseDemucsModel()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Seleccionar pesos GGML de HTDemucs 6s", demucsModel.getParentDirectory(), "*.bin");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (const juce::FileChooser& chooser)
        {
            if (auto* panel = safeThis.getComponent())
            {
                panel->demucsModel = chooser.getResult();
                panel->fileChooser.reset();
                if (panel->demucsModel.existsAsFile())
                    panel->demucsModelLabel.setText(
                        panel->demucsModel.getFileName(), juce::dontSendNotification);
            }
        });
}

std::vector<StemType> StemSeparationPanel::selectedStemTypes() const
{
    const auto standard = standardSixStems();
    std::vector<StemType> selected;
    for (size_t i = 0; i < stemButtons.size(); ++i)
        if (stemButtons[i].getToggleState())
            selected.push_back(standard[i]);
    return selected;
}

void StemSeparationPanel::startSeparation()
{
    errorLabel.setText({}, juce::dontSendNotification);
    if (!selectedFile.existsAsFile())
    {
        errorLabel.setText("Selecciona primero un archivo existente.", juce::dontSendNotification);
        return;
    }
    const auto stems = selectedStemTypes();
    if (stems.empty())
    {
        errorLabel.setText("Selecciona al menos un stem.", juce::dontSendNotification);
        return;
    }
    const bool demucsMode = modeSelector.getSelectedId() == 2;
    if (demucsMode && !demucsExecutable.existsAsFile())
    {
        errorLabel.setText("Selecciona el ejecutable demucs.cpp.", juce::dontSendNotification);
        return;
    }
    if (demucsMode && !demucsModel.existsAsFile())
    {
        errorLabel.setText("Selecciona los pesos GGML de HTDemucs 6s.", juce::dontSendNotification);
        return;
    }

    if (automaticImport.getToggleState() && getFreeTrackCount != nullptr)
    {
        const int freeTracks = getFreeTrackCount();
        if (freeTracks < (int) stems.size())
            errorLabel.setText("Hay " + juce::String(freeTracks) + " pistas libres para "
                               + juce::String(stems.size())
                               + " stems. Al terminar se importaran solo los primeros que entren.",
                               juce::dontSendNotification);
    }

    StemSeparationOptions options;
    options.inputFile = selectedFile;
    options.requestedStems = stems;
    options.normalizeOutput = normalizeOutput.getToggleState();
    options.importAutomatically = automaticImport.getToggleState();
    options.mode = demucsMode ? StemSeparationMode::demucsCppSixStem
                              : StemSeparationMode::mock;
    options.demucsCppExecutable = demucsExecutable;
    options.demucsCppModel = demucsModel;
    completedWasMock = !demucsMode;
    progressValue = 0.0;
    setBusy(true);

    engine.startSeparation(options,
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (float progress, const juce::String& status)
        {
            if (auto* panel = safeThis.getComponent())
            {
                panel->progressValue = progress;
                panel->statusLabel.setText(status, juce::dontSendNotification);
            }
        },
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (StemSeparationResult result)
        {
            if (auto* panel = safeThis.getComponent())
                panel->handleSeparationComplete(std::move(result));
        });
}

void StemSeparationPanel::handleSeparationComplete(StemSeparationResult result)
{
    if (!result.success)
    {
        setBusy(false);
        statusLabel.setText(result.cancelled ? "Cancelado" : "Error", juce::dontSendNotification);
        errorLabel.setText(result.errorMessage, juce::dontSendNotification);
        return;
    }

    if (!automaticImport.getToggleState())
    {
        setBusy(false);
        progressValue = 1.0;
        statusLabel.setText(completedWasMock
                                ? "Finalizado (modo de prueba; no es separacion real)"
                                : "Finalizado (separacion real por CPU)",
                            juce::dontSendNotification);
        errorLabel.setText("Archivos guardados en: " + result.jobDirectory.getFullPathName(),
                           juce::dontSendNotification);
        return;
    }

    statusLabel.setText("Importando pistas", juce::dontSendNotification);
    progressValue = 0.0;
    importer.importAsync(result.stems,
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (float progress, const juce::String& status)
        {
            if (auto* panel = safeThis.getComponent())
            {
                panel->progressValue = progress;
                panel->statusLabel.setText(status, juce::dontSendNotification);
            }
        },
        [safeThis = juce::Component::SafePointer<StemSeparationPanel>(this)]
        (StemImportController::ImportResult importResult)
        {
            if (auto* panel = safeThis.getComponent())
                panel->handleImportComplete(std::move(importResult));
        });
}

void StemSeparationPanel::handleImportComplete(StemImportController::ImportResult result)
{
    setBusy(false);
    if (result.success)
    {
        progressValue = 1.0;
        statusLabel.setText("Finalizado: " + juce::String(result.imported.size())
                            + (completedWasMock
                                ? " pistas importadas (mock; no es separacion real)"
                                : " pistas importadas (Demucs CPU)"),
                            juce::dontSendNotification);
    }
    else
        statusLabel.setText(result.cancelled ? "Cancelado" : "Error", juce::dontSendNotification);
    errorLabel.setText(result.errorMessage, juce::dontSendNotification);
    if (onImportFinished != nullptr)
        onImportFinished(result);
}

void StemSeparationPanel::cancelWork()
{
    engine.cancel();
    importer.cancel();
    statusLabel.setText("Cancelando...", juce::dontSendNotification);
    cancelButton.setEnabled(false);
}

void StemSeparationPanel::setBusy(bool busy)
{
    chooseButton.setEnabled(!busy);
    modeSelector.setEnabled(!busy);
    demucsExecutableButton.setEnabled(!busy);
    demucsModelButton.setEnabled(!busy);
    for (auto& button : stemButtons)
        button.setEnabled(!busy);
    automaticImport.setEnabled(!busy);
    normalizeOutput.setEnabled(!busy);
    separateButton.setEnabled(!busy);
    cancelButton.setEnabled(busy);
}

StemSeparationWindow::StemSeparationWindow(juce::Component& content)
    : juce::DocumentWindow("Separar cancion", juce::Colour(0xff0b0e12),
                           juce::DocumentWindow::closeButton, true)
{
    setUsingNativeTitleBar(true);
    setResizable(false, false);
    setContentNonOwned(&content, true);
    centreWithSize(620, 610);
}
}
