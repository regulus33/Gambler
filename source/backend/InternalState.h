#pragma once

#include "../data/Params.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>

/**
 * InternalState - ValueTree-backed state for non-MIDI-mappable parameters
 *
 * This class stores sequencer state that doesn't need DAW automation or MIDI mapping:
 * - Step note data (64 steps × 8 substeps)
 * - Step notes length per step (64 values)
 * - Step notes current index per step (64 values)
 * - Step modulation data (64 steps × 7 mods × 4 properties)
 * - Sequence metadata (current index, last prob index, view window start)
 * - Play probability array (64 bools)
 * - Note colours (12 values)
 *
 * Benefits over APVTS for internal state:
 * - No DAW parameter exposure (cleaner automation lanes)
 * - Still supports listener pattern for UI updates
 * - Can be serialized to XML for preset saving
 * - Thread-safe with proper locking
 *
 * Usage:
 *   InternalState state;
 *   state.setStepNote(0, 0, 72);
 *   int note = state.getStepNote(0, 0);
 *
 *   // Listen for changes
 *   state.addListener(myListener);
 */
class InternalState : public juce::ValueTree::Listener {
public:
    // Property identifiers for ValueTree
    struct IDs {
        // Step notes: step_X_note_Y
        static juce::Identifier stepNote(int stepIndex, int substepIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_note_" + juce::String(substepIndex));
        }

        // Step notes length: step_X_notes_len
        static juce::Identifier stepNotesLength(int stepIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_notes_len");
        }

        // Step notes current index: step_X_notes_idx
        static juce::Identifier stepNotesCurrentIndex(int stepIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_notes_idx");
        }

        // Step modulation: step_X_mod_Y_type, step_X_mod_Y_cc, step_X_mod_Y_value, step_X_mod_Y_selected
        static juce::Identifier stepModType(int stepIndex, int modIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_mod_" + juce::String(modIndex) + "_type");
        }
        static juce::Identifier stepModCC(int stepIndex, int modIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_mod_" + juce::String(modIndex) + "_cc");
        }
        static juce::Identifier stepModValue(int stepIndex, int modIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_mod_" + juce::String(modIndex) + "_value");
        }
        static juce::Identifier stepModSelected(int stepIndex, int modIndex) {
            return juce::Identifier("step_" + juce::String(stepIndex) + "_mod_" + juce::String(modIndex) + "_selected");
        }

        // Sequence metadata
        static inline const juce::Identifier SEQ_CURRENT_INDEX {"seq_current_index"};
        static inline const juce::Identifier SEQ_LAST_PROB_INDEX {"seq_last_prob_index"};
        static inline const juce::Identifier SEQ_VIEW_WINDOW_START {"seq_view_window_start"};

        // Play probability: prob_X
        static juce::Identifier playProbability(int index) {
            return juce::Identifier("prob_" + juce::String(index));
        }

        // Note colours: note_colour_X
        static juce::Identifier noteColour(int index) {
            return juce::Identifier("note_colour_" + juce::String(index));
        }

        // Root tree type
        static inline const juce::Identifier ROOT_TYPE {"InternalState"};
    };

    /**
     * Listener interface for state changes
     * Components can implement this to receive notifications when internal state changes
     */
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void internalStateChanged() = 0;
    };

    InternalState();
    ~InternalState() override;

    // ========================================
    // LISTENER MANAGEMENT
    // ========================================
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // ========================================
    // STEP NOTES
    // ========================================
    int getStepNote(int stepIndex, int substepIndex) const;
    void setStepNote(int stepIndex, int substepIndex, int noteNumber);

    int getStepNotesLength(int stepIndex) const;
    void setStepNotesLength(int stepIndex, int length);

    int getStepNotesCurrentIndex(int stepIndex) const;
    void setStepNotesCurrentIndex(int stepIndex, int index);

    // ========================================
    // STEP MODULATIONS
    // ========================================
    int getStepModType(int stepIndex, int modIndex) const;
    void setStepModType(int stepIndex, int modIndex, int type);

    int getStepModCC(int stepIndex, int modIndex) const;
    void setStepModCC(int stepIndex, int modIndex, int cc);

    int getStepModValue(int stepIndex, int modIndex) const;
    void setStepModValue(int stepIndex, int modIndex, int value);

    bool getStepModSelected(int stepIndex, int modIndex) const;
    void setStepModSelected(int stepIndex, int modIndex, bool selected);

    // ========================================
    // SEQUENCE METADATA
    // ========================================
    int getCurrentIndex() const;
    void setCurrentIndex(int index);

    int getLastProbabilityIndex() const;
    void setLastProbabilityIndex(int index);

    int getViewWindowStart() const;
    void setViewWindowStart(int start);

    // ========================================
    // PLAY PROBABILITY
    // ========================================
    bool getPlayProbability(int index) const;
    void setPlayProbability(int index, bool enabled);

    // ========================================
    // NOTE COLOURS
    // ========================================
    juce::Colour getNoteColour(int chromaticNote) const;
    void setNoteColour(int chromaticNote, juce::Colour colour);

    // ========================================
    // SERIALIZATION
    // ========================================
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);

    // ========================================
    // INITIALIZATION
    // ========================================
    void initializeDefaults();

private:
    // ValueTree::Listener implementation
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    void notifyListeners();

    juce::ValueTree tree;
    juce::ListenerList<Listener> listeners;

    // Default note colours (same as in Params.h)
    static constexpr int defaultNoteColours[12] = {
        static_cast<int>(0xFFFF6B6B),  // C  - Red
        static_cast<int>(0xFFFF8E53),  // C# - Orange-Red
        static_cast<int>(0xFFFFD93D),  // D  - Yellow
        static_cast<int>(0xFF6BCB77),  // D# - Green
        static_cast<int>(0xFF4ECDC4),  // E  - Teal
        static_cast<int>(0xFF45B7D1),  // F  - Cyan
        static_cast<int>(0xFF5C7AEA),  // F# - Blue
        static_cast<int>(0xFF9B59B6),  // G  - Purple
        static_cast<int>(0xFFE056FD),  // G# - Magenta
        static_cast<int>(0xFFFF6B9D),  // A  - Pink
        static_cast<int>(0xFFFF9F43),  // A# - Orange
        static_cast<int>(0xFFFECA57)   // B  - Gold
    };
};


