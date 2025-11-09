#pragma once

#include "ClockMode.h"
#include "InternalClock.h"

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
        : mode(ClockMode::INTERNAL)
        , subdivision(Subdivision::SIXTEENTH)
        , internalClock()
        , lastDawPulseCount(0)
        , shouldTriggerPulse(false) {
        
        internalClock.setBpm(120.0);
        internalClock.setSubdivision(subdivision);
    }
    
    /**
     * Set the clock mode
     * 
     * @param newMode DAW_SYNC or INTERNAL
     */
    void setMode(ClockMode newMode) {
        mode = newMode;
        
        // Reset state when switching modes
        if (mode == ClockMode::INTERNAL) {
            internalClock.reset();
        } else {
            lastDawPulseCount = 0;
        }
    }
    
    /**
     * Get the current clock mode
     * 
     * @return Current mode (DAW_SYNC or INTERNAL)
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
        subdivision = newSubdivision;
        internalClock.setSubdivision(newSubdivision);
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
     * 
     * @param currentPulseCount Current pulse count from DAW position
     */
    void updateDawSync(int currentPulseCount) {
        if (mode == ClockMode::DAW_SYNC) {
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
    
private:
    ClockMode mode;                // Current clock mode
    Subdivision subdivision;       // Note subdivision
    InternalClock internalClock;   // Internal timing engine
    int lastDawPulseCount;         // Last pulse count from DAW (for edge detection)
    bool shouldTriggerPulse;       // Flag indicating a pulse should be triggered
};

