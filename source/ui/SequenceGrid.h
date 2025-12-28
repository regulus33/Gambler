#pragma once

#include "../backend/Sequence.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace SequenceGridConstants {
    constexpr int gridRows = 8;
    constexpr int gridCols = 8;
    constexpr int squareSize = 21;  // 17 * 1.24 ≈ 21
    constexpr int squareSpacing = 4;  // 3 * 1.24 ≈ 3.72, rounded to 4
    constexpr float squareCornerRadius = 2.48f;  // 2.0 * 1.24
    constexpr float borderWidth = 2.48f;  // 2.0 * 1.24
    // Default colors
    constexpr juce::uint32 defaultGreyHex = 0xff808080;      // Grey for default squares
    constexpr juce::uint32 defaultDarkGreyHex = 0xff3a3a3a;  // Dark grey for rest steps
    constexpr juce::uint32 outOfBoundsGreyHex = 0xff252525;  // Even darker grey for out-of-bounds steps
    // Window row darkening (for non-visible rows)
    constexpr float nonVisibleRowDarkeningFactor = 0.4f;  // Darken non-visible rows to 40% brightness
    // Calculated dimensions
    constexpr int gridWidth = gridCols * squareSize + (gridCols - 1) * squareSpacing;   // 196px (was 157px)
    constexpr int gridHeight = gridRows * squareSize + (gridRows - 1) * squareSpacing;  // 196px (was 157px)
}

/**
 * SquareState - Represents the visual state of a single grid square
 */
struct SquareState {
    juce::Colour color { juce::Colour (SequenceGridConstants::defaultGreyHex) };
    bool hasBorder { false };

    SquareState() = default;
    SquareState (juce::Colour c, bool border = false) : color (c), hasBorder (border) {}
};

/**
 * SequenceGrid
 *
 * An 8x8 grid of colored squares representing the sequence overview.
 * Each square is 17x17px with 3px spacing and 2px border radius.
 *
 * The grid supports:
 * - Dynamic color updates for each square
 * - Optional white borders on individual squares
 * - High-level API for continuous updates during sequencer playback
 */
class SequenceGrid : public juce::Component {
public:
    SequenceGrid() : currentWindowRow(-1) {
        // Initialize all squares to default grey
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row) {
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col) {
                squareStates[row][col] = SquareState();
            }
        }
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceGridConstants;

        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                const auto& state = squareStates[row][col];

                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<float> (
                    static_cast<float> (x),
                    static_cast<float> (y),
                    static_cast<float> (squareSize),
                    static_cast<float> (squareSize));

                // Determine the color to use (darken if outside current window row)
                juce::Colour displayColor = state.color;
                if (currentWindowRow >= 0 && row != currentWindowRow) {
                    // Darken squares outside the current window row
                    displayColor = state.color.darker(nonVisibleRowDarkeningFactor);
                }

                // Fill square with color
                g.setColour (displayColor);
                g.fillRoundedRectangle (squareBounds, squareCornerRadius);

                // Draw white border if enabled
                if (state.hasBorder) {
                    g.setColour (juce::Colours::white);
                    g.drawRoundedRectangle (squareBounds.reduced (0.5f), squareCornerRadius, borderWidth);
                }
            }
        }
    }

    /**
     * Set the state of a single square
     * @param row Row index (0-7)
     * @param col Column index (0-7)
     * @param state The new state for the square
     */
    void setSquareState (int row, int col, const SquareState& state) {
        if (row >= 0 && row < SequenceGridConstants::gridRows &&
            col >= 0 && col < SequenceGridConstants::gridCols) {
            squareStates[row][col] = state;
            repaint();
        }
    }

    /**
     * Set the color of a single square (without changing border state)
     * @param row Row index (0-7)
     * @param col Column index (0-7)
     * @param color The new color
     */
    void setSquareColor (int row, int col, juce::Colour color) {
        if (row >= 0 && row < SequenceGridConstants::gridRows &&
            col >= 0 && col < SequenceGridConstants::gridCols) {
            squareStates[row][col].color = color;
            repaint();
        }
    }

    /**
     * Set the border visibility of a single square (without changing color)
     * @param row Row index (0-7)
     * @param col Column index (0-7)
     * @param hasBorder Whether to show the border
     */
    void setSquareBorder (int row, int col, bool hasBorder) {
        if (row >= 0 && row < SequenceGridConstants::gridRows &&
            col >= 0 && col < SequenceGridConstants::gridCols) {
            squareStates[row][col].hasBorder = hasBorder;
            repaint();
        }
    }

    /**
     * Set all square states at once (for efficient batch updates)
     * @param states 8x8 array of SquareState
     */
    void setAllSquareStates (const std::array<std::array<SquareState, SequenceGridConstants::gridCols>,
                                              SequenceGridConstants::gridRows>& states) {
        squareStates = states;
        repaint();
    }

    /**
     * Set all square colors at once (borders unchanged)
     * @param colors 8x8 array of colors
     */
    void setAllSquareColors (const std::array<std::array<juce::Colour, SequenceGridConstants::gridCols>,
                                              SequenceGridConstants::gridRows>& colors) {
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row) {
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col) {
                squareStates[row][col].color = colors[row][col];
            }
        }
        repaint();
    }

    /**
     * Set an entire row's states (useful for highlighting the current playback row)
     * @param row Row index (0-7)
     * @param states Array of 8 SquareState for the row
     */
    void setRowStates (int row, const std::array<SquareState, SequenceGridConstants::gridCols>& states) {
        if (row >= 0 && row < SequenceGridConstants::gridRows) {
            squareStates[row] = states;
            repaint();
        }
    }

    /**
     * Clear all borders (useful before setting a new active step)
     */
    void clearAllBorders() {
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row) {
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col) {
                squareStates[row][col].hasBorder = false;
            }
        }
        repaint();
    }

    /**
     * Get the state of a single square
     */
    const SquareState& getSquareState (int row, int col) const {
        static SquareState defaultState;
        if (row >= 0 && row < SequenceGridConstants::gridRows &&
            col >= 0 && col < SequenceGridConstants::gridCols) {
            return squareStates[row][col];
        }
        return defaultState;
    }

    /**
     * Get the preferred size for this component
     */
    static constexpr int getPreferredWidth() { return SequenceGridConstants::gridWidth; }
    static constexpr int getPreferredHeight() { return SequenceGridConstants::gridHeight; }

    /**
     * Update the grid from a Sequence object
     * - Each square shows the note color for that step
     * - Steps outside the sequence length are shown in grey
     * - The current step has a white border
     * @param sequence The sequence to display
     */
    void updateFromSequence (const Sequence& sequence) {
        using namespace SequenceGridConstants;

        int sequenceLength = sequence.getLength();
        int currentIndex = sequence.getCurrentIndex();

        for (int stepIndex = 0; stepIndex < 64; ++stepIndex) {
            int row = stepIndex / gridCols;  // 0-7
            int col = stepIndex % gridCols;  // 0-7

            if (stepIndex < sequenceLength) {
                // Step is within bounds
                const auto& step = sequence.getStep (stepIndex);

                // Check if this step's current note is a rest (-1)
                if (step.getStepNotes().hasRest()) {
                    // Step is a rest - show grey
                    squareStates[row][col].color = juce::Colour (defaultDarkGreyHex);
                } else {
                    // Normal step - show note color
                    int midiNote = step.getStepNotes().getCurrentNote().getNoteNumber();
                    int chromaticNote = midiNote % 12;
                    squareStates[row][col].color = getNoteColor (chromaticNote);
                }
            } else {
                // Step is outside bounds - show darker grey
                squareStates[row][col].color = juce::Colour (outOfBoundsGreyHex);
            }

            // Set border for current step only
            squareStates[row][col].hasBorder = (stepIndex == currentIndex);
        }

        repaint();
    }

    /**
     * Set the current window row (0-7) to highlight which row is visible in the sequencer
     * Rows outside this window will be darkened
     * @param row The row index (0-7), or -1 to disable window highlighting
     */
    void setCurrentWindowRow(int row) {
        if (row >= -1 && row < SequenceGridConstants::gridRows && currentWindowRow != row) {
            currentWindowRow = row;
            repaint();
        }
    }

    /**
     * Get the current window row
     */
    int getCurrentWindowRow() const {
        return currentWindowRow;
    }

    /**
     * Mouse click handler - detects which square was clicked
     */
    void mouseDown(const juce::MouseEvent& event) override {
        using namespace SequenceGridConstants;

        auto pos = event.getPosition();

        // Calculate which square was clicked
        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<int>(x, y, squareSize, squareSize);

                if (squareBounds.contains(pos)) {
                    // Calculate step index (0-63)
                    int stepIndex = row * gridCols + col;

                    // Trigger callback if set
                    if (onSquareClicked) {
                        onSquareClicked(stepIndex);
                    }
                    return;
                }
            }
        }
    }

    // Callback for when a square is clicked
    std::function<void(int)> onSquareClicked;

private:
    std::array<std::array<SquareState, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> squareStates;

    int currentWindowRow;  // Current visible window row (0-7), or -1 if disabled

    /**
     * Get the color for a chromatic note (0-11)
     * Same color mapping as SequencerStepsComponent
     */
    juce::Colour getNoteColor (int chromaticNote) const {
        const juce::Colour noteColors[12] = {
            juce::Colour (0xFFFF6B6B), // C - Red
            juce::Colour (0xFFFF9F6B), // Db - Orange
            juce::Colour (0xFFFFD66B), // D - Yellow
            juce::Colour (0xFFFFE66B), // Eb - Light yellow
            juce::Colour (0xFFE6FF6B), // E - Yellow-green
            juce::Colour (0xFF6BFFB4), // F - Light green
            juce::Colour (0xFF6BFFD6), // Gb - Cyan-green
            juce::Colour (0xFF6BD6FF), // G - Cyan
            juce::Colour (0xFF6BB4FF), // Ab - Light blue
            juce::Colour (0xFF9F6BFF), // A - Purple
            juce::Colour (0xFFD66BFF), // Bb - Magenta
            juce::Colour (0xFFFF6BD6)  // B - Pink
        };

        return noteColors[chromaticNote % 12];
    }
};

