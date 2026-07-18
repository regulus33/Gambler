#pragma once

#include "MidiMapping/MidiControlId.h"
#include "MidiMapping/MidiMessageType.h"
#include "MidiLearnRegistry/MidiLearnRegistry.h"

/**
 * MidiNoteRouter
 *
 * Routes incoming MIDI note messages to the correct control.
 *
 * Logic:
 *   1. Check if the note has a user-created mapping (via MidiLearnRegistry)
 *   2. If yes → return that mapped control ID
 *   3. If no  → return the corresponding piano-roll scale key
 *              (note % 12 maps to SCALE_NOTE_C .. SCALE_NOTE_B)
 *
 * This guarantees that every MIDI note always triggers *something* —
 * either a user override or the default scale key.
 *
 * NOTE: callers must gate on MidiLearnRegistry::isLearning() BEFORE routing.
 * While learn mode is active no control may fire, otherwise the very notes
 * being mapped would toggle scale keys as a side effect.
 */
class MidiNoteRouter {
public:
    /**
     * Resolve which MidiControlId a MIDI note should trigger.
     *
     * @param registry  The MIDI learn registry (holds user mappings)
     * @param channel   MIDI channel (1-16)
     * @param noteNumber MIDI note number (0-127)
     * @return The control to trigger — either user mapping or default scale key
     */
    static MidiControlId resolve(const MidiLearnRegistry& registry,
                                 int channel,
                                 int noteNumber) {
        // Check for explicit user mapping first
        MidiControlId userMapping = registry.findUserMapping(
            MidiMessageType::NOTE, channel, noteNumber);

        if (userMapping != MidiControlId::COUNT)
            return userMapping;

        // Default: map to the corresponding scale key (note % 12)
        return defaultScaleKey(noteNumber);
    }

    /**
     * Check whether a note has an explicit user mapping (not the default).
     */
    static bool hasUserMapping(const MidiLearnRegistry& registry,
                               int channel,
                               int noteNumber) {
        return registry.findUserMapping(MidiMessageType::NOTE, channel, noteNumber)
               != MidiControlId::COUNT;
    }

    /**
     * Get the default scale key for any MIDI note number.
     * note % 12 → SCALE_NOTE_C .. SCALE_NOTE_B
     */
    static MidiControlId defaultScaleKey(int noteNumber) {
        int pitchClass = noteNumber % 12;
        return static_cast<MidiControlId>(
            static_cast<int>(MidiControlId::SCALE_NOTE_C) + pitchClass);
    }
};
