#pragma once

#include "NoteRangeControlsConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>

/**
 * NoteRangeGauge
 *
 * Visual representation of the MIDI note range.
 * Displays 128 rectangles (one per MIDI note), colored based on:
 * - Grey: out of range
 * - Green: in range but NOT in scale
 * - Blue: in range AND in scale (actually playable)
 * - White: currently playing note
 *
 * Clicking + dragging the coloured window vertically moves the offset.
 */
class NoteRangeGauge : public juce::Component {
public:
    NoteRangeGauge() {
        scaleNotes.fill(true);
    }

    void setRange(int newMin, int newMax) {
        minNote = newMin;
        maxNote = newMax;
        repaint();
    }

    /** Keep raw APVTS param values in sync so drag conversions are accurate. */
    void setRawParams(int rawOffset, int size) {
        rawOffset_ = rawOffset;
        size_ = size;
    }

    void setScaleNotes(const std::array<bool, 12>& notes) {
        scaleNotes = notes;
        repaint();
    }

    void setActiveNote(int noteNumber) {
        if (activeNote != noteNumber) {
            activeNote = noteNumber;
            repaint();
        }
    }

    void clearActiveNote() {
        if (activeNote != -1) {
            activeNote = -1;
            repaint();
        }
    }

    std::function<void(int newRawOffset)> onOffsetDragged;

    void paint(juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();
        float noteHeight = bounds.getHeight() / static_cast<float>(midiNoteCount);
        float noteWidth = bounds.getWidth();

        for (int i = 0; i < midiNoteCount; ++i) {
            int noteIndex = midiNoteCount - 1 - i;
            float y = bounds.getY() + static_cast<float>(i) * noteHeight;

            juce::Rectangle<float> noteRect(bounds.getX(), y, noteWidth, noteHeight);
            g.setColour(colourForNote(noteIndex));
            g.fillRect(noteRect);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(isInsideWindow(e.y) ? juce::MouseCursor::UpDownResizeCursor
                                           : juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!isInsideWindow(e.y)) {
            dragging_ = false;
            return;
        }
        dragging_ = true;
        dragStartY_ = e.y;
        dragStartRawOffset_ = rawOffset_;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!dragging_ || !onOffsetDragged) return;

        using namespace NoteRangeControlsConstants;
        float noteHeight = static_cast<float>(getHeight()) / static_cast<float>(midiNoteCount);

        float dy = static_cast<float>(e.y - dragStartY_);
        float notesDelta = dy / noteHeight;

        int headroom = 128 - size_;
        if (headroom <= 0) return;

        float rawDelta = -notesDelta * 127.0f / static_cast<float>(headroom);
        int newRaw = juce::jlimit(0, 127, static_cast<int>(std::round(
            static_cast<float>(dragStartRawOffset_) + rawDelta)));

        onOffsetDragged(newRaw);
    }

    void mouseUp(const juce::MouseEvent&) override {
        dragging_ = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

private:
    juce::Colour colourForNote(int noteIndex) const {
        using namespace NoteRangeControlsConstants;
        if (noteIndex == activeNote) return juce::Colours::white;

        bool inRange = (noteIndex >= minNote && noteIndex <= maxNote);
        if (!inRange) return juce::Colour(rangeInactiveColourHex);

        bool inScale = scaleNotes[noteIndex % 12];
        return inScale ? juce::Colour(rangeInScaleColourHex)
                       : juce::Colour(rangeNotInScaleColourHex);
    }

    bool isInsideWindow(int y) const {
        using namespace NoteRangeControlsConstants;
        float noteHeight = static_cast<float>(getHeight()) / static_cast<float>(midiNoteCount);
        float windowTopY    = static_cast<float>(midiNoteCount - 1 - maxNote) * noteHeight;
        float windowBottomY = static_cast<float>(midiNoteCount - minNote)     * noteHeight;
        return static_cast<float>(y) >= windowTopY && static_cast<float>(y) <= windowBottomY;
    }

    int minNote = 0;
    int maxNote = 127;
    int activeNote = -1;
    std::array<bool, 12> scaleNotes{};

    int rawOffset_ = 0;
    int size_ = 128;

    bool dragging_ = false;
    int dragStartY_ = 0;
    int dragStartRawOffset_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteRangeGauge)
};
