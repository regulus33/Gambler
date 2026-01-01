#pragma once

#include "ProcessorCommand.h"
#include "MidiMapping.h"
#include "Sequence.h"
#include <functional>

/**
 * MidiCommandFactory
 *
 * Creates ProcessorCommands for MIDI input that match the commands used by UI components.
 * This ensures MIDI and UI follow the same code path for state changes, allowing
 * completion callbacks to handle all UI updates automatically.
 *
 * The factory takes callbacks for UI updates that will be executed on the message thread
 * after the domain state changes complete on the audio thread.
 */
class MidiCommandFactory {
public:
    /**
     * Callback type for UI updates that should run on message thread
     * These are the completion callbacks that update the UI
     */
    using UIUpdateCallback = std::function<void()>;

    /**
     * Create a command for a button press (momentary or toggle)
     * @param controlId The MIDI control ID for the button
     * @param isPressed True for button down, false for button up (relevant for momentary buttons)
     * @param sequence Reference to the sequence (from PluginProcessor)
     * @param uiCallback Completion callback for UI updates
     * @return ProcessorCommand that can be queued
     */
    static ProcessorCommand createButtonCommand(
        MidiControlId controlId,
        bool isPressed,
        Sequence& sequence,
        UIUpdateCallback uiCallback = nullptr);

    /**
     * Create a command for a ranged control (slider, knob, etc.)
     * @param controlId The MIDI control ID
     * @param normalizedValue Value from 0.0 to 1.0
     * @param sequence Reference to the sequence
     * @param uiCallback Completion callback for UI updates
     * @return ProcessorCommand that can be queued
     */
    static ProcessorCommand createRangedCommand(
        MidiControlId controlId,
        float normalizedValue,
        Sequence& sequence,
        UIUpdateCallback uiCallback = nullptr);

private:
    // Helper to create no-op command for unimplemented controls
    static ProcessorCommand createNoOpCommand(MidiControlId controlId);
};
