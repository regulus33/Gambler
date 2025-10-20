//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_GAMBLERPARAMS_H
#define GAMBLER_GAMBLERPARAMS_H

#include "juce_audio_processors/juce_audio_processors.h"

using namespace std;
using namespace juce;


// this namespace holds keys for the apvts as well as values. Some are consts, some are functions that build the key dynamically.
namespace Params
{
    namespace Defaults
    {
        static constexpr auto STEP_INDEX = 0;
    }
    namespace Keys
    {
        namespace ID
        {
            static String STEP_VELOCITY (int stepNumber)
            {

                return "STEP_" + String(stepNumber) + "_VELOCITY";
            }

            static String STEP_GATE (int stepNumber)
            {

                return "STEP_" + String(stepNumber) + "_GATE";
            }

            static String STEP_NOTE (int stepNumber)
            {

                return "STEP_" + String(stepNumber) + "_NOTE";
            }

            static String ACTIVE_PARAM (int stepNumber)
            {

                return "STEP_" + String(stepNumber) + "_ACTIVE";
            }

            static constexpr const char* STEP_LENGTH = "STEP_LENGTH";
            static constexpr const char* STEP_INDEX = "STEP_INDEX";
        }

        namespace LABEL
        {
            static String STEP_GATE (int stepNumber)
            {

                return "Step " + String(stepNumber) + " Gate";
            }

            static String STEP_VELOCITY (int stepNumber)
            {

                return "Step " + String(stepNumber) + " Velocity";
            }
            // Names
            static String STEP_ACTIVE (int stepNumber)
            {

                return "Step " + String(stepNumber) + " Active";
            }

            static String STEP_NOTE (int stepNumber)
            {

                return "Step " + String(stepNumber) + " Note";
            }

            static constexpr const char* STEP_LENGTH = "Step Length";
            static constexpr const char* CURRENT_STEP = "Current Step";
        }

    };

    static AudioProcessorValueTreeState::ParameterLayout createDefaultParameterLayout()
    {
        vector<unique_ptr<RangedAudioParameter>> params;

        // Global sequencer parameters
        params.push_back (std::make_unique<AudioParameterInt> (
            Params::Keys::ID::STEP_LENGTH, Params::Keys::LABEL::STEP_LENGTH, 1, 128, 16));

        params.push_back (std::make_unique<AudioParameterInt> (
            Keys::ID::STEP_INDEX, Keys::LABEL::CURRENT_STEP, 0, 127, Params::Defaults::STEP_INDEX));

        // Create the 128 apvts step bound params
        for (int i = 0; i < 128; ++i)
        {
            // Step active/trigger
            params.push_back (std::make_unique<AudioParameterBool> (
                Params::Keys::ID::ACTIVE_PARAM (i),
                Params::Keys::LABEL::STEP_ACTIVE (i),
                false));

            // Note (MIDI note number)
            params.push_back (std::make_unique<AudioParameterInt> (
                Params::Keys::ID::STEP_NOTE (i),
                Params::Keys::LABEL::STEP_NOTE (i),
                0,
                127,
                i));

            // Velocity
            params.push_back (std::make_unique<AudioParameterInt> (
                Params::Keys::ID::STEP_VELOCITY (i),
                Params::Keys::LABEL::STEP_VELOCITY (i),
                1,
                127,
                100));

            // Gate length (as percentage of step duration)
            params.push_back (std::make_unique<AudioParameterFloat> (
                Params::Keys::ID::STEP_GATE (i),
                Params::Keys::LABEL::STEP_GATE (i),
                0.0f,
                1.0f,
                0.75f));
        }

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
