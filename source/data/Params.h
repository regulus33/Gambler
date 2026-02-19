//
// Created by Gambler developers on 15.10.2025.
//
    // Comprehensive APVTS parameter definitions for Gambler.
// All sequencer state is backed by APVTS parameters for thread-safe access
// and automatic DAW automation support.
//

#ifndef GAMBLER_GAMBLERPARAMS_H
#define GAMBLER_GAMBLERPARAMS_H

#include "juce_audio_processors/juce_audio_processors.h"
#include <climits>

using namespace std;
using namespace juce;

// Constants for parameter array sizes
namespace ParamConstants {
    constexpr int MAX_STEPS = 64;
    constexpr int MAX_SUBSTEPS = 8;
    constexpr int NUM_MODULATIONS = 7;
    constexpr int NUM_SCALE_NOTES = 12;
    constexpr int DEFAULT_NOTE = 60;  // Middle C
    constexpr int DEFAULT_VELOCITY = 100;
    constexpr int DEFAULT_LENGTH = 8;
}

// This namespace holds keys for the APVTS as well as parameter creation.
// All state is backed by APVTS parameters for thread-safe access.
namespace Params
{
    namespace Keys
    {
        namespace ID
        {
            // ========================================
            // SEQUENCE METADATA
            // ========================================
            inline const String SEQ_LENGTH = "seq_length";
            inline const String SEQ_CURRENT_INDEX = "seq_current_index";
            inline const String SEQ_SELECTED_STEP = "seq_selected_step";
            inline const String SEQ_IS_RANDOM = "seq_is_random";
            inline const String SEQ_IS_SHIFT = "seq_is_shift";
            inline const String SEQ_LAST_PROB_INDEX = "seq_last_prob_index";
            inline const String SEQ_VIEW_WINDOW_START = "seq_view_window_start";

            // Play probability array (64 bools)
            inline String playProbability(int index) {
                return "seq_prob_" + String(index);
            }

            // ========================================
            // STEP NOTE DATA (64 steps × 8 substeps)
            // ========================================
            // Note number for step[stepIndex].substep[substepIndex]
            inline String stepNote(int stepIndex, int substepIndex) {
                return "step_" + String(stepIndex) + "_note_" + String(substepIndex);
            }

            // StepNotes metadata per step
            inline String stepNotesLength(int stepIndex) {
                return "step_" + String(stepIndex) + "_notes_len";
            }
            inline String stepNotesCurrentIndex(int stepIndex) {
                return "step_" + String(stepIndex) + "_notes_idx";
            }

            // ========================================
            // STEP MODULATION DATA (64 steps × 7 modulations)
            // ========================================
            // Modulation type (0=CC, 1=Velocity)
            inline String stepModType(int stepIndex, int modIndex) {
                return "step_" + String(stepIndex) + "_mod_" + String(modIndex) + "_type";
            }
            // CC number (0-127)
            inline String stepModCCNum(int stepIndex, int modIndex) {
                return "step_" + String(stepIndex) + "_mod_" + String(modIndex) + "_cc";
            }
            // Value (0-127)
            inline String stepModValue(int stepIndex, int modIndex) {
                return "step_" + String(stepIndex) + "_mod_" + String(modIndex) + "_val";
            }
            // Selected (bool)
            inline String stepModSelected(int stepIndex, int modIndex) {
                return "step_" + String(stepIndex) + "_mod_" + String(modIndex) + "_sel";
            }

            // ========================================
            // PER-STEP SCALE DATA (64 steps × 12 notes)
            // ========================================
            // Per-step scale notes (each step has its own scale configuration)
            inline String stepScaleNote(int stepIndex, int noteIndex) {
                return "step_" + String(stepIndex) + "_scale_note_" + String(noteIndex);
            }

            // ========================================
            // SCALE WINDOW (Global - applies to all steps)
            // ========================================
            inline const String SCALE_WINDOW_OFFSET = "scale_window_offset";  // 0-127: starting MIDI note
            inline const String SCALE_WINDOW_SIZE = "scale_window_size";      // 1-128: number of notes in window

            // ========================================
            // CLOCK
            // ========================================
            inline const String CLOCK_MODE = "clock_mode";  // 0=INTERNAL, 1=DAW_SYNC, 2=EXTERNAL_MIDI_CLOCK
            inline const String CLOCK_BPM = "clock_bpm";
            inline const String CLOCK_SUBDIVISION = "clock_subdivision";  // 0=QUARTER, 1=EIGHTH, 2=SIXTEENTH, 3=THIRTY_SECOND

            // ========================================
            // MIDI OUTPUT
            // ========================================
            inline const String MIDI_OUTPUT_CHANNEL = "midi_output_channel";  // 1-16

            // ========================================
            // USER PREFERENCES
            // ========================================
            inline const String PREF_PREVIEW_NOTES = "pref_preview_notes";
            inline const String PREF_FOLLOW_STEP = "pref_follow_step";
            inline const String PREF_NOTE_LENGTH = "pref_note_length";

            // ========================================
            // UI STATE (kept for backward compatibility)
            // ========================================
            // Primary row buttons (momentary/trigger)
            inline const String ROLL = "roll";
            inline const String INDEX_LEFT = "index_left";
            inline const String INDEX_RIGHT = "index_right";
            inline const String LENGTH_MINUS = "length_minus";
            inline const String LENGTH_PLUS = "length_plus";
            inline const String NUDGE_LEFT = "nudge_left";
            inline const String NUDGE_RIGHT = "nudge_right";
            // Note: Shift uses SEQ_IS_SHIFT (in Sequence metadata section)

            // Shift mode buttons (replace positions 1-6 when shift is active)
            inline const String DUPLICATE = "duplicate";
            inline const String CLEAR_WINDOW = "clear_window";
            inline const String CLEAR_STEP = "clear_step";
            inline const String RANDOMIZE_STEP = "randomize_step";
            inline const String RANDOMIZE_ALL = "randomize_all";
            inline const String SHUFFLE = "shuffle";

            // Top right buttons
            inline const String EXPORT = "export";
            inline const String SAVE = "save";
            inline const String LOAD = "load";

            // Sub notes buttons
            inline const String SUB_NOTES_PLUS = "sub_notes_plus";
            inline const String SUB_NOTES_MINUS = "sub_notes_minus";

            // Main controls (these now map to the selected step's data)
            inline const String OCTAVE = "octave";
            inline const String NOTE = "note";
            inline const String PROBABILITY = "probability";

            // MIDI mode
            inline const String MIDI_MODE = "midi_mode";

            // Note range (legacy - now use SCALE_RANGE_MIN/MAX)
            inline const String NOTE_RANGE_MIN = "note_range_min";
            inline const String NOTE_RANGE_MAX = "note_range_max";

            // Step buttons (8 buttons for UI window)
            inline String stepButtonSelected(int index) { return "step_button_" + String(index) + "_selected"; }
            inline String stepButtonHighlighted(int index) { return "step_button_" + String(index) + "_highlighted"; }

            // CC controls (7 slots - legacy, now use stepModValue)
            inline String ccNumber(int slot) { return "cc_number_" + String(slot); }
            inline String ccValue(int slot) { return "cc_value_" + String(slot); }

            // Write Scale mode
            inline const String WRITE_SCALE = "write_scale";

            // Scale Latch mode (false = momentary, true = latched)
            inline const String SCALE_LATCH = "scale_latch";

            // Clear Scale trigger
            inline const String CLEAR_SCALE = "clear_scale";

            // ========================================
            // GLOBAL 303 SYNTH PARAMS
            // ========================================
            inline const String SYNTH_TUNING     = "synth_tuning";      // float -12..+12 semitones
            inline const String SYNTH_VOLUME      = "synth_volume";      // float 0..1
            inline const String SYNTH_DIST_DRIVE  = "synth_dist_drive";  // float 0..1
            inline const String SYNTH_DIST_MIX    = "synth_dist_mix";    // float 0..1
            inline const String SYNTH_REVERB_WET  = "synth_reverb_wet";  // float 0..1

            // ========================================
            // NOTE COLOURS (12 customizable note colors)
            // ========================================
            inline String noteColour(int noteIndex) {
                return "note_colour_" + String(noteIndex);
            }
        }

        namespace LABEL
        {
            // Sequence metadata
            inline const String SEQ_LENGTH = "Sequence Length";
            inline const String SEQ_CURRENT_INDEX = "Current Step";
            inline const String SEQ_SELECTED_STEP = "Selected Step";
            inline const String SEQ_IS_RANDOM = "Random Mode";
            inline const String SEQ_IS_SHIFT = "Shift Mode";
            inline const String SEQ_VIEW_WINDOW_START = "View Window Start";

            inline String playProbability(int index) {
                return "Probability " + String(index);
            }

            // Step notes
            inline String stepNote(int stepIndex, int substepIndex) {
                return "Step " + String(stepIndex) + " Note " + String(substepIndex);
            }
            inline String stepNotesLength(int stepIndex) {
                return "Step " + String(stepIndex) + " Substep Count";
            }
            inline String stepNotesCurrentIndex(int stepIndex) {
                return "Step " + String(stepIndex) + " Current Substep";
            }

            // Step modulations
            inline String stepModType(int stepIndex, int modIndex) {
                return "Step " + String(stepIndex) + " Mod " + String(modIndex) + " Type";
            }
            inline String stepModCCNum(int stepIndex, int modIndex) {
                return "Step " + String(stepIndex) + " Mod " + String(modIndex) + " CC#";
            }
            inline String stepModValue(int stepIndex, int modIndex) {
                return "Step " + String(stepIndex) + " Mod " + String(modIndex) + " Value";
            }
            inline String stepModSelected(int stepIndex, int modIndex) {
                return "Step " + String(stepIndex) + " Mod " + String(modIndex) + " Active";
            }

            // Per-step scale
            inline String stepScaleNote(int stepIndex, int noteIndex) {
                static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
                return "Step " + String(stepIndex) + " Scale " + noteNames[noteIndex];
            }
            inline const String SCALE_WINDOW_OFFSET = "Scale Window Offset";
            inline const String SCALE_WINDOW_SIZE = "Scale Window Size";

            // Clock
            inline const String CLOCK_MODE = "Clock Mode";
            inline const String CLOCK_BPM = "BPM";
            inline const String CLOCK_SUBDIVISION = "Subdivision";

            // MIDI Output
            inline const String MIDI_OUTPUT_CHANNEL = "MIDI Output Channel";

            // User preferences
            inline const String PREF_PREVIEW_NOTES = "Preview Notes";
            inline const String PREF_FOLLOW_STEP = "Follow Step";
            inline const String PREF_NOTE_LENGTH = "Note Length";

            // Write Scale mode
            inline const String WRITE_SCALE = "Write Scale";

            // Scale Latch mode
            inline const String SCALE_LATCH = "Scale Latch";

            // Clear Scale trigger
            inline const String CLEAR_SCALE = "Clear Scale";

            // Global 303 synth params
            inline const String SYNTH_TUNING     = "Tuning";
            inline const String SYNTH_VOLUME      = "Volume";
            inline const String SYNTH_DIST_DRIVE  = "Drive";
            inline const String SYNTH_DIST_MIX    = "Dist Mix";
            inline const String SYNTH_REVERB_WET  = "Reverb Wet";

            // Note colours
            inline String noteColour(int noteIndex) {
                static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
                return "Note Colour " + String(noteNames[noteIndex]);
            }

            // Legacy UI labels - Primary row
            inline const String ROLL = "Roll";
            inline const String INDEX_LEFT = "Index Left";
            inline const String INDEX_RIGHT = "Index Right";
            inline const String LENGTH_MINUS = "Length -";
            inline const String LENGTH_PLUS = "Length +";
            inline const String NUDGE_LEFT = "Nudge Left";
            inline const String NUDGE_RIGHT = "Nudge Right";
            // Note: Shift uses SEQ_IS_SHIFT label

            // Shift mode button labels
            inline const String DUPLICATE = "Duplicate";
            inline const String CLEAR_WINDOW = "Clear Window";
            inline const String CLEAR_STEP = "Clear Step";
            inline const String RANDOMIZE_STEP = "Randomize Step";
            inline const String RANDOMIZE_ALL = "Randomize All";
            inline const String SHUFFLE = "Shuffle";

            inline const String EXPORT = "Export";
            inline const String SAVE = "Save";
            inline const String LOAD = "Load";
            inline const String SUB_NOTES_PLUS = "Sub Notes +";
            inline const String SUB_NOTES_MINUS = "Sub Notes -";
            inline const String OCTAVE = "Octave";
            inline const String NOTE = "Note";
            inline const String PROBABILITY = "Probability";
            inline const String MIDI_MODE = "MIDI Mode";
            inline const String NOTE_RANGE_MIN = "Note Range Min";
            inline const String NOTE_RANGE_MAX = "Note Range Max";
            inline String stepButtonSelected(int index) { return "Step " + String(index) + " Selected"; }
            inline String stepButtonHighlighted(int index) { return "Step " + String(index) + " Highlighted"; }
            inline String ccNumber(int slot) { return "CC " + String(slot) + " Number"; }
            inline String ccValue(int slot) { return "CC " + String(slot) + " Value"; }
        }
    };

    /**
     * Create the complete parameter layout for all sequencer state.
     * This creates ~2500 parameters to back all domain state.
     */
    static AudioProcessorValueTreeState::ParameterLayout createDefaultParameterLayout()
    {
        vector<unique_ptr<RangedAudioParameter>> params;

        // Helper lambdas
        auto createBoolParam = [](const String& id, const String& label, bool defaultValue = false) {
            return make_unique<AudioParameterBool>(id, label, defaultValue);
        };

        auto createIntParam = [](const String& id, const String& label, int min, int max, int defaultValue) {
            return make_unique<AudioParameterInt>(id, label, min, max, defaultValue);
        };

        auto createFloatParam = [](const String& id, const String& label, float min, float max, float defaultValue) {
            return make_unique<AudioParameterFloat>(id, label, NormalisableRange<float>(min, max), defaultValue);
        };

        // ========================================
        // SEQUENCE METADATA
        // ========================================
        params.push_back(createIntParam(Keys::ID::SEQ_LENGTH, Keys::LABEL::SEQ_LENGTH,
            1, ParamConstants::MAX_STEPS, ParamConstants::DEFAULT_LENGTH));
        params.push_back(createIntParam(Keys::ID::SEQ_CURRENT_INDEX, Keys::LABEL::SEQ_CURRENT_INDEX,
            -1, ParamConstants::MAX_STEPS - 1, -1));
        params.push_back(createIntParam(Keys::ID::SEQ_SELECTED_STEP, Keys::LABEL::SEQ_SELECTED_STEP,
            0, ParamConstants::MAX_STEPS - 1, 0));
        params.push_back(createBoolParam(Keys::ID::SEQ_IS_RANDOM, Keys::LABEL::SEQ_IS_RANDOM, false));
        params.push_back(createBoolParam(Keys::ID::SEQ_IS_SHIFT, Keys::LABEL::SEQ_IS_SHIFT, false));
        params.push_back(createIntParam(Keys::ID::SEQ_LAST_PROB_INDEX, "Last Prob Index", -1, 63, -1));
        params.push_back(createIntParam(Keys::ID::SEQ_VIEW_WINDOW_START, Keys::LABEL::SEQ_VIEW_WINDOW_START, 0, 56, 0));

        // Play probability array (64 bools, default all true = 100% probability)
        for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
            params.push_back(createBoolParam(Keys::ID::playProbability(i),
                Keys::LABEL::playProbability(i), true));
        }

        // ========================================
        // STEP NOTE DATA (64 steps × 8 substeps)
        // ========================================
        for (int step = 0; step < ParamConstants::MAX_STEPS; ++step) {
            // Note values for each substep (-1 to 127, -1 = rest)
            for (int substep = 0; substep < ParamConstants::MAX_SUBSTEPS; ++substep) {
                params.push_back(createIntParam(
                    Keys::ID::stepNote(step, substep),
                    Keys::LABEL::stepNote(step, substep),
                    -1, 127, ParamConstants::DEFAULT_NOTE));
            }

            // StepNotes metadata
            params.push_back(createIntParam(
                Keys::ID::stepNotesLength(step),
                Keys::LABEL::stepNotesLength(step),
                1, ParamConstants::MAX_SUBSTEPS, 1));
            params.push_back(createIntParam(
                Keys::ID::stepNotesCurrentIndex(step),
                Keys::LABEL::stepNotesCurrentIndex(step),
                -1, ParamConstants::MAX_SUBSTEPS - 1, -1));
        }

        // ========================================
        // STEP MODULATION DATA (64 steps × 7 modulations)
        // ========================================
        for (int step = 0; step < ParamConstants::MAX_STEPS; ++step) {
            for (int mod = 0; mod < ParamConstants::NUM_MODULATIONS; ++mod) {
                // Type: 0=CC, 1=Velocity
                int defaultType = (mod == 0) ? 1 : 0;  // First slot is velocity
                params.push_back(createIntParam(
                    Keys::ID::stepModType(step, mod),
                    Keys::LABEL::stepModType(step, mod),
                    0, 1, defaultType));

                // CC number (0-127)
                params.push_back(createIntParam(
                    Keys::ID::stepModCCNum(step, mod),
                    Keys::LABEL::stepModCCNum(step, mod),
                    0, 127, mod));  // Default CC numbers 0-6

                // Value (0-127)
                int defaultValue = (mod == 0) ? ParamConstants::DEFAULT_VELOCITY : 64;
                params.push_back(createIntParam(
                    Keys::ID::stepModValue(step, mod),
                    Keys::LABEL::stepModValue(step, mod),
                    0, 127, defaultValue));

                // Selected (bool)
                params.push_back(createBoolParam(
                    Keys::ID::stepModSelected(step, mod),
                    Keys::LABEL::stepModSelected(step, mod),
                    false));
            }
        }

        // ========================================
        // PER-STEP SCALE DATA (64 steps × 12 notes)
        // ========================================
        for (int step = 0; step < ParamConstants::MAX_STEPS; ++step) {
            for (int note = 0; note < ParamConstants::NUM_SCALE_NOTES; ++note) {
                params.push_back(createBoolParam(
                    Keys::ID::stepScaleNote(step, note),
                    Keys::LABEL::stepScaleNote(step, note),
                    true));  // All notes enabled by default (chromatic)
            }
        }

        // ========================================
        // SCALE WINDOW (Global - applies to all steps)
        // ========================================
        params.push_back(createIntParam(Keys::ID::SCALE_WINDOW_OFFSET, Keys::LABEL::SCALE_WINDOW_OFFSET, 0, 127, 0));
        params.push_back(createIntParam(Keys::ID::SCALE_WINDOW_SIZE, Keys::LABEL::SCALE_WINDOW_SIZE, 1, 128, 128));

        // ========================================
        // CLOCK
        // ========================================
        params.push_back(createIntParam(Keys::ID::CLOCK_MODE, Keys::LABEL::CLOCK_MODE, 0, 2, 0));  // 0=INTERNAL, 1=DAW_SYNC, 2=EXTERNAL_MIDI_CLOCK
        params.push_back(createFloatParam(Keys::ID::CLOCK_BPM, Keys::LABEL::CLOCK_BPM, 20.0f, 999.0f, 120.0f));
        params.push_back(createIntParam(Keys::ID::CLOCK_SUBDIVISION, Keys::LABEL::CLOCK_SUBDIVISION, 0, 3, 2));  // 2=SIXTEENTH

        // ========================================
        // MIDI OUTPUT
        // ========================================
        params.push_back(createIntParam(Keys::ID::MIDI_OUTPUT_CHANNEL, Keys::LABEL::MIDI_OUTPUT_CHANNEL, 1, 16, 1));

        // ========================================
        // USER PREFERENCES
        // ========================================
        params.push_back(createBoolParam(Keys::ID::PREF_PREVIEW_NOTES, Keys::LABEL::PREF_PREVIEW_NOTES, true));
        params.push_back(createBoolParam(Keys::ID::PREF_FOLLOW_STEP, Keys::LABEL::PREF_FOLLOW_STEP, true));
        params.push_back(createFloatParam(Keys::ID::PREF_NOTE_LENGTH, Keys::LABEL::PREF_NOTE_LENGTH, 0.1f, 2.0f, 0.9f));

        // ========================================
        // LEGACY UI STATE (for backward compatibility)
        // ========================================
        // Primary row buttons
        params.push_back(createBoolParam(Keys::ID::ROLL, Keys::LABEL::ROLL));
        params.push_back(createBoolParam(Keys::ID::INDEX_LEFT, Keys::LABEL::INDEX_LEFT));
        params.push_back(createBoolParam(Keys::ID::INDEX_RIGHT, Keys::LABEL::INDEX_RIGHT));
        params.push_back(createBoolParam(Keys::ID::LENGTH_MINUS, Keys::LABEL::LENGTH_MINUS));
        params.push_back(createBoolParam(Keys::ID::LENGTH_PLUS, Keys::LABEL::LENGTH_PLUS));
        params.push_back(createBoolParam(Keys::ID::NUDGE_LEFT, Keys::LABEL::NUDGE_LEFT));
        params.push_back(createBoolParam(Keys::ID::NUDGE_RIGHT, Keys::LABEL::NUDGE_RIGHT));
        // Note: Shift uses SEQ_IS_SHIFT param (already created in sequence metadata section)

        // Shift mode buttons (replace positions 1-6 when shift is active)
        params.push_back(createBoolParam(Keys::ID::DUPLICATE, Keys::LABEL::DUPLICATE));
        params.push_back(createBoolParam(Keys::ID::CLEAR_WINDOW, Keys::LABEL::CLEAR_WINDOW));
        params.push_back(createBoolParam(Keys::ID::CLEAR_STEP, Keys::LABEL::CLEAR_STEP));
        params.push_back(createBoolParam(Keys::ID::RANDOMIZE_STEP, Keys::LABEL::RANDOMIZE_STEP));
        params.push_back(createBoolParam(Keys::ID::RANDOMIZE_ALL, Keys::LABEL::RANDOMIZE_ALL));
        params.push_back(createBoolParam(Keys::ID::SHUFFLE, Keys::LABEL::SHUFFLE));

        params.push_back(createBoolParam(Keys::ID::EXPORT, Keys::LABEL::EXPORT));
        params.push_back(createBoolParam(Keys::ID::SAVE, Keys::LABEL::SAVE));
        params.push_back(createBoolParam(Keys::ID::LOAD, Keys::LABEL::LOAD));
        params.push_back(createBoolParam(Keys::ID::SUB_NOTES_PLUS, Keys::LABEL::SUB_NOTES_PLUS));
        params.push_back(createBoolParam(Keys::ID::SUB_NOTES_MINUS, Keys::LABEL::SUB_NOTES_MINUS));
        params.push_back(createIntParam(Keys::ID::OCTAVE, Keys::LABEL::OCTAVE, -4, 4, 0));
        params.push_back(createIntParam(Keys::ID::NOTE, Keys::LABEL::NOTE, 0, 127, 60));
        params.push_back(createIntParam(Keys::ID::PROBABILITY, Keys::LABEL::PROBABILITY, 0, 100, 100));
        params.push_back(createIntParam(Keys::ID::MIDI_MODE, Keys::LABEL::MIDI_MODE, 0, 1, 0));
        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MIN, Keys::LABEL::NOTE_RANGE_MIN, 0, 127, 0));
        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MAX, Keys::LABEL::NOTE_RANGE_MAX, 0, 127, 127));

        for (int i = 0; i < 8; ++i) {
            params.push_back(createBoolParam(Keys::ID::stepButtonSelected(i), Keys::LABEL::stepButtonSelected(i)));
            params.push_back(createBoolParam(Keys::ID::stepButtonHighlighted(i), Keys::LABEL::stepButtonHighlighted(i)));
        }

        for (int i = 0; i < 7; ++i) {
            params.push_back(createIntParam(Keys::ID::ccNumber(i), Keys::LABEL::ccNumber(i), 0, 127, i + 1));
            params.push_back(createIntParam(Keys::ID::ccValue(i), Keys::LABEL::ccValue(i), 0, 127, 64));
        }

        // ========================================
        // NOTE COLOURS (12 customizable note colors, stored as ARGB int)
        // ========================================
        {
            const int defaultNoteColours[12] = {
                static_cast<int>(0xFFFF6B6B), // C   - Red
                static_cast<int>(0xFFFF9F6B), // Db  - Orange
                static_cast<int>(0xFFFFD66B), // D   - Yellow
                static_cast<int>(0xFFFFE66B), // Eb  - Light yellow
                static_cast<int>(0xFFE6FF6B), // E   - Yellow-green
                static_cast<int>(0xFF6BFFB4), // F   - Light green
                static_cast<int>(0xFF6BFFD6), // Gb  - Cyan-green
                static_cast<int>(0xFF6BD6FF), // G   - Cyan
                static_cast<int>(0xFF6BB4FF), // Ab  - Light blue
                static_cast<int>(0xFF9F6BFF), // A   - Purple
                static_cast<int>(0xFFD66BFF), // Bb  - Magenta
                static_cast<int>(0xFFFF6BD6)  // B   - Pink
            };
            for (int i = 0; i < 12; ++i) {
                params.push_back(make_unique<AudioParameterInt>(
                    Keys::ID::noteColour(i),
                    Keys::LABEL::noteColour(i),
                    INT_MIN, INT_MAX,
                    defaultNoteColours[i]));
            }
        }

        // ========================================
        // GLOBAL 303 SYNTH PARAMS
        // ========================================
        params.push_back(createFloatParam(Keys::ID::SYNTH_TUNING,    Keys::LABEL::SYNTH_TUNING,    -12.f, 12.f, 0.f));
        params.push_back(createFloatParam(Keys::ID::SYNTH_VOLUME,     Keys::LABEL::SYNTH_VOLUME,     0.f, 1.f, 0.75f));
        params.push_back(createFloatParam(Keys::ID::SYNTH_DIST_DRIVE, Keys::LABEL::SYNTH_DIST_DRIVE, 0.f, 1.f, 0.f));
        params.push_back(createFloatParam(Keys::ID::SYNTH_DIST_MIX,   Keys::LABEL::SYNTH_DIST_MIX,   0.f, 1.f, 0.f));
        params.push_back(createFloatParam(Keys::ID::SYNTH_REVERB_WET, Keys::LABEL::SYNTH_REVERB_WET, 0.f, 1.f, 0.f));

        // Write Scale mode
        params.push_back(createBoolParam(Keys::ID::WRITE_SCALE, Keys::LABEL::WRITE_SCALE, false));

        // Scale Latch mode (false = momentary, true = latched)
        params.push_back(createBoolParam(Keys::ID::SCALE_LATCH, Keys::LABEL::SCALE_LATCH, false));

        // Clear Scale trigger
        params.push_back(createBoolParam(Keys::ID::CLEAR_SCALE, Keys::LABEL::CLEAR_SCALE, false));

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
