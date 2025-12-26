#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/ApplicationComponent.h"

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

private:
    // Recursive snapshot utility
    void snapshotAllComponents(juce::Component* root, const juce::String& basePath, int& count);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;

    // UI Components
    ApplicationComponent applicationComponent;

    // Inspector
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};