//
// Created by Gambler developers on 14.10.2025.
//

#ifndef GAMBLER_SEQUENCER_H
#define GAMBLER_SEQUENCER_H
#include <juce_audio_processors/juce_audio_processors.h>
#include "Step.h"
#include "data/StepKeyBuilder.h"
#include "data/ParamStrings.h"

#include <simd/common.h>

using APVTS = juce::AudioProcessorValueTreeState;
using StepArray = Step[128];

class Sequencer
{
public:
    Sequencer(APVTS& state) : state(state)
    {

    }

    void advance()
    {
        auto length = static_cast<int>(*state.getRawParameterValue("step_length"));
        stepIndex = (stepIndex + 1) % length;
    }

    Step getCurrentStep()
    {
        // gets the id
        juce::String noteParamID = StepKeyBuilder::noteParamID (getStepIndex());
        juce::var note = state.getParameterAsValue (noteParamID).getValue();
        return Step(static_cast<int>(note));
    }

    int getStepIndex()
    {
        auto a = (state.getParameterAsValue (ParamStrings::currentStepId));
        return static_cast<int>(juce::Value(a).getValue());
    }


    APVTS& state;
    StepArray stepArray = {};
    int stepIndex = 0;
    int stepLength = 16;
};

#endif //GAMBLER_SEQUENCER_H
