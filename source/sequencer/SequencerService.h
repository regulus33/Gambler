//
// Created by Gambler developers on 14.10.2025.
//

#ifndef GAMBLER_SEQUENCER_H
#define GAMBLER_SEQUENCER_H
#include <juce_audio_processors/juce_audio_processors.h>
#include "Step.h"
#include "../data/Params.h"

#include <simd/common.h>

using APVTS = juce::AudioProcessorValueTreeState;
using namespace juce;
using namespace std;
using StepArray = Step[128];

class SequencerRepository
{
public:
    explicit SequencerRepository(APVTS& state) : state(state) {}

    int getStepLength() const
    {
        return getIntParameter (Params::Keys::ID::STEP_LENGTH);
    }

    int getIntParameter(const juce::String& parameterID) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto* selection = static_cast<AudioParameterInt*>(state.getParameter(parameterID));
        jassert(selection != nullptr);
        return selection->get();
    }

    bool setIntParameter(const juce::String& parameterID, int value)
    {
        if (auto* param = state.getParameter(parameterID))
        {
            auto range = param->getNormalisableRange();
            auto normalized = range.convertTo0to1(static_cast<float>(value));
            param->setValueNotifyingHost(normalized);
            return true;
        }
        return false;
    }

    int getStepIndex()
    {
        return getIntParameter (Params::Keys::ID::STEP_INDEX);
    }

    bool setStepIndex(int stepIndex)
    {
        return setIntParameter (Params::Keys::ID::STEP_INDEX, stepIndex);
    }

private:
    APVTS& state;
};

class SequencerService
{
public:
    explicit SequencerService(SequencerRepository& repository) : repo(repository)
    {

    }

    void advance()
    {
        auto length = repo.getStepLength();
        auto index = repo.getStepIndex();
        auto newIndex = (index + 1) % length;
        repo.setStepIndex(newIndex);
    }

    Step getCurrentStep()
    {
        juce::String noteParamID = Params::Keys::ID::STEP_NOTE(getStepIndex());
        int midiNote = repo.getIntParameter(noteParamID);
        Step step{};
        step.midiNote = midiNote;
        return step;
    }

    int getStepIndex()
    {
        return repo.getIntParameter(Params::Keys::ID::STEP_INDEX);
    }

    SequencerRepository& repo;
    StepArray stepArray = {};
    int stepLength = 16;
};

#endif //GAMBLER_SEQUENCER_H
