#pragma once

#include "../MidiMapping/MidiMapping.h"
#include <juce_core/juce_core.h>
#include <functional>

/**
 * MIDI message identifier (for display)
 */
struct MidiMessageIdentifier {
    MidiMessageType type = MidiMessageType::CC;
    int channel = 0;  // 1-16
    int number = 0;   // CC number or Note number

    juce::String toString() const {
        juce::String typeStr = (type == MidiMessageType::CC) ? "CC" : "N";
        return "CH" + juce::String(channel) + "-" + typeStr + juce::String(number);
    }

    bool operator==(const MidiMessageIdentifier& other) const {
        return type == other.type && channel == other.channel && number == other.number;
    }
};

struct MidiMessageIdentifierHash {
    std::size_t operator()(const MidiMessageIdentifier& id) const {
        return std::hash<int>()(static_cast<int>(id.type)) ^
               (std::hash<int>()(id.channel) << 1) ^
               (std::hash<int>()(id.number) << 2);
    }
};
