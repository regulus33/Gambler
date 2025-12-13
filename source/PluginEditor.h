#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/PianoRollComponent.h"
#include "ui/MidiDataPerStepSelectionComponent.h"
#include "ui/SequencerStepsComponent.h"
#include "ui/SequencerControlsComponent.h"
#include "ui/MainControlsComponent.h"
#include "ui/SequenceOverviewComponent.h"
#include "ui/SubsequenceControlsComponent.h"
#include "ui/ViewToggleButton.h"
#include "ui/NoteRangeControlsComponent.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::KeyListener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // KeyListener override
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

    // Helper to set active step and update piano roll
    void setActiveStepAndNote (int stepIndex);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;

    // UI Components
    PianoRollComponent pianoRoll;
    MidiDataPerStepSelectionComponent midiDataPerStep;
    SequencerStepsComponent sequencerSteps;
    SequencerControlsComponent sequencerControls;
    MainControlsComponent mainControls;
    SequenceOverviewComponent sequenceOverview;
    SubsequenceControlsComponent subsequenceControls;
    ViewToggleButton viewToggleButton;
    NoteRangeControlsComponent noteRangeControls;

    // Inspector
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
