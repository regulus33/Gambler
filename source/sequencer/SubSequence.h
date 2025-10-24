//
// Created by Gambler developers on 20.10.2025.
//

#ifndef GAMBLER_SUBSEQUENCE_H
#define GAMBLER_SUBSEQUENCE_H
#include "Step.h"

using StepArray = std::array<Step, 128>;

class Subsequence {
public:
    Subsequence() {
        // Initialize steps with default values
        for (int i = 0; i < 8; ++i) {
            this->steps[i].stepIndex = i;
            this->steps[i].midiNote = i; // Default note matches step index
        }
    }

    void advance() {
        stepIndex = (stepIndex + 1) % stepLength;
    }
    void setStepLength(int length) {
        this->stepLength = length;
    }


    int getStepLength() {
        return stepLength;
    }

    int stepIndex;
    int stepLength;
    StepArray steps;
};


#endif //GAMBLER_SUBSEQUENCE_H
