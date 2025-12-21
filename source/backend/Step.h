#pragma once

#include "StepNotes.h"
#include "StepModulations.h"
#include "Scale.h"
#include <random>
#include <juce_core/juce_core.h>

/**
 * Step - A single step in the sequence
 * 
 * Each Step contains:
 * - StepNotes: A sub-sequence of up to 8 notes that cycle on each trigger
 * - StepModulations: 7 modulation slots (CC/velocity) that can be sent with the note
 * 
 * Step is responsible for forwarding data update messages to its children
 * and coordinating the tick() behavior.
 */
class Step {
public:
    Step() = default;

    // Getters
    StepNotes& getStepNotes() { return stepNotes; }
    const StepNotes& getStepNotes() const { return stepNotes; }

    StepModulations& getStepModulations() { return stepModulations; }
    const StepModulations& getStepModulations() const { return stepModulations; }

    /**
     * Tick this step - advances the StepNotes sub-sequence
     * and returns the current note number
     * 
     * @return MIDI note number to play
     */
    int tick() {
        return stepNotes.tick();
    }
    
    /**
     * Get the modulations for this step (doesn't advance anything)
     * 
     * @return Vector of selected modulations
     */
    std::vector<StepModulation> getModulations() const {
        return stepModulations.tick();
    }
    
    // Note operations
    
    /**
     * Randomize the current note in StepNotes
     * Uses the provided Scale for quantization and range
     *
     * Generates random note directly within the user-set range,
     * then quantizes to the scale. This avoids boundary pile-up
     * that would occur from clamping.
     */
    void randomizeNote(const Scale& scale) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // Generate random note directly within the user-set range
        int rangeMin = scale.getRangeMin();
        int rangeMax = scale.getRangeMax();
        std::uniform_int_distribution<> dis(rangeMin, rangeMax);

        int randomNote = dis(gen);
        int quantized = scale.quantize(randomNote);

        // Set the current note (or first note if not yet ticked)
        int idx = stepNotes.getCurrentIndex();
        if (idx == -1) idx = 0;
        stepNotes.setNote(idx, quantized);
    }
    
    /**
     * Update the current note number
     * @param noteNumber MIDI note number (0-127)
     */
    void updateCurrentNoteNumber(int noteNumber) {
        int idx = stepNotes.getCurrentIndex();
        if (idx == -1) idx = 0;
        stepNotes.setNote(idx, noteNumber);
    }
    
    /**
     * Update the current note's octave
     * @param octaveOffset Octave offset (-4 to +4)
     */
    void updateCurrentNoteOctave(int octaveOffset) {
        int idx = stepNotes.getCurrentIndex();
        if (idx == -1) idx = 0;
        
        int currentNote = stepNotes.getNote(idx).getNoteNumber();
        int pitchClass = currentNote % 12;
        int newNote = pitchClass + ((octaveOffset + 5) * 12);  // +5 to handle negative offsets
        
        // Clamp to valid MIDI range
        if (newNote < 0) newNote = 0;
        if (newNote > 127) newNote = 127;
        
        stepNotes.setNote(idx, newNote);
    }
    
    // Modulation operations
    
    /**
     * Randomize all modulation values
     */
    void randomizeModulations() {
        stepModulations.randomize();
    }
    
    /**
     * Update modulation values
     * @param values Array of 7 values to set
     */
    void updateModulations(const std::array<int, StepModulations::NUM_MODULATIONS>& values) {
        stepModulations.updateModulations(values);
    }
    
    /**
     * Reset this step to initial state
     */
    void reset() {
        stepNotes.reset();
    }

    /**
     * Serialize to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("Step");
        xml->addChildElement(stepNotes.toXml().release());
        xml->addChildElement(stepModulations.toXml().release());
        return xml;
    }

    /**
     * Deserialize from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "Step") return;

        for (auto* childXml : xml.getChildIterator()) {
            if (childXml->getTagName() == "StepNotes") {
                stepNotes.fromXml(*childXml);
            } else if (childXml->getTagName() == "StepModulations") {
                stepModulations.fromXml(*childXml);
            }
        }
    }

private:
    StepNotes stepNotes;
    StepModulations stepModulations;
};

