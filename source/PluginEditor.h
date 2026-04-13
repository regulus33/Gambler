#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/ApplicationComponent.h"
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::KeyListener,
                     public juce::ComponentListener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // KeyListener overrides
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;

    // ComponentListener override - for constraining aspect ratio
    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;

    // Access to the application component (for testing)
    ApplicationComponent& getApplicationComponent() { return applicationComponent; }

private:
    // Calculate scale factor based on current window size
    float getScaleFactor() const;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;

    // UI Components
    ApplicationComponent applicationComponent;

    // Virtual MIDI keyboard (standalone only) — uses computer keyboard as MIDI input.
    // Hidden by default; set SHOW_MIDI_KEYBOARD=1 in CMakeLists to show the keyboard strip.
    std::unique_ptr<juce::MidiKeyboardComponent> midiKeyboard;
    static constexpr int keyboardHeight = 64;

#if SHOW_INSPECTOR
    // Inspector (only included when SHOW_INSPECTOR=1)
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };
#endif

    // Track if we're currently adjusting size (to prevent recursion)
    bool isAdjustingSize = false;

    // Current design height (changes with compact mode)
    int currentDesignHeight = MainScreenComponent::fullDesignHeight;

    // Whether auto-compact has already been applied (so we only do it once on first layout)
    bool autoCompactApplied = false;

    // Height reserved for the visible MIDI keyboard strip (0 when hidden)
    int getKeyboardStripHeight() const {
#if SHOW_MIDI_KEYBOARD
        return midiKeyboard ? keyboardHeight : 0;
#else
        return 0;
#endif
    }

    // Check aspect ratio and auto-enter compact mode if needed
    void checkAutoCompact();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};