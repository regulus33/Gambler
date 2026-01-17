#pragma once

#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>

/**
 * StateListenerComponent - Base class for UI components that observe APVTS state
 *
 * This class provides:
 * 1. Automatic APVTS parameter listening with thread-safe UI updates
 * 2. Batched repaints via AsyncUpdater (multiple rapid changes = one repaint)
 * 3. Easy subscription to parameter groups (e.g., all step notes, all scale notes)
 *
 * Usage:
 *   class MyComponent : public StateListenerComponent {
 *   public:
 *       MyComponent(PluginProcessor& p) : StateListenerComponent(p) {
 *           // Subscribe to parameters you care about
 *           listenToParameter(Params::Keys::ID::SEQUENCE_LENGTH);
 *           listenToStepNotes(0, 7);  // Listen to steps 0-7
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
 * - onStateChanged() is ALWAYS called on the message thread via AsyncUpdater
 * - Multiple rapid parameter changes are coalesced into a single onStateChanged() call
 */
class StateListenerComponent : public juce::Component,
                                public juce::AsyncUpdater,
                                private juce::AudioProcessorValueTreeState::Listener {
public:
    explicit StateListenerComponent(PluginProcessor& processor)
        : processorRef(processor) {}

    ~StateListenerComponent() override {
        // Unsubscribe from all parameters
        for (const auto& paramId : subscribedParameters) {
            processorRef.getAPVTS().removeParameterListener(paramId, this);
        }
    }

protected:
    // ========================================
    // SUBSCRIPTION METHODS - Call in constructor
    // ========================================

    /**
     * Listen to a single parameter by ID
     */
    void listenToParameter(const juce::String& paramId) {
        processorRef.getAPVTS().addParameterListener(paramId, this);
        subscribedParameters.push_back(paramId);
    }

    /**
     * Listen to all step note parameters for a range of steps
     * @param startStep First step index (0-63)
     * @param endStep Last step index (inclusive, 0-63)
     */
    void listenToStepNotes(int startStep, int endStep) {
        for (int step = startStep; step <= endStep && step < ParamConstants::MAX_STEPS; step++) {
            for (int substep = 0; substep < ParamConstants::MAX_SUBSTEPS; substep++) {
                listenToParameter(Params::Keys::ID::stepNote(step, substep));
            }
        }
    }

    /**
     * Listen to all step notes length parameters for a range of steps
     */
    void listenToStepNotesLength(int startStep, int endStep) {
        for (int step = startStep; step <= endStep && step < ParamConstants::MAX_STEPS; step++) {
            listenToParameter(Params::Keys::ID::stepNotesLength(step));
        }
    }

    /**
     * Listen to all modulation parameters for a range of steps
     */
    void listenToModulations(int startStep, int endStep) {
        for (int step = startStep; step <= endStep && step < ParamConstants::MAX_STEPS; step++) {
            for (int mod = 0; mod < ParamConstants::NUM_MODULATIONS; mod++) {
                listenToParameter(Params::Keys::ID::stepModType(step, mod));
                listenToParameter(Params::Keys::ID::stepModCCNum(step, mod));
                listenToParameter(Params::Keys::ID::stepModValue(step, mod));
                listenToParameter(Params::Keys::ID::stepModSelected(step, mod));
            }
        }
    }

    /**
     * Listen to all scale parameters (notes + range)
     */
    void listenToScale() {
        for (int i = 0; i < ParamConstants::NUM_SCALE_NOTES; i++) {
            listenToParameter(Params::Keys::ID::scaleNote(i));
        }
        listenToParameter(Params::Keys::ID::SCALE_RANGE_MIN);
        listenToParameter(Params::Keys::ID::SCALE_RANGE_MAX);
    }

    /**
     * Listen to scale range parameters only (min/max)
     */
    void listenToScaleRange() {
        listenToParameter(Params::Keys::ID::SCALE_RANGE_MIN);
        listenToParameter(Params::Keys::ID::SCALE_RANGE_MAX);
    }

    /**
     * Listen to sequence metadata parameters
     */
    void listenToSequenceMetadata() {
        listenToParameter(Params::Keys::ID::SEQ_LENGTH);
        listenToParameter(Params::Keys::ID::SEQ_CURRENT_INDEX);
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);
        listenToParameter(Params::Keys::ID::SEQ_VIEW_WINDOW_START);
    }

    /**
     * Listen to all play probability parameters
     */
    void listenToPlayProbability() {
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            listenToParameter(Params::Keys::ID::playProbability(i));
        }
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
    void parameterChanged(const juce::String& parameterID, float newValue) override {
        // This may be called from ANY thread (audio, message, etc.)
        // Use AsyncUpdater to safely marshal to message thread
        // Multiple rapid calls will be coalesced into one handleAsyncUpdate()
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateListenerComponent)
};

