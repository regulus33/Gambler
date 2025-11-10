#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    // Add piano roll component
    addAndMakeVisible(pianoRoll);

    // Setup callback for scale changes
    pianoRoll.onNoteChanged = []() {
        // TODO: Update backend Scale object when we wire up the backend
        DBG("Piano roll scale changed");
    };

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Set editor size to match the design (approximately 980 x 560 based on the mockup)
    setSize (980, 560);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Background - dark grey matching the design
    g.fillAll (juce::Colour(0xFF1E1E1E));
}

void PluginEditor::resized()
{
    // Position piano roll in top-left corner with some padding
    // Based on the design mockup, it's positioned at approximately (50, 95)
    pianoRoll.setBounds(50, 95, 408, 199);

    // Inspector button in bottom right corner
    inspectButton.setBounds(getWidth() - 110, getHeight() - 35, 100, 25);
}
