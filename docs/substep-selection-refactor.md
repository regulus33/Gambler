# Substep Selection Refactor

## Overview

This document covers the refactoring of `SubsequenceControlsComponent` and related state management to support proper substep selection and editing. The work separated two previously conflated concepts — "which substep is selected for editing" vs "which substep will play next" — and cleaned up duplicate code along the way.

## What Was Removed

### Duplicate `updateSubsequenceBar()`

`MainScreenComponent::updateSubsequenceBar()` was a ~35-line method that duplicated the exact same logic already present in `SubsequenceControlsComponent::onStateChanged()`. Both methods iterated over all 8 bars, checked the note number, set colors, labels, and borders identically.

The duplicate existed because `onStateChanged()` fires automatically via the APVTS listener pattern, but the original authors apparently didn't trust it and added explicit refresh calls. Four call sites in `ApplicationComponent.h` were calling `mainScreen.updateSubsequenceBar()` after every state mutation:

- `onBarClicked` callback
- `onMinusClicked` callback
- `onPlusClicked` callback
- `onStepClicked` callback

All four calls were redundant — `setStepNotesCurrentIndex()`, `setStepNotesLength()`, etc. already trigger `valueTreePropertyChanged` on InternalState, which notifies all `StateListenerComponent` subclasses including `SubsequenceControlsComponent`. Removing them had zero behavioral impact.

### Old `hasBorder` bool on SingleBar

`SingleBar` had a single `bool hasBorder` that was only used to indicate the currently-playing substep. This was too limited for the new requirements (selected, next-to-play, current), so it was replaced with a `BarBorderStyle` enum.

The `clearAllBorders()` method on `SubsequenceBar` was also removed — it was unused outside of the `resized()` method's initialization path, and `onStateChanged()` already sets every bar's border on every update.

## What Was Added

### `selectedSubstepIndex` (per-step state)

**Files:** `InternalState.h/cpp`, `SequenceState.h`

Each of the 64 main steps now has a `selectedSubstepIndex` stored in the InternalState ValueTree under the key `step_X_notes_sel`. This is analogous to how `SEQ_SELECTED_STEP` works for the main sequence, but scoped per step.

Key design decisions:
- Stored in InternalState (not APVTS) because it's UI-only state — no reason to expose it to DAW automation
- Defaults to 0, clamped to `[0, MAX_SUBSTEPS-1]`
- Independent from `stepNotesCurrentIndex` — you can have substep 2 selected for editing while substep 0 is queued to play next

### `BarBorderStyle` enum

```
enum class BarBorderStyle {
    none,       // No border
    selected,   // White solid — user selected this for editing
    next,       // Grey dotted — will play next when parent step triggers
    current     // White solid — currently playing right now
};
```

Priority in `onStateChanged()`: `current > selected > next > none`

### `updateSelectedNoteForStep()`

**File:** `SequenceState.h`

Previously there was only `updateCurrentNoteForStep()` which always wrote to the substep at `stepNotesCurrentIndex`. The new method writes to the substep at `selectedSubstepIndex` instead. The note editing callbacks (`onNoteChanged`, `onOctaveChanged`) in `ApplicationComponent.h` now use this method, so when a user selects substep 3 and changes the pitch, it edits substep 3 — not whichever substep happens to be queued for playback.

### Double-click on SingleBar

`SingleBar` now has a `mouseDoubleClick` handler that fires `onDoubleClick`, which propagates up through `SubsequenceBar::onBarDoubleClicked` to `SubsequenceControlsComponent::onBarDoubleClicked`.

## How the New Mechanism Works

### User Interaction Model

The substep bars now mirror the behavior of the main step buttons (`StepButton`):

| Action | Main Steps (StepButton) | Substeps (SingleBar) |
|--------|------------------------|---------------------|
| Single click | Select for editing (white border) | Select for editing (white border) |
| Double click | Jump playback to this step | Set this as active substep |

### Click Flow

**Single click on substep bar:**
1. `SingleBar::mouseDown` fires `onClick`
2. `SubsequenceBar` propagates to `onBarClicked(index)`
3. `SubsequenceControlsComponent` propagates to its own `onBarClicked(index)`
4. `ApplicationComponent::setupSubsequenceCallbacks` handler calls `state.setSelectedSubstepIndex(selectedStep, index)`
5. Piano roll and note controls update to show the selected substep's note
6. `onStateChanged()` fires, drawing a white solid border on the selected bar

**Double click on substep bar:**
1. `SingleBar::mouseDoubleClick` fires `onDoubleClick`
2. Propagates up to `ApplicationComponent` which calls `state.setStepNotesCurrentIndex(selectedStep, index)`
3. This changes which substep will play next when the parent step triggers
4. `onStateChanged()` fires, updating the visual state

### Visual States in `onStateChanged()`

For each bar `i` within the substep length:

```
if (parent step is playing AND i == currentSubstep):
    -> BarBorderStyle::current (white solid border + active circle indicator)
else if (i == selectedSubstep):
    -> BarBorderStyle::selected (white solid border)
else if (i == currentSubstep):
    -> BarBorderStyle::next (grey dotted border)
else:
    -> BarBorderStyle::none
```

The "next" border is specifically useful when a step has multiple substeps and the parent step is not currently playing. It answers the question "which note will sound when this step triggers?" — critical for sequences with >1 substeps where it's otherwise impossible to predict.

### Note Editing Flow

When the user changes a note via the pitch selector or octave selector:

1. `ApplicationComponent` reads `state.getSelectedSubstepIndex(selectedStep)` to find which substep is selected
2. Reads the current note from `state.getQuantizedStepNote(selectedStep, selectedSubstep)` 
3. Computes the new MIDI note from the pitch/octave change
4. Calls `state.updateSelectedNoteForStep(selectedStep, newNote)` which writes to the selected substep

This is distinct from MIDI input, which still uses `updateCurrentNoteForStep()` to write to the playback substep — that's the correct behavior since MIDI input should affect what's about to play, not what the user is browsing.

## Challenges

### Separating Selection from Playback

The original `onBarClicked` callback called `state.setStepNotesCurrentIndex()`, meaning a single click changed the playback position. This meant you couldn't browse substep notes without disrupting the sequence. The fix required threading a new `selectedSubstepIndex` through three layers (InternalState -> SequenceState -> ApplicationComponent callbacks) and updating every place that reads "which substep's note should I display/edit."

### Dotted Border Rendering

JUCE's `Graphics` class doesn't have a built-in `drawDashedRoundedRectangle`. The solution uses `PathStrokeType::createDashedStroke()` to convert a rounded rectangle path into a dashed version, then fills the resulting path:

```cpp
juce::Path roundedRect;
roundedRect.addRoundedRectangle(insetBounds, barCornerRadius);
const float dashLengths[] = { 3.0f, 3.0f };
juce::Path dashedPath;
juce::PathStrokeType(borderWidth).createDashedStroke(dashedPath, roundedRect, dashLengths, 2);
g.fillPath(dashedPath);
```

### Ensuring Note Edits Target the Right Substep

The `handleAsyncUpdate()` method in `ApplicationComponent` was using `getCurrentNoteForStep()` (which reads from `stepNotesCurrentIndex`) to populate the note controls. This was updated to read from `selectedSubstepIndex` so the displayed note always matches what the user selected, not what's queued for playback.

## Test Coverage

15 new assertions in `tests/SequenceStateTests.cpp`:

- `selectedSubstepIndex` defaults to 0
- `setSelectedSubstepIndex` stores and retrieves correctly
- Clamping to valid range (0 to MAX_SUBSTEPS-1)
- Independence per step (step 0 and step 1 can have different selections)
- Independence from `currentIndex` (selected != current)
- `updateSelectedNoteForStep` edits the selected substep only
- `updateCurrentNoteForStep` edits the current substep, not selected

Total suite: 766 assertions across 100 test cases, all passing.
