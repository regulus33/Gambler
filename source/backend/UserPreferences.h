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
    bool followStep = true;          // Whether to auto-scroll the sequencer window to follow the active step
    bool followPlayback = true;      // Whether the selected step for editing follows the playback position

    // MIDI output preferences
    double defaultNoteLength = 0.9;  // Note length as percentage of step duration (0.0-1.0)
                                     // Default 0.9 = 90% of step length (leaves small gap)
    bool midiClockOutputEnabled = false;  // Whether to send MIDI clock to all devices (standalone only)

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
        xml->setAttribute("followStep", followStep);
        xml->setAttribute("followPlayback", followPlayback);
        xml->setAttribute("defaultNoteLength", defaultNoteLength);
        xml->setAttribute("midiClockOutputEnabled", midiClockOutputEnabled);

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
        followStep = xml.getBoolAttribute("followStep", true);
        followPlayback = xml.getBoolAttribute("followPlayback", true);
        defaultNoteLength = xml.getDoubleAttribute("defaultNoteLength", 0.9);
        midiClockOutputEnabled = xml.getBoolAttribute("midiClockOutputEnabled", false);
    }
};

