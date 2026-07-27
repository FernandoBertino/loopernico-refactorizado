#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <functional>

// ==============================================================================
// MidiController — MIDI learn, mappings y dispatch.
//
// Desacoplado completamente de la UI. MainComponent registra callbacks
// para cada accion, y MidiController los llama cuando llega el mensaje
// MIDI correspondiente.
//
// Uso:
//   1. Registrar callbacks: setActionCallback(index, func)
//   2. Pasar mensajes entrantes: handleMessage(msg)
//   3. Para iniciar/detener el learn: startLearn(actionIndex), stopLearn()
// ==============================================================================
class MidiController
{
public:
    using ActionStateCallback = std::function<void(bool pressed, bool hasRelease)>;

    struct MappingData
    {
        bool isMapped{ false };
        bool isProgramChange{ false };
        bool isNote{ false };
        int number{ -1 };
    };

    enum Action
    {
        RecConfirm = 0, Cancel, Undo, Redo, StopPlay, MuteAll, ResetAll,
        PauseAll, RhythmNextSection,
        NoiseGateToggle, RhythmBoxToggle, CompressorToggle, HarmonizerToggle,
        SustainChordsToggle, FuzzToggle, OverdriveToggle, ChorusToggle,
        TremoloToggle, TapeDelayToggle, RoomReverbToggle, OutputEqToggle,
        Vst3Toggle,
        NumActions
    };
    static constexpr int kNumActions = (int) NumActions;

    MidiController();

    // Registrar una funcion que se llama cuando se dispara la accion.
    // El callback se llama SIEMPRE desde el message thread via callAsync.
    void setActionCallback(int actionIndex, std::function<void()> callback);
    void setActionStateCallback(int actionIndex, ActionStateCallback callback);

    // Procesar un mensaje MIDI entrante (llamar desde handleIncomingMidiMessage)
    // Retorna true si el mensaje fue consumido (era un learn o un mapping conocido)
    bool handleMessage(const juce::MidiMessage& message);

    // Iniciar modo learn para la accion indicada
    void startLearn(int actionIndex);

    // Activar modo de espera sin accion definida todavia.
    // El usuario debe presionar un boton de accion para definir cual mapear.
    void activateWaitMode();

    // Cancelar modo learn sin guardar
    void stopLearn();

    bool isLearning() const     { return learnMode.load(); }
    int  learningAction() const { return actionBeingLearned.load(); }
    MappingData getMapping(int actionIndex) const;
    void setMapping(int actionIndex, const MappingData& mapping);
    void clearMappings();

    // Nombre de la accion para mostrar en la UI
    static const char* actionName(int index);

private:
    struct Mapping {
        std::atomic<bool> isMapped        { false };
        std::atomic<bool> isProgramChange { false };
        std::atomic<bool> isNote          { false };
        std::atomic<int>  number          { -1    };
    };

    Mapping   mappings[kNumActions];
    std::function<void()> callbacks[kNumActions];
    ActionStateCallback stateCallbacks[kNumActions];

    std::atomic<bool> learnMode          { false };
    std::atomic<int>  actionBeingLearned { -1    };
    std::atomic<int>  lastProgramNumber  { -1    };
    std::atomic<juce::int64> lastProgramTimeMs { 0 };
};
