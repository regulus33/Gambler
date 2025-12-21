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
    constexpr juce::uint32 activeColourHex = 0xFFFF7C7C;      // Red - will play if selected
    constexpr juce::uint32 inactiveColourHex = 0xFF333333;    // Grey - will rest if selected
    constexpr juce::uint32 highlightPlayColourHex = 0xFFFFFF00;  // Yellow - selected and playing
    constexpr juce::uint32 highlightRestColourHex = 0xFFFFFFFF;  // White - selected but resting
}

/**
 * ProbabilityGauge
 *
 * A visual component that displays probability as a grid of colored circles.
 * - 8x8 grid (64 circles total)
 * - Each circle is 4x4 pixels
 * - Active circles: #FF7C7C (red) - will play if selected
 * - Inactive circles: #333333 (dark gray) - will rest if selected
 * - Highlighted play: #FFFF00 (yellow) - currently selected and playing
 * - Highlighted rest: #FFFFFF (white) - currently selected but resting
 * - Grid reads left-to-right, top-to-bottom (position 1 = top-left, position 64 = bottom-right)
 *
 * When probability changes, randomly selects which circles are active.
 * When roll is engaged, highlights the randomly selected circle on each tick.
 */
class ProbabilityGauge : public juce::Component {
public:
    ProbabilityGauge()
        : circleStates (ProbabilityGaugeConstants::maxCircles, false)
        , highlightedCircleIndex(-1)
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
                // Check if this circle is currently highlighted
                const bool isHighlighted = (i == highlightedCircleIndex);

                if (isHighlighted)
                {
                    // Highlighted circle: yellow if active (playing), white if inactive (resting)
                    const bool isActive = circleStates[static_cast<size_t> (i)];
                    g.setColour (isActive ? highlightPlayColour : highlightRestColour);
                }
                else
                {
                    // Normal state: red if active, grey if inactive
                    const bool isActive = circleStates[static_cast<size_t> (i)];
                    g.setColour (isActive ? activeColour : inactiveColour);
                }

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

    /**
     * Highlight a specific circle to show it was randomly selected.
     * The circle will be shown in yellow if it's active (playing) or white if inactive (resting).
     * Call with index = -1 to clear the highlight.
     *
     * @param index The circle index to highlight (0-63), or -1 to clear
     */
    void highlightCircle(int index) {
        highlightedCircleIndex = index;
        repaint();
    }

    /**
     * Clear the highlighted circle.
     */
    void clearHighlight() {
        highlightedCircleIndex = -1;
        repaint();
    }

private:
    // Colors
    const juce::Colour activeColour { ProbabilityGaugeConstants::activeColourHex };
    const juce::Colour inactiveColour { ProbabilityGaugeConstants::inactiveColourHex };
    const juce::Colour highlightPlayColour { ProbabilityGaugeConstants::highlightPlayColourHex };
    const juce::Colour highlightRestColour { ProbabilityGaugeConstants::highlightRestColourHex };

    // Total visible circles - can be changed for variable step count
    int totalCircles = ProbabilityGaugeConstants::maxCircles;

    // Tracks which circles are currently active (true = active/red)
    // This is updated from the backend via updateFromProbabilityArray()
    std::vector<bool> circleStates;

    // Tracks which circle is currently highlighted (-1 = none)
    // When roll is engaged, this shows which circle was randomly selected on the last tick
    int highlightedCircleIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbabilityGauge)
};

