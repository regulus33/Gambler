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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
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
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
        // ..do something to the data...
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
    // This is called when a parameter changes (from UI or automation)
    // Update the corresponding State property

    using namespace Params::Keys::ID;

    // Bottom row buttons
    if (parameterID == ROLL)
        state.setRollPressed (newValue > 0.5f);
    else if (parameterID == INDEX_LEFT)
        state.setIndexLeftPressed (newValue > 0.5f);
    else if (parameterID == INDEX_RIGHT)
        state.setIndexRightPressed (newValue > 0.5f);
    else if (parameterID == LENGTH_MINUS)
        state.setLengthMinusPressed (newValue > 0.5f);
    else if (parameterID == LENGTH_PLUS)
        state.setLengthPlusPressed (newValue > 0.5f);
    else if (parameterID == NUDGE_LEFT)
        state.setNudgeLeftPressed (newValue > 0.5f);
    else if (parameterID == NUDGE_RIGHT)
        state.setNudgeRightPressed (newValue > 0.5f);
    else if (parameterID == SHIFT)
        state.setShiftPressed (newValue > 0.5f);

    // Top right buttons
    else if (parameterID == EXPORT)
        state.setExportPressed (newValue > 0.5f);
    else if (parameterID == SAVE)
        state.setSavePressed (newValue > 0.5f);
    else if (parameterID == LOAD)
        state.setLoadPressed (newValue > 0.5f);

    // Sub notes buttons
    else if (parameterID == SUB_NOTES_PLUS)
        state.setSubNotesPlusPressed (newValue > 0.5f);
    else if (parameterID == SUB_NOTES_MINUS)
        state.setSubNotesMinusPressed (newValue > 0.5f);

    // Main controls
    else if (parameterID == OCTAVE)
        state.setOctave (static_cast<int> (newValue));
    else if (parameterID == NOTE)
        state.setNote (static_cast<int> (newValue));
    else if (parameterID == PROBABILITY)
        state.setProbability (static_cast<int> (newValue));

    // MIDI mode
    else if (parameterID == MIDI_MODE) {
        state.setMidiMode (newValue > 0.5f ? State::MidiMode::NOTE_TO_CC : State::MidiMode::CC_TO_NOTE);
    }

    // Note range
    else if (parameterID == NOTE_RANGE_MIN)
        state.setNoteRangeMin (static_cast<int> (newValue));
    else if (parameterID == NOTE_RANGE_MAX)
        state.setNoteRangeMax (static_cast<int> (newValue));

    // Step buttons
    else {
        for (int i = 0; i < State::NUM_STEP_BUTTONS; ++i) {
            if (parameterID == stepButtonSelected (i)) {
                state.getStepButton (i).setSelected (newValue > 0.5f);
                return;
            } else if (parameterID == stepButtonHighlighted (i)) {
                state.getStepButton (i).setHighlighted (newValue > 0.5f);
                return;
            }
        }

        // CC controls
        for (int i = 0; i < State::NUM_CC_SLOTS; ++i) {
            if (parameterID == ccNumber (i)) {
                state.setCCNumber (i, static_cast<int> (newValue));
                return;
            } else if (parameterID == ccValue (i)) {
                state.setCCValue (i, static_cast<int> (newValue));
                return;
            }
        }
    }
}

void PluginProcessor::syncStateToApvts() {
    // Sync State → APVTS (for saving presets)
    // This updates APVTS from the current State

    using namespace Params::Keys::ID;

    // Bottom row buttons
    apvts.getParameter (ROLL)->setValueNotifyingHost (state.isRollPressed() ? 1.0f : 0.0f);
    apvts.getParameter (INDEX_LEFT)->setValueNotifyingHost (state.isIndexLeftPressed() ? 1.0f : 0.0f);
    apvts.getParameter (INDEX_RIGHT)->setValueNotifyingHost (state.isIndexRightPressed() ? 1.0f : 0.0f);
    apvts.getParameter (LENGTH_MINUS)->setValueNotifyingHost (state.isLengthMinusPressed() ? 1.0f : 0.0f);
    apvts.getParameter (LENGTH_PLUS)->setValueNotifyingHost (state.isLengthPlusPressed() ? 1.0f : 0.0f);
    apvts.getParameter (NUDGE_LEFT)->setValueNotifyingHost (state.isNudgeLeftPressed() ? 1.0f : 0.0f);
    apvts.getParameter (NUDGE_RIGHT)->setValueNotifyingHost (state.isNudgeRightPressed() ? 1.0f : 0.0f);
    apvts.getParameter (SHIFT)->setValueNotifyingHost (state.isShiftPressed() ? 1.0f : 0.0f);

    // Top right buttons
    apvts.getParameter (EXPORT)->setValueNotifyingHost (state.isExportPressed() ? 1.0f : 0.0f);
    apvts.getParameter (SAVE)->setValueNotifyingHost (state.isSavePressed() ? 1.0f : 0.0f);
    apvts.getParameter (LOAD)->setValueNotifyingHost (state.isLoadPressed() ? 1.0f : 0.0f);

    // Sub notes buttons
    apvts.getParameter (SUB_NOTES_PLUS)->setValueNotifyingHost (state.isSubNotesPlusPressed() ? 1.0f : 0.0f);
    apvts.getParameter (SUB_NOTES_MINUS)->setValueNotifyingHost (state.isSubNotesMinusPressed() ? 1.0f : 0.0f);

    // Main controls (need to normalize to 0-1 range)
    auto* octaveParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (OCTAVE));
    auto* noteParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (NOTE));
    auto* probabilityParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (PROBABILITY));

    if (octaveParam)
        octaveParam->operator= (state.getOctave());
    if (noteParam)
        noteParam->operator= (state.getNote());
    if (probabilityParam)
        probabilityParam->operator= (state.getProbability());

    // MIDI mode
    auto* midiModeParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (MIDI_MODE));
    if (midiModeParam) {
        midiModeParam->operator= (state.getMidiMode() == State::MidiMode::CC_TO_NOTE ? 0 : 1);
    }

    // Note range
    auto* noteRangeMinParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (NOTE_RANGE_MIN));
    auto* noteRangeMaxParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (NOTE_RANGE_MAX));

    if (noteRangeMinParam)
        noteRangeMinParam->operator= (state.getNoteRangeMin());
    if (noteRangeMaxParam)
        noteRangeMaxParam->operator= (state.getNoteRangeMax());

    // Step buttons
    for (int i = 0; i < State::NUM_STEP_BUTTONS; ++i) {
        const auto& button = state.getStepButton (i);
        apvts.getParameter (stepButtonSelected (i))->setValueNotifyingHost (button.isSelected() ? 1.0f : 0.0f);
        apvts.getParameter (stepButtonHighlighted (i))->setValueNotifyingHost (button.isHighlighted() ? 1.0f : 0.0f);
    }

    // CC controls
    for (int i = 0; i < State::NUM_CC_SLOTS; ++i) {
        auto* ccNumParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (ccNumber (i)));
        auto* ccValParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (ccValue (i)));

        if (ccNumParam)
            ccNumParam->operator= (state.getCCNumber (i));
        if (ccValParam)
            ccValParam->operator= (state.getCCValue (i));
    }
}

void PluginProcessor::syncApvtsToState() {
    // Sync APVTS → State (for loading presets)
    // This updates State from the current APVTS state

    using namespace Params::Keys::ID;

    // Bottom row buttons
    state.setRollPressed (apvts.getRawParameterValue (ROLL)->load() > 0.5f);
    state.setIndexLeftPressed (apvts.getRawParameterValue (INDEX_LEFT)->load() > 0.5f);
    state.setIndexRightPressed (apvts.getRawParameterValue (INDEX_RIGHT)->load() > 0.5f);
    state.setLengthMinusPressed (apvts.getRawParameterValue (LENGTH_MINUS)->load() > 0.5f);
    state.setLengthPlusPressed (apvts.getRawParameterValue (LENGTH_PLUS)->load() > 0.5f);
    state.setNudgeLeftPressed (apvts.getRawParameterValue (NUDGE_LEFT)->load() > 0.5f);
    state.setNudgeRightPressed (apvts.getRawParameterValue (NUDGE_RIGHT)->load() > 0.5f);
    state.setShiftPressed (apvts.getRawParameterValue (SHIFT)->load() > 0.5f);

    // Top right buttons
    state.setExportPressed (apvts.getRawParameterValue (EXPORT)->load() > 0.5f);
    state.setSavePressed (apvts.getRawParameterValue (SAVE)->load() > 0.5f);
    state.setLoadPressed (apvts.getRawParameterValue (LOAD)->load() > 0.5f);

    // Sub notes buttons
    state.setSubNotesPlusPressed (apvts.getRawParameterValue (SUB_NOTES_PLUS)->load() > 0.5f);
    state.setSubNotesMinusPressed (apvts.getRawParameterValue (SUB_NOTES_MINUS)->load() > 0.5f);

    // Main controls
    state.setOctave (static_cast<int> (apvts.getRawParameterValue (OCTAVE)->load()));
    state.setNote (static_cast<int> (apvts.getRawParameterValue (NOTE)->load()));
    state.setProbability (static_cast<int> (apvts.getRawParameterValue (PROBABILITY)->load()));

    // MIDI mode
    int midiModeValue = static_cast<int> (apvts.getRawParameterValue (MIDI_MODE)->load());
    state.setMidiMode (midiModeValue == 0 ? State::MidiMode::CC_TO_NOTE : State::MidiMode::NOTE_TO_CC);

    // Note range
    state.setNoteRangeMin (static_cast<int> (apvts.getRawParameterValue (NOTE_RANGE_MIN)->load()));
    state.setNoteRangeMax (static_cast<int> (apvts.getRawParameterValue (NOTE_RANGE_MAX)->load()));

    // Step buttons
    for (int i = 0; i < State::NUM_STEP_BUTTONS; ++i) {
        state.getStepButton (i).setSelected (apvts.getRawParameterValue (stepButtonSelected (i))->load() > 0.5f);
        state.getStepButton (i).setHighlighted (apvts.getRawParameterValue (stepButtonHighlighted (i))->load() > 0.5f);
    }

    // CC controls
    for (int i = 0; i < State::NUM_CC_SLOTS; ++i) {
        state.setCCNumber (i, static_cast<int> (apvts.getRawParameterValue (ccNumber (i))->load()));
        state.setCCValue (i, static_cast<int> (apvts.getRawParameterValue (ccValue (i))->load()));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
