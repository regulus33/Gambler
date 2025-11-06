//
// Created by Gambler developers on 25.10.2025.
//

#ifndef GAMBLER_SEQUENCE_H
#define GAMBLER_SEQUENCE_H

#include <vector>
#include <random>
#include "Note.h"
#include "State.h"

using Notes = std::vector<Note>;
int constexpr DEFAULT_STEPS = 64;
int constexpr DEFAULT_INDEX = -1;  // Start at -1 so first tick() advances to 0

struct Sequence {
    Notes notes;
    int length = DEFAULT_STEPS;
    int index = DEFAULT_INDEX;
    State* state = nullptr;

    Sequence() {
        initSteps();
    }

    Sequence(State& s) : state(&s) {
        initSteps();
    }

    void setNote(int index, int note) {
        notes[index].setNote (0, note);
    }

    int tick() {
        index = (index + 1) % length;

        // If Roll button is pressed, randomize the note
        if (state && state->isRollPressed()) {
            // Get a random note value between 0-127
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 127);
            int randomNote = dis(gen);
            notes[index].setNote(0, randomNote);
        }

        // Tick the Note's internal subsequence and return the actual MIDI note value
        return notes[index].tick();
    }

    void initSteps() {
        for (int i = 0; i < DEFAULT_STEPS; i++) {
            notes.push_back (Note());
        }
    }
};

#endif //GAMBLER_SEQUENCE_H
