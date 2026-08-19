#include "AudioSettingsPanel.h"
#include "LoopernicoLookAndFeel.h"

namespace
{
    juce::String samplesAndMs(int samples, double sampleRate)
    {
        const double ms = sampleRate > 0.0 ? 1000.0 * (double) samples / sampleRate : 0.0;
        return juce::String(samples) + " muestras / " + juce::String(ms, 3) + " ms";
    }
}

AudioSettingsPanel::AudioSettingsPanel(AudioDeviceController& controllerToUse,
                                       Vst3PluginCatalog& pluginCatalogToUse)
    : controller(controllerToUse),
      pluginCatalog(pluginCatalogToUse),
      selector(controller.getDeviceManager(), 1, 2, 1, 2, false, false, true, false)
{
    setVisible(false);
    setWantsKeyboardFocus(true);

    title.setText("CONFIGURACION Y DIAGNOSTICO DE AUDIO", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    selector.setItemHeight(24);
    addAndMakeVisible(selector);

    summary.setFont(juce::Font(juce::FontOptions(13.0f)));
    summary.setColour(juce::Label::textColourId, juce::Colour(0xffe7edf3));
    summary.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(summary);

    calibrationTitle.setText("CALIBRACION POR DISPOSITIVO", juce::dontSendNotification);
    calibrationTitle.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    calibrationTitle.setColour(juce::Label::textColourId, LnfC::OvdAmber);
    addAndMakeVisible(calibrationTitle);

    calibrationSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    calibrationSlider.setRange(AudioDeviceController::minimumCalibrationSamples,
                               AudioDeviceController::maximumCalibrationSamples, 1.0);
    calibrationSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 92, 24);
    calibrationSlider.setTextValueSuffix(" smp");
    calibrationSlider.onValueChange = [this]()
    {
        controller.setManualCalibrationSamples((int) calibrationSlider.getValue());
        updateCalibrationReadout();
        if (onConfigurationChanged)
            onConfigurationChanged();
    };
    addAndMakeVisible(calibrationSlider);

    calibrationReadout.setFont(juce::Font(juce::FontOptions(11.5f)));
    calibrationReadout.setColour(juce::Label::textColourId, juce::Colour(0xffcad3dc));
    addAndMakeVisible(calibrationReadout);

    directMonitoringNote.setText(
        "BOTON DIRECT DE LA AUDIOBOX: es monitoreo analogico y Loopernico no puede detectarlo. "
        "Si lo activas, apaga MONITOREO ENTRADA para evitar duplicar la guitarra. Los loops y el rhythm/metronomo "
        "siguen sonando porque usan buses independientes.",
        juce::dontSendNotification);
    directMonitoringNote.setFont(juce::Font(juce::FontOptions(11.0f)));
    directMonitoringNote.setColour(juce::Label::textColourId, juce::Colour(0xffb8c3ce));
    directMonitoringNote.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(directMonitoringNote);

    physicalLoopbackNote.setText(
        "LOOPBACK FISICO: no medido. El offset manual corrige la diferencia entre lo reportado por el driver "
        "y una medicion analogica; no se emite ninguna señal de calibracion automaticamente.",
        juce::dontSendNotification);
    physicalLoopbackNote.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    physicalLoopbackNote.setColour(juce::Label::textColourId, juce::Colour(0xffffc56b));
    physicalLoopbackNote.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(physicalLoopbackNote);

    actionStatus.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
    actionStatus.setColour(juce::Label::textColourId, LnfC::TextDim);
    addAndMakeVisible(actionStatus);

    styleButton(recommendedButton, LnfC::PlayGreen);
    styleButton(controlPanelButton, LnfC::Accent);
    styleButton(automaticButton, LnfC::OvdAmber);
    styleButton(softwareMonitoringButton, LnfC::Accent);
    styleButton(closeButton, LnfC::RecRed);
    styleButton(pluginViewButton, juce::Colour(0xffb36bff));
    styleButton(audioViewButton, LnfC::Accent);
    styleButton(addPluginPathButton, LnfC::PlayGreen);
    styleButton(removePluginPathButton, LnfC::RecRed);
    styleButton(defaultPluginPathsButton, LnfC::OvdAmber);
    styleButton(scanPluginsButton, juce::Colour(0xffb36bff));

    softwareMonitoringButton.setClickingTogglesState(true);
    softwareMonitoringButton.onClick = [this]()
    {
        const bool enabled = softwareMonitoringButton.getToggleState();
        controller.setSoftwareInputMonitoringEnabled(enabled);
        softwareMonitoringButton.setButtonText(enabled ? "MONITOREO ENTRADA: SOFTWARE"
                                                        : "MONITOREO ENTRADA: DIRECT");
        actionStatus.setText(enabled
            ? "Entrada monitoreada por Loopernico."
            : "Entrada sin monitoreo de software; loops y rhythm permanecen activos.",
            juce::dontSendNotification);
        actionStatus.setColour(juce::Label::textColourId, enabled ? LnfC::PlayGreen : LnfC::OvdAmber);
        if (onConfigurationChanged)
            onConfigurationChanged();
    };

    recommendedButton.onClick = [this]()
    {
        const auto error = controller.restoreRecommendedConfiguration();
        actionStatus.setText(error.isEmpty() ? "AudioBox ASIO 48 kHz / 256 restaurada."
                                             : "No se pudo restaurar: " + error,
                             juce::dontSendNotification);
        actionStatus.setColour(juce::Label::textColourId,
                               error.isEmpty() ? LnfC::PlayGreen : LnfC::RecRed);
        if (onConfigurationChanged)
            onConfigurationChanged();
    };
    controlPanelButton.onClick = [this]()
    {
        const bool shown = controller.showCurrentDeviceControlPanel();
        actionStatus.setText(shown ? "Panel del driver cerrado; configuracion actualizada."
                                   : "Este dispositivo no ofrece panel de control.",
                             juce::dontSendNotification);
        actionStatus.setColour(juce::Label::textColourId, shown ? LnfC::PlayGreen : LnfC::OvdAmber);
        if (onConfigurationChanged)
            onConfigurationChanged();
    };
    automaticButton.onClick = [this]()
    {
        controller.resetManualCalibration();
        calibrationSlider.setValue(0, juce::dontSendNotification);
        updateCalibrationReadout();
        actionStatus.setText("Compensacion automatica restaurada para este dispositivo.",
                             juce::dontSendNotification);
        actionStatus.setColour(juce::Label::textColourId, LnfC::PlayGreen);
        if (onConfigurationChanged)
            onConfigurationChanged();
    };
    closeButton.onClick = [this]()
    {
        if (onClose)
            onClose();
    };

    pluginViewButton.onClick = [this]() { setPluginView(true); };
    audioViewButton.onClick = [this]() { setPluginView(false); };
    addPluginPathButton.onClick = [this]()
    {
        pluginFolderChooser = std::make_unique<juce::FileChooser>(
            "Agregar carpeta para buscar plugins VST3",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        pluginFolderChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& chooser)
            {
                const auto folder = chooser.getResult();
                pluginFolderChooser.reset();
                if (folder.isDirectory())
                    pluginCatalog.addSearchPath(folder);
            });
    };
    removePluginPathButton.onClick = [this]()
    {
        pluginCatalog.removeSearchPath(pluginPathList.getSelectedRow());
    };
    defaultPluginPathsButton.onClick = [this]() { pluginCatalog.restoreDefaultPaths(); };
    scanPluginsButton.onClick = [this]() { pluginCatalog.startScan(); };

    pluginInstructions.setText(
        "Loopernico busca solamente archivos o paquetes .vst3. El escaneo no abre ni ejecuta "
        "plugins. Las rutas habituales incluyen Common Files\\VST3, Descargas y Plugins junto "
        "al ejecutable.", juce::dontSendNotification);
    pluginInstructions.setFont(juce::Font(juce::FontOptions(12.0f)));
    pluginInstructions.setColour(juce::Label::textColourId, juce::Colour(0xffcad3dc));
    pluginInstructions.setJustificationType(juce::Justification::topLeft);
    pluginPathsTitle.setText("CARPETAS DE BUSQUEDA", juce::dontSendNotification);
    pluginResultsTitle.setText("PLUGINS ENCONTRADOS", juce::dontSendNotification);
    for (auto* label : { &pluginPathsTitle, &pluginResultsTitle })
    {
        label->setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
        label->setColour(juce::Label::textColourId, LnfC::OvdAmber);
    }
    pluginScanStatus.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    pluginScanStatus.setColour(juce::Label::textColourId, LnfC::TextDim);
    pluginPathList.setRowHeight(30);
    pluginPathList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff101419));
    pluginResults.setTextWhenNothingSelected("NINGUN PLUGIN SELECCIONADO");

    for (auto* component : { static_cast<juce::Component*>(&pluginViewButton),
                             static_cast<juce::Component*>(&audioViewButton),
                             static_cast<juce::Component*>(&addPluginPathButton),
                             static_cast<juce::Component*>(&removePluginPathButton),
                             static_cast<juce::Component*>(&defaultPluginPathsButton),
                             static_cast<juce::Component*>(&scanPluginsButton),
                             static_cast<juce::Component*>(&pluginInstructions),
                             static_cast<juce::Component*>(&pluginPathsTitle),
                             static_cast<juce::Component*>(&pluginResultsTitle),
                             static_cast<juce::Component*>(&pluginScanStatus),
                             static_cast<juce::Component*>(&pluginPathList),
                             static_cast<juce::Component*>(&pluginResults) })
        addAndMakeVisible(component);

    addAndMakeVisible(recommendedButton);
    addAndMakeVisible(controlPanelButton);
    addAndMakeVisible(automaticButton);
    addAndMakeVisible(softwareMonitoringButton);
    addAndMakeVisible(closeButton);

    pluginCatalog.addChangeListener(this);
    refreshPluginView();
    setPluginView(false);
}

AudioSettingsPanel::~AudioSettingsPanel()
{
    pluginCatalog.removeChangeListener(this);
}

int AudioSettingsPanel::SearchPathListModel::getNumRows()
{
    return owner.visibleSearchPaths.size();
}

void AudioSettingsPanel::SearchPathListModel::paintListBoxItem(
    int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (!juce::isPositiveAndBelow(row, owner.visibleSearchPaths.size()))
        return;
    if (rowIsSelected)
        g.fillAll(LnfC::Accent.withAlpha(0.22f));
    g.setColour(juce::Colour(0xffe7edf3));
    g.setFont(juce::Font(juce::FontOptions(11.5f)));
    g.drawFittedText(owner.visibleSearchPaths[row], 9, 0, width - 18, height,
                     juce::Justification::centredLeft, 1, 0.82f);
}

void AudioSettingsPanel::showPluginSettings()
{
    setPluginView(true);
}

void AudioSettingsPanel::setPluginView(bool shouldShowPlugins)
{
    pluginView = shouldShowPlugins;
    title.setText(pluginView ? "CONFIGURACION DE PLUGINS VST3"
                             : "CONFIGURACION Y DIAGNOSTICO DE AUDIO",
                  juce::dontSendNotification);

    for (auto* component : { static_cast<juce::Component*>(&selector),
                             static_cast<juce::Component*>(&summary),
                             static_cast<juce::Component*>(&calibrationTitle),
                             static_cast<juce::Component*>(&calibrationReadout),
                             static_cast<juce::Component*>(&directMonitoringNote),
                             static_cast<juce::Component*>(&physicalLoopbackNote),
                             static_cast<juce::Component*>(&actionStatus),
                             static_cast<juce::Component*>(&calibrationSlider),
                             static_cast<juce::Component*>(&automaticButton),
                             static_cast<juce::Component*>(&softwareMonitoringButton),
                             static_cast<juce::Component*>(&recommendedButton),
                             static_cast<juce::Component*>(&controlPanelButton),
                             static_cast<juce::Component*>(&pluginViewButton) })
        component->setVisible(!pluginView);

    for (auto* component : { static_cast<juce::Component*>(&audioViewButton),
                             static_cast<juce::Component*>(&addPluginPathButton),
                             static_cast<juce::Component*>(&removePluginPathButton),
                             static_cast<juce::Component*>(&defaultPluginPathsButton),
                             static_cast<juce::Component*>(&scanPluginsButton),
                             static_cast<juce::Component*>(&pluginInstructions),
                             static_cast<juce::Component*>(&pluginPathsTitle),
                             static_cast<juce::Component*>(&pluginResultsTitle),
                             static_cast<juce::Component*>(&pluginScanStatus),
                             static_cast<juce::Component*>(&pluginPathList),
                             static_cast<juce::Component*>(&pluginResults) })
        component->setVisible(pluginView);

    if (pluginView)
        refreshPluginView();
    resized();
    repaint();
}

void AudioSettingsPanel::refreshPluginView()
{
    visibleSearchPaths = pluginCatalog.getSearchPaths();
    pluginPathList.updateContent();
    if (pluginPathList.getSelectedRow() >= visibleSearchPaths.size())
        pluginPathList.deselectAllRows();

    pluginResults.clear(juce::dontSendNotification);
    const auto plugins = pluginCatalog.getPlugins();
    int itemId = 1;
    for (const auto& plugin : plugins)
        pluginResults.addItem(plugin.getFileNameWithoutExtension() + "  -  "
                                  + plugin.getParentDirectory().getFullPathName(), itemId++);

    pluginScanStatus.setText(pluginCatalog.getStatus(), juce::dontSendNotification);
    pluginScanStatus.setColour(juce::Label::textColourId,
        pluginCatalog.isScanning() ? LnfC::OvdAmber : LnfC::PlayGreen);
    scanPluginsButton.setEnabled(!pluginCatalog.isScanning());
}

void AudioSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &pluginCatalog)
        refreshPluginView();
}

void AudioSettingsPanel::styleButton(juce::TextButton& button, juce::Colour accent)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff14191f));
    button.setColour(juce::TextButton::buttonOnColourId, accent.darker(0.3f));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.92f));
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void AudioSettingsPanel::refresh(int activeEffectsLatencySamples)
{
    const auto status = controller.getStatus();
    currentSampleRate = status.sampleRate > 0.0 ? status.sampleRate : 48000.0;

    LoopernicoLatency::Breakdown latency;
    latency.inputSamples = status.inputLatencySamples;
    latency.outputSamples = status.outputLatencySamples;
    latency.pluginSamples = activeEffectsLatencySamples;
    latency.calibrationSamples = status.manualCalibrationSamples;

    juce::String text;
    if (!status.audioAvailable)
    {
        text = "SIN AUDIO ACTIVO\n" + controller.getInitialisationResult().message;
    }
    else
    {
        text << "DRIVER ACTIVO: " << status.driverType << "\n"
             << "ENTRADA: " << status.inputDevice << "\n"
             << "SALIDA: " << status.outputDevice << "\n\n"
             << "SAMPLE RATE: " << juce::String(status.sampleRate, 0) << " Hz\n"
             << "BUFFER REAL: " << status.bufferSize << " muestras\n"
             << "LATENCIA ENTRADA: " << samplesAndMs(status.inputLatencySamples, status.sampleRate) << "\n"
             << "LATENCIA SALIDA: " << samplesAndMs(status.outputLatencySamples, status.sampleRate) << "\n"
             << "IDA Y VUELTA REPORTADA: "
             << samplesAndMs(latency.driverRoundTripSamples(), status.sampleRate) << "\n"
             << "LATENCIA DE EFECTOS ACTIVA: " << samplesAndMs(activeEffectsLatencySamples, status.sampleRate) << "\n"
             << "COMPENSACION AUTOMATICA: " << samplesAndMs(latency.automaticSamples(), status.sampleRate) << "\n"
             << "OFFSET MANUAL: " << samplesAndMs(status.manualCalibrationSamples, status.sampleRate) << "\n"
             << "TOTAL EFECTIVO USADO: " << samplesAndMs(latency.effectiveSamples(), status.sampleRate) << "\n"
             << "MONITOREO ENTRADA: "
             << (status.softwareInputMonitoringEnabled ? "SOFTWARE" : "DIRECT / EXTERNO") << "\n"
             << "XRUNS / DROPOUTS: "
             << (status.xRunsAvailable ? juce::String(status.xRuns) : "NO DISPONIBLE") << "\n\n";

        if (status.aggressiveBuffer)
            text << "ESTADO: ADVERTENCIA - 64 muestras es experimental en este sistema.";
        else if (status.xRunsAvailable && status.xRuns > 0)
            text << "ESTADO: ADVERTENCIA - el driver reporto xruns.";
        else
            text << "ESTADO: ESTABLE / SIN ADVERTENCIAS REPORTADAS.";
    }
    summary.setText(text, juce::dontSendNotification);
    summary.setColour(juce::Label::textColourId,
                      status.aggressiveBuffer || (status.xRunsAvailable && status.xRuns > 0)
                          ? LnfC::OvdAmber : juce::Colour(0xffe7edf3));

    calibrationSlider.setValue(status.manualCalibrationSamples, juce::dontSendNotification);
    softwareMonitoringButton.setToggleState(status.softwareInputMonitoringEnabled,
                                             juce::dontSendNotification);
    softwareMonitoringButton.setButtonText(status.softwareInputMonitoringEnabled
        ? "MONITOREO ENTRADA: SOFTWARE" : "MONITOREO ENTRADA: DIRECT");
    softwareMonitoringButton.setEnabled(status.audioAvailable);
    controlPanelButton.setEnabled(status.audioAvailable
                                  && controller.getDeviceManager().getCurrentAudioDevice()->hasControlPanel());
    updateCalibrationReadout();
}

void AudioSettingsPanel::updateCalibrationReadout()
{
    const int samples = (int) calibrationSlider.getValue();
    const double ms = currentSampleRate > 0.0 ? 1000.0 * samples / currentSampleRate : 0.0;
    calibrationReadout.setText(
        "Positivo = grabar antes; negativo = grabar despues. Actual: "
        + juce::String(samples) + " muestras (" + juce::String(ms, 3) + " ms).",
        juce::dontSendNotification);
}

void AudioSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.86f));
    if (cardBounds.isEmpty())
        return;

    auto card = cardBounds.toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(card.translated(0.0f, 10.0f).expanded(5.0f), 18.0f);
    juce::ColourGradient background(juce::Colour(0xff242a31), card.getX(), card.getY(),
                                    juce::Colour(0xff0b0e12), card.getRight(), card.getBottom(), true);
    g.setGradientFill(background);
    g.fillRoundedRectangle(card, 16.0f);
    LoopernicoUi::drawFineRackTexture(g, card, 0.42f);
    g.setColour(LnfC::Accent.withAlpha(0.42f));
    g.drawRoundedRectangle(card.reduced(0.8f), 16.0f, 1.4f);

    if (!pluginView)
    {
        const int dividerX = cardBounds.getX() + cardBounds.getWidth() / 2;
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawVerticalLine(dividerX, (float) cardBounds.getY() + 72.0f,
                           (float) cardBounds.getBottom() - 78.0f);
    }
}

void AudioSettingsPanel::resized()
{
    const int width = juce::jmin(1240, getWidth() - 48);
    const int height = juce::jmin(810, getHeight() - 40);
    cardBounds = juce::Rectangle<int>(0, 0, juce::jmax(760, width), juce::jmax(620, height))
                     .withCentre(getLocalBounds().getCentre());

    const int pad = 28;
    const int top = cardBounds.getY() + 18;
    const int left = cardBounds.getX() + pad;
    const int right = cardBounds.getCentreX() + 22;
    const int rightWidth = cardBounds.getRight() - pad - right;

    title.setBounds(left, top, cardBounds.getWidth() - pad * 2 - 150, 34);
    closeButton.setBounds(cardBounds.getRight() - pad - 126, top, 126, 30);

    if (pluginView)
    {
        audioViewButton.setBounds(cardBounds.getRight() - pad - 270, top, 132, 30);
        pluginInstructions.setBounds(left, top + 54, cardBounds.getWidth() - pad * 2, 46);
        pluginPathsTitle.setBounds(left, top + 112, cardBounds.getWidth() - pad * 2, 22);
        pluginPathList.setBounds(left, top + 140, cardBounds.getWidth() - pad * 2, 270);

        const int actionY = top + 422;
        addPluginPathButton.setBounds(left, actionY, 170, 30);
        removePluginPathButton.setBounds(left + 182, actionY, 170, 30);
        defaultPluginPathsButton.setBounds(left + 364, actionY, 170, 30);
        scanPluginsButton.setBounds(cardBounds.getRight() - pad - 150, actionY, 150, 30);

        pluginResultsTitle.setBounds(left, top + 474, cardBounds.getWidth() - pad * 2, 22);
        pluginResults.setBounds(left, top + 502, cardBounds.getWidth() - pad * 2, 32);
        pluginScanStatus.setBounds(left, top + 548, cardBounds.getWidth() - pad * 2, 26);
        return;
    }

    selector.setBounds(left, top + 52, cardBounds.getWidth() / 2 - pad - 18,
                       cardBounds.getHeight() - 142);
    summary.setBounds(right, top + 54, rightWidth, 330);
    calibrationTitle.setBounds(right, top + 390, rightWidth, 22);
    calibrationSlider.setBounds(right, top + 414, rightWidth, 34);
    calibrationReadout.setBounds(right, top + 450, rightWidth, 24);
    automaticButton.setBounds(right, top + 480, 190, 28);
    softwareMonitoringButton.setBounds(right + 202, top + 480, rightWidth - 202, 28);

    physicalLoopbackNote.setBounds(right, top + 520, rightWidth, 48);
    directMonitoringNote.setBounds(right, top + 572, rightWidth, 74);
    actionStatus.setBounds(right, cardBounds.getBottom() - 72, rightWidth, 24);

    const int buttonsY = cardBounds.getBottom() - 40;
    recommendedButton.setBounds(left, buttonsY, 210, 28);
    controlPanelButton.setBounds(left + 222, buttonsY, 170, 28);
    pluginViewButton.setBounds(left + 404, buttonsY, 150, 28);
}

bool AudioSettingsPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose)
            onClose();
        return true;
    }
    return false;
}
