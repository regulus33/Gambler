#pragma once

#include "ClockMode.h"
#include "InternalClock.h"
#include <juce_core/juce_core.h>
#include <vector>

/**
 * Clock - Master timing mechanism for Gambler
 * 
 * The Clock is responsible for determining when the sequencer should advance
 * to the next step. It operates in one of two modes:
 * 
 * 1. DAW_SYNC: Synchronizes with the host DAW's tempo and playhead position
 * 2. INTERNAL: Uses an internal timer based on a user-specified BPM
 * 
 * This is a pure domain object with no JUCE dependencies. It receives timing
 * information from the application layer (PluginProcessor) and determines
 * whether a clock pulse should be generated.
 * 
 * The Clock delegates to InternalClock for internal timing logic.
 * DAW sync logic is handled by the DawSync utility in the application layer.
 */
class Clock {
public:
    /**
     * Default constructor
     * Initializes in INTERNAL mode with 120 BPM and 16th note subdivision
     */
    Clock()
        : mode(ClockMode::INTERNAL) // default to internal
        , subdivision(Subdivision::SIXTEENTH) // TODO: we can make this configurable in Settings
        , internalClock() // initialize our internal clock
        , lastDawPulseCount(0) {
        
        internalClock.setBpm(120.0); // default internal bpm
        internalClock.setSubdivision(subdivision); // default subdivision
    }
    
    /**
     * Sets the clock mode
     * 
     * @param newMode DAW_SYNC or INTERNAL
     */
    void setMode(ClockMode newMode) {
        mode = newMode;

        // Reset state when switching modes
        if (mode == ClockMode::INTERNAL) {
            internalClock.reset(); // reset samples
        } else if (mode == ClockMode::DAW_SYNC) {
            lastDawPulseCount = 0; // reset the pulse daw count.
        } else if (mode == ClockMode::EXTERNAL_MIDI_CLOCK) {
            externalMidiClockPulseCount = 0;
            externalClockRunning = false;
        }
    }
    
    /**
     * Just a getter for clock mode
     * 
     * @return An enum representing mode (DAW_SYNC or INTERNAL)
     */
    ClockMode getMode() const {
        return mode;
    }
    
    /**
     * Set the BPM for internal clock mode
     * 
     * @param bpm BPM value (20.0 - 999.0)
     */
    void setBpm(double bpm) {
        internalClock.setBpm(bpm);
    }
    
    /**
     * Get the current BPM (for internal mode)
     * 
     * @return Current BPM value
     */
    double getBpm() const {
        return internalClock.getBpm();
    }
    
    /**
     * Set the subdivision
     * 
     * @param newSubdivision The subdivision to use
     */
    void setSubdivision(Subdivision newSubdivision) {
        subdivision = newSubdivision; // set for non internal clock
        internalClock.setSubdivision(newSubdivision); // set for internal clock
    }
    
    /**
     * Get the current subdivision
     * 
     * @return Current subdivision
     */
    Subdivision getSubdivision() const {
        return subdivision;
    }
    
    /**
     * Set the sample rate (for internal clock)
     *
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(double sampleRate) {
        internalClock.setSampleRate(sampleRate);
    }

    /**
     * Calculate the number of samples per step based on current clock settings
     *
     * This method handles both INTERNAL and DAW_SYNC modes, providing a unified
     * way to calculate step duration in samples.
     *
     * Formula:
     * Pulses per second = (BPM × pulses_per_quarter_note) / 60
     * Samples per pulse = sample_rate / pulses_per_second
     *
     * @param dawBpm BPM from DAW (only used in DAW_SYNC mode, ignored in INTERNAL mode)
     * @param sampleRate Current audio sample rate
     * @return Number of samples per step, or 0.0 if calculation is not possible
     */
    double calculateSamplesPerStep(double dawBpm, double sampleRate) const {
        if (mode == ClockMode::INTERNAL || mode == ClockMode::EXTERNAL_MIDI_CLOCK) {
            // Use internal clock's calculation
            // For external MIDI clock, use internal BPM as approximation for note length
            return internalClock.calculateSamplesPerPulse();
        } else {
            // DAW_SYNC mode: calculate based on DAW tempo
            if (dawBpm <= 0.0 || sampleRate <= 0.0) {
                return 0.0;  // Invalid parameters
            }
            int pulsesPerQuarterNote = getPulsesPerQuarterNote(subdivision);
            double pulsesPerSecond = (dawBpm * pulsesPerQuarterNote) / 60.0;
            return sampleRate / pulsesPerSecond;
        }
    }

    /**
     * Start the clock
     */
    void start() {
        if (mode == ClockMode::INTERNAL) {
            internalClock.start();
        }
        // In DAW_SYNC mode, the DAW controls start/stop
        // In EXTERNAL_MIDI_CLOCK mode, start/stop is controlled by MIDI Start/Stop messages
    }
    
    /**
     * Stop the clock
     */
    void stop() {
        if (mode == ClockMode::INTERNAL) {
            internalClock.stop();
        }
        if (mode == ClockMode::EXTERNAL_MIDI_CLOCK) {
            externalClockRunning = false;
        }
        // In DAW_SYNC mode, the DAW controls start/stop
    }
    
    /**
     * Check if the clock is running
     * 
     * @return true if running, false otherwise
     */
    bool isRunning() const {
        if (mode == ClockMode::INTERNAL) {
            return internalClock.getIsRunning();
        }
        if (mode == ClockMode::EXTERNAL_MIDI_CLOCK) {
            return externalClockRunning;
        }
        // In DAW_SYNC mode, running state comes from DAW
        return false;  // Will be overridden by updateDawSync
    }
    
    /**
     * Reset the clock state
     */
    void reset() {
        internalClock.reset();
        lastDawPulseCount = 0;
        externalMidiClockPulseCount = 0;
        pendingPulseOffsets.clear();
    }

    /**
     * Process samples for internal clock mode.
     * Pushes the sub-block sample offset of every pulse boundary crossed in
     * this block onto the pending-pulse queue. May enqueue 0..N offsets.
     */
    void processInternalClock(int numSamples) {
        if (mode != ClockMode::INTERNAL) return;
        auto offsets = internalClock.processSamples(numSamples);
        pendingPulseOffsets.insert(pendingPulseOffsets.end(),
                                    offsets.begin(), offsets.end());
    }

    /**
     * Enqueue a list of pre-computed DAW pulse offsets.
     * @param currentPulseCount  Cumulative pulse count from DAW (for edge tracking).
     * @param newPulseOffsets    Sub-block sample offsets, one per pulse that fired this block.
     */
    void enqueueDawPulses(int currentPulseCount, const std::vector<int>& newPulseOffsets) {
        if (mode != ClockMode::DAW_SYNC) return;

        // Detect a backwards jump (DAW looped / restarted). DawSync also re-syncs
        // its own state on jumps, so we just realign here.
        if (currentPulseCount < lastDawPulseCount)
            lastDawPulseCount = currentPulseCount;

        for (int offset : newPulseOffsets)
            pendingPulseOffsets.push_back(offset);

        if (currentPulseCount > lastDawPulseCount)
            lastDawPulseCount = currentPulseCount;
    }

    /**
     * Drain the pending-pulse queue. Each int is a sub-block sample offset
     * (in [0, blockSize)) where a pulse fired this block. The queue is cleared.
     */
    std::vector<int> drainPulses() {
        std::vector<int> result;
        std::swap(result, pendingPulseOffsets);
        return result;
    }

    // ========================================
    // External MIDI Clock support
    // ========================================

    /**
     * Process an incoming external MIDI clock pulse (0xF8).
     * @param sampleOffsetInBlock  The sample offset of the 0xF8 message within
     *        the current audio block (from the MIDI buffer metadata).
     */
    void processExternalMidiClockPulse(int sampleOffsetInBlock = 0) {
        if (mode != ClockMode::EXTERNAL_MIDI_CLOCK) return;

        externalMidiClockPulseCount++;
        int pulsesPerTick = getPulsesPerMidiClock(subdivision);

        if (externalMidiClockPulseCount >= pulsesPerTick) {
            pendingPulseOffsets.push_back(sampleOffsetInBlock);
            externalMidiClockPulseCount -= pulsesPerTick;  // Carry over remainder
        }
    }

    /**
     * Handle MIDI Start message (0xFA) for external clock
     */
    void handleExternalMidiStart() {
        if (mode != ClockMode::EXTERNAL_MIDI_CLOCK) return;
        externalClockRunning = true;
        externalMidiClockPulseCount = 0;
    }

    /**
     * Handle MIDI Stop message (0xFC) for external clock
     */
    void handleExternalMidiStop() {
        if (mode != ClockMode::EXTERNAL_MIDI_CLOCK) return;
        externalClockRunning = false;
        externalMidiClockPulseCount = 0;
    }

    /**
     * Handle MIDI Continue message (0xFB) for external clock
     */
    void handleExternalMidiContinue() {
        if (mode != ClockMode::EXTERNAL_MIDI_CLOCK) return;
        externalClockRunning = true;
        // Don't reset pulse count on continue - resume from where we left off
    }

    /**
     * Check if external MIDI clock is running
     */
    bool isExternalClockRunning() const {
        return externalClockRunning;
    }

    /**
     * Get the number of MIDI clock pulses to send this cycle
     * MIDI clock runs at 24 PPQN (pulses per quarter note)
     * For 16th note subdivision: 24 / 4 = 6 MIDI clocks per tick
     *
     * @return Number of MIDI clock messages to send (6 per tick for 16th notes)
     */
    int getMidiClockPulsesToSend() {
        if (mode != ClockMode::INTERNAL) {
            return 0;  // Only send MIDI clock in internal mode
        }

        // Return everything accumulated — capping at one tick's worth while
        // zeroing the accumulator silently dropped pulses whenever two
        // sequencer ticks landed in the same audio block, drifting external
        // gear behind by a tick each time.
        int pulsesToSend = midiClockPulsesAccumulated;
        midiClockPulsesAccumulated = 0;  // Reset after reading
        return pulsesToSend;
    }

    /**
     * Accumulate MIDI clock pulses (called when shouldTick returns true)
     */
    void accumulateMidiClockPulse() {
        midiClockPulsesAccumulated += getPulsesPerMidiClock(subdivision);
    }
    
    /**
     * Manually trigger a clock pulse at the start of the next block.
     * Used for manual stepping (e.g. keyboard "C" key).
     */
    void manualTrigger() {
        pendingPulseOffsets.push_back(0);
    }
    
    /**
     * Get the internal clock (for testing)
     *
     * @return Reference to the internal clock
     */
    const InternalClock& getInternalClock() const {
        return internalClock;
    }

    /**
     * Serialize clock state to XML
     * Saves mode, BPM, and subdivision (not transient state like isRunning)
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("Clock");

        // Save clock mode
        juce::String modeStr = "daw_sync";
        if (mode == ClockMode::INTERNAL) modeStr = "internal";
        else if (mode == ClockMode::EXTERNAL_MIDI_CLOCK) modeStr = "external_midi_clock";
        xml->setAttribute("mode", modeStr);

        // Save BPM (for internal mode)
        xml->setAttribute("bpm", getBpm());

        // Save subdivision
        int subdivisionValue = 0;
        switch (subdivision) {
            case Subdivision::QUARTER:        subdivisionValue = 0; break;
            case Subdivision::EIGHTH:         subdivisionValue = 1; break;
            case Subdivision::SIXTEENTH:      subdivisionValue = 2; break;
            case Subdivision::THIRTY_SECOND:  subdivisionValue = 3; break;
        }
        xml->setAttribute("subdivision", subdivisionValue);

        return xml;
    }

    /**
     * Deserialize clock state from XML
     * Restores mode, BPM, and subdivision
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "Clock") return;

        // Restore clock mode
        juce::String modeStr = xml.getStringAttribute("mode", "daw_sync");
        ClockMode restoredMode = ClockMode::DAW_SYNC;
        if (modeStr == "internal") restoredMode = ClockMode::INTERNAL;
        else if (modeStr == "external_midi_clock") restoredMode = ClockMode::EXTERNAL_MIDI_CLOCK;
        setMode(restoredMode);

        // Restore BPM
        double restoredBpm = xml.getDoubleAttribute("bpm", 120.0);
        setBpm(restoredBpm);

        // Restore subdivision
        int subdivisionValue = xml.getIntAttribute("subdivision", 2); // Default to SIXTEENTH
        Subdivision restoredSubdivision = Subdivision::SIXTEENTH;
        switch (subdivisionValue) {
            case 0: restoredSubdivision = Subdivision::QUARTER; break;
            case 1: restoredSubdivision = Subdivision::EIGHTH; break;
            case 2: restoredSubdivision = Subdivision::SIXTEENTH; break;
            case 3: restoredSubdivision = Subdivision::THIRTY_SECOND; break;
        }
        setSubdivision(restoredSubdivision);
    }

private:
    ClockMode mode;                // Current clock mode
    Subdivision subdivision;       // Note subdivision
    InternalClock internalClock;   // Internal timing engine
    int lastDawPulseCount;         // Last pulse count from DAW (for edge detection)
    std::vector<int> pendingPulseOffsets;  // Sub-block sample offsets where pulses fired this block
    int midiClockPulsesAccumulated = 0;  // Accumulated MIDI clock pulses to send

    // External MIDI clock state
    int externalMidiClockPulseCount = 0;  // Accumulated external MIDI clock pulses (24 PPQN)
    bool externalClockRunning = false;     // Whether external clock is running (received Start)

    /**
     * Get the number of MIDI clock pulses per sequencer tick
     * MIDI clock is 24 PPQN (pulses per quarter note)
     *
     * Quarter note: 24 pulses
     * Eighth note: 12 pulses
     * Sixteenth note: 6 pulses
     * Thirty-second note: 3 pulses
     */
    static int getPulsesPerMidiClock(Subdivision sub) {
        switch (sub) {
            case Subdivision::QUARTER:        return 24;
            case Subdivision::EIGHTH:         return 12;
            case Subdivision::SIXTEENTH:      return 6;
            case Subdivision::THIRTY_SECOND:  return 3;
            default:                          return 6;
        }
    }
};

