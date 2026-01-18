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

#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../source/PluginProcessor.h"
#include "../source/PluginEditor.h"

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
        : editor(processor)
    {
        // Give components a chance to initialize
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }

    ~PluginTestFixture() {
        // Process any pending messages before destruction
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
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
    void processMessages(int timeoutMs = 50) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(timeoutMs);
    }

    // Helper to simulate a full "tick" cycle (process audio block)
    void simulateAudioBlock(int numSamples = 512) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);
    }

private:
    PluginProcessor processor;
    PluginEditor editor;
};

// ============================================================================
// E2E Test Scenarios
// ============================================================================

SCENARIO("User clicks a step button and state updates", "[e2e][steps]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A fresh plugin instance") {
        PluginTestFixture fixture;
        auto& state = fixture.getState();

        // Initial state: selected step should be 0
        REQUIRE(state.getSelectedStepForEditing() == 0);

        WHEN("User clicks step button at index 3") {
            // Get the sequencer steps component
            auto& mainScreen = fixture.getMainScreen();
            // The step buttons are inside SequencerStepsComponent
            // We simulate the click by calling the callback directly

            // First, we need to access the step button's click handler
            // In the real UI, clicking triggers onStepClicked callback
            // which calls processorRef.triggerStepMidi(stepIndex)

            // Simulate what happens when step 3 is clicked:
            fixture.getProcessor().triggerStepMidi(3);

            // Process messages to let async updates propagate
            fixture.processMessages();

            THEN("The selected step for editing should be 3") {
                // Note: triggerStepMidi sets the selected step
                REQUIRE(state.getSelectedStepForEditing() == 3);
            }
        }
    }
}

SCENARIO("User changes note via note controls", "[e2e][notes]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A plugin with step 0 selected") {
        PluginTestFixture fixture;
        auto& state = fixture.getState();

        // Set initial note to C4 (MIDI 60)
        state.setStepNote(0, 0, 60);
        REQUIRE(state.getStepNote(0, 0) == 60);

        WHEN("User changes the note to E (pitch class 4) via note controls callback") {
            // Simulate what the NoteControlsComponent callback does:
            // It calculates new note = currentOctave * 12 + pitchClass
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int currentOctave = currentNote / 12;  // 60 / 12 = 5
            int newPitchClass = 4;  // E
            int newNote = currentOctave * 12 + newPitchClass;  // 5 * 12 + 4 = 64

            state.updateCurrentNoteForStep(selectedStep, newNote);

            fixture.processMessages();

            THEN("The note should be E4 (MIDI 64)") {
                REQUIRE(state.getCurrentNoteForStep(0) == 64);
            }
        }
    }
}

SCENARIO("User toggles play/pause", "[e2e][transport]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A playing sequencer (default state)") {
        PluginTestFixture fixture;
        auto& processor = fixture.getProcessor();

        // Default state is Playing
        REQUIRE(processor.getSequencerState() == PluginProcessor::SequencerState::Playing);

        WHEN("User clicks pause button") {
            processor.togglePlayPause();
            fixture.processMessages();

            THEN("Sequencer should be paused") {
                REQUIRE(processor.getSequencerState() == PluginProcessor::SequencerState::Paused);
            }

            AND_WHEN("User clicks play button again") {
                processor.togglePlayPause();
                fixture.processMessages();

                THEN("Sequencer should resume playing") {
                    REQUIRE(processor.getSequencerState() == PluginProcessor::SequencerState::Playing);
                }
            }
        }
    }
}

