#pragma once

#include <juce_core/juce_core.h>

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
    // Note: -1 is a valid value representing a rest
    void setNoteNumber(int note) {
        if (note >= -1 && note <= 127) {
            noteNumber = note;
        }
    }

    // Check if this note is a rest
    bool isRest() const {
        return noteNumber == -1;
    }
    
    void setSelected(bool isSelected) {
        selected = isSelected;
    }

    /**
     * Serialize to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("StepNote");
        xml->setAttribute("noteNumber", noteNumber);
        xml->setAttribute("selected", selected);
        return xml;
    }

    /**
     * Deserialize from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "StepNote") return;

        noteNumber = xml.getIntAttribute("noteNumber", 60);
        selected = xml.getBoolAttribute("selected", false);
    }
};

