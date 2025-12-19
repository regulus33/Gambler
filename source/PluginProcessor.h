#pragma once

#include "data/Params.h"
#include "backend/Sequence.h"
#include "backend/Clock.h"
#include "backend/DawSync.h"
#include "backend/UserPreferences.h"
#include "backend/ProcessorCommand.h"
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

    /**
     * Trigger MIDI note for a specific step (for UI preview/audition)
     * This sets the current index to the given step and sends MIDI note on/off.
     * Called from UI thread when user clicks a step or edits note controls.
     * @param stepIndex The step index to trigger (0-63)
     */
    void triggerStepMidi(int stepIndex);

    /**
     * Trigger MIDI note for the current step (for UI preview/audition)
     * Sends MIDI note on/off for whatever note is at the current step.
     * Called from UI thread when user edits note/octave sliders.
     */
    void triggerCurrentStepMidi();

    // User preferences (persisted with project)
    UserPreferences& getUserPreferences() noexcept { return userPreferences; }
    const UserPreferences& getUserPreferences() const noexcept { return userPreferences; }

    /**
     * Queue a command to be executed on the audio thread
     * This is the thread-safe way to modify domain state from the UI thread.
     * Returns false if the queue is full (command was dropped).
     */
    bool queueCommand(ProcessorCommand command) {
        return commandQueue.push(std::move(command));
    }

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

    // User preferences (persisted with project)
    UserPreferences userPreferences;

    // MIDI state
    int lastPlayedNote = -1;  // Track last note for note-off

    // Direct MIDI output for standalone mode
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // Preview MIDI queue (for UI-triggered notes in DAW mode)
    // Thread-safe queue to pass MIDI from UI thread to audio thread
    struct PreviewMidiMessage {
        int noteNumber;
        bool isNoteOn;
        juce::uint8 velocity;
    };
    juce::AbstractFifo previewMidiFifo { 32 };  // Ring buffer for up to 32 messages
    std::array<PreviewMidiMessage, 32> previewMidiBuffer;

    // Command queue for thread-safe UI → Audio communication
    ProcessorCommandQueue commandQueue;

    // Helper function to create parameter layout
    static APVTS::ParameterLayout createParameterLayout();

    // Called when APVTS parameter changes (from UI or preset load)
    void parameterChanged(const juce::String& parameterID, float newValue) override;
};
