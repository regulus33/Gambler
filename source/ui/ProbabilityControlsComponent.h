#pragma once

#include "AnalogueSlider.h"
#include "ProbabilityGauge.h"
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
 * Thread-safe communication:
 * - Slider changes update probability via APVTS-backed SequenceState (thread-safe)
 * - Gauge is updated immediately after state change
 */
class ProbabilityControlsComponent : public juce::Component {
public:
    explicit ProbabilityControlsComponent(PluginProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;

    AnalogueSlider& getProbabilitySlider() { return probabilitySlider; }
    const AnalogueSlider& getProbabilitySlider() const { return probabilitySlider; }

    ProbabilityGauge& getProbabilityGauge() { return probabilityGauge; }
    const ProbabilityGauge& getProbabilityGauge() const { return probabilityGauge; }

private:
    PluginProcessor& processorRef;
    AnalogueSlider probabilitySlider;
    ProbabilityGauge probabilityGauge;
};

