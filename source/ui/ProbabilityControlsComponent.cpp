#include "ProbabilityControlsComponent.h"
#include "../PluginProcessor.h"
#include "Icons.h"

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
    , randomizeButton(&Icons::createDiceIcon, &Icons::createDiceIcon, false, false)
    , divideButton(&Icons::createDivideIcon, &Icons::createDivideIcon, false, false)
{
    setPaintingIsUnclipped(true);
    addAndMakeVisible(probabilitySlider);
    addAndMakeVisible(randomizeButton);
    addAndMakeVisible(divideButton);
    addAndMakeVisible(probabilityGauge);

    listenToPlayProbability();
    syncGaugeGridDim();
    updateUIFromState();

    probabilitySlider.onValueChange = [this](float newValue) {
        if (isUpdatingFromState) return;

        const int percentage = static_cast<int>(std::round(newValue * 100.0f));
        getState().setProbabilityPercentage(percentage);
        DBG("Probability randomized: " << percentage << "%");

        pushStateToGauge();
    };

    randomizeButton.onClick = [this](bool) {
        getState().randomizeProbabilityUniform();
        pushStateToGauge();
    };

    divideButton.onClick = [this](bool) {
        const int next = (getState().getProbabilityDivisionLevel() + 1) % 3;
        getState().setProbabilityDivisionLevel(next);
        syncGaugeGridDim();
        pushStateToGauge();
    };

    probabilityGauge.onCellPainted = [this](int row, int col, bool active) {
        auto& state = getState();
        const int dim = state.getProbabilityGridDim();
        const int blockSize = ProbabilityGaugeConstants::gridSize / dim;
        const int topLeftRow = row * blockSize;
        const int topLeftCol = col * blockSize;

        for (int dr = 0; dr < blockSize; ++dr) {
            for (int dc = 0; dc < blockSize; ++dc) {
                const int idx = (topLeftRow + dr) * ProbabilityGaugeConstants::gridSize
                              + (topLeftCol + dc);
                state.setPlayProbability(idx, active);
            }
        }
        updateUIFromState();
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

    auto bounds = getLocalBounds().reduced(innerPadding);
    const int totalW = bounds.getWidth();

    // 3-column layout: slider | buttons | gauge. Button column is narrow so the
    // slider keeps room for its "Probability" label.
    const int buttonColWidth = juce::jmin(totalW / 8, buttonColMaxWidth);
    const int sideWidth = (totalW - buttonColWidth) / 2;

    auto sliderArea = bounds.removeFromLeft(sideWidth);
    auto buttonCol  = bounds.removeFromLeft(buttonColWidth);
    auto gaugeArea  = bounds;

    probabilitySlider.setBounds(sliderArea.withSizeKeepingCentre(sliderArea.getWidth(), sliderArea.getHeight()));

    constexpr int gaugePadding = 5;
    auto paddedGauge = gaugeArea.reduced(gaugePadding);
    const int gaugeSide = static_cast<int>(std::min(paddedGauge.getWidth(), paddedGauge.getHeight()) * gaugeScale);
    auto gaugeBounds = paddedGauge.withSizeKeepingCentre(gaugeSide, gaugeSide);
    probabilityGauge.setBounds(gaugeBounds);

    // Buttons hug the gauge: dice top edge = gauge top, divide bottom = gauge bottom.
    const int rawSide = juce::jmin(buttonCol.getWidth(), buttonCol.getHeight() / 2);
    const int buttonSide = static_cast<int>(rawSide * buttonScale);
    const int btnX = buttonCol.getCentreX() - buttonSide / 2;
    randomizeButton.setBounds(btnX, gaugeBounds.getY(), buttonSide, buttonSide);
    divideButton.setBounds(btnX, gaugeBounds.getBottom() - buttonSide, buttonSide, buttonSide);
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

void ProbabilityControlsComponent::syncGaugeGridDim() {
    probabilityGauge.setGridDim(getState().getProbabilityGridDim());
}

void ProbabilityControlsComponent::updateUIFromState() {
    isUpdatingFromState = true;

    const auto cells = readProbabilityArray();
    const float percentage = static_cast<float>(countActiveSteps(cells)) / 64.0f;
    probabilitySlider.setValue(percentage, juce::dontSendNotification);
    probabilityGauge.updateFromProbabilityArray(cells);

    isUpdatingFromState = false;
}
