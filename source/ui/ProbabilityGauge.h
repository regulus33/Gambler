#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <functional>
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

        const auto bounds = getLocalBounds();
        const float boundsW = static_cast<float>(bounds.getWidth());
        const float boundsH = static_cast<float>(bounds.getHeight());
        const float cellW = boundsW / static_cast<float>(currentGridDim);
        const float cellH = boundsH / static_cast<float>(currentGridDim);
        const float cellSize = std::min(cellW, cellH);
        const float side = cellSize * 0.6f;
        const float padding = (cellSize - side) * 0.5f;

        const float totalGridSize = cellSize * static_cast<float>(currentGridDim);
        const float offsetX = (boundsW - totalGridSize) * 0.5f;
        const float offsetY = (boundsH - totalGridSize) * 0.5f;

        const int blockSize = gridSize / currentGridDim;

        // The walker stores a 0..(D*D - 1) display-cell index — one tick per
        // visible cell — so the highlight is a direct lookup.
        const int displayCells = currentGridDim * currentGridDim;
        int highlightedDisplayIndex = -1;
        if (highlightedCircleIndex >= 0 && highlightedCircleIndex < displayCells)
            highlightedDisplayIndex = highlightedCircleIndex;

        for (int row = 0; row < currentGridDim; ++row) {
            for (int col = 0; col < currentGridDim; ++col) {
                const int displayIndex = row * currentGridDim + col;
                // Map display cell to top-left underlying step in its block.
                const int underlyingIndex =
                    (row * blockSize) * gridSize + (col * blockSize);

                const float x = offsetX + static_cast<float>(col) * cellSize + padding;
                const float y = offsetY + static_cast<float>(row) * cellSize + padding;

                const bool isHighlighted = (displayIndex == highlightedDisplayIndex);
                const bool isActive = circleStates[static_cast<std::size_t>(underlyingIndex)];

                if (isHighlighted)
                    g.setColour(isActive ? highlightPlayColour : highlightRestColour);
                else
                    g.setColour(isActive ? activeColour : inactiveColour);

                g.fillRect(x, y, side, side);
            }
        }
    }

    void resized() override {
        // The component size is fixed based on the grid dimensions
        // No dynamic resizing needed - parent should allocate appropriate space
    }

    void mouseDown(const juce::MouseEvent& e) override {
        int row = 0, col = 0;
        if (! hitTestCell(e.x, e.y, row, col)) return;

        // Paint state for this drag stroke = inverse of the cell we started on.
        const int blockSize = ProbabilityGaugeConstants::gridSize / currentGridDim;
        const int underlyingIdx =
            (row * blockSize) * ProbabilityGaugeConstants::gridSize + (col * blockSize);
        dragPaintActive_ = ! circleStates[static_cast<std::size_t>(underlyingIdx)];
        lastPaintedRow_ = row;
        lastPaintedCol_ = col;

        if (onCellPainted) onCellPainted(row, col, dragPaintActive_);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        int row = 0, col = 0;
        if (! hitTestCell(e.x, e.y, row, col)) return;
        if (row == lastPaintedRow_ && col == lastPaintedCol_) return;

        lastPaintedRow_ = row;
        lastPaintedCol_ = col;
        if (onCellPainted) onCellPainted(row, col, dragPaintActive_);
    }

    // Fired when the user clicks or drags over a display cell. Coordinates are
    // in the current display grid (0..currentGridDim-1). `active` is the state
    // to apply (set on mouseDown as the inverse of the starting cell, then held
    // for the rest of the drag stroke).
    std::function<void(int row, int col, bool active)> onCellPainted;

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
     * Set the display grid dimension (2, 4, or 8). The component size stays
     * the same — each visual cell grows as the dimension shrinks.
     */
    void setGridDim (int dim) {
        if (dim != 2 && dim != 4 && dim != 8) return;
        currentGridDim = dim;
        repaint();
    }

    int getGridDim() const { return currentGridDim; }

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
    bool hitTestCell(int mouseX, int mouseY, int& rowOut, int& colOut) const {
        const auto bounds = getLocalBounds();
        const float boundsW = static_cast<float>(bounds.getWidth());
        const float boundsH = static_cast<float>(bounds.getHeight());
        const float cellW = boundsW / static_cast<float>(currentGridDim);
        const float cellH = boundsH / static_cast<float>(currentGridDim);
        const float cellSize = std::min(cellW, cellH);
        const float totalGridSize = cellSize * static_cast<float>(currentGridDim);
        const float offsetX = (boundsW - totalGridSize) * 0.5f;
        const float offsetY = (boundsH - totalGridSize) * 0.5f;

        const float relX = static_cast<float>(mouseX) - offsetX;
        const float relY = static_cast<float>(mouseY) - offsetY;
        if (relX < 0.0f || relY < 0.0f || relX >= totalGridSize || relY >= totalGridSize)
            return false;

        const int col = static_cast<int>(relX / cellSize);
        const int row = static_cast<int>(relY / cellSize);
        if (col < 0 || col >= currentGridDim || row < 0 || row >= currentGridDim) return false;

        rowOut = row;
        colOut = col;
        return true;
    }

    // Drag-paint state (set on mouseDown, applied throughout the drag stroke)
    bool dragPaintActive_ = false;
    int lastPaintedRow_ = -1;
    int lastPaintedCol_ = -1;

    // Colors
    const juce::Colour activeColour { ProbabilityGaugeConstants::activeColourHex };
    const juce::Colour inactiveColour { ProbabilityGaugeConstants::inactiveColourHex };
    const juce::Colour highlightPlayColour { ProbabilityGaugeConstants::highlightPlayColourHex };
    const juce::Colour highlightRestColour { ProbabilityGaugeConstants::highlightRestColourHex };

    // Total visible circles - can be changed for variable step count
    int totalCircles = ProbabilityGaugeConstants::maxCircles;

    // Display grid dimension (edge length in cells). 8 = full 8x8, 4 = 4x4, 2 = 2x2.
    int currentGridDim = ProbabilityGaugeConstants::gridSize;

    // Tracks which circles are currently active (true = active/red)
    // This is updated from the backend via updateFromProbabilityArray()
    std::vector<bool> circleStates;

    // Tracks which circle is currently highlighted (-1 = none)
    // When roll is engaged, this shows which circle was randomly selected on the last tick
    int highlightedCircleIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbabilityGauge)
};

