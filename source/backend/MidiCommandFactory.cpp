#include "MidiCommandFactory.h"
#include <juce_core/juce_core.h>

ProcessorCommand MidiCommandFactory::createButtonCommand(
    MidiControlId controlId,
    bool isPressed,
    Sequence& sequence,
    UIUpdateCallback uiCallback)
{
    switch (controlId) {
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

        case MidiControlId::BUTTON_LENGTH_PLUS_SHIFT_CLEAR_STEP:
            // Length + (or Clear Step when shifted)
            // For now, just implement length increase
            return ProcessorCommand(
                [&sequence]() {
                    sequence.increaseLength();
                    DBG("MIDI Length+ - new length: " << sequence.getLength());
                },
                uiCallback
            );

        case MidiControlId::BUTTON_LENGTH_MINUS_SHIFT_RANDOMIZE_STEP:
            // Length - (or Randomize Step when shifted)
            // For now, just implement length decrease
            return ProcessorCommand(
                [&sequence]() {
                    sequence.decreaseLength();
                    DBG("MIDI Length- - new length: " << sequence.getLength());
                },
                uiCallback
            );

        case MidiControlId::BUTTON_NUDGE_LEFT_SHIFT_RANDOMIZE_ALL:
            // Nudge left (or Randomize All when shifted)
            return ProcessorCommand(
                [&sequence]() {
                    sequence.nudgeLeft();
                    DBG("MIDI Nudge left");
                },
                uiCallback
            );

        case MidiControlId::BUTTON_NUDGE_RIGHT_SHIFT_SHUFFLE:
            // Nudge right (or Shuffle when shifted)
            return ProcessorCommand(
                [&sequence]() {
                    sequence.nudgeRight();
                    DBG("MIDI Nudge right");
                },
                uiCallback
            );

        case MidiControlId::BUTTON_PAGE_LEFT_SHIFT_DUPLICATE_SEQUENCE:
        case MidiControlId::BUTTON_PAGE_RIGHT_SHIFT_CLEAR_WINDOW:
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
