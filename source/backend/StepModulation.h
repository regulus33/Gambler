#pragma once

#include <string>
#include <juce_core/juce_core.h>

/**
 * StepModulation - Individual modulation data (CC or Velocity)
 * 
 * Represents a single MIDI data message that can be sent alongside a note.
 * Can be either a CC message or a velocity value.
 */
enum class ModulationType {
    CC,
    Velocity
};

struct StepModulation {
    ModulationType type = ModulationType::CC;
    int ccNumber = 1;  // Default to mod wheel
    int value = 64;    // Default to middle value
    bool selected = false;
    
    StepModulation() = default;
    
    StepModulation(ModulationType t, int cc, int val)
        : type(t), ccNumber(cc), value(val) {}
    
    // Getters
    ModulationType getType() const { return type; }
    int getCCNumber() const { return ccNumber; }
    int getValue() const { return value; }
    bool isSelected() const { return selected; }
    
    // Setters with validation
    void setType(ModulationType t) {
        type = t;
    }
    
    void setCCNumber(int cc) {
        if (cc >= 0 && cc <= 127) {
            ccNumber = cc;
        }
    }
    
    void setValue(int val) {
        if (val >= 0 && val <= 127) {
            value = val;
        }
    }
    
    void setSelected(bool isSelected) {
        selected = isSelected;
    }
    
    // Helper to get a string representation of the type
    std::string getTypeString() const {
        return (type == ModulationType::Velocity) ? "Velocity" : "CC";
    }

    /**
     * Serialize to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("StepModulation");
        xml->setAttribute("type", static_cast<int>(type));
        xml->setAttribute("ccNumber", ccNumber);
        xml->setAttribute("value", value);
        xml->setAttribute("selected", selected);
        return xml;
    }

    /**
     * Deserialize from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "StepModulation") return;

        type = static_cast<ModulationType>(xml.getIntAttribute("type", 0));
        ccNumber = xml.getIntAttribute("ccNumber", 1);
        value = xml.getIntAttribute("value", 64);
        selected = xml.getBoolAttribute("selected", false);
    }
};

