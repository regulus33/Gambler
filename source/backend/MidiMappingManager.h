#pragma once

#include "MidiMapping.h"
#include <vector>
#include <unordered_map>
#include <juce_core/juce_core.h>

/**
 * MidiMappingManager
 *
 * Manages MIDI mappings and provides fast lookup.
 * Phase 1: Hardcoded default configuration
 * Phase 2: User-configurable mappings via UI
 *
 * Design:
 * - Two separate hash maps for CC and Note mappings
 * - Fast O(1) lookup by MIDI number
 * - No channel filtering (all channels 1-16 trigger mappings)
 */
class MidiMappingManager {
public:
    MidiMappingManager() {
        // Initialize with hardcoded default mappings
        initializeDefaultMappings();
    }

    /**
     * Find mapping for a MIDI CC message
     * @param ccNumber CC number (0-127)
     * @return Pointer to mapping if found, nullptr otherwise
     */
    const MidiMapping* findCCMapping(int ccNumber) const {
        auto it = ccMappings.find(ccNumber);
        if (it != ccMappings.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * Find mapping for a MIDI Note message
     * @param noteNumber Note number (0-127)
     * @return Pointer to mapping if found, nullptr otherwise
     */
    const MidiMapping* findNoteMapping(int noteNumber) const {
        auto it = noteMappings.find(noteNumber);
        if (it != noteMappings.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * Generic find mapping by type and number
     */
    const MidiMapping* findMapping(MidiMessageType type, int midiNumber) const {
        if (type == MidiMessageType::CC) {
            return findCCMapping(midiNumber);
        } else {
            return findNoteMapping(midiNumber);
        }
    }

    /**
     * Add or update a mapping
     */
    void addMapping(const MidiMapping& mapping) {
        if (mapping.type == MidiMessageType::CC) {
            ccMappings[mapping.midiNumber] = mapping;
        } else {
            noteMappings[mapping.midiNumber] = mapping;
        }
    }

    /**
     * Remove a mapping
     */
    void removeMapping(MidiMessageType type, int midiNumber) {
        if (type == MidiMessageType::CC) {
            ccMappings.erase(midiNumber);
        } else {
            noteMappings.erase(midiNumber);
        }
    }

    /**
     * Clear all mappings
     */
    void clearAllMappings() {
        ccMappings.clear();
        noteMappings.clear();
    }

    /**
     * Get all CC mappings (for UI display)
     */
    const std::unordered_map<int, MidiMapping>& getCCMappings() const {
        return ccMappings;
    }

    /**
     * Get all Note mappings (for UI display)
     */
    const std::unordered_map<int, MidiMapping>& getNoteMappings() const {
        return noteMappings;
    }

private:
    // Hash maps for fast lookup: MIDI number → MidiMapping
    std::unordered_map<int, MidiMapping> ccMappings;
    std::unordered_map<int, MidiMapping> noteMappings;

    /**
     * Initialize hardcoded default mappings
     *
     * This is Phase 1: hardcoded config to prove the architecture works.
     * Phase 2 will add UI-based configuration.
     *
     * Default mapping scheme:
     * - CC 1-7:   CC Lane values (matches typical MIDI controller layout)
     * - CC 10:    Octave Slider
     * - CC 11:    Note Slider
     * - CC 12:    Probability Slider
     * - CC 14-15: Note Range Min/Max
     * - CC 20-27: Sequencer window steps 0-7
     * - CC 30-37: Subsequence bars 0-7
     * - Notes 36-43 (C2-G2): Sequencer control buttons
     * - Notes 48-55 (C3-G3): Sequencer window steps 0-7 (alternate)
     * - Notes 60 (C4): Roll button (momentary)
     */
    void initializeDefaultMappings() {
        DBG("Initializing default MIDI mappings...");

        // ========== CC LANE VALUES (CC 1-7) ==========
        // Map CC 1-7 to the 7 CC lane value sliders
        addMapping({ MidiMessageType::CC, 1, MidiControlId::CC_LANE_0_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 2, MidiControlId::CC_LANE_1_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 3, MidiControlId::CC_LANE_2_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 4, MidiControlId::CC_LANE_3_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 5, MidiControlId::CC_LANE_4_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 6, MidiControlId::CC_LANE_5_VALUE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 7, MidiControlId::CC_LANE_6_VALUE, 0.0f, 1.0f, false });

        // ========== MAIN CONTROLS (CC 10-12) ==========
        addMapping({ MidiMessageType::CC, 10, MidiControlId::OCTAVE_SLIDER, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 11, MidiControlId::NOTE_SLIDER, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 12, MidiControlId::PROBABILITY_SLIDER, 0.0f, 1.0f, false });

        // ========== NOTE RANGE (CC 14-15) ==========
        addMapping({ MidiMessageType::CC, 14, MidiControlId::NOTE_RANGE_MIN, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 15, MidiControlId::NOTE_RANGE_MAX, 0.0f, 1.0f, false });

        // ========== SEQUENCER WINDOW STEPS (CC 20-27) ==========
        // CC 20-27 map to the 8 visible steps in the sequencer window
        addMapping({ MidiMessageType::CC, 20, MidiControlId::STEP_0, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 21, MidiControlId::STEP_1, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 22, MidiControlId::STEP_2, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 23, MidiControlId::STEP_3, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 24, MidiControlId::STEP_4, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 25, MidiControlId::STEP_5, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 26, MidiControlId::STEP_6, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 27, MidiControlId::STEP_7, 0.0f, 1.0f, false });

        // ========== SUBSEQUENCE BARS (CC 30-37) ==========
        addMapping({ MidiMessageType::CC, 30, MidiControlId::SUBSEQ_BAR_0, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 31, MidiControlId::SUBSEQ_BAR_1, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 32, MidiControlId::SUBSEQ_BAR_2, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 33, MidiControlId::SUBSEQ_BAR_3, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 34, MidiControlId::SUBSEQ_BAR_4, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 35, MidiControlId::SUBSEQ_BAR_5, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 36, MidiControlId::SUBSEQ_BAR_6, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::CC, 37, MidiControlId::SUBSEQ_BAR_7, 0.0f, 1.0f, false });

        // ========== SEQUENCER CONTROL BUTTONS (Notes 36-43 = C2-G2) ==========
        // Momentary behavior: Note On = press, Note Off = release
        addMapping({ MidiMessageType::NOTE, 36, MidiControlId::BUTTON_ROLL, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 37, MidiControlId::BUTTON_PAGE_LEFT_SHIFT_DUPLICATE_SEQUENCE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 38, MidiControlId::BUTTON_PAGE_RIGHT_SHIFT_CLEAR_WINDOW, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 39, MidiControlId::BUTTON_LENGTH_PLUS_SHIFT_CLEAR_STEP, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 40, MidiControlId::BUTTON_LENGTH_MINUS_SHIFT_RANDOMIZE_STEP, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 41, MidiControlId::BUTTON_NUDGE_LEFT_SHIFT_RANDOMIZE_ALL, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 42, MidiControlId::BUTTON_NUDGE_RIGHT_SHIFT_SHUFFLE, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 43, MidiControlId::BUTTON_SHIFT, 0.0f, 1.0f, false });

        // ========== SEQUENCER WINDOW STEPS (Notes 48-55 = C3-G3) ==========
        // Alternate note-based triggering for drum pad controllers
        addMapping({ MidiMessageType::NOTE, 48, MidiControlId::STEP_0, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 49, MidiControlId::STEP_1, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 50, MidiControlId::STEP_2, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 51, MidiControlId::STEP_3, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 52, MidiControlId::STEP_4, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 53, MidiControlId::STEP_5, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 54, MidiControlId::STEP_6, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 55, MidiControlId::STEP_7, 0.0f, 1.0f, false });

        // ========== ROLL BUTTON (Note 60 = C4) ==========
        // Duplicate mapping for easy access on keyboard controllers
        addMapping({ MidiMessageType::NOTE, 60, MidiControlId::BUTTON_ROLL, 0.0f, 1.0f, false });

        // ========== SUBSEQUENCE CONTROLS (Notes 64-73) ==========
        // Notes 64-71 (E4-B4): Subsequence bars 0-7
        addMapping({ MidiMessageType::NOTE, 64, MidiControlId::SUBSEQ_BAR_0, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 65, MidiControlId::SUBSEQ_BAR_1, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 66, MidiControlId::SUBSEQ_BAR_2, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 67, MidiControlId::SUBSEQ_BAR_3, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 68, MidiControlId::SUBSEQ_BAR_4, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 69, MidiControlId::SUBSEQ_BAR_5, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 70, MidiControlId::SUBSEQ_BAR_6, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 71, MidiControlId::SUBSEQ_BAR_7, 0.0f, 1.0f, false });
        // Notes 72-73: Minus/Plus buttons
        addMapping({ MidiMessageType::NOTE, 72, MidiControlId::SUBSEQ_MINUS, 0.0f, 1.0f, false });
        addMapping({ MidiMessageType::NOTE, 73, MidiControlId::SUBSEQ_PLUS, 0.0f, 1.0f, false });

        DBG("Default MIDI mappings initialized:");
        DBG("  - " + juce::String(ccMappings.size()) + " CC mappings");
        DBG("  - " + juce::String(noteMappings.size()) + " Note mappings");
        DBG("  - Total: " + juce::String(ccMappings.size() + noteMappings.size()) + " mappings");
    }
};
