#pragma once

#include "BinaryData.h"
#include "MidiLearnOverlayPainter.h"
#include "StateListenerComponent.h"
#include "../backend/MidiMapping.h"
#include "../backend/Scale.h"
#include "../data/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace NoteRangeControlsConstants {
    // Border
    constexpr float borderWidth = 2.0f;
    constexpr float borderRadius = 3.0f;
    // Content padding (left side - sliders and label)
    constexpr int contentPaddingLeft = 4;   // Extra padding from left edge
    // Sliders
    constexpr int sliderDiameter = 25;
    constexpr int sliderPadding = 8;
    constexpr int sliderVerticalSpacing = 6;
    constexpr int extraSliderGap = 4;          // Additional gap to prevent accidental touches on iPad
    // Slider appearance
    constexpr float trackStrokeWidth = 3.0f;
    constexpr float thumbWidth = 2.0f;
    constexpr float thumbHeight = 8.0f;
    // Range bar (visual representation)
    constexpr int rangeBarHorizontalMargin = 12;
    constexpr int rangeBarVerticalMargin = 12;
    constexpr int rangeBarXOffset = -4;      // Shift gauge left (negative) or right (positive) (rightmost pasdding)
    constexpr int midiNoteCount = 128;
    // Range bar colours
    constexpr juce::uint32 rangeInScaleColourHex = 0xFF2196F3;  // Blue for notes in range AND in scale (playable)
    constexpr juce::uint32 rangeNotInScaleColourHex = 0xFF4CAF50; // Green for notes in range but NOT in scale
    constexpr juce::uint32 rangeInactiveColourHex = 0xFF373737; // Grey for notes out of range
    // Label
    constexpr float labelFontSize = 22.0f;
    constexpr int labelGapFromSlider = 4;
}

/**
 * Custom LookAndFeel for note range rotary sliders
 * - White circular track (stroke only)
 * - White rectangular thumb on the circumference
 */
class NoteRangeSliderLookAndFeel : public juce::LookAndFeel_V4 {
public:
    NoteRangeSliderLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        juce::ignoreUnused (slider);
        using namespace NoteRangeControlsConstants;

        auto bounds = juce::Rectangle<float> (static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

        // Calculate the track circle dimensions (inset by half the stroke width)
        float trackInset = trackStrokeWidth * 0.5f;
        auto trackBounds = bounds.reduced (trackInset);
        float radius = trackBounds.getWidth() * 0.5f;
        auto centre = trackBounds.getCentre();

        // Draw the circular track (white stroke, no fill)
        g.setColour (juce::Colours::white);
        g.drawEllipse (trackBounds, trackStrokeWidth);

        // Calculate the thumb position on the circumference
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Position thumb center slightly inside the circumference so it doesn't extend outside
        float thumbRadius = radius - (thumbHeight * 0.32f);
        float thumbCentreX = centre.x + thumbRadius * std::sin (angle);
        float thumbCentreY = centre.y - thumbRadius * std::cos (angle);

        // Draw the thumb as a rotated rectangle
        juce::Path thumbPath;
        thumbPath.addRectangle (-thumbWidth * 0.5f, -thumbHeight * 0.5f, thumbWidth, thumbHeight);

        // Rotate the thumb to be tangent to the circle (perpendicular to radius)
        juce::AffineTransform transform = juce::AffineTransform::rotation (angle)
                                              .translated (thumbCentreX, thumbCentreY);

        g.setColour (juce::Colours::white);
        g.fillPath (thumbPath, transform);
    }
};

/**
 * NoteRangeGauge
 *
 * Visual representation of the MIDI note range.
 * Displays 128 rectangles (one per MIDI note), colored based on:
 * - Grey: out of range
 * - Green: in range but NOT in scale
 * - Blue: in range AND in scale (actually playable)
 * - White: currently playing note (only while note is active)
 *
 * The coloured window area is draggable: clicking and dragging vertically
 * moves the window offset, keeping the window size fixed.
 */
class NoteRangeGauge : public juce::Component {
public:
    NoteRangeGauge() {
        // Default: all notes in scale (chromatic)
        scaleNotes.fill(true);
    }

    void setRange (int newMin, int newMax) {
        minNote = newMin;
        maxNote = newMax;
        repaint();
    }

    /** Called by the parent whenever the raw APVTS param values change,
     *  so the gauge can do accurate drag-to-offset conversion. */
    void setRawParams (int rawOffset, int size) {
        rawOffset_ = rawOffset;
        size_ = size;
    }

    void setScaleNotes(const std::array<bool, 12>& notes) {
        scaleNotes = notes;
        repaint();
    }

    void setActiveNote (int noteNumber) {
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

    /** Callback invoked during drag with the new raw offset value (0-127). */
    std::function<void(int newRawOffset)> onOffsetDragged;

    void paint (juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();

        // Draw 128 rectangles (one per MIDI note), stacked vertically
        float noteHeight = bounds.getHeight() / static_cast<float>(midiNoteCount);
        float noteWidth = bounds.getWidth();

        for (int i = 0; i < midiNoteCount; ++i) {
            // Draw from top (note 127) to bottom (note 0)
            int noteIndex = midiNoteCount - 1 - i;
            float y = bounds.getY() + static_cast<float>(i) * noteHeight;

            juce::Rectangle<float> noteRect (bounds.getX(), y, noteWidth, noteHeight);

            // Determine color based on state
            bool inRange = (noteIndex >= minNote && noteIndex <= maxNote);
            bool inScale = scaleNotes[noteIndex % 12];

            if (noteIndex == activeNote) {
                // Currently playing note - white
                g.setColour (juce::Colours::white);
            } else if (!inRange) {
                // Out of range - grey
                g.setColour (juce::Colour (rangeInactiveColourHex));
            } else if (inScale) {
                // In range AND in scale - blue (playable)
                g.setColour (juce::Colour (rangeInScaleColourHex));
            } else {
                // In range but NOT in scale - green
                g.setColour (juce::Colour (rangeNotInScaleColourHex));
            }

            g.fillRect (noteRect);
        }
    }

    // -----------------------------------------------------------------------
    // Drag-to-move window interaction

    void mouseMove (const juce::MouseEvent& e) override {
        setMouseCursor (isInsideWindow (e.y) ? juce::MouseCursor::UpDownResizeCursor
                                             : juce::MouseCursor::NormalCursor);
    }

    void mouseDown (const juce::MouseEvent& e) override {
        if (!isInsideWindow (e.y)) {
            dragging_ = false;
            return;
        }
        dragging_ = true;
        dragStartY_ = e.y;
        dragStartRawOffset_ = rawOffset_;
    }

    void mouseDrag (const juce::MouseEvent& e) override {
        if (!dragging_ || !onOffsetDragged) return;

        using namespace NoteRangeControlsConstants;
        float noteHeight = static_cast<float>(getHeight()) / static_cast<float>(midiNoteCount);

        // Positive dy = dragged downward = lower notes = smaller offset
        float dy = static_cast<float>(e.y - dragStartY_);
        float notesDelta = dy / noteHeight;   // how many MIDI notes we've moved

        // Convert note delta to raw-offset delta.
        // actualOffset = rawOffset / 127.0 * headroom  →  rawOffset = actualOffset * 127.0 / headroom
        int headroom = 128 - size_;
        if (headroom <= 0) return;

        float rawDelta = -notesDelta * 127.0f / static_cast<float>(headroom);
        int newRaw = juce::jlimit (0, 127, static_cast<int>(std::round (
            static_cast<float>(dragStartRawOffset_) + rawDelta)));

        onOffsetDragged (newRaw);
    }

    void mouseUp (const juce::MouseEvent&) override {
        dragging_ = false;
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    void mouseExit (const juce::MouseEvent&) override {
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

private:
    /** Returns true if the given y pixel coordinate falls within the coloured window band. */
    bool isInsideWindow (int y) const {
        using namespace NoteRangeControlsConstants;
        float noteHeight = static_cast<float>(getHeight()) / static_cast<float>(midiNoteCount);
        // Note 127 is at the top; note 0 is at the bottom
        float windowTopY    = static_cast<float>(midiNoteCount - 1 - maxNote) * noteHeight;
        float windowBottomY = static_cast<float>(midiNoteCount - minNote)     * noteHeight;
        return static_cast<float>(y) >= windowTopY && static_cast<float>(y) <= windowBottomY;
    }

    int minNote = 0;
    int maxNote = 127;
    int activeNote = -1;  // -1 means no active note
    std::array<bool, 12> scaleNotes{};  // Which pitch classes are in the scale

    // Raw APVTS param values — needed for accurate drag conversion
    int rawOffset_ = 0;
    int size_ = 128;

    // Drag state
    bool dragging_ = false;
    int  dragStartY_ = 0;
    int  dragStartRawOffset_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeGauge)
};

/**
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Contains controls for note range settings using a window-based approach:
 * - Window Size (bottom slider): How many notes are in the valid range (1-128)
 * - Window Offset (top slider): Starting position of the window (0-127)
 *
 * Example: size=45, offset=54 → valid notes are [54, 98] (54 + 45 - 1)
 *
 * Observer Pattern:
 * - Listens to scale window parameters (offset, size)
 * - Automatically updates UI when window parameters change
 * - Directly maps to backend window parameters (no conversion needed)
 */
class NoteRangeControlsComponent : public StateListenerComponent {
public:
    explicit NoteRangeControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        // Load Inter 24pt Bold static font (designed for medium display sizes)
        interBoldFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);

        // Setup window width slider (bottom slider)
        // Range: 1 to 128 (minimum window width is 1 note)
        windowWidthSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        windowWidthSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        windowWidthSlider.setLookAndFeel (&sliderLookAndFeel);
        addAndMakeVisible (windowWidthSlider);

        // Setup window offset slider (top slider)
        // Range: 0 to 127 (starting position of the window)
        windowOffsetSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        windowOffsetSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        windowOffsetSlider.setLookAndFeel (&sliderLookAndFeel);
        addAndMakeVisible (windowOffsetSlider);

        // Create APVTS attachments - these bidirectionally sync sliders with params
        // The param range defines the slider range automatically
        windowOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            getProcessor().getAPVTS(), Params::Keys::ID::SCALE_WINDOW_OFFSET, windowOffsetSlider);
        windowWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            getProcessor().getAPVTS(), Params::Keys::ID::SCALE_WINDOW_SIZE, windowWidthSlider);

        // Setup range gauge
        addAndMakeVisible (rangeGauge);

        // Wire gauge drag → APVTS offset param (goes through the same path as the rotary knob)
        rangeGauge.onOffsetDragged = [this] (int newRawOffset) {
            if (auto* param = getProcessor().getAPVTS().getParameter (Params::Keys::ID::SCALE_WINDOW_OFFSET))
                param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float>(newRawOffset)));
        };

        // Setup MIDI learn overlays (hidden by default, shown during learn mode)
        addAndMakeVisible(offsetOverlay_);
        offsetOverlay_.setVisible(false);
        offsetOverlay_.setInterceptsMouseClicks(true, false);
        offsetOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning()) {
                registry_->setLearningControl(MidiControlId::NOTE_RANGE_MIN);
                if (onMidiLearnSelected) onMidiLearnSelected();
            }
        };

        addAndMakeVisible(widthOverlay_);
        widthOverlay_.setVisible(false);
        widthOverlay_.setInterceptsMouseClicks(true, false);
        widthOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning()) {
                registry_->setLearningControl(MidiControlId::NOTE_RANGE_MAX);
                if (onMidiLearnSelected) onMidiLearnSelected();
            }
        };

        // Subscribe to scale range parameters and global scale notes
        listenToScaleRange();
        listenToGlobalScaleNotes();
    }

    /**
     * Set the current window values (for initialization from backend)
     */
    void setWindow(int offset, int size) {
        // Sliders are synced via APVTS attachments, just update the gauge
        rangeGauge.setRawParams(offset, size);
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);
    }

    /**
     * Set the currently playing note (highlighted in white on the gauge)
     */
    void setActiveNote(int noteNumber) {
        rangeGauge.setActiveNote(noteNumber);
    }

    /**
     * Clear the active note highlight (called on note-off)
     */
    void clearActiveNote() {
        rangeGauge.clearActiveNote();
    }

    /**
     * Set the MIDI learn registry for the note range knobs
     */
    void setRegistry(MidiLearnRegistry* registry) {
        registry_ = registry;
    }

    /**
     * Set the callback for when a control is selected for MIDI learning
     */
    void setMidiLearnSelectedCallback(std::function<void()> callback) {
        onMidiLearnSelected = std::move(callback);
    }

    /**
     * Callback when this control is selected for MIDI learning
     */
    std::function<void()> onMidiLearnSelected;

    /**
     * Callback fired (on the message thread) whenever the note range changes.
     * Subscribers can use this to update dependent UI (e.g. note/octave sliders).
     */
    std::function<void()> onRangeChanged;

    ~NoteRangeControlsComponent() override {
        windowWidthSlider.setLookAndFeel (nullptr);
        windowOffsetSlider.setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();

        // Show/hide MIDI learn overlays based on learn mode
        bool isLearning = registry_ && registry_->isLearning();
        offsetOverlay_.setVisible(isLearning);
        widthOverlay_.setVisible(isLearning);

        // Draw white border inside the bounds
        g.setColour (juce::Colours::white);
        float inset = borderWidth * 0.5f;
        juce::Rectangle<float> borderBounds = bounds.reduced (inset);
        g.drawRoundedRectangle (borderBounds, borderRadius, borderWidth);

        // Paint MIDI learn overlays on top of each slider
        if (isLearning) {
            MidiLearnOverlayPainter::paint(g, windowOffsetSlider.getBounds(),
                                           registry_, MidiControlId::NOTE_RANGE_MIN, 0.0f);
            MidiLearnOverlayPainter::paint(g, windowWidthSlider.getBounds(),
                                           registry_, MidiControlId::NOTE_RANGE_MAX, 0.0f);
        }

        // Draw rotated "Note Range" label in the left half, above the sliders
        // Calculate the available space: from top to just above windowOffsetSlider (top slider)
        float labelAreaTop = static_cast<float>(sliderPadding);
        float labelAreaBottom = static_cast<float>(getHeight() - sliderPadding - sliderDiameter - sliderVerticalSpacing - sliderDiameter - labelGapFromSlider);
        float labelAreaHeight = labelAreaBottom - labelAreaTop;
        float labelAreaCentreX = static_cast<float>(sliderPadding + contentPaddingLeft) + sliderDiameter / 2.0f;
        float labelAreaCentreY = labelAreaTop + labelAreaHeight / 2.0f;

        g.setColour (juce::Colours::white);

        juce::Font labelFont;
        if (interBoldFont != nullptr)
            labelFont = juce::Font (interBoldFont);
        labelFont.setHeight (labelFontSize);
        g.setFont (labelFont);

        // Save the current transform, rotate 90° counter-clockwise, draw text, restore
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                          labelAreaCentreX,
                                                          labelAreaCentreY));

        // Draw text centered at the rotation point (text will now read bottom-to-top)
        juce::Rectangle<float> textBounds (labelAreaCentreX - labelAreaHeight / 2.0f,
                                            labelAreaCentreY - labelFontSize / 2.0f,
                                            labelAreaHeight,
                                            labelFontSize);
        g.drawText ("Note Range", textBounds, juce::Justification::centred, false);
        g.restoreState();
    }

    void resized() override {
        using namespace NoteRangeControlsConstants;

        // Position WindowWidthSlider at bottom-left with padding
        int widthSliderX = sliderPadding + contentPaddingLeft;
        int widthSliderY = getHeight() - sliderPadding - sliderDiameter;
        windowWidthSlider.setBounds (widthSliderX, widthSliderY, sliderDiameter, sliderDiameter);

        // Position WindowOffsetSlider directly above WindowWidthSlider with vertical spacing
        int offsetSliderX = sliderPadding + contentPaddingLeft;
        int offsetSliderY = widthSliderY - sliderVerticalSpacing - sliderDiameter - extraSliderGap;
        windowOffsetSlider.setBounds (offsetSliderX, offsetSliderY, sliderDiameter, sliderDiameter);

        // Position MIDI learn overlays on top of their respective sliders
        offsetOverlay_.setBounds (offsetSliderX, offsetSliderY, sliderDiameter, sliderDiameter);
        widthOverlay_.setBounds (widthSliderX, widthSliderY, sliderDiameter, sliderDiameter);

        // Position range gauge in the right half with margins
        int halfWidth = getWidth() / 2;
        int gaugeX = halfWidth + rangeBarHorizontalMargin + rangeBarXOffset;
        int gaugeY = rangeBarVerticalMargin;
        int gaugeWidth = halfWidth - (rangeBarHorizontalMargin * 2);
        int gaugeHeight = getHeight() - (rangeBarVerticalMargin * 2);
        rangeGauge.setBounds (gaugeX, gaugeY, gaugeWidth, gaugeHeight);
    }

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the sliders and gauge to reflect the current window state and scale.
     */
    void onStateChanged() override {
        auto& state = getState();
        int offset = state.getScaleWindowOffset();
        int size = state.getScaleWindowSize();

        // Keep gauge's raw param copy up to date (needed for drag conversion)
        rangeGauge.setRawParams(offset, size);

        // Update range
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);

        // Update scale notes for coloring
        std::array<bool, 12> scaleNotes{};
        for (int i = 0; i < 12; ++i) {
            scaleNotes[i] = state.getGlobalScaleNote(i);
        }
        rangeGauge.setScaleNotes(scaleNotes);

        // Force immediate repaint of the gauge to ensure visual feedback
        rangeGauge.repaint();

        // Notify subscribers that the range has changed
        if (onRangeChanged)
            onRangeChanged();
    }

private:
    // Transparent overlay component that sits on top of a slider in MIDI learn mode
    class MidiLearnOverlay : public juce::Component {
    public:
        std::function<void()> onMouseDown;

        void mouseDown(const juce::MouseEvent&) override {
            if (onMouseDown) onMouseDown();
        }

        void paint(juce::Graphics&) override {
            // Transparent - the parent paints the overlay visuals
        }
    };

    juce::Typeface::Ptr interBoldFont;
    NoteRangeSliderLookAndFeel sliderLookAndFeel;
    juce::Slider windowWidthSlider;   // Bottom slider: window width (1-128)
    juce::Slider windowOffsetSlider;  // Top slider: window offset (0-127)
    NoteRangeGauge rangeGauge;

    // APVTS attachments - bidirectionally sync sliders with params
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> windowOffsetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> windowWidthAttachment;

    // MIDI learn state
    MidiLearnRegistry* registry_ = nullptr;
    MidiLearnOverlay offsetOverlay_;
    MidiLearnOverlay widthOverlay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeControlsComponent)
};

