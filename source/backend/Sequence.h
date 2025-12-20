#pragma once

#include "Step.h"
#include "Scale.h"
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <juce_core/juce_core.h>

/**
 * Sequence - Master orchestrator of the sequencer
 * 
 * Contains all domain objects related to the operation of the sequencer:
 * - 64 Steps (each with StepNotes and StepModulations)
 * - Scale for quantization and range limiting
 * - Play probability array for rest/play decisions
 * - Current index and length management
 * 
 * This is the main entry point for sequencer operations.
 */
class Sequence {
public:
    static constexpr int MAX_STEPS = 64;
    static constexpr int DEFAULT_LENGTH = 8;
    static constexpr int MIN_LENGTH = 1;
    
    Sequence() {
        // Initialize play probability to all true (100% probability)
        playProbability.fill(true);
    }
    
    // Getters
    int getCurrentIndex() const { return currentIndex; }
    int getLength() const { return length; }
    bool getIsRandom() const { return isRandom; }
    bool getIsShift() const { return isShift; }
    
    Step& getStep(int index) {
        if (index >= 0 && index < MAX_STEPS) {
            return steps[index];
        }
        return steps[0];  // Fallback
    }
    
    const Step& getStep(int index) const {
        if (index >= 0 && index < MAX_STEPS) {
            return steps[index];
        }
        return steps[0];  // Fallback
    }
    
    Step& getCurrentStep() {
        int idx = (currentIndex == -1) ? 0 : currentIndex;
        return steps[idx];
    }
    
    const Step& getCurrentStep() const {
        int idx = (currentIndex == -1) ? 0 : currentIndex;
        return steps[idx];
    }
    
    Scale& getScale() { return scale; }
    const Scale& getScale() const { return scale; }
    
    bool getPlayProbability(int index) const {
        if (index >= 0 && index < MAX_STEPS) {
            return playProbability[index];
        }
        return true;
    }
    
    // Setters
    void setIsRandom(bool random) { isRandom = random; }
    void setIsShift(bool shift) { isShift = shift; }
    
    void setLength(int newLength) {
        if (newLength >= MIN_LENGTH && newLength <= MAX_STEPS) {
            length = newLength;
        }
    }

    /**
     * Set the current index directly (for UI step selection)
     * @param index The step index to set (0 to length-1)
     */
    void setCurrentIndex(int index) {
        if (index >= 0 && index < length) {
            currentIndex = index;
        }
    }
    
    void setPlayProbability(int index, bool shouldPlay) {
        if (index >= 0 && index < MAX_STEPS) {
            playProbability[index] = shouldPlay;
        }
    }
    
    /**
     * Set play probability based on percentage (0-100)
     * Randomly distributes true/false values across the sequence
     * 
     * @param percentage 0-100, where 100 = all steps play, 0 = all steps rest
     */
    void setProbabilityPercentage(int percentage) {
        if (percentage < 0) percentage = 0;
        if (percentage > 100) percentage = 100;
        
        int numToPlay = (length * percentage) / 100;
        
        // Reset all to false
        playProbability.fill(false);
        
        // Randomly set numToPlay indices to true
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::vector<int> indices;
        for (int i = 0; i < length; i++) {
            indices.push_back(i);
        }
        
        std::shuffle(indices.begin(), indices.end(), gen);
        
        for (int i = 0; i < numToPlay && i < static_cast<int>(indices.size()); i++) {
            playProbability[indices[i]] = true;
        }
    }
    
    // Core sequencer operations
    
    /**
     * Advance the sequencer to the next step and return the note to play
     * 
     * This is the main sequencer tick function that:
     * 1. Advances currentIndex
     * 2. Checks if isRandom is enabled and randomizes if so
     * 3. Ticks the current Step's StepNotes
     * 4. Returns the MIDI note number (or -1 if rest)
     * 
     * @return MIDI note number to play, or -1 for rest
     */
    int tick() {
        // Advance index (wraps at length boundary)
        currentIndex = (currentIndex + 1) % length;
        
        // Check play probability - if false, return -1 (rest)
        if (!playProbability[currentIndex]) {
            return -1;  // Rest
        }
        
        // If random mode is enabled, randomize the current step's note
        if (isRandom) {
            steps[currentIndex].randomizeNote(scale);
        }
        
        // Tick the current step and return its note
        return steps[currentIndex].tick();
    }
    
    /**
     * Get modulations for the current step
     * Should be called after tick() to get modulations for the note that was just played
     * 
     * @return Vector of selected modulations
     */
    std::vector<StepModulation> getCurrentModulations() const {
        int idx = (currentIndex == -1) ? 0 : currentIndex;
        return steps[idx].getModulations();
    }
    
    // Index navigation
    
    /**
     * Move current index left (decrease by 1)
     */
    void indexLeft() {
        currentIndex--;
        if (currentIndex < 0) {
            currentIndex = length - 1;
        }
    }
    
    /**
     * Move current index right (increase by 1)
     */
    void indexRight() {
        currentIndex = (currentIndex + 1) % length;
    }
    
    // Length operations
    
    /**
     * Increase sequence length by 1 (up to MAX_STEPS)
     */
    void increaseLength() {
        if (length < MAX_STEPS) {
            length++;
        }
    }
    
    /**
     * Decrease sequence length by 1 (down to MIN_LENGTH)
     */
    void decreaseLength() {
        if (length > MIN_LENGTH) {
            length--;
            // If current index is now out of bounds, wrap it
            if (currentIndex >= length) {
                currentIndex = length - 1;
            }
        }
    }
    
    // Nudge operations (shift entire sequence left/right)
    
    /**
     * Shift the entire sequence to the left within the 64 steps
     * The first step wraps around to the end
     */
    void nudgeLeft() {
        if (length <= 1) return;
        
        Step temp = steps[0];
        for (int i = 0; i < length - 1; i++) {
            steps[i] = steps[i + 1];
        }
        steps[length - 1] = temp;
    }
    
    /**
     * Shift the entire sequence to the right within the 64 steps
     * The last step wraps around to the beginning
     */
    void nudgeRight() {
        if (length <= 1) return;
        
        Step temp = steps[length - 1];
        for (int i = length - 1; i > 0; i--) {
            steps[i] = steps[i - 1];
        }
        steps[0] = temp;
    }
    
    /**
     * Reset the sequence to initial state
     */
    void reset() {
        currentIndex = -1;
        for (auto& step : steps) {
            step.reset();
        }
    }

    /**
     * Quantize all notes in the entire sequence to the current scale.
     *
     * This iterates through all 64 steps and all 8 substeps within each step,
     * quantizing each note to the nearest note in the current scale.
     *
     * Use this when the scale changes to ensure all existing notes
     * are valid within the new scale constraints.
     */
    void quantizeAllNotesToScale() {
        for (int stepIdx = 0; stepIdx < MAX_STEPS; stepIdx++) {
            auto& stepNotes = steps[stepIdx].getStepNotes();
            for (int noteIdx = 0; noteIdx < StepNotes::MAX_LENGTH; noteIdx++) {
                int currentNote = stepNotes.getNote(noteIdx).getNoteNumber();
                int quantizedNote = scale.quantize(currentNote);
                stepNotes.setNote(noteIdx, quantizedNote);
            }
        }
    }

    /**
     * Serialize entire sequence state to XML
     * This is the main entry point for saving plugin state
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("Sequence");

        // Serialize basic properties
        xml->setAttribute("currentIndex", currentIndex);
        xml->setAttribute("length", length);
        xml->setAttribute("isRandom", isRandom);
        xml->setAttribute("isShift", isShift);

        // Serialize scale
        xml->addChildElement(scale.toXml().release());

        // Serialize play probability as comma-separated string
        juce::String playProbStr;
        for (int i = 0; i < MAX_STEPS; i++) {
            if (i > 0) playProbStr += ",";
            playProbStr += playProbability[i] ? "1" : "0";
        }
        xml->setAttribute("playProbability", playProbStr);

        // Serialize all steps
        auto stepsXml = std::make_unique<juce::XmlElement>("Steps");
        for (int i = 0; i < MAX_STEPS; i++) {
            auto stepXml = steps[i].toXml();
            stepXml->setAttribute("index", i);
            stepsXml->addChildElement(stepXml.release());
        }
        xml->addChildElement(stepsXml.release());

        return xml;
    }

    /**
     * Deserialize entire sequence state from XML
     * This is the main entry point for loading plugin state
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "Sequence") return;

        // Deserialize basic properties
        currentIndex = xml.getIntAttribute("currentIndex", -1);
        length = xml.getIntAttribute("length", DEFAULT_LENGTH);
        isRandom = xml.getBoolAttribute("isRandom", false);
        isShift = xml.getBoolAttribute("isShift", false);

        // Deserialize play probability
        juce::String playProbStr = xml.getStringAttribute("playProbability");
        juce::StringArray tokens = juce::StringArray::fromTokens(playProbStr, ",", "");
        for (int i = 0; i < MAX_STEPS && i < tokens.size(); i++) {
            playProbability[i] = (tokens[i].getIntValue() != 0);
        }

        // Deserialize scale
        for (auto* childXml : xml.getChildIterator()) {
            if (childXml->getTagName() == "Scale") {
                scale.fromXml(*childXml);
            } else if (childXml->getTagName() == "Steps") {
                // Deserialize all steps
                for (auto* stepXml : childXml->getChildIterator()) {
                    if (stepXml->getTagName() == "Step") {
                        int index = stepXml->getIntAttribute("index", -1);
                        if (index >= 0 && index < MAX_STEPS) {
                            steps[index].fromXml(*stepXml);
                        }
                    }
                }
            }
        }
    }

private:
    std::array<Step, MAX_STEPS> steps;
    Scale scale;
    std::array<bool, MAX_STEPS> playProbability;

    int currentIndex = -1;  // Start at -1 so first tick() advances to 0
    int length = DEFAULT_LENGTH;
    bool isRandom = false;
    bool isShift = false;  // Unused for now, reserved for future features
};

