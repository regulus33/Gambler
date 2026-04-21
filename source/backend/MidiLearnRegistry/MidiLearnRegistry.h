#pragma once

#include "MappableControlInfo.h"
#include "MidiMessageIdentifier.h"
#include "../MidiMapping/MidiMapping.h"
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
class MidiLearnRegistry {
public:
    MidiLearnRegistry() = default;

    // ========== MIDI Learn Mode Management ==========

    void enterLearnMode() {
        isInLearnMode = true;
        currentLearningControlId = MidiControlId::COUNT;
        DBG("Entered MIDI learn mode");
    }

    void exitLearnMode() {
        isInLearnMode = false;
        currentLearningControlId = MidiControlId::COUNT;
        DBG("Exited MIDI learn mode");
    }

    bool isLearning() const { return isInLearnMode; }

    void setLearningControl(MidiControlId controlId) {
        currentLearningControlId = controlId;
        DBG("Waiting for MIDI input for control: " << getMidiControlName(controlId));
    }

    MidiControlId getLearningControl() const { return currentLearningControlId; }

    // ========== Control Registration ==========

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

    void unregisterControl(MidiControlId controlId) {
        controls.erase(controlId);
    }

    void updateControlBounds(MidiControlId controlId, const juce::Rectangle<int>& bounds) {
        auto it = controls.find(controlId);
        if (it != controls.end())
            it->second.bounds = bounds;
    }

    const std::unordered_map<MidiControlId, MappableControlInfo>& getControls() const {
        return controls;
    }

    // ========== MIDI Learning ==========

    bool learnMidiMessage(MidiMessageType type, int channel, int number) {
        if (!isInLearnMode) return false;
        if (currentLearningControlId == MidiControlId::COUNT) return false;

        MidiMessageIdentifier identifier;
        identifier.type = type;
        identifier.channel = channel;
        identifier.number = number;

        userMappings[identifier] = currentLearningControlId;
        controlToMidi[currentLearningControlId] = identifier;

        DBG("Learned mapping: " << identifier.toString() << " → " <<
            getMidiControlName(currentLearningControlId));

        currentLearningControlId = MidiControlId::COUNT;
        return true;
    }

    MidiControlId findUserMapping(MidiMessageType type, int channel, int number) const {
        MidiMessageIdentifier identifier;
        identifier.type = type;
        identifier.channel = channel;
        identifier.number = number;

        auto it = userMappings.find(identifier);
        if (it != userMappings.end())
            return it->second;
        return MidiControlId::COUNT;
    }

    const MidiMessageIdentifier* getMidiForControl(MidiControlId controlId) const {
        auto it = controlToMidi.find(controlId);
        if (it != controlToMidi.end())
            return &it->second;
        return nullptr;
    }

    void clearMapping(MidiControlId controlId) {
        auto it = controlToMidi.find(controlId);
        if (it != controlToMidi.end()) {
            userMappings.erase(it->second);
            controlToMidi.erase(it);
            DBG("Cleared mapping for " << getMidiControlName(controlId));
        }
    }

    void clearAllMappings() {
        userMappings.clear();
        controlToMidi.clear();
        DBG("Cleared all user MIDI mappings");
    }

    // ========== XML Serialization ==========

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
    bool isInLearnMode = false;
    MidiControlId currentLearningControlId = MidiControlId::COUNT;

    std::unordered_map<MidiControlId, MappableControlInfo> controls;
    std::unordered_map<MidiMessageIdentifier, MidiControlId, MidiMessageIdentifierHash> userMappings;
    std::unordered_map<MidiControlId, MidiMessageIdentifier> controlToMidi;
};
