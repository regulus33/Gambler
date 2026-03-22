#include "ProbabilityControlsComponent.h"
#include "../PluginProcessor.h"

ProbabilityControlsComponent::ProbabilityControlsComponent(PluginProcessor& processor)
    : StateListenerComponent(processor)
    , probabilitySlider("Probability")
{
    setPaintingIsUnclipped(true);
    addAndMakeVisible(probabilitySlider);
    addAndMakeVisible(probabilityGauge);

    // Subscribe to all play probability parameters
    listenToPlayProbability();

    // Initialize UI from APVTS-backed state (thread-safe)
    updateUIFromState();

    // Set up callback for slider changes
    probabilitySlider.onValueChange = [this](float newValue) {
        // Prevent feedback loop when updating from state
        if (isUpdatingFromState) {
            return;
        }

        // Convert 0.0-1.0 to 0-100 percentage
        int percentage = static_cast<int>(std::round(newValue * 100.0f));

        // Update via APVTS-backed state (thread-safe)
        auto& state = getState();
        state.setProbabilityPercentage(percentage);
        DBG("Probability randomized: " << percentage << "%");

        // Read back the randomized probability array and update gauge
        std::array<bool, 64> probabilityArray;
        for (int i = 0; i < 64; ++i) {
            probabilityArray[i] = state.getPlayProbability(i);
        }
        probabilityGauge.updateFromProbabilityArray(probabilityArray);
        DBG("Probability gauge updated from backend");
    };
}

void ProbabilityControlsComponent::paint(juce::Graphics& g) {
    using namespace ProbabilityControlsConstants;
    g.fillAll(juce::Colours::transparentBlack);

    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(r.reduced(0.5f), cornerRadius, borderStrokeWidth);
}

void ProbabilityControlsComponent::resized() {
    using namespace ProbabilityControlsConstants;

    auto bounds = getLocalBounds();
    const int halfWidth = bounds.getWidth() / 2;

    // Split into two equal halves
    auto leftHalf = bounds.removeFromLeft(halfWidth);
    auto rightHalf = bounds;

    // Center the slider within the left half
    auto sliderArea = leftHalf.withSizeKeepingCentre(leftHalf.getWidth(), leftHalf.getHeight());
    probabilitySlider.setBounds(sliderArea);

    // Gauge fills right half with 5px padding, then scaled down
    constexpr int gaugePadding = 5;
    auto gaugeArea = rightHalf.reduced(gaugePadding);
    int side = static_cast<int>(std::min(gaugeArea.getWidth(), gaugeArea.getHeight()) * gaugeScale);
    probabilityGauge.setBounds(gaugeArea.withSizeKeepingCentre(side, side));
}

void ProbabilityControlsComponent::onStateChanged() {
    // Update UI from state when any probability parameter changes
    updateUIFromState();
}

void ProbabilityControlsComponent::updateUIFromState() {
    // Set flag to prevent feedback loop
    isUpdatingFromState = true;

    auto& state = getState();
    int trueCount = 0;
    std::array<bool, 64> probabilityArray;
    for (int i = 0; i < 64; ++i) {
        probabilityArray[i] = state.getPlayProbability(i);
        if (probabilityArray[i]) {
            trueCount++;
        }
    }

    // Calculate percentage and set slider (without triggering callback)
    float percentage = static_cast<float>(trueCount) / 64.0f;
    probabilitySlider.setValue(percentage, juce::dontSendNotification);

    // Update gauge to show current state
    probabilityGauge.updateFromProbabilityArray(probabilityArray);

    // Clear flag
    isUpdatingFromState = false;
}

