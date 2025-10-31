//
// Created by Gambler developers on 25.10.2025.
//

#ifndef GAMBLER_SEQUENCE_H
#define GAMBLER_SEQUENCE_H

#include <vector>
#include <random>
#include "Note.h"
#include "Controls.h"
#include "SequencerOptions.h"

using Notes = std::vector<Note>;
int constexpr DEFAULT_STEPS = 128;
int constexpr DEFAULT_INDEX = -1;  // Start at -1 so first tick() advances to 0

struct Sequence {
    Notes notes;
    int length = DEFAULT_STEPS;
    int index = DEFAULT_INDEX;
    Controls* controls = nullptr;
    SequencerOptions* options = nullptr;

    Sequence() {
        initSteps();
    }

    Sequence(Controls& ctrl, SequencerOptions& opts) : controls(&ctrl), options(&opts) {
        initSteps();
    }

    void setNote(int index, int note) {
        notes[index].setNote (0, note);
    }

    int tick() {
        index = (index + 1) % length;

        // If Roll button is pressed, randomize the note
        if (controls && controls->isRollPressed()) {
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

    SequencerOptions* getOptions() const {
        return options;
    }
};

#endif //GAMBLER_SEQUENCE_H
