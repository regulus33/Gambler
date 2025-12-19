#pragma once

#include <juce_core/juce_core.h>

/**
 * UserPreferences
 * 
 * Stores user customization settings that are not part of the domain model
 * (Clock, Sequence, etc.) but should persist with the plugin state.
 * 
 * This includes UI preferences, editor settings, and other user-specific
 * customizations that don't affect the core sequencer logic.
 */
struct UserPreferences {
    // UI/Editor preferences
    bool previewNotesOnEdit = true;  // Whether to trigger MIDI preview when editing notes
    
    // Future preferences can be added here:
    // - UI theme/color scheme
    // - Grid snap settings
    // - Keyboard shortcuts
    // - Display preferences (show/hide certain UI elements)
    // - etc.
    
    /**
     * Serialize preferences to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("UserPreferences");
        
        xml->setAttribute("previewNotesOnEdit", previewNotesOnEdit);
        
        return xml;
    }
    
    /**
     * Deserialize preferences from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "UserPreferences") {
            return;
        }
        
        previewNotesOnEdit = xml.getBoolAttribute("previewNotesOnEdit", true);
    }
};

