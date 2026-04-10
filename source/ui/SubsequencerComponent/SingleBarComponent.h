#pragma once

#include "SubsequenceConstants.h"
#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include "BinaryData.h"
#include "../MidiLearnOverlayPainter.h"
#include "../UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>

/**
 * BorderStyle - Visual border states for substep bars
 */
enum class BarBorderStyle {
    none,       // No border
    selected,   // White solid border — user selected this substep for editing
    next,       // Grey dotted border — this substep will play next when parent step triggers
    current     // White solid border — this substep is currently playing
};

/**
 * SingleBarComponent - Individual clickable rectangle component
 * Supports MIDI learn mode for mapping to SUBSEQ_BAR_0 through SUBSEQ_BAR_7
 */
class SingleBarComponent : public juce::Component {
public:
    SingleBarComponent()
        : color_ (juce::Colour (SubsequenceControlsConstants::defaultGreyHex))
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
        g.setColour (color_);
        g.fillRoundedRectangle (bounds, barCornerRadius);

        // Animate solid border width toward target
        {
            bool wantsSolid = (borderStyle_ == BarBorderStyle::selected ||
                               borderStyle_ == BarBorderStyle::current);
            float borderTarget = wantsSolid ? borderWidth : 0.0f;
            if (std::abs(borderDisplayWidth_ - borderTarget) > 0.01f) {
                float duration = (borderTarget > borderDisplayWidth_)
                    ? UIConstants::borderFadeInDuration
                    : UIConstants::borderFadeOutDuration;
                float step = frameDeltaSec_ / duration;
                borderDisplayWidth_ = (borderTarget > borderDisplayWidth_)
                    ? juce::jmin(borderDisplayWidth_ + step * borderWidth, borderTarget)
                    : juce::jmax(borderDisplayWidth_ - step * borderWidth, borderTarget);
                needsAnimationRepaint_ = true;
            } else {
                borderDisplayWidth_ = borderTarget;
            }

            auto insetBounds = bounds.reduced(0.5f);

            if (borderDisplayWidth_ > 0.01f) {
                g.setColour(UIConstants::selectedStepOutlineColour);
                g.drawRoundedRectangle(insetBounds, barCornerRadius, borderDisplayWidth_);
            }

            // Dashed border for "next" style (no fade — it's a different visual)
            if (borderStyle_ == BarBorderStyle::next) {
                g.setColour(juce::Colour(0xff888888));
                juce::Path roundedRect;
                roundedRect.addRoundedRectangle(insetBounds, barCornerRadius);
                const float dashLengths[] = { 3.0f, 3.0f };
                juce::Path dashedPath;
                juce::PathStrokeType(borderWidth).createDashedStroke(
                    dashedPath, roundedRect, dashLengths, 2);
                g.fillPath(dashedPath);
            }
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

    void mouseDoubleClick(const juce::MouseEvent& /*event*/) override {
        if (onDoubleClick)
            onDoubleClick();
    }

    void setColor (juce::Colour newColor) {
        color_ = newColor;
        repaint();
    }

    juce::Colour getColor() const {
        return color_;
    }

    void setBorderStyle(BarBorderStyle style) {
        borderStyle_ = style;
        repaint();
    }

    BarBorderStyle getBorderStyle() const {
        return borderStyle_;
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
    std::function<void()> onDoubleClick;

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

private:
    juce::Colour color_;
    BarBorderStyle borderStyle_ = BarBorderStyle::none;
    juce::String noteLabel_;
    bool isCurrent_ = false;
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;

    // Border fade animation state
    float borderDisplayWidth_ = 0.0f;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;
};
