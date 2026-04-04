#pragma once

/**
 * TestApp - High-level testing interface that mirrors the UI component hierarchy.
 *
 * Usage:
 *   TestApp app;
 *   app.navBar().clickPlayPause();
 *   app.sequencerControls().rollButton().press();
 *   app.sequencerControls().rollButton().release();
 *   REQUIRE(app.state().getIsRandom() == false);
 *
 * Each nested accessor returns a lightweight proxy that holds a reference
 * to the real component and exposes user-level actions (click, press, release, set, etc.).
 * The proxy never outlives the TestApp that created it.
 */

#include "test_helpers.h"
#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <backend/MidiLearnRegistry.h>
#include <backend/MidiMapping.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declarations for component types
class SequencerControlsComponent;
class NavBarComponent;
class PianoRollComponent;
class SequencerStepsComponent;
class MainControlsComponent;
class SubsequenceControlsComponent;
class SequenceOverviewComponent;
class NoteRangeControlsComponent;
class ApplicationComponent;

// ============================================================================
// Helper: simulate a mouse click on a JUCE component
// ============================================================================
namespace TestMouse {

    inline juce::MouseEvent makeEvent(juce::Component& comp, juce::Point<int> pos = {5, 5},
                                       int numClicks = 1) {
        auto source = juce::Desktop::getInstance().getMainMouseSource();
        return juce::MouseEvent(
            source,
            pos.toFloat(),
            juce::ModifierKeys(),
            0.0f, // pressure
            0.0f, // orientation
            0.0f, // rotation
            0.0f, // tiltX
            0.0f, // tiltY
            &comp,
            &comp,
            juce::Time::getCurrentTime(),
            pos.toFloat(), // mouseDownPos
            juce::Time::getCurrentTime(),
            numClicks,
            false // mouseWasDragged
        );
    }

    inline void click(juce::Component& comp) {
        auto e = makeEvent(comp);
        comp.mouseDown(e);
        comp.mouseUp(e);
    }

    inline void press(juce::Component& comp) {
        auto e = makeEvent(comp);
        comp.mouseDown(e);
    }

    inline void release(juce::Component& comp) {
        auto e = makeEvent(comp);
        comp.mouseUp(e);
    }

    inline void doubleClick(juce::Component& comp) {
        auto e = makeEvent(comp, {5, 5}, 2);
        comp.mouseDoubleClick(e);
    }

} // namespace TestMouse

// ============================================================================
// Proxy: SequencerControlButton wrapper (Roll, Shift, PageLeft, etc.)
// ============================================================================
class ButtonProxy {
public:
    explicit ButtonProxy(juce::Component& button) : button_(button) {}

    void click()   { TestMouse::click(button_); }
    void press()   { TestMouse::press(button_); }
    void release() { TestMouse::release(button_); }
    bool isVisible() const { return button_.isVisible(); }

    juce::Component& raw() { return button_; }

private:
    juce::Component& button_;
};

// ============================================================================
// Proxy: SequencerControlsComponent
// ============================================================================
class SequencerControlsProxy {
public:
    explicit SequencerControlsProxy(SequencerControlsComponent& comp) : comp_(comp) {}

    // Buttons are private in the real component, but we can access them
    // through the component's children by index (layout order matches slot order).
    // Alternatively, we set APVTS params directly — which is what the buttons do.
    // For a clean proxy, we do what buttons do: set APVTS params.

    // We'll provide high-level actions that match what the button clicks do:
    ButtonProxy rollButton();
    ButtonProxy shiftButton();
    ButtonProxy pageLeftButton();
    ButtonProxy pageRightButton();
    ButtonProxy lengthPlusButton();
    ButtonProxy lengthMinusButton();
    ButtonProxy nudgeLeftButton();
    ButtonProxy nudgeRightButton();
    ButtonProxy duplicateButton();
    ButtonProxy clearWindowButton();
    ButtonProxy clearStepButton();
    ButtonProxy randomizeStepButton();
    ButtonProxy randomizeAllButton();
    ButtonProxy shuffleButton();

    bool isShiftActive() const;

    SequencerControlsComponent& raw() { return comp_; }

private:
    SequencerControlsComponent& comp_;
};

// ============================================================================
// Proxy: NavBarComponent
// ============================================================================
class NavBarProxy {
public:
    explicit NavBarProxy(NavBarComponent& comp) : comp_(comp) {}

    void clickPlayPause();
    void clickStop();
    void clickSettings();
    void clickExport();
    void clickMidiLearn();

    NavBarComponent& raw() { return comp_; }

private:
    NavBarComponent& comp_;
};

// ============================================================================
// Proxy: PianoRollComponent
// ============================================================================
class PianoRollProxy {
public:
    explicit PianoRollProxy(PianoRollComponent& comp) : comp_(comp) {}

    /// Toggle a chromatic note on/off (0=C, 1=Db, ..., 11=B)
    bool toggleNote(int chromaticIndex);

    /// Get the number of enabled notes in the current scale
    int enabledNoteCount() const;

    PianoRollComponent& raw() { return comp_; }

private:
    PianoRollComponent& comp_;
};

// ============================================================================
// Proxy: SequencerStepsComponent (the 8 visible step buttons)
// ============================================================================
class SequencerStepsProxy {
public:
    explicit SequencerStepsProxy(SequencerStepsComponent& comp) : comp_(comp) {}

    /// Click step button at local index (0-7 within current window)
    void clickStep(int localIndex);

    SequencerStepsComponent& raw() { return comp_; }

private:
    SequencerStepsComponent& comp_;
};

// ============================================================================
// Proxy: SubsequenceControlsComponent
// ============================================================================
class SubsequenceProxy {
public:
    explicit SubsequenceProxy(SubsequenceControlsComponent& comp) : comp_(comp) {}

    void clickBar(int index);
    void doubleClickBar(int index);
    void clickPlus();
    void clickMinus();

    SubsequenceControlsComponent& raw() { return comp_; }

private:
    SubsequenceControlsComponent& comp_;
};

// ============================================================================
// Proxy: SequenceOverviewComponent (the 8x8 grid)
// ============================================================================
class SequenceOverviewProxy {
public:
    explicit SequenceOverviewProxy(SequenceOverviewComponent& comp) : comp_(comp) {}

    /// Click a step in the grid (0-63)
    void clickStep(int stepIndex);
    /// Double-click a step in the grid (0-63)
    void doubleClickStep(int stepIndex);

    SequenceOverviewComponent& raw() { return comp_; }

private:
    SequenceOverviewComponent& comp_;
};

// ============================================================================
// TestApp - The main entry point
// ============================================================================
class TestApp {
public:
    TestApp() : editor_(processor_) {
        // Give components time to initialize
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }

    ~TestApp() {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }

    // --- Pump messages (required after UI interactions to process async callbacks) ---
    void processMessages(int timeoutMs = 50) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(timeoutMs);
    }

    // --- Simulate an audio block (for processBlock / clock ticking) ---
    void simulateAudioBlock(int numSamples = 512) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        juce::MidiBuffer midiBuffer;
        processor_.processBlock(buffer, midiBuffer);
    }

    // --- Simulate an audio block and capture MIDI output ---
    juce::MidiBuffer simulateAudioBlockWithMidi(int numSamples = 512) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        juce::MidiBuffer midiBuffer;
        processor_.processBlock(buffer, midiBuffer);
        return midiBuffer;
    }

    // --- Direct state access ---
    SequenceState& state() { return processor_.getState(); }
    PluginProcessor& processor() { return processor_; }
    PluginEditor& editor() { return editor_; }
    ApplicationComponent& appComponent() { return editor_.getApplicationComponent(); }

    // --- Component proxies (mirror the UI hierarchy) ---
    NavBarProxy navBar();
    SequencerControlsProxy sequencerControls();
    SequencerStepsProxy sequencerSteps();
    PianoRollProxy pianoRoll();
    SubsequenceProxy subsequenceControls();
    SequenceOverviewProxy sequenceOverview();

    // --- MIDI learn: programmatically map a control to a MIDI note ---
    void mapMidiNote(MidiControlId controlId, int noteNumber, int channel = 1) {
        auto& registry = processor_.getMidiLearnRegistry();
        registry.enterLearnMode();
        registry.setLearningControl(controlId);
        registry.learnMidiMessage(MidiMessageType::NOTE, channel, noteNumber);
        registry.exitLearnMode();
    }

    // --- Send a MIDI note through processBlock ---
    void sendMidiNoteOn(int noteNumber, int channel = 1, juce::uint8 velocity = 127) {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, velocity), 0);
        processor_.processBlock(buffer, midi);
    }

    void sendMidiNoteOff(int noteNumber, int channel = 1) {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), 0);
        processor_.processBlock(buffer, midi);
    }

    // Convenience: get MainScreenComponent
    MainScreenComponent& mainScreen() {
        return editor_.getApplicationComponent().getMainScreen();
    }

private:
    PluginProcessor processor_;
    PluginEditor editor_;
};

// ============================================================================
// Implementation (inline, since this is header-only)
// ============================================================================

// --- TestApp proxy factories ---

inline NavBarProxy TestApp::navBar() {
    return NavBarProxy(editor_.getApplicationComponent().getNavBar());
}

inline SequencerControlsProxy TestApp::sequencerControls() {
    return SequencerControlsProxy(mainScreen().getSequencerControls());
}

inline SequencerStepsProxy TestApp::sequencerSteps() {
    return SequencerStepsProxy(mainScreen().getSequencerSteps());
}

inline PianoRollProxy TestApp::pianoRoll() {
    return PianoRollProxy(mainScreen().getPianoRoll());
}

inline SubsequenceProxy TestApp::subsequenceControls() {
    return SubsequenceProxy(mainScreen().getSubsequenceControls());
}

inline SequenceOverviewProxy TestApp::sequenceOverview() {
    return SequenceOverviewProxy(mainScreen().getSequenceOverview());
}

// --- NavBarProxy ---

inline void NavBarProxy::clickPlayPause() {
    if (comp_.onPlayPauseClicked) comp_.onPlayPauseClicked();
}

inline void NavBarProxy::clickStop() {
    if (comp_.onStopClicked) comp_.onStopClicked();
}

inline void NavBarProxy::clickSettings() {
    if (comp_.onSettingsClicked) comp_.onSettingsClicked();
}

inline void NavBarProxy::clickExport() {
    if (comp_.onExportClicked) comp_.onExportClicked();
}

inline void NavBarProxy::clickMidiLearn() {
    if (comp_.onMidiLearnClicked) comp_.onMidiLearnClicked(true);
}

// --- PianoRollProxy ---

inline bool PianoRollProxy::toggleNote(int chromaticIndex) {
    return comp_.TEST_simulateKeyClick(chromaticIndex);
}

inline int PianoRollProxy::enabledNoteCount() const {
    return comp_.getSelectedNoteIndices().size();
}

// --- SequencerStepsProxy ---

inline void SequencerStepsProxy::clickStep(int localIndex) {
    if (comp_.onStepClicked) {
        // The callback expects the absolute step index
        // We need the window start to compute it
        // onStepClicked is wired in ApplicationComponent to call triggerStepMidi(stepIndex)
        // but the stepIndex from SequencerStepsComponent is already absolute
        comp_.onStepClicked(localIndex);
    }
}

// --- SubsequenceProxy ---

inline void SubsequenceProxy::clickBar(int index) {
    if (comp_.onBarClicked) comp_.onBarClicked(index);
}

inline void SubsequenceProxy::doubleClickBar(int index) {
    if (comp_.onBarDoubleClicked) comp_.onBarDoubleClicked(index);
}

inline void SubsequenceProxy::clickPlus() {
    if (comp_.onPlusClicked) comp_.onPlusClicked();
}

inline void SubsequenceProxy::clickMinus() {
    if (comp_.onMinusClicked) comp_.onMinusClicked();
}

// --- SequenceOverviewProxy ---

inline void SequenceOverviewProxy::clickStep(int stepIndex) {
    auto& grid = comp_.getSequenceGrid();
    if (grid.onSquareClicked) grid.onSquareClicked(stepIndex);
}

inline void SequenceOverviewProxy::doubleClickStep(int stepIndex) {
    auto& grid = comp_.getSequenceGrid();
    if (grid.onSquareDoubleClicked) grid.onSquareDoubleClicked(stepIndex);
}

// --- SequencerControlsProxy ---
// The buttons are private members. We access them as children by their position
// in the component's child list, OR we use a simpler approach: since each button
// just sets an APVTS param on/off, we can get the child component at the known
// layout slot. The SequencerControlsComponent adds children in a known order.
//
// But actually, the cleanest approach: use getChildComponent() since buttons are
// added in a deterministic order (roll, shift, then normal, then shift-mode).
//
// Order of addAndMakeVisible / addChildComponent:
//   0: rollButton
//   1: shiftButton
//   2: pageLeftButton
//   3: pageRightButton
//   4: lengthPlusButton
//   5: lengthMinusButton
//   6: nudgeLeftButton
//   7: nudgeRightButton
//   8: duplicateButton
//   9: clearWindowButton
//  10: clearStepButton
//  11: randomizeAllButton
//  12: randomizeStepButton
//  13: shuffleButton

inline ButtonProxy SequencerControlsProxy::rollButton() {
    return ButtonProxy(*comp_.getChildComponent(0));
}

inline ButtonProxy SequencerControlsProxy::shiftButton() {
    return ButtonProxy(*comp_.getChildComponent(1));
}

inline ButtonProxy SequencerControlsProxy::pageLeftButton() {
    return ButtonProxy(*comp_.getChildComponent(2));
}

inline ButtonProxy SequencerControlsProxy::pageRightButton() {
    return ButtonProxy(*comp_.getChildComponent(3));
}

inline ButtonProxy SequencerControlsProxy::lengthPlusButton() {
    return ButtonProxy(*comp_.getChildComponent(4));
}

inline ButtonProxy SequencerControlsProxy::lengthMinusButton() {
    return ButtonProxy(*comp_.getChildComponent(5));
}

inline ButtonProxy SequencerControlsProxy::nudgeLeftButton() {
    return ButtonProxy(*comp_.getChildComponent(6));
}

inline ButtonProxy SequencerControlsProxy::nudgeRightButton() {
    return ButtonProxy(*comp_.getChildComponent(7));
}

inline ButtonProxy SequencerControlsProxy::duplicateButton() {
    return ButtonProxy(*comp_.getChildComponent(8));
}

inline ButtonProxy SequencerControlsProxy::clearWindowButton() {
    return ButtonProxy(*comp_.getChildComponent(9));
}

inline ButtonProxy SequencerControlsProxy::clearStepButton() {
    return ButtonProxy(*comp_.getChildComponent(10));
}

inline ButtonProxy SequencerControlsProxy::randomizeAllButton() {
    return ButtonProxy(*comp_.getChildComponent(11));
}

inline ButtonProxy SequencerControlsProxy::randomizeStepButton() {
    return ButtonProxy(*comp_.getChildComponent(12));
}

inline ButtonProxy SequencerControlsProxy::shuffleButton() {
    return ButtonProxy(*comp_.getChildComponent(13));
}

inline bool SequencerControlsProxy::isShiftActive() const {
    return comp_.isShiftActive();
}
