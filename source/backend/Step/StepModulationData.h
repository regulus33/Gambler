#pragma once

#include "../ModulationType.h"

/**
 * StepModulationData - Lightweight struct for step modulation data
 */
struct StepModulationData {
    ModulationType type = ModulationType::CC;
    int ccNumber = 1;
    int value = 64;
    bool selected = false;

    ModulationType getType() const { return type; }
    int getCCNumber() const { return ccNumber; }
    int getValue() const { return value; }
    bool isSelected() const { return selected; }
};
