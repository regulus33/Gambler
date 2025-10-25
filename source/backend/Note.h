//
// Created by Gambler developers on 25.10.2025.
//

#ifndef GAMBLER_NOTE_H
#define GAMBLER_NOTE_H
#include <juce_core/system/juce_PlatformDefs.h>
#include <vector>

using MidiNotes = std::vector<int>;
constexpr int DEFAULT_MIDI_NOTE_LENGTH = 1;
constexpr int DEFAULT_MIDI_NOTE = 60; // C4
constexpr int MAX_MIDI_NOTE_LENGTH = 8; // we just can't fit more on the screen.
constexpr int MIN_MIDI_NOTE_LENGTH = 8; // we just can't fit more on the screen.

struct Note {
    MidiNotes midi_notes;
    int length = DEFAULT_MIDI_NOTE_LENGTH;
    Note() {
        for (int i=0; i < DEFAULT_MIDI_NOTE_LENGTH; i++) {
            midi_notes.push_back (DEFAULT_MIDI_NOTE);
        }
    }

    void increaseMidiNotes() {
        if (length < MAX_MIDI_NOTE_LENGTH) {
            length++;
        } else {
            DBG("[Note][increaseMidiNotes] Max length reached!");
        }
    }

    void decreaseMidiNotes() {
        if (length > MIN_MIDI_NOTE_LENGTH) {
            length--;
        } else {
            DBG("[Note][decreaseMidiNotes] Min length reached!");
        }
    }
};

#endif //GAMBLER_NOTE_H
