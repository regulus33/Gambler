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

    // Initialize APVTS-backed state accessor (with InternalState for non-MIDI-mappable params)
    sequenceState = std::make_unique<SequenceState> (apvts, internalState);

    // Register as listener for all parameter changes
    for (auto* param : apvts.processor.getParameters()) {
        // we dynamic cast the param because we don't know what type it will be and RangedAudioParameter is the top parent of AudioParameterBool and so on.
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*> (param)) {
            // we tell apvts to call this class when any param changes. All we need to pass is the param id. This tells us exactly which parameter to look up and grab the change of.
            apvts.addParameterListener (paramWithID->paramID, this);
        }
    }

    // Initialize clock settings based on standalone vs plugin mode.
    // Use wrapperType instead of PluginHostType for more reliable detection,
    // especially for AUv3 on iOS where PluginHostType may not work correctly.
    bool isStandalone = (wrapperType == wrapperType_Standalone);

    if (isStandalone) {
        // Standalone app - use internal clock
        clock.setMode (ClockMode::INTERNAL);
        // start the clock
        clock.start();
        // set the clock mode in the apvts (TODO: use enum)
        sequenceState->setClockMode (0); // 0 = INTERNAL
    } else {
        // Running in a DAW - use DAW sync
        clock.setMode (ClockMode::DAW_SYNC);
        // set the clock mode
        sequenceState->setClockMode (1); // 1 = DAW_SYNC (TODO: use enum)
    }
    // clock.setBpm(120.0); this isn't necessary because its set by default, but putting it here for completeness.
    clock.setSubdivision (Subdivision::SIXTEENTH);

    // Open first available MIDI output device for standalone mode
    if (isStandalone) {
        // get the available devices from juce api
        auto midiOutputs = juce::MidiOutput::getAvailableDevices();
        // if we find any
        if (!midiOutputs.isEmpty()) {
            // just log them all in dev
            for (const auto& device : midiOutputs) {
                DBG ("Found MIDI output device: " << device.name);
            }
            // TODO: make this configurable through a dropdown in settings
            midiOutput = juce::MidiOutput::openDevice (midiOutputs[0].identifier);
            if (midiOutput) {
                // TODO: make this configurable through a dropdown in settings
                DBG ("Opened MIDI output: " << midiOutputs[0].name);
            }
        } else {
            // if we have nothing plugged in
            DBG ("No MIDI output devices found!");
        }
    }

}

// Generate the param layout for the audio processor value tree.
// remember, createParameterLayout is simply the argument to APVTS() constructor
APVTS::ParameterLayout PluginProcessor::createParameterLayout() {
    // build up that whole param map
    return Params::createDefaultParameterLayout();
}

// The destructor is called
PluginProcessor::~PluginProcessor() {
    // Unregister parameter listeners
    for (auto* param : apvts.processor.getParameters()) {
        // remove listeners
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
    clock.setSampleRate (sampleRate);
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

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    drainVirtualKeyboard(midiMessages, buffer.getNumSamples());
    processMidiInput (midiMessages);
    midiMessages.clear();

    previewMidiFifo.drainInto (midiMessages, getMidiOutputChannel());

    handleNoteOffScheduling(midiMessages, buffer.getNumSamples());
    const int blockSize = buffer.getNumSamples();
    currentSamplePosition += blockSize;

    processClockForBlock(blockSize);
    emitMidiClockPulses();
    detectDawTransport();

    if (sequencerState.load() == PlaybackState::Playing) {
        for (int sampleOffset : clock.drainPulses())
            tickSequencer(midiMessages, sampleOffset, blockSize);
    } else {
        clock.drainPulses();  // discard so they don't pile up while paused
    }
}

void PluginProcessor::drainVirtualKeyboard(juce::MidiBuffer& midiMessages, int numSamples) {
    keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);
}

void PluginProcessor::handleNoteOffScheduling(juce::MidiBuffer& midiMessages, int numSamples) {
    if (noteOffScheduler.processScheduledNoteOff(midiMessages, getMidiOutputChannel(),
                                                  midiOutput.get(), currentSamplePosition,
                                                  numSamples)) {
        auto noteOffCallback = onNoteOff;
        if (noteOffCallback)
            noteOffCallback();
    }
}

void PluginProcessor::processClockForBlock(int numSamples) {
    if (clock.getMode() == ClockMode::DAW_SYNC) {
        if (auto* playHead = getPlayHead()) {
            auto position = playHead->getPosition();
            auto syncResult = dawSync.update(position, getSampleRate(), numSamples);
            clock.enqueueDawPulses(syncResult.currentPulseCount, syncResult.newPulseOffsets);
        }
    } else {
        clock.processInternalClock(numSamples);
    }
}

void PluginProcessor::emitMidiClockPulses() {
    if (clock.getMode() != ClockMode::INTERNAL || midiClockOutputs.empty())
        return;

    int midiClockPulses = clock.getMidiClockPulsesToSend();
    for (int i = 0; i < midiClockPulses; ++i) {
        auto clockMsg = juce::MidiMessage::midiClock();
        for (auto& output : midiClockOutputs)
            if (output)
                output->sendMessageNow(clockMsg);
    }
}

void PluginProcessor::detectDawTransport() {
    if (clock.getMode() != ClockMode::DAW_SYNC)
        return;

    auto* playHead = getPlayHead();
    if (!playHead)
        return;

    auto position = playHead->getPosition();
    bool dawIsPlaying = position.hasValue() && position->getIsPlaying();

    if (dawIsPlaying && !previousDawIsPlaying) {
        auto currentState = sequencerState.load();
        if (currentState == PlaybackState::Stopped || currentState == PlaybackState::Paused) {
            sequencerState.store(PlaybackState::Playing);
            DBG("DAW started - sequencer auto-resumed");
        }
    }
    previousDawIsPlaying = dawIsPlaying;
}

double PluginProcessor::computeCurrentDawBpm() {
    if (clock.getMode() != ClockMode::DAW_SYNC)
        return 0.0;

    auto* playHead = getPlayHead();
    if (!playHead)
        return 0.0;

    auto position = playHead->getPosition();
    if (position.hasValue() && position->getBpm().hasValue())
        return *position->getBpm();
    return 0.0;
}

void PluginProcessor::emitCcModulations(juce::MidiBuffer& midiMessages,
                                         const std::vector<StepModulationData>& mods,
                                         juce::uint8& velocityOut,
                                         int sampleOffsetInBlock) {
    for (const auto& mod : mods) {
        if (mod.getType() == ModulationType::Velocity) {
            velocityOut = static_cast<juce::uint8>(mod.getValue());
        } else if (mod.getType() == ModulationType::CC) {
            auto ccMsg = juce::MidiMessage::controllerEvent(getMidiOutputChannel(), mod.getCCNumber(), mod.getValue());
            midiMessages.addEvent(ccMsg, sampleOffsetInBlock);
            if (midiOutput)
                midiOutput->sendMessageNow(ccMsg);
            DBG ("MIDI CC: " << mod.getCCNumber() << " = " << mod.getValue());
        }
    }
}

void PluginProcessor::tickSequencer(juce::MidiBuffer& midiMessages,
                                     int sampleOffsetInBlock,
                                     int blockSize) {
    if (clock.getMode() == ClockMode::INTERNAL && !midiClockOutputs.empty())
        clock.accumulateMidiClockPulse();

    noteOffScheduler.sendImmediateNoteOff(midiMessages, getMidiOutputChannel(),
                                            midiOutput.get(), sampleOffsetInBlock);

    auto& state = getState();
    int note = state.tick();
    int currentStepIndex = state.getCurrentIndex();

    DBG ("Clock tick! Step: " << currentStepIndex << ", Note: " << note
         << " (block offset: " << sampleOffsetInBlock << ")");

    // currentSamplePosition was bumped to end-of-block at the top of processBlock,
    // so the absolute sample position of this pulse is (end - blockSize + offset).
    const juce::int64 blockStartSample = currentSamplePosition - static_cast<juce::int64>(blockSize);
    const juce::int64 noteOnSample     = blockStartSample + static_cast<juce::int64>(sampleOffsetInBlock);

    if (note >= 0) {
        auto selectedMods = state.getSelectedModulations(currentStepIndex);
        juce::uint8 velocity = 100;
        emitCcModulations(midiMessages, selectedMods, velocity, sampleOffsetInBlock);

        auto noteOn = juce::MidiMessage::noteOn(getMidiOutputChannel(), note, velocity);
        midiMessages.addEvent(noteOn, sampleOffsetInBlock);
        if (midiOutput)
            midiOutput->sendMessageNow(noteOn);
        DBG ("MIDI Note ON: " << note << " (velocity " << (int) velocity << ", channel 1)");
        noteOffScheduler.setLastPlayedNote(note);

        double samplesPerStep = clock.calculateSamplesPerStep(computeCurrentDawBpm(), getSampleRate());
        if (samplesPerStep > 0.0) {
            double noteLengthPercent = state.getNoteLength();
            juce::int64 noteOffDelay = static_cast<juce::int64>(samplesPerStep * noteLengthPercent);
            noteOffScheduler.scheduleNoteOff(note, noteOnSample + noteOffDelay);
            DBG ("Scheduled note-off for note " << note
                 << " (delay: " << noteOffDelay << " samples, " << (noteLengthPercent * 100.0) << "% of step)");
        }
    } else {
        noteOffScheduler.setLastPlayedNote(-1);
        if (state.getIsRandom())
            DBG ("Rest recorded at step: " << currentStepIndex);
    }

    auto callback = onSequenceTick;
    if (callback)
        callback();
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
    // Create root XML element to hold all state
    auto rootXml = std::make_unique<juce::XmlElement> ("GamblerState");

    // Save APVTS parameters (MIDI-mappable state)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement (apvtsXml.release());

    // Save InternalState (non-MIDI-mappable state: step notes, modulations, etc.)
    auto internalStateXml = internalState.getState().createXml();
    rootXml->addChildElement (internalStateXml.release());

    // Save Clock settings (mode, BPM, subdivision)
    rootXml->addChildElement (clock.toXml().release());

    // Save MIDI Learn Mappings
    rootXml->addChildElement (midiLearnRegistry.toXml().release());

    // Convert to binary and save
    copyXmlToBinary (*rootXml, destData);

    DBG ("State saved: APVTS, InternalState, Clock, and MIDI Learn Mappings");
}

// TODO: are we actually using this?
void PluginProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // Load XML from binary data
    std::unique_ptr<juce::XmlElement> rootXml (getXmlFromBinary (data, sizeInBytes));

    if (rootXml != nullptr) {
        // Check if this is the new format (GamblerState root element)
        if (rootXml->hasTagName ("GamblerState")) {
            DBG ("Loading state: GamblerState format");

            // Load APVTS parameters (MIDI-mappable state)
            if (auto* apvtsXml = rootXml->getChildByName (apvts.state.getType())) {
                apvts.replaceState (juce::ValueTree::fromXml (*apvtsXml));
                DBG ("  - APVTS loaded");
            }

            // Load InternalState (non-MIDI-mappable state: step notes, modulations, etc.)
            if (auto* internalStateXml = rootXml->getChildByName (InternalState::IDs::ROOT_TYPE)) {
                internalState.setState (juce::ValueTree::fromXml (*internalStateXml));
                DBG ("  - InternalState loaded (step notes, modulations, etc.)");
            }

            // Load Clock settings
            if (auto* clockXml = rootXml->getChildByName ("Clock")) {
                clock.fromXml (*clockXml);
                juce::String modeStr = (clock.getMode() == ClockMode::INTERNAL) ? "INTERNAL" : "DAW_SYNC";
                DBG ("  - Clock loaded (mode: " + modeStr + ", BPM: " + juce::String (clock.getBpm()) + ")");
            }

            // Load MIDI Learn Mappings
            if (auto* midiLearnXml = rootXml->getChildByName ("MidiLearnMappings")) {
                midiLearnRegistry.fromXml (*midiLearnXml);
                DBG ("  - MIDI Learn Mappings loaded");
            }

            // Backward compatibility: Load from old "UserPreferences" or "Settings" element
            // These are now stored in APVTS, but we need to migrate old saves
            if (auto* prefsXml = rootXml->getChildByName ("UserPreferences")) {
                // Migrate old UserPreferences to APVTS
                if (auto* param = apvts.getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES)) {
                    param->setValueNotifyingHost(prefsXml->getBoolAttribute("previewNotesOnEdit", true) ? 1.0f : 0.0f);
                }
                if (auto* param = apvts.getParameter(Params::Keys::ID::PREF_FOLLOW_STEP)) {
                    param->setValueNotifyingHost(prefsXml->getBoolAttribute("followStep", false) ? 1.0f : 0.0f);
                }
                if (auto* param = apvts.getParameter(Params::Keys::ID::PREF_NOTE_LENGTH)) {
                    float noteLength = static_cast<float>(prefsXml->getDoubleAttribute("defaultNoteLength", 0.9));
                    param->setValueNotifyingHost(param->convertTo0to1(noteLength));
                }
                DBG ("  - Migrated legacy UserPreferences to APVTS");
            }
            else if (auto* settingsXml = rootXml->getChildByName ("Settings")) {
                // Migrate even older Settings format
                if (auto* param = apvts.getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES)) {
                    param->setValueNotifyingHost(settingsXml->getBoolAttribute("previewNotesOnEdit", true) ? 1.0f : 0.0f);
                }
                DBG ("  - Migrated legacy Settings to APVTS");
            }
        }
        // Fallback: Old format (just APVTS)
        else if (rootXml->hasTagName (apvts.state.getType())) {
            DBG ("Loading state: Legacy APVTS-only format");
            // We shouldn't have anything here ideally.
            jassertfalse;
            apvts.replaceState (juce::ValueTree::fromXml (*rootXml));
        }
    }
}

//==============================================================================
// APVTS Parameter Listener
//==============================================================================

// Note all this is pretty straightforward, we are changing the state based on param changes.
void PluginProcessor::parameterChanged (const juce::String& parameterID, float newValue) {
    const bool isOn = newValue > 0.5f;
    handlePrimaryRowParam (parameterID, isOn);
    handleSecondaryRowParam (parameterID, isOn);
    handleDestructiveParam (parameterID, isOn);
    handlePresetCycleParam (parameterID, isOn);
    handleTransportParam (parameterID, isOn);
    handleClockParam (parameterID, newValue);
}

void PluginProcessor::handlePrimaryRowParam (const juce::String& id, bool isOn) {
    auto& state = getState();

    if (id == Params::Keys::ID::ROLL) {
        state.setIsRandom (isOn);
        DBG ("ROLL " + juce::String (isOn ? "ON" : "OFF") + " - isRandom: " + juce::String (state.getIsRandom() ? "true" : "false"));
        return;
    }
    if (id == Params::Keys::ID::LENGTH_PLUS && isOn) {
        state.increaseLength();
        DBG ("LENGTH_PLUS triggered - new length: " + juce::String (state.getLength()));
        return;
    }
    if (id == Params::Keys::ID::LENGTH_MINUS && isOn) {
        state.decreaseLength();
        DBG ("LENGTH_MINUS triggered - new length: " + juce::String (state.getLength()));
        return;
    }
    if (id == Params::Keys::ID::NUDGE_LEFT && isOn) {
        state.nudgeLeft();
        DBG ("NUDGE_LEFT triggered");
        return;
    }
    if (id == Params::Keys::ID::NUDGE_RIGHT && isOn) {
        state.nudgeRight();
        DBG ("NUDGE_RIGHT triggered");
        return;
    }
    if (id == Params::Keys::ID::SEQ_IS_SHIFT) {
        DBG ("SEQ_IS_SHIFT " + juce::String (isOn ? "ON" : "OFF"));
    }
}

void PluginProcessor::handleSecondaryRowParam (const juce::String& id, bool isOn) {
    if (! isOn) return;
    auto& state = getState();

    if (id == Params::Keys::ID::DUPLICATE) {
        const int currentLength = state.getLength();
        const int newLength = juce::jmin (currentLength * 2, ParamConstants::MAX_STEPS);
        const int stepsToCopy = newLength - currentLength;
        for (int i = 0; i < stepsToCopy; i++) {
            const int dstIdx = currentLength + i;
            state.setStepNotesLength (dstIdx, state.getStepNotesLength (i));
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote (dstIdx, n, state.getStepNote (i, n));
            }
        }
        state.setLength (newLength);
        DBG ("DUPLICATE triggered - new length: " + juce::String (newLength));
        return;
    }
    if (id == Params::Keys::ID::CLEAR_STEP) {
        const int selectedStep = state.getSelectedStepForEditing();
        int sub = state.getSelectedSubstepIndex (selectedStep);
        if (sub < 0) sub = 0;
        state.setStepNote (selectedStep, sub, -1);
        DBG ("CLEAR_STEP triggered - cleared step " + juce::String (selectedStep)
             + " substep " + juce::String (sub));
        return;
    }
    if (id == Params::Keys::ID::CLEAR_STEP_ALL) {
        const int selectedStep = state.getSelectedStepForEditing();
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++)
            state.setStepNote (selectedStep, n, -1);
        state.setStepNotesLength (selectedStep, 1);
        DBG ("CLEAR_STEP_ALL triggered - wiped step " + juce::String (selectedStep));
        return;
    }
    if (id == Params::Keys::ID::RANDOMIZE_STEP) {
        const int selectedStep = state.getSelectedStepForEditing();
        state.randomizeStepNote (selectedStep);
        DBG ("RANDOMIZE_STEP triggered - randomized step " + juce::String (selectedStep));
        return;
    }
    if (id == Params::Keys::ID::RANDOMIZE_ALL) {
        for (int i = 0; i < state.getLength(); i++)
            state.randomizeStepNote (i);
        DBG ("RANDOMIZE_ALL triggered");
        return;
    }
    if (id == Params::Keys::ID::SHUFFLE) {
        performShuffle();
        DBG ("SHUFFLE triggered");
    }
}

void PluginProcessor::performShuffle() {
    auto& state = getState();
    const int len = state.getLength();
    static std::random_device rd;
    static std::mt19937 gen (rd());
    for (int i = len - 1; i > 0; i--) {
        std::uniform_int_distribution<> dis (0, i);
        const int j = dis (gen);
        const int lenI = state.getStepNotesLength (i);
        std::array<int, ParamConstants::MAX_SUBSTEPS> notesI{};
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++)
            notesI[static_cast<size_t> (n)] = state.getStepNote (i, n);
        state.setStepNotesLength (i, state.getStepNotesLength (j));
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++)
            state.setStepNote (i, n, state.getStepNote (j, n));
        state.setStepNotesLength (j, lenI);
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++)
            state.setStepNote (j, n, notesI[static_cast<size_t> (n)]);
    }
}

void PluginProcessor::handleDestructiveParam (const juce::String& id, bool isOn) {
    if (! isOn) return;
    if (id == Params::Keys::ID::PREF_DESTRUCTIVE_SCALE) {
        getState().bakeAllExistingNotes();
        DBG ("DESTRUCTIVE_SCALE ON - baked all existing notes to current scale");
    } else if (id == Params::Keys::ID::PREF_DESTRUCTIVE_RANGE) {
        getState().bakeAllExistingNotes();
        DBG ("DESTRUCTIVE_RANGE ON - baked all existing notes to current range");
    }
}

void PluginProcessor::handlePresetCycleParam (const juce::String& id, bool isOn) {
    if (! isOn) return;
    if (id == Params::Keys::ID::PRESET_CYCLE_UP) {
        if (onPresetCycleUp) onPresetCycleUp();
        DBG ("PRESET_CYCLE_UP triggered");
    } else if (id == Params::Keys::ID::PRESET_CYCLE_DOWN) {
        if (onPresetCycleDown) onPresetCycleDown();
        DBG ("PRESET_CYCLE_DOWN triggered");
    }
}

void PluginProcessor::handleTransportParam (const juce::String& id, bool isOn) {
    if (! isOn) return;
    if (id == Params::Keys::ID::PLAY_PAUSE) {
        togglePlayPause();
        if (onPlayPauseTriggered) onPlayPauseTriggered();
        DBG ("PLAY_PAUSE triggered");
    } else if (id == Params::Keys::ID::STOP) {
        stopSequencer();
        if (onStopTriggered) onStopTriggered();
        DBG ("STOP triggered");
    }
}

void PluginProcessor::handleClockParam (const juce::String& id, float newValue) {
    if (id == Params::Keys::ID::CLOCK_MODE) {
        switch (static_cast<int> (newValue)) {
            case 0:
                clock.setMode (ClockMode::INTERNAL);
                clock.start();
                DBG ("Clock mode set to INTERNAL (started)");
                break;
            case 1:
                clock.setMode (ClockMode::DAW_SYNC);
                clock.stop();
                DBG ("Clock mode set to DAW_SYNC");
                break;
            case 2:
                clock.setMode (ClockMode::EXTERNAL_MIDI_CLOCK);
                clock.stop();
                DBG ("Clock mode set to EXTERNAL_MIDI_CLOCK");
                break;
            default:
                break;
        }
        return;
    }
    if (id == Params::Keys::ID::CLOCK_BPM) {
        clock.setBpm (static_cast<double> (newValue));
        DBG ("Clock BPM set to " + juce::String (newValue));
        return;
    }
    if (id == Params::Keys::ID::CLOCK_SUBDIVISION) {
        const auto newSubdivision = static_cast<Subdivision> (static_cast<int> (newValue));
        clock.setSubdivision (newSubdivision);
        dawSync.setSubdivision (newSubdivision);
        DBG ("Clock subdivision set to " + juce::String (static_cast<int> (newValue)));
    }
}

//==============================================================================
// UI Preview Methods
//==============================================================================

void PluginProcessor::triggerStepMidi (int stepIndex) {
    if (stepIndex < 0 || stepIndex >= 64)
        return;

    // A step press takes ownership of the preview channel; cancel any
    // pending slider-preview gate so it can't fire a stale note-off
    // while the user is still holding the step button.
    previewGateTimer.stopTimer();

    auto& state = getState();

    // Set the selected step for editing via APVTS (thread-safe)
    state.setSelectedStepForEditing (stepIndex);

    // Get the selected substep's note for this step from APVTS-backed state
    int note = state.getSelectedNoteForStep (stepIndex);

    // Get step modulations from APVTS-backed state
    std::vector<StepModulationData> selectedMods;
    for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
        if (state.getStepModSelected (stepIndex, i)) {
            StepModulationData mod;
            mod.type = state.getStepModType (stepIndex, i);
            mod.ccNumber = state.getStepModCCNum (stepIndex, i);
            mod.value = state.getStepModValue (stepIndex, i);
            selectedMods.push_back (mod);
        }
    }

    // Determine velocity (default to 100 if no velocity modulation is selected)
    juce::uint8 velocity = 100;
    for (const auto& mod : selectedMods) {
        if (mod.getType() == ModulationType::Velocity) {
            velocity = static_cast<juce::uint8> (mod.getValue());
            break;
        }
    }

    // Send note off for previous note (if any)
    noteOffScheduler.sendPreviewNoteOff (getMidiOutputChannel(), midiOutput.get(), previewMidiFifo);

    // Send note on for the new note
    if (note >= 0) {
        // Send CC messages first (only in standalone mode for now)
        if (midiOutput) {
            for (const auto& mod : selectedMods) {
                if (mod.getType() == ModulationType::CC) {
                    auto ccMsg = juce::MidiMessage::controllerEvent (getMidiOutputChannel(), mod.getCCNumber(), mod.getValue());
                    midiOutput->sendMessageNow (ccMsg);
                    DBG ("Preview MIDI CC (standalone): " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }
        }

        if (midiOutput) {
            auto noteOn = juce::MidiMessage::noteOn (getMidiOutputChannel(), note, velocity);
            midiOutput->sendMessageNow (noteOn);
            DBG ("Preview MIDI Note ON (standalone): " << note << " (velocity " << (int) velocity << ", step " << stepIndex << ")");
        } else {
            previewMidiFifo.push ({ note, true, velocity });
            DBG ("Preview MIDI Note ON queued (DAW): " << note << " (velocity " << (int) velocity << ", step " << stepIndex << ")");
        }
        noteOffScheduler.setLastPlayedNote (note);
    } else {
        noteOffScheduler.setLastPlayedNote (-1);
    }

    // Notify UI that state changed
    // Make a local copy to avoid race condition with UI thread clearing the callback
    auto callback = onSequenceTick;
    if (callback) {
        callback();
    }
}

void PluginProcessor::triggerCurrentStepMidi (int noteOverride) {
    DBG ("=== triggerCurrentStepMidi() called ===");
    DBG ("  midiOutput is " << (midiOutput ? "VALID" : "NULL"));
    DBG ("  noteOverride: " << noteOverride);

    auto& state = getState();

    int selectedStep = state.getSelectedStepForEditing();
    if (selectedStep < 0 || selectedStep >= 64) {
        DBG ("  ERROR: Invalid selected step: " << selectedStep);
        return;
    }

    // Get step modulations from APVTS-backed state
    std::vector<StepModulationData> selectedMods;
    for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
        if (state.getStepModSelected (selectedStep, i)) {
            StepModulationData mod;
            mod.type = state.getStepModType (selectedStep, i);
            mod.ccNumber = state.getStepModCCNum (selectedStep, i);
            mod.value = state.getStepModValue (selectedStep, i);
            selectedMods.push_back (mod);
        }
    }

    // Determine velocity (default to 100 if no velocity modulation is selected)
    juce::uint8 velocity = 100;
    for (const auto& mod : selectedMods) {
        if (mod.getType() == ModulationType::Velocity) {
            velocity = static_cast<juce::uint8> (mod.getValue());
            break;
        }
    }

    // Use override note if provided, otherwise read from step
    int note = 0;
    if (noteOverride >= 0) {
        note = noteOverride;
        DBG ("  Using override note: " << note);
    } else {
        note = state.getSelectedNoteForStep (selectedStep);
        DBG ("  Selected step: " << selectedStep << ", Note from step: " << note);
    }

    // Send note off for previous note (if any)
    noteOffScheduler.sendPreviewNoteOff (getMidiOutputChannel(), midiOutput.get(), previewMidiFifo);

    // Send note on for the new note
    if (note >= 0) {
        // Send CC messages first (only in standalone mode for now)
        if (midiOutput) {
            for (const auto& mod : selectedMods) {
                if (mod.getType() == ModulationType::CC) {
                    auto ccMsg = juce::MidiMessage::controllerEvent (getMidiOutputChannel(), mod.getCCNumber(), mod.getValue());
                    midiOutput->sendMessageNow (ccMsg);
                    DBG ("  Preview MIDI CC (standalone): " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }
        }

        if (midiOutput) {
            auto noteOn = juce::MidiMessage::noteOn (getMidiOutputChannel(), note, velocity);
            midiOutput->sendMessageNow (noteOn);
            DBG ("  Preview MIDI Note ON sent (standalone): " << note << " (velocity " << (int) velocity << ")");
        } else {
            previewMidiFifo.push ({ note, true, velocity });
            DBG ("  Preview MIDI Note ON queued (DAW): " << note << " (velocity " << (int) velocity << ")");
        }
        noteOffScheduler.setLastPlayedNote (note);

        // Schedule a matching preview note-off after `noteLength × stepDuration`
        // so a slider tweak doesn't leave the previewed note ringing forever.
        // Fallback when the DAW BPM is unavailable (standalone idle, tests):
        // assume a 16th-note step at 120 BPM (125 ms) so the gate still elapses.
        const double samplesPerStep = clock.calculateSamplesPerStep (computeCurrentDawBpm(), getSampleRate());
        const double sr = getSampleRate();
        const float noteLength = state.getNoteLength();
        double stepMs = 125.0;
        if (samplesPerStep > 0.0 && sr > 0.0)
            stepMs = samplesPerStep * 1000.0 / sr;
        const int gateMs = juce::jmax (1, static_cast<int> (stepMs * noteLength));
        previewGateTimer.startTimer (gateMs);
    } else {
        noteOffScheduler.setLastPlayedNote (-1);
        DBG ("  No note to play (note < 0)");
    }
    DBG ("=== triggerCurrentStepMidi() complete ===");
}

void PluginProcessor::releasePreviewMidi() {
    previewGateTimer.stopTimer();
    noteOffScheduler.sendPreviewNoteOff (getMidiOutputChannel(), midiOutput.get(), previewMidiFifo);
    noteOffScheduler.setLastPlayedNote (-1);
    DBG ("releasePreviewMidi() — preview note released");
}

void PluginProcessor::firePreviewGateOff() {
    noteOffScheduler.sendPreviewNoteOff (getMidiOutputChannel(), midiOutput.get(), previewMidiFifo);
    noteOffScheduler.setLastPlayedNote (-1);
    DBG ("firePreviewGateOff() — slider-preview gate elapsed");
}

//==============================================================================
// Sequencer Playback Control
//==============================================================================

void PluginProcessor::togglePlayPause() {
    auto currentState = sequencerState.load();

    if (currentState == PlaybackState::Stopped) {
        // Stopped → Playing: Reset to step 0 and start
        getState().reset();
        sequencerState.store (PlaybackState::Playing);

        // Start the internal clock if in internal mode
        if (clock.getMode() == ClockMode::INTERNAL) {
            clock.start();
        }

        DBG ("Sequencer started from stopped (index reset to 0)");
    } else if (currentState == PlaybackState::Playing) {
        // Playing → Paused: Hold at current index
        sequencerState.store (PlaybackState::Paused);

        DBG ("Sequencer paused at index " << getState().getCurrentIndex());
    } else if (currentState == PlaybackState::Paused) {
        // Paused → Playing: Resume from current index
        sequencerState.store (PlaybackState::Playing);

        // Make sure internal clock is running if in internal mode
        if (clock.getMode() == ClockMode::INTERNAL) {
            clock.start();
        }

        DBG ("Sequencer resumed from index " << getState().getCurrentIndex());
    }
}

void PluginProcessor::stopSequencer() {
    sequencerState.store (PlaybackState::Stopped);

    // Reset sequence to step 0
    getState().reset();

    // Stop the internal clock if in internal mode
    if (clock.getMode() == ClockMode::INTERNAL) {
        clock.stop();
    }

    // Send note off for any playing note and clear all scheduling state
    noteOffScheduler.stopAndClear (getMidiOutputChannel(), midiOutput.get());

    DBG ("Sequencer stopped and reset to step 0");
}

//==============================================================================
// Preset State Methods
//==============================================================================

std::unique_ptr<juce::XmlElement> PluginProcessor::getStateAsXml() {
    // Create root XML element to hold all state (same format as getStateInformation)
    auto rootXml = std::make_unique<juce::XmlElement> ("GamblerState");

    // Save APVTS parameters (MIDI-mappable state)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement (apvtsXml.release());

    // Save InternalState (non-MIDI-mappable state: step notes, modulations, etc.)
    auto internalStateXml = internalState.getState().createXml();
    rootXml->addChildElement (internalStateXml.release());

    // Save Clock settings (mode, BPM, subdivision)
    rootXml->addChildElement (clock.toXml().release());

    // Save MIDI Learn Mappings
    rootXml->addChildElement (midiLearnRegistry.toXml().release());

    return rootXml;
}

void PluginProcessor::loadStateFromXml (const juce::XmlElement& xml) {
    if (!xml.hasTagName ("GamblerState")) {
        DBG ("PluginProcessor::loadStateFromXml - Invalid XML format (expected GamblerState root)");
        return;
    }

    DBG ("Loading preset state from XML");

    // Load APVTS parameters (MIDI-mappable state)
    if (auto* apvtsXml = xml.getChildByName (apvts.state.getType())) {
        apvts.replaceState (juce::ValueTree::fromXml (*apvtsXml));
        DBG ("  - APVTS loaded");
    }

    // Load InternalState (non-MIDI-mappable state: step notes, modulations, etc.)
    if (auto* internalStateXml = xml.getChildByName (InternalState::IDs::ROOT_TYPE)) {
        internalState.setState (juce::ValueTree::fromXml (*internalStateXml));
        DBG ("  - InternalState loaded (step notes, modulations, etc.)");
    }

    // Clock settings (mode, BPM, subdivision) are handled automatically:
    // apvts.replaceState() above updates the APVTS params, which fires parameterChanged,
    // which applies the new values to the Clock object.

    // Load MIDI Learn Mappings directly (we're already on the message thread)
    if (auto* midiLearnXml = xml.getChildByName ("MidiLearnMappings")) {
        midiLearnRegistry.fromXml (*midiLearnXml);
        DBG ("  - MIDI Learn Mappings loaded");
    }
}

//==============================================================================
// MIDI Input Processing
//==============================================================================
// In here we do 3 things:
// 1. listen to external midi clock and forward it to clock
// 2. Listen to midi, check our midiLearnRegistry (basically a map of messages to control ids) and if a match, trigger that control
// 3. Saving new midi mappings if in midi learn mode.
void PluginProcessor::processMidiInput (const juce::MidiBuffer& midiMessages) {
    for (const auto& metadata : midiMessages) {
        auto msg = metadata.getMessage();

        if (handleTransportMessage (msg, metadata.samplePosition))
            continue;

        if (msg.isController())
            handleControlChange (msg);
        else if (msg.isNoteOn() || msg.isNoteOff())
            handleNoteMessage (msg);
    }
}

bool PluginProcessor::handleTransportMessage (const juce::MidiMessage& msg, int sampleOffsetInBlock) {
    if (clock.getMode() != ClockMode::EXTERNAL_MIDI_CLOCK)
        return false;

    if (msg.isMidiClock()) {  // 0xF8 timing clock
        clock.processExternalMidiClockPulse(sampleOffsetInBlock);
        return true;
    }
    if (msg.isMidiStart()) {  // 0xFA
        clock.handleExternalMidiStart();
        sequencerState.store (PlaybackState::Playing);
        DBG ("External MIDI Start received - sequencer playing");
        return true;
    }
    if (msg.isMidiContinue()) {  // 0xFB
        clock.handleExternalMidiContinue();
        sequencerState.store (PlaybackState::Playing);
        DBG ("External MIDI Continue received - sequencer resumed");
        return true;
    }
    if (msg.isMidiStop()) {  // 0xFC
        clock.handleExternalMidiStop();
        DBG ("External MIDI Stop received");
        return true;
    }
    return false;
}

void PluginProcessor::handleControlChange (const juce::MidiMessage& msg) {
    const int ccNumber = msg.getControllerNumber();
    const int ccValue  = msg.getControllerValue();
    const int channel  = msg.getChannel();

    routeLearnableMessage (MidiMessageType::CC, channel, ccNumber, ccValue, ccValue >= 64);
}

void PluginProcessor::handleNoteMessage (const juce::MidiMessage& msg) {
    const int noteNumber = msg.getNoteNumber();
    const int channel    = msg.getChannel();
    const bool isNoteOn  = msg.isNoteOn();
    const int value      = isNoteOn ? 127 : 0;

    // While in learn mode, never fire controls — just consume the message.
    // Note Off doesn't count as a "learn event" (only Note On teaches a mapping).
    if (midiLearnRegistry.isLearning()) {
        if (! isNoteOn)
            return;
        // Note On in learn mode falls through to routeLearnableMessage,
        // which will call learnMidiMessage and return without firing controls.
    }

    routeLearnableMessage (MidiMessageType::NOTE, channel, noteNumber, value, isNoteOn);
}

void PluginProcessor::routeLearnableMessage (MidiMessageType type,
                                              int channel,
                                              int midiNumber,
                                              int value,
                                              bool isOnOrPositive) {
    if (midiLearnRegistry.isLearning()) {
        if (midiLearnRegistry.learnMidiMessage (type, channel, midiNumber)) {
            DBG ("Learned: CH" << channel
                 << (type == MidiMessageType::CC ? "-CC" : "-N") << midiNumber);
            notifyMidiLearned();
        }
        // Always return in learn mode — never fire controls while mapping.
        return;
    }

    MidiControlId controlId = midiLearnRegistry.findUserMapping(type, channel, midiNumber);
    if (controlId == MidiControlId::COUNT)
        return;

    MidiMapping mapping { type, midiNumber, controlId };

    if (type == MidiMessageType::CC) {
        DBG ("MIDI CC " << midiNumber << " = " << value
             << " → " << getMidiControlName (controlId));
    } else {
        DBG ("MIDI Note " << midiNumber << " " << (isOnOrPositive ? "ON" : "OFF")
             << " → " << getMidiControlName (controlId));
    }

    handleMidiMapping (mapping, value, isOnOrPositive);
}

void PluginProcessor::notifyMidiLearned() {
    // Make a local copy to avoid a race with the UI thread clearing the callback.
    auto callback = onMidiLearned;
    if (callback) {
        DBG ("  Calling onMidiLearned callback");
        callback();
    } else {
        DBG ("  WARNING: onMidiLearned callback is null!");
    }
}

void PluginProcessor::fireTickCallback() {
    auto callback = onSequenceTick;
    if (callback) callback();
}

void PluginProcessor::setTriggerParam (const juce::String& paramId, bool isOn) {
    if (auto* param = apvts.getParameter (paramId)) {
        param->setValueNotifyingHost (isOn ? 1.0f : 0.0f);
        DBG ("  → " << paramId << " param set to " << (isOn ? "ON" : "OFF"));
    }
}

void PluginProcessor::handleOctaveSlider (int midiValue) {
    auto& state = getState();
    const int octaveIndex = MidiValueConverter::midiToDiscreteIndex (midiValue, 9);
    const int selectedStep = state.getSelectedStepForEditing();
    const int currentNote = state.getSelectedNoteForStep (selectedStep);
    const int pitchClass = currentNote % 12;
    const int newNote = octaveIndex * 12 + pitchClass;
    state.updateSelectedNoteForStep (selectedStep, newNote);
    DBG ("  → Octave changed to " << octaveIndex << " (MIDI note " << newNote << ")");
    fireTickCallback();
}

void PluginProcessor::handleNoteSlider (int midiValue) {
    auto& state = getState();
    const int pitchClass = MidiValueConverter::midiToDiscreteIndex (midiValue, 11);
    const int selectedStep = state.getSelectedStepForEditing();
    const int currentNote = state.getSelectedNoteForStep (selectedStep);
    const int octave = currentNote / 12;
    const int newNote = octave * 12 + pitchClass;
    state.updateSelectedNoteForStep (selectedStep, newNote);
    DBG ("  → Note changed to pitch class " << pitchClass << " (MIDI note " << newNote << ")");
    fireTickCallback();
}

void PluginProcessor::handleProbabilitySlider (float normalizedValue) {
    const int percentage = static_cast<int> (normalizedValue * 100.0f);
    getState().setProbabilityPercentage (percentage);
    DBG ("  → Probability changed to " << percentage << "%");
    fireTickCallback();
}

void PluginProcessor::handleShiftAwareButton (const juce::String& primaryParam,
                                              const juce::String& shiftParam,
                                              bool isOn) {
    const bool isShiftActive = getState().getIsShift();
    setTriggerParam (isShiftActive ? shiftParam : primaryParam, isOn);
}

void PluginProcessor::handleShiftToggle (bool isOn) {
    if (! isOn) return;
    if (auto* param = apvts.getParameter (Params::Keys::ID::SEQ_IS_SHIFT)) {
        const bool currentState = param->getValue() > 0.5f;
        param->setValueNotifyingHost (currentState ? 0.0f : 1.0f);
        DBG ("  → SEQ_IS_SHIFT toggled to " << (! currentState ? "ON" : "OFF"));
    }
}

void PluginProcessor::handleStepSelect (MidiControlId controlId) {
    auto& state = getState();
    const int localStepIndex = static_cast<int> (controlId) - static_cast<int> (MidiControlId::STEP_0);
    const int absoluteStep = state.getViewWindowStart() + localStepIndex;
    if (absoluteStep < state.getLength()) {
        state.setSelectedStepForEditing (absoluteStep);
        DBG ("  → Step " << absoluteStep << " selected for editing (window " << localStepIndex << ")");
    }
    fireTickCallback();
}

void PluginProcessor::handleSubseqMinus() {
    auto& state = getState();
    const int selectedStep = state.getSelectedStepForEditing();
    const int currentLength = state.getStepNotesLength (selectedStep);
    if (currentLength > 1) {
        state.setStepNotesLength (selectedStep, currentLength - 1);
        DBG ("  → Substep length decreased to " << (currentLength - 1));
    }
    fireTickCallback();
}

void PluginProcessor::handleSubseqPlus() {
    auto& state = getState();
    const int selectedStep = state.getSelectedStepForEditing();
    const int currentLength = state.getStepNotesLength (selectedStep);
    if (currentLength < ParamConstants::MAX_SUBSTEPS) {
        state.setStepNotesLength (selectedStep, currentLength + 1);
        DBG ("  → Substep length increased to " << (currentLength + 1));
    }
    fireTickCallback();
}

void PluginProcessor::handleSubseqBarSelect (MidiControlId controlId) {
    auto& state = getState();
    const int barIndex = static_cast<int> (controlId) - static_cast<int> (MidiControlId::SUBSEQ_BAR_0);
    const int selectedStep = state.getSelectedStepForEditing();
    if (barIndex < state.getStepNotesLength (selectedStep)) {
        state.setSelectedSubstepIndex (selectedStep, barIndex);
        DBG ("  → Substep " << barIndex << " selected for editing");
    }
    fireTickCallback();
}

void PluginProcessor::handleNoteRangeMin (int midiValue) {
    const int offset = MidiValueConverter::midiToDiscreteIndex (midiValue, 127);
    getState().setScaleWindowOffset (offset);
    DBG ("  → Note range offset set to " << offset);
    fireTickCallback();
}

void PluginProcessor::handleNoteRangeMax (int midiValue) {
    const int size = MidiValueConverter::midiToDiscreteIndex (midiValue, 127) + 1;
    getState().setScaleWindowSize (size);
    DBG ("  → Note range size set to " << size);
    fireTickCallback();
}

void PluginProcessor::handleSubdivision (int midiValue) {
    const int subdivisionIndex = MidiValueConverter::midiToDiscreteIndex (midiValue, 3);
    if (auto* param = apvts.getParameter (Params::Keys::ID::CLOCK_SUBDIVISION)) {
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (subdivisionIndex)));
        DBG ("  → Subdivision set to " << subdivisionIndex);
    }
}

void PluginProcessor::handleScaleNote (MidiControlId controlId, bool isOn) {
    auto& state = getState();
    const int noteIndex = static_cast<int> (controlId) - static_cast<int> (MidiControlId::SCALE_NOTE_C);
    const bool currentState = state.getGlobalScaleNote (noteIndex);

    bool isLatched = false;
    if (auto* latchParam = apvts.getParameter (Params::Keys::ID::SCALE_LATCH))
        isLatched = latchParam->getValue() > 0.5f;

    if (isLatched) {
        if (isOn) {
            state.setGlobalScaleNote (noteIndex, ! currentState);
            DBG ("  → Global scale note " << noteIndex << " toggled to " << (! currentState ? "ON" : "OFF"));
            fireTickCallback();
        }
    } else {
        if (isOn != currentState) {
            state.setGlobalScaleNote (noteIndex, isOn);
            DBG ("  → Global scale note " << noteIndex << " momentary set to " << (isOn ? "ON" : "OFF"));
            fireTickCallback();
        }
    }
}

void PluginProcessor::handleMidiMapping (const MidiMapping& mapping, int midiValue, bool isNoteOnOrCCPositive) {
    const float normalizedValue = MidiValueConverter::midiToScaled (
        midiValue, mapping.minValue, mapping.maxValue, mapping.invert);

    switch (mapping.controlId) {
        case MidiControlId::OCTAVE_SLIDER:      handleOctaveSlider (midiValue); break;
        case MidiControlId::NOTE_SLIDER:        handleNoteSlider (midiValue); break;
        case MidiControlId::PROBABILITY_SLIDER: handleProbabilitySlider (normalizedValue); break;

        case MidiControlId::BUTTON_ROLL:
            setTriggerParam (Params::Keys::ID::ROLL, isNoteOnOrCCPositive);
            break;

        case MidiControlId::BUTTON_PAGE_LEFT:
            handleShiftAwareButton (Params::Keys::ID::INDEX_LEFT, Params::Keys::ID::DUPLICATE, isNoteOnOrCCPositive);
            break;
        case MidiControlId::BUTTON_PAGE_RIGHT:
            handleShiftAwareButton (Params::Keys::ID::INDEX_RIGHT, Params::Keys::ID::CLEAR_WINDOW, isNoteOnOrCCPositive);
            break;
        case MidiControlId::BUTTON_LENGTH_PLUS:
            handleShiftAwareButton (Params::Keys::ID::LENGTH_PLUS, Params::Keys::ID::CLEAR_STEP, isNoteOnOrCCPositive);
            break;
        case MidiControlId::BUTTON_LENGTH_MINUS:
            handleShiftAwareButton (Params::Keys::ID::LENGTH_MINUS, Params::Keys::ID::RANDOMIZE_ALL, isNoteOnOrCCPositive);
            break;
        case MidiControlId::BUTTON_NUDGE_LEFT:
            handleShiftAwareButton (Params::Keys::ID::NUDGE_LEFT, Params::Keys::ID::RANDOMIZE_STEP, isNoteOnOrCCPositive);
            break;
        case MidiControlId::BUTTON_NUDGE_RIGHT:
            handleShiftAwareButton (Params::Keys::ID::NUDGE_RIGHT, Params::Keys::ID::SHUFFLE, isNoteOnOrCCPositive);
            break;

        case MidiControlId::BUTTON_SHIFT:
            handleShiftToggle (isNoteOnOrCCPositive);
            break;

        case MidiControlId::STEP_0: case MidiControlId::STEP_1:
        case MidiControlId::STEP_2: case MidiControlId::STEP_3:
        case MidiControlId::STEP_4: case MidiControlId::STEP_5:
        case MidiControlId::STEP_6: case MidiControlId::STEP_7:
            if (isNoteOnOrCCPositive) handleStepSelect (mapping.controlId);
            break;

        case MidiControlId::SUBSEQ_MINUS:
            if (isNoteOnOrCCPositive) handleSubseqMinus();
            break;
        case MidiControlId::SUBSEQ_PLUS:
            if (isNoteOnOrCCPositive) handleSubseqPlus();
            break;

        case MidiControlId::SUBSEQ_BAR_0: case MidiControlId::SUBSEQ_BAR_1:
        case MidiControlId::SUBSEQ_BAR_2: case MidiControlId::SUBSEQ_BAR_3:
        case MidiControlId::SUBSEQ_BAR_4: case MidiControlId::SUBSEQ_BAR_5:
        case MidiControlId::SUBSEQ_BAR_6: case MidiControlId::SUBSEQ_BAR_7:
            if (isNoteOnOrCCPositive) handleSubseqBarSelect (mapping.controlId);
            break;

        case MidiControlId::NOTE_RANGE_MIN:     handleNoteRangeMin (midiValue); break;
        case MidiControlId::NOTE_RANGE_MAX:     handleNoteRangeMax (midiValue); break;
        case MidiControlId::SUBDIVISION_SLIDER: handleSubdivision (midiValue); break;

        case MidiControlId::SCALE_NOTE_C:  case MidiControlId::SCALE_NOTE_Db:
        case MidiControlId::SCALE_NOTE_D:  case MidiControlId::SCALE_NOTE_Eb:
        case MidiControlId::SCALE_NOTE_E:  case MidiControlId::SCALE_NOTE_F:
        case MidiControlId::SCALE_NOTE_Gb: case MidiControlId::SCALE_NOTE_G:
        case MidiControlId::SCALE_NOTE_Ab: case MidiControlId::SCALE_NOTE_A:
        case MidiControlId::SCALE_NOTE_Bb: case MidiControlId::SCALE_NOTE_B:
            handleScaleNote (mapping.controlId, isNoteOnOrCCPositive);
            break;

        case MidiControlId::SETTINGS_TOGGLE:
            if (isNoteOnOrCCPositive) DBG ("  → Settings toggle (UI only)");
            break;

        case MidiControlId::PRESET_CYCLE_UP:
            setTriggerParam (Params::Keys::ID::PRESET_CYCLE_UP, isNoteOnOrCCPositive);
            break;
        case MidiControlId::PRESET_CYCLE_DOWN:
            setTriggerParam (Params::Keys::ID::PRESET_CYCLE_DOWN, isNoteOnOrCCPositive);
            break;
        case MidiControlId::PLAY_PAUSE:
            setTriggerParam (Params::Keys::ID::PLAY_PAUSE, isNoteOnOrCCPositive);
            break;
        case MidiControlId::STOP:
            setTriggerParam (Params::Keys::ID::STOP, isNoteOnOrCCPositive);
            break;
        case MidiControlId::SCALE_LATCH:
            setTriggerParam (Params::Keys::ID::SCALE_LATCH, isNoteOnOrCCPositive);
            break;

        case MidiControlId::COUNT:
            break;
        default:
            DBG ("  → Unhandled MIDI control: " << getMidiControlName (mapping.controlId));
            break;
    }
}

//==============================================================================
// MIDI Clock Output Management
//==============================================================================

void PluginProcessor::openAllMidiClockOutputs() {
    // Close any existing clock outputs
    closeAllMidiClockOutputs();

    // Get all available MIDI output devices
    auto devices = juce::MidiOutput::getAvailableDevices();

    DBG ("Opening MIDI clock outputs for " << devices.size() << " devices");

    // Open each device for MIDI clock output
    for (const auto& device : devices) {
        auto output = juce::MidiOutput::openDevice (device.identifier);
        if (output) {
            DBG ("  Opened MIDI clock output: " << device.name);
            midiClockOutputs.push_back (std::move (output));

            // Send MIDI clock start message
            auto startMsg = juce::MidiMessage::midiStart();
            midiClockOutputs.back()->sendMessageNow (startMsg);
        } else {
            DBG ("  Failed to open MIDI clock output: " << device.name);
        }
    }
}

void PluginProcessor::closeAllMidiClockOutputs() {
    // Send MIDI stop message to all devices before closing
    auto stopMsg = juce::MidiMessage::midiStop();
    for (auto& output : midiClockOutputs) {
        if (output) {
            output->sendMessageNow (stopMsg);
        }
    }

    // Clear all MIDI clock outputs
    midiClockOutputs.clear();
    DBG ("Closed all MIDI clock outputs");
}

juce::StringArray PluginProcessor::getAvailableMidiOutputDeviceNames() const {
    juce::StringArray names;
    auto devices = juce::MidiOutput::getAvailableDevices();

    for (const auto& device : devices) {
        names.add (device.name);
    }

    return names;
}

int PluginProcessor::getMidiOutputChannel() const {
    if (auto* param = dynamic_cast<juce::AudioParameterInt*>(
            apvts.getParameter(Params::Keys::ID::MIDI_OUTPUT_CHANNEL))) {
        return param->get();
    }
    return 1;  // Default fallback
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
