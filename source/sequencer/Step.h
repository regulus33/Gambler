//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_STEP_H
#define GAMBLER_STEP_H

struct Step
{
    int midiNote = 60;        // Default to middle C
    int midiVelocity = 100;   // Default velocity
    int stepIndex = 0;
    float gateLength = 0.75f; // Default gate length (75% of step duration)
    bool active = false;
    bool selected = false;
};

#endif //GAMBLER_STEP_H
