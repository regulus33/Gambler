/**
 * E2E-style Tests for Gambler
 *
 * These tests simulate user interactions by directly calling component methods
 * and verifying state changes through the APVTS-backed SequenceState.
 *
 * Pattern:
 * 1. Create PluginProcessor (owns APVTS and all state)
 * 2. Create PluginEditor (owns UI component hierarchy)
 * 3. Navigate to components via getters and call their public methods/callbacks
 * 4. Verify state changes via processor.getState()
 *
 * Note: JUCE MessageManager must be initialized for UI components to work.
 * The test framework handles this via GamblerTestInitialiser.
 */

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
#include "helpers/test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * RAII helper to initialize JUCE's MessageManager for GUI tests.
 * This is required for any test that creates UI components.
 */
class GamblerTestInitialiser {
public:
    GamblerTestInitialiser() {
        juce::MessageManager::getInstance();
    }
    ~GamblerTestInitialiser() {
        juce::MessageManager::deleteInstance();
    }
};

/**
 * Test fixture that provides a fully initialized plugin with UI.
 * Use this as the base for E2E-style tests.
 */
class PluginTestFixture {
public:
    PluginTestFixture()
        : editor (processor) {
        // Give components a chance to initialize
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }

    ~PluginTestFixture() {
        // Process any pending messages before destruction
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }

    // Access the processor (state, clock, etc.)
    PluginProcessor& getProcessor() { return processor; }

    // Access the editor (UI hierarchy)
    PluginEditor& getEditor() { return editor; }

    // Convenience: get the APVTS-backed state
    SequenceState& getState() { return processor.getState(); }

    // Convenience: get the main screen component
    MainScreenComponent& getMainScreen() {
        return editor.getApplicationComponent().getMainScreen();
    }

    // Helper to process pending messages (simulates event loop)
    void processMessages (int timeoutMs = 50) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (timeoutMs);
    }

    // Helper to simulate a full "tick" cycle (process audio block)
    void simulateAudioBlock (int numSamples = 512) {
        juce::AudioBuffer<float> buffer (2, numSamples);
        juce::MidiBuffer midiBuffer;
        processor.processBlock (buffer, midiBuffer);
    }

private:
    PluginProcessor processor;
    PluginEditor editor;
};

// ============================================================================
// E2E Test Scenarios
// ============================================================================

SCENARIO ("User clicks piano keys to modify scale", "[e2e][piano-roll]") {
    GamblerTestInitialiser juceInit;

    GIVEN ("A fresh plugin with default chromatic scale") {
        PluginTestFixture fixture;
        auto& state = fixture.getState();
        auto& mainScreen = fixture.getMainScreen();
        auto& pianoRoll = mainScreen.getPianoRoll();

        // All 12 notes should be enabled by default (chromatic scale)
        REQUIRE (pianoRoll.getSelectedNoteIndices().size() == 12);

        WHEN ("User clicks C# to disable it") {
            bool wasToggled = pianoRoll.TEST_simulateKeyClick(1);  // Db/C# is index 1
            fixture.processMessages();

            THEN ("C# is disabled and scale is updated in state") {
                REQUIRE (wasToggled == true);

                // Check UI state
                auto* dbKey = pianoRoll.getKeyData(1);
                REQUIRE (dbKey != nullptr);
                REQUIRE (dbKey->isToggled == false);
                REQUIRE (pianoRoll.getSelectedNoteIndices().size() == 11);

                // Check backend state (APVTS) - scale is now per-step
                auto scaleNotes = state.getStepScaleNotes(0);
                REQUIRE (scaleNotes[1] == 0);  // C# disabled
            }
        }

        WHEN ("User creates a C major scale by disabling all black keys") {
            // Disable: Db, Eb, Gb, Ab, Bb (indices 1, 3, 6, 8, 10)
            pianoRoll.TEST_simulateKeyClick(1);
            pianoRoll.TEST_simulateKeyClick(3);
            pianoRoll.TEST_simulateKeyClick(6);
            pianoRoll.TEST_simulateKeyClick(8);
            pianoRoll.TEST_simulateKeyClick(10);
            fixture.processMessages();

            THEN ("Only white keys remain enabled") {
                auto selectedNames = pianoRoll.getSelectedNoteNames();
                REQUIRE (selectedNames.size() == 7);

                // White keys should be present
                REQUIRE (selectedNames.contains("C"));
                REQUIRE (selectedNames.contains("D"));
                REQUIRE (selectedNames.contains("E"));
                REQUIRE (selectedNames.contains("F"));
                REQUIRE (selectedNames.contains("G"));
                REQUIRE (selectedNames.contains("A"));
                REQUIRE (selectedNames.contains("B"));

                // Black keys should not be present
                REQUIRE_FALSE (selectedNames.contains("Db"));
                REQUIRE_FALSE (selectedNames.contains("Eb"));
                REQUIRE_FALSE (selectedNames.contains("Gb"));
                REQUIRE_FALSE (selectedNames.contains("Ab"));
                REQUIRE_FALSE (selectedNames.contains("Bb"));

                // Backend state should match - scale is now per-step
                auto scaleNotes = state.getStepScaleNotes(0);
                REQUIRE (scaleNotes[0] == 1);   // C
                REQUIRE (scaleNotes[1] == 0);   // Db
                REQUIRE (scaleNotes[2] == 1);   // D
                REQUIRE (scaleNotes[3] == 0);   // Eb
                REQUIRE (scaleNotes[4] == 1);   // E
                REQUIRE (scaleNotes[5] == 1);   // F
                REQUIRE (scaleNotes[6] == 0);   // Gb
                REQUIRE (scaleNotes[7] == 1);   // G
                REQUIRE (scaleNotes[8] == 0);   // Ab
                REQUIRE (scaleNotes[9] == 1);   // A
                REQUIRE (scaleNotes[10] == 0);  // Bb
                REQUIRE (scaleNotes[11] == 1);  // B
            }
        }

        WHEN ("User disables all notes including the last one") {
            // Disable all notes except C
            for (int i = 1; i < 12; i++) {
                pianoRoll.TEST_simulateKeyClick(i);
            }
            fixture.processMessages();

            // Verify only C remains
            REQUIRE (pianoRoll.getSelectedNoteIndices().size() == 1);
            REQUIRE (pianoRoll.getKeyData(0)->isToggled == true);

            // Disable C too - the piano roll allows this
            bool wasToggled = pianoRoll.TEST_simulateKeyClick(0);
            fixture.processMessages();

            THEN ("C is toggled off") {
                REQUIRE (wasToggled == true);
                REQUIRE (pianoRoll.getKeyData(0)->isToggled == false);
                REQUIRE (pianoRoll.getSelectedNoteIndices().size() == 0);
            }
        }
    }
}

SCENARIO ("User clicks a step button and state updates", "[e2e][steps]") {
    GamblerTestInitialiser juceInit;
    // Plugin
    GIVEN ("A fresh plugin instance") {
        PluginTestFixture fixture;
        auto& state = fixture.getState();
        auto& mainScreen = fixture.getMainScreen();

        // Initial state: selected step should be 0
        REQUIRE (state.getSelectedStepForEditing() == 0);

        WHEN ("User clicks step button at index 3") {
            // Get the actual step button component from the UI hierarchy
            auto& sequencerSteps = mainScreen.getSequencerSteps();
            auto* stepButton = sequencerSteps.getStepButton (3); // Local index 3 = absolute step 3 (window starts at 0)
            REQUIRE (stepButton != nullptr);

            // Simulate a click by invoking the button's onStepClicked callback
            // This is what mouseDown() does internally after checking for MIDI learn mode
            // The callback is wired up in SequencerStepsComponent to forward to MainScreenComponent
            REQUIRE (stepButton->onStepClicked != nullptr);
            stepButton->onStepClicked (stepButton->getAbsoluteStepIndex());

            // Process messages to let async updates propagate
            fixture.processMessages();

            THEN ("The selected step for editing should be 3") {
                // The click flows through: StepButton.onStepClicked callback
                // -> SequencerStepsComponent.onStepClicked -> MainScreenComponent.setupSequencerCallbacks
                // -> processorRef.triggerStepMidi(stepIndex) -> state.setSelectedStepForEditing(stepIndex)
                REQUIRE (state.getSelectedStepForEditing() == 3);
            }
        }
    }
}

SCENARIO ("User changes note via note controls", "[e2e][notes]") {
    GamblerTestInitialiser juceInit;

    GIVEN ("A plugin with step 0 selected") {
        PluginTestFixture fixture;
        auto& processor = fixture.getProcessor();
        auto& state = processor.getState();
        auto& mainScreen = fixture.getMainScreen();

        // Ensure step 0 is selected and set initial note to C5 (MIDI 60)
        state.setSelectedStepForEditing (0);
        state.setStepNote (0, 0, 60);
        fixture.processMessages();

        int initialNote = state.getCurrentNoteForStep (0);
        int initialOctave = initialNote / 12;
        INFO ("Initial note: " << initialNote << " (octave " << initialOctave << ")");

        WHEN ("User changes the note slider to E (index 4)") {
            // Get the actual note slider from the UI hierarchy
            auto& noteControls = mainScreen.getMainControls().getNoteControls();

            // Set the note slider value - this triggers onValueChange -> onNoteChanged callback
            // which flows through MainScreenComponent to update the state
            noteControls.getNoteSlider().getSlider().getSlider().setValue (4.0, juce::sendNotification);
            fixture.processMessages();

            THEN ("The note should be E at the same octave") {
                int resultNote = state.getCurrentNoteForStep (0);
                REQUIRE (resultNote % 12 == 4); // Pitch class E
                REQUIRE (resultNote / 12 == initialOctave); // Same octave
            }
        }

        WHEN ("User changes the octave slider to octave 6") {
            auto& noteControls = mainScreen.getMainControls().getNoteControls();
            int initialPitchClass = initialNote % 12;

            // Set the octave slider value - this triggers onValueChange -> onOctaveChanged callback
            noteControls.getOctaveSlider().getSlider().getSlider().setValue (6.0, juce::sendNotification);
            fixture.processMessages();

            THEN ("The note should be at octave 6 with the same pitch class") {
                int resultNote = state.getCurrentNoteForStep (0);
                REQUIRE (resultNote / 12 == 6); // Octave 6
                REQUIRE (resultNote % 12 == initialPitchClass); // Same pitch class
            }
        }
    }
}

SCENARIO ("User toggles play/pause", "[e2e][transport]") {
    GamblerTestInitialiser juceInit;

    GIVEN ("A playing sequencer (default state)") {
        PluginTestFixture fixture;
        auto& processor = fixture.getProcessor();

        // Default state is Playing
        REQUIRE (processor.getSequencerState() == PluginProcessor::SequencerState::Playing);

        WHEN ("User clicks pause button") {
            processor.togglePlayPause();
            fixture.processMessages();

            THEN ("Sequencer should be paused") {
                REQUIRE (processor.getSequencerState() == PluginProcessor::SequencerState::Paused);
            }

            AND_WHEN ("User clicks play button again") {
                processor.togglePlayPause();
                fixture.processMessages();

                THEN ("Sequencer should resume playing") {
                    REQUIRE (processor.getSequencerState() == PluginProcessor::SequencerState::Playing);
                }
            }
        }
    }
}

// ============================================================================
// NEW TEST: Sequence Length Change with Visual Snapshot
// ============================================================================
//
// This test demonstrates:
// 1. How to simulate a button click via APVTS parameter
// 2. How to verify state changes
// 3. How to use snapshot testing for visual regression
//
// The mental model:
//
//   User clicks "Length +" button
//            ↓
//   Button's mouseDown() sets LENGTH_PLUS param to 1.0
//            ↓
//   PluginProcessor::parameterChanged() receives callback
//            ↓
//   Processor calls state.increaseLength()
//            ↓
//   SequenceState updates SEQ_LENGTH param
//            ↓
//   UI components listening to SEQ_LENGTH repaint
//            ↓
//   We verify: state changed AND visual changed
//
// ============================================================================

SCENARIO ("User clicks Length+ and sequence grows", "[e2e][length]") {
    // ========================================================================
    // LINE 1: Initialize JUCE's MessageManager
    // ========================================================================
    // JUCE UI components need the MessageManager to exist. This RAII class
    // creates it on construction and destroys it when the test ends.
    // Without this, any UI component creation will crash.
    GamblerTestInitialiser juceInit;

    GIVEN ("A plugin with default sequence length of 8") {
        // ====================================================================
        // LINE 2: Create the test fixture
        // ====================================================================
        // PluginTestFixture creates:
        //   - A PluginProcessor (owns all state, APVTS, audio processing)
        //   - A PluginEditor (owns all UI components)
        // This mimics what happens when a DAW loads the plugin.
        PluginTestFixture fixture;

        // ====================================================================
        // LINE 3: Get references to things we'll interact with
        // ====================================================================
        // We grab references to:
        //   - processor: to access APVTS parameters (simulating button clicks)
        //   - state: to verify state changes happened
        //   - mainScreen: to access UI components for snapshots
        auto& processor = fixture.getProcessor();
        auto& state = fixture.getState();
        auto& mainScreen = fixture.getMainScreen();

        // ====================================================================
        // LINE 4: Verify our starting assumption
        // ====================================================================
        // Always verify the precondition! If default length isn't 8,
        // our test logic would be wrong.
        int initialLength = state.getLength();
        REQUIRE (initialLength == 8);  // Default sequence length

        WHEN ("User clicks the Length+ button") {
            // ================================================================
            // LINE 5: Take a "before" snapshot for visual comparison
            // ================================================================
            // We snapshot the sequence overview grid (8x8 cells showing all 64 steps).
            // This grid visually shows which steps are "active" (within length).
            // After increasing length, more cells should be highlighted.
            SnapshotTester snapshot ("length_plus_sequence_overview");
            auto& sequenceOverview = mainScreen.getSequenceOverview();

            // ================================================================
            // LINE 6: Simulate the button click via APVTS parameter
            // ================================================================
            // Instead of calling button.mouseDown(), we set the parameter directly.
            // This is actually MORE realistic because:
            //   - It tests the same code path as MIDI controller input
            //   - It tests the same code path as DAW automation
            //   - The button's mouseDown() does exactly this internally
            //
            // getParameter() returns a RangedAudioParameter*
            // setValueNotifyingHost(1.0f) = "button pressed" (true)
            // setValueNotifyingHost(0.0f) = "button released" (false)
            auto* lengthPlusParam = processor.getAPVTS().getParameter (
                Params::Keys::ID::LENGTH_PLUS
            );
            REQUIRE (lengthPlusParam != nullptr);  // Param must exist

            // Simulate press (this triggers the length increase)
            lengthPlusParam->setValueNotifyingHost (1.0f);

            // Simulate release (good practice, matches real button behavior)
            lengthPlusParam->setValueNotifyingHost (0.0f);

            // ================================================================
            // LINE 7: Process pending messages
            // ================================================================
            // JUCE uses async message passing for UI updates. When a parameter
            // changes, it posts a message to repaint components. We need to
            // "pump" the message loop to let those updates happen.
            // 50ms is usually enough for UI to settle.
            fixture.processMessages (50);

            THEN ("The sequence length should be 9") {
                // ============================================================
                // LINE 8: Verify the state change
                // ============================================================
                // This is the primary assertion: did the length actually increase?
                REQUIRE (state.getLength() == initialLength + 1);
            }

            AND_THEN ("The sequence overview grid should visually update") {
                // ============================================================
                // LINE 9: Take "after" snapshot and compare
                // ============================================================
                // Capture the current visual state of the component.
                // The grid should now show 9 active steps instead of 8.
                snapshot.capture (sequenceOverview);

                // Compare against the golden reference image.
                // First run: creates the golden (test passes)
                // Later runs: compares against golden (fails if different)
                bool visuallyCorrect = snapshot.matches();

                if (!visuallyCorrect) {
                    // Save debug artifacts when test fails
                    snapshot.saveFailureArtifacts();
                    INFO ("Snapshot mismatch! Check tests/snapshots/ for diff images");
                    INFO ("Pixels different: " << snapshot.getDifferenceCount());
                }

                REQUIRE (visuallyCorrect);
            }
        }
    }
}
