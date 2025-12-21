#pragma once

#include "StepNote.h"
#include <array>
#include <juce_core/juce_core.h>

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

    // Check if there's a rest at a specific index
    bool hasRestAt(int index) const {
        if (index >= 0 && index < MAX_LENGTH) {
            return notes[index].isRest();
        }
        return false;
    }

    // Check if the current note is a rest
    bool hasRest() const {
        return getCurrentNote().isRest();
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

    /**
     * Set the current index directly (for UI selection)
     * Also updates the selection state of all notes
     * @param index The index to set as current (0 to length-1, or -1 for none)
     */
    void setCurrentIndex(int index) {
        if (index >= -1 && index < length) {
            currentIndex = index;
            // Update selection state - only current note is selected
            for (int i = 0; i < MAX_LENGTH; i++) {
                notes[i].setSelected(i == currentIndex);
            }
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

    /**
     * Serialize to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("StepNotes");
        xml->setAttribute("length", length);
        xml->setAttribute("currentIndex", currentIndex);

        for (int i = 0; i < MAX_LENGTH; i++) {
            xml->addChildElement(notes[i].toXml().release());
        }

        return xml;
    }

    /**
     * Deserialize from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "StepNotes") return;

        length = xml.getIntAttribute("length", DEFAULT_LENGTH);
        currentIndex = xml.getIntAttribute("currentIndex", -1);

        int noteIndex = 0;
        for (auto* noteXml : xml.getChildIterator()) {
            if (noteIndex < MAX_LENGTH && noteXml->getTagName() == "StepNote") {
                notes[noteIndex].fromXml(*noteXml);
                noteIndex++;
            }
        }
    }

private:
    std::array<StepNote, MAX_LENGTH> notes;
    int length = DEFAULT_LENGTH;
    int currentIndex = -1;  // Start at -1 so first tick() advances to 0
};

