#pragma once
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdlib>
#include <ctime>

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

// Warning don't call this too much bc it seeds on every call which is slow I think. For high performance seed once at start.
static int getRandFromZeroToExcluding(int max) {
    srand(time(0));
    return rand() % max;
}
