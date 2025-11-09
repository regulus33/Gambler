#pragma once

/**
 * StepNote - Individual note within a StepNotes sub-sequence
 * 
 * This is a simple data container representing a single MIDI note
 * with selection state for UI interaction.
 */
struct StepNote {
    int noteNumber = 60;  // Middle C default
    bool selected = false;
    
    StepNote() = default;
    
    explicit StepNote(int note) : noteNumber(note) {}
    
    StepNote(int note, bool isSelected) 
        : noteNumber(note), selected(isSelected) {}
    
    // Getters
    int getNoteNumber() const { return noteNumber; }
    bool isSelected() const { return selected; }
    
    // Setters with validation
    void setNoteNumber(int note) {
        if (note >= 0 && note <= 127) {
            noteNumber = note;
        }
    }
    
    void setSelected(bool isSelected) {
        selected = isSelected;
    }
};

