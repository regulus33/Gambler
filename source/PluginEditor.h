#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/ApplicationComponent.h"

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

    // KeyListener override
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

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

#if SHOW_INSPECTOR
    // Inspector (only included when SHOW_INSPECTOR=1)
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };
#endif

    // Track if we're currently adjusting size (to prevent recursion)
    bool isAdjustingSize = false;

    // Current design height (changes with compact mode)
    int currentDesignHeight = 514;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};