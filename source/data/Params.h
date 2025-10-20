//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_GAMBLERPARAMS_H
#define GAMBLER_GAMBLERPARAMS_H

#include "juce_audio_processors/juce_audio_processors.h"

using namespace std;
using namespace juce;


// this namespace holds keys for the apvts as well as values.
// Step-related data is now stored in the Sequencer domain object, not in APVTS.
namespace Params
{
    namespace Keys
    {
        namespace ID
        {
            // Currently no parameters - step data is stored in Sequencer domain object
        }

        namespace LABEL
        {
            // Currently no parameters - step data is stored in Sequencer domain object
        }

    };

    static AudioProcessorValueTreeState::ParameterLayout createDefaultParameterLayout()
    {
        vector<unique_ptr<RangedAudioParameter>> params;

        // No parameters currently - step data is stored in Sequencer domain object
        // Future: Add global plugin parameters here (e.g., master volume, tempo sync, etc.)

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
