//
// Created by Gambler developers on 14.10.2025.
//

#ifndef GAMBLER_SEQUENCER_H
#define GAMBLER_SEQUENCER_H
#include "Controls.h"
#include "Step.h"
#include <array>
#include <random>

class Sequencer
{
public:
    Sequencer()
    {
        // Initialize steps with default values
        for (int i = 0; i < 128; ++i)
        {
            steps[i].stepIndex = i;
            steps[i].midiNote = i; // Default note matches step index
        }
    }

    void advance()
    {
        // If random mode is enabled, randomize the note of the current step before advancing
        if (controls.getRandom())
        {
            steps[stepIndex].midiNote = getRandomNote();
        }

        stepIndex = (stepIndex + 1) % stepLength;
    }

    Step getCurrentStep() const
    {
        return steps[stepIndex];
    }

    int getStepIndex() const
    {
        return stepIndex;
    }

    void setStepIndex(int index)
    {
        if (index >= 0 && index < 128)
        {
            stepIndex = index;
        }
    }

    int getStepLength() const
    {
        return stepLength;
    }

    void setStepLength(int length)
    {
        if (length >= 1 && length <= 128)
        {
            stepLength = length;
        }
    }

    Step& getStep(int index)
    {
        return steps[index];
    }

    const Step& getStep(int index) const
    {
        return steps[index];
    }

    Controls& getControls()
    {
        return controls;
    }

private:
    int getRandomNote()
    {
        std::uniform_int_distribution<int> distribution(0, 127);
        return distribution(randomEngine);
    }

    Controls controls;
    std::array<Step, 128> steps;
    int stepIndex = 0;
    int stepLength = 16;
    // seed the
    std::mt19937 randomEngine{std::random_device{}()};
};

#endif //GAMBLER_SEQUENCER_H
