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
        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOn = juce::MidiMessage::noteOn(1, note, (juce::uint8)100);
            midiOutput->sendMessageNow(noteOn);
            DBG("Preview MIDI Note ON (standalone): " << note << " (step " << stepIndex << ")");
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { note, true, 100 };
                previewMidiFifo.finishedWrite(1);
                DBG("Preview MIDI Note ON queued (DAW): " << note << " (step " << stepIndex << ")");
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

void PluginProcessor::triggerCurrentStepMidi() {
    DBG("=== triggerCurrentStepMidi() called ===");
    DBG("  midiOutput is " << (midiOutput ? "VALID" : "NULL"));

    int currentStep = sequence.getCurrentIndex();
    if (currentStep < 0 || currentStep >= 64) {
        DBG("  ERROR: Invalid current step: " << currentStep);
        return;
    }

    // Get the note for the current step
    const auto& step = sequence.getStep(currentStep);
    int note = step.getStepNotes().getCurrentNote().getNoteNumber();
    DBG("  Current step: " << currentStep << ", Note: " << note);

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
        if (midiOutput) {
            // Standalone mode: send directly to MIDI device
            auto noteOn = juce::MidiMessage::noteOn(1, note, (juce::uint8)100);
            midiOutput->sendMessageNow(noteOn);
            DBG("  Preview MIDI Note ON sent (standalone): " << note);
        } else {
            // DAW mode: queue for audio thread
            int start1, size1, start2, size2;
            previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) {
                previewMidiBuffer[start1] = { note, true, 100 };
                previewMidiFifo.finishedWrite(1);
                DBG("  Preview MIDI Note ON queued (DAW): " << note);
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
