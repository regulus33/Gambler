#pragma once

/**
 * Type of MIDI message
 */
enum class MidiMessageType {
    CC,     // Control Change (0-127)
    NOTE    // Note On/Off (0-127)
};
