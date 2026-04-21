#pragma once

/**
 * Utility class for converting MIDI values to control values
 */
class MidiValueConverter {
public:
    /**
     * Convert MIDI CC value (0-127) to normalized value (0.0-1.0)
     */
    static float midiToNormalized(int midiValue, bool invert = false) {
        if (invert) {
            return (127 - midiValue) / 127.0f;
        }
        return midiValue / 127.0f;
    }

    /**
     * Convert MIDI value to scaled value with min/max range
     */
    static float midiToScaled(int midiValue, float minValue, float maxValue, bool invert = false) {
        float normalized = midiToNormalized(midiValue, invert);
        return minValue + (normalized * (maxValue - minValue));
    }

    /**
     * Convert MIDI CC value to boolean (threshold at 64)
     */
    static bool midiToBoolean(int midiValue) {
        return midiValue >= 64;
    }

    /**
     * Convert MIDI value to discrete integer index
     */
    static int midiToDiscreteIndex(int midiValue, int maxIndex) {
        float normalized = midiValue / 127.0f;
        int index = static_cast<int>(normalized * maxIndex);
        if (index > maxIndex) index = maxIndex;
        return index;
    }
};
