//
// Created by Gambler developers on 25.10.2025.
//

#ifndef GAMBLER_SEQUENCE_H
#define GAMBLER_SEQUENCE_H

#include <vector>
#include "Note.h"

using Notes = std::vector<Note>;
int constexpr DEFAULT_STEPS = 128;

struct Sequence {
    Notes notes;

    Sequence() {
        initSteps();
    }

    void initSteps() {
        for (int i = 0; i < DEFAULT_STEPS; i++) {
            notes.push_back (Note());
        }
    }

};

#endif //GAMBLER_SEQUENCE_H
