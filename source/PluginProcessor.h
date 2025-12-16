#pragma once

#include "data/Params.h"
#include "backend/Sequence.h"
#include "backend/Clock.h"
#include "backend/DawSync.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>

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
    Sequence& getSequence() noexcept { return sequence; }
    const Sequence& getSequence() const noexcept { return sequence; }

    Clock& getClock() noexcept { return clock; }
    const Clock& getClock() const noexcept { return clock; }

    // Callback for UI updates when sequence ticks (called from audio thread)
    // The callback should be thread-safe (e.g., trigger an AsyncUpdater)
    std::function<void()> onSequenceTick;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)

    // ========================================
    // Application Layer - Controller
    // ========================================

    // Persistence layer (APVTS)
    APVTS apvts;

    // Domain layer
    Sequence sequence;
    Clock clock;
    DawSync dawSync;  // DAW synchronization utility

    // MIDI state
    int lastPlayedNote = -1;  // Track last note for note-off

    // Direct MIDI output for standalone mode
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // Helper function to create parameter layout
    static APVTS::ParameterLayout createParameterLayout();

    // ========================================
    // Sync Methods: APVTS ↔ State
    // ========================================

    // Called when APVTS parameter changes (from UI or preset load)
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Sync all State → APVTS (for saving presets)
    void syncStateToApvts();

    // Sync all APVTS → State (for loading presets)
    void syncApvtsToState();
};
