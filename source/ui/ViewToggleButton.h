#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * ViewToggleButton
 * 
 * Toggles between PianoRoll view (shows "CC") and MIDI data per step view (shows "Note").
 * 
 * Size: 46×30px
 * Background: #484848 (medium grey)
 * Corner radius: 3px
 * Text: 12px bold white, Inter font
 */
class ViewToggleButton : public juce::Component {
public:
    ViewToggleButton() {
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }
    
    void paint (juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Colors
        juce::Colour bgColor = juce::Colour(0xff484848);
        juce::Colour textColor = juce::Colours::white;
        
        const float cornerRadius = 3.0f;
        
        // Fill background
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // Draw text
        g.setColour(textColor);
        juce::Font font(juce::FontOptions(interTypeface).withHeight(12.0f));
        font.setBold(true);
        g.setFont(font);
        
        // Show "CC" when viewing PianoRoll, "Note" when viewing MIDI data
        juce::String text = showPianoRoll ? "CC" : "Note";
        g.drawText(text, bounds.toNearestInt(), juce::Justification::centred);
    }
    
    void mouseDown (const juce::MouseEvent& /*event*/) override {
        showPianoRoll = !showPianoRoll;
        repaint();
        
        if (onToggle) {
            onToggle(showPianoRoll);
        }
    }
    
    // Callback when toggle state changes
    // Parameter: true = show PianoRoll, false = show MIDI data
    std::function<void(bool)> onToggle;
    
    bool showPianoRoll = true;
    
private:
    juce::Typeface::Ptr interTypeface;
};

