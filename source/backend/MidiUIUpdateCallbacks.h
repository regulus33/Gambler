#pragma once

#include "MidiMapping.h"
#include <functional>
#include <unordered_map>

/**
 * MidiUIUpdateCallbacks
 *
 * Registry of UI update callbacks for MIDI-controlled elements.
 * When MIDI changes domain state via commands, these callbacks update the UI
 * to reflect the changes (e.g., button press animations, slider positions).
 *
 * This is set up by UI components and used by PluginProcessor when handling MIDI.
 */
class MidiUIUpdateCallbacks {
public:
    // Callback signature: receives normalized value (0.0-1.0) or button state (0.0=off, 1.0=on)
    using UICallback = std::function<void(float value)>;

    /**
     * Register a UI update callback for a specific control
     * @param controlId The MIDI control ID
     * @param callback Function to call on message thread after state change
     */
    void registerCallback(MidiControlId controlId, UICallback callback) {
        callbacks[controlId] = callback;
    }

    /**
     * Get the UI update callback for a control
     * @param controlId The MIDI control ID
     * @return The callback, or nullptr if not registered
     */
    UICallback getCallback(MidiControlId controlId) const {
        auto it = callbacks.find(controlId);
        if (it != callbacks.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * Check if a control has a registered callback
     */
    bool hasCallback(MidiControlId controlId) const {
        return callbacks.find(controlId) != callbacks.end();
    }

    /**
     * Clear all callbacks (e.g., when UI is destroyed)
     */
    void clear() {
        callbacks.clear();
    }

private:
    std::unordered_map<MidiControlId, UICallback> callbacks;
};
