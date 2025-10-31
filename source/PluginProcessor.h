#pragma once

#include "data/Params.h"
#include "backend/Controls.h"
#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif

using APVTS = juce::AudioProcessorValueTreeState;

class PluginProcessor : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS& getAPVTS() noexcept { return apvts; }
    const APVTS& getAPVTS() const noexcept { return apvts; }

    // Access to domain layer
    Controls& getControls() noexcept { return controls; }
    const Controls& getControls() const noexcept { return controls; }


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)

    // ========================================
    // Application Layer - Controller
    // ========================================

    // Persistence layer (APVTS)
    APVTS apvts;

    // Domain layer
    Controls controls;

    // Helper function to create parameter layout
    static APVTS::ParameterLayout createParameterLayout();

    // ========================================
    // Sync Methods: APVTS ↔ Controls
    // ========================================

    // Called when APVTS parameter changes (from UI or preset load)
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Sync all Controls state → APVTS (for saving presets)
    void syncControlsToApvts();

    // Sync all APVTS → Controls state (for loading presets)
    void syncApvtsToControls();
};
