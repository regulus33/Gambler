#pragma once

#include "BinaryData.h"
#include "StateListenerComponent.h"
#include "MidiLearnOverlayPainter.h"
#include "MusicConstants.h"
#include "UIConstants.h"
#include "../backend/MidiLearnRegistry.h"
#include "../backend/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>
#include <array>

namespace SubsequenceControlsConstants {
    // Scaling factor to match SequenceGrid height (196px)
    // Current unscaled height: title(24) + gap(13) + bars(157) + gap(16) + buttons(15) = 225px
    // To fit in 196px with title fixed: (196 - 24) / (13 + 157 + 16 + 15) = 172 / 201 ≈ 0.856
    constexpr float heightScaleFactor = 1.1f;

    // Base dimensions (before scaling)
    constexpr int baseGapBelowTitle = 13;
    constexpr float baseBarWidth = 50.f;
    constexpr int baseBarHeight = 17;
    constexpr int baseBarSpacing = 3;
    constexpr int baseButtonHeight = 15;
    constexpr int baseGapAboveButtons = 16;

    // Scaled dimensions
    constexpr int gapBelowTitle = 3;
    constexpr float barWidth = baseBarWidth * heightScaleFactor;  // ~42.8px
    constexpr int barHeight = static_cast<int>(baseBarHeight * heightScaleFactor);  // ~14px
    constexpr int barSpacing = static_cast<int>(baseBarSpacing * heightScaleFactor);  // ~2px
    constexpr int buttonHeight = static_cast<int>(baseButtonHeight * heightScaleFactor);  // ~12px
    constexpr int gapAboveButtons = static_cast<int>(baseGapAboveButtons * heightScaleFactor);  // ~13px

    // Other dimensions (scaled)
    constexpr float barCornerRadius = 2.0f * heightScaleFactor;
    constexpr float borderWidth = 2.0f * heightScaleFactor;
    constexpr int numBars = 8;

    // Calculated bar stack dimensions
    constexpr int barStackHeight = numBars * barHeight + (numBars - 1) * barSpacing;  // ~134px

    // Buttons
    constexpr float buttonWidthRatio = 0.38f;
    constexpr float buttonWidth = barWidth * buttonWidthRatio;
    constexpr float buttonCornerRadius = 3.0f * heightScaleFactor;
    constexpr float buttonFontSize = 12.0f * heightScaleFactor;

    // Colors
    constexpr juce::uint32 defaultGreyHex = 0xff484848;  // Same as button background
    constexpr juce::uint32 restGreyHex = 0xff505050;     // Lighter grey for rest substeps (matches StepButton)
    constexpr juce::uint32 buttonBgHex = 0xff484848;

    // Note label
    constexpr float fontSize = 10.0f;
    constexpr float activeCircleDiameter = 14.0f;

    // Container
    constexpr int horizontalPadding = 10;  // Padding on each side
    constexpr int containerWidth = static_cast<int>(barWidth) + 2 * horizontalPadding;

    // Total height calculation
    constexpr int totalHeight = gapBelowTitle + barStackHeight + gapAboveButtons + buttonHeight;
}

/**
 * SingleBar - Individual clickable rectangle component
 * Supports MIDI learn mode for mapping to SUBSEQ_BAR_0 through SUBSEQ_BAR_7
 */
class SingleBar : public juce::Component {
public:
    SingleBar()
        : color (juce::Colour (SubsequenceControlsConstants::defaultGreyHex)),
          hasBorder (false)
    {
    }

    void setMidiControlId(MidiControlId controlId) {
        midiControlId_ = controlId;
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;
        auto bounds = getLocalBounds().toFloat();

        // Fill bar with color
        g.setColour (color);
        g.fillRoundedRectangle (bounds, barCornerRadius);

        // Draw white border if this is the current (playing) substep
        if (hasBorder) {
            g.setColour (UIConstants::selectedStepOutlineColour);
            g.drawRoundedRectangle (bounds.reduced (0.5f), barCornerRadius, borderWidth);
        }

        // Draw note label if we have one
        if (noteLabel_.isNotEmpty()) {
            auto interFont = juce::Typeface::createSystemTypefaceFor(
                BinaryData::InterVariableFont_ttf,
                BinaryData::InterVariableFont_ttfSize);
            juce::Font font(juce::FontOptions(interFont).withHeight(fontSize));
            font.setBold(true);

            if (isCurrent_) {
                // Active substep: dark circle + white text (matches StepButton)
                float circleDiam = activeCircleDiameter;
                float circleX = bounds.getCentreX() - circleDiam * 0.5f;
                float circleY = bounds.getCentreY() - circleDiam * 0.5f;
                auto circleBounds = juce::Rectangle<float>(circleX, circleY, circleDiam, circleDiam);

                g.setColour(juce::Colours::black);
                g.fillEllipse(circleBounds);

                g.setColour(juce::Colours::white);
                g.setFont(font);
                g.drawText(noteLabel_, circleBounds, juce::Justification::centred, false);
            } else {
                // Normal substep: black text on colored background
                g.setColour(juce::Colours::black);
                g.setFont(font);
                g.drawText(noteLabel_, bounds, juce::Justification::centred, false);
            }
        }

        // Draw MIDI learn overlay on top
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, barCornerRadius);
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        // Handle MIDI learn mode first
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }

        if (onClick)
            onClick();
    }

    void setColor (juce::Colour newColor) {
        color = newColor;
        repaint();
    }

    juce::Colour getColor() const {
        return color;
    }

    void setBorder (bool shouldHaveBorder) {
        hasBorder = shouldHaveBorder;
        repaint();
    }

    bool getBorder() const {
        return hasBorder;
    }

    void setNoteLabel(const juce::String& label) {
        noteLabel_ = label;
        repaint();
    }

    void setCurrent(bool current) {
        isCurrent_ = current;
        repaint();
    }

    std::function<void()> onClick;

private:
    juce::Colour color;
    bool hasBorder;
    juce::String noteLabel_;
    bool isCurrent_ = false;
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

/**
 * SubsequenceBar - A vertical stack of 8 clickable rectangle components
 */
class SubsequenceBar : public juce::Component {
public:
    SubsequenceBar() {
        // Create and add all bar components
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i] = std::make_unique<SingleBar>();
            addAndMakeVisible (bars[i].get());

            // Set MIDI control ID (SUBSEQ_BAR_0 through SUBSEQ_BAR_7)
            bars[i]->setMidiControlId(static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SUBSEQ_BAR_0) + i));

            // Setup click callback for each bar
            bars[i]->onClick = [this, i]() {
                if (onBarClicked)
                    onBarClicked (i);
            };
        }
    }

    /**
     * Set the MIDI learn registry for all bars
     */
    void setRegistry(MidiLearnRegistry* reg) {
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i]->setRegistry(reg);
        }
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        for (int i = 0; i < numBars; ++i) {
            int y = i * (barHeight + barSpacing);
            bars[i]->setBounds (0, y, static_cast<int>(barWidth), barHeight);
        }
    }

    /**
     * Set the color of a single bar
     * @param index Bar index (0-7)
     * @param color The new color
     */
    void setBarColor (int index, juce::Colour color) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setColor (color);
        }
    }

    /**
     * Get the color of a single bar
     * @param index Bar index (0-7)
     */
    juce::Colour getBarColor (int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getColor();
        }
        return juce::Colour (SubsequenceControlsConstants::defaultGreyHex);
    }

    /**
     * Set the border visibility of a single bar
     * @param index Bar index (0-7)
     * @param hasBorder Whether to show the border
     */
    void setBarBorder (int index, bool hasBorder) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setBorder (hasBorder);
        }
    }

    /**
     * Get the border state of a single bar
     * @param index Bar index (0-7)
     */
    bool getBarBorder (int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getBorder();
        }
        return false;
    }

    /**
     * Set the note label of a single bar
     * @param index Bar index (0-7)
     * @param label Note name (e.g., "C4") or empty for no label
     */
    void setBarNoteLabel(int index, const juce::String& label) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setNoteLabel(label);
        }
    }

    /**
     * Set whether a bar is the current (playing) substep
     * @param index Bar index (0-7)
     * @param current Whether this is the active playing substep
     */
    void setBarCurrent(int index, bool current) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setCurrent(current);
        }
    }

    /**
     * Clear all borders
     */
    void clearAllBorders() {
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i]->setBorder (false);
        }
    }

    /**
     * Get direct access to a bar component
     * @param index Bar index (0-7)
     */
    SingleBar* getBar (int index) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index].get();
        }
        return nullptr;
    }

    // Callback for bar clicks
    std::function<void(int)> onBarClicked;

private:
    std::array<std::unique_ptr<SingleBar>, SubsequenceControlsConstants::numBars> bars;
};

/**
 * SubsequenceButton - Simple button for +/- actions
 * Supports MIDI learn mode for mapping to SUBSEQ_MINUS and SUBSEQ_PLUS
 */
class SubsequenceButton : public juce::Component {
public:
    explicit SubsequenceButton (const juce::String& labelText)
        : label (labelText)
    {
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        vblank = std::make_unique<juce::VBlankAttachment>(this, [this](double ts) {
            onVBlank(ts);
        });
    }

    void setMidiControlId(MidiControlId controlId) {
        midiControlId_ = controlId;
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;
        auto bounds = getLocalBounds().toFloat();

        // Animate display colours toward targets
        auto [targetBg, targetText] = getTargetColours();
        if (!coloursInitialised_) {
            displayBg_ = targetBg;
            displayText_ = targetText;
            coloursInitialised_ = true;
        } else {
            float t = juce::jmin(1.0f, colourFadeSpeed_ * frameDelta_);
            if (displayBg_ != targetBg) {
                displayBg_ = displayBg_.interpolatedWith(targetBg, t);
                if (displayBg_ != targetBg) needsRepaint_ = true;
            }
            if (displayText_ != targetText) {
                displayText_ = displayText_.interpolatedWith(targetText, t);
                if (displayText_ != targetText) needsRepaint_ = true;
            }
        }

        // Background
        g.setColour(displayBg_);
        g.fillRoundedRectangle(bounds, buttonCornerRadius);

        // Text
        g.setColour(displayText_);
        juce::Font font (juce::FontOptions (interTypeface).withHeight (buttonFontSize));
        font.setBold (true);
        g.setFont (font);
        g.drawText (label, bounds.toNearestInt(), juce::Justification::centred);

        // Draw MIDI learn overlay on top
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, buttonCornerRadius);
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        // Handle MIDI learn mode first
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }

        pressed_ = true;
        needsRepaint_ = true;
        repaint();

        if (onClick)
            onClick();
    }

    void mouseUp (const juce::MouseEvent& /*event*/) override {
        pressed_ = false;
        needsRepaint_ = true;
        repaint();
    }

    std::function<void()> onClick;

private:
    static constexpr float colourFadeSpeed_ = 10.0f;

    std::pair<juce::Colour, juce::Colour> getTargetColours() const {
        if (pressed_)
            return { juce::Colours::white, juce::Colour(SubsequenceControlsConstants::buttonBgHex) };
        return { juce::Colour(SubsequenceControlsConstants::buttonBgHex), juce::Colours::white };
    }

    void onVBlank(double timestamp) {
        if (lastVBlankTime_ > 0.0) {
            double delta = timestamp - lastVBlankTime_;
            frameDelta_ = static_cast<float>(juce::jlimit(0.001, 0.1, delta));
        }
        lastVBlankTime_ = timestamp;

        if (needsRepaint_) {
            needsRepaint_ = false;
            repaint();
        }
    }

    juce::String label;
    juce::Typeface::Ptr interTypeface;
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;

    // Animation state
    std::unique_ptr<juce::VBlankAttachment> vblank;
    bool pressed_ = false;
    bool coloursInitialised_ = false;
    bool needsRepaint_ = false;
    juce::Colour displayBg_ { SubsequenceControlsConstants::buttonBgHex };
    juce::Colour displayText_ { juce::Colours::white };
    float frameDelta_ = 0.016f;
    double lastVBlankTime_ = 0.0;
};

/**
 * SubsequenceControlsComponent
 *
 * Container for the Subsequence controls UI.
 * - Title "Sub" at the top
 * - SubsequenceBar (8 vertical clickable rectangles)
 * - Two buttons: "-" and "+"
 *
 * Observer Pattern:
 * - Listens to selected step and substep parameters
 * - Automatically updates UI when substep data changes
 *
 * MIDI Learn:
 * - Supports MIDI mapping for SUBSEQ_BAR_0-7, SUBSEQ_MINUS, SUBSEQ_PLUS
 */
class SubsequenceControlsComponent : public StateListenerComponent {
public:
    explicit SubsequenceControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor),
          minusButton ("-"),
          plusButton ("+")
    {
        addAndMakeVisible (subsequenceBar);
        addAndMakeVisible (minusButton);
        addAndMakeVisible (plusButton);

        // Set MIDI control IDs for buttons
        minusButton.setMidiControlId(MidiControlId::SUBSEQ_MINUS);
        plusButton.setMidiControlId(MidiControlId::SUBSEQ_PLUS);

        // Setup bar click callback
        subsequenceBar.onBarClicked = [this] (int index) {
            if (onBarClicked)
                onBarClicked (index);
        };

        // Setup button callbacks
        minusButton.onClick = [this]() {
            if (onMinusClicked)
                onMinusClicked();
        };

        plusButton.onClick = [this]() {
            if (onPlusClicked)
                onPlusClicked();
        };

        // Subscribe to sequence metadata (selected step, current index, length)
        listenToSequenceMetadata();
        listenToStepNotes(0, ParamConstants::MAX_STEPS - 1);
        listenToStepNotesLength(0, ParamConstants::MAX_STEPS - 1);

        // Subscribe to note colour changes
        for (int i = 0; i < 12; ++i)
            listenToParameter(Params::Keys::ID::noteColour(i));
    }

    /**
     * Set the MIDI learn registry for all child components
     */
    void setMidiLearnRegistry(MidiLearnRegistry* registry) {
        subsequenceBar.setRegistry(registry);
        minusButton.setRegistry(registry);
        plusButton.setRegistry(registry);
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        int barY = gapBelowTitle;

        // Center the bar horizontally
        int barX = (getWidth() - static_cast<int>(barWidth)) / 2;
        subsequenceBar.setBounds (barX, barY, static_cast<int>(barWidth), barStackHeight);

        // Position buttons below the bar
        int buttonsY = subsequenceBar.getBottom() + gapAboveButtons;

        // Calculate button positions so they align with bar edges
        // Left edge of "-" aligns with left edge of bar
        // Right edge of "+" aligns with right edge of bar
        int buttonWidthInt = static_cast<int>(std::ceil(buttonWidth));
        int totalButtonsWidth = 2 * buttonWidthInt;
        int spacing = static_cast<int>(barWidth) - totalButtonsWidth;

        minusButton.setBounds (barX, buttonsY, buttonWidthInt, buttonHeight);
        plusButton.setBounds (barX + buttonWidthInt + spacing, buttonsY, buttonWidthInt, buttonHeight);
    }

    // Access to the bar for external updates
    SubsequenceBar& getSubsequenceBar() { return subsequenceBar; }
    const SubsequenceBar& getSubsequenceBar() const { return subsequenceBar; }

    // Callbacks
    std::function<void(int)> onBarClicked;
    std::function<void()> onMinusClicked;
    std::function<void()> onPlusClicked;

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the subsequence bar to reflect the current substep state.
     */
    void onStateChanged() override {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        int substepLength = state.getStepNotesLength(selectedStep);
        int currentSubstep = state.getStepNotesCurrentIndex(selectedStep);
        bool isCurrentStep = (selectedStep == state.getCurrentIndex());

        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            if (i < substepLength) {
                int noteNumber = state.getQuantizedStepNote(selectedStep, i);
                bool isCurrent = isCurrentStep && (i == currentSubstep);

                if (noteNumber < 0) {
                    // Rest substep
                    subsequenceBar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::restGreyHex));
                    subsequenceBar.setBarNoteLabel(i, "");
                } else {
                    // Normal note substep
                    if (isCurrent) {
                        subsequenceBar.setBarColor(i, UIConstants::activeStepColour);
                    } else {
                        int pitchClass = noteNumber % 12;
                        subsequenceBar.setBarColor(i, state.getNoteColour(pitchClass));
                    }
                    subsequenceBar.setBarNoteLabel(i, MusicConstants::midiNoteToNoteName(noteNumber));
                }

                subsequenceBar.setBarCurrent(i, isCurrent);
                subsequenceBar.setBarBorder(i, isCurrent);
            } else {
                // Inactive substep
                subsequenceBar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::defaultGreyHex));
                subsequenceBar.setBarNoteLabel(i, "");
                subsequenceBar.setBarCurrent(i, false);
                subsequenceBar.setBarBorder(i, false);
            }
        }
    }

private:
    SubsequenceBar subsequenceBar;
    SubsequenceButton minusButton;
    SubsequenceButton plusButton;
};

