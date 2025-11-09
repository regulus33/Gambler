#pragma once

#include <string>

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
};

