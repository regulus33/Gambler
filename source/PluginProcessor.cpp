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

    // Queue command to set the sequence's current index (thread-safe)
    queueCommand(ProcessorCommand([this, stepIndex]() {
        sequence.setCurrentIndex(stepIndex);
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

    int currentStep = sequence.getCurrentIndex();
    if (currentStep < 0 || currentStep >= 64) {
        DBG("  ERROR: Invalid current step: " << currentStep);
        return;
    }

    // Get step modulations
    const auto& step = sequence.getStep(currentStep);
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
        DBG("  Current step: " << currentStep << ", Note from step: " << note);
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
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
