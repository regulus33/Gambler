//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_GAMBLERPARAMS_H
#define GAMBLER_GAMBLERPARAMS_H

#include "juce_audio_processors/juce_audio_processors.h"

using namespace std;
using namespace juce;


// This namespace holds keys for the APVTS as well as parameter creation.
// All Controls properties are mapped to APVTS parameters for persistence.
namespace Params
{
    namespace Keys
    {
        namespace ID
        {
            // Bottom row buttons
            inline const String ROLL = "roll";
            inline const String INDEX_LEFT = "index_left";
            inline const String INDEX_RIGHT = "index_right";
            inline const String LENGTH_MINUS = "length_minus";
            inline const String LENGTH_PLUS = "length_plus";
            inline const String NUDGE_LEFT = "nudge_left";
            inline const String NUDGE_RIGHT = "nudge_right";
            inline const String SHIFT = "shift";

            // Top right buttons
            inline const String EXPORT = "export";
            inline const String SAVE = "save";
            inline const String LOAD = "load";

            // Sub notes buttons
            inline const String SUB_NOTES_PLUS = "sub_notes_plus";
            inline const String SUB_NOTES_MINUS = "sub_notes_minus";

            // Main controls
            inline const String OCTAVE = "octave";
            inline const String NOTE = "note";
            inline const String PROBABILITY = "probability";

            // MIDI mode
            inline const String MIDI_MODE = "midi_mode";

            // Clock mode (true = internal clock, false = DAW sync)
            inline const String USE_INTERNAL_CLOCK = "use_internal_clock";

            // Note range
            inline const String NOTE_RANGE_MIN = "note_range_min";
            inline const String NOTE_RANGE_MAX = "note_range_max";

            // Step buttons (8 buttons)
            inline String stepButtonSelected(int index) { return "step_button_" + String(index) + "_selected"; }
            inline String stepButtonHighlighted(int index) { return "step_button_" + String(index) + "_highlighted"; }

            // CC controls (7 slots)
            inline String ccNumber(int slot) { return "cc_number_" + String(slot); }
            inline String ccValue(int slot) { return "cc_value_" + String(slot); }
        }

        namespace LABEL
        {
            // Bottom row buttons
            inline const String ROLL = "Roll";
            inline const String INDEX_LEFT = "Index Left";
            inline const String INDEX_RIGHT = "Index Right";
            inline const String LENGTH_MINUS = "Length -";
            inline const String LENGTH_PLUS = "Length +";
            inline const String NUDGE_LEFT = "Nudge Left";
            inline const String NUDGE_RIGHT = "Nudge Right";
            inline const String SHIFT = "Shift";

            // Top right buttons
            inline const String EXPORT = "Export";
            inline const String SAVE = "Save";
            inline const String LOAD = "Load";

            // Sub notes buttons
            inline const String SUB_NOTES_PLUS = "Sub Notes +";
            inline const String SUB_NOTES_MINUS = "Sub Notes -";

            // Main controls
            inline const String OCTAVE = "Octave";
            inline const String NOTE = "Note";
            inline const String PROBABILITY = "Probability";

            // MIDI mode
            inline const String MIDI_MODE = "MIDI Mode";

            // Clock mode
            inline const String USE_INTERNAL_CLOCK = "Use Internal Clock";

            // Note range
            inline const String NOTE_RANGE_MIN = "Note Range Min";
            inline const String NOTE_RANGE_MAX = "Note Range Max";

            // Step buttons
            inline String stepButtonSelected(int index) { return "Step " + String(index) + " Selected"; }
            inline String stepButtonHighlighted(int index) { return "Step " + String(index) + " Highlighted"; }

            // CC controls
            inline String ccNumber(int slot) { return "CC " + String(slot) + " Number"; }
            inline String ccValue(int slot) { return "CC " + String(slot) + " Value"; }
        }
    };

    static AudioProcessorValueTreeState::ParameterLayout createDefaultParameterLayout()
    {
        vector<unique_ptr<RangedAudioParameter>> params;

        // Helper lambda to create bool parameters
        auto createBoolParam = [](const String& id, const String& label, bool defaultValue = false) {
            return make_unique<AudioParameterBool>(id, label, defaultValue);
        };

        // Helper lambda to create int parameters
        auto createIntParam = [](const String& id, const String& label, int min, int max, int defaultValue) {
            return make_unique<AudioParameterInt>(id, label, min, max, defaultValue);
        };

        // Bottom row buttons
        params.push_back(createBoolParam(Keys::ID::ROLL, Keys::LABEL::ROLL));
        params.push_back(createBoolParam(Keys::ID::INDEX_LEFT, Keys::LABEL::INDEX_LEFT));
        params.push_back(createBoolParam(Keys::ID::INDEX_RIGHT, Keys::LABEL::INDEX_RIGHT));
        params.push_back(createBoolParam(Keys::ID::LENGTH_MINUS, Keys::LABEL::LENGTH_MINUS));
        params.push_back(createBoolParam(Keys::ID::LENGTH_PLUS, Keys::LABEL::LENGTH_PLUS));
        params.push_back(createBoolParam(Keys::ID::NUDGE_LEFT, Keys::LABEL::NUDGE_LEFT));
        params.push_back(createBoolParam(Keys::ID::NUDGE_RIGHT, Keys::LABEL::NUDGE_RIGHT));
        params.push_back(createBoolParam(Keys::ID::SHIFT, Keys::LABEL::SHIFT));

        // Top right buttons
        params.push_back(createBoolParam(Keys::ID::EXPORT, Keys::LABEL::EXPORT));
        params.push_back(createBoolParam(Keys::ID::SAVE, Keys::LABEL::SAVE));
        params.push_back(createBoolParam(Keys::ID::LOAD, Keys::LABEL::LOAD));

        // Sub notes buttons
        params.push_back(createBoolParam(Keys::ID::SUB_NOTES_PLUS, Keys::LABEL::SUB_NOTES_PLUS));
        params.push_back(createBoolParam(Keys::ID::SUB_NOTES_MINUS, Keys::LABEL::SUB_NOTES_MINUS));

        // Main controls
        params.push_back(createIntParam(Keys::ID::OCTAVE, Keys::LABEL::OCTAVE, -4, 4, 0));
        params.push_back(createIntParam(Keys::ID::NOTE, Keys::LABEL::NOTE, 0, 127, 60));
        params.push_back(createIntParam(Keys::ID::PROBABILITY, Keys::LABEL::PROBABILITY, 0, 100, 100));

        // MIDI mode (0 = CC_TO_NOTE, 1 = NOTE_TO_CC)
        params.push_back(createIntParam(Keys::ID::MIDI_MODE, Keys::LABEL::MIDI_MODE, 0, 1, 0));

        // Clock mode (true = internal clock, false = DAW sync)
        // Default to internal clock for standalone, DAW sync for plugin
#if JucePlugin_Build_Standalone
        params.push_back(createBoolParam(Keys::ID::USE_INTERNAL_CLOCK, Keys::LABEL::USE_INTERNAL_CLOCK, true));
#else
        params.push_back(createBoolParam(Keys::ID::USE_INTERNAL_CLOCK, Keys::LABEL::USE_INTERNAL_CLOCK, false));
#endif

        // Note range
        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MIN, Keys::LABEL::NOTE_RANGE_MIN, 0, 127, 0));
        params.push_back(createIntParam(Keys::ID::NOTE_RANGE_MAX, Keys::LABEL::NOTE_RANGE_MAX, 0, 127, 127));

        // Step buttons (8 buttons, each with selected and highlighted state)
        for (int i = 0; i < 8; ++i) {
            params.push_back(createBoolParam(Keys::ID::stepButtonSelected(i), Keys::LABEL::stepButtonSelected(i)));
            params.push_back(createBoolParam(Keys::ID::stepButtonHighlighted(i), Keys::LABEL::stepButtonHighlighted(i)));
        }

        // CC controls (7 slots, each with number and value)
        for (int i = 0; i < 7; ++i) {
            params.push_back(createIntParam(Keys::ID::ccNumber(i), Keys::LABEL::ccNumber(i), 0, 127, i + 1));
            params.push_back(createIntParam(Keys::ID::ccValue(i), Keys::LABEL::ccValue(i), 0, 127, 64));
        }

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
