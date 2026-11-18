#pragma once

#include "../backend/InternalState.h"
#include "../backend/MidiMapping/MidiControlId.h"
#include "../data/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <functional>
#include <vector>

class PluginProcessor;

/**
 * UiBridge - the one seam between the sequencer and its UI.
 *
 * The UI (a web page) knows nothing about APVTS, InternalState or threads. It
 * receives events and sends actions; the names and payloads are the contract
 * documented at the top of web/js/bridge.js.
 *
 *   engine -> UI   onEvent("state" | "tick" | "noteOff" | "toast", payload)
 *   UI -> engine   handleAction(name, payload)
 *
 * Everything here runs on the message thread. The audio thread only sets
 * atomic flags, which a timer picks up; that timer also drains the notes
 * Roll mode generates on the audio thread, so they are stored even while
 * no editor is open.
 */
class UiBridge : private juce::Timer,
                 private juce::AudioProcessorValueTreeState::Listener,
                 private InternalState::Listener {
public:
    explicit UiBridge (PluginProcessor&);
    ~UiBridge() override;

    /** Set by the editor while it is open. */
    std::function<void (const juce::Identifier& event, const juce::var& payload)> onEvent;

    /** Exporting needs a native file dialog, so the editor provides it. */
    std::function<void()> onExportRequested;

    void handleAction (const juce::String& action, const juce::var& payload);

    /** The full state the UI renders from. */
    juce::var buildSnapshot() const;

    /** Deliver anything pending now. The timer calls this; tests can call it directly. */
    void flush();

    bool canUndo() const { return ! undoStack.empty(); }

private:
    void timerCallback() override { flush(); }
    void parameterChanged (const juce::String&, float) override { dirty = true; }
    void internalStateChanged() override { dirty = true; }

    void emit (const juce::Identifier& event, const juce::var& payload) const;
    void toast (const juce::String& text) const;
    /** Opens a pre-filled GitHub bug report, or copies its link where a plugin can't open URLs (AUv3). */
    void reportBug() const;

    // actions
    void setParam (const juce::String& id, const juce::var& value);
    void setFloatParam (const juce::String& paramId, float value);
    void pulseParam (const juce::String& paramId);
    void previewSelected (int noteOverride = -1);
    void clearAll();
    void paintProbabilityCell (int cell, bool on);
    void loadPreset (const juce::String& name);
    void cyclePreset (int direction);

    // undo: step notes + sequence length, captured before each destructive edit
    struct StepData {
        std::array<int, ParamConstants::MAX_SUBSTEPS> notes {};
        int count = 1;
    };
    struct UndoEntry {
        int length = 0;
        std::array<StepData, ParamConstants::MAX_STEPS> steps {};
    };
    void pushUndo();
    void undo();

    PluginProcessor& processor;
    std::vector<UndoEntry> undoStack;

    std::atomic<bool> dirty { true };
    std::atomic<bool> tickPending { false };
    std::atomic<bool> noteOffPending { false };
    std::atomic<int> presetCyclePending { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UiBridge)
};
