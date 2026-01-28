#pragma once

#include "BinaryData.h"
#include "NoteControlsComponent.h"
#include "ProbabilityControlsComponent.h"
#include "UIConstants.h"
#include "../backend/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declaration
class PluginProcessor;

namespace MainControlsConstants {
    // Title
    constexpr float titleFontSize = 16.0f;
    constexpr int titleHeight = 24;
    constexpr int gapBelowTitle = 13;
    // Container layout
    constexpr int containerWidth = 198;
    constexpr int containerHeight = 100;
    constexpr int gapBetweenContainers = 8;
}

/**
 * MainControlsComponent
 *
 * Top-level container for the Main Controls UI.
 * - Title at the top
 * - NoteControlsComponent for note/octave selection
 * - ProbabilityControlsComponent for probability controls
 */
class MainControlsComponent : public juce::Component {
public:
    explicit MainControlsComponent(PluginProcessor& processor)
        : interBoldFont (juce::Typeface::createSystemTypefaceFor (
              BinaryData::Inter_24ptBold_ttf,
              BinaryData::Inter_24ptBold_ttfSize))
        , probabilityControls(processor)
    {
        addAndMakeVisible (noteControls);
        addAndMakeVisible (probabilityControls);
    }

    void paint (juce::Graphics& g) override {
        using namespace MainControlsConstants;
        g.fillAll (juce::Colours::transparentBlack);

        juce::Font titleFont;
        if (interBoldFont != nullptr)
            titleFont = juce::Font (interBoldFont);

        titleFont.setHeight (titleFontSize);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (titleHeight);
        titleArea.translate (0, UIConstants::titleYOffset);
        g.drawText ("Main Controls", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        using namespace MainControlsConstants;

        noteControls.setBounds (
            0,
            titleHeight + gapBelowTitle,
            containerWidth,
            containerHeight);

        probabilityControls.setBounds (
            0,
            noteControls.getBottom() + gapBetweenContainers,
            containerWidth,
            ProbabilityControlsConstants::containerHeight);
    }

    /**
     * Set the MIDI learn registry for child components
     */
    void setRegistry(MidiLearnRegistry* registry) {
        noteControls.setRegistry(registry);
        // probabilityControls.setRegistry(registry); // TODO: Add when probability slider is MIDI mappable
    }

    // Accessors for child components
    NoteControlsComponent& getNoteControls() { return noteControls; }
    const NoteControlsComponent& getNoteControls() const { return noteControls; }

    ProbabilityControlsComponent& getProbabilityControls() { return probabilityControls; }
    const ProbabilityControlsComponent& getProbabilityControls() const { return probabilityControls; }

private:
    juce::Typeface::Ptr interBoldFont;
    NoteControlsComponent noteControls;
    ProbabilityControlsComponent probabilityControls;
};

