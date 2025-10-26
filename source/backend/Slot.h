//
// Created by Gambler developers on 26.10.2025.
//

#ifndef GAMBLER_SLOT_H
#define GAMBLER_SLOT_H

constexpr int DEFAULT_CC_NUMBER = 1; // Modulation wheel

enum class SlotType {
    CC,
    Velocity
};

struct Slot {
    SlotType type = SlotType::CC;
    int ccNumber = DEFAULT_CC_NUMBER;
    int value = 64;
    

    Slot(SlotType type, int ccNumber, int value) {
        this->type = type;
        this->ccNumber = ccNumber;
        this->value = value;
    }

    Slot() = default;
};

#endif //GAMBLER_SLOT_H

