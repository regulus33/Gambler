#pragma once

#include "ProbabilityGauge.h"
#include "StateListenerComponent.h"
#include "sliders/ProbabilitySlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declaration
class PluginProcessor;

namespace ProbabilityControlsConstants {
    constexpr float borderStrokeWidth = 2.5f;
    constexpr float cornerRadius = 3.0f;
    constexpr int gaugeSize = 56;
    constexpr int containerHeight = 94;  // Height of the component
}

/**
 * ProbabilityControlsComponent
 *
 * A bordered container for probability-related controls.
 * Displays a 1px white border with 3px corner radius around the controls.
 * Contains an AnalogueSlider (left) and ProbabilityGauge (right) in a 2-column layout.
 *
 * Observer Pattern:
 * - Listens to all play probability parameters
 * - Automatically updates slider and gauge when probability changes (e.g., from MIDI)
 *
 * Thread-safe communication:
 * - Slider changes update probability via APVTS-backed SequenceState (thread-safe)
 * - Gauge is updated immediately after state change
 */
class ProbabilityControlsComponent : public StateListenerComponent {
public:
    explicit ProbabilityControlsComponent(PluginProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;

    /**
     * Set the MIDI learn registry for the probability slider
     */
    void setRegistry(MidiLearnRegistry* registry) {
        probabilitySlider.setMidiLearnRegistry(registry);
    }

    /**
     * Set the callback for when the slider is selected for MIDI learning
     */
    void setMidiLearnSelectedCallback(std::function<void()> callback) {
        probabilitySlider.onMidiLearnSelected = callback;
    }

    AnalogueSlider& getProbabilitySlider() { return probabilitySlider; }
    const AnalogueSlider& getProbabilitySlider() const { return probabilitySlider; }

    ProbabilityGauge& getProbabilityGauge() { return probabilityGauge; }
    const ProbabilityGauge& getProbabilityGauge() const { return probabilityGauge; }

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the slider and gauge to reflect the current probability state.
     */
    void onStateChanged() override;

private:
    void updateUIFromState();

    AnalogueSlider probabilitySlider;
    ProbabilityGauge probabilityGauge;
    bool isUpdatingFromState = false;  // Prevent feedback loop
};

