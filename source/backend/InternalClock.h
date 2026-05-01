#pragma once

#include "ClockMode.h"
#include <cmath>
#include <vector>

/**
 * InternalClock - Pure domain object for internal timing
 * 
 * Handles internal clock timing based on BPM and sample counts.
 * This is a pure domain object with no JUCE dependencies.
 * 
 * The InternalClock maintains sample-accurate timing by:
 * 1. Receiving sample count and sample rate from audio callback
 * 2. Accumulating elapsed samples
 * 3. Calculating when the next pulse should occur based on BPM and subdivision
 * 4. Triggering a pulse when accumulated samples exceed the pulse interval
 */
class InternalClock {
public:
    /**
     * Default constructor
     * Initializes with 120 BPM and 16th note subdivision
     */
    InternalClock() 
        : bpm(120.0)
        , subdivision(Subdivision::SIXTEENTH)
        , sampleRate(44100.0)
        , accumulatedSamples(0.0)
        , isRunning(false) {
    }
    
    /**
     * Set the BPM for internal timing
     * 
     * @param newBpm BPM value (20.0 - 999.0)
     */
    void setBpm(double newBpm) {
        if (newBpm < 20.0) newBpm = 20.0;
        if (newBpm > 999.0) newBpm = 999.0;
        bpm = newBpm;
    }
    
    /**
     * Get the current BPM
     * 
     * @return Current BPM value
     */
    double getBpm() const {
        return bpm;
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
     * Set the sample rate
     * 
     * @param newSampleRate Sample rate in Hz
     */
    void setSampleRate(double newSampleRate) {
        if (newSampleRate > 0.0) {
            sampleRate = newSampleRate;
        }
    }
    
    /**
     * Get the current sample rate
     * 
     * @return Sample rate in Hz
     */
    double getSampleRate() const {
        return sampleRate;
    }
    
    /**
     * Start the internal clock
     */
    void start() {
        isRunning = true;
    }
    
    /**
     * Stop the internal clock
     */
    void stop() {
        isRunning = false;
    }
    
    /**
     * Check if the clock is running
     * 
     * @return true if running, false otherwise
     */
    bool getIsRunning() const {
        return isRunning;
    }
    
    /**
     * Reset the internal clock state
     * Clears accumulated samples
     */
    void reset() {
        accumulatedSamples = 0.0;
    }
    
    /**
     * Process a block of samples and return the sub-block sample offsets
     * where pulses crossed the boundary.
     *
     * Multiple pulses may fire in a single block (block size > samplesPerPulse).
     * accumulatedSamples carries the fractional residue forward so cross-block
     * timing stays exact.
     */
    std::vector<int> processSamples(int numSamples) {
        std::vector<int> offsets;
        if (!isRunning || numSamples <= 0) return offsets;

        const double samplesPerPulse = calculateSamplesPerPulse();
        if (samplesPerPulse <= 0.0) return offsets;

        // samplesUntilNextPulse: distance from the start of this block to the
        // next pulse boundary. accumulatedSamples is "samples elapsed since the
        // last pulse" at block start.
        double samplesUntilNextPulse = samplesPerPulse - accumulatedSamples;

        while (samplesUntilNextPulse < static_cast<double>(numSamples)) {
            int offset = static_cast<int>(std::lround(samplesUntilNextPulse));
            if (offset < 0) offset = 0;
            if (offset >= numSamples) offset = numSamples - 1;
            offsets.push_back(offset);
            samplesUntilNextPulse += samplesPerPulse;
        }

        accumulatedSamples += static_cast<double>(numSamples);
        accumulatedSamples -= static_cast<double>(offsets.size()) * samplesPerPulse;

        return offsets;
    }
    
    /**
     * Calculate the number of samples per pulse based on current BPM and subdivision
     * 
     * Formula:
     * Pulses per second = (BPM × pulses_per_quarter_note) / 60
     * Samples per pulse = sample_rate / pulses_per_second
     * 
     * @return Number of samples per pulse
     */
    double calculateSamplesPerPulse() const {
        int pulsesPerQuarterNote = getPulsesPerQuarterNote(subdivision);
        double pulsesPerSecond = (bpm * pulsesPerQuarterNote) / 60.0;
        return sampleRate / pulsesPerSecond;
    }
    
    /**
     * Get the accumulated samples (for testing)
     * 
     * @return Current accumulated sample count
     */
    double getAccumulatedSamples() const {
        return accumulatedSamples;
    }
    
private:
    double bpm;                    // Beats per minute (20.0 - 999.0)
    Subdivision subdivision;       // Note subdivision
    double sampleRate;             // Sample rate in Hz
    double accumulatedSamples;     // Accumulated samples since last pulse
    bool isRunning;                // Whether the clock is running
};

