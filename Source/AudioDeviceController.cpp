#include "AudioDeviceController.h"

#include <limits>

namespace
{
    constexpr auto kSettingsTag = "LOOPERNICO_AUDIO_SETTINGS";
    constexpr auto kDeviceSetupTag = "DEVICESETUP";
    constexpr auto kCalibrationTag = "CALIBRATION";
    constexpr auto kMonitoringTag = "MONITORING";
    constexpr auto kAsioType = "ASIO";
    constexpr auto kAudioBoxDriver = "AudioBox ASIO Driver";
    constexpr auto kWasapiLowLatencyType = "Windows Audio (Low Latency Mode)";
    constexpr auto kWasapiSharedType = "Windows Audio";

    bool containsName(const juce::StringArray& names, const juce::String& wanted)
    {
        for (const auto& name : names)
            if (name.trim().equalsIgnoreCase(wanted.trim()))
                return true;
        return false;
    }

    juce::String stateInputName(const juce::XmlElement& state)
    {
        const auto common = state.getStringAttribute("audioDeviceName");
        return common.isNotEmpty() ? common : state.getStringAttribute("audioInputDeviceName");
    }

    juce::String stateOutputName(const juce::XmlElement& state)
    {
        const auto common = state.getStringAttribute("audioDeviceName");
        return common.isNotEmpty() ? common : state.getStringAttribute("audioOutputDeviceName");
    }
}

AudioDeviceController::AudioDeviceController(juce::AudioDeviceManager& managerToUse)
    : manager(managerToUse),
      settingsFile(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Loopernico")
                       .getChildFile("audio-settings.xml"))
{
    const auto logDirectory = settingsFile.getParentDirectory().getChildFile("Logs");
    logDirectory.createDirectory();
    fileLogger = std::make_unique<juce::FileLogger>(logDirectory.getChildFile("audio.log"),
                                                    "Loopernico audio diagnostics",
                                                    1024 * 1024);
    loadSettings();
}

AudioDeviceController::~AudioDeviceController()
{
    flushPendingSettings();
}

void AudioDeviceController::loadSettings()
{
    if (settingsFile.existsAsFile())
        settings = juce::parseXML(settingsFile);

    if (settings == nullptr || !settings->hasTagName(kSettingsTag))
        settings = std::make_unique<juce::XmlElement>(kSettingsTag);

    settings->setAttribute("version", 2);
}

juce::AudioIODeviceType* AudioDeviceController::findDeviceType(const juce::String& typeName) const
{
    for (auto* type : manager.getAvailableDeviceTypes())
        if (type != nullptr && type->getTypeName().equalsIgnoreCase(typeName))
            return type;
    return nullptr;
}

bool AudioDeviceController::validateSavedState(const juce::XmlElement& state,
                                               juce::String& reason) const
{
    if (!state.hasTagName(kDeviceSetupTag))
    {
        reason = "El estado guardado no contiene DEVICESETUP.";
        return false;
    }

    auto* type = findDeviceType(state.getStringAttribute("deviceType"));
    if (type == nullptr)
    {
        reason = "El tipo de driver guardado ya no esta disponible.";
        return false;
    }

    type->scanForDevices();
    const auto inputName = stateInputName(state);
    const auto outputName = stateOutputName(state);
    if (inputName.isEmpty() || !containsName(type->getDeviceNames(true), inputName))
    {
        reason = "El dispositivo de entrada guardado no esta disponible: " + inputName;
        return false;
    }
    if (outputName.isEmpty() || !containsName(type->getDeviceNames(false), outputName))
    {
        reason = "El dispositivo de salida guardado no esta disponible: " + outputName;
        return false;
    }

    reason.clear();
    return true;
}

std::unique_ptr<juce::XmlElement> AudioDeviceController::makeDeviceState(
    const juce::String& typeName,
    const juce::String& inputName,
    const juce::String& outputName,
    double sampleRate,
    int bufferSize) const
{
    auto state = std::make_unique<juce::XmlElement>(kDeviceSetupTag);
    state->setAttribute("deviceType", typeName);
    state->setAttribute("audioInputDeviceName", inputName);
    state->setAttribute("audioOutputDeviceName", outputName);
    state->setAttribute("audioDeviceRate", sampleRate);
    state->setAttribute("audioDeviceBufferSize", bufferSize);
    state->setAttribute("audioDeviceInChans", "11");
    state->setAttribute("audioDeviceOutChans", "11");
    return state;
}

std::unique_ptr<juce::XmlElement> AudioDeviceController::makeRecommendedAsioState() const
{
    auto* type = findDeviceType(kAsioType);
    if (type == nullptr)
        return {};

    type->scanForDevices();
    if (!containsName(type->getDeviceNames(true), kAudioBoxDriver)
        || !containsName(type->getDeviceNames(false), kAudioBoxDriver))
        return {};

    return makeDeviceState(kAsioType, kAudioBoxDriver, kAudioBoxDriver, 48000.0,
                           preferredStartupBufferSamples);
}

juce::String AudioDeviceController::chooseSafeEndpoint(const juce::StringArray& names,
                                                       bool input) const
{
    int bestScore = std::numeric_limits<int>::min();
    juce::String best;

    for (int index = 0; index < names.size(); ++index)
    {
        const auto name = names[index];
        int score = -index;
        if (name.containsIgnoreCase("AudioBox") || name.containsIgnoreCase("PreSonus")) score += 1000;
        if (name.containsIgnoreCase("USB")) score += 100;
        if (input && (name.containsIgnoreCase("Line") || name.containsIgnoreCase("Microphone"))) score += 30;
        if (!input && (name.containsIgnoreCase("Speaker") || name.containsIgnoreCase("Headphone"))) score += 30;
        if (name.containsIgnoreCase("HDMI") || name.containsIgnoreCase("NVIDIA")
            || name.containsIgnoreCase("Display Audio")) score -= 1000;

        if (score > bestScore)
        {
            bestScore = score;
            best = name;
        }
    }
    return best;
}

std::unique_ptr<juce::XmlElement> AudioDeviceController::makeWindowsFallbackState(
    const juce::String& typeName,
    double sampleRate,
    int bufferSize) const
{
    auto* type = findDeviceType(typeName);
    if (type == nullptr)
        return {};

    type->scanForDevices();
    const auto input = chooseSafeEndpoint(type->getDeviceNames(true), true);
    const auto output = chooseSafeEndpoint(type->getDeviceNames(false), false);
    if (input.isEmpty() || output.isEmpty())
        return {};

    return makeDeviceState(typeName, input, output, sampleRate, bufferSize);
}

bool AudioDeviceController::tryOpenState(const juce::XmlElement& state,
                                         const juce::String& description,
                                         juce::String& errorMessage)
{
    juce::XmlElement startupState(state);
    startupState.setAttribute("audioDeviceBufferSize", preferredStartupBufferSamples);

    juce::String validationError;
    if (!validateSavedState(startupState, validationError))
    {
        errorMessage = validationError;
        log(description + " omitida: " + validationError);
        return false;
    }

    errorMessage = manager.initialise(2, 2, &startupState, false);
    if (errorMessage.isNotEmpty() || manager.getCurrentAudioDevice() == nullptr)
    {
        if (errorMessage.isEmpty())
            errorMessage = "El driver no creo un dispositivo de audio activo.";
        log(description + " fallo: " + errorMessage);
        return false;
    }

    if (!applyPreferredStartupBuffer(errorMessage))
    {
        log(description + " no pudo fijar el buffer de inicio: " + errorMessage);
        return false;
    }

    logCurrentDevice(description + " activa");
    return true;
}

bool AudioDeviceController::applyPreferredStartupBuffer(juce::String& errorMessage)
{
    auto* device = manager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        errorMessage = "No hay un dispositivo de audio activo.";
        return false;
    }

    if (device->getCurrentBufferSizeSamples() != preferredStartupBufferSamples)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.bufferSize = preferredStartupBufferSamples;
        errorMessage = manager.setAudioDeviceSetup(setup, true);
        if (errorMessage.isNotEmpty())
            return false;

        device = manager.getCurrentAudioDevice();
    }

    const int actualBuffer = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    if (actualBuffer != preferredStartupBufferSamples)
    {
        errorMessage = "El driver devolvio " + juce::String(actualBuffer)
                     + " muestras al solicitar " + juce::String(preferredStartupBufferSamples) + ".";
        return false;
    }

    errorMessage.clear();
    return true;
}

AudioDeviceController::InitialisationResult AudioDeviceController::initialise(
    bool forceFallbackForDiagnostics)
{
    initialisationResult = {};
    automaticFallbackSignature.clear();
    juce::String error;

    // Force JUCE to enumerate every compiled backend before validating a saved
    // device type. This includes ASIO only when JUCE_ASIO was enabled at build time.
    manager.getAvailableDeviceTypes();

    if (!forceFallbackForDiagnostics)
    {
        if (auto* saved = settings->getChildByName(kDeviceSetupTag))
        {
            if (tryOpenState(*saved, "Configuracion guardada", error))
            {
                initialisationResult.audioAvailable = true;
                initialisationResult.restoredSavedConfiguration = true;
                initialisationResult.message = "Configuracion de audio restaurada.";
                lastObservedSignature = currentDeviceSignature();
                refreshCachedDevicePreferences();
                persistCurrentDevicePreference();
                return initialisationResult;
            }
            initialisationResult.message = "No se pudo restaurar la configuracion guardada: " + error;
        }
    }

    if (!forceFallbackForDiagnostics)
    {
        if (auto recommended = makeRecommendedAsioState())
        {
            if (tryOpenState(*recommended, "ASIO recomendado (AudioBox, 48 kHz, 256)", error))
            {
                initialisationResult.audioAvailable = true;
                initialisationResult.message = "AudioBox ASIO activa a 48 kHz / 256 muestras.";
                lastObservedSignature = currentDeviceSignature();
                refreshCachedDevicePreferences();
                persistCurrentDevicePreference();
                return initialisationResult;
            }
            initialisationResult.message << " ASIO recomendado fallo: " << error;
        }
        else
        {
            const auto reason = findDeviceType(kAsioType) == nullptr
                ? "El backend ASIO no esta disponible en este build."
                : "AudioBox ASIO Driver no esta disponible.";
            log(reason);
            initialisationResult.message << " " << reason;
        }
    }
    else
    {
        log("Diagnostico: se omiten configuracion guardada y ASIO recomendado para probar fallback.");
    }

    const auto tryFallback = [this, &error](const juce::String& typeName,
                                            const juce::String& label)
    {
        if (auto requested = makeWindowsFallbackState(typeName, 48000.0,
                                                       preferredStartupBufferSamples))
            if (tryOpenState(*requested, label + " 48 kHz / 256", error))
                return true;

        if (auto deviceDefault = makeWindowsFallbackState(typeName, 0.0,
                                                           preferredStartupBufferSamples))
            if (tryOpenState(*deviceDefault, label + " con sample rate seguro / 256", error))
                return true;
        return false;
    };

    if (tryFallback(kWasapiLowLatencyType, "Fallback WASAPI Low Latency")
        || tryFallback(kWasapiSharedType, "Fallback WASAPI compartido"))
    {
        initialisationResult.audioAvailable = true;
        initialisationResult.usingFallback = true;
        initialisationResult.message = "Se uso un fallback recuperable: "
                                     + manager.getCurrentAudioDeviceType()
                                     + ". Abri AUDIO para revisar el dispositivo.";
        automaticFallbackSignature = currentDeviceSignature();
        lastObservedSignature = automaticFallbackSignature;
        refreshCachedDevicePreferences();
        return initialisationResult;
    }

    error = manager.initialiseWithDefaultDevices(2, 2);
    initialisationResult.audioAvailable = error.isEmpty() && manager.getCurrentAudioDevice() != nullptr;
    if (initialisationResult.audioAvailable && !applyPreferredStartupBuffer(error))
        initialisationResult.audioAvailable = false;
    initialisationResult.usingFallback = true;
    initialisationResult.message = initialisationResult.audioAvailable
        ? "Se uso el dispositivo seguro disponible. Revisalo en AUDIO."
        : "No se pudo abrir ningun dispositivo de audio: " + error;
    automaticFallbackSignature = currentDeviceSignature();
    lastObservedSignature = automaticFallbackSignature;
    refreshCachedDevicePreferences();
    log(initialisationResult.message);
    if (initialisationResult.audioAvailable)
        logCurrentDevice("Fallback final activo");
    return initialisationResult;
}

juce::String AudioDeviceController::restoreRecommendedConfiguration()
{
    std::unique_ptr<juce::XmlElement> previous;
    if (auto state = manager.createStateXml())
        previous = std::move(state);

    auto recommended = makeRecommendedAsioState();
    if (recommended == nullptr)
        return "AudioBox ASIO Driver no esta disponible.";

    juce::String error;
    if (!tryOpenState(*recommended, "Restauracion recomendada", error))
    {
        if (previous != nullptr)
        {
            juce::String restoreError;
            if (!tryOpenState(*previous, "Restauracion del dispositivo anterior", restoreError))
                log("Tampoco se pudo reabrir el dispositivo anterior: " + restoreError);
        }
        return error;
    }

    initialisationResult.audioAvailable = true;
    initialisationResult.usingFallback = false;
    initialisationResult.message = "AudioBox ASIO restaurada a 48 kHz / 256 muestras.";
    automaticFallbackSignature.clear();
    refreshCachedDevicePreferences();
    persistCurrentDevicePreference();
    return {};
}

void AudioDeviceController::persistCurrentDevicePreference()
{
    auto current = manager.createStateXml();
    if (current == nullptr || manager.getCurrentAudioDevice() == nullptr)
        return;

    if (auto* old = settings->getChildByName(kDeviceSetupTag))
        settings->removeChildElement(old, true);
    settings->prependChildElement(new juce::XmlElement(*current));
    markSettingsDirty();
}

void AudioDeviceController::handleDeviceManagerChange()
{
    const auto signature = currentDeviceSignature();
    if (signature != lastObservedSignature)
    {
        lastObservedSignature = signature;
        logCurrentDevice("Cambio de dispositivo");
        refreshCachedDevicePreferences();

        // Do not turn an automatic fallback into the user's preference. JUCE's
        // state XML still records explicit choices made in the selector.
        if (signature != automaticFallbackSignature)
        {
            initialisationResult.usingFallback = false;
            automaticFallbackSignature.clear();
            persistCurrentDevicePreference();
        }
    }
}

void AudioDeviceController::poll()
{
    handleDeviceManagerChange();
    if (auto* device = manager.getCurrentAudioDevice())
    {
        const auto error = device->getLastError();
        if (error.isNotEmpty() && error != lastObservedError)
        {
            lastObservedError = error;
            log("Error de ejecucion del dispositivo: " + error);
        }
    }

    if (settingsDirty && juce::Time::currentTimeMillis() - settingsDirtySinceMs >= 400)
        saveSettingsNow();
}

AudioDeviceController::Status AudioDeviceController::getStatus() const
{
    Status status;
    status.usingFallback = initialisationResult.usingFallback;
    if (auto* device = manager.getCurrentAudioDevice())
    {
        const auto setup = manager.getAudioDeviceSetup();
        status.audioAvailable = true;
        status.driverType = manager.getCurrentAudioDeviceType();
        status.inputDevice = setup.inputDeviceName;
        status.outputDevice = setup.outputDeviceName;
        status.sampleRate = device->getCurrentSampleRate();
        status.bufferSize = device->getCurrentBufferSizeSamples();
        status.inputLatencySamples = juce::jmax(0, device->getInputLatencyInSamples());
        status.outputLatencySamples = juce::jmax(0, device->getOutputLatencyInSamples());
        status.xRuns = device->getXRunCount();
        status.xRunsAvailable = status.xRuns >= 0;
        status.lastError = device->getLastError();
        status.aggressiveBuffer = status.bufferSize > 0 && status.bufferSize <= 64;
    }
    status.manualCalibrationEnabled = isManualCalibrationEnabled();
    status.manualCalibrationSamples = getManualCalibrationSamples();
    status.softwareInputMonitoringEnabled = isSoftwareInputMonitoringEnabled();
    return status;
}

juce::String AudioDeviceController::currentDeviceKey() const
{
    if (manager.getCurrentAudioDevice() == nullptr)
        return "sin-dispositivo";

    const auto setup = manager.getAudioDeviceSetup();
    return manager.getCurrentAudioDeviceType() + "|"
         + setup.inputDeviceName + "|" + setup.outputDeviceName;
}

juce::String AudioDeviceController::currentDeviceSignature() const
{
    auto* device = manager.getCurrentAudioDevice();
    if (device == nullptr)
        return "sin-dispositivo";
    const auto setup = manager.getAudioDeviceSetup();
    return currentDeviceKey() + "|" + juce::String(device->getCurrentSampleRate(), 2)
         + "|" + juce::String(device->getCurrentBufferSizeSamples())
         + "|" + setup.inputChannels.toString(2)
         + "|" + setup.outputChannels.toString(2);
}

juce::XmlElement* AudioDeviceController::findCalibrationElement(const juce::String& key) const
{
    for (auto* child = settings->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        if (child->hasTagName(kCalibrationTag)
            && child->getStringAttribute("deviceKey") == key)
            return child;
    return nullptr;
}

juce::XmlElement* AudioDeviceController::findMonitoringElement(const juce::String& key) const
{
    for (auto* child = settings->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        if (child->hasTagName(kMonitoringTag)
            && child->getStringAttribute("deviceKey") == key)
            return child;
    return nullptr;
}

void AudioDeviceController::refreshCachedDevicePreferences()
{
    const auto key = currentDeviceKey();
    const auto* monitoring = findMonitoringElement(key);
    softwareInputMonitoringEnabled.store(
        monitoring == nullptr || monitoring->getBoolAttribute("enabled", true),
        std::memory_order_relaxed);
}

void AudioDeviceController::setManualCalibrationSamples(int samples)
{
    const auto key = currentDeviceKey();
    auto* calibration = findCalibrationElement(key);
    if (calibration == nullptr)
    {
        calibration = new juce::XmlElement(kCalibrationTag);
        calibration->setAttribute("deviceKey", key);
        settings->addChildElement(calibration);
    }

    calibration->setAttribute("enabled", true);
    calibration->setAttribute("offsetSamples",
                              juce::jlimit(minimumCalibrationSamples,
                                           maximumCalibrationSamples, samples));
    markSettingsDirty();
}

void AudioDeviceController::resetManualCalibration()
{
    if (auto* calibration = findCalibrationElement(currentDeviceKey()))
    {
        calibration->setAttribute("enabled", false);
        calibration->setAttribute("offsetSamples", 0);
        markSettingsDirty();
    }
}

int AudioDeviceController::getManualCalibrationSamples() const
{
    if (auto* calibration = findCalibrationElement(currentDeviceKey()))
        if (calibration->getBoolAttribute("enabled"))
            return juce::jlimit(minimumCalibrationSamples,
                                maximumCalibrationSamples,
                                calibration->getIntAttribute("offsetSamples"));
    return 0;
}

bool AudioDeviceController::isManualCalibrationEnabled() const
{
    if (auto* calibration = findCalibrationElement(currentDeviceKey()))
        return calibration->getBoolAttribute("enabled");
    return false;
}

void AudioDeviceController::setSoftwareInputMonitoringEnabled(bool enabled)
{
    if (manager.getCurrentAudioDevice() == nullptr)
        return;

    const auto key = currentDeviceKey();
    auto* monitoring = findMonitoringElement(key);
    if (monitoring == nullptr)
    {
        monitoring = new juce::XmlElement(kMonitoringTag);
        monitoring->setAttribute("deviceKey", key);
        settings->addChildElement(monitoring);
    }

    monitoring->setAttribute("enabled", enabled);
    softwareInputMonitoringEnabled.store(enabled, std::memory_order_relaxed);
    markSettingsDirty();
    log(juce::String("Monitoreo de entrada por software ")
        + (enabled ? "activado" : "desactivado") + " para " + key + ".");
}

bool AudioDeviceController::showCurrentDeviceControlPanel()
{
    auto* device = manager.getCurrentAudioDevice();
    if (device == nullptr || !device->hasControlPanel())
        return false;

    const bool shown = device->showControlPanel();
    handleDeviceManagerChange();
    return shown;
}

void AudioDeviceController::markSettingsDirty()
{
    settingsDirty = true;
    settingsDirtySinceMs = juce::Time::currentTimeMillis();
}

void AudioDeviceController::saveSettingsNow()
{
    settingsFile.getParentDirectory().createDirectory();
    if (!settings->writeTo(settingsFile))
        log("No se pudo guardar " + settingsFile.getFullPathName());
    settingsDirty = false;
}

void AudioDeviceController::flushPendingSettings()
{
    if (settingsDirty)
        saveSettingsNow();
}

juce::File AudioDeviceController::getLogFile() const
{
    return fileLogger != nullptr ? fileLogger->getLogFile() : juce::File();
}

void AudioDeviceController::log(const juce::String& message)
{
    const auto timestamped = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S")
                           + "  " + message;
    if (fileLogger != nullptr)
        fileLogger->logMessage(timestamped);
    juce::Logger::writeToLog(timestamped);
}

void AudioDeviceController::logCurrentDevice(const juce::String& prefix)
{
    const auto status = getStatus();
    if (!status.audioAvailable)
    {
        log(prefix + ": sin dispositivo activo.");
        return;
    }

    log(prefix + ": driver=" + status.driverType
        + ", entrada=" + status.inputDevice
        + ", salida=" + status.outputDevice
        + ", sampleRate=" + juce::String(status.sampleRate, 0)
        + ", bufferReal=" + juce::String(status.bufferSize)
        + ", latEntrada=" + juce::String(status.inputLatencySamples)
        + ", latSalida=" + juce::String(status.outputLatencySamples)
        + ", xruns=" + (status.xRunsAvailable ? juce::String(status.xRuns) : "no disponible"));
}
