#pragma once

#include "ClockMode.h"
#include "InternalClock.h"
#include <juce_core/juce_core.h>

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
        , lastDawPulseCount(0) // TODO: what is this?
        , shouldTriggerPulse(false) {
        
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
        } else {
            lastDawPulseCount = 0; // reset the pulse daw count.
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
        if (mode == ClockMode::INTERNAL) {
            // Use internal clock's calculation
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
    }
    
    /**
     * Stop the clock
     */
    void stop() {
        if (mode == ClockMode::INTERNAL) {
            internalClock.stop();
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
        // In DAW_SYNC mode, running state comes from DAW
        return false;  // Will be overridden by updateDawSync
    }
    
    /**
     * Reset the clock state
     */
    void reset() {
        internalClock.reset();
        lastDawPulseCount = 0;
        shouldTriggerPulse = false;
    }
    
    /**
     * Process samples for internal clock mode
     * 
     * This should be called from the audio callback when in INTERNAL mode.
     * 
     * @param numSamples Number of samples in the current audio buffer
     */
    void processInternalClock(int numSamples) {
        if (mode == ClockMode::INTERNAL) {
            shouldTriggerPulse = internalClock.processSamples(numSamples);
        }
    }
    
    /**
     * Update DAW sync state
     *
     * This should be called from the audio callback when in DAW_SYNC mode.
     * The pulse count should be calculated by the DawSync utility based on
     * the AudioPlayHead position.
     * The algorithm is based on ppqn which we get from playHead, we are getting a double back
     * and every frame (call of processblock) that number increments by just a hair (remember doubles are higher precision than floats)
     * so we continuously in every prcess block multiply the ppqPosition by 4 (or whatever the subdivision is) and
     * that's our number, then whenever that number reaches a whole number, the pulse ticks and we add the count.
     *
     * @param currentPulseCount Current pulse count from DAW position
     */
    void updateDawSync(int currentPulseCount) {
        if (mode == ClockMode::DAW_SYNC) {
            // Detect if pulse count went backwards (DAW restarted or looped)
            // otherwise stopping daw makes seq stop and won't start back.
            if (currentPulseCount < lastDawPulseCount) {
                // Reset to allow pulses to trigger from the new position
                lastDawPulseCount = currentPulseCount - 1;
            }

            // Trigger a pulse if the count has incremented
            if (currentPulseCount > lastDawPulseCount) {
                shouldTriggerPulse = true;
                lastDawPulseCount = currentPulseCount;
            } else {
                shouldTriggerPulse = false;
            }
        }
    }
    
    /**
     * Check if a clock pulse should be triggered
     *
     * This is the main method that the sequencer checks to determine
     * if it should advance to the next step.
     *
     * @return true if a pulse should be triggered, false otherwise
     */
    bool shouldTick() {
        bool result = shouldTriggerPulse;
        shouldTriggerPulse = false;  // Clear the flag after reading
        return result;
    }

    /**
     * Get the number of MIDI clock pulses to send this cycle
     * MIDI clock runs at 24 PPQN (pulses per quarter note)
     * For 16th note subdivision: 24 / 4 = 6 MIDI clocks per tick
     *
     * @return Number of MIDI clock messages to send (0-6 for 16th notes)
     */
    int getMidiClockPulsesToSend() {
        if (mode != ClockMode::INTERNAL) {
            return 0;  // Only send MIDI clock in internal mode
        }

        int pulsesPerTick = getPulsesPerMidiClock(subdivision);
        int pulsesToSend = midiClockPulsesAccumulated;
        midiClockPulsesAccumulated = 0;  // Reset after reading
        return pulsesToSend > pulsesPerTick ? pulsesPerTick : pulsesToSend;
    }

    /**
     * Accumulate MIDI clock pulses (called when shouldTick returns true)
     */
    void accumulateMidiClockPulse() {
        midiClockPulsesAccumulated += getPulsesPerMidiClock(subdivision);
    }
    
    /**
     * Manually trigger a clock pulse
     * 
     * This is used for manual stepping (e.g., keyboard "C" key)
     */
    void manualTrigger() {
        shouldTriggerPulse = true;
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
        xml->setAttribute("mode", mode == ClockMode::INTERNAL ? "internal" : "daw_sync");

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
        ClockMode restoredMode = (modeStr == "internal") ? ClockMode::INTERNAL : ClockMode::DAW_SYNC;
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
    bool shouldTriggerPulse;       // Flag indicating a pulse should be triggered
    int midiClockPulsesAccumulated = 0;  // Accumulated MIDI clock pulses to send

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

