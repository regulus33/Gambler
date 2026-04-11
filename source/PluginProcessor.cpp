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
    juce::ScopedNoDenormals noDenormals; // this is a precision thing, may not be needed for this
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    // Process incoming MIDI for control mapping (before clearing)
    // we just do it all here so we don't muddy up this method too much more.
    processMidiInput (midiMessages);

    // Clear incoming MIDI (we're generating, not processing MIDI through and this is good practice with buffers)
    midiMessages.clear();

    // This kinda has nothing to do with midi, its just sending preview notes...
    // more info in PreviewMidiFifo file for why we use this.
    previewMidiFifo.drainInto (midiMessages, getMidiOutputChannel());

    // Check if scheduled note-off should be sent (sample-accurate within this buffer)
    if (noteOffScheduler.processScheduledNoteOff (midiMessages, getMidiOutputChannel(),
                                                   midiOutput.get(), currentSamplePosition,
                                                   buffer.getNumSamples()))
    {
        // Notify UI that note-off occurred (thread-safe via AsyncUpdater)
        auto noteOffCallback = onNoteOff;
        if (noteOffCallback)
            noteOffCallback();
    }

    // Update current sample position for the midi note off scheduling
    currentSamplePosition += buffer.getNumSamples();

    // Process clock based on current mode
    if (clock.getMode() == ClockMode::DAW_SYNC) {
        // DAW sync mode: get timing from host transport (that's the play-head)
        if (auto* playHead = getPlayHead()) {
            // get the ppqn count
            auto position = playHead->getPosition();
            // save the latest position into DawSync
            int pulseCount = dawSync.update (position);
            // send clock the latest pulse count which we calculate by feeding the position to dawSync.update() above.
            clock.updateDawSync (pulseCount);
        }
    } else {
        // Internal clock mode, accumulate / process samples with sample accurate clock
        clock.processInternalClock (buffer.getNumSamples());
    }

    // If we are in standalone, we can send the midi clock out to the device.
    // NOTE: we send clock to all outputs when in internal clock mode.
    if (clock.getMode() == ClockMode::INTERNAL && !midiClockOutputs.empty()) {
        // Get accumulated MIDI clock pulses to send
        int midiClockPulses = clock.getMidiClockPulsesToSend();

        // Send accumulated MIDI clock messages
        for (int i = 0; i < midiClockPulses; ++i) {
            auto clockMsg = juce::MidiMessage::midiClock();
            for (auto& output : midiClockOutputs) {
                if (output) {
                    output->sendMessageNow (clockMsg);
                }
            }
        }
    }

    // ================ DAW TRANSPORT DETECTION (for auto-resume in external clock mode) ================================
    // Detect when DAW transitions from stopped to playing in external clock mode
    if (clock.getMode() == ClockMode::DAW_SYNC) {
        if (auto* playHead = getPlayHead()) {
            auto position = playHead->getPosition();
            bool dawIsPlaying = position.hasValue() && position->getIsPlaying();

            // Detect transition from stopped to playing
            if (dawIsPlaying && !previousDawIsPlaying) {
                // DAW just started - resume sequencer playback (to Playing state)
                auto currentState = sequencerState.load();
                if (currentState == PlaybackState::Stopped || currentState == PlaybackState::Paused) {
                    sequencerState.store(PlaybackState::Playing);
                    DBG("DAW started - sequencer auto-resumed");
                }
            }

            previousDawIsPlaying = dawIsPlaying;
        }
    }

    // ================ MAIN PART ========================================================================================
    if (sequencerState.load() == PlaybackState::Playing && clock.shouldTick()) {
        // Accumulate MIDI clock pulses for the next cycle
        // We need to do this because the normal clock rate is not even close to the ppqn midi uses.
        // So we accumulate and barrage the midi ticks (we spread them out but all in between 16th note ppqn cycles which is gambler's default timing)
        if (clock.getMode() == ClockMode::INTERNAL && !midiClockOutputs.empty()) {
            clock.accumulateMidiClockPulse();
        }

        // Send immediate note-off for previous note and cancel any scheduled one
        noteOffScheduler.sendImmediateNoteOff (midiMessages, getMidiOutputChannel(), midiOutput.get());

        // Tick the sequence to get the next note (using APVTS-backed state)
        auto& state = getState(); // get the avpts wrapper

        int note = state.tick(); // get next note (quantized with the global scale)
        int currentStepIndex = state.getCurrentIndex(); // get current step index

        DBG ("Clock tick! Step: " << currentStepIndex << ", Note: " << note);

        // Make sure it is not a rest
        if (note >= 0) {
            // Get step modulations for this step (using APVTS-backed state)
            auto selectedMods = state.getSelectedModulations (currentStepIndex);

            // Determine velocity (default to 100 if no velocity modulation is selected)
            juce::uint8 velocity = 100;

            // Send CC messages and check for velocity modulation
            // These come from the step modulations panel, 7 slots for each step
            for (const auto& mod : selectedMods) {
                // if velocity then modulate the note
                if (mod.getType() == ModulationType::Velocity) {
                    // Use this as the note velocity
                    velocity = static_cast<juce::uint8> (mod.getValue());
                    // Otherwise it is just a CC, send that
                } else if (mod.getType() == ModulationType::CC) {
                    // Send CC message
                    auto ccMsg = juce::MidiMessage::controllerEvent (getMidiOutputChannel(), mod.getCCNumber(), mod.getValue());
                    // send the event
                    midiMessages.addEvent (ccMsg, 0);

                    // Send directly to MIDI output device (for standalone mode)
                    if (midiOutput) {
                        midiOutput->sendMessageNow (ccMsg);
                    }
                    DBG ("MIDI CC: " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }

            // Valid note - send note on with velocity
            auto noteOn = juce::MidiMessage::noteOn (getMidiOutputChannel(), note, velocity);
            // add this note to the messages
            midiMessages.addEvent (noteOn, 0);

            // Send directly to MIDI output device (for standalone mode)
            if (midiOutput) {
                // send the message.
                midiOutput->sendMessageNow (noteOn);
            }
            DBG ("MIDI Note ON: " << note << " (velocity " << (int) velocity << ", channel 1)");
            noteOffScheduler.setLastPlayedNote (note);

            // Calculate samples per step based on current clock settings
            double dawBpm = 0.0;
            if (clock.getMode() == ClockMode::DAW_SYNC) {
                if (auto* playHead = getPlayHead()) {
                    auto position = playHead->getPosition();
                    if (position.hasValue() && position->getBpm().hasValue()) {
                        dawBpm = *position->getBpm();
                    }
                }
            }

            double samplesPerStep = clock.calculateSamplesPerStep(dawBpm, getSampleRate());

            if (samplesPerStep > 0.0) {
                double noteLengthPercent = state.getNoteLength();
                juce::int64 noteOffDelay = static_cast<juce::int64> (samplesPerStep * noteLengthPercent);
                noteOffScheduler.scheduleNoteOff (note, currentSamplePosition + noteOffDelay);
                DBG ("Scheduled note-off for note " << note
                     << " (delay: " << noteOffDelay << " samples, " << (noteLengthPercent * 100.0) << "% of step)");
            }
        } else {
            // Rest - no note to play
            noteOffScheduler.setLastPlayedNote (-1);
            // Note: Rest recording is now handled inside state.tick() when roll is engaged
            if (state.getIsRandom()) {
                DBG ("Rest recorded at step: " << currentStepIndex);
            }
        }

        // Notify UI that a tick occurred (thread-safe via AsyncUpdater)
        // Make a local copy to avoid race condition with UI thread clearing the callback
        auto callback = onSequenceTick;
        if (callback) {
            callback();
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
    // Handle sequencer control button actions
    // These are triggered when the button param transitions to ON (true)
    // The param is set by UI clicks, DAW automation, or MIDI input

    bool isOn = newValue > 0.5f;

    // ========== SEQUENCER CONTROL BUTTONS ==========
    // Only trigger action on the ON transition (debounce)

    // ========== PRIMARY ROW BUTTONS ==========

    if (parameterID == Params::Keys::ID::ROLL) {
        // Roll is momentary - set isRandom to match param state
        getState().setIsRandom (isOn);
        DBG ("ROLL " + juce::String (isOn ? "ON" : "OFF") + " - isRandom: " + juce::String (getState().getIsRandom() ? "true" : "false"));
    }

    // PAGE_LEFT and PAGE_RIGHT are handled in UI (need window position)
    // The param change will trigger UI callback via StateListenerComponent

    if (parameterID == Params::Keys::ID::LENGTH_PLUS && isOn) {
        getState().increaseLength();
        DBG ("LENGTH_PLUS triggered - new length: " + juce::String (getState().getLength()));
    }

    if (parameterID == Params::Keys::ID::LENGTH_MINUS && isOn) {
        getState().decreaseLength();
        DBG ("LENGTH_MINUS triggered - new length: " + juce::String (getState().getLength()));
    }

    if (parameterID == Params::Keys::ID::NUDGE_LEFT && isOn) {
        getState().nudgeLeft();
        DBG ("NUDGE_LEFT triggered");
    }

    if (parameterID == Params::Keys::ID::NUDGE_RIGHT && isOn) {
        getState().nudgeRight();
        DBG ("NUDGE_RIGHT triggered");
    }

    if (parameterID == Params::Keys::ID::SEQ_IS_SHIFT) {
        // SEQ_IS_SHIFT is the actual shift state - just log it
        DBG ("SEQ_IS_SHIFT " + juce::String (isOn ? "ON" : "OFF"));
    }

    // ========== SECONDARY ROW BUTTONS ==========

    if (parameterID == Params::Keys::ID::DUPLICATE && isOn) {
        auto& state = getState();
        int currentLength = state.getLength();
        int newLength = juce::jmin (currentLength * 2, ParamConstants::MAX_STEPS);
        int stepsToCopy = newLength - currentLength;

        for (int i = 0; i < stepsToCopy; i++) {
            int srcIdx = i;
            int dstIdx = currentLength + i;
            state.setStepNotesLength (dstIdx, state.getStepNotesLength (srcIdx));
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote (dstIdx, n, state.getStepNote (srcIdx, n));
            }
        }
        state.setLength (newLength);
        DBG ("DUPLICATE triggered - new length: " + juce::String (newLength));
    }

    // CLEAR_WINDOW is handled in UI (needs window position)

    if (parameterID == Params::Keys::ID::CLEAR_STEP && isOn) {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
            state.setStepNote (selectedStep, n, -1);
        }
        DBG ("CLEAR_STEP triggered - cleared step " + juce::String (selectedStep));
    }

    if (parameterID == Params::Keys::ID::RANDOMIZE_STEP && isOn) {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        state.randomizeStepNote (selectedStep);
        DBG ("RANDOMIZE_STEP triggered - randomized step " + juce::String (selectedStep));
    }

    if (parameterID == Params::Keys::ID::RANDOMIZE_ALL && isOn) {
        auto& state = getState();
        for (int i = 0; i < state.getLength(); i++) {
            state.randomizeStepNote (i);
        }
        DBG ("RANDOMIZE_ALL triggered");
    }

    if (parameterID == Params::Keys::ID::SHUFFLE && isOn) {
        auto& state = getState();
        int len = state.getLength();
        static std::random_device rd;
        static std::mt19937 gen (rd());
        for (int i = len - 1; i > 0; i--) {
            std::uniform_int_distribution<> dis (0, i);
            int j = dis (gen);
            // Swap steps i and j
            int lenI = state.getStepNotesLength (i);
            std::array<int, ParamConstants::MAX_SUBSTEPS> notesI;
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                notesI[static_cast<size_t> (n)] = state.getStepNote (i, n);
            }
            // Copy j to i
            state.setStepNotesLength (i, state.getStepNotesLength (j));
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote (i, n, state.getStepNote (j, n));
            }
            // Copy saved i to j
            state.setStepNotesLength (j, lenI);
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote (j, n, notesI[static_cast<size_t> (n)]);
            }
        }
        DBG ("SHUFFLE triggered");
    }

    // ========== DESTRUCTIVE MODE TRANSITIONS ==========
    // When switching from OFF → ON, immediately bake all existing notes
    // so the user doesn't hear out-of-scale/range notes that were stored raw.

    if (parameterID == Params::Keys::ID::PREF_DESTRUCTIVE_SCALE && isOn) {
        getState().bakeAllExistingNotes();
        DBG ("DESTRUCTIVE_SCALE ON - baked all existing notes to current scale");
    }

    if (parameterID == Params::Keys::ID::PREF_DESTRUCTIVE_RANGE && isOn) {
        getState().bakeAllExistingNotes();
        DBG ("DESTRUCTIVE_RANGE ON - baked all existing notes to current range");
    }

    // ========== PRESET CYCLING ==========
    // These are trigger params - action happens on the ON transition
    // The actual preset loading is handled by the UI via onPresetCycleUp/Down callbacks

    if (parameterID == Params::Keys::ID::PRESET_CYCLE_UP && isOn) {
        if (onPresetCycleUp) {
            onPresetCycleUp();
        }
        DBG ("PRESET_CYCLE_UP triggered");
    }

    if (parameterID == Params::Keys::ID::PRESET_CYCLE_DOWN && isOn) {
        if (onPresetCycleDown) {
            onPresetCycleDown();
        }
        DBG ("PRESET_CYCLE_DOWN triggered");
    }

    // ========== TRANSPORT CONTROLS ==========
    // These are trigger params - action happens on the ON transition

    if (parameterID == Params::Keys::ID::PLAY_PAUSE && isOn) {
        togglePlayPause();
        if (onPlayPauseTriggered) {
            onPlayPauseTriggered();
        }
        DBG ("PLAY_PAUSE triggered");
    }

    if (parameterID == Params::Keys::ID::STOP && isOn) {
        stopSequencer();
        if (onStopTriggered) {
            onStopTriggered();
        }
        DBG ("STOP triggered");
    }

    // ========== CLOCK PARAMETERS ==========

    if (parameterID == Params::Keys::ID::CLOCK_MODE) {
        int intValue = static_cast<int>(newValue);
        // APVTS values: 0=INTERNAL, 1=DAW_SYNC, 2=EXTERNAL_MIDI_CLOCK
        switch (intValue) {
            case 0:
                clock.setMode(ClockMode::INTERNAL);
                clock.start();
                DBG("Clock mode set to INTERNAL (started)");
                break;
            case 1:
                clock.setMode(ClockMode::DAW_SYNC);
                clock.stop();
                DBG("Clock mode set to DAW_SYNC");
                break;
            case 2:
                clock.setMode(ClockMode::EXTERNAL_MIDI_CLOCK);
                clock.stop();
                DBG("Clock mode set to EXTERNAL_MIDI_CLOCK");
                break;
            default:
                break;
        }
    }

    if (parameterID == Params::Keys::ID::CLOCK_BPM) {
        clock.setBpm(static_cast<double>(newValue));
        DBG("Clock BPM set to " + juce::String(newValue));
    }

    if (parameterID == Params::Keys::ID::CLOCK_SUBDIVISION) {
        auto newSubdivision = static_cast<Subdivision>(static_cast<int>(newValue));
        clock.setSubdivision(newSubdivision);
        dawSync.setSubdivision(newSubdivision);
        DBG("Clock subdivision set to " + juce::String(static_cast<int>(newValue)));
    }

}

//==============================================================================
// UI Preview Methods
//==============================================================================

void PluginProcessor::triggerStepMidi (int stepIndex) {
    if (stepIndex < 0 || stepIndex >= 64)
        return;

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
    int note;
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
    } else {
        noteOffScheduler.setLastPlayedNote (-1);
        DBG ("  No note to play (note < 0)");
    }
    DBG ("=== triggerCurrentStepMidi() complete ===");
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

        if (handleTransportMessage (msg))
            continue;

        if (msg.isController())
            handleControlChange (msg);
        else if (msg.isNoteOn() || msg.isNoteOff())
            handleNoteMessage (msg);
    }
}

bool PluginProcessor::handleTransportMessage (const juce::MidiMessage& msg) {
    if (clock.getMode() != ClockMode::EXTERNAL_MIDI_CLOCK)
        return false;

    if (msg.isMidiClock()) {  // 0xF8 timing clock
        clock.processExternalMidiClockPulse();
        return true;
    }
    if (msg.isMidiStart()) {  // 0xFA
        clock.handleExternalMidiStart();
        sequencerState.store (SequencerState::Playing);
        DBG ("External MIDI Start received - sequencer playing");
        return true;
    }
    if (msg.isMidiContinue()) {  // 0xFB
        clock.handleExternalMidiContinue();
        sequencerState.store (SequencerState::Playing);
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

    // Note Off shouldn't count as a "learn event" - only learn on Note On.
    if (! isNoteOn && midiLearnRegistry.isLearning()) {
        // Still allow a mapped Note Off to fire; just skip the learn branch.
        MidiControlId controlId = midiLearnRegistry.findUserMapping (
            MidiMessageType::NOTE, channel, noteNumber);
        if (controlId != MidiControlId::COUNT) {
            MidiMapping mapping { MidiMessageType::NOTE, noteNumber, controlId };
            DBG ("MIDI Note " << noteNumber << " OFF → " << getMidiControlName (controlId));
            handleMidiMapping (mapping, value, false);
        }
        return;
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
            return;
        }
    }

    MidiControlId controlId = midiLearnRegistry.findUserMapping (type, channel, midiNumber);
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

void PluginProcessor::handleMidiMapping (const MidiMapping& mapping, int midiValue, bool isNoteOnOrCCPositive) {
    auto& state = getState();

    // Convert MIDI value using the mapping's conversion settings
    float normalizedValue = MidiValueConverter::midiToScaled (
        midiValue,
        mapping.minValue,
        mapping.maxValue,
        mapping.invert);

    // Handle each control type
    switch (mapping.controlId) {
        // ========== OCTAVE SLIDER ==========
        case MidiControlId::OCTAVE_SLIDER: {
            int octaveIndex = MidiValueConverter::midiToDiscreteIndex (midiValue, 9); // 0-9
            // Update via APVTS-backed state (thread-safe)
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getSelectedNoteForStep (selectedStep);
            int pitchClass = currentNote % 12;
            int newNote = octaveIndex * 12 + pitchClass;
            state.updateSelectedNoteForStep (selectedStep, newNote);
            DBG ("  → Octave changed to " << octaveIndex << " (MIDI note " << newNote << ")");
            auto callback = onSequenceTick;
            if (callback)
                callback();
            break;
        }

        // ========== NOTE SLIDER ==========
        case MidiControlId::NOTE_SLIDER: {
            int pitchClass = MidiValueConverter::midiToDiscreteIndex (midiValue, 11); // 0-11
            // Update via APVTS-backed state (thread-safe)
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getSelectedNoteForStep (selectedStep);
            int octave = currentNote / 12;
            int newNote = octave * 12 + pitchClass;
            state.updateSelectedNoteForStep (selectedStep, newNote);
            DBG ("  → Note changed to pitch class " << pitchClass << " (MIDI note " << newNote << ")");
            auto callback = onSequenceTick;
            if (callback)
                callback();
            break;
        }

        // ========== PROBABILITY SLIDER ==========
        case MidiControlId::PROBABILITY_SLIDER: {
            int percentage = static_cast<int> (normalizedValue * 100.0f);
            state.setProbabilityPercentage (percentage);
            DBG ("  → Probability changed to " << percentage << "%");
            auto callback = onSequenceTick;
            if (callback)
                callback();
            break;
        }

            // ========== SEQUENCER CONTROL BUTTONS - PRIMARY ROW ==========
            // All buttons use param-based approach: set param on note-on/off,
            // parameterChanged handles the action, UI observes param for visual state

        case MidiControlId::BUTTON_ROLL: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::ROLL)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → ROLL param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 1: PAGE_LEFT / DUPLICATE (shift-aware) ==========
        case MidiControlId::BUTTON_PAGE_LEFT: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::DUPLICATE : Params::Keys::ID::INDEX_LEFT;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "DUPLICATE" : "INDEX_LEFT") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 2: PAGE_RIGHT / CLEAR_WINDOW (shift-aware) ==========
        case MidiControlId::BUTTON_PAGE_RIGHT: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::CLEAR_WINDOW : Params::Keys::ID::INDEX_RIGHT;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "CLEAR_WINDOW" : "INDEX_RIGHT") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 3: LENGTH_PLUS / CLEAR_STEP (shift-aware) ==========
        case MidiControlId::BUTTON_LENGTH_PLUS: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::CLEAR_STEP : Params::Keys::ID::LENGTH_PLUS;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "CLEAR_STEP" : "LENGTH_PLUS") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 4: LENGTH_MINUS / RANDOMIZE_ALL (shift-aware) ==========
        case MidiControlId::BUTTON_LENGTH_MINUS: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::RANDOMIZE_ALL : Params::Keys::ID::LENGTH_MINUS;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "RANDOMIZE_ALL" : "LENGTH_MINUS") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 5: NUDGE_LEFT / RANDOMIZE_STEP (shift-aware) ==========
        case MidiControlId::BUTTON_NUDGE_LEFT: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::RANDOMIZE_STEP : Params::Keys::ID::NUDGE_LEFT;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "RANDOMIZE_STEP" : "NUDGE_LEFT") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== POSITION 6: NUDGE_RIGHT / SHUFFLE (shift-aware) ==========
        case MidiControlId::BUTTON_NUDGE_RIGHT: {
            bool isShiftActive = state.getIsShift();
            juce::String paramId = isShiftActive ? Params::Keys::ID::SHUFFLE : Params::Keys::ID::NUDGE_RIGHT;
            if (auto* param = apvts.getParameter (paramId)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → " << (isShiftActive ? "SHUFFLE" : "NUDGE_RIGHT") << " param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_SHIFT: {
            // Shift is a toggle - only toggle on note-on/CC positive
            if (isNoteOnOrCCPositive) {
                if (auto* param = apvts.getParameter (Params::Keys::ID::SEQ_IS_SHIFT)) {
                    bool currentState = param->getValue() > 0.5f;
                    param->setValueNotifyingHost (currentState ? 0.0f : 1.0f);
                    DBG ("  → SEQ_IS_SHIFT toggled to " << (!currentState ? "ON" : "OFF"));
                }
            }
            break;
        }

        // ========== SEQUENCER WINDOW STEPS (0-7) ==========
        case MidiControlId::STEP_0:
        case MidiControlId::STEP_1:
        case MidiControlId::STEP_2:
        case MidiControlId::STEP_3:
        case MidiControlId::STEP_4:
        case MidiControlId::STEP_5:
        case MidiControlId::STEP_6:
        case MidiControlId::STEP_7: {
            if (isNoteOnOrCCPositive) {
                int localStepIndex = static_cast<int> (mapping.controlId) - static_cast<int> (MidiControlId::STEP_0);
                int windowStart = state.getViewWindowStart();
                int absoluteStep = windowStart + localStepIndex;
                if (absoluteStep < state.getLength()) {
                    state.setSelectedStepForEditing (absoluteStep);
                    DBG ("  → Step " << absoluteStep << " selected for editing (window " << localStepIndex << ")");
                }
                auto callback = onSequenceTick;
                if (callback)
                    callback();
            }
            break;
        }

        // ========== SUBSEQUENCE CONTROLS ==========
        case MidiControlId::SUBSEQ_MINUS: {
            if (isNoteOnOrCCPositive) {
                int selectedStep = state.getSelectedStepForEditing();
                int currentLength = state.getStepNotesLength (selectedStep);
                if (currentLength > 1) {
                    state.setStepNotesLength (selectedStep, currentLength - 1);
                    DBG ("  → Substep length decreased to " << (currentLength - 1));
                }
                auto callback = onSequenceTick;
                if (callback)
                    callback();
            }
            break;
        }

        case MidiControlId::SUBSEQ_PLUS: {
            if (isNoteOnOrCCPositive) {
                int selectedStep = state.getSelectedStepForEditing();
                int currentLength = state.getStepNotesLength (selectedStep);
                if (currentLength < ParamConstants::MAX_SUBSTEPS) {
                    state.setStepNotesLength (selectedStep, currentLength + 1);
                    DBG ("  → Substep length increased to " << (currentLength + 1));
                }
                auto callback = onSequenceTick;
                if (callback)
                    callback();
            }
            break;
        }

        case MidiControlId::SUBSEQ_BAR_0:
        case MidiControlId::SUBSEQ_BAR_1:
        case MidiControlId::SUBSEQ_BAR_2:
        case MidiControlId::SUBSEQ_BAR_3:
        case MidiControlId::SUBSEQ_BAR_4:
        case MidiControlId::SUBSEQ_BAR_5:
        case MidiControlId::SUBSEQ_BAR_6:
        case MidiControlId::SUBSEQ_BAR_7: {
            if (isNoteOnOrCCPositive) {
                int barIndex = static_cast<int> (mapping.controlId) - static_cast<int> (MidiControlId::SUBSEQ_BAR_0);
                int selectedStep = state.getSelectedStepForEditing();
                int substepLength = state.getStepNotesLength (selectedStep);
                if (barIndex < substepLength) {
                    state.setSelectedSubstepIndex (selectedStep, barIndex);
                    DBG ("  → Substep " << barIndex << " selected for editing");
                }
                if (onSequenceTick)
                    onSequenceTick();
            }
            break;
        }

        // ========== NOTE RANGE CONTROLS ==========
        // These set the APVTS window params directly so param listeners fire
        case MidiControlId::NOTE_RANGE_MIN: {
            int offset = MidiValueConverter::midiToDiscreteIndex (midiValue, 127);
            state.setScaleWindowOffset (offset);
            DBG ("  → Note range offset set to " << offset);
            if (onSequenceTick)
                onSequenceTick();
            break;
        }

        case MidiControlId::NOTE_RANGE_MAX: {
            int size = MidiValueConverter::midiToDiscreteIndex (midiValue, 127) + 1; // 1-128
            state.setScaleWindowSize (size);
            DBG ("  → Note range size set to " << size);
            if (onSequenceTick)
                onSequenceTick();
            break;
        }

        // ========== SUBDIVISION SLIDER ==========
        case MidiControlId::SUBDIVISION_SLIDER: {
            int subdivisionIndex = MidiValueConverter::midiToDiscreteIndex (midiValue, 3);
            if (auto* param = apvts.getParameter (Params::Keys::ID::CLOCK_SUBDIVISION)) {
                param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (subdivisionIndex)));
                DBG ("  → Subdivision set to " << subdivisionIndex);
            }
            break;
        }

        // ========== PIANO ROLL SCALE NOTES ==========
        case MidiControlId::SCALE_NOTE_C:
        case MidiControlId::SCALE_NOTE_Db:
        case MidiControlId::SCALE_NOTE_D:
        case MidiControlId::SCALE_NOTE_Eb:
        case MidiControlId::SCALE_NOTE_E:
        case MidiControlId::SCALE_NOTE_F:
        case MidiControlId::SCALE_NOTE_Gb:
        case MidiControlId::SCALE_NOTE_G:
        case MidiControlId::SCALE_NOTE_Ab:
        case MidiControlId::SCALE_NOTE_A:
        case MidiControlId::SCALE_NOTE_Bb:
        case MidiControlId::SCALE_NOTE_B: {
            int noteIndex = static_cast<int> (mapping.controlId) - static_cast<int> (MidiControlId::SCALE_NOTE_C);
            bool currentState = state.getGlobalScaleNote(noteIndex);

            // Toggle mode: Note ON toggles, Note OFF does nothing
            if (isNoteOnOrCCPositive) {
                state.setGlobalScaleNote(noteIndex, !currentState);
                DBG ("  → Global scale note " << noteIndex << " toggled to " << (!currentState ? "ON" : "OFF"));
                if (onSequenceTick)
                    onSequenceTick();
            }
            break;
        }

        // ========== NAVIGATION ==========
        case MidiControlId::SETTINGS_TOGGLE: {
            if (isNoteOnOrCCPositive) {
                // This is purely a UI concern - would need UI callback
                DBG ("  → Settings toggle (UI only)");
            }
            break;
        }

        // ========== PRESET CONTROLS ==========
        case MidiControlId::PRESET_CYCLE_UP: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::PRESET_CYCLE_UP)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → PRESET_CYCLE_UP param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::PRESET_CYCLE_DOWN: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::PRESET_CYCLE_DOWN)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → PRESET_CYCLE_DOWN param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== TRANSPORT CONTROLS ==========
        case MidiControlId::PLAY_PAUSE: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::PLAY_PAUSE)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → PLAY_PAUSE param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::STOP: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::STOP)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → STOP param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::SCALE_LATCH: {
            if (auto* param = apvts.getParameter (Params::Keys::ID::SCALE_LATCH)) {
                param->setValueNotifyingHost (isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG ("  → SCALE_LATCH param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::COUNT:
            // This is just a sentinel value, not an actual control
            break;

        default:
            // Unhandled control - log for debugging
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
