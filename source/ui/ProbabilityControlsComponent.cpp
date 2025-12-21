#include "ProbabilityControlsComponent.h"
#include "../PluginProcessor.h"
#include "../backend/ProcessorCommand.h"

ProbabilityControlsComponent::ProbabilityControlsComponent(PluginProcessor& processor)
    : processorRef(processor)
    , probabilitySlider("Probability")
{
    addAndMakeVisible(probabilitySlider);
    addAndMakeVisible(probabilityGauge);

    // Set up callback for slider changes
    // This triggers randomization in the audio thread and updates the gauge via completion callback
    probabilitySlider.onValueChange = [this](float newValue) {
        // Convert 0.0-1.0 to 0-100 percentage
        int percentage = static_cast<int>(std::round(newValue * 100.0f));

        // Queue command to randomize probability array in audio thread
        processorRef.queueCommand(ProcessorCommand(
            // Command: runs on audio thread
            [this, percentage]() {
                processorRef.getSequence().setProbabilityPercentage(percentage);
                DBG("Probability randomized: " << percentage << "%");
            },
            // Completion callback: runs on message thread after command executes
            [this]() {
                // Read the randomized probability array from the backend
                auto& sequence = processorRef.getSequence();
                std::array<bool, 64> probabilityArray;
                for (int i = 0; i < 64; ++i) {
                    probabilityArray[i] = sequence.getPlayProbability(i);
                }

                // Update the gauge with the backend data
                probabilityGauge.updateFromProbabilityArray(probabilityArray);
                DBG("Probability gauge updated from backend");
            }
        ));
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

    // Center the slider within the left half (use full height of left half)
    auto sliderArea = leftHalf.withSizeKeepingCentre(leftHalf.getWidth(), leftHalf.getHeight());
    probabilitySlider.setBounds(sliderArea);

    // Center the gauge within the right half
    auto gaugeArea = rightHalf.withSizeKeepingCentre(gaugeSize, gaugeSize);
    probabilityGauge.setBounds(gaugeArea);
}

