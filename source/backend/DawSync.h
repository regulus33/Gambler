#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ClockMode.h"

/**
 * DawSync - Utility for DAW synchronization
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
        : subdivision(Subdivision::SIXTEENTH)
        , lastPpqPosition(0.0)
        , currentPulseCount(0)
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
     * Update the DAW sync state from AudioPlayHead position
     * 
     * This should be called from the audio callback.
     * 
     * @param position Optional position info from AudioPlayHead
     * @return Current pulse count
     */
    int update(const juce::Optional<juce::AudioPlayHead::PositionInfo>& position) {
        if (!position.hasValue()) {
            // No position info available
            isPlaying = false;
            return currentPulseCount;
        }
        
        auto& pos = *position;
        
        // Check if DAW is playing
        if (pos.getIsPlaying()) {
            isPlaying = true;
            
            // Get PPQ position (quarter note position)
            auto ppqPos = pos.getPpqPosition();
            if (ppqPos.hasValue()) {
                double currentPpq = *ppqPos;
                
                // Detect playhead jump (user scrubbed or looped)
                if (currentPpq < lastPpqPosition - 0.1 || currentPpq > lastPpqPosition + 1.0) {
                    // Playhead jumped - recalculate pulse count
                    currentPulseCount = calculatePulseCount(currentPpq);
                } else {
                    // Normal playback - update pulse count
                    currentPulseCount = calculatePulseCount(currentPpq);
                }
                
                lastPpqPosition = currentPpq;
            }
        } else {
            // DAW stopped
            if (isPlaying) {
                // Just stopped - reset
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
    Subdivision subdivision;       // Note subdivision
    double lastPpqPosition;        // Last PPQ position from DAW
    int currentPulseCount;         // Current pulse count
    bool isPlaying;                // Whether DAW is playing
};

