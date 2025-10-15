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
        juce::String noteParamID = StepKeyBuilder::noteParamID (getStepIndex().getValue());
        int note = state.getParameterAsValue (noteParamID).getValue();
        return Step(static_cast<int>(note));
    }

    juce::Value getStepIndex()
    {
         auto a = (state.getParameterAsValue (ParamStrings::currentStepId));
        return a;

    }


    APVTS& state;
    StepArray stepArray = {};
    int stepIndex = 0;
    int stepLength = 16;
};

#endif //GAMBLER_SEQUENCER_H
