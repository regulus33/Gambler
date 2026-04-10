# Gambler Deep Dive: Backend Logic & MIDI Generation

A hands-on guide for debugging in CLion. Each section explains a concept, shows the relevant code, and suggests breakpoint locations. Set breakpoints, run Standalone, and step through.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [How a Note Gets Played (The Main Loop)](#2-how-a-note-gets-played)
3. [The Sequencer Tick: Step Advancement](#3-the-sequencer-tick)
4. [Scale Quantization: The Two Modes](#4-scale-quantization)
5. [Destructive Mode: Baking Notes](#5-destructive-mode)
6. [Note Range (Window) System](#6-note-range-window-system)
7. [How UI Changes Flow to the Backend](#7-ui-to-backend-flow)
8. [How Backend Changes Flow to the UI](#8-backend-to-ui-flow)
9. [MIDI Input Mapping](#9-midi-input-mapping)
10. [Random Mode (Roll)](#10-random-mode)
11. [Thread Safety & Lock-Free Patterns](#11-thread-safety)
12. [Suggested Debugging Sessions](#12-debugging-sessions)

---

## 1. Architecture Overview

The app has three layers, running on two threads:

```
┌──────────────────────────────────────────────────────┐
│  UI THREAD (message thread)                          │
│                                                      │
│  ApplicationComponent                                │
│    ├─ PianoRoll (scale selection)                     │
│    ├─ NoteControls (pitch class + octave sliders)     │
│    ├─ NoteRangeControls (min/max range knobs)         │
│    ├─ SequencerSteps (8 visible step buttons)         │
│    ├─ SequenceOverview (8x8 grid)                     │
│    └─ SubsequenceControls (substep bars)              │
│                                                      │
│  All UI reads go through SequenceState (quantized)   │
│  All UI writes go through SequenceState (baked)      │
├──────────────────────────────────────────────────────┤
│  SHARED STATE                                        │
│                                                      │
│  APVTS (AudioProcessorValueTreeState)                │
│    ├─ Scale notes: global_scale_note_0..11           │
│    ├─ Range window: scale_window_offset, _size       │
│    ├─ Destructive flags: pref_destructive_scale/range│
│    ├─ Sequencer buttons: roll, length_plus, etc.     │
│    └─ Clock: clock_mode, clock_bpm, clock_subdivision│
│                                                      │
│  InternalState (non-APVTS, too many params)          │
│    ├─ Step notes: step_N_note_M (64 steps x 8 subs) │
│    ├─ Step modulations (CC/velocity per step)        │
│    └─ Step metadata (substep count, current index)   │
├──────────────────────────────────────────────────────┤
│  AUDIO THREAD (real-time, ~every 5ms)                │
│                                                      │
│  PluginProcessor::processBlock()                     │
│    ├─ Process incoming MIDI (control mapping)        │
│    ├─ Check clock → shouldTick()?                    │
│    ├─ state.tick() → quantized MIDI note             │
│    ├─ Send MIDI note-on                              │
│    └─ Schedule sample-accurate note-off              │
└──────────────────────────────────────────────────────┘
```

**Key classes and where they live:**

| Class | File | Role |
|-------|------|------|
| `PluginProcessor` | `source/PluginProcessor.cpp` | Audio callback, MIDI I/O, parameter listener |
| `SequenceState` | `source/backend/SequenceState.h` | Public API wrapping state + quantizer |
| `Sequence` | `source/backend/Sequence.h` | Playback engine (tick, nudge, randomize) |
| `Step` | `source/backend/Step.h` | Lightweight view of one step's data |
| `ScaleQuantizer` | `source/backend/ScaleQuantizer.h` | Scale/range quantization logic |
| `Scale` | `source/backend/Scale.h` | Static math for range window conversions |
| `ApplicationComponent` | `source/ui/ApplicationComponent.h` | Wires UI callbacks to state |

---

## 2. How a Note Gets Played

The heart of the plugin is `processBlock()`. Every ~5ms, the audio host calls this with a buffer to fill. Here's the complete flow:

**File:** `source/PluginProcessor.cpp:179`

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
```

### Step-by-step walkthrough:

**1. Process incoming MIDI for control mapping** (line 190)
```cpp
processMidiInput(midiMessages);
```
Any incoming MIDI CC/notes are checked against the MidiLearnRegistry. If a mapping exists (e.g., CC 74 → NOTE_SLIDER), it's handled immediately. More on this in [Section 9](#9-midi-input-mapping).

**2. Clear the MIDI buffer** (line 193)
```cpp
midiMessages.clear();
```
Gambler generates MIDI — it doesn't pass through input. The buffer is cleared so we can fill it with our own notes.

**3. Handle preview notes from UI** (lines 196-230)
When you click a step or change a note slider with "preview" enabled, the UI queues a short MIDI note into a lock-free ring buffer. processBlock drains it here.

**4. Sample-accurate note-off** (lines 243-280)
If a note-off was scheduled from the previous tick, check if we've reached the target sample position. If so, send the note-off at the exact right sample within the buffer.

> **Breakpoint:** `source/PluginProcessor.cpp:257` — set a conditional breakpoint here (`scheduledNoteOffSample >= 0`) to watch note-offs fire.

**5. Clock check** (lines 286-300)
The clock determines whether it's time to advance the sequencer. Three modes:
- **INTERNAL:** Sample counter reaches the threshold for the current BPM
- **DAW_SYNC:** Playhead position crossed a subdivision boundary
- **EXTERNAL_MIDI:** Received enough MIDI clock messages

**6. THE MAIN TICK** (line 341)
```cpp
if (sequencerState.load() == SequencerState::Playing && clock.shouldTick()) {
```
This is the gate. Everything below only runs when:
- The sequencer is Playing (not Paused/Stopped)
- The clock says it's time for the next step

> **Breakpoint:** `source/PluginProcessor.cpp:341` — THE most important breakpoint. Hit Play in standalone and watch each tick.

**7. Send note-off for previous note** (line 356)
```cpp
if (lastPlayedNote >= 0) {
    auto noteOff = juce::MidiMessage::noteOff(getMidiOutputChannel(), lastPlayedNote, (juce::uint8)0);
    midiMessages.addEvent(noteOff, 0);
}
```

**8. Tick the sequence** (line 370)
```cpp
int note = state.tick();    // Returns quantized MIDI note, or -1 for rest
int currentStepIndex = state.getCurrentIndex();
```
This is where the magic happens. `state.tick()` advances the step counter and returns the note to play — already quantized to the current scale and range. See [Section 3](#3-the-sequencer-tick) for what happens inside.

> **Breakpoint:** `source/PluginProcessor.cpp:370` — watch `note` to see what the sequencer decided to play.

**9. Handle modulations** (line 378)
```cpp
auto selectedMods = state.getSelectedModulations(currentStepIndex);
```
Each step can have up to 7 modulation slots (velocity or CC). These are sent as MIDI messages alongside the note.

**10. Send MIDI note-on** (line 406)
```cpp
auto noteOn = juce::MidiMessage::noteOn(getMidiOutputChannel(), note, velocity);
midiMessages.addEvent(noteOn, 0);
```

**11. Schedule note-off** (lines 432-446)
```cpp
double samplesPerStep = clock.calculateSamplesPerStep(dawBpm, getSampleRate());
double noteLengthPercent = state.getNoteLength();
juce::int64 noteOffDelay = static_cast<juce::int64>(samplesPerStep * noteLengthPercent);
scheduledNoteOffSample = currentSamplePosition + noteOffDelay;
```
The note-off isn't sent now — it's scheduled for a future processBlock call, at the exact sample position. This is what gives notes their length (gate time).

---

## 3. The Sequencer Tick

When `processBlock` calls `state.tick()`, here's the call chain:

```
PluginProcessor::processBlock()     (PluginProcessor.cpp:370)
  └─ SequenceState::tick()          (SequenceState.h:319)
       └─ Sequence::tick()          (Sequence.h:138)
            ├─ advance currentIndex: (idx + 1) % length
            ├─ if random mode: → generateRandomNoteUnquantized() → bake + quantize
            └─ else: → tickStep(idx)
                      └─ Step::advanceSubstep()
                      └─ Step::note(substepIndex)  ← raw from storage
                      └─ ScaleQuantizer::quantize(note)  ← on-the-fly
```

**File:** `source/backend/Sequence.h:138`

```cpp
int tick() {
    int len = length();
    int idx = (currentIndex() + 1) % len;    // Wrap around
    setCurrentIndex(idx);

    // Probability index advances independently (always 0-63)
    int probIdx = (lastProbabilityIndex() + 1) % ParamConstants::MAX_STEPS;
    setLastProbabilityIndex(probIdx);

    if (isRandom()) {
        // Roll mode — generate random note, see Section 10
        ...
    }

    return tickStep(idx);
}
```

**The normal path — `tickStep()`:** (Sequence.h:163)
```cpp
int tickStep(int stepIndex) {
    Step s = step(stepIndex);
    int substepIndex = s.advanceSubstep();   // Advance within the step's substeps
    int note = s.note(substepIndex);         // Read raw note from InternalState
    if (note < 0) return note;               // -1 = rest, pass through
    return scaleQ.quantize(note);            // Quantize on-the-fly for playback
}
```

**This is the critical insight:** The raw note is stored unquantized (e.g., MIDI 61 = Db). At playback time, `quantize()` snaps it to the nearest note in the active scale. If you later change the scale, the same raw note 61 will quantize to a different result — without modifying storage.

> **Breakpoint:** `source/backend/Sequence.h:166` — watch `note` (raw) vs the return value (quantized). Change the scale in the piano roll between ticks and see the same raw note produce different results.

---

## 4. Scale Quantization: The Two Modes

`ScaleQuantizer` has two methods that look similar but serve completely different purposes:

### `quantize()` — Display/Playback (Non-Destructive)

**File:** `source/backend/ScaleQuantizer.h:139`

```cpp
int quantize(int noteNumber) const {
    bool skipRange = destructiveRange();   // If destructive range is ON, skip clamping
    bool skipScale = destructiveScale();   // If destructive scale is ON, skip snapping

    int rMin = rangeMin();
    int rMax = rangeMax();

    // Step 1: Clamp to range (unless destructive range is ON)
    if (!skipRange)
        noteNumber = juce::jlimit(rMin, rMax, noteNumber);

    // Step 2: Snap to scale (unless destructive scale is ON)
    if (skipScale)
        return noteNumber;

    if (!anyScaleNoteEnabled()) return -1;
    if (isInScale(noteNumber)) return noteNumber;

    // Step 3: Find nearest in-scale note by expanding distance
    for (int d = 1; d <= 127; d++) {
        int down = noteNumber - d;
        if (down >= searchMin && isInScale(down)) return down;  // Prefers DOWN
        int up = noteNumber + d;
        if (up <= searchMax && isInScale(up)) return up;
    }
    return -1;
}
```

**When is this called?**
- Every `tick()` during playback (Sequence.h:168)
- Every UI read: `getQuantizedStepNote()`, `getCurrentNoteForStep()` (SequenceState.h:136-148)
- The grid display, the note sliders, the active dot on piano roll — all read through this

**Key behavior:**
- **Prefers downward** when equidistant. If raw note is 61 (Db) and scale is {C, D}, it snaps DOWN to C (60), not up to D (62).
- **Respects range bounds.** If range is 50-71, the search for nearest in-scale note only looks within 50-71.
- **Skips its own axis when destructive is ON.** If destructive scale is ON, `quantize()` skips the scale snap entirely — it assumes the raw data was already baked.

> **Breakpoint:** `source/backend/ScaleQuantizer.h:139` — set a conditional breakpoint on `noteNumber == 61` (or whatever note you're investigating). Watch `skipRange`, `skipScale`, `rMin`, `rMax` to understand the decision.

### `bake()` — Storage (Destructive)

**File:** `source/backend/ScaleQuantizer.h:173`

```cpp
int bake(int noteNumber) const {
    if (noteNumber < 0) return noteNumber;

    bool bakeR = destructiveRange();
    bool bakeS = destructiveScale();

    if (!bakeR && !bakeS) return noteNumber;  // No-op when both flags OFF

    // Step 1: Clamp to range (if destructive range is ON)
    if (bakeR)
        noteNumber = juce::jlimit(rangeMin(), rangeMax(), noteNumber);

    // Step 2: Snap to scale (if destructive scale is ON)
    if (bakeS) {
        if (!anyScaleNoteEnabled()) return -1;
        if (!isInScale(noteNumber)) {
            // Find nearest, same algorithm as quantize()
            ...
        }
    }
    return noteNumber;
}
```

**When is this called?**
- Before storing any note: `updateSelectedNoteForStep()` (SequenceState.h:164)
- Before storing any note: `updateCurrentNoteForStep()` (SequenceState.h:152)
- During random mode: `queueNoteWrite(idx, substepIdx, scaleQ.bake(unquantized))` (Sequence.h:155)
- Mass bake: `bakeAllExistingNotes()` (Sequence.h:237)

**When destructive flags are OFF**, `bake()` returns the note unchanged — raw storage is untouched. When flags are ON, the note is modified before being written.

> **Breakpoint:** `source/backend/ScaleQuantizer.h:173` — watch `bakeR`, `bakeS`, and compare input `noteNumber` to return value.

### The Symmetry

| | `quantize()` | `bake()` |
|---|---|---|
| **Called during** | Playback & display | Storage (writing notes) |
| **Modifies storage?** | No | Yes (if flags ON) |
| **Active when destructive OFF** | Yes (applies scale+range) | No (returns unchanged) |
| **Active when destructive ON** | No (skips, assumes pre-baked) | Yes (snaps before storage) |

They're complementary halves of the same system. At any given time, exactly one of them is doing the quantization work.

---

## 5. Destructive Mode

When you click the "Bake Scale" or "Bake Range" button in the navbar, here's what happens:

**File:** `source/PluginProcessor.cpp:695-707`

```cpp
// In parameterChanged():
if (parameterID == Params::Keys::ID::PREF_DESTRUCTIVE_SCALE && isOn) {
    getState().bakeAllExistingNotes();
}

if (parameterID == Params::Keys::ID::PREF_DESTRUCTIVE_RANGE && isOn) {
    getState().bakeAllExistingNotes();
}
```

**`bakeAllExistingNotes()`** walks every step and substep, calling `bake()` on each:

**File:** `source/backend/Sequence.h:237`
```cpp
void bakeAllExistingNotes() {
    for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
        Step s = step(i);
        for (int sub = 0; sub < ParamConstants::MAX_SUBSTEPS; ++sub) {
            int note = s.note(sub);
            int baked = scaleQ.bake(note);
            if (baked != note)
                s.setNote(sub, baked);
        }
    }
}
```

### What this means in practice:

**Scenario: Non-destructive scale (default)**
1. You have raw note 61 (Db) stored in step 3
2. You set scale to C major (C, D, E, F, G, A, B)
3. During playback, `quantize(61)` snaps it to 60 (C) on-the-fly
4. Raw storage still says 61
5. You remove the scale → note plays as 61 again

**Scenario: Destructive scale**
1. You have raw note 61 (Db) stored in step 3
2. You set scale to C major
3. You click "Bake Scale" button → `bakeAllExistingNotes()` runs
4. `bake(61)` with C major → returns 60 (C)
5. Raw storage is overwritten: 61 → 60
6. During playback, `quantize()` is skipped (destructive flag ON)
7. You remove the scale → note still plays as 60. **The original 61 is gone forever.**

> **Breakpoint:** `source/backend/Sequence.h:242` — set conditional breakpoint `baked != note` to catch only the notes that actually change during a bake operation.

> **Breakpoint:** `source/PluginProcessor.cpp:700` — catch the moment destructive mode is activated.

---

## 6. Note Range (Window) System

The note range restricts which MIDI notes are available. It's defined by two APVTS parameters that map to a window on the 0-127 MIDI range:

**File:** `source/backend/Scale.h:47-77`

### The Window Parameters

```
APVTS Params:
  scale_window_offset  (0-127)  →  raw knob position
  scale_window_size    (1-128)  →  how many MIDI notes wide

The offset knob doesn't map linearly to MIDI note position.
Instead, it maps to the "headroom" — the valid start positions:

  headroom = 128 - size
  actual_min = round(offset / 127.0 * headroom)
  actual_max = actual_min + size - 1
```

**Why the indirection?** So the offset and size knobs are independent. No matter where you set the offset, the window never exceeds MIDI 127. The size knob doesn't silently shrink when you move the offset.

```cpp
// Scale.h:47
static int computeActualOffset(int rawOffset, int size) {
    int headroom = 128 - size;
    if (headroom <= 0) return 0;
    double t = static_cast<double>(rawOffset) / 127.0;
    return static_cast<int>(std::round(t * static_cast<double>(headroom)));
}

// Scale.h:54 — final MIDI min
static int computeMinFromWindow(int rawOffset, int size) {
    return juce::jlimit(0, 127, computeActualOffset(rawOffset, size));
}

// Scale.h:58 — final MIDI max
static int computeMaxFromWindow(int rawOffset, int size) {
    int minNote = computeMinFromWindow(rawOffset, size);
    return juce::jlimit(0, 127, minNote + size - 1);
}
```

### Where range is applied

**During quantization** (ScaleQuantizer.h:146):
```cpp
if (!skipRange)
    noteNumber = juce::jlimit(rMin, rMax, noteNumber);
```

**During UI slider rebuilding** (ApplicationComponent.h — `updateNoteControlOptions()`):
The note and octave sliders only show options that exist within the current range AND scale. For example, if range is 50-71 and scale is {C, E, G}, the note slider shows only {C, E, G} and the octave slider shows only octaves 4-5 (where those notes fall in range).

> **Breakpoint:** `source/backend/Scale.h:47` in `computeActualOffset` — pass different offset/size combinations and watch the math.

---

## 7. UI to Backend Flow

When the user interacts with the UI, here's how the change reaches the backend:

### Example: User changes the Note Slider to "E"

**File:** `source/ui/ApplicationComponent.h:117`

```cpp
mainScreen.getMainControls().getNoteControls().onNoteChanged = [this](int pitchClass) {
    auto& state = processorRef.getState();
    int selectedStep = state.getSelectedStepForEditing();         // Which step is selected
    int selectedSubstep = state.getSelectedSubstepIndex(selectedStep);
    int currentNote = state.getQuantizedStepNote(selectedStep, selectedSubstep);
    int currentOctave = currentNote / 12;                         // Preserve the octave
    int newNote = currentOctave * 12 + pitchClass;                // Combine: octave + new pc

    state.updateSelectedNoteForStep(selectedStep, newNote);       // Store (with bake)
};
```

The chain continues:

```
ApplicationComponent callback
  └─ SequenceState::updateSelectedNoteForStep(step, note)    (SequenceState.h:161)
       └─ step.setNote(substepIdx, bakeNoteForStorage(note)) 
            ├─ bakeNoteForStorage() → ScaleQuantizer::bake() (maybe transforms the note)
            └─ Step::setNote() → InternalState::setStepNote() (writes to storage)
```

> **Breakpoint:** `source/backend/SequenceState.h:164` — watch `noteNumber` going in and `bakeNoteForStorage(noteNumber)` coming out. If destructive mode is OFF, they'll be identical. If ON, the note may be snapped.

### Example: User toggles a scale note on the Piano Roll

**File:** `source/ui/ApplicationComponent.h:103`

```cpp
mainScreen.getPianoRoll().onNoteChanged = [this]() {
    Scale newScale = mainScreen.getPianoRoll().getScale();
    auto& state = processorRef.getState();
    state.setGlobalScaleNotes(newScale.getScaleNotes());   // Write 12 APVTS params

    int selectedStep = state.getSelectedStepForEditing();
    int selectedNote = state.getSelectedNoteForStep(selectedStep);
    updateNoteControlOptions(selectedNote);                 // Rebuild sliders

    triggerAsyncUpdate();                                   // Refresh UI
};
```

This writes to 12 APVTS bool params (`global_scale_note_0` through `global_scale_note_11`). The change is immediately visible to the audio thread's next `quantize()` call — no synchronization needed because APVTS reads are atomic.

---

## 8. Backend to UI Flow

After the audio thread ticks, the UI needs to update. Here's the pattern:

```
AUDIO THREAD                          MESSAGE THREAD
─────────────                         ──────────────
processBlock()
  └─ state.tick()
  └─ onSequenceTick callback ──────→ triggerAsyncUpdate()
                                       └─ handleAsyncUpdate()
                                            ├─ applyPendingNoteWrites()
                                            ├─ updateNoteControlOptions()
                                            └─ mainScreen.updateFromSequence()
```

**File:** `source/ui/ApplicationComponent.h:319`

```cpp
void handleAsyncUpdate() override {
    // 1. Apply any queued note writes from the audio thread (random mode)
    processorRef.getState().applyPendingNoteWrites();

    // 2. Rebuild slider options BEFORE refreshing display
    //    (octave slider needs current scale+range to map correctly)
    auto& state = processorRef.getState();
    int selectedStep = state.getSelectedStepForEditing();
    int selectedSubstep = state.getSelectedSubstepIndex(selectedStep);
    int selectedNote = state.getQuantizedStepNote(selectedStep, selectedSubstep);
    updateNoteControlOptions(selectedNote);

    // 3. Refresh all visual components
    mainScreen.updateFromSequence();

    updatePlayPauseButtonState();
    repaintMidiLearnComponents();
}
```

**Why `applyPendingNoteWrites()` first?** In random mode, the audio thread generates notes but can't write to InternalState directly (it's not safe). Instead, it queues writes to a lock-free ring buffer. The message thread applies them here before reading the state for display.

> **Breakpoint:** `source/ui/ApplicationComponent.h:320` — this fires after every sequencer tick. Step through to see the full UI update cycle.

---

## 9. MIDI Input Mapping

When a MIDI CC or Note message arrives, here's the path:

```
processBlock()
  └─ processMidiInput(midiMessages)           (PluginProcessor.cpp:190)
       └─ for each MIDI message:
            └─ midiLearnRegistry.findMapping() (check if mapped)
                 └─ handleMidiMapping(mapping, value, isOn)
```

**File:** `source/PluginProcessor.cpp:1239`

`handleMidiMapping` is a big switch on `MidiControlId`. Here are the important cases:

### Scale Notes (Piano Roll via MIDI)
```cpp
// Line 1492-1515
case MidiControlId::SCALE_NOTE_C:  // through SCALE_NOTE_B
    int noteIndex = mapping.controlId - MidiControlId::SCALE_NOTE_C;  // 0-11
    bool currentState = state.getGlobalScaleNote(noteIndex);
    if (isNoteOnOrCCPositive) {
        state.setGlobalScaleNote(noteIndex, !currentState);  // Toggle!
    }
```
Scale notes are **toggled** on MIDI note-on. Note-off is ignored.

### Note Slider (Pitch Class via MIDI)
```cpp
// Line 1268-1280
case MidiControlId::NOTE_SLIDER:
    int pitchClass = MidiValueConverter::midiToDiscreteIndex(midiValue, 11);  // 0-11
    int selectedStep = state.getSelectedStepForEditing();
    int currentNote = state.getSelectedNoteForStep(selectedStep);
    int octave = currentNote / 12;
    int newNote = octave * 12 + pitchClass;
    state.updateSelectedNoteForStep(selectedStep, newNote);
```

**Important difference from UI:** The MIDI path uses the full chromatic range (0-11), while the UI slider only shows notes that are in the current scale. So MIDI CC can set a pitch class that's not in the scale — it'll be quantized during playback.

### Note Range
```cpp
// Line 1463-1478
case MidiControlId::NOTE_RANGE_MIN:
    int offset = MidiValueConverter::midiToDiscreteIndex(midiValue, 127);
    state.setScaleWindowOffset(offset);    // Sets the raw offset param directly

case MidiControlId::NOTE_RANGE_MAX:
    int size = MidiValueConverter::midiToDiscreteIndex(midiValue, 127) + 1;  // 1-128
    state.setScaleWindowSize(size);        // Sets the raw size param directly
```

### Destructive Toggle (via MIDI)
```cpp
// Line 1559+ (DESTRUCTIVE_SCALE / DESTRUCTIVE_RANGE)
case MidiControlId::DESTRUCTIVE_SCALE:
    if (auto* param = apvts.getParameter(Params::Keys::ID::PREF_DESTRUCTIVE_SCALE)) {
        param->setValueNotifyingHost(isNoteOnOrCCPositive ? 1.0f : 0.0f);
    }
```
This sets the APVTS param, which fires `parameterChanged()`, which calls `bakeAllExistingNotes()`. Same path as clicking the button.

### Value Conversion
```cpp
// MidiMapping.h
static int midiToDiscreteIndex(int midiValue, int maxIndex) {
    return static_cast<int>(std::floor(
        static_cast<float>(midiValue) / 127.0f * static_cast<float>(maxIndex)
    ));
}
```
Maps MIDI 0-127 to 0-maxIndex. For example, with maxIndex=11 (pitch classes): CC value 127 → index 11 (B), CC value 0 → index 0 (C), CC value 58 → index 5 (F).

> **Breakpoint:** `source/PluginProcessor.cpp:1250` — at the top of the switch in `handleMidiMapping`. Watch `mapping.controlId` and `midiValue` to see what's being mapped where.

---

## 10. Random Mode (Roll)

When the Roll button is held, `isRandom()` returns true and `tick()` takes a different path:

**File:** `source/backend/Sequence.h:146`

```cpp
if (isRandom()) {
    int substepIdx = step(idx).advanceSubstep();

    // Check play probability (each step has a probability 0-100%)
    if (!playProbability(probIdx)) {
        queueNoteWrite(idx, substepIdx, -1);    // Write rest to storage
        return -1;                               // Silence
    }

    // Generate random note within the current range
    int unquantized = generateRandomNoteUnquantized();

    // STORE the baked version (destructive-aware)
    queueNoteWrite(idx, substepIdx, scaleQ.bake(unquantized));

    // PLAY the quantized version (display-aware)
    return scaleQ.quantize(unquantized);
}
```

**Key insight:** The note that gets **stored** and the note that gets **played** might be different!
- `bake()` is for storage — it only modifies if destructive flags are ON
- `quantize()` is for playback — it only modifies if destructive flags are OFF
- In non-destructive mode: stored = raw random, played = quantized
- In destructive mode: stored = baked (snapped), played = raw (no quantization needed)

**Random note generation** (Sequence.h:217):
```cpp
int generateRandomNoteUnquantized() const {
    int rMin = scaleQ.rangeMin();
    int rMax = scaleQ.rangeMax();
    std::uniform_int_distribution<> dis(rMin, rMax);
    return dis(gen);    // Uniform random within range
}
```

**Why queue writes?** The audio thread can't safely write to InternalState. It uses a lock-free SPSC (single-producer, single-consumer) ring buffer:

```
Audio thread:  queueNoteWrite(step, substep, note)  →  ring buffer
Message thread: applyPendingNoteWrites()             ←  ring buffer  →  InternalState
```

> **Breakpoint:** `source/backend/Sequence.h:154` — watch `unquantized` (raw random) vs `scaleQ.bake(unquantized)` (stored) vs `scaleQ.quantize(unquantized)` (played).

---

## 11. Thread Safety

### What's safe to read from the audio thread?
- **APVTS params** — atomic reads via `getRawParameterValue()->load()`
- **InternalState** — read-only from audio thread (writes queued)

### What's safe to write from the audio thread?
- **APVTS params** — via `setValueNotifyingHost()` (atomic)
- **Pending note buffer** — lock-free ring buffer (Sequence.h)
- **MIDI output buffer** — owned by processBlock's scope

### What must happen on the message thread?
- **InternalState writes** — only via `applyPendingNoteWrites()`
- **UI component updates** — via `handleAsyncUpdate()`
- **Slider option rebuilding** — via `updateNoteControlOptions()`

### The handoff pattern:
```cpp
// Audio thread signals UI thread:
auto callback = onSequenceTick;  // Copy the std::function (atomic pointer)
if (callback) callback();        // This calls triggerAsyncUpdate()

// Message thread responds:
void handleAsyncUpdate() override {
    // Safe to touch UI and InternalState here
}
```

---

## 12. Suggested Debugging Sessions

### Session 1: Watch a Note Play (30 min)

**Goal:** Understand the complete lifecycle of a single note.

1. Launch standalone in CLion
2. Set breakpoints at:
   - `PluginProcessor.cpp:341` (main tick gate)
   - `Sequence.h:163` (tickStep)
   - `ScaleQuantizer.h:139` (quantize)
   - `PluginProcessor.cpp:406` (note-on sent)
   - `PluginProcessor.cpp:440` (note-off scheduled)
3. Click Play in the app
4. Step through each breakpoint. Watch:
   - Which step index is current
   - The raw note from storage
   - The quantized result
   - The MIDI note-on message
   - The sample position for note-off

### Session 2: Scale Changes Mid-Playback (30 min)

**Goal:** See non-destructive quantization in action.

1. Set up a sequence with notes C4(60), Db4(61), D4(62), Eb4(63)
2. Start playback
3. Set breakpoint at `ScaleQuantizer.h:153` (the `isInScale` check)
4. While playing, toggle off Db and Eb in the piano roll
5. Watch how `quantize(61)` now returns 60 (snaps down to C)
6. Toggle them back on — 61 plays as 61 again (storage unchanged)

### Session 3: Destructive Mode Baking (20 min)

**Goal:** See how bakeAllExistingNotes transforms storage.

1. Set up notes: C4(60), Db4(61), D4(62), Eb4(63)
2. Set scale to C major (disable Db, Eb, Gb, Ab, Bb)
3. Set breakpoint at `Sequence.h:242` with condition `baked != note`
4. Click the "Bake Scale" button in the navbar
5. Watch: note 61 → baked to 60, note 63 → baked to 62
6. After baking, remove the scale — notes still play as 60, 62 (originals are gone)

### Session 4: Range + Scale Interaction (30 min)

**Goal:** Understand how range clamping and scale snapping compose.

1. Set notes across a wide range: C3(48), C5(72), C7(96)
2. Set range to 50-71
3. Set breakpoint at `ScaleQuantizer.h:146` (range clamp)
4. Watch: 48 → clamped to 50, 96 → clamped to 71
5. Now add a scale (C, E, G only)
6. Watch: 50 → nearest in-scale within 50-71 is 52 (E3)? No — check: 48→50→nearest in {C,E,G} within 50-71. C=48 is below range, E=52 is in range. So 50→52.

### Session 5: UI Slider Rebuilding (20 min)

**Goal:** Understand why the note/octave sliders change options.

1. Set breakpoint at `ApplicationComponent.h` in `updateNoteControlOptions()`
2. Change the scale or range
3. Step through the loop that builds `noteNames` and `octaveLabels`
4. Watch which pitch classes are "reachable" (exist in scale AND in range)
5. Watch which octaves are valid for the current pitch class

### Session 6: Random Mode + Lock-Free Writes (20 min)

**Goal:** See the audio→UI thread handoff for random notes.

1. Set breakpoints at:
   - `Sequence.h:154` (random note generated)
   - `Sequence.h:155` (queued to ring buffer)
   - `Sequence.h:265` (applied on message thread — `applyPendingNoteWrites`)
2. Hold the Roll button, start playback
3. Watch random notes get generated, queued, then applied
4. Notice the notes appearing in the step grid after `applyPendingNoteWrites`

---

## Quick Reference: Key Line Numbers

| What | File | Line |
|------|------|------|
| processBlock entry | PluginProcessor.cpp | 179 |
| Main tick gate | PluginProcessor.cpp | 341 |
| state.tick() call | PluginProcessor.cpp | 370 |
| MIDI note-on sent | PluginProcessor.cpp | 406 |
| Note-off scheduled | PluginProcessor.cpp | 440 |
| parameterChanged entry | PluginProcessor.cpp | 575 |
| Destructive bake trigger | PluginProcessor.cpp | 699 |
| handleMidiMapping entry | PluginProcessor.cpp | 1239 |
| Sequence::tick() | Sequence.h | 138 |
| Sequence::tickStep() | Sequence.h | 163 |
| Random note generation | Sequence.h | 217 |
| bakeAllExistingNotes() | Sequence.h | 237 |
| Lock-free note queue | Sequence.h | 263 |
| ScaleQuantizer::quantize() | ScaleQuantizer.h | 139 |
| ScaleQuantizer::bake() | ScaleQuantizer.h | 173 |
| Range window math | Scale.h | 47 |
| SequenceState::getCurrentNoteForStep() | SequenceState.h | 147 |
| SequenceState::updateSelectedNoteForStep() | SequenceState.h | 161 |
| handleAsyncUpdate() | ApplicationComponent.h | 319 |
| Piano roll → state wiring | ApplicationComponent.h | 103 |
| Note slider → state wiring | ApplicationComponent.h | 117 |
| Octave slider → state wiring | ApplicationComponent.h | 137 |
