//
// Created by Gambler developers on 25.10.2025.
//

#ifndef GAMBLER_NOTE_H
#define GAMBLER_NOTE_H
#include <juce_core/system/juce_PlatformDefs.h>
#include <vector>
#include <array>
#include "Slot.h"

using MidiNotes = std::vector<int>;
constexpr int DEFAULT_MIDI_NOTE_LENGTH = 1;
constexpr int DEFAULT_MIDI_NOTE = 60; // C4
constexpr int MAX_MIDI_NOTE_LENGTH = 8; // we just can't fit more on the screen.
constexpr int MIN_MIDI_NOTE_LENGTH = 1; // minimum is 1, not 8
constexpr int NUM_SLOTS = 7;
constexpr int DEFAULT_VELOCITY = 100;

struct Note {
    MidiNotes notes;
    int length = DEFAULT_MIDI_NOTE_LENGTH;
    int index = 0;
    std::array<Slot, NUM_SLOTS> slots;

    Note() {
        for (int i = 0; i < DEFAULT_MIDI_NOTE_LENGTH; i++) {
            notes.push_back (DEFAULT_MIDI_NOTE);
        }

        // Initialize slots with default values
        // First slot defaults to velocity, rest to CC
        slots[0] = Slot(SlotType::Velocity, 0, DEFAULT_VELOCITY);
        for (int i = 1; i < NUM_SLOTS; i++) {
            slots[i] = Slot(SlotType::CC, i, 64);
        }
    }

    void setNote (int index, int note) {
        if (index > this->length || index < 0) {
            DBG ("Cannot set a note that is outside length!");
            jassertfalse;
            return;
        }
        if (!(note <= 127 && note >= 0)) {
            DBG ("Midi note out of bounds!");
            jassertfalse;
            return;
        }

        notes[index] = note;
    }

    int tick() {
        int currentNote = notes[index];
        index = (index + 1) % length;
        return currentNote;
    }

    void increaseMidiNotes() {
        if (length < MAX_MIDI_NOTE_LENGTH) {
            length++;
            // Ensure the notes vector has enough elements
            if (notes.size() < static_cast<size_t>(length)) {
                notes.push_back(DEFAULT_MIDI_NOTE);
            }
        } else {
            DBG ("[Note][increaseMidiNotes] Max length reached!");
        }
    }

    void decreaseMidiNotes() {
        if (length > MIN_MIDI_NOTE_LENGTH) {
            length--;
        } else {
            DBG ("[Note][decreaseMidiNotes] Min length reached!");
        }
    }

    /**
     * @brief Sets the type of a slot (CC or Velocity)
     * @param slotIndex Index of the slot (0-6)
     * @param type The type to set (CC or Velocity)
     */
    void setSlotType(int slotIndex, SlotType type) {
        if (slotIndex >= 0 && slotIndex < NUM_SLOTS) {
            slots[slotIndex].type = type;
        } else {
            DBG ("Slot index out of bounds!");
            jassertfalse;
        }
    }

    /**
     * @brief Sets the CC number for a slot
     * @param slotIndex Index of the slot (0-6)
     * @param ccNumber CC number (0-127)
     */
    void setSlotCCNumber(int slotIndex, int ccNumber) {
        if (slotIndex >= 0 && slotIndex < NUM_SLOTS) {
            if (ccNumber >= 0 && ccNumber <= 127) {
                slots[slotIndex].ccNumber = ccNumber;
            } else {
                DBG ("CC number out of bounds!");
                jassertfalse;
            }
        } else {
            DBG ("Slot index out of bounds!");
            jassertfalse;
        }
    }

    /**
     * @brief Sets the value for a slot
     * @param slotIndex Index of the slot (0-6)
     * @param value Value (0-127)
     */
    void setSlotValue(int slotIndex, int value) {
        if (slotIndex >= 0 && slotIndex < NUM_SLOTS) {
            if (value >= 0 && value <= 127) {
                slots[slotIndex].value = value;
            } else {
                DBG ("Slot value out of bounds!");
                jassertfalse;
            }
        } else {
            DBG ("Slot index out of bounds!");
            jassertfalse;
        }
    }

    /**
     * @brief Gets a slot by index
     * @param slotIndex Index of the slot (0-6)
     * @return Reference to the slot
     */
    Slot& getSlot(int slotIndex) {
        if (slotIndex >= 0 && slotIndex < NUM_SLOTS) {
            return slots[slotIndex];
        }
        DBG ("Slot index out of bounds!");
        jassertfalse;
        return slots[0]; // Fallback
    }
};

#endif //GAMBLER_NOTE_H
