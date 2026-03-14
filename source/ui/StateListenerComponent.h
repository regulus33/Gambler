#pragma once

#include "../PluginProcessor.h"
#include "../backend/InternalState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>

/**
 * StateListenerComponent - Base class for UI components that observe state
 *
 * This class provides:
 * 1. Automatic APVTS parameter listening with thread-safe UI updates
 * 2. Automatic InternalState listening for non-MIDI-mappable state
 * 3. Batched repaints via AsyncUpdater (multiple rapid changes = one repaint)
 * 4. Easy subscription to parameter groups (e.g., all step notes, all scale notes)
 *
 * Usage:
 *   class MyComponent : public StateListenerComponent {
 *   public:
 *       MyComponent(PluginProcessor& p) : StateListenerComponent(p) {
 *           // Subscribe to APVTS parameters you care about
 *           listenToParameter(Params::Keys::ID::SEQUENCE_LENGTH);
 *           // Subscribe to InternalState changes
 *           listenToInternalState();
 *       }
 *
 *   protected:
 *       void onStateChanged() override {
 *           // Called on message thread when any subscribed parameter changes
 *           // Read from getState() and update your UI
 *           repaint();
 *       }
 *   };
 *
 * Thread Safety:
 * - parameterChanged() may be called from ANY thread (audio, message, etc.)
 * - internalStateChanged() may be called from ANY thread
 * - onStateChanged() is ALWAYS called on the message thread via AsyncUpdater
 * - Multiple rapid parameter changes are coalesced into a single onStateChanged() call
 */
class StateListenerComponent : public juce::Component,
                                public juce::AsyncUpdater,
                                private juce::AudioProcessorValueTreeState::Listener,
                                private InternalState::Listener {
public:
    explicit StateListenerComponent(PluginProcessor& processor)
        : processorRef(processor) {}

    ~StateListenerComponent() override {
        // Unsubscribe from all APVTS parameters
        for (const auto& paramId : subscribedParameters) {
            processorRef.getAPVTS().removeParameterListener(paramId, this);
        }
        // Unsubscribe from InternalState
        if (listeningToInternalState) {
            processorRef.getInternalState().removeListener(this);
        }
    }

protected:
    // ========================================
    // SUBSCRIPTION METHODS - Call in constructor
    // ========================================

    /**
     * Listen to a single APVTS parameter by ID
     */
    void listenToParameter(const juce::String& paramId) {
        // Only add listener if the parameter exists in APVTS
        if (processorRef.getAPVTS().getParameter(paramId) != nullptr) {
            processorRef.getAPVTS().addParameterListener(paramId, this);
            subscribedParameters.push_back(paramId);
        }
    }

    /**
     * Listen to InternalState changes (step notes, modulations, play probability, etc.)
     * Call this if your component needs to react to any InternalState changes.
     */
    void listenToInternalState() {
        if (!listeningToInternalState) {
            processorRef.getInternalState().addListener(this);
            listeningToInternalState = true;
        }
    }

    /**
     * Listen to all step note parameters for a range of steps
     * NOTE: Step notes are now in InternalState, so this just enables InternalState listening
     * @param startStep First step index (0-63)
     * @param endStep Last step index (inclusive, 0-63)
     */
    void listenToStepNotes([[maybe_unused]] int startStep, [[maybe_unused]] int endStep) {
        // Step notes are now in InternalState
        listenToInternalState();
    }

    /**
     * Listen to all step notes length parameters for a range of steps
     * NOTE: Step notes length is now in InternalState
     */
    void listenToStepNotesLength([[maybe_unused]] int startStep, [[maybe_unused]] int endStep) {
        // Step notes length is now in InternalState
        listenToInternalState();
    }

    /**
     * Listen to all modulation parameters for a range of steps
     * NOTE: Modulations are now in InternalState
     */
    void listenToModulations([[maybe_unused]] int startStep, [[maybe_unused]] int endStep) {
        // Modulations are now in InternalState
        listenToInternalState();
    }

    /**
     * Listen to scale window parameters (offset/size)
     */
    void listenToScaleRange() {
        listenToParameter(Params::Keys::ID::SCALE_WINDOW_OFFSET);
        listenToParameter(Params::Keys::ID::SCALE_WINDOW_SIZE);
    }

    /**
     * Listen to all global scale note parameters
     */
    void listenToGlobalScaleNotes() {
        for (int note = 0; note < ParamConstants::NUM_SCALE_NOTES; note++) {
            listenToParameter(Params::Keys::ID::globalScaleNote(note));
        }
    }

    /**
     * Listen to sequence metadata parameters
     * NOTE: SEQ_CURRENT_INDEX and SEQ_VIEW_WINDOW_START are now in InternalState
     */
    void listenToSequenceMetadata() {
        listenToParameter(Params::Keys::ID::SEQ_LENGTH);
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);
        // SEQ_CURRENT_INDEX and SEQ_VIEW_WINDOW_START are now in InternalState
        listenToInternalState();
    }

    /**
     * Listen to all play probability parameters
     * NOTE: Play probability is now in InternalState
     */
    void listenToPlayProbability() {
        // Play probability is now in InternalState
        listenToInternalState();
    }

    // ========================================
    // OVERRIDE THIS - Called when state changes
    // ========================================

    /**
     * Called on the message thread when any subscribed parameter changes.
     * Override this to update your UI based on the new state.
     *
     * Read state using: processorRef.getState()
     */
    virtual void onStateChanged() = 0;

    // ========================================
    // ACCESS TO PROCESSOR
    // ========================================

    PluginProcessor& getProcessor() { return processorRef; }
    const PluginProcessor& getProcessor() const { return processorRef; }

    SequenceState& getState() { return processorRef.getState(); }
    const SequenceState& getState() const { return processorRef.getState(); }

private:
    // AudioProcessorValueTreeState::Listener implementation
    void parameterChanged(const juce::String& /*parameterID*/, float /*newValue*/) override {
        // This may be called from ANY thread (audio, message, etc.)
        // Use AsyncUpdater to safely marshal to message thread
        // Multiple rapid calls will be coalesced into one handleAsyncUpdate()
        triggerAsyncUpdate();
    }

    // InternalState::Listener implementation
    void internalStateChanged() override {
        // This may be called from ANY thread
        // Use AsyncUpdater to safely marshal to message thread
        triggerAsyncUpdate();
    }

    // AsyncUpdater implementation
    void handleAsyncUpdate() override {
        // This is called on the message thread
        // Safe to update UI here
        onStateChanged();
    }

    PluginProcessor& processorRef;
    std::vector<juce::String> subscribedParameters;
    bool listeningToInternalState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateListenerComponent)
};

