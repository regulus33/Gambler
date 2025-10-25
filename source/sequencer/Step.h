//
// Created by Gambler developers on 15.10.2025.
//

#ifndef GAMBLER_STEP_H
#define GAMBLER_STEP_H

/**
 * @class Step
 * @details is a container for a SubSequence, it does not hold any state of its own.
 * think of it as an interface through which the Sequencer accesses 1 of 8 steps
 */
class Step
{
public:
    Step() = default;

    // Getters
    int getMidiNote() const { return midiNote; }
    int getMidiVelocity() const { return midiVelocity; }
    int getSubStepIndex() const { return subStepIndex; }
    float getGateLength() const { return gateLength; }
    bool isActive() const { return active; }
    bool isSelected() const { return selected; }

    // Setters
    void setMidiNote(int note) { midiNote = note; }
    void setMidiVelocity(int velocity) { midiVelocity = velocity; }
    void setGateLength(float length) { gateLength = length; }
    void setActive(bool isActive) { active = isActive; }
    void setSelected(bool isSelected) { selected = isSelected; }

    void advance() {

    }

private:
    int midiNote = 60;        // Default to middle C
    int midiVelocity = 100;   // Default velocity
    int subStepIndex = 0;
    float gateLength = 0.75f; // Default gate length (75% of step duration)
    bool active = false;
    bool selected = false;
};

#endif //GAMBLER_STEP_H
