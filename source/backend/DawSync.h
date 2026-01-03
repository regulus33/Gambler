#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ClockMode.h"

/**
 *
 * 
 * This class handles the DAW synchronization logic by interfacing with
 * JUCE's AudioPlayHead. It is kept separate from the Clock class to
 * maintain clean separation of concerns.
 * 
 * DawSync is NOT a pure domain object - it contains JUCE-specific code.
 * 
 * The DawSync utility:
 * 1. Queries the AudioPlayHead for current position (in PPQ)
 * 2. Converts the PPQ position to pulse count based on subdivision
 * 3. Detects when the pulse count increments
 * 4. Handles transport start/stop, tempo changes, and playhead jumps
 */
class DawSync {
public:
    /**
     * Default constructor
     */
    DawSync()
        : subdivision(Subdivision::SIXTEENTH) // initial ppqn
        , lastPpqPosition(0.0) // this is basically a counter
        , currentPulseCount(0) // this is the rounded truncated version of the above counter
        , isPlaying(false) {
    }
    
    /**
     * Set the subdivision
     * 
     * @param newSubdivision The subdivision to use
     */
    void setSubdivision(Subdivision newSubdivision) {
        subdivision = newSubdivision;
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
     * This is called every frame of processBlock()
     *
     * @param position Optional position info from AudioPlayHead
     * @return Current pulse count
     */
    int update(const juce::Optional<juce::AudioPlayHead::PositionInfo>& position) {
        if (!position.hasValue()) {
            // No position info available
            // not sure when this would be the case
            isPlaying = false;
            return currentPulseCount;
        }

        // this is the audio playhead position info object
        auto& pos = *position;
        
        // Check if DAW is playing by talking to the position info
        if (pos.getIsPlaying()) {
            isPlaying = true;
            
            // This provides us with that double, i.e we are at bar 4.6787
            auto ppqPos = pos.getPpqPosition();
            if (ppqPos.hasValue()) {
                // this is not a derefernce of a pointer, it s juce optional and how we get the value.
                double currentPpq = *ppqPos;
                
                // Detect play-head jump using buffers
                // 0.1 is to make sure that we don't mistakenly detect backwards movement from
                // floating point accumulation errors
                // Expected: 8.5000000000
                // Actual:   8.4999999997  // Oops, looks like it went "backward"!
                // similarly with the forward jump but we can play with this...
                // NOTE: this +1 maybe could be a source of a stupid bug?
                if (currentPpq < lastPpqPosition - backPlayHeadJumpBuffer || currentPpq > lastPpqPosition + forwardPlayHeadJumpBuffer) {
                    // Playhead jumped - recalculate pulse count
                    currentPulseCount = calculatePulseCount(currentPpq);
                } else {
                    // Normal playback - update pulse count
                    currentPulseCount = calculatePulseCount(currentPpq);
                }

                lastPpqPosition = currentPpq; // just get ready for the next round
            }
        } else {
            // DAW stopped
            if (isPlaying) {
                // Just stopped - reset
                // TODO: this should be configurable in settings
                reset();
            }
            isPlaying = false;
        }
        
        return currentPulseCount;
    }
    
    /**
     * Calculate pulse count from PPQ position
     * 
     * Formula:
     * Pulse count = PPQ position × pulses_per_quarter_note
     * 
     * @param ppqPosition Position in quarter notes
     * @return Pulse count
     */
    int calculatePulseCount(double ppqPosition) const {
        int pulsesPerQuarterNote = getPulsesPerQuarterNote(subdivision);
        return static_cast<int>(ppqPosition * pulsesPerQuarterNote);
    }
    
    /**
     * Check if the DAW is playing
     * 
     * @return true if playing, false otherwise
     */
    bool getIsPlaying() const {
        return isPlaying;
    }
    
    /**
     * Get the current pulse count
     * 
     * @return Current pulse count
     */
    int getCurrentPulseCount() const {
        return currentPulseCount;
    }
    
    /**
     * Reset the DAW sync state
     */
    void reset() {
        lastPpqPosition = 0.0;
        currentPulseCount = 0;
    }
    
    /**
     * Get the last PPQ position (for testing)
     * 
     * @return Last PPQ position
     */
    double getLastPpqPosition() const {
        return lastPpqPosition;
    }
    
private:
    Subdivision subdivision;                    // Note subdivision
    double lastPpqPosition;                     // Last PPQ position from DAW
    int currentPulseCount;                      // Current pulse count
    bool isPlaying;                             // Whether DAW is playing
    double backPlayHeadJumpBuffer = 0.1;        // detection buffer see notes above
    double forwardPlayHeadJumpBuffer = 1.0;     // detection buffer see notes above
};

