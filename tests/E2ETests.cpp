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
            noteControls.getNoteSlider().getSlider().setValue (4.0, juce::sendNotification);
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
            noteControls.getOctaveSlider().getSlider().setValue (6.0, juce::sendNotification);
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
