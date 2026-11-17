#pragma once

#include <string>

/**
 * Unique identifier for each controllable UI element
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
    OCTAVE_SLIDER,
    NOTE_SLIDER,

    // ========== PROBABILITY CONTROLS ==========
    PROBABILITY_SLIDER,

    // ========== SEQUENCER STEPS (8 visible steps in window) ==========
    STEP_0, STEP_1, STEP_2, STEP_3, STEP_4, STEP_5, STEP_6, STEP_7,

    // ========== SEQUENCER CONTROLS (8 buttons) ==========
    // Position 0: BUTTON_ROLL (always Roll)
    // Position 1: BUTTON_PAGE_LEFT (Page Left / Duplicate when shift active)
    // Position 2: BUTTON_PAGE_RIGHT (Page Right / Clear Window when shift active)
    // Position 3: BUTTON_LENGTH_PLUS (Length+ / Clear Step when shift active)
    // Position 4: BUTTON_LENGTH_MINUS (Length- / Randomize All when shift active)
    // Position 5: BUTTON_NUDGE_LEFT (Nudge Left / Randomize Step when shift active)
    // Position 6: BUTTON_NUDGE_RIGHT (Nudge Right / Shuffle when shift active)
    // Position 7: BUTTON_SHIFT (always Shift)
    BUTTON_ROLL,
    BUTTON_PAGE_LEFT,
    BUTTON_PAGE_RIGHT,
    BUTTON_LENGTH_PLUS,
    BUTTON_LENGTH_MINUS,
    BUTTON_NUDGE_LEFT,
    BUTTON_NUDGE_RIGHT,
    BUTTON_SHIFT,

    // ========== SUBSEQUENCE CONTROLS (10 controls) ==========
    SUBSEQ_BAR_0, SUBSEQ_BAR_1, SUBSEQ_BAR_2, SUBSEQ_BAR_3,
    SUBSEQ_BAR_4, SUBSEQ_BAR_5, SUBSEQ_BAR_6, SUBSEQ_BAR_7,
    SUBSEQ_MINUS,
    SUBSEQ_PLUS,

    // ========== NOTE RANGE CONTROLS (2 sliders) ==========
    NOTE_RANGE_MIN,
    NOTE_RANGE_MAX,

    // ========== CLOCK CONTROLS ==========
    SUBDIVISION_SLIDER,

    // ========== NAVIGATION ==========
    SETTINGS_TOGGLE,

    // ========== PRESET CONTROLS ==========
    PRESET_CYCLE_UP,
    PRESET_CYCLE_DOWN,

    // ========== TRANSPORT CONTROLS ==========
    PLAY_PAUSE,
    STOP,

    // ========== SCALE CONTROLS ==========
    SCALE_LATCH,

    // ========== DESTRUCTIVE TOGGLES ==========
    DESTRUCTIVE_SCALE,
    DESTRUCTIVE_RANGE,

    // ========== FLAT ACTION BUTTONS (web UI: no shift layer) ==========
    // Appended after the original ids: mappings are saved by number, so the order above must not change.
    BUTTON_DUPLICATE,
    BUTTON_SHUFFLE,
    BUTTON_RANDOMIZE,
    BUTTON_CLEAR,
    BUTTON_RANDOMIZE_ALL,
    BUTTON_CLEAR_PAGE,

    // ========== Total count ==========
    COUNT
};

/**
 * Get human-readable name for a control ID (for debugging)
 */
inline std::string getMidiControlName(MidiControlId id) {
    switch (id) {
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

        case MidiControlId::OCTAVE_SLIDER:      return "Octave Slider";
        case MidiControlId::NOTE_SLIDER:        return "Note Slider";
        case MidiControlId::PROBABILITY_SLIDER: return "Probability Slider";

        case MidiControlId::STEP_0: return "Step 0";
        case MidiControlId::STEP_1: return "Step 1";
        case MidiControlId::STEP_2: return "Step 2";
        case MidiControlId::STEP_3: return "Step 3";
        case MidiControlId::STEP_4: return "Step 4";
        case MidiControlId::STEP_5: return "Step 5";
        case MidiControlId::STEP_6: return "Step 6";
        case MidiControlId::STEP_7: return "Step 7";

        case MidiControlId::BUTTON_ROLL:           return "Roll Button";
        case MidiControlId::BUTTON_PAGE_LEFT:      return "Page Left / Duplicate";
        case MidiControlId::BUTTON_PAGE_RIGHT:     return "Page Right / Clear Window";
        case MidiControlId::BUTTON_LENGTH_PLUS:    return "Length + / Clear Step";
        case MidiControlId::BUTTON_LENGTH_MINUS:   return "Length - / Randomize All";
        case MidiControlId::BUTTON_NUDGE_LEFT:     return "Nudge Left / Randomize Step";
        case MidiControlId::BUTTON_NUDGE_RIGHT:    return "Nudge Right / Shuffle";
        case MidiControlId::BUTTON_SHIFT:          return "Shift Button";

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

        case MidiControlId::NOTE_RANGE_MIN: return "Note Range Offset";
        case MidiControlId::NOTE_RANGE_MAX: return "Note Range Size";

        case MidiControlId::SUBDIVISION_SLIDER: return "Subdivision";

        case MidiControlId::SETTINGS_TOGGLE: return "Settings Toggle";

        case MidiControlId::PRESET_CYCLE_UP: return "Preset Cycle Up";
        case MidiControlId::PRESET_CYCLE_DOWN: return "Preset Cycle Down";

        case MidiControlId::PLAY_PAUSE: return "Play/Pause";
        case MidiControlId::STOP: return "Stop";

        case MidiControlId::SCALE_LATCH: return "Scale Latch";

        case MidiControlId::DESTRUCTIVE_SCALE: return "Bake Scale";
        case MidiControlId::DESTRUCTIVE_RANGE: return "Bake Range";

        case MidiControlId::BUTTON_DUPLICATE: return "Duplicate";
        case MidiControlId::BUTTON_SHUFFLE:   return "Shuffle";
        case MidiControlId::BUTTON_RANDOMIZE: return "Randomize Step";
        case MidiControlId::BUTTON_CLEAR:     return "Clear Step";
        case MidiControlId::BUTTON_RANDOMIZE_ALL: return "Randomize All";
        case MidiControlId::BUTTON_CLEAR_PAGE:    return "Clear Page";

        default: return "Unknown Control";
    }
}
