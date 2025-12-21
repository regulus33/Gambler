#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <vector>

namespace ProbabilityGaugeConstants {
    constexpr int gridSize = 8;
    constexpr int maxCircles = 64;
    constexpr int circleDiameter = 4;
    constexpr int circleSpacing = 3;
    // Colors
    constexpr juce::uint32 activeColourHex = 0xFFFF7C7C;
    constexpr juce::uint32 inactiveColourHex = 0xFF333333;
}

/**
 * ProbabilityGauge
 *
 * A visual component that displays probability as a grid of colored circles.
 * - 8x8 grid (64 circles total)
 * - Each circle is 4x4 pixels
 * - Active circles: #FF7C7C (red)
 * - Inactive circles: #333333 (dark gray)
 * - Grid reads left-to-right, top-to-bottom (position 1 = top-left, position 64 = bottom-right)
 *
 * When probability changes, randomly selects which circles are active.
 */
class ProbabilityGauge : public juce::Component {
public:
    ProbabilityGauge()
        : circleStates (ProbabilityGaugeConstants::maxCircles, false)
    {
    }

    ~ProbabilityGauge() override = default;

    void paint (juce::Graphics& g) override {
        using namespace ProbabilityGaugeConstants;
        const int cellSize = circleDiameter + circleSpacing;

        for (int i = 0; i < maxCircles; ++i)
        {
            // Calculate grid position (left-to-right, top-to-bottom)
            const int col = i % gridSize;
            const int row = i / gridSize;

            const int x = col * cellSize;
            const int y = row * cellSize;

            // Determine if this circle should be visible
            const bool isVisible = (i < totalCircles);

            if (isVisible)
            {
                // Determine color based on active state
                const bool isActive = circleStates[static_cast<size_t> (i)];
                g.setColour (isActive ? activeColour : inactiveColour);
                g.fillEllipse (static_cast<float> (x),
                               static_cast<float> (y),
                               static_cast<float> (circleDiameter),
                               static_cast<float> (circleDiameter));
            }
            // If not visible, don't draw anything (circle is hidden)
        }
    }

    void resized() override {
        // The component size is fixed based on the grid dimensions
        // No dynamic resizing needed - parent should allocate appropriate space
    }

    /**
     * Update the gauge display from a boolean array (from backend).
     * This is the primary way to update the gauge - it receives data from the audio thread
     * via a completion callback after the backend has randomized the probability array.
     *
     * @param probabilityArray Array of 64 booleans (true = red/active, false = grey/inactive)
     */
    void updateFromProbabilityArray(const std::array<bool, ProbabilityGaugeConstants::maxCircles>& probabilityArray) {
        // Copy the array to our local state
        std::copy(probabilityArray.begin(), probabilityArray.end(), circleStates.begin());
        repaint();
    }

    /**
     * Set the number of visible circles (for variable step count support).
     * Circles are hidden from position 64 backwards.
     * @param count Number of circles to show (1-64)
     *
     * NOTE: To test different step counts, call this method with different values.
     * This is not yet wired to the actual sequencer step count.
     */
    void setStepCount (int count) {
        totalCircles = juce::jlimit (1, ProbabilityGaugeConstants::maxCircles, count);
        repaint();
    }

    /**
     * Get the current step count (visible circles).
     */
    int getStepCount() const { return totalCircles; }

private:
    // Colors
    const juce::Colour activeColour { ProbabilityGaugeConstants::activeColourHex };
    const juce::Colour inactiveColour { ProbabilityGaugeConstants::inactiveColourHex };

    // Total visible circles - can be changed for variable step count
    int totalCircles = ProbabilityGaugeConstants::maxCircles;

    // Tracks which circles are currently active (true = active/red)
    // This is updated from the backend via updateFromProbabilityArray()
    std::vector<bool> circleStates;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbabilityGauge)
};

