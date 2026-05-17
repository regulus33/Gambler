#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ClockMode.h"
#include <cmath>
#include <vector>

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
     * Result of a single processBlock-worth of DAW sync work.
     */
    struct UpdateResult {
        int currentPulseCount = 0;
        std::vector<int> newPulseOffsets;  // sub-block sample offsets for pulses that fired
    };

    /**
     * Called every processBlock() in DAW_SYNC mode. Returns the cumulative
     * DAW pulse count plus the sub-block sample offset of every pulse that
     * crossed inside this block.
     *
     * @param position    Optional playhead position info.
     * @param sampleRate  Audio sample rate (required for sub-block math).
     * @param numSamples  Length of this audio block (samples).
     */
    UpdateResult update(const juce::Optional<juce::AudioPlayHead::PositionInfo>& position,
                        double sampleRate,
                        int numSamples) {
        UpdateResult result;
        result.currentPulseCount = currentPulseCount;

        if (!position.hasValue()) {
            isPlaying = false;
            return result;
        }

        auto& pos = *position;
        if (!pos.getIsPlaying()) {
            if (isPlaying) reset();
            isPlaying = false;
            result.currentPulseCount = currentPulseCount;
            return result;
        }

        isPlaying = true;
        auto ppqPos = pos.getPpqPosition();
        if (!ppqPos.hasValue())
            return result;

        const double currentPpq = *ppqPos;
        const int pulsesPerQuarter = getPulsesPerQuarterNote(subdivision);

        // Detect play-head jump (loop point, manual seek, transport-state
        // gap). The contract for DAW_SYNC is "the host clocks the sequencer":
        // a tick fires only when PPQ moves forward continuously across a
        // pulse boundary. Anything else — backwards motion, or a forward
        // jump bigger than what continuous playback in one block could
        // produce — must re-anchor without emitting pulses, otherwise we
        // end up firing spurious catch-up pulses to "match" the new
        // position.
        const double observedDelta = currentPpq - lastPpqPosition;
        double expectedDelta = -1.0;
        auto bpmOpt = pos.getBpm();
        if (bpmOpt.hasValue() && *bpmOpt > 0.0 && sampleRate > 0.0 && numSamples > 0) {
            expectedDelta = (static_cast<double>(numSamples) / sampleRate) * (*bpmOpt / 60.0);
        }

        bool jumped = observedDelta < -backPlayHeadJumpBuffer;
        if (!jumped) {
            if (expectedDelta > 0.0) {
                jumped = observedDelta > expectedDelta * continuousPlaybackTolerance;
            } else {
                // BPM unavailable — keep the legacy generous threshold so we
                // don't false-positive on hosts that don't report tempo.
                jumped = observedDelta > forwardPlayHeadJumpBuffer;
            }
        }
        if (jumped) {
            // Backwards jump = DAW loop-back (or rare backwards scrub during
            // play). Fire one pulse at sub-block offset 0 representing the
            // pulse-at-loop-start. Without this, every DAW loop iteration
            // silently drops the pulse at the loop boundary and the sequencer
            // drifts one step per loop.
            //
            // Forward jump = scrub during play / start-from-mid-position.
            // No pulses — re-anchor only, matching the "no catch-up" contract.
            const bool backwardsJump = observedDelta < 0.0;
            if (backwardsJump)
                result.newPulseOffsets.push_back(0);

            currentPulseCount = calculatePulseCount(currentPpq);
            lastPpqPosition = currentPpq;
            result.currentPulseCount = currentPulseCount;
            return result;
        }

        // Compute new pulses that crossed inside this block.
        // Block spans [lastPpqPosition, currentPpq) in PPQ space.
        if (sampleRate > 0.0 && numSamples > 0 && currentPpq > lastPpqPosition) {
            const double pulsesAtBlockStart = lastPpqPosition * static_cast<double>(pulsesPerQuarter);
            const double pulsesAtBlockEnd   = currentPpq      * static_cast<double>(pulsesPerQuarter);

            // First pulse index strictly greater than block-start. Using
            // floor()+1 (rather than ceil) keeps the boundary case "block
            // starts exactly on a pulse" from double-firing across blocks.
            int nextPulseIndex = static_cast<int>(std::floor(pulsesAtBlockStart)) + 1;

            // PPQ → samples conversion factor for this block.
            // BPM may be missing in tests; derive from the PPQ delta if so.
            double samplesPerQuarter = 0.0;
            if (bpmOpt.hasValue() && *bpmOpt > 0.0) {
                samplesPerQuarter = sampleRate * 60.0 / *bpmOpt;
            } else {
                const double ppqDelta = currentPpq - lastPpqPosition;
                if (ppqDelta > 0.0)
                    samplesPerQuarter = static_cast<double>(numSamples) / ppqDelta;
            }
            while (static_cast<double>(nextPulseIndex) < pulsesAtBlockEnd) {
                const double pulsePpq = static_cast<double>(nextPulseIndex)
                                          / static_cast<double>(pulsesPerQuarter);
                const double offsetD = (pulsePpq - lastPpqPosition)
                                          * samplesPerQuarter;
                int offset = static_cast<int>(std::lround(offsetD));
                if (offset < 0) offset = 0;
                if (offset >= numSamples) offset = numSamples - 1;
                result.newPulseOffsets.push_back(offset);
                nextPulseIndex++;
            }
        }

        currentPulseCount = calculatePulseCount(currentPpq);
        lastPpqPosition = currentPpq;
        result.currentPulseCount = currentPulseCount;
        return result;
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
    // Backwards-motion tolerance: hosts may report tiny non-monotonic PPQ
    // jitter during continuous play. Anything more than ~1ms-equivalent of
    // backwards motion is a real seek, not jitter.
    double backPlayHeadJumpBuffer = 0.001;

    // Forward jump multiplier on top of the per-block expected PPQ delta
    // (numSamples/sampleRate × BPM/60). 1.5 leaves headroom for host jitter
    // while still rejecting any deliberate forward scrub.
    double continuousPlaybackTolerance = 1.5;

    // Fallback forward threshold used when the host doesn't report BPM and
    // we can't compute a tight per-block expected delta.
    double forwardPlayHeadJumpBuffer = 1.0;
};

