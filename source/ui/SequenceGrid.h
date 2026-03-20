#pragma once

#include "../backend/SequenceState.h"
#include "UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
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
    constexpr juce::uint32 defaultDarkGreyHex = 0xff808080;  // Dark grey for rest steps
    constexpr juce::uint32 outOfBoundsGreyHex = 0xff3B3B3B;
    constexpr juce::uint32 dragPreviewOrangeHex = 0xFFFF8C00; // Orange color for drag preview
    // Window row opacity (for non-visible rows)
    constexpr float nonVisibleRowOpacity = 0.3f;  // Non-visible rows at 30% opacity
    constexpr float colourFadeSpeed = 8.0f;  // Lerp speed for colour transitions
    // Calculated dimensions
    constexpr int gridWidth = gridCols * squareSize + (gridCols - 1) * squareSpacing;   // 196px (was 157px)
    constexpr int gridHeight = gridRows * squareSize + (gridRows - 1) * squareSpacing;  // 196px (was 157px)
}

/**
 * SquareState - Represents the visual state of a single grid square
 */
struct SquareState {
    juce::Colour color { juce::Colour (SequenceGridConstants::defaultGreyHex) };
    bool hasBorder { false };      // Border indicates selected step for editing
    bool isCurrent { false };      // Current step (playback position) - white fill

    SquareState() = default;
    SquareState (juce::Colour c, bool border = false, bool current = false)
        : color (c), hasBorder (border), isCurrent (current) {}
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
        auto defaultColour = juce::Colour(SequenceGridConstants::defaultGreyHex);
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row) {
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col) {
                squareStates[row][col] = SquareState();
                displayColours_[row][col] = defaultColour;
            }
        }
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceGridConstants;

        bool stillAnimating = false;

        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                const auto& state = squareStates[row][col];
                int stepIndex = row * gridCols + col;

                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<float> (
                    static_cast<float> (x),
                    static_cast<float> (y),
                    static_cast<float> (squareSize),
                    static_cast<float> (squareSize));

                // Determine the target fill color
                juce::Colour targetColor;
                if (state.isCurrent) {
                    targetColor = UIConstants::activeStepColour;
                } else {
                    targetColor = state.color;

                    if (currentWindowRow >= 0 && row != currentWindowRow) {
                        targetColor = targetColor.withAlpha(nonVisibleRowOpacity);
                    }

                    if (isDraggingLength) {
                        if (stepIndex < dragTargetLength) {
                            targetColor = juce::Colour(dragPreviewOrangeHex);
                        } else {
                            targetColor = juce::Colour(outOfBoundsGreyHex);
                        }
                    }
                }

                // Animate display colour toward target
                auto& dc = displayColours_[row][col];
                if (dc != targetColor) {
                    float t = juce::jmin(1.0f, colourFadeSpeed * frameDeltaSec_);
                    dc = dc.interpolatedWith(targetColor, t);
                    if (dc != targetColor)
                        stillAnimating = true;
                }

                // Fill square with animated color
                g.setColour (dc);
                g.fillRoundedRectangle (squareBounds, squareCornerRadius);

                // Draw white border if enabled (selected step for editing)
                if (state.hasBorder) {
                    g.setColour (UIConstants::selectedStepOutlineColour);
                    g.drawRoundedRectangle (squareBounds.reduced (0.5f), squareCornerRadius, borderWidth);
                }

                // During drag: highlight the target square (last step in new length)
                if (isDraggingLength && stepIndex == dragTargetLength - 1) {
                    g.setColour (UIConstants::stepIndicatorWhite.withAlpha(0.5f));
                    g.drawRoundedRectangle (squareBounds.reduced (1.0f), squareCornerRadius, 1.5f);
                }
            }
        }

        needsAnimationRepaint_ = stillAnimating;
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr && vBlankAttachment_ == nullptr) {
            vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(
                this, [this](double timestampSec) {
                    double delta = (lastVBlankTimestamp_ > 0.0)
                        ? (timestampSec - lastVBlankTimestamp_)
                        : (1.0 / 60.0);
                    lastVBlankTimestamp_ = timestampSec;
                    frameDeltaSec_ = static_cast<float>(juce::jlimit(0.001, 0.1, delta));

                    if (needsAnimationRepaint_) {
                        needsAnimationRepaint_ = false;
                        repaint();
                    }
                });
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
     * Update the grid from a SequenceState object (APVTS-backed)
     * - Each square shows the note color for that step
     * - Steps outside the sequence length are shown in grey
     * - The current playback step has a white fill
     * - The selected step for editing has a white border
     * @param state The SequenceState to display
     * @param playbackStep The current playback position (-1 if not playing)
     */
    void updateFromState (SequenceState& state, int playbackStep = -1) {
        using namespace SequenceGridConstants;
        stateRef = &state;

        int sequenceLength = state.getLength();
        int selectedStep = state.getSelectedStepForEditing();

        for (int stepIndex = 0; stepIndex < 64; ++stepIndex) {
            int row = stepIndex / gridCols;  // 0-7
            int col = stepIndex % gridCols;  // 0-7

            if (stepIndex < sequenceLength) {
                // Step is within bounds
                // Get the quantized note (accounts for scale quantization)
                int midiNote = state.getCurrentNoteForStep(stepIndex);

                // Check if quantized note is a rest (-1)
                // This happens when: 1) stored note is a rest, or 2) no scale notes are selected
                if (midiNote < 0) {
                    // Step is a rest or quantized to rest - show grey
                    squareStates[row][col].color = juce::Colour (defaultGreyHex);
                } else {
                    // Normal step - show note color
                    int chromaticNote = midiNote % 12;
                    squareStates[row][col].color = getNoteColor (chromaticNote);
                }
            } else {
                // Step is outside bounds - show darker grey
                squareStates[row][col].color = juce::Colour (outOfBoundsGreyHex);
            }

            // Set border for selected step (for editing)
            squareStates[row][col].hasBorder = (stepIndex == selectedStep);

            // Set current state for playback position (white fill)
            squareStates[row][col].isCurrent = (stepIndex == playbackStep);
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

        // Store initial mouse position and reset drag state
        mouseDownPosition = pos;
        isDraggingLength = false;
        mouseDownStepIndex = -1;

        // Calculate which square was clicked
        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<int>(x, y, squareSize, squareSize);

                if (squareBounds.contains(pos)) {
                    // Calculate step index (0-63)
                    mouseDownStepIndex = row * gridCols + col;
                    return;
                }
            }
        }
    }

    /**
     * Mouse drag handler - changes sequence length by dragging to different squares
     */
    void mouseDrag(const juce::MouseEvent& event) override {
        using namespace SequenceGridConstants;

        auto pos = event.getPosition();

        // Find which square we're currently hovering over
        int hoveredStepIndex = getSquareIndexAtPosition(pos);

        // If we're over a valid square and it's different from where we started, enter drag mode
        if (hoveredStepIndex >= 0 && hoveredStepIndex != mouseDownStepIndex) {
            isDraggingLength = true;
            dragTargetLength = hoveredStepIndex + 1; // Length is index + 1
            repaint(); // Show visual feedback
        }
    }

    /**
     * Mouse up handler - finalizes length change or triggers click
     */
    void mouseUp(const juce::MouseEvent& event) override {
        if (isDraggingLength) {
            // We were dragging - finalize the length change
            if (onLengthDragged && dragTargetLength > 0) {
                onLengthDragged(dragTargetLength);
            }
            isDraggingLength = false;
            repaint();
        } else if (mouseDownStepIndex >= 0) {
            // No drag occurred - treat as a click
            if (onSquareClicked) {
                onSquareClicked(mouseDownStepIndex);
            }
        }

        mouseDownStepIndex = -1;
    }

    /**
     * Mouse double-click handler - detects which square was double-clicked
     */
    void mouseDoubleClick(const juce::MouseEvent& event) override {
        using namespace SequenceGridConstants;

        auto pos = event.getPosition();

        // Calculate which square was double-clicked
        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<int>(x, y, squareSize, squareSize);

                if (squareBounds.contains(pos)) {
                    // Calculate step index (0-63)
                    int stepIndex = row * gridCols + col;

                    // Trigger callback if set
                    if (onSquareDoubleClicked) {
                        onSquareDoubleClicked(stepIndex);
                    }
                    return;
                }
            }
        }
    }

    // Callback for when a square is clicked
    std::function<void(int)> onSquareClicked;

    // Callback for when a square is double-clicked (to force jump to that step)
    std::function<void(int)> onSquareDoubleClicked;

    // Callback for when sequence length is changed by dragging
    std::function<void(int)> onLengthDragged;

private:
    std::array<std::array<SquareState, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> squareStates;

    int currentWindowRow;  // Current visible window row (0-7), or -1 if disabled

    // Drag state for length adjustment
    juce::Point<int> mouseDownPosition;
    int mouseDownStepIndex = -1;
    bool isDraggingLength = false;
    int dragTargetLength = 0;

    /**
     * Helper method to get the step index at a given position
     * Returns -1 if position is not over any square
     */
    int getSquareIndexAtPosition(const juce::Point<int>& pos) const {
        using namespace SequenceGridConstants;

        for (int row = 0; row < gridRows; ++row) {
            for (int col = 0; col < gridCols; ++col) {
                int x = col * (squareSize + squareSpacing);
                int y = row * (squareSize + squareSpacing);

                auto squareBounds = juce::Rectangle<int>(x, y, squareSize, squareSize);

                if (squareBounds.contains(pos)) {
                    return row * gridCols + col;
                }
            }
        }
        return -1;
    }

    /**
     * Get the color for a chromatic note (0-11)
     * Reads from SequenceState if available, otherwise uses hardcoded defaults
     */
    juce::Colour getNoteColor (int chromaticNote) const {
        if (stateRef != nullptr) {
            return stateRef->getNoteColour(chromaticNote);
        }
        // Fallback to hardcoded defaults (shouldn't normally be reached)
        const juce::Colour noteColors[12] = {
            juce::Colour (0xFFFF6B6B), juce::Colour (0xFFFF9F6B),
            juce::Colour (0xFFFFD66B), juce::Colour (0xFFFFE66B),
            juce::Colour (0xFFE6FF6B), juce::Colour (0xFF6BFFB4),
            juce::Colour (0xFF6BFFD6), juce::Colour (0xFF6BD6FF),
            juce::Colour (0xFF6BB4FF), juce::Colour (0xFF9F6BFF),
            juce::Colour (0xFFD66BFF), juce::Colour (0xFFFF6BD6)
        };
        return noteColors[chromaticNote % 12];
    }

    SequenceState* stateRef = nullptr;

    // Colour fade animation state
    std::array<std::array<juce::Colour, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> displayColours_;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;
};

