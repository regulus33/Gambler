#pragma once

#include "Icons.h"
#include "MidiLearnOverlayPainter.h"
#include "../backend/MidiLearnRegistry.h"
#include "../backend/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace ToggleIconButtonConstants {
    constexpr int contentSize = 20;      // Icon content area (20px) - 2x original
    constexpr int padding = 8;           // Padding all around (8px) - 2x original
    constexpr int totalSize = contentSize + (padding * 2);  // 36px total
}

/**
 * ToggleIconButton
 *
 * A generic toggleable icon button that displays different icons based on toggle state.
 * Can be used for settings button (gear/X), view toggle, or any other two-state icon button.
 *
 * Size: 36×36px (20px content + 8px padding all around)
 * Background: Transparent
 * Icon: White stroked path
 */
class ToggleIconButton : public juce::Component {
public:
    /**
     * Constructor
     * @param iconPathFunc1 Function that returns the path for state 1 (not toggled)
     * @param iconPathFunc2 Function that returns the path for state 2 (toggled)
     * @param useStroke If true, draws icons with stroke; if false, fills icons
     * @param autoToggle If true, button toggles itself on click; if false, external code must control state
     * @param scaleIcon1 Scale factor for icon 1 (1.0 = normal, 0.75 = 75% size)
     * @param scaleIcon2 Scale factor for icon 2 (1.0 = normal, 0.75 = 75% size)
     */
    ToggleIconButton(std::function<juce::Path()> iconPathFunc1,
                     std::function<juce::Path()> iconPathFunc2,
                     bool useStroke = true,
                     bool autoToggle = true,
                     float scaleIcon1 = 1.0f,
                     float scaleIcon2 = 1.0f)
        : iconPath1(iconPathFunc1),
          iconPath2(iconPathFunc2),
          drawWithStroke(useStroke),
          autoToggleEnabled(autoToggle),
          isToggled(false),
          color1(juce::Colours::white),
          color2(juce::Colours::white),
          iconScale1(scaleIcon1),
          iconScale2(scaleIcon2)
    {
    }

    void setMidiControlId(MidiControlId controlId) {
        midiControlId_ = controlId;
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    void paint (juce::Graphics& g) override {
        using namespace ToggleIconButtonConstants;

        auto path = isToggled ? iconPath2() : iconPath1();
        auto color = isToggled ? color2 : color1;
        float iconScale = isToggled ? iconScale2 : iconScale1;

        // Derive padding and content from actual bounds so the icon scales with the component
        float boundsSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float paddingRatio = static_cast<float>(padding) / static_cast<float>(totalSize);
        float actualPadding = boundsSize * paddingRatio;
        float actualContent = boundsSize - actualPadding * 2.0f;
        auto iconArea = juce::Rectangle<float>(actualPadding, actualPadding, actualContent, actualContent);

        if (drawWithStroke)
            Icons::strokeIcon(g, path, iconArea, color, 1.5f, true, iconScale);
        else
            Icons::drawIcon(g, path, iconArea, color, true, iconScale);

        // Draw MIDI learn overlay on top
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        // Handle MIDI learn mode first
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }

        // Only auto-toggle if enabled
        if (autoToggleEnabled) {
            isToggled = !isToggled;
            repaint();
        }

        if (onClick) {
            onClick(isToggled);
        }
    }

    void mouseEnter (const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit (const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    // Set the toggle state programmatically (without triggering callback)
    void setToggled(bool toggled) {
        if (isToggled != toggled) {
            isToggled = toggled;
            repaint();
        }
    }

    // Get current toggle state
    bool getToggled() const { return isToggled; }

    // Set the color for state 1 (not toggled)
    void setColor1(juce::Colour newColor) {
        if (color1 != newColor) {
            color1 = newColor;
            repaint();
        }
    }

    // Set the color for state 2 (toggled)
    void setColor2(juce::Colour newColor) {
        if (color2 != newColor) {
            color2 = newColor;
            repaint();
        }
    }

    // Callback when button is clicked
    // Parameter: true if toggled on, false if toggled off
    std::function<void(bool)> onClick;

private:
    std::function<juce::Path()> iconPath1;  // Icon for state 1 (not toggled)
    std::function<juce::Path()> iconPath2;  // Icon for state 2 (toggled)
    bool drawWithStroke;
    bool autoToggleEnabled;
    bool isToggled;
    juce::Colour color1;  // Color for state 1 (not toggled)
    juce::Colour color2;  // Color for state 2 (toggled)
    float iconScale1;     // Scale factor for icon 1
    float iconScale2;     // Scale factor for icon 2
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

