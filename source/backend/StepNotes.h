#pragma once

#include "StepNote.h"
#include <array>

/**
 * StepNotes - Sub-sequence of notes within a Step
 * 
 * Contains up to 8 notes that cycle through on each parent Step trigger.
 * This creates evolving pattern variations where steps with length > 1
 * cycle through their substep notes on each sequence pass-through.
 * 
 * Example: A 4-bar loop where step 3 has 2 substeps [60, 70]:
 * - First pass: plays note 60
 * - Second pass: plays note 70
 * - Third pass: wraps back to 60
 */
class StepNotes {
public:
    static constexpr int MAX_LENGTH = 8;
    static constexpr int MIN_LENGTH = 1;
    static constexpr int DEFAULT_LENGTH = 1;
    static constexpr int DEFAULT_NOTE = 60;
    
    StepNotes() {
        // Initialize all notes to default
        for (int i = 0; i < MAX_LENGTH; i++) {
            notes[i] = StepNote(DEFAULT_NOTE);
        }
    }
    
    // Getters
    int getLength() const { return length; }
    int getCurrentIndex() const { return currentIndex; }
    
    const StepNote& getNote(int index) const { 
        if (index >= 0 && index < MAX_LENGTH) {
            return notes[index];
        }
        return notes[0];  // Fallback
    }
    
    StepNote& getNote(int index) {
        if (index >= 0 && index < MAX_LENGTH) {
            return notes[index];
        }
        return notes[0];  // Fallback
    }
    
    // Get the currently selected note (the one at currentIndex)
    const StepNote& getCurrentNote() const {
        int idx = (currentIndex == -1) ? 0 : currentIndex;
        return notes[idx];
    }
    
    // Setters
    void setNote(int index, int noteNumber) {
        if (index >= 0 && index < MAX_LENGTH) {
            notes[index].setNoteNumber(noteNumber);
        }
    }
    
    void setLength(int newLength) {
        if (newLength >= MIN_LENGTH && newLength <= MAX_LENGTH) {
            length = newLength;
        }
    }
    
    void setNoteSelected(int index, bool selected) {
        if (index >= 0 && index < MAX_LENGTH) {
            notes[index].setSelected(selected);
        }
    }
    
    // Operations
    void increaseLength() {
        if (length < MAX_LENGTH) {
            length++;
        }
    }
    
    void decreaseLength() {
        if (length > MIN_LENGTH) {
            length--;
        }
    }
    
    /**
     * Advance to next note in the sub-sequence and return it.
     * Updates selection state to mark current note as selected.
     * 
     * @return The MIDI note number at the new current index
     */
    int tick() {
        // Advance index (wraps at length boundary)
        currentIndex = (currentIndex + 1) % length;
        
        // Update selection state - only current note is selected
        for (int i = 0; i < MAX_LENGTH; i++) {
            notes[i].setSelected(i == currentIndex);
        }
        
        return notes[currentIndex].getNoteNumber();
    }
    
    /**
     * Reset the index to -1 (pre-first-tick state)
     * Used when sequence restarts or for testing
     */
    void reset() {
        currentIndex = -1;
        for (int i = 0; i < MAX_LENGTH; i++) {
            notes[i].setSelected(false);
        }
    }
    
private:
    std::array<StepNote, MAX_LENGTH> notes;
    int length = DEFAULT_LENGTH;
    int currentIndex = -1;  // Start at -1 so first tick() advances to 0
};

