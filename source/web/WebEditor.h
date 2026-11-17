#pragma once

#include "../PluginProcessor.h"
#include "../bridge/UiBridge.h"
#include <juce_gui_extra/juce_gui_extra.h>

/**
 * WebEditor - the plugin window: one web view showing the UI in /web.
 *
 * All it does is serve the bundled web files, forward UI actions to the
 * UiBridge, forward bridge events to the page, and own the native dialogs
 * the page cannot show itself (MIDI export).
 */
class WebEditor : public juce::AudioProcessorEditor {
public:
    explicit WebEditor (PluginProcessor&);
    ~WebEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Resource = juce::WebBrowserComponent::Resource;

    static std::optional<Resource> provideResource (const juce::String& url);
    void exportMidi();

    PluginProcessor& processorRef;
    UiBridge& bridge;
    juce::WebBrowserComponent webView;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File exportedFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebEditor)
};
