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

    // Initialize APVTS-backed state accessor
    sequenceState = std::make_unique<SequenceState>(apvts);

    // Register as listener for all parameter changes
    // This ensures State stays in sync when parameters change from UI
    for (auto* param : apvts.processor.getParameters()) {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*> (param)) {
            apvts.addParameterListener (paramWithID->paramID, this);
        }
    }

    // Initialize clock settings based on host type
    // Default to internal clock for standalone, DAW sync for plugin
    juce::PluginHostType hostType;
    if (hostType.type == juce::PluginHostType::UnknownHost) {
        // Standalone app - use internal clock
        clock.setMode(ClockMode::INTERNAL);
        clock.start();
        sequenceState->setClockMode(0);  // 0 = INTERNAL
    } else {
        // Running in a DAW - use DAW sync
        clock.setMode(ClockMode::DAW_SYNC);
        sequenceState->setClockMode(1);  // 1 = DAW_SYNC
    }
    clock.setBpm(120.0);
    clock.setSubdivision(Subdivision::SIXTEENTH);

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

    // Process incoming MIDI for control mapping (before clearing)
    processMidiInput(midiMessages);

    // Clear incoming MIDI (we're generating, not processing MIDI through)
    midiMessages.clear();

    // Execute any queued commands from UI thread (thread-safe state modifications)
    int numCommands = commandQueue.executeAll();
    if (numCommands > 0) {
        DBG("Executed " << numCommands << " queued commands");

        // If any commands had completion callbacks, notify the UI to execute them
        if (commandQueue.hasCompletionCallbacks() && onCompletionCallbacksPending) {
            onCompletionCallbacksPending();
        }
    }

    // Process any queued preview MIDI messages (from UI thread)
    int start1, size1, start2, size2;
    previewMidiFifo.prepareToRead(previewMidiFifo.getNumReady(), start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i) {
        const auto& msg = previewMidiBuffer[start1 + i];
        if (msg.isNoteOn) {
            auto noteOn = juce::MidiMessage::noteOn(1, msg.noteNumber, msg.velocity);
            midiMessages.addEvent(noteOn, 0);
            DBG("Preview MIDI Note ON added to buffer: " << msg.noteNumber);
        } else {
            auto noteOff = juce::MidiMessage::noteOff(1, msg.noteNumber, msg.velocity);
            midiMessages.addEvent(noteOff, 0);
            DBG("Preview MIDI Note OFF added to buffer: " << msg.noteNumber);
        }
    }

    for (int i = 0; i < size2; ++i) {
        const auto& msg = previewMidiBuffer[start2 + i];
        if (msg.isNoteOn) {
            auto noteOn = juce::MidiMessage::noteOn(1, msg.noteNumber, msg.velocity);
            midiMessages.addEvent(noteOn, 0);
            DBG("Preview MIDI Note ON added to buffer: " << msg.noteNumber);
        } else {
            auto noteOff = juce::MidiMessage::noteOff(1, msg.noteNumber, msg.velocity);
            midiMessages.addEvent(noteOff, 0);
            DBG("Preview MIDI Note OFF added to buffer: " << msg.noteNumber);
        }
    }

    if (size1 + size2 > 0) {
        previewMidiFifo.finishedRead(size1 + size2);
    }

    // Check if scheduled note-off should be sent
    if (scheduledNoteOffSample >= 0 && currentSamplePosition >= scheduledNoteOffSample) {
        auto noteOff = juce::MidiMessage::noteOff(1, scheduledNoteOffNote, scheduledNoteOffVelocity);
        midiMessages.addEvent(noteOff, 0);

        // Send directly to MIDI output device (for standalone mode)
        if (midiOutput) {
            midiOutput->sendMessageNow(noteOff);
        }
        DBG("Scheduled MIDI Note OFF: " << scheduledNoteOffNote << " at sample " << currentSamplePosition);

        // Clear scheduled note-off
        scheduledNoteOffSample = -1;
        scheduledNoteOffNote = -1;
    }

    // Update current sample position
    currentSamplePosition += buffer.getNumSamples();

    // Process clock based on current mode
    if (clock.getMode() == ClockMode::DAW_SYNC) {
        // DAW sync mode: get timing from host transport
        if (auto* playHead = getPlayHead()) {
            auto position = playHead->getPosition();
            int pulseCount = dawSync.update(position);
            clock.updateDawSync(pulseCount);
        }
    } else {
        // Internal clock mode: process samples for timing
        clock.processInternalClock(buffer.getNumSamples());
    }

    // Check if clock triggered a tick
    if (clock.shouldTick()) {
        // Cancel any scheduled note-off (we're starting a new step)
        scheduledNoteOffSample = -1;
        scheduledNoteOffNote = -1;

        // Send note off for previous note immediately (if any)
        if (lastPlayedNote >= 0) {
            auto noteOff = juce::MidiMessage::noteOff(1, lastPlayedNote, (juce::uint8)0);
            midiMessages.addEvent(noteOff, 0);

            // Send directly to MIDI output device (for standalone mode)
            if (midiOutput) {
                midiOutput->sendMessageNow(noteOff);
            }
            DBG("MIDI Note OFF: " << lastPlayedNote);
        }

        // Tick the sequence to get the next note (using APVTS-backed state)
        auto& state = getState();
        int note = state.tick();
        int currentStepIndex = state.getCurrentIndex();

        DBG("Clock tick! Step: " << currentStepIndex << ", Note: " << note);

        if (note >= 0) {
            // Get step modulations for this step (using APVTS-backed state)
            auto selectedMods = state.getSelectedModulations(currentStepIndex);

            // Determine velocity (default to 100 if no velocity modulation is selected)
            juce::uint8 velocity = 100;

            // Send CC messages and check for velocity modulation
            for (const auto& mod : selectedMods) {
                if (mod.getType() == ModulationType::Velocity) {
                    // Use this as the note velocity
                    velocity = static_cast<juce::uint8>(mod.getValue());
                } else if (mod.getType() == ModulationType::CC) {
                    // Send CC message
                    auto ccMsg = juce::MidiMessage::controllerEvent(1, mod.getCCNumber(), mod.getValue());
                    midiMessages.addEvent(ccMsg, 0);

                    // Send directly to MIDI output device (for standalone mode)
                    if (midiOutput) {
                        midiOutput->sendMessageNow(ccMsg);
                    }
                    DBG("MIDI CC: " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }

            // Valid note - send note on with velocity
            auto noteOn = juce::MidiMessage::noteOn(1, note, velocity);
            midiMessages.addEvent(noteOn, 0);

            // Send directly to MIDI output device (for standalone mode)
            if (midiOutput) {
                midiOutput->sendMessageNow(noteOn);
            }
            DBG("MIDI Note ON: " << note << " (velocity " << (int)velocity << ", channel 1)");
            lastPlayedNote = note;

            // Schedule note-off based on note length setting
            // Calculate samples per step based on current clock settings
            double samplesPerStep = 0.0;
            if (clock.getMode() == ClockMode::INTERNAL) {
                samplesPerStep = clock.getInternalClock().calculateSamplesPerPulse();
            } else {
                // For DAW sync, we need to calculate based on DAW tempo
                // Get tempo from play head if available
                if (auto* playHead = getPlayHead()) {
                    auto position = playHead->getPosition();
                    if (position.hasValue() && position->getBpm().hasValue()) {
                        double bpm = *position->getBpm();
                        double sampleRate = getSampleRate();
                        int pulsesPerQuarterNote = getPulsesPerQuarterNote(clock.getSubdivision());
                        double pulsesPerSecond = (bpm * pulsesPerQuarterNote) / 60.0;
                        samplesPerStep = sampleRate / pulsesPerSecond;
                    }
                }
            }

            if (samplesPerStep > 0.0) {
                // Calculate note-off time based on note length percentage (using APVTS-backed state)
                double noteLengthPercent = state.getNoteLength();
                juce::int64 noteOffDelay = static_cast<juce::int64>(samplesPerStep * noteLengthPercent);
                scheduledNoteOffSample = currentSamplePosition + noteOffDelay;
                scheduledNoteOffNote = note;
                scheduledNoteOffVelocity = 0;  // Standard note-off velocity

                DBG("Scheduled note-off for note " << note << " at sample " << scheduledNoteOffSample
                    << " (delay: " << noteOffDelay << " samples, " << (noteLengthPercent * 100.0) << "% of step)");
            }
        } else {
            // Rest - no note to play
            lastPlayedNote = -1;
            // Note: Rest recording is now handled inside state.tick() when roll is engaged
            if (state.getIsRandom()) {
                DBG("Rest recorded at step: " << currentStepIndex);
            }
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
    // Create root XML element to hold all state
    auto rootXml = std::make_unique<juce::XmlElement>("GamblerState");

    // Save APVTS parameters (contains all sequence data now)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement(apvtsXml.release());

    // Save Clock settings (mode, BPM, subdivision)
    rootXml->addChildElement(clock.toXml().release());

    // Save User Preferences
    rootXml->addChildElement(userPreferences.toXml().release());

    // Convert to binary and save
    copyXmlToBinary(*rootXml, destData);

    DBG("State saved: APVTS (sequence data), Clock, and UserPreferences");
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // Load XML from binary data
    std::unique_ptr<juce::XmlElement> rootXml (getXmlFromBinary (data, sizeInBytes));

    if (rootXml != nullptr) {
        // Check if this is the new format (GamblerState root element)
        if (rootXml->hasTagName("GamblerState")) {
            DBG("Loading state: GamblerState format");

            // Load APVTS parameters (contains all sequence data)
            if (auto* apvtsXml = rootXml->getChildByName(apvts.state.getType())) {
                apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));
                DBG("  - APVTS loaded (sequence data included)");
            }

            // Load Clock settings
            if (auto* clockXml = rootXml->getChildByName("Clock")) {
                clock.fromXml(*clockXml);
                juce::String modeStr = (clock.getMode() == ClockMode::INTERNAL) ? "INTERNAL" : "DAW_SYNC";
                DBG("  - Clock loaded (mode: " + modeStr + ", BPM: " + juce::String(clock.getBpm()) + ")");
            }

            // Load User Preferences
            if (auto* prefsXml = rootXml->getChildByName("UserPreferences")) {
                userPreferences.fromXml(*prefsXml);
                DBG("  - UserPreferences loaded (previewNotesOnEdit: " +
                    juce::String(userPreferences.previewNotesOnEdit ? "true" : "false") + ")");
            }
            // Backward compatibility: Load from old "Settings" element
            else if (auto* settingsXml = rootXml->getChildByName("Settings")) {
                userPreferences.previewNotesOnEdit = settingsXml->getBoolAttribute("previewNotesOnEdit", true);
                DBG("  - UserPreferences loaded from legacy Settings format");
            }
        }
        // Fallback: Old format (just APVTS)
        else if (rootXml->hasTagName(apvts.state.getType())) {
            DBG("Loading state: Legacy APVTS-only format");
            apvts.replaceState(juce::ValueTree::fromXml(*rootXml));
        }
    }
}

//==============================================================================
// APVTS Parameter Listener
//==============================================================================

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
        getState().setIsRandom(isOn);
        DBG("ROLL " + juce::String(isOn ? "ON" : "OFF") + " - isRandom: " + juce::String(getState().getIsRandom() ? "true" : "false"));
    }

    // PAGE_LEFT and PAGE_RIGHT are handled in UI (need window position)
    // The param change will trigger UI callback via StateListenerComponent

    if (parameterID == Params::Keys::ID::LENGTH_PLUS && isOn) {
        getState().increaseLength();
        DBG("LENGTH_PLUS triggered - new length: " + juce::String(getState().getLength()));
    }

    if (parameterID == Params::Keys::ID::LENGTH_MINUS && isOn) {
        getState().decreaseLength();
        DBG("LENGTH_MINUS triggered - new length: " + juce::String(getState().getLength()));
    }

    if (parameterID == Params::Keys::ID::NUDGE_LEFT && isOn) {
        getState().nudgeLeft();
        DBG("NUDGE_LEFT triggered");
    }

    if (parameterID == Params::Keys::ID::NUDGE_RIGHT && isOn) {
        getState().nudgeRight();
        DBG("NUDGE_RIGHT triggered");
    }

    if (parameterID == Params::Keys::ID::SHIFT) {
        // Shift is a toggle - set isShift to match param state
        getState().setIsShift(isOn);
        DBG("SHIFT " + juce::String(isOn ? "ON" : "OFF"));
    }

    // ========== SECONDARY ROW BUTTONS ==========

    if (parameterID == Params::Keys::ID::DUPLICATE && isOn) {
        auto& state = getState();
        int currentLength = state.getLength();
        int newLength = juce::jmin(currentLength * 2, ParamConstants::MAX_STEPS);
        int stepsToCopy = newLength - currentLength;

        for (int i = 0; i < stepsToCopy; i++) {
            int srcIdx = i;
            int dstIdx = currentLength + i;
            state.setStepNotesLength(dstIdx, state.getStepNotesLength(srcIdx));
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote(dstIdx, n, state.getStepNote(srcIdx, n));
            }
        }
        state.setLength(newLength);
        DBG("DUPLICATE triggered - new length: " + juce::String(newLength));
    }

    // CLEAR_WINDOW is handled in UI (needs window position)

    if (parameterID == Params::Keys::ID::CLEAR_STEP && isOn) {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
            state.setStepNote(selectedStep, n, -1);
        }
        DBG("CLEAR_STEP triggered - cleared step " + juce::String(selectedStep));
    }

    if (parameterID == Params::Keys::ID::RANDOMIZE_STEP && isOn) {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        state.randomizeStepNote(selectedStep);
        DBG("RANDOMIZE_STEP triggered - randomized step " + juce::String(selectedStep));
    }

    if (parameterID == Params::Keys::ID::RANDOMIZE_ALL && isOn) {
        auto& state = getState();
        for (int i = 0; i < state.getLength(); i++) {
            state.randomizeStepNote(i);
        }
        DBG("RANDOMIZE_ALL triggered");
    }

    if (parameterID == Params::Keys::ID::SHUFFLE && isOn) {
        auto& state = getState();
        int len = state.getLength();
        static std::random_device rd;
        static std::mt19937 gen(rd());
        for (int i = len - 1; i > 0; i--) {
            std::uniform_int_distribution<> dis(0, i);
            int j = dis(gen);
            // Swap steps i and j
            int lenI = state.getStepNotesLength(i);
            std::array<int, ParamConstants::MAX_SUBSTEPS> notesI;
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                notesI[static_cast<size_t>(n)] = state.getStepNote(i, n);
            }
            // Copy j to i
            state.setStepNotesLength(i, state.getStepNotesLength(j));
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote(i, n, state.getStepNote(j, n));
            }
            // Copy saved i to j
            state.setStepNotesLength(j, lenI);
            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                state.setStepNote(j, n, notesI[static_cast<size_t>(n)]);
            }
        }
        DBG("SHUFFLE triggered");
    }
}

//==============================================================================
// UI Preview Methods
//==============================================================================

void PluginProcessor::triggerStepMidi(int stepIndex) {
    if (stepIndex < 0 || stepIndex >= 64) return;

    auto& state = getState();

    // Set the selected step for editing via APVTS (thread-safe)
    state.setSelectedStepForEditing(stepIndex);

    // Get the note for this step from APVTS-backed state
    int note = state.getCurrentNoteForStep(stepIndex);

    // Get step modulations from APVTS-backed state
    std::vector<StepModulation> selectedMods;
    for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
        if (state.getStepModSelected(stepIndex, i)) {
            StepModulation mod;
            mod.setType(static_cast<ModulationType>(state.getStepModType(stepIndex, i)));
            mod.setCCNumber(state.getStepModCCNum(stepIndex, i));
            mod.setValue(state.getStepModValue(stepIndex, i));
            selectedMods.push_back(mod);
        }
    }

    // Determine velocity (default to 100 if no velocity modulation is selected)
    juce::uint8 velocity = 100;
    for (const auto& mod : selectedMods) {
        if (mod.getType() == ModulationType::Velocity) {
            velocity = static_cast<juce::uint8>(mod.getValue());
            break;
        }
    }

    // Send note off for previous note (if any)
    if (lastPlayedNote >= 0) {
        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOff = juce::MidiMessage::noteOff(1, lastPlayedNote, (juce::uint8)0);
            midiOutput->sendMessageNow(noteOff);
            DBG("Preview MIDI Note OFF (standalone): " << lastPlayedNote);
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { lastPlayedNote, false, 0 };
                previewMidiFifo.finishedWrite(1);
                DBG("Preview MIDI Note OFF queued (DAW): " << lastPlayedNote);
            }
        }
    }

    // Send note on for the new note
    if (note >= 0) {
        // Send CC messages first (only in standalone mode for now)
        // In DAW mode, CC messages would need to be queued via the audio thread
        if (midiOutput) {
            for (const auto& mod : selectedMods) {
                if (mod.getType() == ModulationType::CC) {
                    auto ccMsg = juce::MidiMessage::controllerEvent(1, mod.getCCNumber(), mod.getValue());
                    midiOutput->sendMessageNow(ccMsg);
                    DBG("Preview MIDI CC (standalone): " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }
        }

        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOn = juce::MidiMessage::noteOn(1, note, velocity);
            midiOutput->sendMessageNow(noteOn);
            DBG("Preview MIDI Note ON (standalone): " << note << " (velocity " << (int)velocity << ", step " << stepIndex << ")");
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { note, true, velocity };
                previewMidiFifo.finishedWrite(1);
                DBG("Preview MIDI Note ON queued (DAW): " << note << " (velocity " << (int)velocity << ", step " << stepIndex << ")");
            }
        }
        lastPlayedNote = note;
    } else {
        lastPlayedNote = -1;
    }

    // Notify UI that state changed
    if (onSequenceTick) {
        onSequenceTick();
    }
}

void PluginProcessor::triggerCurrentStepMidi(int noteOverride) {
    DBG("=== triggerCurrentStepMidi() called ===");
    DBG("  midiOutput is " << (midiOutput ? "VALID" : "NULL"));
    DBG("  noteOverride: " << noteOverride);

    auto& state = getState();

    int selectedStep = state.getSelectedStepForEditing();
    if (selectedStep < 0 || selectedStep >= 64) {
        DBG("  ERROR: Invalid selected step: " << selectedStep);
        return;
    }

    // Get step modulations from APVTS-backed state
    std::vector<StepModulation> selectedMods;
    for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
        if (state.getStepModSelected(selectedStep, i)) {
            StepModulation mod;
            mod.setType(static_cast<ModulationType>(state.getStepModType(selectedStep, i)));
            mod.setCCNumber(state.getStepModCCNum(selectedStep, i));
            mod.setValue(state.getStepModValue(selectedStep, i));
            selectedMods.push_back(mod);
        }
    }

    // Determine velocity (default to 100 if no velocity modulation is selected)
    juce::uint8 velocity = 100;
    for (const auto& mod : selectedMods) {
        if (mod.getType() == ModulationType::Velocity) {
            velocity = static_cast<juce::uint8>(mod.getValue());
            break;
        }
    }

    // Use override note if provided, otherwise read from step
    int note;
    if (noteOverride >= 0) {
        note = noteOverride;
        DBG("  Using override note: " << note);
    } else {
        note = state.getCurrentNoteForStep(selectedStep);
        DBG("  Selected step: " << selectedStep << ", Note from step: " << note);
    }

    // Send note off for previous note (if any)
    if (lastPlayedNote >= 0) {
        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOff = juce::MidiMessage::noteOff(1, lastPlayedNote, (juce::uint8)0);
            midiOutput->sendMessageNow(noteOff);
            DBG("  Preview MIDI Note OFF sent (standalone): " << lastPlayedNote);
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { lastPlayedNote, false, 0 };
                previewMidiFifo.finishedWrite(1);
                DBG("  Preview MIDI Note OFF queued (DAW): " << lastPlayedNote);
            } else {
                DBG("  Preview MIDI queue full, Note OFF dropped: " << lastPlayedNote);
            }
        }
    }

    // Send note on for the new note
    if (note >= 0) {
        // Send CC messages first (only in standalone mode for now)
        // In DAW mode, CC messages would need to be queued via the audio thread
        if (midiOutput) {
            for (const auto& mod : selectedMods) {
                if (mod.getType() == ModulationType::CC) {
                    auto ccMsg = juce::MidiMessage::controllerEvent(1, mod.getCCNumber(), mod.getValue());
                    midiOutput->sendMessageNow(ccMsg);
                    DBG("  Preview MIDI CC (standalone): " << mod.getCCNumber() << " = " << mod.getValue());
                }
            }
        }

        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOn = juce::MidiMessage::noteOn(1, note, velocity);
            midiOutput->sendMessageNow(noteOn);
            DBG("  Preview MIDI Note ON sent (standalone): " << note << " (velocity " << (int)velocity << ")");
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { note, true, velocity };
                previewMidiFifo.finishedWrite(1);
                DBG("  Preview MIDI Note ON queued (DAW): " << note << " (velocity " << (int)velocity << ")");
            } else {
                DBG("  Preview MIDI queue full, Note ON dropped: " << note);
            }
        }
        lastPlayedNote = note;
    } else {
        lastPlayedNote = -1;
        DBG("  No note to play (note < 0)");
    }
    DBG("=== triggerCurrentStepMidi() complete ===");
}

//==============================================================================
// Preset State Methods
//==============================================================================

std::unique_ptr<juce::XmlElement> PluginProcessor::getStateAsXml() {
    // Create root XML element to hold all state (same format as getStateInformation)
    auto rootXml = std::make_unique<juce::XmlElement>("GamblerState");

    // Save APVTS parameters (contains all sequence data)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement(apvtsXml.release());

    // Save Clock settings (mode, BPM, subdivision)
    rootXml->addChildElement(clock.toXml().release());

    // Save User Preferences
    rootXml->addChildElement(userPreferences.toXml().release());

    // Save MIDI Learn Mappings
    rootXml->addChildElement(midiLearnRegistry.toXml().release());

    return rootXml;
}

void PluginProcessor::loadStateFromXml(const juce::XmlElement& xml) {
    if (!xml.hasTagName("GamblerState")) {
        DBG("PluginProcessor::loadStateFromXml - Invalid XML format (expected GamblerState root)");
        return;
    }

    DBG("Loading preset state from XML");

    // Load APVTS parameters (contains all sequence data)
    if (auto* apvtsXml = xml.getChildByName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));
        DBG("  - APVTS loaded (sequence data included)");
    }

    // Queue command to load domain state on audio thread
    // We need to copy the XML data since the original may be destroyed
    auto xmlCopy = std::make_shared<juce::XmlElement>(xml);

    queueCommand(ProcessorCommand(
        [this, xmlCopy]() {
            // Load Clock settings
            if (auto* clockXml = xmlCopy->getChildByName("Clock")) {
                clock.fromXml(*clockXml);
                juce::String modeStr = (clock.getMode() == ClockMode::INTERNAL) ? "INTERNAL" : "DAW_SYNC";
                DBG("  - Clock loaded (mode: " + modeStr + ", BPM: " + juce::String(clock.getBpm()) + ")");
            }

            // Load User Preferences
            if (auto* prefsXml = xmlCopy->getChildByName("UserPreferences")) {
                userPreferences.fromXml(*prefsXml);
                DBG("  - UserPreferences loaded");
            }

            // Load MIDI Learn Mappings
            if (auto* midiLearnXml = xmlCopy->getChildByName("MidiLearnMappings")) {
                midiLearnRegistry.fromXml(*midiLearnXml);
                DBG("  - MIDI Learn Mappings loaded");
            }
        }
    ));
}

//==============================================================================
// MIDI Input Processing
//==============================================================================

void PluginProcessor::processMidiInput(const juce::MidiBuffer& midiMessages) {
    // Iterate through all incoming MIDI messages
    for (const auto& metadata : midiMessages) {
        auto msg = metadata.getMessage();

        // Process CC messages
        if (msg.isController()) {
            int ccNumber = msg.getControllerNumber();
            int ccValue = msg.getControllerValue();
            int channel = msg.getChannel();

            // Check if we're in MIDI learn mode first
            if (midiLearnRegistry.isLearning()) {
                bool learned = midiLearnRegistry.learnMidiMessage(
                    MidiMessageType::CC, channel, ccNumber);
                if (learned) {
                    DBG("Learned: CH" << channel << "-CC" << ccNumber);
                    // Trigger UI update callback
                    if (onMidiLearned) {
                        DBG("  Calling onMidiLearned callback");
                        onMidiLearned();
                    } else {
                        DBG("  WARNING: onMidiLearned callback is null!");
                    }
                    continue;  // Don't process as regular mapping
                }
            }

            // Check user mappings first (higher priority than defaults)
            MidiControlId userControlId = midiLearnRegistry.findUserMapping(
                MidiMessageType::CC, channel, ccNumber);

            if (userControlId != MidiControlId::COUNT) {
                // User mapping found - create temporary mapping struct
                MidiMapping userMapping;
                userMapping.type = MidiMessageType::CC;
                userMapping.midiNumber = ccNumber;
                userMapping.controlId = userControlId;
                userMapping.minValue = 0.0f;
                userMapping.maxValue = 1.0f;
                userMapping.invert = false;

                DBG("MIDI CC " << ccNumber << " (user) = " << ccValue << " → " <<
                    getMidiControlName(userControlId));
                handleMidiMapping(userMapping, ccValue, ccValue >= 64);
            }
            else {
                // Fall back to hardcoded default mappings
                const MidiMapping* mapping = midiMappingManager.findCCMapping(ccNumber);
                if (mapping) {
                    DBG("MIDI CC " << ccNumber << " (default) = " << ccValue << " → " <<
                        getMidiControlName(mapping->controlId));
                    handleMidiMapping(*mapping, ccValue, ccValue >= 64);
                }
            }
        }
        // Process Note On/Off messages
        else if (msg.isNoteOn() || msg.isNoteOff()) {
            int noteNumber = msg.getNoteNumber();
            bool isNoteOn = msg.isNoteOn();
            int channel = msg.getChannel();

            // Check if we're in MIDI learn mode first
            if (midiLearnRegistry.isLearning() && isNoteOn) {
                bool learned = midiLearnRegistry.learnMidiMessage(
                    MidiMessageType::NOTE, channel, noteNumber);
                if (learned) {
                    DBG("Learned: CH" << channel << "-N" << noteNumber);
                    // Trigger UI update callback
                    if (onMidiLearned) {
                        DBG("  Calling onMidiLearned callback");
                        onMidiLearned();
                    } else {
                        DBG("  WARNING: onMidiLearned callback is null!");
                    }
                    continue;  // Don't process as regular mapping
                }
            }

            // Check user mappings first (higher priority than defaults)
            MidiControlId userControlId = midiLearnRegistry.findUserMapping(
                MidiMessageType::NOTE, channel, noteNumber);

            if (userControlId != MidiControlId::COUNT) {
                // User mapping found
                MidiMapping userMapping;
                userMapping.type = MidiMessageType::NOTE;
                userMapping.midiNumber = noteNumber;
                userMapping.controlId = userControlId;
                userMapping.minValue = 0.0f;
                userMapping.maxValue = 1.0f;
                userMapping.invert = false;

                DBG("MIDI Note " << noteNumber << " (user) " << (isNoteOn ? "ON" : "OFF") <<
                    " → " << getMidiControlName(userControlId));
                int value = isNoteOn ? 127 : 0;
                handleMidiMapping(userMapping, value, isNoteOn);
            }
            else {
                // Fall back to hardcoded default mappings
                const MidiMapping* mapping = midiMappingManager.findNoteMapping(noteNumber);
                if (mapping) {
                    DBG("MIDI Note " << noteNumber << " (default) " << (isNoteOn ? "ON" : "OFF") <<
                        " → " << getMidiControlName(mapping->controlId));
                    int value = isNoteOn ? 127 : 0;
                    handleMidiMapping(*mapping, value, isNoteOn);
                }
            }
        }
    }
}

void PluginProcessor::handleMidiMapping(const MidiMapping& mapping, int midiValue, bool isNoteOnOrCCPositive) {
    auto& state = getState();

    // Convert MIDI value using the mapping's conversion settings
    float normalizedValue = MidiValueConverter::midiToScaled(
        midiValue, mapping.minValue, mapping.maxValue, mapping.invert);

    // Handle each control type
    switch (mapping.controlId) {
        // ========== OCTAVE SLIDER ==========
        case MidiControlId::OCTAVE_SLIDER: {
            int octaveIndex = MidiValueConverter::midiToDiscreteIndex(midiValue, 9);  // 0-9
            // Update via APVTS-backed state (thread-safe)
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int pitchClass = currentNote % 12;
            int newNote = octaveIndex * 12 + pitchClass;
            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("  → Octave changed to " << octaveIndex << " (MIDI note " << newNote << ")");
            if (onSequenceTick) onSequenceTick();
            break;
        }

        // ========== NOTE SLIDER ==========
        case MidiControlId::NOTE_SLIDER: {
            int pitchClass = MidiValueConverter::midiToDiscreteIndex(midiValue, 11);  // 0-11
            // Update via APVTS-backed state (thread-safe)
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int octave = currentNote / 12;
            int newNote = octave * 12 + pitchClass;
            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("  → Note changed to pitch class " << pitchClass << " (MIDI note " << newNote << ")");
            if (onSequenceTick) onSequenceTick();
            break;
        }

        // ========== PROBABILITY SLIDER ==========
        case MidiControlId::PROBABILITY_SLIDER: {
            int percentage = static_cast<int>(normalizedValue * 100.0f);
            state.setProbabilityPercentage(percentage);
            DBG("  → Probability changed to " << percentage << "%");
            if (onSequenceTick) onSequenceTick();
            break;
        }

        // ========== SEQUENCER CONTROL BUTTONS - PRIMARY ROW ==========
        // All buttons use param-based approach: set param on note-on/off,
        // parameterChanged handles the action, UI observes param for visual state

        case MidiControlId::BUTTON_ROLL: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::ROLL)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → ROLL param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_PAGE_LEFT: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::INDEX_LEFT)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → INDEX_LEFT param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_PAGE_RIGHT: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::INDEX_RIGHT)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → INDEX_RIGHT param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_LENGTH_PLUS: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::LENGTH_PLUS)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → LENGTH_PLUS param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_LENGTH_MINUS: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::LENGTH_MINUS)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → LENGTH_MINUS param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_NUDGE_LEFT: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::NUDGE_LEFT)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → NUDGE_LEFT param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_NUDGE_RIGHT: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::NUDGE_RIGHT)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → NUDGE_RIGHT param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_SHIFT: {
            // Shift is a toggle - only toggle on note-on
            if (isNoteOnOrCCPositive) {
                if (auto* param = apvts.getParameter(Params::Keys::ID::SHIFT)) {
                    bool currentState = param->getValue() > 0.5f;
                    param->setValueNotifyingHost(currentState ? 0.0f : 1.0f);
                    DBG("  → SHIFT toggled to " << (!currentState ? "ON" : "OFF"));
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
                int localStepIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::STEP_0);
                DBG("  → Step " << localStepIndex << " clicked");
            }
            break;
        }

        // ========== SUBSEQUENCE CONTROLS ==========
        case MidiControlId::SUBSEQ_MINUS: {
            if (isNoteOnOrCCPositive) {
                int selectedStep = state.getSelectedStepForEditing();
                int currentLength = state.getStepNotesLength(selectedStep);
                if (currentLength > 1) {
                    state.setStepNotesLength(selectedStep, currentLength - 1);
                    DBG("  → Substep length decreased to " << (currentLength - 1));
                }
                if (onSequenceTick) onSequenceTick();
            }
            break;
        }

        case MidiControlId::SUBSEQ_PLUS: {
            if (isNoteOnOrCCPositive) {
                int selectedStep = state.getSelectedStepForEditing();
                int currentLength = state.getStepNotesLength(selectedStep);
                if (currentLength < ParamConstants::MAX_SUBSTEPS) {
                    state.setStepNotesLength(selectedStep, currentLength + 1);
                    DBG("  → Substep length increased to " << (currentLength + 1));
                }
                if (onSequenceTick) onSequenceTick();
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
                int barIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::SUBSEQ_BAR_0);
                int selectedStep = state.getSelectedStepForEditing();
                int substepLength = state.getStepNotesLength(selectedStep);
                if (barIndex < substepLength) {
                    state.setStepNotesCurrentIndex(selectedStep, barIndex);
                    DBG("  → Substep " << barIndex << " selected");
                }
                if (onSequenceTick) onSequenceTick();
            }
            break;
        }

        // ========== NOTE RANGE CONTROLS ==========
        case MidiControlId::NOTE_RANGE_MIN: {
            int minNote = MidiValueConverter::midiToDiscreteIndex(midiValue, 127);
            state.setScaleRangeMin(minNote);
            DBG("  → Note range min set to " << minNote);
            if (onSequenceTick) onSequenceTick();
            break;
        }

        case MidiControlId::NOTE_RANGE_MAX: {
            int maxNote = MidiValueConverter::midiToDiscreteIndex(midiValue, 127);
            state.setScaleRangeMax(maxNote);
            DBG("  → Note range max set to " << maxNote);
            if (onSequenceTick) onSequenceTick();
            break;
        }

        // ========== CC LANE VALUES ==========
        case MidiControlId::CC_LANE_0_VALUE:
        case MidiControlId::CC_LANE_1_VALUE:
        case MidiControlId::CC_LANE_2_VALUE:
        case MidiControlId::CC_LANE_3_VALUE:
        case MidiControlId::CC_LANE_4_VALUE:
        case MidiControlId::CC_LANE_5_VALUE:
        case MidiControlId::CC_LANE_6_VALUE: {
            int laneIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::CC_LANE_0_VALUE);
            int ccValue = static_cast<int>(normalizedValue * 127.0f);
            if (laneIndex >= 0 && laneIndex < ParamConstants::NUM_MODULATIONS) {
                int selectedStep = state.getSelectedStepForEditing();
                state.setStepModValue(selectedStep, laneIndex, ccValue);
                DBG("  → CC Lane " << laneIndex << " value set to " << ccValue);
            }
            if (onSequenceTick) onSequenceTick();
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
            if (isNoteOnOrCCPositive) {
                int noteIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::SCALE_NOTE_C);
                bool currentState = state.getScaleNote(noteIndex);

                // Only toggle if there's more than one note in the scale
                int activeNotes = 0;
                for (int i = 0; i < 12; i++) {
                    if (state.getScaleNote(i)) activeNotes++;
                }

                if (activeNotes > 1 || !currentState) {
                    state.setScaleNote(noteIndex, !currentState);
                    state.quantizeAllNotesToScale();
                    DBG("  → Scale note " << noteIndex << " toggled to " << (!currentState ? "ON" : "OFF"));
                } else {
                    DBG("  → Cannot disable last scale note");
                }
                if (onSequenceTick) onSequenceTick();
            }
            break;
        }

        // ========== SEQUENCER CONTROL BUTTONS - SECONDARY ROW ==========
        // All buttons use param-based approach

        case MidiControlId::BUTTON_DUPLICATE: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::DUPLICATE)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → DUPLICATE param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_CLEAR_WINDOW: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::CLEAR_WINDOW)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → CLEAR_WINDOW param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_CLEAR_STEP: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::CLEAR_STEP)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → CLEAR_STEP param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_RANDOMIZE_STEP: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::RANDOMIZE_STEP)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → RANDOMIZE_STEP param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_RANDOMIZE_ALL: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::RANDOMIZE_ALL)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → RANDOMIZE_ALL param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        case MidiControlId::BUTTON_SHUFFLE: {
            if (auto* param = apvts.getParameter(Params::Keys::ID::SHUFFLE)) {
                param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
                DBG("  → SHUFFLE param set to " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }
            break;
        }

        // ========== CC LANE TOGGLES ==========
        case MidiControlId::CC_LANE_0_TOGGLE:
        case MidiControlId::CC_LANE_1_TOGGLE:
        case MidiControlId::CC_LANE_2_TOGGLE:
        case MidiControlId::CC_LANE_3_TOGGLE:
        case MidiControlId::CC_LANE_4_TOGGLE:
        case MidiControlId::CC_LANE_5_TOGGLE:
        case MidiControlId::CC_LANE_6_TOGGLE: {
            if (isNoteOnOrCCPositive) {
                int laneIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::CC_LANE_0_TOGGLE);
                if (laneIndex >= 0 && laneIndex < ParamConstants::NUM_MODULATIONS) {
                    int selectedStep = state.getSelectedStepForEditing();
                    bool currentSelected = state.getStepModSelected(selectedStep, laneIndex);
                    state.setStepModSelected(selectedStep, laneIndex, !currentSelected);
                    DBG("  → CC Lane " << laneIndex << " toggled to " << (!currentSelected ? "ON" : "OFF"));
                }
                if (onSequenceTick) onSequenceTick();
            }
            break;
        }

        // ========== SEQUENCE GRID (64 absolute steps) ==========
        case MidiControlId::GRID_STEP_0:  case MidiControlId::GRID_STEP_1:
        case MidiControlId::GRID_STEP_2:  case MidiControlId::GRID_STEP_3:
        case MidiControlId::GRID_STEP_4:  case MidiControlId::GRID_STEP_5:
        case MidiControlId::GRID_STEP_6:  case MidiControlId::GRID_STEP_7:
        case MidiControlId::GRID_STEP_8:  case MidiControlId::GRID_STEP_9:
        case MidiControlId::GRID_STEP_10: case MidiControlId::GRID_STEP_11:
        case MidiControlId::GRID_STEP_12: case MidiControlId::GRID_STEP_13:
        case MidiControlId::GRID_STEP_14: case MidiControlId::GRID_STEP_15:
        case MidiControlId::GRID_STEP_16: case MidiControlId::GRID_STEP_17:
        case MidiControlId::GRID_STEP_18: case MidiControlId::GRID_STEP_19:
        case MidiControlId::GRID_STEP_20: case MidiControlId::GRID_STEP_21:
        case MidiControlId::GRID_STEP_22: case MidiControlId::GRID_STEP_23:
        case MidiControlId::GRID_STEP_24: case MidiControlId::GRID_STEP_25:
        case MidiControlId::GRID_STEP_26: case MidiControlId::GRID_STEP_27:
        case MidiControlId::GRID_STEP_28: case MidiControlId::GRID_STEP_29:
        case MidiControlId::GRID_STEP_30: case MidiControlId::GRID_STEP_31:
        case MidiControlId::GRID_STEP_32: case MidiControlId::GRID_STEP_33:
        case MidiControlId::GRID_STEP_34: case MidiControlId::GRID_STEP_35:
        case MidiControlId::GRID_STEP_36: case MidiControlId::GRID_STEP_37:
        case MidiControlId::GRID_STEP_38: case MidiControlId::GRID_STEP_39:
        case MidiControlId::GRID_STEP_40: case MidiControlId::GRID_STEP_41:
        case MidiControlId::GRID_STEP_42: case MidiControlId::GRID_STEP_43:
        case MidiControlId::GRID_STEP_44: case MidiControlId::GRID_STEP_45:
        case MidiControlId::GRID_STEP_46: case MidiControlId::GRID_STEP_47:
        case MidiControlId::GRID_STEP_48: case MidiControlId::GRID_STEP_49:
        case MidiControlId::GRID_STEP_50: case MidiControlId::GRID_STEP_51:
        case MidiControlId::GRID_STEP_52: case MidiControlId::GRID_STEP_53:
        case MidiControlId::GRID_STEP_54: case MidiControlId::GRID_STEP_55:
        case MidiControlId::GRID_STEP_56: case MidiControlId::GRID_STEP_57:
        case MidiControlId::GRID_STEP_58: case MidiControlId::GRID_STEP_59:
        case MidiControlId::GRID_STEP_60: case MidiControlId::GRID_STEP_61:
        case MidiControlId::GRID_STEP_62: case MidiControlId::GRID_STEP_63: {
            if (isNoteOnOrCCPositive) {
                int gridStepIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::GRID_STEP_0);
                state.setSelectedStepForEditing(gridStepIndex);
                DBG("  → Grid step " << gridStepIndex << " selected for editing");
                if (onSequenceTick) onSequenceTick();
            }
            break;
        }

        // ========== NAVIGATION ==========
        case MidiControlId::VIEW_TOGGLE: {
            if (isNoteOnOrCCPositive) {
                // This is purely a UI concern - would need UI callback
                DBG("  → View toggle (UI only)");
            }
            break;
        }

        case MidiControlId::SETTINGS_TOGGLE: {
            if (isNoteOnOrCCPositive) {
                // This is purely a UI concern - would need UI callback
                DBG("  → Settings toggle (UI only)");
            }
            break;
        }

        case MidiControlId::COUNT:
            // This is just a sentinel value, not an actual control
            break;

        default:
            // Unhandled control - log for debugging
            DBG("  → Unhandled MIDI control: " << getMidiControlName(mapping.controlId));
            break;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
