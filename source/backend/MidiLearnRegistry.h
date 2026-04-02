#pragma once

#include "MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>


/**
 * MidiLearnRegistry
 *
 * Central registry for MIDI learn mode and user-configurable MIDI mappings.
 * Lives in PluginProcessor and manages:
 * - MIDI learn state machine
 * - User-created mappings (separate from hardcoded defaults)
 * - Control bounds for overlay rendering
 * - Callbacks for UI updates
 *
 * Thread Safety:
 * - Called from both audio thread (MIDI input) and message thread (UI)
 * - Uses callbacks and flags for cross-thread communication
 */

/**
 * Information about a mappable control
 */
struct MappableControlInfo {
    MidiControlId controlId;
    juce::Rectangle<int> bounds;  // Bounds in screen coordinates
    juce::String displayName;     // Human-readable name
    bool isActive = true;         // Whether control is currently visible/mappable
};

/**
 * MIDI message identifier (for display)
 */
struct MidiMessageIdentifier {
    MidiMessageType type;
    int channel;      // 1-16
    int number;       // CC number or Note number

    juce::String toString() const {
        juce::String typeStr = (type == MidiMessageType::CC) ? "CC" : "N";
        return "CH" + juce::String(channel) + "-" + typeStr + juce::String(number);
    }

    bool operator==(const MidiMessageIdentifier& other) const {
        return type == other.type && channel == other.channel && number == other.number;
    }
};

// Hash function for MidiMessageIdentifier
struct MidiMessageIdentifierHash {
    std::size_t operator()(const MidiMessageIdentifier& id) const {
        return std::hash<int>()(static_cast<int>(id.type)) ^
               (std::hash<int>()(id.channel) << 1) ^
               (std::hash<int>()(id.number) << 2);
    }
};

class MidiLearnRegistry {
public:
    MidiLearnRegistry() = default;

    // ========== MIDI Learn Mode Management ==========

    /**
     * Enter MIDI learn mode
     * All registered controls will show overlay
     */
    void enterLearnMode() {
        isInLearnMode = true;
        currentLearningControlId = MidiControlId::COUNT;  // None selected
        DBG("Entered MIDI learn mode");
    }

    /**
     * Exit MIDI learn mode
     */
    void exitLearnMode() {
        isInLearnMode = false;
        currentLearningControlId = MidiControlId::COUNT;
        DBG("Exited MIDI learn mode");
    }

    /**
     * Check if currently in learn mode
     */
    bool isLearning() const {
        return isInLearnMode;
    }

    /**
     * Set which control is currently waiting for MIDI input
     */
    void setLearningControl(MidiControlId controlId) {
        currentLearningControlId = controlId;
        DBG("Waiting for MIDI input for control: " << getMidiControlName(controlId));
    }

    /**
     * Get the control that's currently learning (or COUNT if none)
     */
    MidiControlId getLearningControl() const {
        return currentLearningControlId;
    }

    // ========== Control Registration ==========

    /**
     * Register a mappable control
     * Called by MidiMappableComponent when it becomes visible
     */
    void registerControl(MidiControlId controlId,
                        const juce::Rectangle<int>& bounds,
                        const juce::String& displayName) {
        MappableControlInfo info;
        info.controlId = controlId;
        info.bounds = bounds;
        info.displayName = displayName;
        info.isActive = true;

        controls[controlId] = info;
    }

    /**
     * Unregister a control
     * Called by MidiMappableComponent on destruction
     */
    void unregisterControl(MidiControlId controlId) {
        controls.erase(controlId);
    }

    /**
     * Update control bounds (e.g., when component is resized/moved)
     */
    void updateControlBounds(MidiControlId controlId, const juce::Rectangle<int>& bounds) {
        auto it = controls.find(controlId);
        if (it != controls.end()) {
            it->second.bounds = bounds;
        }
    }

    /**
     * Get all registered controls
     */
    const std::unordered_map<MidiControlId, MappableControlInfo>& getControls() const {
        return controls;
    }

    // ========== MIDI Learning ==========

    /**
     * Process an incoming MIDI message during learn mode
     * Called from audio thread (processBlock)
     * Returns true if a mapping was created
     */
    bool learnMidiMessage(MidiMessageType type, int channel, int number) {
        if (!isInLearnMode) return false;
        if (currentLearningControlId == MidiControlId::COUNT) return false;

        // Create identifier
        MidiMessageIdentifier identifier;
        identifier.type = type;
        identifier.channel = channel;
        identifier.number = number;

        // Store the mapping
        userMappings[identifier] = currentLearningControlId;

        // Also store reverse mapping for display
        controlToMidi[currentLearningControlId] = identifier;

        DBG("Learned mapping: " << identifier.toString() << " → " <<
            getMidiControlName(currentLearningControlId));

        // Clear learning state (user can click another control to continue)
        currentLearningControlId = MidiControlId::COUNT;

        return true;
    }

    /**
     * Find user mapping for a MIDI message
     * Returns COUNT if not mapped
     */
    MidiControlId findUserMapping(MidiMessageType type, int channel, int number) const {
        MidiMessageIdentifier identifier;
        identifier.type = type;
        identifier.channel = channel;
        identifier.number = number;

        auto it = userMappings.find(identifier);
        if (it != userMappings.end()) {
            return it->second;
        }
        return MidiControlId::COUNT;
    }

    /**
     * Get MIDI message identifier for a control (for display)
     * Returns nullptr if not mapped
     */
    const MidiMessageIdentifier* getMidiForControl(MidiControlId controlId) const {
        auto it = controlToMidi.find(controlId);
        if (it != controlToMidi.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * Clear a specific mapping
     */
    void clearMapping(MidiControlId controlId) {
        // Find and remove from both maps
        auto it = controlToMidi.find(controlId);
        if (it != controlToMidi.end()) {
            userMappings.erase(it->second);
            controlToMidi.erase(it);
            DBG("Cleared mapping for " << getMidiControlName(controlId));
        }
    }

    /**
     * Clear all user mappings (debug/reset function)
     */
    void clearUserMappings() {
        int count = static_cast<int>(userMappings.size());
        userMappings.clear();
        controlToMidi.clear();
        DBG("Cleared " << count << " user MIDI mappings");
    }

    /**
     * Clear all user mappings
     */
    void clearAllMappings() {
        userMappings.clear();
        controlToMidi.clear();
        DBG("Cleared all user MIDI mappings");
    }

    // ========== XML Serialization ==========

    /**
     * Serialize user mappings to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("MidiLearnMappings");

        for (const auto& [identifier, controlId] : userMappings) {
            auto mappingXml = xml->createNewChildElement("Mapping");
            mappingXml->setAttribute("type", identifier.type == MidiMessageType::CC ? "CC" : "NOTE");
            mappingXml->setAttribute("channel", identifier.channel);
            mappingXml->setAttribute("number", identifier.number);
            mappingXml->setAttribute("controlId", static_cast<int>(controlId));
        }

        return xml;
    }

    /**
     * Load user mappings from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        clearAllMappings();

        for (auto* mappingXml : xml.getChildIterator()) {
            if (mappingXml->hasTagName("Mapping")) {
                MidiMessageIdentifier identifier;
                identifier.type = mappingXml->getStringAttribute("type") == "CC" ?
                    MidiMessageType::CC : MidiMessageType::NOTE;
                identifier.channel = mappingXml->getIntAttribute("channel");
                identifier.number = mappingXml->getIntAttribute("number");

                auto controlId = static_cast<MidiControlId>(mappingXml->getIntAttribute("controlId"));

                userMappings[identifier] = controlId;
                controlToMidi[controlId] = identifier;
            }
        }

        DBG("Loaded " << userMappings.size() << " user MIDI mappings from XML");
    }

private:
    // MIDI learn state
    bool isInLearnMode = false;
    MidiControlId currentLearningControlId = MidiControlId::COUNT;

    // Registered controls (controlId → info)
    std::unordered_map<MidiControlId, MappableControlInfo> controls;

    // User mappings (MIDI → controlId)
    std::unordered_map<MidiMessageIdentifier, MidiControlId, MidiMessageIdentifierHash> userMappings;

    // Reverse mapping for display (controlId → MIDI)
    std::unordered_map<MidiControlId, MidiMessageIdentifier> controlToMidi;
};
