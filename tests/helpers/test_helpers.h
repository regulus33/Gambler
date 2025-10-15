#pragma once
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>

using std::make_unique;


/* This is a helper function to run tests within the context of a plugin editor.
 *
 * Read more here: https://github.com/sudara/pamplejuce/issues/18#issuecomment-1425836807
 *
 * Example usage (screenshots the plugin)
 *
  runWithinPluginEditor ([&] (PluginProcessor& plugin) {
    auto snapshot = plugin.getActiveEditor()->createComponentSnapshot (plugin.getActiveEditor()->getLocalBounds(), true, 2.0f);
    auto file = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile ("snapshot.jpeg");
    file.deleteFile();
    juce::FileOutputStream stream (file);
    juce::JPEGImageFormat jpeg;
    jpeg.writeImageToStream (snapshot, stream);

    REQUIRE (file.existsAsFile());
   });

 */
[[maybe_unused]] static void runWithinPluginEditor (const std::function<void (PluginProcessor& plugin)>& testCode)
{
    PluginProcessor plugin;
    const auto editor = plugin.createEditorIfNeeded();

    testCode (plugin);

    plugin.editorBeingDeleted (editor);
    delete editor;
}

APVTS createTestApvts(PluginProcessor& processoer)
{
    APVTS::ParameterLayout layout;
    // Add common test parameters
    layout.add(std::make_unique<juce::AudioParameterInt>("stepCount", "Step Count", 1, 128, 16));

    return juce::AudioProcessorValueTreeState (processoer, nullptr, "TEST", std::move(layout));
}
