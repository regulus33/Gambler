//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_STEP_H
#define GAMBLER_STEP_H

struct Step
{
    int midiNote;
    int midiVelocity;
    int stepIndex;
    bool active;
    bool selected;
};

#endif //GAMBLER_STEP_H
