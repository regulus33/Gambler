//
// Created by Gambler developers on 14.10.2025.
//

#ifndef GAMBLER_SEQUENCER_H
#define GAMBLER_SEQUENCER_H
#include "Controls.h"
#include "Step.h"
#include "SubSequence.h"
#include <array>
#include <random>


using StepArray = std::array<Step, 128>;
/**
 * @brief Main backend of the app.
 */

class Sequencer {
public:
    /**
     * @brief Constructs a new Sequencer with default values
     *
     * Initializes all 128 steps with their step index and matching MIDI note values.
     */
    Sequencer() {
        // Initialize steps with default values
        for (int i = 0; i < 128; ++i) {
            this->steps[i].setMidiNote(i); // Default note matches step index
        }
    }

    /**
     * @brief Advances to the next step in the sequence
     *
     * If random mode is enabled, randomizes the MIDI note of the current step
     * before advancing. The step index wraps around to 0 after reaching the
     * sequence length.
     */
    void advance() {
        // If random mode is enabled, randomize the note of the current step before advancing
        if (controls.getRandom()) {
            steps[stepIndex].setMidiNote(getRandomNote());
        }

        stepIndex = (stepIndex + 1) % stepLength;
    }

    /**
       * @brief Gets the current step by reference
       * @return Reference to the current Step
       */
    Step getCurrentStep() const {
        return this->getStep (this->getStepIndex());
    }

    /**
     * @brief Gets the current step index
     * @return The current step index (0-127)
     */
    int getStepIndex() const {
        return stepIndex;
    }

    /**
     * @brief Sets the current step index
     * @param index The new step index (must be between 0 and 127)
     */
    void setStepIndex (int index) {
        if (index >= 0 && index < 128) {
            stepIndex = index;
        }
    }

    /**
     * @brief Gets the sequence length
     * @return The current sequence length (1-128)
     */
    int getStepLength() const {
        return stepLength;
    }

    /**
     * @brief Sets the sequence length
     * @param length The new sequence length (must be between 1 and 128)
     */
    void setStepLength (int length) {
        if (length >= 1 && length <= 128) {
            stepLength = length;
        }
    }

    /**
     * @brief Gets a step at the specified index
     * @param index The step index to retrieve
     * @return Reference to the Step at the given index
     */
    Step& getStep (int index) {
        return steps[index];
    }

    /**
     * @brief Gets a step at the specified index (const version)
     * @param index The step index to retrieve
     * @return Const reference to the Step at the given index
     */
    const Step& getStep (int index) const {
        return steps[index];
    }

    /**
     * @brief Gets the controls object
     * @return Reference to the Controls object
     */
    Controls& getControls() {
        return controls;
    }

private:
    /**
     * @brief Generates a random MIDI note value
     * @return Random note value between 0 and 127
     */
    int getRandomNote() {
        std::uniform_int_distribution<int> distribution (0, 127);
        return distribution (randomEngine);
    }

    StepArray steps;
    Controls controls;
    int stepIndex = 0;
    int stepLength = 16;
    std::mt19937 randomEngine { std::random_device {}() };
};

#endif //GAMBLER_SEQUENCER_H