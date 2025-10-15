//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_STEPKEYBUILDER_H
#define GAMBLER_STEPKEYBUILDER_H

#include <juce_core/juce_core.h>

class StepKeyBuilder
{
public:
    // IDs
    static juce::String activeParamID (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "step_" + stepNum + "_active";
    }

    static juce::String noteParamID (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "step_" + stepNum + "_note";
    }

    static juce::String velocityParamID (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "step_" + stepNum + "_velocity";
    }

    static juce::String gateParamID (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "step_" + stepNum + "_gate";
    }

    // Names
    static juce::String activeParamName (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "Step " + stepNum + " Active";
    }

    static juce::String noteParamName (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "Step " + stepNum + " Note";
    }

    static juce::String velocityParamName (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "Step " + stepNum + " Velocity";
    }

    static juce::String gateParamName (int stepNumber)
    {
        juce::String stepNum (stepNumber);
        return "Step " + stepNum + " Gate";
    }
};

#endif //GAMBLER_STEPKEYBUILDER_H
