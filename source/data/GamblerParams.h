//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_GAMBLERPARAMS_H
#define GAMBLER_GAMBLERPARAMS_H

#include "ParamStrings.h"
#include "StepKeyBuilder.h"
using RangedAudioParameterVector = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;

struct GamblerParams
{
    static juce::AudioProcessorValueTreeState::ParameterLayout createDefaultParameterLayout()
    {
        RangedAudioParameterVector params;

        // Global sequencer parameters
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            ParamStrings::STEP_LENGTH, ParamStrings::LABEL_STEP_LENGTH, 1, 128, 16));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            ParamStrings::CURRENT_STEP, ParamStrings::LABEL_CURRENT_STEP, 0, 127, 99));

        // Per-step parameters (128 steps)
        for (int i = 0; i < 128; ++i)
        {

            // Step active/trigger
            params.push_back (std::make_unique<juce::AudioParameterBool> (
                StepKeyBuilder::activeParamID(i),
                StepKeyBuilder::activeParamName(i),
                false));

            // Note (MIDI note number)
            params.push_back (std::make_unique<juce::AudioParameterInt> (
                StepKeyBuilder::noteParamID(i),
                StepKeyBuilder::noteParamName(i),
                0,
                127,
                i));

            // Velocity
            params.push_back (std::make_unique<juce::AudioParameterInt> (
                StepKeyBuilder::velocityParamID(i),
                StepKeyBuilder::velocityParamName(i),
                1,
                127,
                100));

            // Gate length (as percentage of step duration)
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                StepKeyBuilder::gateParamID(i),
                StepKeyBuilder::gateParamName(i),
                0.0f,
                1.0f,
                0.75f));
        }

        return { params.begin(), params.end() };
    }
};

#endif //GAMBLER_GAMBLERPARAMS_H
