#include "MidiController.h"

MidiController::MidiController()
{
    for (int i = 0; i < kNumActions; ++i)
        callbacks[i] = nullptr;
}

void MidiController::setActionCallback(int idx, std::function<void()> cb)
{
    if (idx >= 0 && idx < kNumActions)
        callbacks[idx] = std::move(cb);
}

void MidiController::setActionStateCallback(int idx, ActionStateCallback cb)
{
    if (idx >= 0 && idx < kNumActions)
        stateCallbacks[idx] = std::move(cb);
}

void MidiController::startLearn(int actionIndex)
{
    if (actionIndex < 0 || actionIndex >= kNumActions) return;
    actionBeingLearned.store(actionIndex);
    learnMode.store(true);
}

void MidiController::activateWaitMode()
{
    // Pone el modo learn activo pero sin accion definida todavia.
    // isLearning() devuelve true, learningAction() devuelve -1.
    // El usuario debe presionar un boton de accion para definir cual mapear.
    actionBeingLearned.store(-1);
    learnMode.store(true);
}

void MidiController::stopLearn()
{
    learnMode.store(false);
    actionBeingLearned.store(-1);
}

MidiController::MappingData MidiController::getMapping(int actionIndex) const
{
    if (actionIndex < 0 || actionIndex >= kNumActions)
        return {};

    MappingData mapping;
    mapping.isMapped = mappings[actionIndex].isMapped.load();
    mapping.isProgramChange = mappings[actionIndex].isProgramChange.load();
    mapping.isNote = mappings[actionIndex].isNote.load();
    mapping.number = mappings[actionIndex].number.load();
    return mapping;
}

void MidiController::setMapping(int actionIndex, const MappingData& mapping)
{
    if (actionIndex < 0 || actionIndex >= kNumActions)
        return;

    // Un mensaje solo puede pertenecer a una accion. Reasignarlo elimina el
    // mapping anterior y evita que el orden interno decida que boton responde.
    if (mapping.isMapped && mapping.number >= 0)
    {
        for (int i = 0; i < kNumActions; ++i)
        {
            if (i == actionIndex || !mappings[i].isMapped.load()
                || mappings[i].number.load() != mapping.number)
                continue;

            const bool sameType = mappings[i].isProgramChange.load() == mapping.isProgramChange
                               && mappings[i].isNote.load() == mapping.isNote;
            if (sameType)
            {
                mappings[i].isMapped.store(false);
                mappings[i].isProgramChange.store(false);
                mappings[i].isNote.store(false);
                mappings[i].number.store(-1);
            }
        }
    }

    mappings[actionIndex].isMapped.store(mapping.isMapped);
    mappings[actionIndex].isProgramChange.store(mapping.isProgramChange);
    mappings[actionIndex].isNote.store(mapping.isNote);
    mappings[actionIndex].number.store(mapping.number);
}

void MidiController::clearMappings()
{
    for (int i = 0; i < kNumActions; ++i)
        setMapping(i, {});
}

bool MidiController::handleMessage(const juce::MidiMessage& msg)
{
    const bool isProgram = msg.isProgramChange();
    const bool isController = msg.isController();
    const bool isNoteOn = msg.isNoteOn();
    const bool isNoteOff = msg.isNoteOff();

    // Solo procesar Program Change, CC y notas del controlador.
    if (!isProgram && !isController && !isNoteOn && !isNoteOff) return false;

    int num = isProgram ? msg.getProgramChangeNumber()
            : isController ? msg.getControllerNumber()
                                   : msg.getNoteNumber();

    // Algunos foot controllers envian el mismo Program Change al presionar y
    // al soltar. Para acciones toggle eso producia ON seguido inmediatamente
    // de OFF y parecia que el pedal nunca respondia.
    if (isProgram)
    {
        constexpr juce::int64 kProgramChangeDebounceMs = 180;
        const auto now = juce::Time::currentTimeMillis();
        const auto previousTime = lastProgramTimeMs.load();
        const auto previousNumber = lastProgramNumber.load();
        lastProgramNumber.store(num);
        lastProgramTimeMs.store(now);

        if (previousNumber == num && now >= previousTime
            && now - previousTime < kProgramChangeDebounceMs)
            return true;
    }

    // --- Modo learn activo ---
    if (learnMode.load())
    {
        // Aprender solo desde eventos de "press" para evitar guardar el release.
        if ((isController && msg.getControllerValue() == 0) || isNoteOff)
            return false;

        int action = actionBeingLearned.load();
        if (action >= 0 && action < kNumActions)
        {
            MappingData learnedMapping;
            learnedMapping.isMapped = true;
            learnedMapping.isProgramChange = isProgram;
            learnedMapping.isNote = isNoteOn;
            learnedMapping.number = num;
            setMapping(action, learnedMapping);
            learnMode.store(false);
            actionBeingLearned.store(-1);
            return true;
        }
    }

    // --- Dispatch normal ---
    for (int i = 0; i < kNumActions; ++i)
    {
        if (!mappings[i].isMapped.load()) continue;
        if (mappings[i].number.load() != num) continue;

        bool typeMatch =
            (mappings[i].isProgramChange.load() && isProgram) ||
            (mappings[i].isNote.load()           && (isNoteOn || isNoteOff)) ||
            (!mappings[i].isProgramChange.load() && !mappings[i].isNote.load() && isController);

        if (!typeMatch)
            continue;

        // Program Change no tiene evento de release: se trata como una accion
        // toggle normal. CC y notas conservan press/release para un sustain
        // momentaneo.
        if (isProgram && callbacks[i])
        {
            auto cb = callbacks[i];
            juce::MessageManager::callAsync([cb]() { cb(); });
            return true;
        }

        if (stateCallbacks[i])
        {
            const bool pressed = isProgram
                ? true
                : isController
                    ? msg.getControllerValue() != 0
                    : isNoteOn;
            const bool hasRelease = isController || mappings[i].isNote.load();

            auto cb = stateCallbacks[i];
            juce::MessageManager::callAsync([cb, pressed, hasRelease]() { cb(pressed, hasRelease); });
            return true;
        }

        if (callbacks[i] && (isProgram || isNoteOn || (isController && msg.getControllerValue() != 0)))
        {
            auto cb = callbacks[i]; // copiar para capturar en lambda
            juce::MessageManager::callAsync([cb]() { cb(); });
            return true;
        }
    }

    return false;
}

const char* MidiController::actionName(int index)
{
    static const char* names[kNumActions] = {
        "REC/CONFIRM", "CANCEL", "UNDO", "REDO", "IR ARRIBA", "MUTE ALL",
        "RESET ALL", "PAUSA/RESUME", "RHYTHM NEXT", "SUSTAIN GRAN",
        "PEDAL NOISE GATE", "PEDAL RHYTHM BOX", "PEDAL COMPRESSOR",
        "PEDAL HARMONIZER", "PEDAL SUSTAIN CHORDS", "PEDAL FUZZ",
        "PEDAL SUNSET", "PEDAL CHORUS", "PEDAL TREMOLO",
        "PEDAL TAPE DELAY", "PEDAL ROOM REVERB", "PEDAL OUTPUT EQ",
        "PEDAL VST3"
    };
    if (index < 0 || index >= kNumActions) return "???";
    return names[index];
}
