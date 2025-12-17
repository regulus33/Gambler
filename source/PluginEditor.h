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
#include "ui/SettingsButtonComponent.h"
#include "ui/SettingsWindowComponent.h"
#include "ui/LogoComponent.h"
#include "ui/UIConstants.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::KeyListener,
                     private juce::AsyncUpdater
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
    // AsyncUpdater callback for UI refresh (called on message thread)
    void handleAsyncUpdate() override;
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
    SettingsButtonComponent settingsButton;
    std::unique_ptr<SettingsWindow> settingsWindow;
    LogoComponent logo;
    juce::Label versionLabel;

    // Inspector
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    // Settings state (synced with SettingsWindowComponent)
    bool previewNotesOnEdit = true;  // Default: enabled

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
