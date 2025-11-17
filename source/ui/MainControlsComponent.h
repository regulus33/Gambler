#pragma once

#include "BinaryData.h"
#include "NotchedSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MainControlsComponent
 *
 * Top-level container for the Main Controls UI.
 * - Title at the top
 * - NotchedSlider for note selection
 */
class MainControlsComponent : public juce::Component {
public:
    MainControlsComponent() {
        // Load Inter font for the title (kept consistent with the rest of the UI)
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Create the note notched slider with chromatic scale
        juce::Array<juce::String> noteNames;
        noteNames.add ("C");
        noteNames.add ("Db");
        noteNames.add ("D");
        noteNames.add ("Eb");
        noteNames.add ("E");
        noteNames.add ("F");
        noteNames.add ("Gb");
        noteNames.add ("G");
        noteNames.add ("Ab");
        noteNames.add ("A");
        noteNames.add ("Bb");
        noteNames.add ("B");

        juce::Array<juce::Colour> noteColours;
        noteColours.add (juce::Colour (0xFFFF6B6B)); // C   - Red
        noteColours.add (juce::Colour (0xFFFF9F6B)); // Db  - Orange
        noteColours.add (juce::Colour (0xFFFFD66B)); // D   - Yellow
        noteColours.add (juce::Colour (0xFFFFE66B)); // Eb  - Light yellow
        noteColours.add (juce::Colour (0xFFE6FF6B)); // E   - Yellow-green
        noteColours.add (juce::Colour (0xFF6BFFB4)); // F   - Light green
        noteColours.add (juce::Colour (0xFF6BFFD6)); // Gb  - Cyan-green
        noteColours.add (juce::Colour (0xFF6BD6FF)); // G   - Cyan
        noteColours.add (juce::Colour (0xFF6BB4FF)); // Ab  - Light blue
        noteColours.add (juce::Colour (0xFF9F6BFF)); // A   - Purple
        noteColours.add (juce::Colour (0xFFD66BFF)); // Bb  - Magenta
        noteColours.add (juce::Colour (0xFFFF6BD6)); // B   - Pink

        noteSlider = std::make_unique<NotchedSlider> ("Note", noteNames, noteColours);
        addAndMakeVisible (*noteSlider);

        noteSlider2 = std::make_unique<NotchedSlider> ("Note", noteNames, noteColours);
        addAndMakeVisible (*noteSlider2);
    }

    void paint (juce::Graphics& g) override {
        g.fillAll (juce::Colours::transparentBlack);

        // Title
        juce::Font titleFont;
        if (interFont != nullptr)
            titleFont = juce::Font (interFont);

        titleFont.setHeight (16.0f);
        titleFont.setBold (true);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (24);
        g.drawText ("Main Controls", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        auto area = getLocalBounds();

        // Reserve a strip for the title
        const int titleHeight = 24;
        area.removeFromTop (titleHeight);

        area = area.reduced (4);

        // Split the area horizontally for two sliders
        auto slider1Area = area.removeFromLeft (area.getWidth() / 2);
        noteSlider->setBounds (slider1Area);
        noteSlider2->setBounds (area);
    }

private:
    juce::Typeface::Ptr interFont;
    std::unique_ptr<NotchedSlider> noteSlider;
    std::unique_ptr<NotchedSlider> noteSlider2;
};

