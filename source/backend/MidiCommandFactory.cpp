#include "MidiCommandFactory.h"
#include <juce_core/juce_core.h>

ProcessorCommand MidiCommandFactory::createButtonCommand(
    MidiControlId controlId,
    bool isPressed,
    Sequence& sequence,
    UIUpdateCallback uiCallback)
{
    switch (controlId) {
        // ========== PRIMARY ROW BUTTONS ==========
        case MidiControlId::BUTTON_ROLL:
            // Roll button - momentary: enables random mode while pressed
            return ProcessorCommand(
                [&sequence, isPressed]() {
                    sequence.setIsRandom(isPressed);
                    DBG("MIDI Roll " << (isPressed ? "pressed" : "released") << " - random mode " << (isPressed ? "ON" : "OFF"));
                },
                uiCallback
            );

        case MidiControlId::BUTTON_SHIFT:
            // Shift button - toggle mode (we track state in UI component)
            // For MIDI, we'll just trigger on press, not release
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    []() {
                        DBG("MIDI Shift button pressed");
                        // State is managed by UI component
                    },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_LENGTH_PLUS:
            return ProcessorCommand(
                [&sequence]() {
                    sequence.increaseLength();
                    DBG("MIDI Length+ - new length: " << sequence.getLength());
                },
                uiCallback
            );

        case MidiControlId::BUTTON_LENGTH_MINUS:
            return ProcessorCommand(
                [&sequence]() {
                    sequence.decreaseLength();
                    DBG("MIDI Length- - new length: " << sequence.getLength());
                },
                uiCallback
            );

        case MidiControlId::BUTTON_NUDGE_LEFT:
            return ProcessorCommand(
                [&sequence]() {
                    sequence.nudgeLeft();
                    DBG("MIDI Nudge left");
                },
                uiCallback
            );

        case MidiControlId::BUTTON_NUDGE_RIGHT:
            return ProcessorCommand(
                [&sequence]() {
                    sequence.nudgeRight();
                    DBG("MIDI Nudge right");
                },
                uiCallback
            );

        case MidiControlId::BUTTON_PAGE_LEFT:
        case MidiControlId::BUTTON_PAGE_RIGHT:
            // Page left/right - these are UI-only (change window view)
            // We'll handle these via UI callback only
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    [controlId]() {
                        DBG("MIDI Page button - " << getMidiControlName(controlId));
                    },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        // ========== SECONDARY ROW BUTTONS ==========
        case MidiControlId::BUTTON_DUPLICATE:
            // Duplicate sequence - handled via UI callback (needs access to state)
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    []() { DBG("MIDI Duplicate button pressed"); },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_CLEAR_WINDOW:
            // Clear current window - handled via UI callback
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    []() { DBG("MIDI Clear Window button pressed"); },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_CLEAR_STEP:
            // Clear current step - handled via UI callback
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    []() { DBG("MIDI Clear Step button pressed"); },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_RANDOMIZE_STEP:
            // Randomize current step
            if (isPressed && uiCallback) {
                return ProcessorCommand(
                    []() { DBG("MIDI Randomize Step button pressed"); },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_RANDOMIZE_ALL:
            // Randomize all steps
            if (isPressed) {
                return ProcessorCommand(
                    [&sequence]() {
                        sequence.randomizeAllNotes();
                        DBG("MIDI Randomize All");
                    },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        case MidiControlId::BUTTON_SHUFFLE:
            // Shuffle step order
            if (isPressed) {
                return ProcessorCommand(
                    [&sequence]() {
                        sequence.shuffle();
                        DBG("MIDI Shuffle");
                    },
                    uiCallback
                );
            }
            return createNoOpCommand(controlId);

        default:
            return createNoOpCommand(controlId);
    }
}

ProcessorCommand MidiCommandFactory::createRangedCommand(
    MidiControlId controlId,
    float normalizedValue,
    Sequence& sequence,
    UIUpdateCallback uiCallback)
{
    // TODO: Implement ranged controls (sliders, etc.)
    // For now, return no-op
    return ProcessorCommand(
        [controlId, normalizedValue]() {
            DBG("MIDI Ranged control: " << getMidiControlName(controlId) << " = " << normalizedValue);
        },
        uiCallback
    );
}

ProcessorCommand MidiCommandFactory::createNoOpCommand(MidiControlId controlId)
{
    return ProcessorCommand(
        [controlId]() {
            DBG("MIDI control not implemented: " << getMidiControlName(controlId));
        }
    );
}
