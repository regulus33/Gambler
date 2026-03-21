#pragma once

#include <string>

/**
 * MidiMapping.h
 *
 * Data structures for MIDI mapping system.
 * Maps incoming MIDI messages (CC and Note On/Off) to UI controls.
 *
 * Design:
 * - All MIDI channels (1-16) trigger mappings (no channel filtering)
 * - CC >= 64 triggers "on" for boolean controls
 * - Note On/Off provides momentary behavior for buttons
 * - MIDI respects current UI shift state for alternate functions
 */

/**
 * Type of MIDI message
 */
enum class MidiMessageType {
    CC,     // Control Change (0-127)
    NOTE    // Note On/Off (0-127)
};

/**
 * Unique identifier for each controllable UI element
 * Total: ~150+ controls across the application
 */
enum class MidiControlId {
    // ========== PIANO ROLL (12 toggles) ==========
    SCALE_NOTE_C,
    SCALE_NOTE_Db,
    SCALE_NOTE_D,
    SCALE_NOTE_Eb,
    SCALE_NOTE_E,
    SCALE_NOTE_F,
    SCALE_NOTE_Gb,
    SCALE_NOTE_G,
    SCALE_NOTE_Ab,
    SCALE_NOTE_A,
    SCALE_NOTE_Bb,
    SCALE_NOTE_B,

    // ========== NOTE CONTROLS (2 sliders) ==========
    OCTAVE_SLIDER,      // 0-9 (octaves 0-9)
    NOTE_SLIDER,        // 0-11 (pitch class, filtered to scale notes)

    // ========== PROBABILITY CONTROLS ==========
    PROBABILITY_SLIDER, // 0.0-1.0 (0%-100%)

    // ========== SEQUENCER STEPS (8 visible steps in window) ==========
    STEP_0, STEP_1, STEP_2, STEP_3, STEP_4, STEP_5, STEP_6, STEP_7,

    // ========== SEQUENCER CONTROLS (8 buttons) ==========
    // Note: Buttons at positions 1-6 are shift-aware and trigger different actions
    // based on the shift state. MIDI mappings are shared between primary and shift functions.
    //
    // Position 0: BUTTON_ROLL (always Roll)
    // Position 1: BUTTON_PAGE_LEFT (Page Left / Duplicate when shift active)
    // Position 2: BUTTON_PAGE_RIGHT (Page Right / Clear Window when shift active)
    // Position 3: BUTTON_LENGTH_PLUS (Length+ / Clear Step when shift active)
    // Position 4: BUTTON_LENGTH_MINUS (Length- / Randomize All when shift active)
    // Position 5: BUTTON_NUDGE_LEFT (Nudge Left / Randomize Step when shift active)
    // Position 6: BUTTON_NUDGE_RIGHT (Nudge Right / Shuffle when shift active)
    // Position 7: BUTTON_SHIFT (always Shift)
    BUTTON_ROLL,           // Position 0: Momentary: enable random mode
    BUTTON_PAGE_LEFT,      // Position 1: Scroll view window left / Duplicate (shift)
    BUTTON_PAGE_RIGHT,     // Position 2: Scroll view window right / Clear Window (shift)
    BUTTON_LENGTH_PLUS,    // Position 3: Increase sequence length / Clear Step (shift)
    BUTTON_LENGTH_MINUS,   // Position 4: Decrease sequence length / Randomize All (shift)
    BUTTON_NUDGE_LEFT,     // Position 5: Shift all notes left / Randomize Step (shift)
    BUTTON_NUDGE_RIGHT,    // Position 6: Shift all notes right / Shuffle (shift)
    BUTTON_SHIFT,          // Position 7: Toggle to show/hide secondary row

    // ========== SUBSEQUENCE CONTROLS (10 controls) ==========
    SUBSEQ_BAR_0, SUBSEQ_BAR_1, SUBSEQ_BAR_2, SUBSEQ_BAR_3,
    SUBSEQ_BAR_4, SUBSEQ_BAR_5, SUBSEQ_BAR_6, SUBSEQ_BAR_7,
    SUBSEQ_MINUS,  // Decrease substep length
    SUBSEQ_PLUS,   // Increase substep length

    // ========== NOTE RANGE CONTROLS (2 sliders) ==========
    NOTE_RANGE_MIN,  // 0-127 (window offset)
    NOTE_RANGE_MAX,  // 1-128 (window size)

    // ========== CLOCK CONTROLS ==========
    SUBDIVISION_SLIDER,  // 0-3 (quarter, eighth, sixteenth, thirty-second)

    // ========== NAVIGATION ==========
    SETTINGS_TOGGLE,  // Toggle settings screen

    // ========== PRESET CONTROLS ==========
    PRESET_CYCLE_UP,    // Cycle to next preset
    PRESET_CYCLE_DOWN,  // Cycle to previous preset

    // ========== TRANSPORT CONTROLS ==========
    PLAY_PAUSE,  // Toggle play/pause
    STOP,        // Stop sequencer

    // ========== SCALE CONTROLS ==========
    SCALE_LATCH, // Toggle scale latch mode

    // ========== Total count ==========
    COUNT
};

/**
 * Mapping from MIDI message to UI control
 * All MIDI channels (1-16) accepted - no channel filtering
 */
struct MidiMapping {
    MidiMessageType type;      // CC or NOTE
    int midiNumber;            // CC number (0-127) or Note number (0-127)
    MidiControlId controlId;   // Which control to trigger

    // Value transformation (for ranged controls like sliders)
    float minValue = 0.0f;     // Min normalized value (0.0-1.0)
    float maxValue = 1.0f;     // Max normalized value (0.0-1.0)
    bool invert = false;       // Invert MIDI value (127 becomes 0, 0 becomes 127)
};

/**
 * Utility class for converting MIDI values to control values
 */
class MidiValueConverter {
public:
    /**
     * Convert MIDI CC value (0-127) to normalized value (0.0-1.0)
     */
    static float midiToNormalized(int midiValue, bool invert = false) {
        if (invert) {
            return (127 - midiValue) / 127.0f;
        }
        return midiValue / 127.0f;
    }

    /**
     * Convert MIDI value to scaled value with min/max range
     */
    static float midiToScaled(int midiValue, float minValue, float maxValue, bool invert = false) {
        float normalized = midiToNormalized(midiValue, invert);
        return minValue + (normalized * (maxValue - minValue));
    }

    /**
     * Convert MIDI CC value to boolean (threshold at 64)
     * CC >= 64 = true, CC < 64 = false
     */
    static bool midiToBoolean(int midiValue) {
        return midiValue >= 64;
    }

    /**
     * Convert MIDI value to discrete integer index
     * Example: MIDI 0-127 → octave index 0-9
     */
    static int midiToDiscreteIndex(int midiValue, int maxIndex) {
        float normalized = midiValue / 127.0f;
        int index = static_cast<int>(normalized * maxIndex);
        // Clamp to ensure we don't exceed maxIndex
        if (index > maxIndex) index = maxIndex;
        return index;
    }
};

/**
 * Get human-readable name for a control ID (for debugging)
 */
inline std::string getMidiControlName(MidiControlId id) {
    switch (id) {
        // Piano Roll
        case MidiControlId::SCALE_NOTE_C:   return "Scale Note C";
        case MidiControlId::SCALE_NOTE_Db:  return "Scale Note Db";
        case MidiControlId::SCALE_NOTE_D:   return "Scale Note D";
        case MidiControlId::SCALE_NOTE_Eb:  return "Scale Note Eb";
        case MidiControlId::SCALE_NOTE_E:   return "Scale Note E";
        case MidiControlId::SCALE_NOTE_F:   return "Scale Note F";
        case MidiControlId::SCALE_NOTE_Gb:  return "Scale Note Gb";
        case MidiControlId::SCALE_NOTE_G:   return "Scale Note G";
        case MidiControlId::SCALE_NOTE_Ab:  return "Scale Note Ab";
        case MidiControlId::SCALE_NOTE_A:   return "Scale Note A";
        case MidiControlId::SCALE_NOTE_Bb:  return "Scale Note Bb";
        case MidiControlId::SCALE_NOTE_B:   return "Scale Note B";

        // Note Controls
        case MidiControlId::OCTAVE_SLIDER:      return "Octave Slider";
        case MidiControlId::NOTE_SLIDER:        return "Note Slider";
        case MidiControlId::PROBABILITY_SLIDER: return "Probability Slider";

        // Sequencer Steps
        case MidiControlId::STEP_0: return "Step 0";
        case MidiControlId::STEP_1: return "Step 1";
        case MidiControlId::STEP_2: return "Step 2";
        case MidiControlId::STEP_3: return "Step 3";
        case MidiControlId::STEP_4: return "Step 4";
        case MidiControlId::STEP_5: return "Step 5";
        case MidiControlId::STEP_6: return "Step 6";
        case MidiControlId::STEP_7: return "Step 7";

        // Sequencer Controls (shift-aware buttons share MIDI mappings)
        case MidiControlId::BUTTON_ROLL:           return "Roll Button";
        case MidiControlId::BUTTON_PAGE_LEFT:      return "Page Left / Duplicate";
        case MidiControlId::BUTTON_PAGE_RIGHT:     return "Page Right / Clear Window";
        case MidiControlId::BUTTON_LENGTH_PLUS:    return "Length + / Clear Step";
        case MidiControlId::BUTTON_LENGTH_MINUS:   return "Length - / Randomize All";
        case MidiControlId::BUTTON_NUDGE_LEFT:     return "Nudge Left / Randomize Step";
        case MidiControlId::BUTTON_NUDGE_RIGHT:    return "Nudge Right / Shuffle";
        case MidiControlId::BUTTON_SHIFT:          return "Shift Button";

        // Subsequence Controls
        case MidiControlId::SUBSEQ_BAR_0: return "Subseq Bar 0";
        case MidiControlId::SUBSEQ_BAR_1: return "Subseq Bar 1";
        case MidiControlId::SUBSEQ_BAR_2: return "Subseq Bar 2";
        case MidiControlId::SUBSEQ_BAR_3: return "Subseq Bar 3";
        case MidiControlId::SUBSEQ_BAR_4: return "Subseq Bar 4";
        case MidiControlId::SUBSEQ_BAR_5: return "Subseq Bar 5";
        case MidiControlId::SUBSEQ_BAR_6: return "Subseq Bar 6";
        case MidiControlId::SUBSEQ_BAR_7: return "Subseq Bar 7";
        case MidiControlId::SUBSEQ_MINUS:  return "Subseq Minus Button";
        case MidiControlId::SUBSEQ_PLUS:   return "Subseq Plus Button";

        // Note Range
        case MidiControlId::NOTE_RANGE_MIN: return "Note Range Offset";
        case MidiControlId::NOTE_RANGE_MAX: return "Note Range Size";

        // Clock Controls
        case MidiControlId::SUBDIVISION_SLIDER: return "Subdivision";

        // Navigation
        case MidiControlId::SETTINGS_TOGGLE: return "Settings Toggle";

        // Preset Controls
        case MidiControlId::PRESET_CYCLE_UP: return "Preset Cycle Up";
        case MidiControlId::PRESET_CYCLE_DOWN: return "Preset Cycle Down";

        // Transport Controls
        case MidiControlId::PLAY_PAUSE: return "Play/Pause";
        case MidiControlId::STOP: return "Stop";

        case MidiControlId::SCALE_LATCH: return "Scale Latch";

        default: return "Unknown Control";
    }
}
