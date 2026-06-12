# Gambler — Developer Documentation

This document is the high-level guide to Gambler's architecture, information flow, and the decisions that shape the codebase. It is aimed at developers who need to understand where logic lives, how data moves, and what the common traps are. It is **not** a class-by-class API reference; follow the file links and read the headers for that.

For environment-specific how-tos (iOS builds, IDE setup, etc.), see the remaining files in [`docs/`](docs/).

---

## 1. What Gambler Is

Gambler is a JUCE-based MIDI effect / sequencer. It generates MIDI notes from a 64-step sequence, where each step can hold 1–8 substeps. It runs as a standalone app and as a plugin (VST3, AU, AUv3 on iOS). The core loop is: a clock ticks, the sequencer advances, and the app emits note-on/off events (plus optional CC/velocity modulations) on a configurable MIDI channel.

Key concepts:

- **Step / substep**: 64 top-level steps. Each step has 1–8 substeps that cycle when the step is triggered. Substeps are the actual playable notes.
- **Scale & range**: A global 12-note scale and a MIDI note range window constrain which notes can sound. Quantization can be live (non-destructive) or baked into stored data.
- **Probability / Roll**: An Eden-growth probability pattern decides which steps/rests play. A separate "random mode" (Roll) replaces stored notes with random ones from the range.
- **Clock modes**: Internal clock, DAW-synced clock, or external MIDI clock.
- **MIDI Learn**: Map external CC or note messages to on-screen controls.
- **Presets**: Saved as XML in the user's app-data directory.

---

## 2. Architectural Layers

The code is organized into a few clear layers. Know them before you change anything.

```
┌─────────────────────────────────────────┐
│  UI Layer (message thread)              │
│  PluginEditor → ApplicationComponent    │
│    → MainScreenComponent / Settings...  │
│    → individual control components      │
├─────────────────────────────────────────┤
│  Application / Controller Layer         │
│  PluginProcessor                        │
│    - APVTS (DAW-visible parameters)     │
│    - InternalState (ValueTree)          │
│    - SequenceState (domain facade)      │
│    - Clock / DawSync / NoteOffScheduler │
│    - PresetManager / MidiLearnRegistry  │
├─────────────────────────────────────────┤
│  Domain Layer                           │
│  Sequence → Step → InternalState        │
│  ScaleQuantizer → Scale                 │
│  EdenGrowthPattern (probability)        │
├─────────────────────────────────────────┤
│  JUCE / OS / iOS Native Layer           │
│  MidiOutput, MidiBuffer, FileChooser,   │
│  iOSNativeDialogs, ContentSharer...     │
└─────────────────────────────────────────┘
```

### 2.1 PluginProcessor is the controller

`PluginProcessor` owns every long-lived object and orchestrates the audio callback. It is the only place that touches all of:

- `APVTS` — DAW-automatable parameters.
- `InternalState` — non-automatable per-step data.
- `SequenceState` — the domain API that hides the APVTS/InternalState split.
- `Clock` / `DawSync` — timing.
- `NoteOffScheduler` — sample-accurate mono note-off timing.
- `PreviewMidiFifo` — UI-to-audio preview note queue.
- `PresetManager` / `MidiLearnRegistry` — user data.

Most new features will either start here or need to be wired through here.

### 2.2 State is split in two on purpose

This is the most important design decision in the app.

| Store | Owns | Why |
|-------|------|-----|
| **APVTS** | Global scale notes, clock mode/BPM/subdivision, sequence length, selected step, random/shift flags, output channel, user preferences, all mappable button triggers, note colours | Must be visible to the DAW for automation and plugin state recall. |
| **InternalState** | Per-step note data (64 × 8), substep lengths/current/selected indices, per-step modulations (7 slots), play-probability bools (64), view window, current index, last probability index | Too many parameters to expose as DAW automation lanes; also changes too fast during playback. ValueTree makes listener-based UI updates easy. |

`SequenceState` is the unified read/write API over both. UI code and most processor code should go through `SequenceState`, not directly to APVTS or InternalState.

**Why this split?** JUCE hosts impose limits on parameter counts and automation lanes. Exposing 64 steps × 8 substeps × multiple properties as parameters would be unwieldy and slow. APVTS holds the "small, automatable" surface; InternalState holds the "large, per-step" surface.

### 2.3 Domain objects are thin views

- `Sequence` holds no data. It reads from APVTS/InternalState and applies playback logic.
- `Step` is a lightweight view into one step of `InternalState`.
- `ScaleQuantizer` is a view over APVTS for scale/range state.

This means domain objects are cheap to construct but must not outlive the underlying stores.

---

## 3. Information Flow

### 3.1 Audio thread: `processBlock`

The audio callback is the heartbeat. It runs independently of the UI and must stay fast.

```
processBlock(buffer, midiMessages)
  ├─ drainVirtualKeyboard(midiMessages)       // standalone computer-keyboard input
  ├─ processMidiInput(midiMessages)           // learn, mappings, external clock
  ├─ midiMessages.clear()                     // we consume input, then generate output
  ├─ previewMidiFifo.drainInto(midiMessages)  // UI-triggered preview notes
  ├─ handleNoteOffScheduling(...)             // sample-accurate note-offs
  ├─ processClockForBlock(...)                // internal or DAW-sync pulses
  ├─ emitMidiClockPulses()                    // standalone MIDI clock out
  ├─ detectDawTransport()                     // auto-resume in DAW mode
  └─ for each pulse offset:
       tickSequencer(midiMessages, offset, blockSize)
```

`tickSequencer` is where MIDI is actually produced:

1. Send an immediate note-off for the previous note (mono voice).
2. Call `SequenceState::tick()` → `Sequence::tick()`.
3. `Sequence::tick()` advances the current step, advances the substep, checks probability, and either returns a stored quantized note or (in random mode) generates a random note.
4. Emit selected CC modulations, then a note-on.
5. Schedule a future note-off after `noteLength × samplesPerStep`.
6. Fire `onSequenceTick` callback so the UI can repaint.

### 3.2 UI thread: callbacks and AsyncUpdater

User interactions generally follow this path:

```
UI component callback
  ├─ calls SequenceState setter (e.g. setStepNote, setLength)
  │    ├─ writes APVTS parameter OR InternalState ValueTree property
  │    └─ InternalState notifies its listeners
  ├─ some callbacks call PluginProcessor helpers directly
  │    (preview MIDI, transport, preset loading)
  └─ ApplicationComponent::handleAsyncUpdate() repaints the world
```

`ApplicationComponent` is the central UI coordinator. It wires all component callbacks to the processor, listens to `onSequenceTick` / `onNoteOff` / `onMidiLearned` via `juce::AsyncUpdater`, and calls `MainScreenComponent::updateFromSequence()` to refresh the view.

### 3.3 Audio → UI notifications

The audio thread must not touch UI components directly. It uses `std::function` callbacks:

| Callback | Fired when | UI handler |
|----------|------------|------------|
| `onSequenceTick` | Sequencer advanced | `ApplicationComponent::triggerAsyncUpdate()` → `handleAsyncUpdate()` |
| `onNoteOff` | Scheduled note-off emitted | `MessageManager::callAsync()` to clear the active-note highlight |
| `onMidiLearned` | A new MIDI mapping was learned | `ApplicationComponent::triggerAsyncUpdate()` |
| `onPresetCycleUp/Down` | MIDI preset-cycle triggered | `MessageManager::callAsync()` to cycle presets |
| `onPlayPauseTriggered` / `onStopTriggered` | Transport triggered by MIDI | `MessageManager::callAsync()` to update button state |

Always make local copies of these callbacks before calling them (the processor does this everywhere) to avoid races with the UI detaching them.

### 3.4 UI → Audio preview MIDI

When the user clicks a step or moves a note slider, the UI wants to hear the note immediately. In **standalone** mode, `PluginProcessor` has an open `juce::MidiOutput` and can send directly. In **plugin** mode, the UI cannot write to the audio `MidiBuffer`, so it pushes a preview event into `PreviewMidiFifo`. The next `processBlock` drains the FIFO into `midiMessages`.

### 3.5 State persistence

Full state is saved/loaded as XML with a root `<GamblerState>` element containing:

1. APVTS copy
2. InternalState ValueTree
3. Clock settings
4. MIDI Learn mappings

This same XML format is used for both DAW plugin state (`getStateInformation` / `setStateInformation`) and user presets (`PresetManager`).

---

## 4. Core Business Logic

### 4.1 Sequencer: `Sequence` / `Step`

`Sequence` is the playback engine. Its main entry point is `tick()`:

- Advances `currentIndex` modulo sequence length.
- Advances the active substep for that step.
- Checks probability (in random mode, a rest also stores `-1` for the generated note).
- Returns the quantized note, or `-1` for a rest.

`Sequence::tickStep()` is the non-random path: it just advances the substep and quantizes the stored note.

`Step` exposes substeps as `note(0..7)`, `substepCount()`, `currentSubstep()`, `selectedSubstep()`. There is an important distinction:

- `currentSubstep` = which substep will play next when the step is triggered.
- `selectedSubstep` = which substep the user is currently editing in the UI.

The two were conflated in earlier versions and are now separate. Editing the note sliders writes to `selectedSubstep`; playback reads from `currentSubstep`.

### 4.2 Scale, range, and quantization

The global scale is a 12-bit chromatic mask stored in APVTS. The range is stored as an offset + size window (`SCALE_WINDOW_OFFSET`, `SCALE_WINDOW_SIZE`) and exposed as `rangeMin` / `rangeMax`.

`ScaleQuantizer` provides two operations:

- `quantize(note)` — snap a raw note to the nearest enabled scale degree and clamp to range. Used during playback.
- `bake(note)` — same snapping, but intended for storage. Used when destructive scale/range modes are enabled, or when random mode generates a note.

The "destructive" preferences control whether edits permanently bake quantization into stored notes. When enabled, existing notes are re-baked via `Sequence::bakeAllExistingNotes()`.

### 4.3 Probability / Roll

Probability is not per-step probability in the traditional sense. It is driven by `EdenGrowthPattern`:

- The pattern lives on a grid of dimension 8×8, 4×4, or 2×2 (the "division level").
- The grid is tiled across the 64-step sequence, so each pattern cell controls a block of steps.
- The user sets a percentage of active cells. The pattern generator produces clustered "Eden growth" shapes.
- `Sequence::tick()` maps the current tick counter to a pattern cell via `tickToUnderlyingIndex()` and checks `playProbability(probIdx)`.

In **random mode** (Roll), the probability grid decides whether the step is a rest. If it plays, a new random note is generated from the range, baked, and optionally quantized live.

`currentProbabilityPercentage` and the pattern are regenerated whenever the probability percentage or division level changes.

### 4.4 Clock

`Clock` has three modes:

1. **INTERNAL** — counts samples using `InternalClock`. BPM and subdivision are user-controlled.
2. **DAW_SYNC** — receives PPQ position from `AudioPlayHead` via `DawSync`. The DAW is the master.
3. **EXTERNAL_MIDI_CLOCK** — counts incoming `0xF8` MIDI clock pulses and responds to Start/Stop/Continue.

`Clock` accumulates "pulses" (sub-block sample offsets) in `pendingPulseOffsets`. `processBlock` drains them and ticks the sequencer once per pulse.

`DawSync` is the subtle part. It:

- Detects playhead jumps (loop boundaries, manual seeks).
- Emits one pulse at offset `0` on a **backwards** jump so loop iterations don't drift by one step.
- Re-anchors on forward jumps without emitting catch-up pulses.
- Falls back to a generous threshold if the host doesn't report BPM.

### 4.5 Note-off scheduling

Gambler is mono: only one sequencer note can sound at a time. `NoteOffScheduler` manages this:

- `lastPlayedNote` tracks the currently sounding note.
- `sendImmediateNoteOff()` cancels any scheduled note-off and emits one now (used when the next step triggers).
- `scheduleNoteOff()` sets a future sample-accurate note-off based on the `noteLength` preference.
- `processScheduledNoteOff()` is called once per `processBlock` and emits the note-off if its sample time falls inside the block.

The preview path has a separate `sendPreviewNoteOff()` that either sends directly (standalone) or pushes into `PreviewMidiFifo` (plugin).

### 4.6 Modulations

Each step has 7 modulation slots. A slot can be:

- **Velocity** — overrides the default velocity (100).
- **CC** — emits a controller event at the same sample offset as the note-on.

Only slots marked `selected` are emitted. Modulation data lives in `InternalState`.

### 4.7 MIDI Learn

`MidiLearnRegistry` maps incoming MIDI messages (CC or Note) to a `MidiControlId`. The UI can enter "learn mode"; the next CC or note message is captured and assigned to the waiting control.

When a mapped message arrives in `processMidiInput`, `handleMidiMapping()` dispatches it. Many controls are handled by setting APVTS parameters (which then go through `parameterChanged()`), while continuous controls like note/octave/range update `SequenceState` directly.

The shift-aware buttons use `SEQ_IS_SHIFT` to decide whether to trigger the primary or secondary action.

### 4.8 Presets

`PresetManager` reads/writes XML files in the user's application-data directory (`~/Library/Application Support/Gambler/Presets` on macOS). Presets are the same `<GamblerState>` XML used for DAW state.

Loading a preset calls `PluginProcessor::loadStateFromXml()`, which replaces APVTS and InternalState. Because APVTS replacement fires `parameterChanged()`, clock settings are automatically applied to the `Clock` object.

---

## 5. UI Architecture

The UI is built from JUCE components. The key top-level pieces:

- `PluginEditor` — JUCE editor shell. Handles aspect ratio / compact mode.
- `ApplicationComponent` — owns the navbar, main screen, settings screen, preset selector, and all processor-facing callbacks.
- `MainScreenComponent` — the main sequencer view: piano roll, main controls, sequence overview, subsequencer, sequencer steps/controls, note range.
- `SettingsScreenComponent` — clock, preferences, MIDI output settings.

`ApplicationComponent::handleAsyncUpdate()` is the single place that updates the UI after audio or state changes. It calls `applyPendingNoteWrites()` first, then `MainScreenComponent::updateFromSequence()`.

Most subcomponents use callback lambdas rather than direct processor access. This keeps the UI tree decoupled but means `ApplicationComponent` is the central wiring point.

---

## 6. Platform Differences

### 6.1 Standalone vs plugin

In **standalone** mode:

- Clock defaults to INTERNAL and starts automatically.
- `PluginProcessor` opens the first available `juce::MidiOutput` device.
- MIDI clock output can be enabled in settings to send `0xF8` to all devices.
- File export uses `FileChooser::launchAsync`.

In **plugin** mode:

- Clock defaults to DAW_SYNC.
- MIDI is written to the `juce::MidiBuffer` provided by the host.
- Preview notes go through `PreviewMidiFifo` because there is no direct MIDI output.

### 6.2 iOS / AUv3

- AUv3 cannot present top-level windows. File export on iOS uses native share sheets (`iOSNativeDialogs`, `ContentSharer`) rather than `FileChooser`.
- Popup menus and dropdowns have platform-specific workarounds in `PlatformUtils`.
- iOS assertions are disabled in release builds (`JUCE_DISABLE_ASSERTIONS=1`).
- Standalone app installation also registers the AUv3 extension on the device.

See [`docs/no-bullshit-ios.md`](docs/no-bullshit-ios.md) and the remaining iOS guides for build/run procedures.

---

## 7. Gotchas & Important Caveats

These are the things most likely to bite you.

### 7.1 InternalState is touched from the audio thread

`InternalState` is a JUCE `ValueTree`. It is **not** designed as a real-time data structure. Reads and writes from `processBlock` allocate and lock. In practice the tree is small and operations are fast, but this is a known risk. If you add large per-step data or perform expensive mutations inside `Sequence::tick()`, you will eventually glitch.

### 7.2 There is no command queue anymore

Older docs mention a `ProcessorCommandQueue` pattern. That has been removed. The UI now writes directly into APVTS and InternalState. APVTS parameter access is atomic, but InternalState is not. Keep mutations small and avoid iterating the whole sequence in the audio callback.

### 7.3 Random-mode note writes cross the audio→message boundary

When `Roll` is on, the audio thread generates random notes and wants to store them. It does not write directly to `InternalState`; it pushes them into a lock-free `pendingNoteBuffer`. The UI thread drains this buffer in `ApplicationComponent::handleAsyncUpdate()` via `SequenceState::applyPendingNoteWrites()`. If the UI is not updating, pending writes accumulate and may be dropped when the ring buffer fills.

### 7.4 `processBlock` consumes incoming MIDI

The processor clears `midiMessages` after `processMidiInput()`. Input MIDI is used only for MIDI learn, external clock, and the virtual keyboard. It is **not** passed through to the output.

### 7.5 Note-offs are mono and scheduled

Only one scheduled note-off exists at a time. A new step trigger sends an immediate note-off for `lastPlayedNote` and schedules a new one. Long note lengths (> 1.0) are allowed and will produce overlapping note-ons, but the scheduler only tracks one note-off, so very long gates combined with fast sequences may behave unexpectedly.

### 7.6 DAW sync jump detection is heuristic

`DawSync` uses BPM-reported expected deltas and fallback thresholds. Hosts that lie about BPM, report jittery PPQ, or don't report BPM at all can cause missed or extra ticks. The backwards-jump-at-loop-boundary fix is essential; don't remove it without testing looped playback in multiple DAWs.

### 7.7 Clock mode param and `Clock` object must agree

The clock mode is stored in APVTS (`CLOCK_MODE`) and mirrored in the `Clock` object. `PluginProcessor::handleClockParam()` updates `Clock` when the param changes. The settings UI changes the param, not the clock directly. If you bypass `parameterChanged()` when setting clock mode, the two will diverge.

### 7.8 Preset loading happens on the message thread

`loadStateFromXml()` replaces APVTS and InternalState. This is intended to be called from the message thread (UI preset load). It is not safe to call from `processBlock`.

### 7.9 Preview MIDI ownership

`triggerStepMidi()` and `triggerCurrentStepMidi()` both write to `lastPlayedNote` and the preview path. Holding a step button down while moving a slider can race on `lastPlayedNote`. The code tries to cancel the slider gate timer when a step is pressed, but be cautious when adding new preview entry points.

### 7.10 Scale/range changes can invalidate stored notes

If you disable scale degrees or shrink the range, stored notes may no longer quantize to anything. `ScaleQuantizer::quantize()` returns `-1` in that case, producing a rest. Destructive modes re-bake notes on demand, but non-destructive mode relies on live quantization.

---

## 8. Where to Start When Working on...

| Task | Start here |
|------|------------|
| **Add a new parameter** | `source/data/Params.h` (add ID + layout), then `source/backend/SequenceState.h` and the UI component. |
| **Change playback behavior** | `source/backend/Sequence.h` (`tick`, `tickStep`), then `source/PluginProcessor.cpp` (`tickSequencer`). |
| **Change clock / sync** | `source/backend/Clock.h`, `source/backend/DawSync.h`, `source/PluginProcessor.cpp` (`handleClockParam`, `processClockForBlock`). |
| **Change scale / quantization** | `source/backend/ScaleQuantizer.h`, `source/backend/Scale.h`. |
| **Change probability / Roll** | `source/algorithms/probability/EdenGrowthPattern.h`, `source/backend/Sequence.h` (`setProbabilityPercentage`, `tick`). |
| **Add / wire a UI control** | The component header in `source/ui/...`, then `source/ui/ApplicationComponent.h` to wire callbacks and refresh. |
| **Add a preset field** | Ensure it lives in APVTS or InternalState; `getStateAsXml` / `loadStateFromXml` / `getStateInformation` / `setStateInformation` already save both stores. |
| **Add MIDI learn support** | `source/backend/MidiLearnRegistry/`, `source/backend/MidiMapping/`, and `handleMidiMapping()` in `PluginProcessor.cpp`. |
| **Add tests** | `tests/` uses Catch2. Add cases that exercise `SequenceState`, `Sequence`, `DawSync`, or `ScaleQuantizer`. |

---

## 9. Build & Test Quick Reference

```bash
# Configure and build standalone (macOS/Linux)
cmake -B build -G Ninja
cmake --build build --target Gambler2_Standalone

# Run tests
cmake --build build --target Tests
./build/Tests

# iOS (see docs/ for full details)
./build-ios.sh
```

Key CMake targets: `Gambler2_Standalone`, `Gambler2_VST3`, `Gambler2_AU`, `Gambler2_AUv3`, `Tests`, `Benchmarks`.

---

## 10. Related Documentation

- [`docs/no-bullshit-ios.md`](docs/no-bullshit-ios.md) — blunt iOS build/run cheat sheet.
- [`docs/iOS-Build-Guide.md`](docs/iOS-Build-Guide.md) — full iOS build walkthrough.
- [`docs/iOS-Quick-Reference.md`](docs/iOS-Quick-Reference.md) — iOS commands and config values.
- [`docs/CLion-iOS-Setup.md`](docs/CLion-iOS-Setup.md) — CLion iOS profile setup.
- [`docs/VSCODE_SETUP.md`](docs/VSCODE_SETUP.md) — VS Code tasks and launch configs.

If you are trying to understand a specific algorithm, read the header files linked above. This document is the map; the headers are the territory.
