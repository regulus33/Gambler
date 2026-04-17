#include "ProbabilityControlsComponent.h"
#include "../PluginProcessor.h"

namespace {
    int countActiveSteps(const std::array<bool, 64>& cells) {
        int count = 0;
        for (bool on : cells) if (on) ++count;
        return count;
    }
}

ProbabilityControlsComponent::ProbabilityControlsComponent(PluginProcessor& processor)
    : StateListenerComponent(processor)
    , probabilitySlider("Probability")
{
    setPaintingIsUnclipped(true);
    addAndMakeVisible(probabilitySlider);
    addAndMakeVisible(probabilityGauge);

    listenToPlayProbability();
    updateUIFromState();

    probabilitySlider.onValueChange = [this](float newValue) {
        if (isUpdatingFromState) return;

        const int percentage = static_cast<int>(std::round(newValue * 100.0f));
        getState().setProbabilityPercentage(percentage);
        DBG("Probability randomized: " << percentage << "%");

        pushStateToGauge();
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

    auto leftHalf = bounds.removeFromLeft(halfWidth);
    auto rightHalf = bounds;

    auto sliderArea = leftHalf.withSizeKeepingCentre(leftHalf.getWidth(), leftHalf.getHeight());
    probabilitySlider.setBounds(sliderArea);

    constexpr int gaugePadding = 5;
    auto gaugeArea = rightHalf.reduced(gaugePadding);
    int side = static_cast<int>(std::min(gaugeArea.getWidth(), gaugeArea.getHeight()) * gaugeScale);
    probabilityGauge.setBounds(gaugeArea.withSizeKeepingCentre(side, side));
}

void ProbabilityControlsComponent::onStateChanged() {
    updateUIFromState();
}

std::array<bool, 64> ProbabilityControlsComponent::readProbabilityArray() const {
    std::array<bool, 64> cells{};
    auto& state = getState();
    for (int i = 0; i < 64; ++i)
        cells[static_cast<std::size_t>(i)] = state.getPlayProbability(i);
    return cells;
}

void ProbabilityControlsComponent::pushStateToGauge() {
    probabilityGauge.updateFromProbabilityArray(readProbabilityArray());
}

void ProbabilityControlsComponent::updateUIFromState() {
    isUpdatingFromState = true;

    const auto cells = readProbabilityArray();
    const float percentage = static_cast<float>(countActiveSteps(cells)) / 64.0f;
    probabilitySlider.setValue(percentage, juce::dontSendNotification);
    probabilityGauge.updateFromProbabilityArray(cells);

    isUpdatingFromState = false;
}
