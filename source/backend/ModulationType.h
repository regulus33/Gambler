#pragma once

/**
 * ModulationType - Type of MIDI modulation data
 *
 * Represents whether a step modulation slot controls
 * a CC message or the note velocity.
 */
enum class ModulationType {
    CC,
    Velocity
};
