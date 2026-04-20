#pragma once

#include "../StateListenerComponent.h"
#include "../Icons.h"
#include "../MidiLearnOverlayPainter.h"
#include "../../backend/MidiLearnRegistry.h"
#include "../../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>

// Use the same constants as SequencerControlsComponent for visual consistency
namespace SequencerButtonConstants {
    constexpr float cornerRadius = 4.4f;
    constexpr float borderWidth = 2.2f;
    constexpr juce::uint32 bgColorActive = 0xffffffff;     // White when active
    constexpr juce::uint32 bgColorInactive = 0xff303030;  // Dark grey when inactive
    constexpr juce::uint32 bgColorLocked = 0xff5a1a1a;    // Dark maroon when locked
    constexpr float maxIconSize = 26.0f;
    constexpr float textAreaHeight = 33.0f;   // Match SequencerControlsConstants
    constexpr float iconVerticalOffset = 13.0f;
    constexpr float fontSize = 13.0f;         // Match SequencerControlsConstants
    constexpr float textYOffset = 13.0f;      // Match SequencerControlsConstants
    constexpr float textHeight = 13.0f;       // Match SequencerControlsConstants
    constexpr float colourFadeSpeed = 8.0f;   // Lerp speed (higher = faster fade)
}

/**
 * SequencerControlButton - Base class for APVTS-driven sequencer control buttons
 * 
 * This implements the observer pattern where:
 * 1. mouseDown/mouseUp only set the APVTS parameter (true/false)
 * 2. Visual state is driven entirely by listening to the APVTS parameter
 * 3. Actions are triggered by PluginProcessor's parameterChanged listener
 * 
 * Subclasses just need to specify:
 * - The parameter ID to control
 * - The icon and label
 * - The MidiControlId for MIDI learn
 */
class SequencerControlButton : public StateListenerComponent {
public:
    SequencerControlButton(PluginProcessor& processor, 
                           const juce::String& paramId,
                           MidiControlId midiControlId)
        : StateListenerComponent(processor)
        , paramId_(paramId)
        , midiControlId_(midiControlId)
    {
        // Subscribe to our parameter
        listenToParameter(paramId_);
    }

    void setIcon(const juce::Path& icon) {
        iconPath_ = icon;
        repaint();
    }

    void setLabel(const juce::String& label) {
        label_ = label;
        repaint();
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    MidiLearnRegistry* getRegistry() const {
        return registry_;
    }

    void paint(juce::Graphics& g) override {
        using namespace SequencerButtonConstants;
        auto bounds = getLocalBounds().toFloat();

        // Visual state is driven by APVTS parameter (overridable by subclass)
        auto [targetBg, targetContent] = getTargetColours();

        // Animate: lerp display colours toward targets
        if (!displayColoursInitialised_) {
            displayBgColour_ = targetBg;
            displayContentColour_ = targetContent;
            displayColoursInitialised_ = true;
        } else {
            float t = juce::jmin(1.0f, colourFadeSpeed * frameDeltaSec_);
            if (displayBgColour_ != targetBg) {
                displayBgColour_ = displayBgColour_.interpolatedWith(targetBg, t);
                if (displayBgColour_ != targetBg)
                    needsAnimationRepaint_ = true;
            }
            if (displayContentColour_ != targetContent) {
                displayContentColour_ = displayContentColour_.interpolatedWith(targetContent, t);
                if (displayContentColour_ != targetContent)
                    needsAnimationRepaint_ = true;
            }
        }

        juce::Colour borderColor = juce::Colours::white;

        // Fill background with animated color
        g.setColour(displayBgColour_);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Draw border
        g.setColour(borderColor);
        float inset = borderWidth * 0.5f;
        g.drawRoundedRectangle(bounds.reduced(inset), cornerRadius - inset, borderWidth);

        // Draw icon
        if (iconPath_.has_value()) {
            g.setColour(displayContentColour_);
            auto iconBounds = iconPath_->getBounds();
            float longestSide = juce::jmax(iconBounds.getWidth(), iconBounds.getHeight());
            float scale = maxIconSize / longestSide;
            float iconAreaHeight = bounds.getHeight() - textAreaHeight;
            float scaledWidth = iconBounds.getWidth() * scale;
            float scaledHeight = iconBounds.getHeight() * scale;
            float offsetX = (bounds.getWidth() - scaledWidth) * 0.5f;
            float offsetY = (iconAreaHeight - scaledHeight) * 0.5f + iconVerticalOffset;

            juce::AffineTransform transform = juce::AffineTransform::scale(scale)
                .translated(offsetX - iconBounds.getX() * scale,
                           offsetY - iconBounds.getY() * scale);
            g.fillPath(*iconPath_, transform);
        }

        // Draw label
        if (label_.isNotEmpty()) {
            g.setColour(displayContentColour_);
            juce::Font font{juce::FontOptions(fontSize)};
            font.setBold(true);
            g.setFont(font);
            float textY = bounds.getHeight() - textYOffset - textHeight;
            g.drawText(label_, juce::Rectangle<float>(0, textY, bounds.getWidth(), textHeight).toNearestInt(),
                      juce::Justification::centredBottom);
        }

        // Draw MIDI learn overlay
        paintMidiLearnOverlay(g);
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override {
        // Handle MIDI learn mode
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }

        // Set parameter to ON - this triggers the listener chain
        setParamOn(true);
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override {
        // Set parameter to OFF
        setParamOn(false);
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

protected:
    virtual std::pair<juce::Colour, juce::Colour> getTargetColours() const {
        using namespace SequencerButtonConstants;
        bool isActive = isParamOn();
        return { isActive ? juce::Colour(bgColorActive) : juce::Colour(bgColorInactive),
                 isActive ? juce::Colours::black : juce::Colours::white };
    }

    void onStateChanged() override {
        // Parameter changed - repaint to start animation toward new state
        repaint();
    }

    bool isParamOn() const {
        if (auto* param = getProcessor().getAPVTS().getParameter(paramId_)) {
            return param->getValue() > 0.5f;
        }
        return false;
    }

    void setParamOn(bool on) {
        if (auto* param = getProcessor().getAPVTS().getParameter(paramId_)) {
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        }
    }

private:
    void paintMidiLearnOverlay(juce::Graphics& g) {
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_,
                                      SequencerButtonConstants::cornerRadius);
    }

    juce::String paramId_;
    MidiControlId midiControlId_;
    juce::String label_;
    std::optional<juce::Path> iconPath_;
    MidiLearnRegistry* registry_ = nullptr;

    // Colour fade animation state
    juce::Colour displayBgColour_ { SequencerButtonConstants::bgColorInactive };
    juce::Colour displayContentColour_ { juce::Colours::white };
    bool displayColoursInitialised_ = false;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;
};

