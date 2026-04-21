#pragma once

#include "StyledDropdownConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * StyledDropdownMenuLookAndFeel
 *
 * Draws the dark popup menu background used on desktop.
 */
class StyledDropdownMenuLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
        g.fillAll(juce::Colour(0xFF1E1E1E));
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRect(0, 0, width, height, 1);
    }

    void getIdealPopupMenuItemSize(const juce::String& /*text*/, bool /*isSeparator*/,
                                    int /*standardMenuItemHeight*/, int& idealWidth, int& idealHeight) override {
        idealWidth = 200;
        idealHeight = StyledDropdownConstants::itemHeight;
    }
};
