#pragma once

/**
 * ClockMode - Determines the timing source for the Clock
 * 
 * DAW_SYNC: Synchronizes with the host DAW's tempo and playhead position
 * INTERNAL: Uses an internal timer based on a user-specified BPM
 */
enum class ClockMode {
    DAW_SYNC,
    INTERNAL,
    EXTERNAL_MIDI_CLOCK
};

/**
 * Subdivision - Note subdivision for clock pulses
 * 
 * Determines how frequently the sequencer advances:
 * - QUARTER: One tick per beat (1/4 note)
 * - EIGHTH: Two ticks per beat (1/8 note)
 * - SIXTEENTH: Four ticks per beat (1/16 note)
 * - THIRTY_SECOND: Eight ticks per beat (1/32 note)
 */
enum class Subdivision {
    QUARTER,        // 1/4 note
    EIGHTH,         // 1/8 note
    SIXTEENTH,      // 1/16 note
    THIRTY_SECOND   // 1/32 note
};

/**
 * Get the number of pulses per quarter note for a given subdivision
 * 
 * @param subdivision The subdivision to convert
 * @return Number of pulses per quarter note
 */
inline int getPulsesPerQuarterNote(Subdivision subdivision) {
    switch (subdivision) {
        case Subdivision::QUARTER:        return 1;
        case Subdivision::EIGHTH:         return 2;
        case Subdivision::SIXTEENTH:      return 4;
        case Subdivision::THIRTY_SECOND:  return 8;
        default:                          return 4;  // Default to 16th notes
    }
}

