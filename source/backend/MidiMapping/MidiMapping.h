#pragma once

#include "MidiMessageType.h"
#include "MidiControlId.h"
#include "MidiValueConverter.h"

/**
 * MidiMapping
 *
 * Data structures for the MIDI mapping system.
 * Maps incoming MIDI messages (CC and Note On/Off) to UI controls.
 *
 * Design:
 * - All MIDI channels (1-16) trigger mappings (no channel filtering)
 * - CC >= 64 triggers "on" for boolean controls
 * - Note On/Off provides momentary behavior for buttons
 * - MIDI respects current UI shift state for alternate functions
 */
struct MidiMapping {
    MidiMessageType type = MidiMessageType::CC;
    int midiNumber = 0;
    MidiControlId controlId = MidiControlId::COUNT;

    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool invert = false;
};
