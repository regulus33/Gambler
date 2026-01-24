#pragma once

#include "backend/Clock.h"
#include "backend/DawSync.h"
#include "backend/MidiLearnRegistry.h"
#include "backend/PresetManager.h"
#include "backend/ProcessorCommand.h"
#include "backend/SequenceState.h"
#include "backend/UserPreferences.h"
#include "data/Params.h"
#include <juce_audio_devices/juce_audio_devices.h>
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

    // Access to APVTS-backed state (thread-safe, single source of truth)
    SequenceState& getState() noexcept { return *sequenceState; }
    const SequenceState& getState() const noexcept { return *sequenceState; }

    Clock& getClock() noexcept { return clock; }
    const Clock& getClock() const noexcept { return clock; }

    // Callback for UI updates when sequence ticks (called from audio thread)
    // The callback should be thread-safe (e.g., trigger an AsyncUpdater)
    std::function<void()> onSequenceTick;

    // Callback for notifying when command completion callbacks are pending
    // Called from audio thread - should trigger an AsyncUpdater to execute
    // completion callbacks on the message thread
    std::function<void()> onCompletionCallbacksPending;

    // Callback for UI updates when MIDI learn mapping is created (called from audio thread)
    // The callback should be thread-safe (e.g., trigger an AsyncUpdater)
    std::function<void()> onMidiLearned;

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
     *
     * @param noteOverride If >= 0, plays this note instead of reading from the step.
     *                     This avoids race conditions when the note was just changed
     *                     via a queued command that hasn't executed yet.
     */
    void triggerCurrentStepMidi(int noteOverride = -1);

    /**
     * Sequencer playback states
     */
    enum class SequencerState {
        Stopped,  // Not playing, index at 0
        Playing,  // Advancing through sequence
        Paused    // Holding at current index
    };

    /**
     * Toggle play/pause
     * - If stopped: starts playing from index 0
     * - If playing: pauses at current index
     * - If paused: resumes playing from current index
     * Thread-safe - can be called from any thread.
     */
    void togglePlayPause();

    /**
     * Stop the sequencer playback
     * Disables the sequencer from responding to clock ticks and resets to step 0.
     * Thread-safe - can be called from any thread.
     */
    void stopSequencer();

    /**
     * Get the current sequencer state
     * @return Current state (Stopped, Playing, or Paused)
     */
    SequencerState getSequencerState() const { return sequencerState.load(); }

    // User preferences (persisted with project)
    UserPreferences& getUserPreferences() noexcept { return userPreferences; }
    const UserPreferences& getUserPreferences() const noexcept { return userPreferences; }

    // Preset manager
    PresetManager& getPresetManager() noexcept { return presetManager; }
    const PresetManager& getPresetManager() const noexcept { return presetManager; }

    // MIDI learn registry
    MidiLearnRegistry& getMidiLearnRegistry() noexcept { return midiLearnRegistry; }
    const MidiLearnRegistry& getMidiLearnRegistry() const noexcept { return midiLearnRegistry; }

    /**
     * Get the current plugin state as XML (for preset saving)
     * This creates a complete snapshot of all state that should be saved.
     */
    std::unique_ptr<juce::XmlElement> getStateAsXml();

    /**
     * Load plugin state from XML (for preset loading)
     * This restores all state from a previously saved snapshot.
     * Should be called from the message thread - will queue commands for audio thread.
     */
    void loadStateFromXml(const juce::XmlElement& xml);

    /**
     * Queue a command to be executed on the audio thread
     * This is the thread-safe way to modify domain state from the UI thread.
     * Returns false if the queue is full (command was dropped).
     */
    bool queueCommand(ProcessorCommand command) {
        return commandQueue.push(std::move(command));
    }

    /**
     * Execute any pending completion callbacks (called from message thread)
     * This should be called from handleAsyncUpdate() after onCompletionCallbacksPending triggers.
     * Returns the number of callbacks executed.
     */
    int executeCompletionCallbacks() {
        return commandQueue.executeCompletionCallbacks();
    }

    /**
     * Open all available MIDI devices for clock output (standalone mode only)
     * This should be called when the user wants to send MIDI clock to all devices.
     */
    void openAllMidiClockOutputs();

    /**
     * Close all MIDI clock output devices
     */
    void closeAllMidiClockOutputs();

    /**
     * Get list of available MIDI output device names
     */
    juce::StringArray getAvailableMidiOutputDeviceNames() const;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)

    // ========================================
    // Application Layer - Controller
    // ========================================

    // Persistence layer (APVTS) - single source of truth for all state
    APVTS apvts;

    // APVTS-backed state accessor (thread-safe)
    std::unique_ptr<SequenceState> sequenceState;

    // Clock and DAW synchronization
    Clock clock; // clock just tells us if we shouldTick()
    DawSync dawSync; // daw sing

    // User preferences (persisted with project)
    UserPreferences userPreferences;

    // Preset manager
    PresetManager presetManager;

    // Sequencer playback state
    std::atomic<SequencerState> sequencerState { SequencerState::Playing };  // Start playing by default

    // Track DAW transport state for auto-resume in external clock mode
    bool previousDawIsPlaying = false;

    // MIDI state
    int lastPlayedNote = -1;  // Track last note for note-off

    // Note-off scheduling (for timed note length)
    juce::int64 currentSamplePosition = 0;  // Current sample position in audio stream
    juce::int64 scheduledNoteOffSample = -1;  // Sample position when note-off should be sent (-1 = no note-off scheduled)
    int scheduledNoteOffNote = -1;  // Note number to send note-off for (-1 = none)
    juce::uint8 scheduledNoteOffVelocity = 0;  // Velocity for note-off

    // Direct MIDI output for standalone mode
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // MIDI clock output devices (for sending clock to all devices in standalone mode)
    std::vector<std::unique_ptr<juce::MidiOutput>> midiClockOutputs;

    // Track MIDI clock timing
    int midiClockPulsesSinceLastTick = 0;
    static constexpr int MIDI_CLOCKS_PER_TICK = 6;  // 24 PPQN = 6 clocks per 16th note at default subdivision

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

    // MIDI learn registry for user-configurable mappings
    MidiLearnRegistry midiLearnRegistry;

    // Helper function to create parameter layout
    static APVTS::ParameterLayout createParameterLayout();

    // Called when APVTS parameter changes (from UI or preset load)
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // MIDI input processing
    /**
     * Process incoming MIDI messages and trigger mapped controls
     * Called from processBlock() before clearing the MIDI buffer
     * @param midiMessages Incoming MIDI buffer from DAW/hardware
     */
    void processMidiInput(const juce::MidiBuffer& midiMessages);

    /**
     * Handle a single MIDI mapping by queuing the appropriate command
     * @param mapping The MIDI mapping configuration
     * @param midiValue The MIDI value (0-127)
     * @param isNoteOnOrCCPositive True for Note On or positive CC value
     */
    void handleMidiMapping(const MidiMapping& mapping, int midiValue, bool isNoteOnOrCCPositive);
};
