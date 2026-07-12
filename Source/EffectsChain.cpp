#include "EffectsChain.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float kPitchLookbackSamples = 128.0f;
    constexpr float kDrumMixGain = 0.92f;
    constexpr int kRhythmFillStartStep = 12;

    constexpr const char* kRhythmStyleFolders[] =
    {
        "Rock",
        "Funk",
        "Disco",
        "Latin",
        "HipHop",
        "House",
        "BossaNova",
        "Techno",
        "Blues"
    };

    int variationIndexForSection(EffectsChain::RhythmSection section)
    {
        switch (section)
        {
            case EffectsChain::RhythmSection::Verse:  return 0;
            case EffectsChain::RhythmSection::Chorus: return 1;
            case EffectsChain::RhythmSection::Fill:   return 2;
        }

        return 0;
    }

    void clampPattern(std::array<float, 16>& pattern)
    {
        for (auto& value : pattern)
            value = juce::jlimit(0.0f, 1.0f, value);
    }

    void applyChorusLift(std::array<float, 16>& kickPattern,
                         std::array<float, 16>& snarePattern,
                         std::array<float, 16>& hatPattern)
    {
        for (int i = 0; i < 16; ++i)
            hatPattern[(size_t) i] = juce::jmax(hatPattern[(size_t) i], (i % 2 == 0) ? 0.30f : 0.16f);

        kickPattern[0]  = juce::jmax(kickPattern[0], 0.96f);
        kickPattern[8]  = juce::jmax(kickPattern[8], 0.90f);
        kickPattern[10] = juce::jmax(kickPattern[10], 0.28f);
        kickPattern[14] = juce::jmax(kickPattern[14], 0.24f);
        snarePattern[4] = juce::jmax(snarePattern[4], 0.84f);
        snarePattern[12]= juce::jmax(snarePattern[12], 0.94f);
        snarePattern[15]= juce::jmax(snarePattern[15], 0.18f);
    }

    void applyResolvingFill(std::array<float, 16>& kickPattern,
                            std::array<float, 16>& snarePattern,
                            std::array<float, 16>& hatPattern)
    {
        for (int i = 0; i < kRhythmFillStartStep; ++i)
        {
            kickPattern[(size_t) i] = 0.0f;
            snarePattern[(size_t) i] = 0.0f;
            hatPattern[(size_t) i] = 0.0f;
        }

        hatPattern[12] = 0.34f;
        hatPattern[13] = 0.40f;
        hatPattern[14] = 0.54f;
        hatPattern[15] = 0.66f;
        kickPattern[12] = 0.82f;
        kickPattern[14] = 0.34f;
        kickPattern[15] = 0.98f;
        snarePattern[13] = 0.42f;
        snarePattern[14] = 0.62f;
        snarePattern[15] = 0.92f;
    }
}

EffectsChain::EffectsChain()
    : juce::Thread("Rhythm sample loader")
{
    for (int i = 0; i < kPedalCount; ++i)
        pedalOrder[(size_t) i].store(i);

    juce::addDefaultFormatsToManager(pluginFormatManager);
    rhythmSampleFormatManager.registerBasicFormats();
    rhythmSampleBank = std::make_shared<RhythmSampleBank>();
    startThread(juce::Thread::Priority::low);
}

EffectsChain::~EffectsChain()
{
    signalThreadShouldExit();
    notify();
    stopThread(5000);
}

EffectsChain::PreparedExternalPlugin::~PreparedExternalPlugin()
{
    release();
}

EffectsChain::PreparedExternalPlugin::PreparedExternalPlugin(PreparedExternalPlugin&& other) noexcept
{
    *this = std::move(other);
}

EffectsChain::PreparedExternalPlugin& EffectsChain::PreparedExternalPlugin::operator=(PreparedExternalPlugin&& other) noexcept
{
    if (this != &other)
    {
        release();
        instance = std::move(other.instance);
        scratchBuffer = std::move(other.scratchBuffer);
        midiBuffer = std::move(other.midiBuffer);
        inputChannels = other.inputChannels;
        outputChannels = other.outputChannels;
        name = std::move(other.name);
        path = std::move(other.path);
        enabled = other.enabled;
        other.inputChannels = 0;
        other.outputChannels = 0;
    }
    return *this;
}

void EffectsChain::PreparedExternalPlugin::release() noexcept
{
    if (instance != nullptr)
        instance->releaseResources();
    instance.reset();
}

bool EffectsChain::loadExternalVst3(const juce::File& pluginFile, juce::String& errorMessage)
{
    PreparedExternalPlugin candidate;
    if (!prepareExternalVst3(pluginFile, candidate, errorMessage))
    {
        // Compatibilidad: la API historica descargaba el plugin actual aun si
        // el nuevo archivo no podia abrirse.
        unloadExternalPlugin();
        return false;
    }

    auto retired = commitExternalPlugin(std::move(candidate));
    return true;
}

bool EffectsChain::prepareExternalVst3(const juce::File& pluginFile,
                                       PreparedExternalPlugin& candidate,
                                       juce::String& errorMessage,
                                       const juce::MemoryBlock* stateToRestore)
{
    PreparedExternalPlugin prepared;

    if (!pluginFile.exists())
    {
        errorMessage = "No se encontro el archivo VST3 seleccionado.";
        return false;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    juce::AudioPluginFormat* vst3HostFormat = nullptr;

    for (auto* format : pluginFormatManager.getFormats())
    {
        if (format != nullptr && format->getName().containsIgnoreCase("VST3"))
        {
            vst3HostFormat = format;
            break;
        }
    }

    if (vst3HostFormat == nullptr)
    {
        errorMessage = "Este build no tiene habilitado el host VST3.";
        return false;
    }

    vst3HostFormat->findAllTypesForFile(descriptions, pluginFile.getFullPathName());

    if (descriptions.isEmpty())
    {
        errorMessage = "Ese archivo no expone un plugin VST3 valido.";
        return false;
    }

    prepared.instance = pluginFormatManager.createPluginInstance(*descriptions.getFirst(),
                                                                 sampleRate_,
                                                                 maxBlockSize_,
                                                                 errorMessage);

    if (prepared.instance == nullptr)
        return false;

    prepared.instance->enableAllBuses();
    prepared.instance->setRateAndBufferSizeDetails(sampleRate_, maxBlockSize_);
    prepared.instance->prepareToPlay(sampleRate_, maxBlockSize_);
    prepared.instance->reset();

    if (stateToRestore != nullptr && stateToRestore->getSize() > 0)
        prepared.instance->setStateInformation(stateToRestore->getData(), (int) stateToRestore->getSize());

    prepared.inputChannels = prepared.instance->getTotalNumInputChannels();
    prepared.outputChannels = prepared.instance->getTotalNumOutputChannels();
    const int requiredChannels = juce::jmax(1, juce::jmax(prepared.inputChannels,
                                                          prepared.outputChannels));
    prepared.scratchBuffer.setSize(requiredChannels, maxBlockSize_, false, true, false);
    prepared.scratchBuffer.clear();
    prepared.midiBuffer.ensureSize(64 * 1024);
    prepared.midiBuffer.clear();
    prepared.name = prepared.instance->getName();
    prepared.path = pluginFile.getFullPathName();
    prepared.enabled = true;

    candidate = std::move(prepared);
    errorMessage.clear();
    return true;
}

EffectsChain::PreparedExternalPlugin EffectsChain::commitExternalPlugin(PreparedExternalPlugin&& candidate) noexcept
{
    PreparedExternalPlugin retired;
    std::swap(retired.instance, externalPluginInstance);
    std::swap(retired.scratchBuffer, externalPluginScratchBuffer);
    std::swap(retired.midiBuffer, externalPluginMidiBuffer);
    std::swap(retired.inputChannels, externalPluginInputChannels);
    std::swap(retired.outputChannels, externalPluginOutputChannels);
    std::swap(retired.name, externalPluginName);
    std::swap(retired.path, externalPluginPath);
    retired.enabled = externalPluginEnabled.exchange(candidate.enabled);

    std::swap(candidate.instance, externalPluginInstance);
    std::swap(candidate.scratchBuffer, externalPluginScratchBuffer);
    std::swap(candidate.midiBuffer, externalPluginMidiBuffer);
    std::swap(candidate.inputChannels, externalPluginInputChannels);
    std::swap(candidate.outputChannels, externalPluginOutputChannels);
    std::swap(candidate.name, externalPluginName);
    std::swap(candidate.path, externalPluginPath);
    return retired;
}

EffectsChain::PreparedExternalPlugin EffectsChain::detachExternalPlugin() noexcept
{
    PreparedExternalPlugin empty;
    empty.enabled = false;
    return commitExternalPlugin(std::move(empty));
}

void EffectsChain::unloadExternalPlugin()
{
    auto retired = detachExternalPlugin();
}

bool EffectsChain::copyExternalPluginState(juce::MemoryBlock& state) const
{
    state.reset();

    auto* plugin = externalPluginInstance.get();
    if (plugin == nullptr)
        return false;

    plugin->getStateInformation(state);
    return state.getSize() > 0;
}

bool EffectsChain::restoreExternalPluginState(const juce::MemoryBlock& state, juce::String& errorMessage)
{
    auto* plugin = externalPluginInstance.get();
    if (plugin == nullptr)
    {
        errorMessage = "No hay un VST3 cargado para restaurar su estado.";
        return false;
    }

    if (state.getSize() == 0)
    {
        errorMessage.clear();
        return true;
    }

    plugin->setStateInformation(state.getData(), (int) state.getSize());
    errorMessage.clear();
    return true;
}

void EffectsChain::setPedalOrder(const std::array<int, kPedalCount>& order)
{
    for (int i = 0; i < kPedalCount; ++i)
        pedalOrder[(size_t) i].store(juce::jlimit(0, kPedalCount - 1, order[(size_t) i]));
}

EffectsChain::RhythmSampleSlot EffectsChain::sampleSlotForInstrument(DrumInstrument instrument)
{
    switch (instrument)
    {
        case DrumInstrument::Kick:  return RhythmSampleSlot::Kick;
        case DrumInstrument::Snare: return RhythmSampleSlot::Snare;
        case DrumInstrument::Hat:   return RhythmSampleSlot::Hat;
    }

    return RhythmSampleSlot::Kick;
}

void EffectsChain::triggerRhythmSample(RhythmSampleSlot slot, float velocity)
{
    auto& pool = rhythmSampleBank->pools[(size_t) slot];
    if (pool.entries.empty())
        return;

    auto& voice = rhythmSampleVoices[(size_t) slot];
    auto& chosen = pool.entries[(size_t) pool.nextIndex];
    voice.active = true;
    voice.position = 0;
    voice.velocity = juce::jlimit(0.05f, 1.2f, velocity);
    voice.sample = &chosen;
    pool.nextIndex = (pool.nextIndex + 1) % (int) pool.entries.size();
}

void EffectsChain::triggerDrumVoice(DrumInstrument instrument, float velocity)
{
    const auto slot = sampleSlotForInstrument(instrument);
    triggerRhythmSample(slot, velocity);
    if (!rhythmSampleBank->pools[(size_t) slot].entries.empty())
        return;

    DrumVoice* voice = nullptr;
    switch (instrument)
    {
        case DrumInstrument::Kick:  voice = &kickVoice;  break;
        case DrumInstrument::Snare: voice = &snareVoice; break;
        case DrumInstrument::Hat:   voice = &hatVoice;   break;
    }

    if (voice == nullptr)
        return;

    voice->active = true;
    voice->env = juce::jlimit(0.05f, 1.0f, velocity);
    voice->phase = 0.0f;
    voice->tone = 0.0f;
    voice->velocity = velocity;
    voice->noiseState = 0.0f;

    switch (instrument)
    {
        case DrumInstrument::Kick:
            voice->baseFreq = 52.0f + velocity * 20.0f;
            voice->pitchSweep = 118.0f + velocity * 46.0f;
            break;
        case DrumInstrument::Snare:
            voice->baseFreq = 210.0f + velocity * 20.0f;
            voice->pitchSweep = 0.0f;
            break;
        case DrumInstrument::Hat:
            voice->baseFreq = 3100.0f + velocity * 900.0f;
            voice->pitchSweep = 0.0f;
            break;
    }
}

float EffectsChain::renderRhythmSample(RhythmSampleSlot slot, int channel)
{
    auto& voice = rhythmSampleVoices[(size_t) slot];
    if (!voice.active || voice.sample == nullptr)
        return 0.0f;

    const auto& sample = *voice.sample;
    const int numChannels = sample.audio.getNumChannels();
    const int numSamples = sample.audio.getNumSamples();
    if (numChannels <= 0 || numSamples <= 0 || voice.position >= numSamples)
    {
        voice.active = false;
        voice.sample = nullptr;
        return 0.0f;
    }

    const int sourceChannel = juce::jlimit(0, numChannels - 1, channel);
    float out = sample.audio.getSample(sourceChannel, voice.position);
    if (numChannels == 1 && channel == 1)
        out = sample.audio.getSample(0, voice.position);

    return out * voice.velocity;
}

void EffectsChain::advanceRhythmSampleVoices()
{
    for (auto& voice : rhythmSampleVoices)
    {
        if (!voice.active || voice.sample == nullptr)
            continue;

        ++voice.position;
        if (voice.position >= voice.sample->audio.getNumSamples())
        {
            voice.active = false;
            voice.sample = nullptr;
        }
    }
}

std::array<float, 2> EffectsChain::renderDrumFrame(DrumInstrument instrument)
{
    const auto slot = sampleSlotForInstrument(instrument);
    if (!rhythmSampleBank->pools[(size_t) slot].entries.empty())
        return { renderRhythmSample(slot, 0), renderRhythmSample(slot, 1) };

    DrumVoice* voice = nullptr;
    switch (instrument)
    {
        case DrumInstrument::Kick:  voice = &kickVoice;  break;
        case DrumInstrument::Snare: voice = &snareVoice; break;
        case DrumInstrument::Hat:   voice = &hatVoice;   break;
    }

    if (voice == nullptr || !voice->active || voice->env <= 0.0001f)
        return {};

    const float white = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    float sample = 0.0f;

    if (instrument == DrumInstrument::Kick)
    {
        const float sweep = voice->pitchSweep * (0.15f + voice->env * voice->env);
        const float freq = voice->baseFreq + sweep;
        voice->phase += juce::MathConstants<float>::twoPi * freq / (float)sampleRate_;
        if (voice->phase > juce::MathConstants<float>::twoPi)
            voice->phase -= juce::MathConstants<float>::twoPi;

        const float body = std::sin(voice->phase);
        const float sub  = std::sin(voice->phase * 0.5f);
        voice->tone += 0.18f * (white - voice->tone);
        const float click = (white - voice->tone) * voice->env * 0.08f;
        sample = (body * 0.74f + sub * 0.18f + click) * voice->env;
        sample = std::tanh(sample * 1.6f);
        voice->env *= 0.9892f;
    }
    else if (instrument == DrumInstrument::Snare)
    {
        voice->phase += juce::MathConstants<float>::twoPi * voice->baseFreq / (float)sampleRate_;
        if (voice->phase > juce::MathConstants<float>::twoPi)
            voice->phase -= juce::MathConstants<float>::twoPi;

        const float ring = std::sin(voice->phase) * 0.42f + std::sin(voice->phase * 1.97f) * 0.16f;
        voice->tone += 0.12f * (ring - voice->tone);
        voice->noiseState += 0.72f * (white - voice->noiseState);
        const float crack = white - voice->noiseState * 0.82f;
        sample = (crack * 0.76f + voice->tone * 0.38f) * voice->env;
        sample = std::tanh(sample * 1.55f);
        voice->env *= 0.9835f;
    }
    else
    {
        const float oscA = std::sin(voice->phase);
        const float oscB = std::sin(voice->phase * 1.618f);
        voice->phase += juce::MathConstants<float>::twoPi * voice->baseFreq / (float)sampleRate_;
        if (voice->phase > juce::MathConstants<float>::twoPi)
            voice->phase -= juce::MathConstants<float>::twoPi;

        voice->noiseState = 0.18f * voice->noiseState + 0.82f * white;
        const float hp = white - voice->noiseState;
        sample = (hp * 0.74f + oscA * 0.12f + oscB * 0.07f) * voice->env;
        voice->env *= 0.9560f;
    }

    if (voice->env <= 0.0015f)
        voice->active = false;

    if (instrument == DrumInstrument::Hat)
        return { std::tanh(sample * 0.92f * 1.35f), std::tanh(sample * 1.35f) };

    return { sample, sample };
}

void EffectsChain::registerDrumTrigger(DrumInstrument instrument, float velocity)
{
    triggerDrumVoice(instrument, velocity);
    drumLastInstrument.store((int) instrument);
}

juce::File EffectsChain::locateRhythmSamplesRoot() const
{
    auto searchFrom = [](juce::File start)
    {
        for (int depth = 0; depth < 8 && start.exists(); ++depth)
        {
            const auto candidate = start.getChildFile("Assets").getChildFile("Samples").getChildFile("RhythmBox");
            if (candidate.isDirectory())
                return candidate;

            const auto parent = start.getParentDirectory();
            if (parent == start)
                break;
            start = parent;
        }

        return juce::File();
    };

    auto found = searchFrom(juce::File::getCurrentWorkingDirectory());
    if (found.exists())
        return found;

    return searchFrom(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory());
}

juce::File EffectsChain::locateProjectRoot() const
{
    auto searchFrom = [](juce::File start)
    {
        for (int depth = 0; depth < 8 && start.exists(); ++depth)
        {
            if (start.getChildFile("Source").isDirectory())
                return start;

            const auto parent = start.getParentDirectory();
            if (parent == start)
                break;
            start = parent;
        }

        return juce::File();
    };

    auto found = searchFrom(juce::File::getCurrentWorkingDirectory());
    if (found.exists())
        return found;

    return searchFrom(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory());
}

void EffectsChain::loadSampleFileIntoPool(SamplePool& pool, const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    std::unique_ptr<juce::AudioFormatReader> reader(rhythmSampleFormatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return;

    LoadedSample loaded;
    loaded.audio.setSize((int) juce::jlimit((juce::int64) 1, (juce::int64) 2, (juce::int64) reader->numChannels),
                         (int) reader->lengthInSamples);
    reader->read(&loaded.audio, 0, (int) reader->lengthInSamples, 0, true, true);
    loaded.sourceName = file.getFileName();
    pool.entries.push_back(std::move(loaded));
}

void EffectsChain::loadSamplesIntoPool(SamplePool& pool, const juce::File& folder, int maxFiles)
{
    pool.entries.clear();
    pool.nextIndex = 0;

    if (!folder.isDirectory())
        return;

    const juce::StringArray patterns { "*.wav", "*.aif", "*.aiff", "*.flac", "*.ogg", "*.mp3" };

    juce::Array<juce::File> orderedFiles;

    for (const auto& pattern : patterns)
    {
        juce::Array<juce::File> files;
        folder.findChildFiles(files, juce::File::findFiles, false, pattern);
        files.sort();

        for (const auto& file : files)
        {
            orderedFiles.add(file);
            if (maxFiles > 0 && orderedFiles.size() >= maxFiles)
                break;
        }

        if (maxFiles > 0 && orderedFiles.size() >= maxFiles)
            break;
    }

    for (const auto& file : orderedFiles)
        loadSampleFileIntoPool(pool, file);
}

std::shared_ptr<EffectsChain::RhythmSampleBank> EffectsChain::loadRhythmSamples(int styleIndex, bool metronomeMode)
{
    auto bank = std::make_shared<RhythmSampleBank>();
    bank->styleIndex = styleIndex;
    bank->metronomeMode = metronomeMode;

    const auto samplesRoot = locateRhythmSamplesRoot();
    if (!samplesRoot.isDirectory())
        return bank;

    bank->samplesRootFound = true;

    loadSamplesIntoPool(bank->pools[(size_t) RhythmSampleSlot::Accent],
                        samplesRoot.getChildFile("Metronome").getChildFile("Accent"), 1);
    loadSamplesIntoPool(bank->pools[(size_t) RhythmSampleSlot::Tick],
                        samplesRoot.getChildFile("Metronome").getChildFile("Tick"), 1);

    const auto projectRoot = locateProjectRoot();
    if (projectRoot.isDirectory())
    {
        const auto sticksFolder = projectRoot.getChildFile("Source")
            .getChildFile("The Crate vol.6")
            .getChildFile("Percs")
            .getChildFile("Sticks");

        auto& countInAccentPool = bank->pools[(size_t) RhythmSampleSlot::CountInAccent];
        auto& countInTickPool = bank->pools[(size_t) RhythmSampleSlot::CountInTick];
        countInAccentPool.entries.clear();
        countInAccentPool.nextIndex = 0;
        countInTickPool.entries.clear();
        countInTickPool.nextIndex = 0;
        loadSampleFileIntoPool(countInAccentPool, sticksFolder.getChildFile("Stick 01 (John Tropea - Look What They've Done to My Song).wav"));
        loadSampleFileIntoPool(countInTickPool, sticksFolder.getChildFile("Stick 02 (John Tropea - Look What They've Done to My Song).wav"));
    }

    if (!metronomeMode)
    {
        const int safeStyleIndex = juce::jlimit(0, (int) juce::numElementsInArray(kRhythmStyleFolders) - 1, styleIndex);
        const auto styleFolder = samplesRoot.getChildFile(kRhythmStyleFolders[safeStyleIndex]);
        const bool curatedTightSelection =
            styleFolder.getFileName() == "BossaNova" || styleFolder.getFileName() == "Blues";
        const int styleSampleLimit = curatedTightSelection ? 4 : 0;
        loadSamplesIntoPool(bank->pools[(size_t) RhythmSampleSlot::Kick],  styleFolder.getChildFile("Kick"), styleSampleLimit);
        loadSamplesIntoPool(bank->pools[(size_t) RhythmSampleSlot::Snare], styleFolder.getChildFile("Snare"), styleSampleLimit);
        loadSamplesIntoPool(bank->pools[(size_t) RhythmSampleSlot::Hat],   styleFolder.getChildFile("Hat"), styleSampleLimit);
    }

    return bank;
}

void EffectsChain::requestRhythmSampleLoad(int styleIndex, bool metronomeMode) noexcept
{
    if (requestedRhythmSampleStyle.load() == styleIndex
        && requestedRhythmSampleMetronome.load() == metronomeMode)
        return;

    requestedRhythmSampleStyle.store(styleIndex);
    requestedRhythmSampleMetronome.store(metronomeMode);
    rhythmSampleRequestSerial.fetch_add(1);
    notify();
}

void EffectsChain::applyPendingRhythmSampleBank(int styleIndex, bool metronomeMode) noexcept
{
    const juce::SpinLock::ScopedTryLockType lock(rhythmSampleBankLock);
    if (!lock.isLocked() || pendingRhythmSampleBank == nullptr
        || pendingRhythmSampleBank->styleIndex != styleIndex
        || pendingRhythmSampleBank->metronomeMode != metronomeMode)
        return;

    for (auto& voice : rhythmSampleVoices)
        voice = {};

    retiredRhythmSampleBank = std::move(rhythmSampleBank);
    rhythmSampleBank = std::move(pendingRhythmSampleBank);
}

void EffectsChain::run()
{
    unsigned int handledSerial = 0;
    while (!threadShouldExit())
    {
        const auto serial = rhythmSampleRequestSerial.load();
        if (serial == handledSerial)
        {
            wait(-1);
            continue;
        }

        const int style = requestedRhythmSampleStyle.load();
        const bool metronome = requestedRhythmSampleMetronome.load();
        auto loadedBank = loadRhythmSamples(style, metronome);
        handledSerial = serial;

        const juce::SpinLock::ScopedLockType lock(rhythmSampleBankLock);
        retiredRhythmSampleBank.reset();
        pendingRhythmSampleBank = std::move(loadedBank);
    }
}

void EffectsChain::configureRhythmPattern(int styleIndex, int variationIndex, bool metronomeMode)
{
    kickPattern.fill(0.0f);
    snarePattern.fill(0.0f);
    hatPattern.fill(0.0f);

    if (metronomeMode)
    {
        hatPattern[0] = 0.95f;
        hatPattern[4] = 0.50f;
        hatPattern[8] = 0.50f;
        hatPattern[12] = 0.50f;
        return;
    }

    switch (juce::jlimit(0, 8, styleIndex))
    {
        case 0: // Rock
            kickPattern[0] = 1.0f; kickPattern[6] = 0.28f; kickPattern[8] = 0.82f;
            snarePattern[4] = 0.92f; snarePattern[12] = 0.96f;
            hatPattern[0] = hatPattern[2] = hatPattern[4] = hatPattern[6] = 0.34f;
            hatPattern[8] = hatPattern[10] = hatPattern[12] = hatPattern[14] = 0.32f;
            break;

        case 1: // Funk
            kickPattern[0] = 0.92f; kickPattern[3] = 0.22f; kickPattern[7] = 0.42f; kickPattern[10] = 0.26f;
            snarePattern[4] = 0.88f; snarePattern[12] = 0.90f;
            hatPattern[0] = hatPattern[1] = hatPattern[2] = hatPattern[3] = 0.20f;
            hatPattern[4] = hatPattern[5] = hatPattern[6] = hatPattern[7] = 0.22f;
            hatPattern[8] = hatPattern[9] = hatPattern[10] = hatPattern[11] = 0.20f;
            hatPattern[12] = hatPattern[13] = hatPattern[14] = hatPattern[15] = 0.24f;
            break;

        case 2: // Disco
            kickPattern[0] = kickPattern[4] = kickPattern[8] = kickPattern[12] = 0.86f;
            snarePattern[4] = 0.54f; snarePattern[12] = 0.58f;
            for (int i = 0; i < kDrumPatternSteps; ++i) hatPattern[i] = (i % 2 == 0) ? 0.34f : 0.22f;
            break;

        case 3: // Latin
            kickPattern[0] = 0.82f; kickPattern[5] = 0.28f; kickPattern[8] = 0.64f; kickPattern[11] = 0.24f;
            snarePattern[3] = 0.36f; snarePattern[6] = 0.26f; snarePattern[10] = 0.42f; snarePattern[14] = 0.30f;
            hatPattern[0] = hatPattern[2] = hatPattern[4] = hatPattern[6] = 0.24f;
            hatPattern[8] = hatPattern[10] = hatPattern[12] = hatPattern[14] = 0.24f;
            break;

        case 4: // HipHop
            kickPattern[0] = 0.96f; kickPattern[7] = 0.34f; kickPattern[10] = 0.26f;
            snarePattern[4] = 0.82f; snarePattern[12] = 0.88f;
            hatPattern[0] = hatPattern[2] = hatPattern[4] = hatPattern[6] = 0.20f;
            hatPattern[8] = hatPattern[10] = hatPattern[12] = hatPattern[14] = 0.20f;
            break;

        case 5: // House
            kickPattern[0] = kickPattern[4] = kickPattern[8] = kickPattern[12] = 1.0f;
            snarePattern[4] = 0.34f; snarePattern[12] = 0.42f;
            for (int i = 0; i < kDrumPatternSteps; ++i) hatPattern[i] = (i % 2 == 0) ? 0.28f : 0.12f;
            break;

        case 6: // Bossa Nova
            kickPattern[0] = 0.72f; kickPattern[3] = 0.20f; kickPattern[8] = 0.54f; kickPattern[11] = 0.18f;
            snarePattern[2] = 0.14f; snarePattern[6] = 0.24f; snarePattern[10] = 0.30f; snarePattern[14] = 0.22f;
            hatPattern[0] = 0.20f; hatPattern[2] = 0.28f; hatPattern[4] = 0.20f; hatPattern[6] = 0.30f;
            hatPattern[8] = 0.20f; hatPattern[10] = 0.28f; hatPattern[12] = 0.20f; hatPattern[14] = 0.30f;
            hatPattern[1] = 0.08f; hatPattern[5] = 0.10f; hatPattern[9] = 0.08f; hatPattern[13] = 0.10f;
            break;

        case 7: // Techno
            kickPattern[0] = kickPattern[4] = kickPattern[8] = kickPattern[12] = 1.0f;
            snarePattern[4] = 0.42f; snarePattern[12] = 0.48f;
            for (int i = 0; i < kDrumPatternSteps; ++i) hatPattern[i] = (i % 2 == 0) ? 0.30f : 0.15f;
            hatPattern[3] = 0.22f; hatPattern[7] = 0.22f; hatPattern[11] = 0.22f; hatPattern[15] = 0.26f;
            break;

        case 8: // Blues
            kickPattern[0] = 0.88f; kickPattern[5] = 0.22f; kickPattern[8] = 0.68f; kickPattern[13] = 0.20f;
            snarePattern[4] = 0.74f; snarePattern[12] = 0.82f;
            hatPattern[0] = 0.22f; hatPattern[2] = 0.28f; hatPattern[4] = 0.22f; hatPattern[6] = 0.30f;
            hatPattern[8] = 0.22f; hatPattern[10] = 0.28f; hatPattern[12] = 0.22f; hatPattern[14] = 0.30f;
            break;
    }

    if (variationIndex == 1)
        applyChorusLift(kickPattern, snarePattern, hatPattern);
    else if (variationIndex >= 2)
        applyResolvingFill(kickPattern, snarePattern, hatPattern);

    clampPattern(kickPattern);
    clampPattern(snarePattern);
    clampPattern(hatPattern);
}

void EffectsChain::advanceRhythmSequencer(float tempoBpm, float swingAmount, bool metronomeMode, int styleIndex)
{
    const float bpm = juce::jlimit(50.0f, 220.0f, tempoBpm);
    const float stepSamplesBase = ((float) sampleRate_ * 60.0f / bpm) * 0.25f;
    const float quarterNoteSamples = stepSamplesBase * 4.0f;
    const float swingDepth = juce::jlimit(0.0f, 1.0f, swingAmount) * 0.28f;
    const int verseBars = rhythmVerseBars.load();
    const int chorusBars = rhythmChorusBars.load();
    const bool countInWithSticks = rhythmCountInSound.load() == (int) RhythmCountInSound::Sticks;

    while (rhythmCountInActive && drumSampleCounter >= rhythmCountInNextSample)
    {
        if (rhythmCountInBeatIndex >= rhythmCountInTotalBeats)
        {
            rhythmCountInActive = false;
            rhythmCountInBeatIndex = 0;
            rhythmCountInTotalBeats = 0;
            rhythmCountInBeatsRemainingDisplay.store(-1);
            drumStepIndex = 0;
            drumNextStepSample = drumSampleCounter;
            rhythmCountInTailFadeLengthSamples = juce::jmax(32, (int) (sampleRate_ * 0.018));
            rhythmCountInTailFadeSamplesRemaining = rhythmCountInTailFadeLengthSamples;
            rhythmLaunchFadeLengthSamples = juce::jmax(32, (int) (sampleRate_ * 0.020));
            rhythmLaunchFadeSamplesRemaining = rhythmLaunchFadeLengthSamples;
            break;
        }

        const int beatInBar = rhythmCountInBeatIndex % 4;
        if (countInWithSticks)
        {
            if (beatInBar == 0)
                triggerRhythmSample(RhythmSampleSlot::CountInAccent, 1.0f);
            else
                triggerRhythmSample(RhythmSampleSlot::CountInTick, 0.84f);
        }
        else
        {
            if (beatInBar == 0)
                triggerRhythmSample(RhythmSampleSlot::Accent, 1.0f);
            else
                triggerRhythmSample(RhythmSampleSlot::Tick, 0.68f);
        }

        ++rhythmCountInBeatIndex;
        rhythmCountInBeatsRemainingDisplay.store(juce::jmax(0, rhythmCountInTotalBeats - rhythmCountInBeatIndex));
        rhythmCountInNextSample += (juce::int64) juce::jmax(1.0f, quarterNoteSamples);
    }

    if (rhythmCountInActive)
        return;

    while (drumSampleCounter >= drumNextStepSample)
    {
        const int step = drumStepIndex % kDrumPatternSteps;
        const bool isMainSection = rhythmSectionState == RhythmSection::Verse || rhythmSectionState == RhythmSection::Chorus;
        const int currentSectionBars = rhythmSectionState == RhythmSection::Verse ? verseBars : chorusBars;
        const bool lastBarBeforeTransition = isMainSection
            && rhythmTransitionQueued
            && rhythmBarsElapsedInSection >= juce::jmax(0, currentSectionBars - 1);

        if (!metronomeMode && lastBarBeforeTransition && step == kRhythmFillStartStep)
        {
            rhythmSectionState = RhythmSection::Fill;
            rhythmCurrentVariation = variationIndexForSection(rhythmSectionState);
            configureRhythmPattern(styleIndex, rhythmCurrentVariation, false);
            rhythmCurrentSectionDisplay.store((int) rhythmSectionState);
        }

        const float kickVel = kickPattern[(size_t) step];
        const float snareVel = snarePattern[(size_t) step];
        const float hatVel = hatPattern[(size_t) step];

        const bool hasMetronomeSamples =
            !rhythmSampleBank->pools[(size_t) RhythmSampleSlot::Accent].entries.empty()
            || !rhythmSampleBank->pools[(size_t) RhythmSampleSlot::Tick].entries.empty();

        if (metronomeMode && hasMetronomeSamples)
        {
            if (step == 0)
                triggerRhythmSample(RhythmSampleSlot::Accent, 1.0f);
            else if (step == 4 || step == 8 || step == 12)
                triggerRhythmSample(RhythmSampleSlot::Tick, 0.70f);
        }
        else
        {
            if (kickVel > 0.001f) triggerDrumVoice(DrumInstrument::Kick, kickVel);
            if (snareVel > 0.001f) triggerDrumVoice(DrumInstrument::Snare, snareVel);
            if (hatVel > 0.001f) triggerDrumVoice(DrumInstrument::Hat, hatVel);
        }

        const bool swungStep = !metronomeMode && (step % 2 == 1);
        const float nextStepSamples = stepSamplesBase * (swungStep ? (1.0f + swingDepth) : (1.0f - swingDepth));
        drumNextStepSample += (juce::int64) juce::jmax(1.0f, nextStepSamples);
        drumStepIndex = (step + 1) % kDrumPatternSteps;

        if (drumStepIndex == 0 && !metronomeMode)
        {
            if (rhythmSectionState == RhythmSection::Fill)
            {
                rhythmSectionState = rhythmPendingMainSection;
                rhythmTransitionQueued = false;
                rhythmBarsElapsedInSection = 0;
                rhythmCurrentVariation = variationIndexForSection(rhythmSectionState);
                configureRhythmPattern(styleIndex, rhythmCurrentVariation, false);
                rhythmCurrentSectionDisplay.store((int) rhythmSectionState);
                rhythmPendingSectionDisplay.store(-1);
            }
            else
            {
                ++rhythmBarsElapsedInSection;

                if (rhythmBarsElapsedInSection >= juce::jmax(1, currentSectionBars))
                    rhythmBarsElapsedInSection = 0;
            }
        }
    }
}

float EffectsChain::estimateRecentPitchHz(int channel, int writePosition) const
{
    if (channel < 0 || channel >= pitchBuffer.getNumChannels())
        return 0.0f;

    const auto* data = pitchBuffer.getReadPointer(channel);
    const int bufLen = pitchBuffer.getNumSamples();
    if (data == nullptr || bufLen <= 0)
        return 0.0f;

    // Una ventana de 17 ms no contiene suficientes ciclos de la sexta cuerda
    // (E2 = 82 Hz) y tiende a seleccionar sus armónicos. Usamos al menos tres
    // períodos de la nota más grave habitual de guitarra y elegimos el primer
    // pico fuerte de autocorrelación: éste corresponde al fundamental.
    const int minLag = juce::jlimit(8, bufLen / 4, (int) (sampleRate_ / 520.0));
    const int maxLag = juce::jlimit(minLag + 1, juce::jmin(bufLen / 2, 4095),
                                    (int) (sampleRate_ / 65.0));
    const int window = juce::jlimit(1536, bufLen / 2, juce::jmax(1536, maxLag * 3));

    float bestScore = 0.0f;
    int bestLag = 0;
    std::array<float, 4096> scores{};

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        float energyA = 0.0f;
        float energyB = 0.0f;

        for (int i = 0; i < window; ++i)
        {
            const int idxA = (writePosition - i + bufLen) % bufLen;
            const int idxB = (writePosition - i - lag + bufLen * 2) % bufLen;
            const float a = data[idxA];
            const float b = data[idxB];
            corr += a * b;
            energyA += a * a;
            energyB += b * b;
        }

        const float denom = std::sqrt(energyA * energyB) + 1.0e-9f;
        const float score = corr / denom;
        scores[(size_t) (lag - minLag)] = score;
        if (score > bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestLag == 0 || bestScore < 0.62f)
        return 0.0f;

    // El máximo global puede caer en un múltiplo del período. El primer máximo
    // local suficientemente cercano al mejor evita que el afinador salte una
    // octava en guitarras eléctricas con mucho contenido armónico.
    const float peakThreshold = juce::jmax(0.62f, bestScore * 0.88f);
    int selectedLag = bestLag;
    for (int lag = minLag + 1; lag < maxLag; ++lag)
    {
        const float previous = scores[(size_t) (lag - minLag - 1)];
        const float current  = scores[(size_t) (lag - minLag)];
        const float next     = scores[(size_t) (lag - minLag + 1)];
        if (current >= peakThreshold && current >= previous && current > next)
        {
            selectedLag = lag;
            break;
        }
    }

    // Interpolación parabólica para que el indicador en cents no avance a
    // saltos de un sample de retardo.
    const int index = selectedLag - minLag;
    const float left = scores[(size_t) juce::jmax(0, index - 1)];
    const float centre = scores[(size_t) index];
    const float right = scores[(size_t) juce::jmin(maxLag - minLag, index + 1)];
    const float denominator = left - 2.0f * centre + right;
    const float adjustment = std::abs(denominator) > 1.0e-6f
        ? juce::jlimit(-0.5f, 0.5f, 0.5f * (left - right) / denominator) : 0.0f;
    return (float) sampleRate_ / ((float) selectedLag + adjustment);
}

void EffectsChain::prepareExternalPluginScratchBuffer()
{
    auto* plugin = externalPluginInstance.get();
    externalPluginInputChannels = plugin != nullptr ? plugin->getTotalNumInputChannels() : 0;
    externalPluginOutputChannels = plugin != nullptr ? plugin->getTotalNumOutputChannels() : 0;

    const int requiredChannels = juce::jmax(1, juce::jmax(externalPluginInputChannels,
                                                          externalPluginOutputChannels));
    externalPluginScratchBuffer.setSize(requiredChannels, maxBlockSize_, false, true, false);
    externalPluginScratchBuffer.clear();

    // clear() conserva esta reserva, evitando crecimiento del MidiBuffer en el callback.
    externalPluginMidiBuffer.ensureSize(64 * 1024);
    externalPluginMidiBuffer.clear();
}

void EffectsChain::processExternalPluginBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!isExternalPluginEnabled() || numSamples <= 0)
        return;

    auto* plugin = externalPluginInstance.get();
    if (plugin == nullptr || plugin->isSuspended())
        return;

    const int availableSamples = buffer.getNumSamples() - startSample;
    const int samplesToProcess = juce::jmin(numSamples, juce::jmax(0, availableSamples));
    const int scratchBlockSize = externalPluginScratchBuffer.getNumSamples();
    const int scratchChannels = externalPluginScratchBuffer.getNumChannels();
    if (samplesToProcess <= 0 || scratchBlockSize <= 0 || scratchChannels <= 0)
        return;

    const int sourceChannels = buffer.getNumChannels();
    const int copiedInputChannels = juce::jmin(sourceChannels,
                                               juce::jmin(externalPluginInputChannels, scratchChannels));
    const int copiedOutputChannels = juce::jmin(sourceChannels,
                                                juce::jmin(externalPluginOutputChannels, scratchChannels));

    for (int offset = 0; offset < samplesToProcess; offset += scratchBlockSize)
    {
        const int chunkSize = juce::jmin(scratchBlockSize, samplesToProcess - offset);
        externalPluginScratchBuffer.clear(0, chunkSize);

        for (int channel = 0; channel < copiedInputChannels; ++channel)
            externalPluginScratchBuffer.copyFrom(channel, 0, buffer, channel,
                                                 startSample + offset, chunkSize);

        // La capacidad permanece fija; el plugin solo ve el tramo valido del scratch.
        juce::AudioBuffer<float> chunk(externalPluginScratchBuffer.getArrayOfWritePointers(),
                                       scratchChannels, 0, chunkSize);
        externalPluginMidiBuffer.clear();
        plugin->processBlock(chunk, externalPluginMidiBuffer);

        for (int channel = 0; channel < copiedOutputChannels; ++channel)
            buffer.copyFrom(channel, startSample + offset, chunk, channel, 0, chunkSize);
    }
}

void EffectsChain::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    maxBlockSize_ = juce::jmax(16, maxBlockSize);

    delayBuffer.setSize(2, (int)(sampleRate_ * 3.0));
    chorusBuffer.setSize(2, juce::jmax(1024, (int)(sampleRate_ * 0.08)));
    pitchBuffer.setSize(2, kPitchBufLen);
    freezeBuffer.setSize(2, kFreezeBufLen);
    chordBodyBuffer.setSize(2, juce::jmax(4096, (int) (sampleRate_ * 0.070)));
    for (auto& layer : sustainLayers)
        layer.buffer.setSize(2, kFreezeBufLen);
    chordSustainLayer.buffer.setSize(2, kFreezeBufLen);
    roomReverb.setSampleRate(sampleRate_);
    outputEq.prepare(sampleRate_);
    if (externalPluginInstance != nullptr)
    {
        externalPluginInstance->releaseResources();
        externalPluginInstance->setRateAndBufferSizeDetails(sampleRate_, maxBlockSize_);
        externalPluginInstance->prepareToPlay(sampleRate_, maxBlockSize_);
        externalPluginInstance->reset();
    }

    prepareExternalPluginScratchBuffer();

    reset();
}

void EffectsChain::reset()
{
    if (externalPluginInstance != nullptr)
        externalPluginInstance->reset();

    delayBuffer.clear();
    chorusBuffer.clear();
    pitchBuffer.clear();
    freezeBuffer.clear();
    chordBodyBuffer.clear();
    externalPluginMidiBuffer.clear();
    for (auto& layer : sustainLayers)
    {
        layer.buffer.clear();
        layer.capturedLen = 0;
        layer.readPhase = 0.0;
        layer.grainPhase = 0.0;
        layer.grainLen = 0;
        layer.grainStartA = 0;
        layer.grainStartB = 0;
        layer.active = false;
        layer.wetLevel = 0.0f;
        layer.brightness = 1.0f;
        layer.smooth[0] = layer.smooth[1] = 0.0f;
        layer.toneState[0] = layer.toneState[1] = 0.0f;
    }
    for (auto& grain : freezeGrains)
        grain = {};
    chordSustainLayer.buffer.clear();
    chordSustainLayer.capturedLen = 0;
    chordSustainLayer.readPhase = 0.0;
    chordSustainLayer.grainPhase = 0.0;
    chordSustainLayer.grainLen = 0;
    chordSustainLayer.grainStartA = 0;
    chordSustainLayer.grainStartB = 0;
    chordSustainLayer.active = false;
    chordSustainLayer.wetLevel = 0.0f;
    chordSustainLayer.brightness = 1.0f;
    chordSustainLayer.smooth[0] = chordSustainLayer.smooth[1] = 0.0f;
    chordSustainLayer.toneState[0] = chordSustainLayer.toneState[1] = 0.0f;
    for (auto& grain : chordFreezeGrains)
        grain = {};

    delayWritePos   = 0;
    chorusWritePos  = 0;
    pitchWritePos   = 0;
    freezeWritePos  = 0;
    chordBodyWritePos = 0;
    freezeHoldActive = false;
    freezeLayerWriteIndex = 0;
    chordFreezeHoldActive = false;

    std::memset(lowPassFilters, 0, sizeof(lowPassFilters));
    noiseGateEnv = 0.0f;
    noiseGateGain = 1.0f;
    std::memset(chordBodyDamp,   0, sizeof(chordBodyDamp));
    std::memset(compEnv,        0, sizeof(compEnv));
    std::memset(compLowBand,    0, sizeof(compLowBand));
    std::memset(compMidLowBand, 0, sizeof(compMidLowBand));
    std::memset(drumTriggerEnv, 0, sizeof(drumTriggerEnv));
    std::memset(drumTriggerPulse, 0, sizeof(drumTriggerPulse));
    drumLowBand = 0.0f;
    drumHighBand = 0.0f;
    drumPrevPulse = 0.0f;
    drumPeakValue = 0.0f;
    drumPeakLowBand = 0.0f;
    drumPeakHighBand = 0.0f;
    drumPeakTransient = 0.0f;
    drumLastTriggerLevel = 0.0f;
    drumDetectorDisplay = 0.0f;
    drumTransientDisplay = 0.0f;
    drumOutputDisplay = 0.0f;
    rhythmToneState[0] = 0.0f;
    rhythmToneState[1] = 0.0f;
    pitchPhaseHarm = 0.0;
    chorusPhase    = 0.0;
    tremoloPhase   = 0.0;
    roomReverb.reset();
    outputEq.reset();

    kickVoice = {};
    snareVoice = {};
    hatVoice = {};
    for (auto& voice : rhythmSampleVoices)
        voice = {};
    kickPattern.fill(0.0f);
    snarePattern.fill(0.0f);
    hatPattern.fill(0.0f);
    kickPattern[0] = 0.85f;
    kickPattern[8] = 0.45f;
    snarePattern[4] = 0.72f;
    snarePattern[12] = 0.75f;
    hatPattern[0] = hatPattern[4] = hatPattern[8] = hatPattern[12] = 0.25f;
    drumStepIndex = 0;
    drumStepSamples = juce::jmax(1, (int)(sampleRate_ * 0.125));
    drumCooldownSamples = 0;
    drumReleaseSamples = 0;
    drumNoInputCounter = 0;
    drumRecentIntervalWrite = 0;
    drumRecentIntervalCount = 0;
    drumDensityCounter = 0;
    drumBarCounter = 0;
    drumTriggerRoundRobin = 0;
    drumTriggerArmed = true;
    drumWasEnabled = false;
    rhythmWasEnabled = false;
    rhythmCurrentStyle = -1;
    rhythmCurrentVariation = -1;
    rhythmCurrentMetronome = false;
    rhythmSectionState = RhythmSection::Verse;
    rhythmPendingMainSection = RhythmSection::Chorus;
    rhythmBarsElapsedInSection = 0;
    rhythmTransitionQueued = false;
    rhythmCountInActive = false;
    rhythmCountInBeatIndex = 0;
    rhythmCountInTotalBeats = 0;
    rhythmCountInNextSample = 0;
    rhythmCountInTailFadeSamplesRemaining = 0;
    rhythmCountInTailFadeLengthSamples = 0;
    rhythmLaunchFadeSamplesRemaining = 0;
    rhythmLaunchFadeLengthSamples = 0;
    drumSampleCounter = 0;
    drumNextStepSample = 0;
    drumLastTriggerSample = -1;
    drumRecentIntervals.fill(0);
    rhythmCurrentSectionDisplay.store((int) RhythmSection::Verse);
    rhythmPendingSectionDisplay.store(-1);
    rhythmCountInBeatsRemainingDisplay.store(-1);
    drumDetectorLevel.store(0.0f);
    drumTransientLevel.store(0.0f);
    drumOutputLevel.store(0.0f);
    drumDetectedPitchHz.store(0.0f);
    drumLastInstrument.store(-1);
    drumActiveVoices.store(0);
}

void EffectsChain::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    processExternalPluginBlock(buffer, startSample, numSamples);

    // =========================================================================
    // 1. CARGA DE ATOMICS — una sola vez por bloque, fuera de cualquier loop.
    // =========================================================================
    const bool gateOn = noiseGateEnabled.load();
    const bool cOn    = compEnabled.load();
    const bool rhythmOn = rhythmEnabled.load();
    const bool drumOn = drumTriggerEnabled.load();
    const bool harmOn = harmonicEnabled.load();
    const bool fzOn   = fuzzEnabled.load();
    const bool drOn   = driveEnabled.load();
    const bool choOn  = chorusEnabled.load();
    const bool trOn   = tremoloEnabled.load();
    const bool delOn  = delayEnabled.load();
    const bool revOn  = reverbEnabled.load();
    const bool eqOn   = eqEnabled.load();
    const bool freezeOn = freezeEnabled.load();
    const bool chordFreezeOn = chordFreezeEnabled.load();
    int orderedPedals[kPedalCount]{};
    for (int i = 0; i < kPedalCount; ++i)
        orderedPedals[i] = pedalOrder[(size_t) i].load();

    const float cMix    = compMix.load();
    const float gateThresholdDb = noiseGateThresholdDb.load();
    const float gateAttackMs = noiseGateAttackMs.load();
    const float gateReleaseMs = noiseGateReleaseMs.load();
    const float cGain   = compGain.load();
    const float cAction = compAction.load();
    const float cTail   = compTail.load();
    const float cEq     = compEq.load();
    const float rhythmTempo = rhythmTempoBpm.load();
    const float rhythmLevelValue = rhythmLevel.load();
    const float rhythmSwingValue = rhythmSwing.load();
    const float rhythmToneValue = rhythmTone.load();
    const int rhythmStyleIndex = rhythmStyle.load();
    const int rhythmModeIndex = rhythmMode.load();
    const bool rhythmCountInOn = rhythmCountInEnabled.load();
    const int rhythmCountInBarsValue = rhythmCountInBars.load();
    const int requestedRhythmSection = rhythmSectionRequest.exchange(-1);
    const bool requestedRhythmAdvance = rhythmAdvanceRequest.exchange(false);
    const float drumSens = drumTriggerSensitivity.load();
    const float drumGate = drumTriggerGate.load();
    const float drumLevel = drumTriggerLevel.load();
    const float drumCooldownMs = drumTriggerCooldownMs.load();
    const float drumKickMix = drumKickLevel.load();
    const float drumSnareMix = drumSnareLevel.load();
    const float drumHatMix = drumHatLevel.load();
    const float drumPitchTolerance = drumPitchToleranceHz.load();
    const float drumKickPitch = drumKickPitchHz.load();
    const float drumSnarePitch = drumSnarePitchHz.load();
    const float drumHatPitch = drumHatPitchHz.load();
    const int   drumPlacement = drumTriggerPlacement.load();
    const int   harmInterval = harmonicInterval.load();
    const float harmM   = harmonicMix.load();
    const float harmL   = harmonicLevel.load();
    const float fzL     = fuzzLevel.load();
    const float fzT     = fuzzTone.load();
    const float drL     = driveLevel.load();
    const float drT     = driveTone.load();
    const float delM    = delayMix.load();
    const float delTime = delayTimeMs.load();
    const float delFB   = delayFeedback.load();
    const float revM    = reverbMix.load();
    const float revRoom = reverbRoomSize.load();
    const float revDamp = reverbDamping.load();
    const float frzMix  = freezeMix.load();
    const float frzWindowMs = freezeWindowMs.load();
    const float frzRiseMs = freezeRiseMs.load();
    const float frzTailMs = freezeTailMs.load();
    const float frzTone = freezeTone.load();
    const bool  frzWetOnly = freezeWetOnly.load();
    const float chordFrzMix = chordFreezeMix.load();
    const float chordFrzWindowMs = chordFreezeWindowMs.load();
    const float chordFrzRiseMs = chordFreezeRiseMs.load();
    const float chordFrzTailMs = chordFreezeTailMs.load();
    const float chordFrzTone = chordFreezeTone.load();
    const float chordFrzCrossfade = chordFreezeCrossfade.load();
    const float choM    = chorusMix.load();
    const float choRate = chorusRate.load();
    const float choDepth= chorusDepth.load();
    const float trM     = tremoloMix.load();
    const float trRate  = tremoloRate.load();
    const float trDepth = tremoloDepth.load();
    const bool metronomeMode = rhythmModeIndex == (int) RhythmMode::Metronome;
    const bool rhythmSoloMode = rhythmModeIndex == (int) RhythmMode::Solo;

    outputEq.setEnabled(eqOn);
    outputEq.updateCoefficientsIfNeeded();

    if (rhythmOn)
    {
        if (!rhythmWasEnabled)
        {
            drumStepIndex = 0;
            drumSampleCounter = 0;
            drumNextStepSample = 0;
            kickVoice = {};
            snareVoice = {};
            hatVoice = {};
            rhythmWasEnabled = true;
            rhythmCurrentStyle = -1;
            rhythmCurrentVariation = -1;
            rhythmCurrentMetronome = !metronomeMode;
            rhythmSectionState = RhythmSection::Verse;
            rhythmPendingMainSection = RhythmSection::Chorus;
            rhythmBarsElapsedInSection = 0;
            rhythmTransitionQueued = false;
            rhythmCountInActive = rhythmCountInOn && rhythmCountInBarsValue > 0;
            rhythmCountInBeatIndex = 0;
            rhythmCountInTotalBeats = rhythmCountInBarsValue * 4;
            rhythmCountInNextSample = 0;
            rhythmCountInTailFadeSamplesRemaining = 0;
            rhythmCountInTailFadeLengthSamples = 0;
            rhythmLaunchFadeSamplesRemaining = 0;
            rhythmLaunchFadeLengthSamples = 0;
            rhythmCurrentSectionDisplay.store((int) rhythmSectionState);
            rhythmPendingSectionDisplay.store(-1);
            rhythmCountInBeatsRemainingDisplay.store(rhythmCountInActive ? rhythmCountInTotalBeats : -1);
        }

        if (requestedRhythmSection >= 0 && requestedRhythmSection <= (int) RhythmSection::Chorus)
        {
            rhythmSectionState = (RhythmSection) requestedRhythmSection;
            rhythmBarsElapsedInSection = 0;
            rhythmTransitionQueued = false;
            rhythmPendingMainSection = (rhythmSectionState == RhythmSection::Chorus)
                ? RhythmSection::Verse
                : RhythmSection::Chorus;
            rhythmCountInActive = false;
            rhythmCountInBeatIndex = 0;
            rhythmCountInTotalBeats = 0;
            rhythmCountInTailFadeSamplesRemaining = 0;
            rhythmCountInTailFadeLengthSamples = 0;
            rhythmLaunchFadeSamplesRemaining = 0;
            rhythmLaunchFadeLengthSamples = 0;
            rhythmCurrentStyle = -1;
            rhythmCurrentVariation = -1;
            rhythmCurrentSectionDisplay.store((int) rhythmSectionState);
            rhythmPendingSectionDisplay.store(-1);
            rhythmCountInBeatsRemainingDisplay.store(-1);
        }

        if (requestedRhythmAdvance && rhythmSectionState != RhythmSection::Fill)
        {
            rhythmPendingMainSection = (rhythmSectionState == RhythmSection::Verse)
                ? RhythmSection::Chorus
                : RhythmSection::Verse;
            rhythmTransitionQueued = true;
            rhythmPendingSectionDisplay.store((int) rhythmPendingMainSection);
        }

        const int rhythmVariationIndex = variationIndexForSection(rhythmSectionState);
        requestRhythmSampleLoad(rhythmStyleIndex, metronomeMode);
        applyPendingRhythmSampleBank(rhythmStyleIndex, metronomeMode);

        if (rhythmCurrentStyle != rhythmStyleIndex
            || rhythmCurrentVariation != rhythmVariationIndex
            || rhythmCurrentMetronome != metronomeMode)
        {
            configureRhythmPattern(rhythmStyleIndex, rhythmVariationIndex, metronomeMode);
            rhythmCurrentStyle = rhythmStyleIndex;
            rhythmCurrentVariation = rhythmVariationIndex;
            rhythmCurrentMetronome = metronomeMode;
        }
    }
    else
    {
        rhythmWasEnabled = false;
        rhythmCountInActive = false;
        rhythmCountInBeatIndex = 0;
        rhythmCountInTotalBeats = 0;
        rhythmCountInTailFadeSamplesRemaining = 0;
        rhythmCountInTailFadeLengthSamples = 0;
        rhythmLaunchFadeSamplesRemaining = 0;
        rhythmLaunchFadeLengthSamples = 0;
        rhythmPendingSectionDisplay.store(-1);
        rhythmCountInBeatsRemainingDisplay.store(-1);
    }

    if (drumOn && !drumWasEnabled)
    {
        drumTriggerArmed = true;
        drumWasEnabled = true;
    }
    else if (!drumOn)
    {
        drumTriggerArmed = true;
        drumWasEnabled = false;
    }

    // =========================================================================
    // 2. PRE-COMPUTO de valores derivados — eliminan calculos del loop interno.
    // =========================================================================

    // Compressor: pow solo una vez
    const float cThresh = std::pow(10.0f, compThresholdDb.load() / 20.0f);
    const float cAttackMs = juce::jmap(juce::jlimit(0.0f, 1.0f, cAction), 2.0f, 80.0f);
    const float cReleaseMs = juce::jmap(juce::jlimit(0.0f, 1.0f, cTail), 35.0f, 900.0f);
    const float cAttackCoeff = std::exp(-1.0f / juce::jmax(1.0f, cAttackMs * 0.001f * (float)sampleRate_));
    const float cReleaseCoeff = std::exp(-1.0f / juce::jmax(1.0f, cReleaseMs * 0.001f * (float)sampleRate_));
    const float cOutputGain = juce::jlimit(0.0f, 3.0f, cGain);
    const float cEqAmount = juce::jlimit(-1.0f, 1.0f, cEq);
    const float cLowCoeff = juce::jlimit(0.0f, 1.0f, 220.0f / (float)sampleRate_ * 2.0f);
    const float cMidCoeff = juce::jlimit(0.0f, 1.0f, 1800.0f / (float)sampleRate_ * 2.0f);
    const float gateThreshold = std::pow(10.0f, gateThresholdDb / 20.0f);
    const float gateCloseThreshold = gateThreshold * 0.58f;
    const float gateEnvAttackCoeff = std::exp(-1.0f / juce::jmax(1.0f, 0.0015f * (float) sampleRate_));
    const float gateEnvReleaseCoeff = std::exp(-1.0f / juce::jmax(1.0f, 0.045f * (float) sampleRate_));
    const float gateOpenCoeff = std::exp(-1.0f / juce::jmax(1.0f, gateAttackMs * 0.001f * (float) sampleRate_));
    const float gateCloseCoeff = std::exp(-1.0f / juce::jmax(1.0f, gateReleaseMs * 0.001f * (float) sampleRate_));
    const float drumGateThresh = juce::jmap(juce::jlimit(0.0f, 1.0f, drumGate), 0.0015f, 0.045f);
    const float drumSensGain = juce::jmap(juce::jlimit(0.0f, 1.0f, drumSens), 1.2f, 10.0f);
    const int drumCooldownTargetSamples = juce::jmax(1, (int) ((drumCooldownMs / 1000.0f) * sampleRate_));
    const int drumReleaseTargetSamples = juce::jmax(3, drumCooldownTargetSamples / 5);
    const float drumPeakShape = juce::jlimit(0.0f, 1.0f, drumTriggerDecay.load());
    const float drumTransientThresh = drumGateThresh * juce::jmap(drumPeakShape, 0.55f, 1.35f);
    const float drumRearmThresh = drumGateThresh * juce::jmap(drumPeakShape, 0.95f, 0.58f);
    const float drumRearmTransientThresh = drumTransientThresh * 0.34f;
    const float rhythmToneCoeff = juce::jmap(juce::jlimit(0.0f, 1.0f, rhythmToneValue), 0.045f, 0.22f);

    // Pitch shifter: std::pow FUERA del loop (antes eran 2 pows x44100 veces/seg)
    const float pitchDeltaHarm = 1.0f - std::pow(2.0f, (float)harmInterval / 12.0f);
    // Fuzz y drive: factores constantes por bloque
    const float fuzzDriveFactor = fuzz.load() * 40.0f + 1.0f;
    const float driveDriveFactor= drive.load() * 12.0f + 1.0f;

    // Delay: tiempo en muestras constante por bloque
    const float currentDelaySamples = (delTime / 1000.0f) * (float)sampleRate_;

    // Chorus: componente fija de delay
    const float choBaseDelay = (float)sampleRate_ * 0.015f;
    const float choVarDelay  = (float)sampleRate_ * 0.007f * choDepth;
    const int freezeTargetLen = juce::jlimit(96, kFreezeBufLen - 1,
                                             (int)((frzWindowMs / 1000.0f) * (float)sampleRate_));
    const int chordFreezeTargetLen = juce::jlimit(256, kFreezeBufLen - 1,
                                                  (int)((chordFrzWindowMs / 1000.0f) * (float)sampleRate_));

    // Chorus: incremento de fase constante por bloque
    const double deltaChorusPhase = (2.0 * juce::MathConstants<double>::pi * (double)choRate) / sampleRate_;
    const double deltaTremoloPhase = (2.0 * juce::MathConstants<double>::pi * (double)trRate) / sampleRate_;

    // Pitch: valores de ventana como floats, calculados una vez
    const int   winSize  = juce::jmax(256, (int)(0.04 * sampleRate_));
    const float winSizeF = (float)winSize;
    const float halfWinF = winSizeF * 0.5f;
    const float kPBLenF  = (float)kPitchBufLen;

    // Stutter setup (igual que antes — logica de estado)
    int channels = juce::jmin(buffer.getNumChannels(), 2);
    channels = juce::jmin(channels, pitchBuffer.getNumChannels());
    channels = juce::jmin(channels, delayBuffer.getNumChannels());
    channels = juce::jmin(channels, chorusBuffer.getNumChannels());
    channels = juce::jmin(channels, freezeBuffer.getNumChannels());
    channels = juce::jmin(channels, chordBodyBuffer.getNumChannels());
    if (channels <= 0 || numSamples <= 0)
        return;
    const int delayBufLen  = delayBuffer.getNumSamples();
    const int chorusBufLen = chorusBuffer.getNumSamples();
    const int freezeBufLen = freezeBuffer.getNumSamples();
    const int chordBodyBufLen = chordBodyBuffer.getNumSamples();
    const float freezeRiseSamples = juce::jmax(1.0f, (frzRiseMs / 1000.0f) * (float) sampleRate_);
    const float freezeTailSamples = juce::jmax(1.0f, (frzTailMs / 1000.0f) * (float) sampleRate_);
    const float freezeWetRiseStep = 1.0f / freezeRiseSamples;
    const float freezeWetTailStep = 1.0f / freezeTailSamples;
    const float freezeToneDecay = juce::jlimit(0.99985f, 0.9999995f,
                                               juce::jmap(juce::jlimit(0.0f, 1.0f, frzTone), 0.99985f, 0.9999995f));
    const float freezeToneBaseCoeff = juce::jmap(juce::jlimit(0.0f, 1.0f, frzTone), 0.010f, 0.095f);
    const double freezeAdvance = 1.0;
    const float freezeMakeupGain = 1.85f;
    const float freezeGrainDensity = juce::jlimit(0.0f, 1.0f, freezeDensity.load());
    const int freezeCrossfadeSamples = juce::jlimit(24,
                                                    juce::jmax(24, (int) (freezeTargetLen * juce::jmap(freezeGrainDensity, 0.10f, 0.42f))),
                                                    juce::jmax(32, freezeTargetLen / 2));
    const int activeFreezeGrains = juce::jlimit(4, EffectsChain::kChordFreezeGrainCount,
                                                4 + (int) std::round(freezeGrainDensity * (float) (EffectsChain::kChordFreezeGrainCount - 4)));
    const int freezeGrainLen = juce::jlimit((int) (sampleRate_ * 0.070),
                                            juce::jmax(128, (int) (sampleRate_ * juce::jmap(freezeGrainDensity, 0.120f, 0.210f))),
                                            juce::jmax(160, freezeTargetLen / 2));
    const float chordFreezeRiseSamples = juce::jmax(1.0f, (chordFrzRiseMs / 1000.0f) * (float) sampleRate_);
    const float chordFreezeTailSamples = juce::jmax(1.0f, (chordFrzTailMs / 1000.0f) * (float) sampleRate_);
    const float chordFreezeWetRiseStep = 1.0f / chordFreezeRiseSamples;
    const float chordFreezeWetTailStep = 1.0f / chordFreezeTailSamples;
    const float chordFreezeToneDecay = juce::jlimit(0.99992f, 0.9999997f,
                                                    juce::jmap(juce::jlimit(0.0f, 1.0f, chordFrzTone), 0.99992f, 0.9999997f));
    const float chordFreezeToneBaseCoeff = juce::jmap(juce::jlimit(0.0f, 1.0f, chordFrzTone), 0.010f, 0.095f);
    const float chordFreezeMakeupGain = 1.85f;
    const float chordGrainDensity = juce::jlimit(0.0f, 1.0f, chordFrzCrossfade);
    const int chordFreezeCrossfadeSamples = juce::jlimit(24,
                                                         juce::jmax(24, (int) (chordFreezeTargetLen * juce::jmap(chordGrainDensity, 0.10f, 0.42f))),
                                                         juce::jmax(32, chordFreezeTargetLen / 2));
    const int activeChordGrains = juce::jlimit(4, EffectsChain::kChordFreezeGrainCount,
                                               4 + (int) std::round(chordGrainDensity * (float) (EffectsChain::kChordFreezeGrainCount - 4)));
    const int chordGrainLen = juce::jlimit((int) (sampleRate_ * 0.070),
                                           juce::jmax(128, (int) (sampleRate_ * juce::jmap(chordGrainDensity, 0.120f, 0.210f))),
                                           juce::jmax(160, chordFreezeTargetLen / 2));

    // =========================================================================
    // 3. PUNTEROS CRUDOS — eliminan el bounds-checking de getSample/setSample
    //    en cada muestra de cada canal.
    // =========================================================================
    float* pitchPtr[2]   = {};
    float* delayPtr[2]   = {};
    float* chorusPtr[2]  = {};
    float* freezePtr[2]  = {};
    float* chordBodyPtr[2] = {};
    float* outPtr[2]     = {};

    for (int ch = 0; ch < channels; ++ch)
    {
        pitchPtr[ch]   = pitchBuffer.getWritePointer(ch);
        delayPtr[ch]   = delayBuffer.getWritePointer(ch);
        chorusPtr[ch]  = chorusBuffer.getWritePointer(ch);
        freezePtr[ch]  = freezeBuffer.getWritePointer(ch);
        chordBodyPtr[ch] = chordBodyBuffer.getWritePointer(ch);
        outPtr[ch]     = buffer.getWritePointer(ch) + startSample;
    }

    auto readFreezeSample = [&](const SustainLayer& layer, int ch, double position)
    {
        while (position < 0.0)
            position += (double) layer.capturedLen;
        while (position >= (double) layer.capturedLen)
            position -= (double) layer.capturedLen;

        const int idxA = ((int) position) % layer.capturedLen;
        const int idxB = (idxA + 1) % layer.capturedLen;
        const float frac = (float) (position - std::floor(position));
        const float* layerPtr = layer.buffer.getReadPointer(ch);
        return layerPtr[idxA] + frac * (layerPtr[idxB] - layerPtr[idxA]);
    };

    auto chooseChordGrainStart = [&](const SustainLayer& layer, int avoidStart)
    {
        if (layer.capturedLen <= juce::jmax(2, layer.grainLen))
            return 0;

        const int maxStart = juce::jmax(1, layer.capturedLen - layer.grainLen - 1);
        int bestStart = juce::Random::getSystemRandom().nextInt(maxStart);
        float bestScore = std::numeric_limits<float>::max();

        for (int attempt = 0; attempt < 6; ++attempt)
        {
            const int start = juce::Random::getSystemRandom().nextInt(maxStart);
            const int distance = std::abs(start - avoidStart);
            const float distancePenalty = distance < layer.grainLen / 2 ? 0.25f : 0.0f;
            float edge = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                const float* ptr = layer.buffer.getReadPointer(ch);
                const int end = juce::jmin(layer.capturedLen - 1, start + layer.grainLen - 1);
                edge += std::abs(ptr[start] - ptr[end]);
            }

            edge /= (float) juce::jmax(1, channels);
            const float score = edge + distancePenalty;
            if (score < bestScore)
            {
                bestScore = score;
                bestStart = start;
            }
        }

        return bestStart;
    };


    auto analyseFreezeSourceComplexity = [&]()
    {
        const int analysisLen = juce::jlimit(96, juce::jmax(96, freezeTargetLen / 2), 640);
        const int start = freezeWritePos - analysisLen;
        int zeroCrossings = 0;
        float avgAbs = 0.0f;
        float diffAbs = 0.0f;
        float prevMono = 0.0f;
        bool hasPrev = false;

        auto wrappedIndex = [freezeBufLen](int idx)
        {
            while (idx < 0)
                idx += freezeBufLen;
            while (idx >= freezeBufLen)
                idx -= freezeBufLen;
            return idx;
        };

        for (int i = 0; i < analysisLen; ++i)
        {
            const int idx = wrappedIndex(start + i);
            float mono = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
                mono += freezePtr[ch][idx];
            mono /= (float) juce::jmax(1, channels);

            avgAbs += std::abs(mono);
            if (hasPrev)
            {
                if ((mono >= 0.0f) != (prevMono >= 0.0f))
                    ++zeroCrossings;
                diffAbs += std::abs(mono - prevMono);
            }

            prevMono = mono;
            hasPrev = true;
        }

        avgAbs /= (float) analysisLen;
        diffAbs /= (float) juce::jmax(1, analysisLen - 1);

        const float zcRatio = (float) zeroCrossings / (float) juce::jmax(1, analysisLen - 1);
        const float busyRatio = diffAbs / (avgAbs + 1.0e-5f);

        return juce::jlimit(0.0f, 1.0f,
                             juce::jmap(zcRatio, 0.02f, 0.24f, 0.0f, 1.0f) * 0.55f
                           + juce::jmap(busyRatio, 0.10f, 1.25f, 0.0f, 1.0f) * 0.45f);
    };

    auto chooseFreezeCaptureStart = [&](int captureLen, float complexityHint)
    {
        const int attackSkip = juce::jmax(24, (int) (sampleRate_ * 0.014));
        const int extraDecaySeek = (int) ((0.015f + 0.050f * complexityHint) * (float) sampleRate_);
        const int minOffset = captureLen + attackSkip + extraDecaySeek;
        const int maxOffset = juce::jmin(freezeBufLen - 2,
                                         minOffset + juce::jmax(captureLen / 2, (int) (sampleRate_ * (0.08f + 0.05f * complexityHint))));
        const int step = juce::jmax(8, captureLen / 20);
        float bestScore = std::numeric_limits<float>::max();
        int bestStart = freezeWritePos - captureLen;

        auto wrappedIndex = [freezeBufLen](int idx)
        {
            while (idx < 0)
                idx += freezeBufLen;
            while (idx >= freezeBufLen)
                idx -= freezeBufLen;
            return idx;
        };

        for (int offset = minOffset; offset <= maxOffset; offset += step)
        {
            const int start = wrappedIndex(freezeWritePos - offset);
            const int end = wrappedIndex(start + captureLen - 1);
            const int quarter = juce::jmax(8, captureLen / 4);

            float seam = 0.0f;
            float attackEnergy = 0.0f;
            float tailEnergy = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                seam += std::abs(freezePtr[ch][start] - freezePtr[ch][end]);

                for (int i = 0; i < quarter; ++i)
                {
                    const int attackIdx = wrappedIndex(start + i);
                    const int tailIdx = wrappedIndex(end - i);
                    attackEnergy += std::abs(freezePtr[ch][attackIdx]);
                    tailEnergy += std::abs(freezePtr[ch][tailIdx]);
                }
            }

            seam /= (float) juce::jmax(1, channels);
            attackEnergy /= (float) juce::jmax(1, channels * quarter);
            tailEnergy /= (float) juce::jmax(1, channels * quarter);

            if (attackEnergy < 0.005f)
                continue;

            const float decayRatio = tailEnergy / (attackEnergy + 1.0e-5f);
            const float targetDecay = juce::jmap(complexityHint, 0.68f, 0.82f);
            const float decayPenalty = std::abs(decayRatio - targetDecay);
            const float silencePenalty = tailEnergy < 0.0025f ? 0.18f : 0.0f;
            const float recencyPenalty = 0.00001f * (float) (offset - minOffset);
            const float seamWeight = juce::jmap(complexityHint, 1.2f, 1.8f);
            const float score = seam * seamWeight + decayPenalty * 0.30f + silencePenalty + recencyPenalty;

            if (score < bestScore)
            {
                bestScore = score;
                bestStart = start;
            }
        }

        while (bestStart < 0)
            bestStart += freezeBufLen;
        while (bestStart >= freezeBufLen)
            bestStart -= freezeBufLen;
        return bestStart;
    };

    auto startFreezeCapture = [&]()
    {
        freezeHoldActive = true;
        for (auto& otherLayer : sustainLayers)
        {
            otherLayer.active = false;
            otherLayer.wetLevel = 0.0f;
        }

        auto& layer = sustainLayers[0];
        const int captureLen = freezeTargetLen;
        const int captureStart = chooseFreezeCaptureStart(captureLen, analyseFreezeSourceComplexity());
        layer.capturedLen = captureLen;
        layer.readPhase = 0.0;
        layer.grainPhase = 0.0;
        layer.grainLen = juce::jmin(freezeGrainLen, juce::jmax(64, captureLen / 2));
        layer.active = true;
        layer.wetLevel = 0.0f;
        layer.brightness = 1.0f;
        layer.smooth[0] = layer.smooth[1] = 0.0f;
        layer.toneState[0] = layer.toneState[1] = 0.0f;

        for (int ch = 0; ch < channels; ++ch)
        {
            float* dst = layer.buffer.getWritePointer(ch);
            for (int i = 0; i < captureLen; ++i)
                dst[i] = freezePtr[ch][(captureStart + i) % freezeBufLen];

            const int crossfadeLen = juce::jmin(freezeCrossfadeSamples,
                                                juce::jmax(8, captureLen / 3));
            for (int i = 0; i < crossfadeLen; ++i)
            {
                const float blend = (float) i / (float) juce::jmax(1, crossfadeLen - 1);
                const float fadeOut = std::cos(blend * juce::MathConstants<float>::halfPi);
                const float fadeIn  = std::sin(blend * juce::MathConstants<float>::halfPi);
                const int startIdx = i;
                const int endIdx = captureLen - crossfadeLen + i;
                const float a = dst[startIdx];
                const float b = dst[endIdx];
                const float merged = b * fadeOut + a * fadeIn;
                dst[startIdx] = merged;
                dst[endIdx] = merged;
            }

            float avgEnv = 0.0f;
            for (int i = 0; i < captureLen; ++i)
                avgEnv += std::abs(dst[i]);
            avgEnv /= (float) juce::jmax(1, captureLen);

            const float targetEnv = juce::jmax(0.004f, avgEnv);
            float env = targetEnv;
            const float envFollow = 0.030f;

            for (int i = 0; i < captureLen; ++i)
            {
                env += envFollow * (std::abs(dst[i]) - env);
                const float gain = juce::jlimit(0.72f, 1.45f, targetEnv / (env + 1.0e-5f));
                dst[i] *= gain;
            }
        }

        int previousStart = -layer.grainLen;
        const float grainGain = 1.0f / juce::jmax(1.0f, (float) activeFreezeGrains * 0.50f);
        for (int i = 0; i < EffectsChain::kChordFreezeGrainCount; ++i)
        {
            auto& grain = freezeGrains[(size_t) i];
            grain.length = layer.grainLen;
            grain.phase = (double) grain.length * (double) i / (double) EffectsChain::kChordFreezeGrainCount;
            grain.start = chooseChordGrainStart(layer, previousStart);
            grain.rate = 0.9975 + (double) (i % 5) * 0.00125;
            grain.gain = i < activeFreezeGrains ? grainGain : 0.0f;
            previousStart = grain.start;
        }
    };

    auto chooseChordFreezeCaptureStart = [&]()
    {
        const int attackSkip = juce::jmax(64, (int) (sampleRate_ * 0.035));
        const int minOffset = chordFreezeTargetLen + attackSkip;
        const int maxOffset = juce::jmin(freezeBufLen - 2,
                                         minOffset + juce::jmax(chordFreezeTargetLen / 2, (int) (sampleRate_ * 0.12)));
        const int step = juce::jmax(12, chordFreezeTargetLen / 24);
        float bestScore = std::numeric_limits<float>::max();
        int bestStart = freezeWritePos - chordFreezeTargetLen;

        auto wrappedIndex = [freezeBufLen](int idx)
        {
            while (idx < 0)
                idx += freezeBufLen;
            while (idx >= freezeBufLen)
                idx -= freezeBufLen;
            return idx;
        };

        for (int offset = minOffset; offset <= maxOffset; offset += step)
        {
            const int start = wrappedIndex(freezeWritePos - offset);
            const int end = wrappedIndex(start + chordFreezeTargetLen - 1);
            const int edgeLen = juce::jmax(16, chordFreezeCrossfadeSamples);
            float seam = 0.0f;
            float bodyEnergy = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                for (int i = 0; i < edgeLen; ++i)
                {
                    const int a = wrappedIndex(start + i);
                    const int b = wrappedIndex(end - edgeLen + 1 + i);
                    seam += std::abs(freezePtr[ch][a] - freezePtr[ch][b]);
                }

                for (int i = 0; i < chordFreezeTargetLen; i += juce::jmax(8, chordFreezeTargetLen / 32))
                    bodyEnergy += std::abs(freezePtr[ch][wrappedIndex(start + i)]);
            }

            seam /= (float) juce::jmax(1, channels * edgeLen);
            bodyEnergy /= (float) juce::jmax(1, channels * juce::jmax(1, chordFreezeTargetLen / juce::jmax(8, chordFreezeTargetLen / 32)));

            const float silencePenalty = bodyEnergy < 0.004f ? 0.5f : 0.0f;
            const float recencyPenalty = 0.00001f * (float) (offset - minOffset);
            const float score = seam * 1.7f + silencePenalty + recencyPenalty;
            if (score < bestScore)
            {
                bestScore = score;
                bestStart = start;
            }
        }

        while (bestStart < 0)
            bestStart += freezeBufLen;
        while (bestStart >= freezeBufLen)
            bestStart -= freezeBufLen;
        return bestStart;
    };

    auto startChordFreezeCapture = [&]()
    {
        chordFreezeHoldActive = true;
        chordBodyBuffer.clear();
        chordBodyWritePos = 0;
        chordBodyDamp[0] = chordBodyDamp[1] = 0.0f;
        chordSustainLayer.capturedLen = chordFreezeTargetLen;
        chordSustainLayer.readPhase = 0.0;
        chordSustainLayer.grainPhase = 0.0;
        chordSustainLayer.grainLen = juce::jmin(chordGrainLen, juce::jmax(64, chordFreezeTargetLen / 2));
        chordSustainLayer.active = true;
        chordSustainLayer.wetLevel = 0.0f;
        chordSustainLayer.brightness = 1.0f;
        chordSustainLayer.smooth[0] = chordSustainLayer.smooth[1] = 0.0f;
        chordSustainLayer.toneState[0] = chordSustainLayer.toneState[1] = 0.0f;

        const int captureStart = chooseChordFreezeCaptureStart();
        for (int ch = 0; ch < channels; ++ch)
        {
            float* dst = chordSustainLayer.buffer.getWritePointer(ch);
            for (int i = 0; i < chordFreezeTargetLen; ++i)
                dst[i] = freezePtr[ch][(captureStart + i) % freezeBufLen];

            const int crossfadeLen = juce::jmin(chordFreezeCrossfadeSamples,
                                                juce::jmax(8, chordFreezeTargetLen / 3));
            for (int i = 0; i < crossfadeLen; ++i)
            {
                const float blend = (float) i / (float) juce::jmax(1, crossfadeLen - 1);
                const float fadeOut = std::cos(blend * juce::MathConstants<float>::halfPi);
                const float fadeIn  = std::sin(blend * juce::MathConstants<float>::halfPi);
                const int startIdx = i;
                const int endIdx = chordFreezeTargetLen - crossfadeLen + i;
                const float a = dst[startIdx];
                const float b = dst[endIdx];
                const float merged = b * fadeOut + a * fadeIn;
                dst[startIdx] = merged;
                dst[endIdx] = merged;
            }

            // Flatten the internal level movement a bit so held chords feel steadier.
            float avgEnv = 0.0f;
            for (int i = 0; i < chordFreezeTargetLen; ++i)
                avgEnv += std::abs(dst[i]);
            avgEnv /= (float) juce::jmax(1, chordFreezeTargetLen);

            const float targetEnv = juce::jmax(0.004f, avgEnv);
            float env = targetEnv;
            const float envFollow = 0.030f;

            for (int i = 0; i < chordFreezeTargetLen; ++i)
            {
                env += envFollow * (std::abs(dst[i]) - env);
                const float gain = juce::jlimit(0.72f, 1.45f, targetEnv / (env + 1.0e-5f));
                dst[i] *= gain;
            }
        }

        int previousStart = -chordSustainLayer.grainLen;
        const float grainGain = 1.0f / juce::jmax(1.0f, (float) activeChordGrains * 0.50f);
        for (int i = 0; i < EffectsChain::kChordFreezeGrainCount; ++i)
        {
            auto& grain = chordFreezeGrains[(size_t) i];
            grain.length = chordSustainLayer.grainLen;
            grain.phase = (double) grain.length * (double) i / (double) EffectsChain::kChordFreezeGrainCount;
            grain.start = chooseChordGrainStart(chordSustainLayer, previousStart);
            grain.rate = 0.9975 + (double) (i % 5) * 0.00125;
            grain.gain = i < activeChordGrains ? grainGain : 0.0f;
            previousStart = grain.start;
        }
    };

    if (freezeOn && !freezeHoldActive)
        startFreezeCapture();
    else if (!freezeOn && freezeHoldActive)
        freezeHoldActive = false;

    if (chordFreezeOn && !chordFreezeHoldActive)
        startChordFreezeCapture();
    else if (!chordFreezeOn && chordFreezeHoldActive)
        chordFreezeHoldActive = false;

    // =========================================================================
    // 4. LOOP PRINCIPAL — limpio de allocaciones, pows y atomic loads.
    // =========================================================================
    for (int s = 0; s < numSamples; ++s)
    {
        if (rhythmOn)
            advanceRhythmSequencer(rhythmTempo, rhythmSwingValue, metronomeMode, rhythmStyleIndex);

        if (gateOn)
        {
            float gateInput = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
                gateInput = juce::jmax(gateInput, std::abs(outPtr[ch][s]));

            const float envCoeff = gateInput > noiseGateEnv ? gateEnvAttackCoeff : gateEnvReleaseCoeff;
            noiseGateEnv = gateInput + envCoeff * (noiseGateEnv - gateInput);

            const float targetGain = noiseGateEnv >= gateThreshold ? 1.0f
                                   : (noiseGateEnv <= gateCloseThreshold ? 0.0f : noiseGateGain);
            const float gainCoeff = targetGain > noiseGateGain ? gateOpenCoeff : gateCloseCoeff;
            noiseGateGain = targetGain + gainCoeff * (noiseGateGain - targetGain);
        }
        else
        {
            noiseGateEnv = 0.0f;
            noiseGateGain = 1.0f;
        }

        float detectorInputSample = 0.0f;
        float detectorProcessedSample = 0.0f;
        if (drumOn)
        {
            for (int ch = 0; ch < channels; ++ch)
                detectorInputSample = juce::jmax(detectorInputSample, std::abs(outPtr[ch][s]));

            if (detectorInputSample > 0.00001f)
                drumNoInputCounter = 0;
            else
                ++drumNoInputCounter;
        }

        float drumBusSample[2]{};
        if (rhythmOn || drumOn)
        {
            const auto kickFrame = renderDrumFrame(DrumInstrument::Kick);
            const auto snareFrame = renderDrumFrame(DrumInstrument::Snare);
            const auto hatFrame = renderDrumFrame(DrumInstrument::Hat);

            for (int ch = 0; ch < channels; ++ch)
            {
                float baseRhythmSample =
                    (kickFrame[(size_t) ch] * drumKickMix
                   + snareFrame[(size_t) ch] * drumSnareMix
                   + hatFrame[(size_t) ch] * drumHatMix
                   + renderRhythmSample(RhythmSampleSlot::Accent, ch)
                   + renderRhythmSample(RhythmSampleSlot::Tick, ch))
                    * (kDrumMixGain * (rhythmOn ? rhythmLevelValue : drumLevel));

                float countInSample =
                    (renderRhythmSample(RhythmSampleSlot::CountInAccent, ch)
                   + renderRhythmSample(RhythmSampleSlot::CountInTick, ch))
                    * (kDrumMixGain * (rhythmOn ? rhythmLevelValue : drumLevel));

                rhythmToneState[ch] += rhythmToneCoeff * (baseRhythmSample - rhythmToneState[ch]);
                const float lowBand = rhythmToneState[ch];
                const float highBand = baseRhythmSample - lowBand;
                const float tonePos = juce::jlimit(0.0f, 1.0f, rhythmToneValue);

                if (tonePos < 0.5f)
                {
                    const float warmMix = tonePos * 2.0f;
                    baseRhythmSample = juce::jmap(warmMix, lowBand * 1.12f, baseRhythmSample);
                }
                else
                {
                    const float brightMix = (tonePos - 0.5f) * 2.0f;
                    baseRhythmSample = juce::jmap(brightMix, baseRhythmSample, baseRhythmSample + highBand * 1.35f);
                }

                if (rhythmLaunchFadeSamplesRemaining > 0 && rhythmLaunchFadeLengthSamples > 0)
                {
                    const float fadeProgress = 1.0f - ((float) rhythmLaunchFadeSamplesRemaining / (float) rhythmLaunchFadeLengthSamples);
                    const float fadeIn = juce::jmap(juce::jlimit(0.0f, 1.0f, fadeProgress), 0.78f, 1.0f);
                    baseRhythmSample *= fadeIn;
                }

                if (rhythmCountInTailFadeSamplesRemaining > 0 && rhythmCountInTailFadeLengthSamples > 0)
                {
                    const float fadeOut = (float) rhythmCountInTailFadeSamplesRemaining / (float) rhythmCountInTailFadeLengthSamples;
                    countInSample *= juce::jlimit(0.0f, 1.0f, fadeOut);
                }

                drumBusSample[ch] = baseRhythmSample + countInSample;
            }

            if (rhythmCountInTailFadeSamplesRemaining > 0)
                --rhythmCountInTailFadeSamplesRemaining;
            if (rhythmLaunchFadeSamplesRemaining > 0)
                --rhythmLaunchFadeSamplesRemaining;

            advanceRhythmSampleVoices();

            float drumOutMeter = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
                drumOutMeter = juce::jmax(drumOutMeter, std::abs(drumBusSample[ch]));

            drumOutputDisplay = juce::jmax(drumOutputDisplay * 0.992f,
                                           juce::jlimit(0.0f, 1.0f, drumOutMeter * 2.0f));
            drumOutputLevel.store(drumOutputDisplay);
            int activeVoices = (kickVoice.active ? 1 : 0) + (snareVoice.active ? 1 : 0) + (hatVoice.active ? 1 : 0);
            for (const auto& voice : rhythmSampleVoices)
                activeVoices += voice.active ? 1 : 0;
            drumActiveVoices.store(activeVoices);
        }
        else
        {
            drumOutputDisplay *= 0.96f;
            drumOutputLevel.store(0.0f);
            drumDetectedPitchHz.store(0.0f);
            drumActiveVoices.store(0);
        }

        for (int ch = 0; ch < channels; ++ch)
        {
            float sample = outPtr[ch][s] * noiseGateGain;
            const float pitchInputSample = sample;

            if (rhythmOn)
            {
                if (rhythmSoloMode)
                    sample = drumBusSample[ch];
                else
                    sample += drumBusSample[ch];
            }

            // Señal seca para los efectos de pitch existentes.
            pitchPtr[ch][pitchWritePos] = pitchInputSample;
            // La voz octavada de una 12 cuerdas sólo debe reforzar el registro
            // de E/A/D/G. Dos polos a ~300 Hz atenúan las parejas de B y e,
            // que en una guitarra de 12 cuerdas son unísonas.
            freezePtr[ch][freezeWritePos] = sample;

            #if 0
            if (tunerOn && ch == channels - 1)
            {
                if (++tunerSampleCounter >= juce::jmax(128, (int) (sampleRate_ * 0.018)))
                {
                    tunerSampleCounter = 0;
                    const float detectedHz = estimateRecentPitchHz(0, pitchWritePos);
                    // La detección ya valida energía en toda su ventana. Mirar
                    // sólo este sample era inestable: podía coincidir con un
                    // cruce por cero de la cuerda y borrar la lectura.
                    if (detectedHz > 20.0f && detectedHz < 1800.0f)
                    {
                        tunerMissedDetections = 0;
                        tunerRecentHz[(size_t) tunerRecentHzWritePos] = detectedHz;
                        tunerRecentHzWritePos = (tunerRecentHzWritePos + 1) % (int) tunerRecentHz.size();
                        tunerRecentHzCount = juce::jmin((int) tunerRecentHz.size(), tunerRecentHzCount + 1);

                        const float filteredHz = medianFrequency(tunerRecentHz, tunerRecentHzCount);
                        const float jumpCents = centsBetweenFrequencies(filteredHz, tunerSmoothedHz);
                        const float smoothing = tunerSmoothedHz <= 0.0f ? 1.0f
                                              : jumpCents < 10.0f ? 0.10f
                                              : jumpCents < 35.0f ? 0.16f
                                                                  : 0.24f;
                        tunerSmoothedHz = tunerSmoothedHz <= 0.0f
                            ? filteredHz
                            : tunerSmoothedHz + smoothing * (filteredHz - tunerSmoothedHz);

                        const float midiFloat = 69.0f + 12.0f * std::log2(tunerSmoothedHz / tunerReference);
                        const int roundedMidi = juce::roundToInt(midiFloat);
                        if (tunerStableNoteIndex < 0)
                        {
                            tunerStableNoteIndex = roundedMidi;
                        }
                        else
                        {
                            const float semitoneDistance = midiFloat - (float) tunerStableNoteIndex;
                            if (std::abs(semitoneDistance) >= 0.65f)
                                tunerStableNoteIndex = roundedMidi;
                        }

                        const int pitchClass = (tunerStableNoteIndex % 12 + 12) % 12;
                        static constexpr float kGuitarSweetOffsets[12] = {
                            0.0f, 0.0f, 0.0f, -1.0f, -2.0f, 0.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, -2.0f
                        };
                        static constexpr float kBassSweetOffsets[12] = {
                            0.0f, 0.0f, 0.0f, -0.5f, -1.0f, 0.0f, 0.0f, -0.5f, 0.0f, -0.5f, 0.0f, -1.0f
                        };
                        const float sweetOffset = tunerSweetenerValue == 1 ? kGuitarSweetOffsets[pitchClass]
                                                : tunerSweetenerValue == 2 ? kBassSweetOffsets[pitchClass]
                                                                           : 0.0f;
                        const float rawCents = juce::jlimit(-50.0f, 50.0f,
                                                            (midiFloat - (float) tunerStableNoteIndex) * 100.0f - sweetOffset);
                        if (std::abs(rawCents) <= 2.0f)
                        {
                            tunerDisplayedCents = 0.0f;
                        }
                        else
                        {
                            const float centsSmoothing = std::abs(rawCents - tunerDisplayedCents) < 6.0f ? 0.18f : 0.30f;
                            tunerDisplayedCents += centsSmoothing * (rawCents - tunerDisplayedCents);
                            if (std::abs(tunerDisplayedCents) < 0.35f)
                                tunerDisplayedCents = 0.0f;
                        }

                        tunerFrequencyHz.store(tunerSmoothedHz);
                        tunerCents.store(tunerDisplayedCents);
                        tunerNoteIndex.store(tunerStableNoteIndex);
                        tunerHasSignal.store(true);
                    }
                    else
                    {
                        ++tunerMissedDetections;
                        // Conserva la última nota durante ~1.25 s al apagarse
                        // la cuerda, tiempo suficiente para leer y ajustar.
                        if (tunerMissedDetections >= 70)
                        {
                            tunerSmoothedHz = 0.0f;
                            tunerRecentHz.fill(0.0f);
                            tunerRecentHzWritePos = 0;
                            tunerRecentHzCount = 0;
                            tunerStableNoteIndex = -1;
                            tunerDisplayedCents = 0.0f;
                            tunerMissedDetections = 0;
                            tunerFrequencyHz.store(0.0f);
                            tunerCents.store(0.0f);
                            tunerNoteIndex.store(-1);
                            tunerHasSignal.store(false);
                        }
                    }
                }
            }

            #endif
            // --- COMPRESOR ---
            if (cOn)
            {
                const float dry = sample;
                const float absSample = std::abs(sample);
                const float detectorCoeff = absSample > compEnv[ch] ? cAttackCoeff : cReleaseCoeff;
                compEnv[ch] = absSample + detectorCoeff * (compEnv[ch] - absSample);

                float wet = sample;
                if (compEnv[ch] > cThresh && compEnv[ch] > 0.0001f)
                {
                    constexpr float kCompRatio = 4.0f;
                    const float over = compEnv[ch] / cThresh;
                    const float reducedOver = std::pow(over, (1.0f / kCompRatio) - 1.0f);
                    wet *= reducedOver;
                }

                wet *= cOutputGain;

                compLowBand[ch] += cLowCoeff * (wet - compLowBand[ch]);
                compMidLowBand[ch] += cMidCoeff * (wet - compMidLowBand[ch]);
                const float lowBand = compLowBand[ch];
                const float highBand = wet - compMidLowBand[ch];
                const float midBand = compMidLowBand[ch] - lowBand;

                float eqAccent = midBand * 1.15f;
                if (cEqAmount < 0.0f)
                    eqAccent = juce::jmap(-cEqAmount, midBand * 1.15f, lowBand * 1.3f);
                else if (cEqAmount > 0.0f)
                    eqAccent = juce::jmap(cEqAmount, midBand * 1.15f, highBand * 1.3f);

                wet += eqAccent * 0.45f;
                sample = dry * (1.0f - cMix) + wet * cMix;
            }

            if (drumOn)
                detectorProcessedSample = juce::jmax(detectorProcessedSample, std::abs(sample));

            if (drumOn && ch == channels - 1)
            {
                const float triggerInput = (drumPlacement == (int) DrumTriggerPlacement::BeforeCompressor)
                    ? detectorInputSample
                    : detectorProcessedSample;
                const float detector = std::abs(triggerInput) * drumSensGain;
                drumTriggerEnv[ch] += 0.55f * (detector - drumTriggerEnv[ch]);
                drumTriggerPulse[ch] += 0.004f * (detector - drumTriggerPulse[ch]);
                const float transient = juce::jmax(0.0f, drumTriggerEnv[ch] - drumTriggerPulse[ch]);
                const float detectorMeter = (drumNoInputCounter > (int)sampleRate_ / 8)
                    ? 0.0f
                    : juce::jlimit(0.0f, 1.0f, detector * 0.25f);
                drumDetectorDisplay = juce::jmax(drumDetectorDisplay * 0.992f, detectorMeter);
                drumTransientDisplay = juce::jmax(drumTransientDisplay * 0.992f,
                                                  juce::jlimit(0.0f, 1.0f, transient * 1.6f));
                drumDetectorLevel.store(drumDetectorDisplay);
                drumTransientLevel.store(drumTransientDisplay);

                drumLowBand += 0.030f * (detector - drumLowBand);
                const float highResidual = juce::jmax(0.0f, detector - drumLowBand);
                drumHighBand += 0.20f * (highResidual - drumHighBand);

                const float dynamicRearmThresh = juce::jmax(drumRearmThresh, drumLastTriggerLevel * 0.52f);
                if (detector < dynamicRearmThresh || transient < drumRearmTransientThresh)
                {
                    if (++drumReleaseSamples >= drumReleaseTargetSamples)
                    {
                        drumTriggerArmed = true;
                        drumPeakValue = 0.0f;
                        drumPeakLowBand = 0.0f;
                        drumPeakHighBand = 0.0f;
                        drumPeakTransient = 0.0f;
                        drumPeakSamples = 0;
                    }
                }
                else
                {
                    drumReleaseSamples = 0;
                }

                const bool risingAttack = detector > drumPrevPulse + drumGateThresh * 0.05f;
                const bool onset = drumTriggerArmed
                    && drumCooldownSamples <= 0
                    && detector > drumGateThresh
                    && transient > drumTransientThresh
                    && risingAttack;
                if (onset)
                {
                    drumPeakValue = detector;
                    drumPeakLowBand = drumLowBand;
                    drumPeakHighBand = drumHighBand;
                    drumPeakTransient = transient;

                    const float velocity = juce::jlimit(0.12f, 1.0f,
                        drumPeakValue * 0.64f + drumPeakTransient * 2.10f);
                    const float spectralTotal = drumPeakLowBand + drumPeakHighBand + 0.0001f;
                    const float brightness = drumPeakHighBand / spectralTotal;
                    const float attackRatio = drumPeakTransient / (drumPeakValue + 0.0001f);
                    const float detectedPitchHz = estimateRecentPitchHz(0, pitchWritePos);
                    DrumInstrument instrument = DrumInstrument::Snare;

                    drumDetectedPitchHz.store(detectedPitchHz);

                    const float kickPitchDelta = std::abs(detectedPitchHz - drumKickPitch);
                    const float snarePitchDelta = std::abs(detectedPitchHz - drumSnarePitch);
                    const float hatPitchDelta = std::abs(detectedPitchHz - drumHatPitch);
                    const float pitchTolerance = juce::jmax(6.0f, drumPitchTolerance);

                    if (detectedPitchHz > 0.0f
                        && (kickPitchDelta <= pitchTolerance
                            || snarePitchDelta <= pitchTolerance
                            || hatPitchDelta <= pitchTolerance))
                    {
                        if (kickPitchDelta <= snarePitchDelta && kickPitchDelta <= hatPitchDelta)
                            instrument = DrumInstrument::Kick;
                        else if (hatPitchDelta <= snarePitchDelta)
                            instrument = DrumInstrument::Hat;
                        else
                            instrument = DrumInstrument::Snare;
                    }
                    else
                    {
                        if (brightness < 0.22f || attackRatio < 0.44f)
                            instrument = DrumInstrument::Kick;
                        else if (brightness > 0.46f || attackRatio > 0.92f)
                            instrument = DrumInstrument::Hat;
                    }

                    drumLastTriggerSample = drumSampleCounter;
                    drumDensityCounter = juce::jmin(8, drumDensityCounter + 1);
                    drumCooldownSamples = drumCooldownTargetSamples;
                    drumTriggerArmed = false;
                    drumReleaseSamples = 0;
                    drumLastTriggerLevel = drumPeakValue;
                    registerDrumTrigger(instrument, velocity);

                    if (instrument == DrumInstrument::Snare && velocity > 0.42f)
                        triggerDrumVoice(DrumInstrument::Hat, juce::jlimit(0.10f, 0.32f, velocity * 0.16f));

                    drumPeakValue = 0.0f;
                    drumPeakLowBand = 0.0f;
                    drumPeakHighBand = 0.0f;
                    drumPeakTransient = 0.0f;
                    drumPeakSamples = 0;
                }

                drumPrevPulse = detector;
            }

            if (drumOn && !rhythmOn)
                sample = drumBusSample[ch];

            // --- ARMONIZADOR ---
            if (harmOn)
            {
                const float dry = sample;
                float rPos1 = (float)pitchWritePos - kPitchLookbackSamples - (float)pitchPhaseHarm;
                if (rPos1 < 0.0f) rPos1 += kPBLenF;
                if (rPos1 >= kPBLenF) rPos1 -= kPBLenF;
                const int   a1 = (int)rPos1;
                const int   b1 = (a1 + 1) % kPitchBufLen;
                const float f1 = rPos1 - (float)a1;
                const float s1 = pitchPtr[ch][a1] + f1 * (pitchPtr[ch][b1] - pitchPtr[ch][a1]);

                float p2 = (float)pitchPhaseHarm + halfWinF;
                if (p2 >= winSizeF) p2 -= winSizeF;
                float rPos2 = (float)pitchWritePos - kPitchLookbackSamples - p2;
                if (rPos2 < 0.0f) rPos2 += kPBLenF;
                if (rPos2 >= kPBLenF) rPos2 -= kPBLenF;
                const int   a2 = (int)rPos2;
                const int   b2 = (a2 + 1) % kPitchBufLen;
                const float f2 = rPos2 - (float)a2;
                const float s2 = pitchPtr[ch][a2] + f2 * (pitchPtr[ch][b2] - pitchPtr[ch][a2]);

                const float w1 = ((float)pitchPhaseHarm < halfWinF)
                    ? ((float)pitchPhaseHarm / halfWinF)
                    : ((winSizeF - (float)pitchPhaseHarm) / halfWinF);
                const float wet = s1 * w1 + s2 * (1.0f - w1);
                sample = dry * (1.0f - harmM) + wet * (harmM * harmL * 0.707f);
            }

            #if 0
            auto processTwelveString = [&]()
            {
                if (twelveDelayBufLen <= 1)
                    return;

                // El buffer sigue vivo estando en bypass: al activar el pedal
                // no puede salir audio de una interpretación anterior.
                const float dry = sample;
                twelveDelayPtr[ch][twelveStringDelayWritePos] = dry;

                const float targetBlend = twelveOn ? twelveMix : 0.0f;
                twelveStringBlend[ch] += 0.0045f * (targetBlend - twelveStringBlend[ch]);
                const float blend = twelveStringBlend[ch];
                if (blend < 0.0001f)
                    return;

                auto readPitchVoice = [&](double phase)
                {
                    float rPos1 = (float) pitchWritePos - kPitchLookbackSamples - (float) phase;
                    while (rPos1 < 0.0f) rPos1 += kPBLenF;
                    while (rPos1 >= kPBLenF) rPos1 -= kPBLenF;
                    const int a1 = (int) rPos1;
                    const int b1 = (a1 + 1) % kPitchBufLen;
                    const float f1 = rPos1 - (float) a1;
                    const float s1 = twelvePitchPtr[ch][a1] + f1 * (twelvePitchPtr[ch][b1] - twelvePitchPtr[ch][a1]);

                    float p2 = (float) phase + halfWinF;
                    if (p2 >= winSizeF) p2 -= winSizeF;
                    float rPos2 = (float) pitchWritePos - kPitchLookbackSamples - p2;
                    while (rPos2 < 0.0f) rPos2 += kPBLenF;
                    while (rPos2 >= kPBLenF) rPos2 -= kPBLenF;
                    const int a2 = (int) rPos2;
                    const int b2 = (a2 + 1) % kPitchBufLen;
                    const float f2 = rPos2 - (float) a2;
                    const float s2 = twelvePitchPtr[ch][a2] + f2 * (twelvePitchPtr[ch][b2] - twelvePitchPtr[ch][a2]);

                    const float w1 = ((float) phase < halfWinF)
                        ? ((float) phase / halfWinF)
                        : ((winSizeF - (float) phase) / halfWinF);
                    return s1 * w1 + s2 * (1.0f - w1);
                };

                int delayIndex = twelveStringDelayWritePos - twelveDelaySamples;
                while (delayIndex < 0)
                    delayIndex += twelveDelayBufLen;

                const int widthChannel = (channels > 1 && twelveWidth > 0.0f) ? 1 - ch : ch;
                const float delayedSame = twelveDelayPtr[ch][delayIndex];
                const float delayedWide = twelveDelayPtr[widthChannel][delayIndex];

                const float octaveA = readPitchVoice(twelveStringPhaseA);
                const float octaveB = readPitchVoice(twelveStringPhaseB);
                const float octaveSpread = channels > 1
                    ? (ch == 0 ? octaveA * (1.0f - twelveWidth * 0.35f) + octaveB * (twelveWidth * 0.35f)
                               : octaveB * (1.0f - twelveWidth * 0.35f) + octaveA * (twelveWidth * 0.35f))
                    : (octaveA + octaveB) * 0.5f;

                lowPassFilters[4][ch] += 0.030f * (octaveSpread - lowPassFilters[4][ch]);
                lowPassFilters[5][ch] += 0.040f * (dry - lowPassFilters[5][ch]);
                const float octaveAir = octaveSpread - lowPassFilters[4][ch];
                const float octaveCourse = lowPassFilters[4][ch] * (0.72f + twelveBody * 0.20f)
                                         + octaveAir * (0.05f + twelveOctave * 0.10f);
                const float delayedCourse = delayedSame * (1.0f - twelveWidth * 0.45f) + delayedWide * (twelveWidth * 0.45f);
                const float bodyBloom = lowPassFilters[5][ch] * (0.10f + twelveBody * 0.18f);
                const float wet = delayedCourse * (0.42f + twelveBody * 0.30f)
                                + octaveCourse * (0.16f + twelveOctave * 0.30f)
                                + bodyBloom;

                // Nivel estable y bypass transparente. La suma representa las
                // cuerdas extra pero se normaliza para no provocar saltos al
                // conmutar; la saturación no forma parte de una 12 cuerdas.
                // La pareja al unísono tiene que ser claramente audible. La
                // versión anterior la normalizaba en exceso y, con la mezcla
                // predeterminada, casi no alteraba el sonido de la guitarra.
                sample = dry * (1.0f - blend * 0.20f) + wet * blend * 0.95f;
            };

            #endif
            auto processFuzz = [&]()
            {
                if (fzOn)
                {
                    const float dry = sample;
                    float wet = sample * fuzzDriveFactor;
                    if (wet >  0.4f) wet =  0.4f + (wet - 0.4f) * 0.05f;
                    if (wet < -0.3f) wet = -0.3f + (wet + 0.3f) * 0.05f;
                    lowPassFilters[1][ch] += fzT * (wet - lowPassFilters[1][ch]);
                    sample = dry * (1.0f - fzL * 0.5f) + lowPassFilters[1][ch] * (fzL * 0.8f);
                }
            };

            auto processOverdrive = [&]()
            {
                if (drOn)
                {
                    const float dry = sample;
                    float wet = std::atan(sample * driveDriveFactor) / juce::MathConstants<float>::halfPi;
                    lowPassFilters[2][ch] += drT * (wet - lowPassFilters[2][ch]);
                    sample = dry * (1.0f - drL * 0.4f) + lowPassFilters[2][ch] * (drL * 0.75f);
                }
            };

            auto processSustainGran = [&]()
            {
                freezePtr[ch][freezeWritePos] = sample;

                auto& sustainLayer = sustainLayers[0];
                if (sustainLayer.active && sustainLayer.capturedLen > 0 && sustainLayer.wetLevel > 0.0001f)
                {
                    float wet = 0.0f;
                    float windowSum = 0.0f;
                    for (const auto& grain : freezeGrains)
                    {
                        if (grain.gain <= 0.0f || grain.length <= 0)
                            continue;

                        const float norm = (float) (grain.phase / (double) juce::jmax(1, grain.length));
                        const float window = 0.5f - 0.5f * std::cos(norm * juce::MathConstants<float>::twoPi);
                        wet += readFreezeSample(sustainLayer, ch, (double) grain.start + grain.phase) * window * grain.gain;
                        windowSum += window * grain.gain;
                    }

                    wet = windowSum > 0.0001f ? wet / windowSum : 0.0f;
                    sustainLayer.smooth[ch] += 0.018f * (wet - sustainLayer.smooth[ch]);
                    wet = sustainLayer.smooth[ch];
                    if (ch == channels - 1)
                        sustainLayer.brightness = juce::jmax(0.25f, sustainLayer.brightness * freezeToneDecay);
                    sustainLayer.toneState[ch] += (freezeToneBaseCoeff * sustainLayer.brightness)
                                                * (wet - sustainLayer.toneState[ch]);
                    wet = sustainLayer.toneState[ch];

                    const float sustainWet = wet * (frzMix * freezeMakeupGain * sustainLayer.wetLevel);
                    sample = frzWetOnly ? sustainWet : (sample + sustainWet);
                }
            };

            auto processSustainChords = [&]()
            {
                freezePtr[ch][freezeWritePos] = sample;

                if (chordSustainLayer.active && chordSustainLayer.capturedLen > 0 && chordSustainLayer.wetLevel > 0.0001f)
                {
                    float chordWet = 0.0f;
                    float windowSum = 0.0f;
                    for (const auto& grain : chordFreezeGrains)
                    {
                        if (grain.gain <= 0.0f || grain.length <= 0)
                            continue;

                        const float norm = (float) (grain.phase / (double) juce::jmax(1, grain.length));
                        const float window = 0.5f - 0.5f * std::cos(norm * juce::MathConstants<float>::twoPi);
                        chordWet += readFreezeSample(chordSustainLayer, ch, (double) grain.start + grain.phase) * window * grain.gain;
                        windowSum += window * grain.gain;
                    }

                    chordWet = windowSum > 0.0001f ? chordWet / windowSum : 0.0f;
                    chordSustainLayer.smooth[ch] += 0.018f * (chordWet - chordSustainLayer.smooth[ch]);
                    chordWet = chordSustainLayer.smooth[ch];
                    if (ch == channels - 1)
                        chordSustainLayer.brightness = juce::jmax(0.25f, chordSustainLayer.brightness * chordFreezeToneDecay);
                    chordSustainLayer.toneState[ch] += (chordFreezeToneBaseCoeff * chordSustainLayer.brightness)
                                                    * (chordWet - chordSustainLayer.toneState[ch]);
                    chordWet = chordSustainLayer.toneState[ch];

                    const int bodyTapA = juce::jlimit(1, chordBodyBufLen - 1, (int) (sampleRate_ * 0.011f));
                    const int bodyTapB = juce::jlimit(1, chordBodyBufLen - 1, (int) (sampleRate_ * 0.019f));
                    const int bodyTapC = juce::jlimit(1, chordBodyBufLen - 1, (int) (sampleRate_ * 0.031f));
                    const int bodyTapD = juce::jlimit(1, chordBodyBufLen - 1, (int) (sampleRate_ * 0.047f));
                    auto readChordBody = [&](int delaySamples)
                    {
                        int idx = chordBodyWritePos - delaySamples;
                        while (idx < 0)
                            idx += chordBodyBufLen;
                        return chordBodyPtr[ch][idx];
                    };

                    const float body = readChordBody(bodyTapA) * 0.36f
                                     - readChordBody(bodyTapB) * 0.24f
                                     + readChordBody(bodyTapC) * 0.18f
                                     - readChordBody(bodyTapD) * 0.12f;
                    chordBodyDamp[ch] += 0.14f * (body - chordBodyDamp[ch]);
                    chordBodyPtr[ch][chordBodyWritePos] = chordWet + chordBodyDamp[ch] * 0.32f;
                    chordWet += chordBodyDamp[ch] * 0.22f;

                    sample += chordWet * (chordFrzMix * chordFreezeMakeupGain * chordSustainLayer.wetLevel);
                }
            };

            auto processChorus = [&]()
            {
                chorusPtr[ch][chorusWritePos] = sample;
                if (choOn)
                {
                    const float lfo = std::sin((float)chorusPhase
                                      + (float)ch * juce::MathConstants<float>::halfPi) * 0.5f + 0.5f;
                    float rPos = (float)chorusWritePos - (choBaseDelay + lfo * choVarDelay);
                    while (rPos < 0.0f) rPos += (float)chorusBufLen;
                    const int ia = (int)rPos;
                    const int ib = (ia + 1) % chorusBufLen;
                    const float wet = chorusPtr[ch][ia] * 0.5f + chorusPtr[ch][ib] * 0.5f;
                    sample = sample * (1.0f - choM * 0.5f) + wet * (choM * 0.5f);
                }
            };

            auto processTremolo = [&]()
            {
                if (trOn)
                {
                    const float dry = sample;
                    const float lfo = 0.5f * (std::sin((float)tremoloPhase) + 1.0f);
                    const float gain = (1.0f - trDepth) + (trDepth * lfo);
                    const float wet = dry * gain;
                    sample = dry * (1.0f - trM) + wet * trM;
                }
            };

            auto processDelay = [&]()
            {
                float rPos = (float)delayWritePos - currentDelaySamples;
                if (rPos < 0.0f) rPos += (float)delayBufLen;
                const int   di1  = (int)rPos;
                const int   di2  = (di1 + 1) % delayBufLen;
                const float dfrc = rPos - (float)di1;
                float delWet = delayPtr[ch][di1] + dfrc * (delayPtr[ch][di2] - delayPtr[ch][di1]);
                lowPassFilters[3][ch] += 0.25f * (delWet - lowPassFilters[3][ch]);
                delWet = lowPassFilters[3][ch];

                if (delOn)
                {
                    const float dry = sample;
                    sample = dry * (1.0f - delM * 0.5f) + delWet * (delM * 0.7f);
                    delayPtr[ch][delayWritePos] = sample + delWet * delFB;
                }
                else
                {
                    delayPtr[ch][delayWritePos] = sample + delWet * (delFB * 0.3f);
                }
            };

            for (int orderIndex = 0; orderIndex < kPedalCount; ++orderIndex)
            {
                switch (orderedPedals[orderIndex])
                {
                    case SustainGranPedal:   processSustainGran(); break;
                    case SustainChordsPedal: processSustainChords(); break;
                    case FuzzPedal:          processFuzz(); break;
                    case OverdrivePedal:     processOverdrive(); break;
                    case ChorusPedal:        processChorus(); break;
                    case TremoloPedal:       processTremolo(); break;
                    case DelayPedal:         processDelay(); break;
                    default: break;
                }
            }

            // --- SATURACION DE SALIDA ---
            // CORRECCION: antes era std::tanh(sample) SIEMPRE, incluso con todos
            // los efectos apagados. Ahora solo se aplica si hay efectos de ganancia
            // activos que puedan llevar la señal fuera de rango.
            if (fzOn || drOn || chordFreezeOn)
                sample = std::tanh(sample);

            outPtr[ch][s] = sample;
        }

        // --- AVANCE DE POSICIONES (una vez por muestra, fuera del loop de canales) ---
        delayWritePos   = (delayWritePos   + 1) % delayBufLen;
        chorusWritePos  = (chorusWritePos  + 1) % chorusBufLen;
        pitchWritePos   = (pitchWritePos   + 1) % kPitchBufLen;
        freezeWritePos  = (freezeWritePos  + 1) % freezeBufLen;
        chordBodyWritePos = (chordBodyWritePos + 1) % chordBodyBufLen;

        auto& sustainLayer = sustainLayers[0];
        if (sustainLayer.active && sustainLayer.capturedLen > 0)
        {
            const float targetWetLevel = freezeHoldActive ? 1.0f : 0.0f;
            if (sustainLayer.wetLevel < targetWetLevel)
                sustainLayer.wetLevel = juce::jmin(targetWetLevel, sustainLayer.wetLevel + freezeWetRiseStep);
            else if (sustainLayer.wetLevel > targetWetLevel)
                sustainLayer.wetLevel = juce::jmax(targetWetLevel, sustainLayer.wetLevel - freezeWetTailStep);

            sustainLayer.grainPhase += freezeAdvance;
            for (int i = 0; i < EffectsChain::kChordFreezeGrainCount; ++i)
            {
                auto& grain = freezeGrains[(size_t) i];
                if (grain.gain <= 0.0f || grain.length <= 0)
                    continue;

                grain.phase += grain.rate;
                while (grain.phase >= (double) grain.length)
                {
                    grain.phase -= (double) grain.length;
                    grain.start = chooseChordGrainStart(sustainLayer, grain.start);
                    grain.rate = 0.997 + (double) ((i + grain.start) % 7) * 0.001;
                }
            }

            if (!freezeHoldActive && sustainLayer.wetLevel <= 0.0001f)
            {
                sustainLayer.active = false;
                sustainLayer.capturedLen = 0;
                sustainLayer.readPhase = 0.0;
                sustainLayer.grainPhase = 0.0;
                sustainLayer.brightness = 1.0f;
                sustainLayer.smooth[0] = sustainLayer.smooth[1] = 0.0f;
                sustainLayer.toneState[0] = sustainLayer.toneState[1] = 0.0f;
                for (auto& grain : freezeGrains)
                    grain = {};
            }
        }

        if (chordSustainLayer.active && chordSustainLayer.capturedLen > 0)
        {
            const float targetWetLevel = chordFreezeHoldActive ? 1.0f : 0.0f;
            if (chordSustainLayer.wetLevel < targetWetLevel)
                chordSustainLayer.wetLevel = juce::jmin(targetWetLevel, chordSustainLayer.wetLevel + chordFreezeWetRiseStep);
            else if (chordSustainLayer.wetLevel > targetWetLevel)
                chordSustainLayer.wetLevel = juce::jmax(targetWetLevel, chordSustainLayer.wetLevel - chordFreezeWetTailStep);

            chordSustainLayer.grainPhase += freezeAdvance;
            for (int i = 0; i < EffectsChain::kChordFreezeGrainCount; ++i)
            {
                auto& grain = chordFreezeGrains[(size_t) i];
                if (grain.gain <= 0.0f || grain.length <= 0)
                    continue;

                grain.phase += grain.rate;
                while (grain.phase >= (double) grain.length)
                {
                    grain.phase -= (double) grain.length;
                    grain.start = chooseChordGrainStart(chordSustainLayer, grain.start);
                    grain.rate = 0.997 + (double) ((i + grain.start) % 7) * 0.001;
                }
            }

            if (!chordFreezeHoldActive && chordSustainLayer.wetLevel <= 0.0001f)
            {
                chordSustainLayer.active = false;
                chordSustainLayer.capturedLen = 0;
                chordSustainLayer.readPhase = 0.0;
                chordSustainLayer.grainPhase = 0.0;
                chordSustainLayer.brightness = 1.0f;
                chordSustainLayer.smooth[0] = chordSustainLayer.smooth[1] = 0.0f;
                chordSustainLayer.toneState[0] = chordSustainLayer.toneState[1] = 0.0f;
                for (auto& grain : chordFreezeGrains)
                    grain = {};
            }
        }

        if (drumCooldownSamples > 0)
            --drumCooldownSamples;

        drumLastTriggerLevel *= 0.9992f;

        ++drumSampleCounter;

        // Avance de fase del harmonizer.
        pitchPhaseHarm += pitchDeltaHarm;
        while (pitchPhaseHarm >= winSizeF) pitchPhaseHarm -= winSizeF;
        while (pitchPhaseHarm <  0.0)     pitchPhaseHarm += winSizeF;

        // Avance de fase del chorus
        // CORRECCION: deltaChorusPhase pre-calculado fuera del loop
        chorusPhase += deltaChorusPhase;
        if (chorusPhase >= 2.0 * juce::MathConstants<double>::pi)
            chorusPhase -= 2.0 * juce::MathConstants<double>::pi;

        tremoloPhase += deltaTremoloPhase;
        if (tremoloPhase >= 2.0 * juce::MathConstants<double>::pi)
            tremoloPhase -= 2.0 * juce::MathConstants<double>::pi;
    }

    if (revOn && channels > 0)
    {
        juce::Reverb::Parameters params;
        params.roomSize   = juce::jlimit(0.0f, 1.0f, revRoom);
        params.damping    = juce::jlimit(0.0f, 1.0f, revDamp);
        params.wetLevel   = juce::jlimit(0.0f, 0.45f, revM * 0.32f);
        params.dryLevel   = juce::jlimit(0.0f, 1.0f, 1.0f - revM * 0.18f);
        params.width      = 0.85f;
        params.freezeMode = 0.0f;
        roomReverb.setParameters(params);

        if (channels >= 2)
            roomReverb.processStereo(buffer.getWritePointer(0, startSample),
                                     buffer.getWritePointer(1, startSample),
                                     numSamples);
        else
            roomReverb.processMono(buffer.getWritePointer(0, startSample), numSamples);
    }

    if (eqOn && channels > 0)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch, startSample);
            for (int s = 0; s < numSamples; ++s)
                channelData[s] = outputEq.processSample(ch, channelData[s]);
        }
    }
}
