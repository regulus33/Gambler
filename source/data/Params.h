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
            // GLOBAL SCALE DATA (12 notes)
            // ========================================
            // Global scale notes (single scale applies to all steps)
            inline String globalScaleNote(int noteIndex) {
                return "global_scale_note_" + String(noteIndex);
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
            inline const String PREF_DESTRUCTIVE_SCALE = "pref_destructive_scale";
            inline const String PREF_DESTRUCTIVE_RANGE = "pref_destructive_range";

            // ========================================
            // SEQUENCER CONTROL BUTTONS (MIDI-mappable)
            // ========================================
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

            // Note range
            inline const String NOTE_RANGE_MIN = "note_range_min";
            inline const String NOTE_RANGE_MAX = "note_range_max";

            // Scale Latch mode (false = momentary, true = latched)
            inline const String SCALE_LATCH = "scale_latch";


            // Preset cycling triggers (MIDI-mappable)
            inline const String PRESET_CYCLE_UP = "preset_cycle_up";
            inline const String PRESET_CYCLE_DOWN = "preset_cycle_down";

            // Transport controls (MIDI-mappable)
            inline const String PLAY_PAUSE = "play_pause";
            inline const String STOP = "stop";

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

            // Global scale
            inline String globalScaleNote(int noteIndex) {
                static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
                return "Global Scale " + String(noteNames[noteIndex]);
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
            inline const String PREF_DESTRUCTIVE_SCALE = "Destructive Scale";
            inline const String PREF_DESTRUCTIVE_RANGE = "Destructive Range";

            // Scale Latch mode
            inline const String SCALE_LATCH = "Scale Latch";


            // Preset cycling triggers
            inline const String PRESET_CYCLE_UP = "Preset Cycle Up";
            inline const String PRESET_CYCLE_DOWN = "Preset Cycle Down";

            // Transport controls
            inline const String PLAY_PAUSE = "Play/Pause";
            inline const String STOP = "Stop";

            // Note colours
            inline String noteColour(int noteIndex) {
                static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
                return "Note Colour " + String(noteNames[noteIndex]);
            }

            // Sequencer control button labels
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

            inline const String NOTE_RANGE_MIN = "Note Range Min";
            inline const String NOTE_RANGE_MAX = "Note Range Max";
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
        // SEQUENCE METADATA (MIDI-mappable params only)
        // ========================================
        params.push_back(createIntParam(Keys::ID::SEQ_LENGTH, Keys::LABEL::SEQ_LENGTH,
            1, ParamConstants::MAX_STEPS, ParamConstants::DEFAULT_LENGTH));
        // NOTE: SEQ_CURRENT_INDEX is now in InternalState (not MIDI-mappable, changes too frequently)
        params.push_back(createIntParam(Keys::ID::SEQ_SELECTED_STEP, Keys::LABEL::SEQ_SELECTED_STEP,
            0, ParamConstants::MAX_STEPS - 1, 0));
        params.push_back(createBoolParam(Keys::ID::SEQ_IS_RANDOM, Keys::LABEL::SEQ_IS_RANDOM, false));
        params.push_back(createBoolParam(Keys::ID::SEQ_IS_SHIFT, Keys::LABEL::SEQ_IS_SHIFT, false));
        // NOTE: seq_last_prob_index, seq_view_window_start are now in InternalState
        // NOTE: Play probability array (64 bools) is now in InternalState
        // NOTE: Step note data (64 steps × 8 substeps) is now in InternalState
        // NOTE: Step modulation data (64 steps × 7 modulations) is now in InternalState
        // NOTE: Note colours (12 values) are now in InternalState

        // ========================================
        // GLOBAL SCALE DATA (12 notes)
        // ========================================
        for (int note = 0; note < ParamConstants::NUM_SCALE_NOTES; ++note) {
            params.push_back(createBoolParam(
                Keys::ID::globalScaleNote(note),
                Keys::LABEL::globalScaleNote(note),
                true));  // All notes enabled by default (chromatic)
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
        params.push_back(createBoolParam(Keys::ID::PREF_FOLLOW_STEP, Keys::LABEL::PREF_FOLLOW_STEP, false));
        params.push_back(createFloatParam(Keys::ID::PREF_NOTE_LENGTH, Keys::LABEL::PREF_NOTE_LENGTH, 0.1f, 2.0f, 0.9f));
        params.push_back(createBoolParam(Keys::ID::PREF_DESTRUCTIVE_SCALE, Keys::LABEL::PREF_DESTRUCTIVE_SCALE, false));
        params.push_back(createBoolParam(Keys::ID::PREF_DESTRUCTIVE_RANGE, Keys::LABEL::PREF_DESTRUCTIVE_RANGE, false));

        // ========================================
        // SEQUENCER CONTROL BUTTONS (MIDI-mappable)
        // ========================================
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

        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MIN, Keys::LABEL::NOTE_RANGE_MIN, 0, 127, 0));
        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MAX, Keys::LABEL::NOTE_RANGE_MAX, 0, 127, 127));

        // NOTE: Note colours (12 customizable note colors) are now in InternalState

        // Scale Latch mode (false = momentary, true = latched)
        params.push_back(createBoolParam(Keys::ID::SCALE_LATCH, Keys::LABEL::SCALE_LATCH, false));


        // Preset cycling triggers (MIDI-mappable)
        params.push_back(createBoolParam(Keys::ID::PRESET_CYCLE_UP, Keys::LABEL::PRESET_CYCLE_UP, false));
        params.push_back(createBoolParam(Keys::ID::PRESET_CYCLE_DOWN, Keys::LABEL::PRESET_CYCLE_DOWN, false));

        // Transport controls (MIDI-mappable)
        params.push_back(createBoolParam(Keys::ID::PLAY_PAUSE, Keys::LABEL::PLAY_PAUSE, false));
        params.push_back(createBoolParam(Keys::ID::STOP, Keys::LABEL::STOP, false));

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
