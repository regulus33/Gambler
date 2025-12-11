#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

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
        : circleStates (maxCircles, false)
        , rng (std::random_device{}())
    {
    }

    ~ProbabilityGauge() override = default;

    void paint (juce::Graphics& g) override {
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
     * Update the gauge display based on a probability value.
     * @param probability Value between 0.0 and 1.0
     * Randomly selects which circles are active based on the probability.
     */
    void setProbability (float probability) {
        // Clamp probability to valid range
        currentProbability = juce::jlimit (0.0f, 1.0f, probability);
        randomizeActiveCircles();
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
        totalCircles = juce::jlimit (1, maxCircles, count);
        // Re-randomize to ensure active count is correct for new total
        randomizeActiveCircles();
        repaint();
    }

    /**
     * Get the current probability value.
     */
    float getProbability() const { return currentProbability; }

    /**
     * Get the current step count (visible circles).
     */
    int getStepCount() const { return totalCircles; }

private:
    static constexpr int gridSize = 8;           // 8x8 grid
    static constexpr int maxCircles = 64;        // Maximum number of circles
    static constexpr int circleDiameter = 4;     // Each circle is 5x5 pixels (scaled 2.2x from 4)
    static constexpr int circleSpacing = 3;      // 2px gap between circles (scaled 2.2x from 1)

    // Colors
    const juce::Colour activeColour { 0xFFFF7C7C };   // Red/active
    const juce::Colour inactiveColour { 0xFF333333 }; // Dark gray/inactive

    float currentProbability = 0.0f;

    // Total visible circles - can be changed for variable step count
    // NOTE: Change this value to test different step counts
    int totalCircles = maxCircles;

    // Tracks which circles are currently active (true = active/red)
    std::vector<bool> circleStates;

    // Random number generator for shuffling
    std::mt19937 rng;

    /**
     * Randomly select which circles should be active based on the current probability.
     */
    void randomizeActiveCircles() {
        // Reset all circles to inactive
        std::fill (circleStates.begin(), circleStates.end(), false);

        // Calculate how many circles should be active
        const int activeCount = static_cast<int> (std::round (currentProbability * static_cast<float> (totalCircles)));

        if (activeCount <= 0)
            return;

        // Create a list of indices for visible circles only
        std::vector<int> indices (static_cast<size_t> (totalCircles));
        std::iota (indices.begin(), indices.end(), 0);

        // Shuffle the indices
        std::shuffle (indices.begin(), indices.end(), rng);

        // Mark the first 'activeCount' shuffled indices as active
        for (int i = 0; i < activeCount && i < totalCircles; ++i)
        {
            circleStates[static_cast<size_t> (indices[static_cast<size_t> (i)])] = true;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbabilityGauge)
};

