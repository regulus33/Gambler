/**
 * VisualSnapshotTests.cpp
 *
 * One snapshot test per UI component. Run with:
 *   UPDATE_SNAPSHOTS=1 ./build/Tests "[visual]"   — save/refresh goldens
 *   ./build/Tests "[visual]"                       — compare against goldens
 *
 * Goldens live in tests/snapshots/. The _ACTUAL and _DIFF files written on
 * mismatch are what Claude reads during UI development sessions.
 */

#include "helpers/test_helpers.h"
#include "../source/ui/GlobalScaleQuantizerComponent.h"
#include "../source/ui/LogoComponent.h"
#include "../source/ui/NoteColourPickerComponent.h"
#include "../source/ui/StyledDropdown.h"
#include "../source/ui/SavePresetDialogComponent.h"
#include "../source/ui/ScaleLatchButton.h"
#include "../source/ui/SettingsGroup.h"
#include "../source/ui/SettingsScreenComponent.h"
#include "../source/ui/ToggleIconButton.h"
#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Helpers
// ============================================================================

static GamblerTestInitialiser& getJuceInit()
{
    static GamblerTestInitialiser instance;
    return instance;
}

static void snap (juce::Component& comp, const char* name)
{
    SnapshotTester s (name);
    s.capture (comp);
    s.matches();
}

// ============================================================================
// Components accessed via the live PluginEditor tree
// (already have correct bounds from the layout system)
// ============================================================================

TEST_CASE ("Visual - PluginEditor", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getEditor(), "plugin_editor");
}

TEST_CASE ("Visual - ApplicationComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getEditor().getApplicationComponent(), "application_component");
}

TEST_CASE ("Visual - MainScreenComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen(), "main_screen");
}

TEST_CASE ("Visual - NavBarComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getNavBar(), "nav_bar");
}

TEST_CASE ("Visual - PianoRollComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getPianoRoll(), "piano_roll");
}

TEST_CASE ("Visual - SequencerStepsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getSequencerSteps(), "sequencer_steps");
}

TEST_CASE ("Visual - SequencerControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getSequencerControls(), "sequencer_controls");
}

TEST_CASE ("Visual - MainControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getMainControls(), "main_controls");
}

TEST_CASE ("Visual - NoteControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getMainControls().getNoteControls(), "note_controls");
}

TEST_CASE ("Visual - ProbabilityControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getMainControls().getProbabilityControls(), "probability_controls");
}

TEST_CASE ("Visual - ProbabilityGauge", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getMainControls().getProbabilityControls().getProbabilityGauge(), "probability_gauge");
}

static void snapEdenAt (PluginTestFixture& f, int percent, const char* name)
{
    f.getState().setProbabilityPercentage (percent);
    std::array<bool, 64> cells{};
    for (int i = 0; i < 64; ++i)
        cells[static_cast<std::size_t>(i)] = f.getState().getPlayProbability (i);
    auto& gauge = f.getMainScreen().getMainControls().getProbabilityControls().getProbabilityGauge();
    gauge.updateFromProbabilityArray (cells);
    snap (gauge, name);
}

TEST_CASE ("Visual - ProbabilityGauge Eden 25%", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snapEdenAt (f, 25, "probability_gauge_eden_25");
}

TEST_CASE ("Visual - ProbabilityGauge Eden 50%", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snapEdenAt (f, 50, "probability_gauge_eden_50");
}

TEST_CASE ("Visual - ProbabilityGauge Eden 75%", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snapEdenAt (f, 75, "probability_gauge_eden_75");
}

TEST_CASE ("Visual - SequenceOverviewComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getSequenceOverview(), "sequence_overview");
}

TEST_CASE ("Visual - SequenceGrid", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getSequenceOverview().getSequenceGrid(), "sequence_grid");
}

TEST_CASE ("Visual - SubsequenceControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getSubsequenceControls(), "subsequence_controls");
}

TEST_CASE ("Visual - Subsequence length 1 active", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& state = f.getState();

    // Step 0: 1 substep with C4 (MIDI 60), currently playing
    state.setSelectedStepForEditing(0);
    state.setStepNote(0, 0, 60);           // C4
    state.setStepNotesLength(0, 1);
    state.setCurrentIndex(0);              // step 0 is playing
    state.setStepNotesCurrentIndex(0, 0);  // substep 0 is active
    f.processMessages();

    snap (f.getMainScreen().getSubsequenceControls(), "subsequence_length1_active");
}

TEST_CASE ("Visual - Subsequence length 8 mixed", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& state = f.getState();

    // Step 0: 8 substeps with mixed notes, rests, and one active
    state.setSelectedStepForEditing(0);
    state.setStepNote(0, 0, 60);   // C4
    state.setStepNote(0, 1, 64);   // E4
    state.setStepNote(0, 2, -1);   // rest
    state.setStepNote(0, 3, 67);   // G4
    state.setStepNote(0, 4, 72);   // C5
    state.setStepNote(0, 5, -1);   // rest
    state.setStepNote(0, 6, 69);   // A4
    state.setStepNote(0, 7, 63);   // Eb4
    state.setStepNotesLength(0, 8);
    state.setCurrentIndex(0);              // step 0 is playing
    state.setStepNotesCurrentIndex(0, 3);  // substep 3 (G4) is active
    f.processMessages();

    snap (f.getMainScreen().getSubsequenceControls(), "subsequence_length8_mixed");
}

TEST_CASE ("Visual - Subsequence length 2 note and rest", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& state = f.getState();

    // Step 0: 2 substeps — one note, one rest — remaining 6 are inactive
    state.setSelectedStepForEditing(0);
    state.setStepNote(0, 0, 65);           // F4
    state.setStepNote(0, 1, -1);           // rest
    state.setStepNotesLength(0, 2);
    state.setCurrentIndex(1);              // not playing this step
    f.processMessages();

    snap (f.getMainScreen().getSubsequenceControls(), "subsequence_length2_note_rest");
}

TEST_CASE ("Visual - NoteRangeControlsComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getMainScreen().getNoteRangeControls(), "note_range_controls");
}

TEST_CASE ("Visual - PresetSelectorComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getPresetSelector(), "preset_selector");
}

TEST_CASE ("Visual - SubdivisionSlider", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    snap (f.getNavBar().getSubdivisionSlider(), "subdivision_slider");
}

TEST_CASE ("Visual - StepButton", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto* btn = f.getMainScreen().getSequencerSteps().getStepButton (0);
    REQUIRE (btn != nullptr);
    snap (*btn, "step_button");
}

// ============================================================================
// Components that need a PluginProcessor but aren't in the live tree
// ============================================================================

TEST_CASE ("Visual - SettingsScreenComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    SettingsScreenComponent comp (f.getProcessor());
    comp.setSize (1012, 600);
    snap (comp, "settings_screen");
}

TEST_CASE ("Visual - ScaleLatchButton", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    ScaleLatchButton comp (f.getProcessor());
    // ScaleLatchButton sets its own size in its constructor
    snap (comp, "scale_latch_button");
}

TEST_CASE ("Visual - GlobalScaleQuantizerComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    GlobalScaleQuantizerComponent comp (f.getProcessor());
    comp.setSize (400, 120);
    snap (comp, "global_scale_quantizer");
}

TEST_CASE ("Visual - NoteColourPickerComponent", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    NoteColourPickerComponent comp (f.getProcessor());
    comp.setSize (400, 80);
    snap (comp, "note_colour_picker");
}

// ============================================================================
// Standalone components (no PluginProcessor needed)
// ============================================================================

TEST_CASE ("Visual - SavePresetDialogComponent", "[visual]")
{
    getJuceInit();
    SavePresetDialogComponent comp;
    comp.setSize (400, 200);
    snap (comp, "save_preset_dialog");
}

TEST_CASE ("Visual - StyledDropdown", "[visual]")
{
    getJuceInit();
    StyledDropdown comp;
    comp.setSize (300, 300);
    snap (comp, "styled_dropdown");
}

TEST_CASE ("Visual - LogoComponent", "[visual]")
{
    getJuceInit();
    LogoComponent comp;
    comp.setSize (120, 40);
    snap (comp, "logo");
}

TEST_CASE ("Visual - ToggleIconButton", "[visual]")
{
    getJuceInit();
    ToggleIconButton comp ([] { return juce::Path(); }, [] { return juce::Path(); });
    comp.setSize (30, 30);
    snap (comp, "toggle_icon_button");
}

TEST_CASE ("Visual - SettingsGroup", "[visual]")
{
    getJuceInit();
    SettingsGroup comp ("Test Group");
    comp.setSize (300, 120);
    snap (comp, "settings_group");
}

// ============================================================================
// Compact mode snapshots — iOS AUv3 hosting window (~3:1 aspect ratio)
// ============================================================================

TEST_CASE ("Visual - Compact Top Row", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& mainScreen = f.getMainScreen();
    mainScreen.setCompactMode(true);
    mainScreen.setShowingTopRow(true);
    int h = mainScreen.getIdealHeight();
    mainScreen.setSize(1012, h);
    f.processMessages(20);
    snap (mainScreen, "compact_top_row");
}

TEST_CASE ("Visual - Compact Bottom Row", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& mainScreen = f.getMainScreen();
    mainScreen.setCompactMode(true);
    mainScreen.setShowingTopRow(false);
    int h = mainScreen.getIdealHeight();
    mainScreen.setSize(1012, h);
    f.processMessages(20);
    snap (mainScreen, "compact_bottom_row");
}

TEST_CASE ("Visual - Compact Top Row (tall window)", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& mainScreen = f.getMainScreen();
    mainScreen.setCompactMode(true);
    mainScreen.setShowingTopRow(true);
    // Host gives us 300px more than ideal → 200px absorbed as top gap,
    // remaining 100px sits below content.
    int h = mainScreen.getIdealHeight() + 300;
    mainScreen.setSize(1012, h);
    f.processMessages(20);
    snap (mainScreen, "compact_top_row_tall");
}

TEST_CASE ("Visual - Compact Bottom Row (tall window)", "[visual]")
{
    getJuceInit();
    PluginTestFixture f;
    auto& mainScreen = f.getMainScreen();
    mainScreen.setCompactMode(true);
    mainScreen.setShowingTopRow(false);
    int h = mainScreen.getIdealHeight() + 300;
    mainScreen.setSize(1012, h);
    f.processMessages(20);
    snap (mainScreen, "compact_bottom_row_tall");
}
