#pragma once

#include "../backend/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiMappableComponent
 *
 * Base class for UI components that can be MIDI mapped.
 * Provides:
 * - Automatic registration/unregistration with MidiLearnRegistry
 * - Overlay rendering during MIDI learn mode
 * - Visual feedback for mapped controls
 *
 * Usage:
 * 1. Inherit from this class instead of juce::Component
 * 2. Call setMidiControlId() in constructor
 * 3. Call setRegistry() to connect to PluginProcessor's registry
 * 4. Optionally override getMappableBounds() for custom overlay area
 */
class MidiMappableComponent : public juce::Component {
public:
    MidiMappableComponent() = default;

    ~MidiMappableComponent() override {
        // Unregister on destruction
        if (registry && controlId != MidiControlId::COUNT) {
            registry->unregisterControl(controlId);
        }
    }

    /**
     * Set the MIDI control ID for this component
     * Call this in your component's constructor
     */
    void setMidiControlId(MidiControlId id) {
        controlId = id;
        updateRegistration();
    }

    /**
     * Set the registry reference
     * Called by parent component (e.g., ApplicationComponent)
     */
    void setRegistry(MidiLearnRegistry* reg) {
        registry = reg;
        updateRegistration();
    }

    /**
     * Get the bounds that should be used for the MIDI learn overlay
     * Override this if you want the overlay to cover a specific sub-area
     */
    virtual juce::Rectangle<int> getMappableBounds() const {
        return getLocalBounds();
    }

    /**
     * Get display name for this control
     * Override to customize
     */
    virtual juce::String getDisplayName() const {
        return getMidiControlName(controlId);
    }

    /**
     * Paint the MIDI learn overlay on top of the component
     * Called automatically when in learn mode
     */
    void paintMidiLearnOverlay(juce::Graphics& g) {
        if (!registry || !registry->isLearning()) return;
        if (controlId == MidiControlId::COUNT) return;

        auto bounds = getMappableBounds();

        // Determine if this control is currently being learned
        bool isLearningThisControl = (registry->getLearningControl() == controlId);

        // Semi-transparent blue rectangle (40% opacity)
        auto overlayColor = juce::Colour(0x6680CCFF);  // Blue with ~40% opacity (was 25%)
        g.setColour(overlayColor);
        g.fillRect(bounds);

        // Yellow/gold border if actively learning (thick stroke)
        if (isLearningThisControl) {
            g.setColour(juce::Colour(0xFFFFD700));  // Yellow/gold
            g.drawRect(bounds, 4);  // 4px stroke (was 3px)
        }

        // Display MIDI mapping info (if mapped) in yellow
        const auto* midiId = registry->getMidiForControl(controlId);
        if (midiId) {
            g.setColour(juce::Colour(0xFFFFD700));  // Yellow/gold (was white)
            g.setFont(9.0f);
            g.drawText(midiId->toString(),
                      bounds,
                      juce::Justification::centred);
        }
    }

    /**
     * Handle click on overlay during MIDI learn mode
     * Returns true if click was handled
     */
    bool handleMidiLearnClick(const juce::MouseEvent& e) {
        if (!registry || !registry->isLearning()) return false;
        if (controlId == MidiControlId::COUNT) return false;

        auto bounds = getMappableBounds();
        if (bounds.contains(e.getPosition())) {
            // Set this control as the learning target
            registry->setLearningControl(controlId);
            repaint();
            return true;
        }

        return false;
    }

protected:
    /**
     * Call this from resized() to update bounds in registry
     */
    void updateMidiBounds() {
        if (registry && controlId != MidiControlId::COUNT) {
            auto screenBounds = getScreenBounds();
            auto mappableBounds = getMappableBounds();
            auto absoluteBounds = mappableBounds + screenBounds.getPosition();
            registry->updateControlBounds(controlId, absoluteBounds);
        }
    }

private:
    void updateRegistration() {
        if (registry && controlId != MidiControlId::COUNT && isShowing()) {
            auto screenBounds = getScreenBounds();
            auto mappableBounds = getMappableBounds();
            auto absoluteBounds = mappableBounds + screenBounds.getPosition();
            registry->registerControl(controlId, absoluteBounds, getDisplayName());
        }
    }

    MidiLearnRegistry* registry = nullptr;
    MidiControlId controlId = MidiControlId::COUNT;
};
