#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    #endif
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts (*this, nullptr, "Parameters", createParameterLayout()) {
    // Initialize State from APVTS (in case we're loading a saved state)
    syncApvtsToState();

    // Register as listener for all parameter changes
    // This ensures State stays in sync when parameters change from UI
    for (auto* param : apvts.processor.getParameters()) {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*> (param)) {
            apvts.addParameterListener (paramWithID->paramID, this);
        }
    }

    // Initialize clock: internal mode at 120 BPM, started by default
    clock.setMode(ClockMode::INTERNAL);
    clock.setBpm(120.0);
    clock.setSubdivision(Subdivision::SIXTEENTH);
    clock.start();

    // Enable random mode so notes are randomized within the scale
    sequence.setIsRandom(true);

    // Open first available MIDI output device for standalone mode
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    if (!midiOutputs.isEmpty()) {
        for (const auto& device : midiOutputs) {
            DBG("Found MIDI output device: " << device.name);
        }
        midiOutput = juce::MidiOutput::openDevice(midiOutputs[0].identifier);
        if (midiOutput) {
            DBG("Opened MIDI output: " << midiOutputs[0].name);
        }
    } else {
        DBG("No MIDI output devices found!");
    }
}

APVTS::ParameterLayout PluginProcessor::createParameterLayout() {
    return Params::createDefaultParameterLayout();
}

PluginProcessor::~PluginProcessor() {
    // Unregister parameter listeners
    for (auto* param : apvts.processor.getParameters()) {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*> (param)) {
            apvts.removeParameterListener (paramWithID->paramID, this);
        }
    }
}

//==============================================================================
const juce::String PluginProcessor::getName() const {
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int PluginProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram() {
    return 0;
}

void PluginProcessor::setCurrentProgram (int index) {
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index) {
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName) {
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused (samplesPerBlock);

    // Set sample rate for internal clock timing
    clock.setSampleRate(sampleRate);
}

void PluginProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Clear incoming MIDI (we're generating, not processing)
    midiMessages.clear();

    // Process the internal clock
    clock.processInternalClock(buffer.getNumSamples());

    // Check if clock triggered a tick
    if (clock.shouldTick()) {
        // Send note off for previous note (if any)
        if (lastPlayedNote >= 0) {
            auto noteOff = juce::MidiMessage::noteOff(1, lastPlayedNote, (juce::uint8)0);
            midiMessages.addEvent(noteOff, 0);

            // Send directly to MIDI output device (for standalone mode)
            if (midiOutput) {
                midiOutput->sendMessageNow(noteOff);
            }
            DBG("MIDI Note OFF: " << lastPlayedNote);
        }

        // Tick the sequence to get the next note
        int note = sequence.tick();

        DBG("Clock tick! Step: " << sequence.getCurrentIndex() << ", Note: " << note);

        if (note >= 0) {
            // Valid note - send note on
            auto noteOn = juce::MidiMessage::noteOn(1, note, (juce::uint8)100);
            midiMessages.addEvent(noteOn, 0);

            // Send directly to MIDI output device (for standalone mode)
            if (midiOutput) {
                midiOutput->sendMessageNow(noteOn);
            }
            DBG("MIDI Note ON: " << note << " (velocity 100, channel 1)");
            lastPlayedNote = note;
        } else {
            // Rest - no note to play
            lastPlayedNote = -1;
        }

        // Notify UI that a tick occurred (thread-safe via AsyncUpdater)
        if (onSequenceTick) {
            onSequenceTick();
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData) {
    // Sync State → APVTS before saving
    syncStateToApvts();

    // Save APVTS state
    auto valueTreeState = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (valueTreeState.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // Load APVTS state
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr) {
        if (xmlState->hasTagName (apvts.state.getType())) {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

            // Sync APVTS → State after loading
            syncApvtsToState();
        }
    }
}

//==============================================================================
// Sync Methods: APVTS ↔ State
//==============================================================================

void PluginProcessor::parameterChanged (const juce::String& parameterID, float newValue) {
    // TODO: Implement APVTS → Sequence sync when UI is ready
    juce::ignoreUnused (parameterID, newValue);
}

void PluginProcessor::syncStateToApvts() {
    // TODO: Implement Sequence → APVTS sync for preset saving
}

void PluginProcessor::syncApvtsToState() {
    // TODO: Implement APVTS → Sequence sync for preset loading
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
