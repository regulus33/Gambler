#pragma once

#include "NoteControlsComponent.h"
#include "ProbabilityControlsComponent.h"
#include "../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declaration
class PluginProcessor;

namespace MainControlsConstants {
    // Container layout
    constexpr int containerWidth = 198;
    constexpr int containerHeight = 100;
    constexpr int gapBetweenContainers = 8;
}

/**
 * MainControlsComponent
 *
 * Top-level container for the Main Controls UI.
 * - NoteControlsComponent for note/octave selection
 * - ProbabilityControlsComponent for probability controls
 */
class MainControlsComponent : public juce::Component {
public:
    explicit MainControlsComponent(PluginProcessor& processor)
        : probabilityControls(processor)
    {
        setPaintingIsUnclipped(true);
        addAndMakeVisible (noteControls);
        addAndMakeVisible (probabilityControls);
    }

    void resized() override {
        using namespace MainControlsConstants;

        auto area = getLocalBounds();
        noteControls.setBounds (area.removeFromTop (containerHeight));
        area.removeFromTop (gapBetweenContainers);
        probabilityControls.setBounds (area.removeFromTop (ProbabilityControlsConstants::containerHeight));
    }

    /**
     * Set the MIDI learn registry for child components
     */
    void setRegistry(MidiLearnRegistry* registry) {
        noteControls.setRegistry(registry);
        probabilityControls.setRegistry(registry);
    }

    /**
     * Set the callback for when a control is selected for MIDI learning
     */
    void setMidiLearnSelectedCallback(std::function<void()> callback) {
        noteControls.setMidiLearnSelectedCallback(callback);
        probabilityControls.setMidiLearnSelectedCallback(callback);
    }

    // Accessors for child components
    NoteControlsComponent& getNoteControls() { return noteControls; }
    const NoteControlsComponent& getNoteControls() const { return noteControls; }

    ProbabilityControlsComponent& getProbabilityControls() { return probabilityControls; }
    const ProbabilityControlsComponent& getProbabilityControls() const { return probabilityControls; }

private:
    NoteControlsComponent noteControls;
    ProbabilityControlsComponent probabilityControls;
};

