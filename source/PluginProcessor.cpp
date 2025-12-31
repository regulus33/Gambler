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
    // Register as listener for all parameter changes
    // This ensures State stays in sync when parameters change from UI
    for (auto* param : apvts.processor.getParameters()) {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*> (param)) {
            apvts.addParameterListener (paramWithID->paramID, this);
        }
    }

    // Initialize clock settings
    // Default to internal clock for standalone, DAW sync for plugin
    // Note: We use runtime detection because the shared code is compiled with
    // all plugin format flags enabled, so preprocessor checks don't work here.
    // or at least in development.
    // it just runs on startup so who cares.
    juce::PluginHostType hostType;
    if (hostType.type == juce::PluginHostType::UnknownHost) {
        // Standalone app - use internal clock
        clock.setMode(ClockMode::INTERNAL);
        clock.start();
    } else {
        // Running in a DAW - use DAW sync
        clock.setMode(ClockMode::DAW_SYNC);
    }
    clock.setBpm(120.0);
    clock.setSubdivision(Subdivision::SIXTEENTH);

    // Random mode is off by default (enabled when Roll button is held)
    sequence.setIsRandom(false);

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

        // Tick the sequence to get the next note
        int note = sequence.tick();

        DBG("Clock tick! Step: " << sequence.getCurrentIndex() << ", Note: " << note);

        // Get current step to record rest if roll is engaged
        auto& currentStep = sequence.getCurrentStep();

        if (note >= 0) {
            // Get step modulations for this step
            const auto& stepMods = currentStep.getStepModulations();
            auto selectedMods = stepMods.getSelectedModulations();

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
                // Calculate note-off time based on note length percentage
                double noteLengthPercent = userPreferences.defaultNoteLength;
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

            // If roll is engaged, record this step as a rest (note number = -1)
            if (sequence.getIsRandom()) {
                int idx = currentStep.getStepNotes().getCurrentIndex();
                if (idx == -1) idx = 0;
                currentStep.getStepNotes().setNote(idx, -1);
                DBG("Rest recorded at step: " << sequence.getCurrentIndex());
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

    // Save APVTS parameters (UI control states)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement(apvtsXml.release());

    // Save Sequence data (all 64 steps with notes, velocities, gates, modulations, etc.)
    rootXml->addChildElement(sequence.toXml().release());

    // Save Clock settings (mode, BPM, subdivision)
    rootXml->addChildElement(clock.toXml().release());

    // Save User Preferences
    rootXml->addChildElement(userPreferences.toXml().release());

    // Convert to binary and save
    copyXmlToBinary(*rootXml, destData);

    DBG("State saved: Sequence, Clock, and UserPreferences");
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // Load XML from binary data
    std::unique_ptr<juce::XmlElement> rootXml (getXmlFromBinary (data, sizeInBytes));

    if (rootXml != nullptr) {
        // Check if this is the new format (GamblerState root element)
        if (rootXml->hasTagName("GamblerState")) {
            DBG("Loading state: GamblerState format");

            // Load APVTS parameters
            if (auto* apvtsXml = rootXml->getChildByName(apvts.state.getType())) {
                apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));
                DBG("  - APVTS loaded");
            }

            // Load Sequence data
            if (auto* seqXml = rootXml->getChildByName("Sequence")) {
                sequence.fromXml(*seqXml);
                DBG("  - Sequence loaded");
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
    juce::ignoreUnused(parameterID, newValue);
    // APVTS parameters are used for UI state only
    // Sequence data is persisted directly via toXml/fromXml
}

//==============================================================================
// UI Preview Methods
//==============================================================================

void PluginProcessor::triggerStepMidi(int stepIndex) {
    if (stepIndex < 0 || stepIndex >= 64) return;

    // Queue command to set the selected step for editing (thread-safe)
    queueCommand(ProcessorCommand([this, stepIndex]() {
        sequence.setSelectedStepForEditing(stepIndex);
    }));

    // Get the note for this step
    const auto& step = sequence.getStep(stepIndex);
    int note = step.getStepNotes().getCurrentNote().getNoteNumber();

    // Get step modulations
    const auto& stepMods = step.getStepModulations();
    auto selectedMods = stepMods.getSelectedModulations();

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

    int selectedStep = sequence.getSelectedStepForEditing();
    if (selectedStep < 0 || selectedStep >= 64) {
        DBG("  ERROR: Invalid selected step: " << selectedStep);
        return;
    }

    // Get step modulations
    const auto& step = sequence.getStep(selectedStep);
    const auto& stepMods = step.getStepModulations();
    auto selectedMods = stepMods.getSelectedModulations();

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
        note = step.getStepNotes().getCurrentNote().getNoteNumber();
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

    // Save APVTS parameters (UI control states)
    auto apvtsXml = apvts.copyState().createXml();
    rootXml->addChildElement(apvtsXml.release());

    // Save Sequence data (all 64 steps with notes, velocities, gates, modulations, etc.)
    rootXml->addChildElement(sequence.toXml().release());

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

    // Load APVTS parameters
    if (auto* apvtsXml = xml.getChildByName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));
        DBG("  - APVTS loaded");
    }

    // Queue command to load domain state on audio thread
    // We need to copy the XML data since the original may be destroyed
    auto xmlCopy = std::make_shared<juce::XmlElement>(xml);

    queueCommand(ProcessorCommand(
        [this, xmlCopy]() {
            // Load Sequence data
            if (auto* seqXml = xmlCopy->getChildByName("Sequence")) {
                sequence.fromXml(*seqXml);
                DBG("  - Sequence loaded");
            }

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
                        onMidiLearned();
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
                        onMidiLearned();
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
    // Convert MIDI value using the mapping's conversion settings
    float normalizedValue = MidiValueConverter::midiToScaled(
        midiValue, mapping.minValue, mapping.maxValue, mapping.invert);

    // Handle each control type
    switch (mapping.controlId) {
        // ========== OCTAVE SLIDER ==========
        case MidiControlId::OCTAVE_SLIDER: {
            int octaveIndex = MidiValueConverter::midiToDiscreteIndex(midiValue, 9);  // 0-9
            queueCommand(ProcessorCommand(
                [this, octaveIndex]() {
                    auto& seq = getSequence();
                    int selectedStep = seq.getSelectedStepForEditing();
                    int currentNote = seq.getStep(selectedStep).getStepNotes().getCurrentNote().getNoteNumber();
                    int pitchClass = currentNote % 12;
                    int newNote = octaveIndex * 12 + pitchClass;
                    seq.getStep(selectedStep).updateCurrentNoteNumber(newNote);
                    DBG("  → Octave changed to " << octaveIndex << " (MIDI note " << newNote << ")");
                },
                [this]() {
                    // Trigger UI refresh
                    if (onSequenceTick) onSequenceTick();
                }
            ));
            break;
        }

        // ========== NOTE SLIDER ==========
        case MidiControlId::NOTE_SLIDER: {
            // Note slider is filtered to scale notes, so we need to map MIDI value to scale index
            // For now, use a simple linear mapping (TODO: respect current scale)
            int pitchClass = MidiValueConverter::midiToDiscreteIndex(midiValue, 11);  // 0-11
            queueCommand(ProcessorCommand(
                [this, pitchClass]() {
                    auto& seq = getSequence();
                    int selectedStep = seq.getSelectedStepForEditing();
                    int currentNote = seq.getStep(selectedStep).getStepNotes().getCurrentNote().getNoteNumber();
                    int octave = currentNote / 12;
                    int newNote = octave * 12 + pitchClass;
                    seq.getStep(selectedStep).updateCurrentNoteNumber(newNote);
                    DBG("  → Note changed to pitch class " << pitchClass << " (MIDI note " << newNote << ")");
                },
                [this]() {
                    // Trigger UI refresh
                    if (onSequenceTick) onSequenceTick();
                }
            ));
            break;
        }

        // ========== PROBABILITY SLIDER ==========
        case MidiControlId::PROBABILITY_SLIDER: {
            int percentage = static_cast<int>(normalizedValue * 100.0f);
            queueCommand(ProcessorCommand(
                [this, percentage]() {
                    getSequence().setProbabilityPercentage(percentage);
                    DBG("  → Probability changed to " << percentage << "%");
                },
                [this]() {
                    // Trigger UI refresh
                    if (onSequenceTick) onSequenceTick();
                }
            ));
            break;
        }

        // ========== ROLL BUTTON (Momentary) ==========
        case MidiControlId::BUTTON_ROLL: {
            queueCommand(ProcessorCommand([this, isNoteOnOrCCPositive]() {
                getSequence().setIsRandom(isNoteOnOrCCPositive);
                DBG("  → Roll " << (isNoteOnOrCCPositive ? "ON" : "OFF"));
            }));
            break;
        }

        // ========== SEQUENCER CONTROL BUTTONS ==========
        case MidiControlId::BUTTON_PAGE_LEFT_SHIFT_DUPLICATE_SEQUENCE: {
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Duplicate sequence
                            auto& seq = getSequence();
                            int currentLength = seq.getLength();
                            int newLength = juce::jmin(currentLength * 2, Sequence::MAX_STEPS);
                            int stepsToCopy = newLength - currentLength;

                            for (int i = 0; i < stepsToCopy; i++) {
                                int srcIdx = i;
                                int dstIdx = currentLength + i;
                                auto& srcStep = seq.getStep(srcIdx);
                                auto& dstStep = seq.getStep(dstIdx);
                                auto& srcNotes = srcStep.getStepNotes();
                                auto& dstNotes = dstStep.getStepNotes();
                                dstNotes.setLength(srcNotes.getLength());
                                for (int n = 0; n < StepNotes::MAX_LENGTH; n++) {
                                    dstNotes.setNote(n, srcNotes.getNote(n).getNoteNumber());
                                }
                            }
                            seq.setLength(newLength);
                            DBG("  → Duplicated sequence to " << newLength << " steps");
                        } else {
                            // Normal: Page left (handled by UI)
                            DBG("  → Page left (UI only)");
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        case MidiControlId::BUTTON_LENGTH_PLUS_SHIFT_CLEAR_STEP: {
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Clear current step
                            auto& seq = getSequence();
                            int selectedStep = seq.getSelectedStepForEditing();
                            auto& stepNotes = seq.getStep(selectedStep).getStepNotes();
                            for (int n = 0; n < StepNotes::MAX_LENGTH; n++) {
                                stepNotes.setNote(n, -1);
                            }
                            DBG("  → Cleared step " << selectedStep);
                        } else {
                            // Normal: Increase length
                            getSequence().increaseLength();
                            DBG("  → Length increased to " << getSequence().getLength());
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        case MidiControlId::BUTTON_LENGTH_MINUS_SHIFT_RANDOMIZE_STEP: {
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Randomize current step
                            auto& seq = getSequence();
                            int selectedStep = seq.getSelectedStepForEditing();
                            seq.getStep(selectedStep).randomizeNote(seq.getScale());
                            DBG("  → Randomized step " << selectedStep);
                        } else {
                            // Normal: Decrease length
                            getSequence().decreaseLength();
                            DBG("  → Length decreased to " << getSequence().getLength());
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        case MidiControlId::BUTTON_SHIFT: {
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                queueCommand(ProcessorCommand(
                    [this]() {
                        auto& seq = getSequence();
                        bool newShiftState = !seq.getIsShift();
                        seq.setIsShift(newShiftState);
                        DBG("  → Shift " << (newShiftState ? "ON" : "OFF"));
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
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
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                // Calculate step index within current window
                int localStepIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::STEP_0);
                queueCommand(ProcessorCommand(
                    [this, localStepIndex]() {
                        // For now, trigger step 0-7 directly (TODO: respect window position)
                        DBG("  → Step " << localStepIndex << " clicked");
                        // This would trigger MIDI preview in the full implementation
                    }
                ));
            }
            break;
        }

        // ========== SUBSEQUENCE CONTROLS ==========
        case MidiControlId::SUBSEQ_MINUS: {
            if (isNoteOnOrCCPositive) {
                queueCommand(ProcessorCommand(
                    [this]() {
                        auto& seq = getSequence();
                        int selectedStep = seq.getSelectedStepForEditing();
                        auto& stepNotes = seq.getStep(selectedStep).getStepNotes();
                        stepNotes.decreaseLength();
                        DBG("  → Substep length decreased to " << stepNotes.getLength());
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        case MidiControlId::SUBSEQ_PLUS: {
            if (isNoteOnOrCCPositive) {
                queueCommand(ProcessorCommand(
                    [this]() {
                        auto& seq = getSequence();
                        int selectedStep = seq.getSelectedStepForEditing();
                        auto& stepNotes = seq.getStep(selectedStep).getStepNotes();
                        stepNotes.increaseLength();
                        DBG("  → Substep length increased to " << stepNotes.getLength());
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
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
                queueCommand(ProcessorCommand(
                    [this, barIndex]() {
                        auto& seq = getSequence();
                        int selectedStep = seq.getSelectedStepForEditing();
                        auto& stepNotes = seq.getStep(selectedStep).getStepNotes();
                        if (barIndex < stepNotes.getLength()) {
                            stepNotes.setCurrentIndex(barIndex);
                            DBG("  → Substep " << barIndex << " selected");
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        // ========== NOTE RANGE CONTROLS ==========
        case MidiControlId::NOTE_RANGE_MIN: {
            int minNote = MidiValueConverter::midiToDiscreteIndex(midiValue, 127);
            queueCommand(ProcessorCommand(
                [this, minNote]() {
                    getSequence().getScale().setRangeMin(minNote);
                    DBG("  → Note range min set to " << minNote);
                },
                [this]() {
                    if (onSequenceTick) onSequenceTick();
                }
            ));
            break;
        }

        case MidiControlId::NOTE_RANGE_MAX: {
            int maxNote = MidiValueConverter::midiToDiscreteIndex(midiValue, 127);
            queueCommand(ProcessorCommand(
                [this, maxNote]() {
                    getSequence().getScale().setRangeMax(maxNote);
                    DBG("  → Note range max set to " << maxNote);
                },
                [this]() {
                    if (onSequenceTick) onSequenceTick();
                }
            ));
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
            queueCommand(ProcessorCommand(
                [this, laneIndex, ccValue]() {
                    auto& seq = getSequence();
                    int selectedStep = seq.getSelectedStepForEditing();
                    auto& mods = seq.getStep(selectedStep).getStepModulations();
                    // Only set value if lane index is within bounds
                    if (laneIndex >= 0 && laneIndex < 7) {
                        auto& mod = mods.getModulation(laneIndex);
                        mod.setValue(ccValue);
                        DBG("  → CC Lane " << laneIndex << " value set to " << ccValue);
                    }
                },
                [this]() {
                    if (onSequenceTick) onSequenceTick();
                }
            ));
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
            if (isNoteOnOrCCPositive) {  // Only trigger on button press
                int noteIndex = static_cast<int>(mapping.controlId) - static_cast<int>(MidiControlId::SCALE_NOTE_C);
                queueCommand(ProcessorCommand(
                    [this, noteIndex]() {
                        auto& scale = getSequence().getScale();
                        // Toggle the scale note
                        bool currentState = scale.getScaleNotes()[noteIndex];

                        // Only toggle if there's more than one note in the scale
                        int activeNotes = 0;
                        for (int i = 0; i < 12; i++) {
                            if (scale.getScaleNotes()[i]) activeNotes++;
                        }

                        if (activeNotes > 1 || !currentState) {
                            scale.setScaleNote(noteIndex, !currentState);
                            // Quantize all notes to the new scale
                            getSequence().quantizeAllNotesToScale();
                            DBG("  → Scale note " << noteIndex << " toggled to " << (!currentState ? "ON" : "OFF"));
                        } else {
                            DBG("  → Cannot disable last scale note");
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        // ========== REMAINING SEQUENCER CONTROL BUTTONS ==========
        case MidiControlId::BUTTON_PAGE_RIGHT_SHIFT_CLEAR_WINDOW: {
            if (isNoteOnOrCCPositive) {
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Clear window (handled by UI - would need window position)
                            DBG("  → Clear window (UI only)");
                        } else {
                            // Normal: Page right (handled by UI)
                            DBG("  → Page right (UI only)");
                        }
                    }
                ));
            }
            break;
        }

        case MidiControlId::BUTTON_NUDGE_LEFT_SHIFT_RANDOMIZE_ALL: {
            if (isNoteOnOrCCPositive) {
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Randomize all steps
                            auto& seq = getSequence();
                            for (int i = 0; i < Sequence::MAX_STEPS; i++) {
                                seq.getStep(i).randomizeNote(seq.getScale());
                            }
                            DBG("  → Randomized all steps");
                        } else {
                            // Normal: Nudge left
                            getSequence().nudgeLeft();
                            DBG("  → Nudged left");
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
            }
            break;
        }

        case MidiControlId::BUTTON_NUDGE_RIGHT_SHIFT_SHUFFLE: {
            if (isNoteOnOrCCPositive) {
                queueCommand(ProcessorCommand(
                    [this]() {
                        bool isShifted = getSequence().getIsShift();
                        if (isShifted) {
                            // Shifted: Shuffle all steps
                            auto& seq = getSequence();
                            int len = seq.getLength();
                            // Fisher-Yates shuffle
                            static std::random_device rd;
                            static std::mt19937 gen(rd());
                            for (int i = len - 1; i > 0; i--) {
                                std::uniform_int_distribution<> dis(0, i);
                                int j = dis(gen);
                                // Swap steps i and j
                                auto& stepI = seq.getStep(i).getStepNotes();
                                auto& stepJ = seq.getStep(j).getStepNotes();
                                // Swap lengths
                                int lenI = stepI.getLength();
                                int lenJ = stepJ.getLength();
                                stepI.setLength(lenJ);
                                stepJ.setLength(lenI);
                                // Swap all notes
                                for (int n = 0; n < StepNotes::MAX_LENGTH; n++) {
                                    int noteI = stepI.getNote(n).getNoteNumber();
                                    int noteJ = stepJ.getNote(n).getNoteNumber();
                                    stepI.setNote(n, noteJ);
                                    stepJ.setNote(n, noteI);
                                }
                            }
                            DBG("  → Shuffled all steps");
                        } else {
                            // Normal: Nudge right
                            getSequence().nudgeRight();
                            DBG("  → Nudged right");
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
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
                queueCommand(ProcessorCommand(
                    [this, laneIndex]() {
                        auto& seq = getSequence();
                        int selectedStep = seq.getSelectedStepForEditing();
                        auto& mods = seq.getStep(selectedStep).getStepModulations();
                        if (laneIndex >= 0 && laneIndex < 7) {
                            auto& mod = mods.getModulation(laneIndex);
                            // Toggle selection
                            mod.setSelected(!mod.isSelected());
                            DBG("  → CC Lane " << laneIndex << " toggled " << (mod.isSelected() ? "ON" : "OFF"));
                        }
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
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
                queueCommand(ProcessorCommand(
                    [this, gridStepIndex]() {
                        // Set this as the selected step for editing
                        getSequence().setSelectedStepForEditing(gridStepIndex);
                        DBG("  → Grid step " << gridStepIndex << " selected for editing");
                        // Note: Window navigation would be handled by UI in full implementation
                    },
                    [this]() {
                        if (onSequenceTick) onSequenceTick();
                    }
                ));
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
