#pragma once

#include "SubsequenceConstants.h"
#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include "BinaryData.h"
#include "../MidiLearnOverlayPainter.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>
#include <memory>

/**
 * SubsequenceButtonComponent - Simple button for +/- actions
 * Supports MIDI learn mode for mapping to SUBSEQ_MINUS and SUBSEQ_PLUS
 */
class SubsequenceButtonComponent : public juce::Component {
public:
    explicit SubsequenceButtonComponent (const juce::String& labelText)
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
