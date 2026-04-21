#pragma once

#include "SequenceGridConstants.h"
#include "SquareState.h"
#include "../../backend/SequenceState.h"
#include "../UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <array>

/**
 * SequenceGrid
 *
 * An 8x8 grid of colored squares representing the sequence overview.
 *
 * The grid supports:
 * - Dynamic color updates for each square
 * - Optional white borders on individual squares
 * - High-level API for continuous updates during sequencer playback
 */
class SequenceGrid : public juce::Component {
public:
    SequenceGrid() : currentWindowRow(-1) {
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
                int stepIndex = row * gridCols + col;
                auto squareBounds = getSquareBounds(row, col).toFloat();

                juce::Colour targetColor = computeTargetColour(row, col, stepIndex);
                stillAnimating |= animateFill(row, col, targetColor, squareBounds, g);
                stillAnimating |= animateBorder(row, col, squareBounds, g);
                paintDragTargetHighlight(stepIndex, squareBounds, g);
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

    void setSquareState (int row, int col, const SquareState& state) {
        if (isValidPosition(row, col)) {
            squareStates[row][col] = state;
            repaint();
        }
    }

    void setSquareColor (int row, int col, juce::Colour color) {
        if (isValidPosition(row, col)) {
            squareStates[row][col].color = color;
            repaint();
        }
    }

    void setSquareBorder (int row, int col, bool hasBorder) {
        if (isValidPosition(row, col)) {
            squareStates[row][col].hasBorder = hasBorder;
            repaint();
        }
    }

    void setAllSquareStates (const std::array<std::array<SquareState, SequenceGridConstants::gridCols>,
                                              SequenceGridConstants::gridRows>& states) {
        squareStates = states;
        repaint();
    }

    void setAllSquareColors (const std::array<std::array<juce::Colour, SequenceGridConstants::gridCols>,
                                              SequenceGridConstants::gridRows>& colors) {
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row)
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col)
                squareStates[row][col].color = colors[row][col];
        repaint();
    }

    void setRowStates (int row, const std::array<SquareState, SequenceGridConstants::gridCols>& states) {
        if (row >= 0 && row < SequenceGridConstants::gridRows) {
            squareStates[row] = states;
            repaint();
        }
    }

    void clearAllBorders() {
        for (int row = 0; row < SequenceGridConstants::gridRows; ++row)
            for (int col = 0; col < SequenceGridConstants::gridCols; ++col)
                squareStates[row][col].hasBorder = false;
        repaint();
    }

    const SquareState& getSquareState (int row, int col) const {
        static SquareState defaultState;
        if (isValidPosition(row, col))
            return squareStates[row][col];
        return defaultState;
    }

    static constexpr int getPreferredWidth() { return SequenceGridConstants::gridWidth; }
    static constexpr int getPreferredHeight() { return SequenceGridConstants::gridHeight; }

    void updateFromState (SequenceState& state, int playbackStep = -1) {
        using namespace SequenceGridConstants;
        stateRef = &state;

        int sequenceLength = state.getLength();
        int selectedStep = state.getSelectedStepForEditing();

        for (int stepIndex = 0; stepIndex < 64; ++stepIndex) {
            int row = stepIndex / gridCols;
            int col = stepIndex % gridCols;

            squareStates[row][col].color = colourForStep(state, stepIndex, sequenceLength);
            squareStates[row][col].hasBorder = (stepIndex == selectedStep);
            squareStates[row][col].isCurrent = (stepIndex == playbackStep);
        }

        repaint();
    }

    void setCurrentWindowRow(int row) {
        if (row >= -1 && row < SequenceGridConstants::gridRows && currentWindowRow != row) {
            currentWindowRow = row;
            repaint();
        }
    }

    int getCurrentWindowRow() const { return currentWindowRow; }

    void mouseDown(const juce::MouseEvent& event) override {
        mouseDownPosition = event.getPosition();
        isDraggingLength = false;
        mouseDownStepIndex = getSquareIndexAtPosition(mouseDownPosition);
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        int hoveredStepIndex = getSquareIndexAtPosition(event.getPosition());

        if (hoveredStepIndex >= 0 && hoveredStepIndex != mouseDownStepIndex) {
            isDraggingLength = true;
            dragTargetLength = hoveredStepIndex + 1;
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override {
        if (isDraggingLength) {
            if (onLengthDragged && dragTargetLength > 0)
                onLengthDragged(dragTargetLength);
            isDraggingLength = false;
            repaint();
        } else if (mouseDownStepIndex >= 0 && onSquareClicked) {
            onSquareClicked(mouseDownStepIndex);
        }

        mouseDownStepIndex = -1;
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override {
        int stepIndex = getSquareIndexAtPosition(event.getPosition());
        if (stepIndex >= 0 && onSquareDoubleClicked)
            onSquareDoubleClicked(stepIndex);
    }

    std::function<void(int)> onSquareClicked;
    std::function<void(int)> onSquareDoubleClicked;
    std::function<void(int)> onLengthDragged;

private:
    static bool isValidPosition(int row, int col) {
        return row >= 0 && row < SequenceGridConstants::gridRows
            && col >= 0 && col < SequenceGridConstants::gridCols;
    }

    static juce::Rectangle<int> getSquareBounds(int row, int col) {
        using namespace SequenceGridConstants;
        int x = col * (squareSize + squareSpacing);
        int y = row * (squareSize + squareSpacing);
        return { x, y, squareSize, squareSize };
    }

    int getSquareIndexAtPosition(const juce::Point<int>& pos) const {
        using namespace SequenceGridConstants;
        for (int row = 0; row < gridRows; ++row)
            for (int col = 0; col < gridCols; ++col)
                if (getSquareBounds(row, col).contains(pos))
                    return row * gridCols + col;
        return -1;
    }

    juce::Colour computeTargetColour(int row, int col, int stepIndex) const {
        using namespace SequenceGridConstants;
        const auto& state = squareStates[row][col];

        if (state.isCurrent)
            return UIConstants::activeStepColour;

        juce::Colour target = state.color;

        if (currentWindowRow >= 0 && row != currentWindowRow)
            target = target.withAlpha(nonVisibleRowOpacity);

        if (isDraggingLength) {
            target = (stepIndex < dragTargetLength)
                ? juce::Colour(dragPreviewOrangeHex)
                : juce::Colour(outOfBoundsGreyHex);
        }

        return target;
    }

    bool animateFill(int row, int col, juce::Colour targetColor,
                     juce::Rectangle<float> bounds, juce::Graphics& g) {
        using namespace SequenceGridConstants;
        auto& dc = displayColours_[row][col];
        bool stillAnimating = false;
        if (dc != targetColor) {
            float t = juce::jmin(1.0f, colourFadeSpeed * frameDeltaSec_);
            dc = dc.interpolatedWith(targetColor, t);
            if (dc != targetColor)
                stillAnimating = true;
        }
        g.setColour(dc);
        g.fillRoundedRectangle(bounds, squareCornerRadius);
        return stillAnimating;
    }

    bool animateBorder(int row, int col, juce::Rectangle<float> bounds, juce::Graphics& g) {
        using namespace SequenceGridConstants;
        const auto& state = squareStates[row][col];
        float borderTarget = state.hasBorder ? borderWidth : 0.0f;
        float& bw = borderDisplayWidths_[row][col];
        bool stillAnimating = false;

        if (std::abs(bw - borderTarget) > 0.01f) {
            float duration = (borderTarget > bw)
                ? UIConstants::borderFadeInDuration
                : UIConstants::borderFadeOutDuration;
            float step = frameDeltaSec_ / duration;
            bw = (borderTarget > bw)
                ? juce::jmin(bw + step * borderWidth, borderTarget)
                : juce::jmax(bw - step * borderWidth, borderTarget);
            stillAnimating = true;
        } else {
            bw = borderTarget;
        }

        if (bw > 0.01f) {
            g.setColour(UIConstants::selectedStepOutlineColour);
            g.drawRoundedRectangle(bounds.reduced(0.5f), squareCornerRadius, bw);
        }
        return stillAnimating;
    }

    void paintDragTargetHighlight(int stepIndex, juce::Rectangle<float> bounds, juce::Graphics& g) const {
        using namespace SequenceGridConstants;
        if (isDraggingLength && stepIndex == dragTargetLength - 1) {
            g.setColour(UIConstants::stepIndicatorWhite.withAlpha(0.5f));
            g.drawRoundedRectangle(bounds.reduced(1.0f), squareCornerRadius, 1.5f);
        }
    }

    juce::Colour colourForStep(SequenceState& state, int stepIndex, int sequenceLength) const {
        using namespace SequenceGridConstants;
        if (stepIndex >= sequenceLength)
            return juce::Colour(outOfBoundsGreyHex);

        int midiNote = state.getCurrentNoteForStep(stepIndex);
        if (midiNote < 0)
            return juce::Colour(defaultGreyHex);

        return getNoteColor(midiNote % 12);
    }

    juce::Colour getNoteColor (int chromaticNote) const {
        if (stateRef != nullptr)
            return stateRef->getNoteColour(chromaticNote);
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

    std::array<std::array<SquareState, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> squareStates;

    int currentWindowRow;

    juce::Point<int> mouseDownPosition;
    int mouseDownStepIndex = -1;
    bool isDraggingLength = false;
    int dragTargetLength = 0;

    SequenceState* stateRef = nullptr;

    std::array<std::array<float, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> borderDisplayWidths_ = {};

    std::array<std::array<juce::Colour, SequenceGridConstants::gridCols>,
               SequenceGridConstants::gridRows> displayColours_;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;
};
