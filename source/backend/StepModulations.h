#pragma once

#include "StepModulation.h"
#include <array>
#include <vector>
#include <random>

/**
 * StepModulations - Container for 7 modulation slots per step
 * 
 * Holds CC and velocity data that can be sent alongside the note.
 * The user can select which modulations are active via the UI.
 */
class StepModulations {
public:
    static constexpr int NUM_MODULATIONS = 7;
    static constexpr int DEFAULT_VELOCITY = 100;
    
    StepModulations() {
        // First slot defaults to velocity
        modulations[0] = StepModulation(ModulationType::Velocity, 0, DEFAULT_VELOCITY);
        
        // Rest default to CC 1-6
        for (int i = 1; i < NUM_MODULATIONS; i++) {
            modulations[i] = StepModulation(ModulationType::CC, i, 64);
        }
    }
    
    // Getters
    const StepModulation& getModulation(int index) const {
        if (index >= 0 && index < NUM_MODULATIONS) {
            return modulations[index];
        }
        return modulations[0];  // Fallback
    }
    
    StepModulation& getModulation(int index) {
        if (index >= 0 && index < NUM_MODULATIONS) {
            return modulations[index];
        }
        return modulations[0];  // Fallback
    }
    
    /**
     * Get all modulations as a vector (for MIDI output)
     * Only returns modulations that are selected
     */
    std::vector<StepModulation> getSelectedModulations() const {
        std::vector<StepModulation> selected;
        for (const auto& mod : modulations) {
            if (mod.isSelected()) {
                selected.push_back(mod);
            }
        }
        return selected;
    }
    
    /**
     * Get all modulations (regardless of selection state)
     */
    std::vector<StepModulation> getAllModulations() const {
        return std::vector<StepModulation>(modulations.begin(), modulations.end());
    }
    
    // Setters
    void setModulationType(int index, ModulationType type) {
        if (index >= 0 && index < NUM_MODULATIONS) {
            modulations[index].setType(type);
        }
    }
    
    void setModulationCCNumber(int index, int ccNumber) {
        if (index >= 0 && index < NUM_MODULATIONS) {
            modulations[index].setCCNumber(ccNumber);
        }
    }
    
    void setModulationValue(int index, int value) {
        if (index >= 0 && index < NUM_MODULATIONS) {
            modulations[index].setValue(value);
        }
    }
    
    void setModulationSelected(int index, bool selected) {
        if (index >= 0 && index < NUM_MODULATIONS) {
            modulations[index].setSelected(selected);
        }
    }
    
    /**
     * Update multiple modulation values at once
     * @param values Array of values to set (must have NUM_MODULATIONS elements)
     */
    void updateModulations(const std::array<int, NUM_MODULATIONS>& values) {
        for (int i = 0; i < NUM_MODULATIONS; i++) {
            modulations[i].setValue(values[i]);
        }
    }
    
    /**
     * Randomize all modulation values
     */
    void randomize() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 127);
        
        for (auto& mod : modulations) {
            mod.setValue(dis(gen));
        }
    }
    
    /**
     * Tick - returns all modulations (doesn't advance anything)
     * This is called when the parent Step is triggered
     */
    std::vector<StepModulation> tick() const {
        return getSelectedModulations();
    }
    
private:
    std::array<StepModulation, NUM_MODULATIONS> modulations;
};

