//
// Created by Gambler developers on 14.10.2025.
//

#ifndef GAMBLER_SEQUENCER_H
#define GAMBLER_SEQUENCER_H
#include <juce_audio_processors/juce_audio_processors.h>
using APVTS = juce::AudioProcessorValueTreeState;
class Sequencer
{
public:
    Sequencer(APVTS& state) : state(state)
    {

    }

    Sequencer() = default;

    bool test()
    {
        return true;
    }

private:
    APVTS& state;
};

#endif //GAMBLER_SEQUENCER_H
