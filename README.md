# Gambler2

A JUCE-based MIDI step sequencer plugin with probabilistic note generation, scale quantization, and per-step substep editing.

**Formats:** Standalone, AU, VST3, AUv3, CLAP
**Company:** ToneGarden

## Build

```bash
cmake -B build && cmake --build build                          # full build
cmake --build build --target Gambler2_Standalone                # standalone only
cmake --build build --target Tests && ./build/Tests             # run all tests
UPDATE_SNAPSHOTS=1 ./build/Tests "[visual]"                    # update visual goldens
```

Requires C++23, JUCE 8.0.x, ninja.

---

## UI Component Inventory

### Component Hierarchy

```
ApplicationComponent
├─ MainScreenComponent
│  ├─ NavBarComponent
│  │  ├─ PresetSelectorComponent
│  │  │  ├─ PresetDropdownComponent
│  │  │  └─ PresetCycleButton (×2)
│  │  ├─ LogoComponent
│  │  ├─ ToggleIconButton (×6: MIDI Learn, Settings, Play/Pause, Stop, Compact, Swap)
│  │  ├─ ScaleLatchButton
│  │  ├─ ExportButton
│  │  └─ SubdivisionSlider
│  ├─ PianoRollComponent
│  ├─ MainControlsComponent
│  │  ├─ NoteControlsComponent
│  │  │  ├─ OctaveSlider (NotchedSlider)
│  │  │  └─ NoteSlider (NotchedSlider)
│  │  └─ ProbabilityControlsComponent
│  │     ├─ AnalogueSlider (ProbabilitySlider)
│  │     └─ ProbabilityGauge
│  ├─ SequenceOverviewComponent
│  │  └─ SequenceGrid
│  ├─ SequencerStepsComponent
│  │  └─ StepButton (×8)
│  ├─ SequencerControlsComponent
│  │  ├─ RollButton
│  │  ├─ ShiftButton
│  │  ├─ Normal mode: PageLeft, PageRight, Length+, Length−, NudgeLeft, NudgeRight
│  │  └─ Shift mode:  Duplicate, ClearWindow, ClearStep, RandAll, RandStep, Shuffle
│  ├─ SubsequenceControlsComponent
│  │  ├─ SubsequenceBar → SingleBar (×8)
│  │  └─ SubsequenceButton (−/+)
│  └─ NoteRangeControlsComponent
│     ├─ NoteRangeGauge
│     └─ Slider (offset/size)
│
└─ SettingsScreenComponent
   ├─ NavBarComponent (shared)
   ├─ GlobalScaleQuantizerComponent
   ├─ NoteColourPickerComponent
   └─ SettingsGroup (multiple)
```

### Main Screens

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| ApplicationComponent | `source/ui/ApplicationComponent.h` | ![](tests/snapshots/application_component.png) | Top-level orchestrator; swaps between main and settings screens |
| MainScreenComponent | `source/ui/MainScreenComponent.h` | ![](tests/snapshots/main_screen.png) | Primary sequencer workspace with all controls |
| SettingsScreenComponent | `source/ui/SettingsScreenComponent.h` | ![](tests/snapshots/settings_screen.png) | Full-screen settings (Clock, Playback, MIDI config) |
| PluginEditor | `source/PluginEditor.h` | ![](tests/snapshots/plugin_editor.png) | JUCE editor wrapper; handles keyboard input and responsive scaling |

### Navigation Bar

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| NavBarComponent | `source/ui/NavBarComponent.h` | ![](tests/snapshots/nav_bar.png) | Top bar with preset selector, transport, tools, settings |
| LogoComponent | `source/ui/LogoComponent.h` | ![](tests/snapshots/logo.png) | "Gambler" text logo with dice icon |
| PresetSelectorComponent | `source/ui/PresetSelectorComponent.h` | ![](tests/snapshots/preset_selector.png) | Dropdown + cycle buttons for preset navigation |
| PresetDropdownComponent | `source/ui/PresetDropdownComponent.h` | ![](tests/snapshots/preset_dropdown.png) | ComboBox-based preset selection |
| SavePresetDialogComponent | `source/ui/SavePresetDialogComponent.h` | ![](tests/snapshots/save_preset_dialog.png) | Modal dialog for saving presets |
| ExportButton | `source/ui/NavBarComponent.h` | — | Click-to-save or drag-to-DAW MIDI export |
| ScaleLatchButton | `source/ui/ScaleLatchButton.h` | ![](tests/snapshots/scale_latch_button.png) | Toggle momentary/latched scale editing |
| SubdivisionSlider | `source/ui/sliders/SubdivisionSlider.h` | ![](tests/snapshots/subdivision_slider.png) | Clock division selector (1/4, 1/8, 1/16, 1/32) |
| ToggleIconButton | `source/ui/ToggleIconButton.h` | ![](tests/snapshots/toggle_icon_button.png) | Generic two-state icon button (used for Settings, Play, Stop, etc.) |

### Piano Roll & Scale

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| PianoRollComponent | `source/ui/PianoRollComponent.h` | ![](tests/snapshots/piano_roll.png) | 12-key chromatic keyboard for global scale selection |
| GlobalScaleQuantizerComponent | `source/ui/GlobalScaleQuantizerComponent.h` | ![](tests/snapshots/global_scale_quantizer.png) | Scale type dropdown + preset scale buttons (settings) |
| NoteColourPickerComponent | `source/ui/NoteColourPickerComponent.h` | ![](tests/snapshots/note_colour_picker.png) | Per-note colour customization (settings) |

### Main Controls

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| MainControlsComponent | `source/ui/MainControlsComponent.h` | ![](tests/snapshots/main_controls.png) | Container for note/octave + probability controls |
| NoteControlsComponent | `source/ui/NoteControlsComponent.h` | ![](tests/snapshots/note_controls.png) | Bordered panel with OctaveSlider + NoteSlider |
| OctaveSlider | `source/ui/sliders/OctaveSlider.h` | — | Discrete rotary for octave (0-9) |
| NoteSlider | `source/ui/sliders/NoteSlider.h` | — | Discrete rotary for pitch class (C-B), scale-filtered |
| NotchedSlider | `source/ui/sliders/NotchedSlider.h` | — | Base discrete rotary with colored dot notches |
| ProbabilityControlsComponent | `source/ui/ProbabilityControlsComponent.h` | ![](tests/snapshots/probability_controls.png) | Probability slider + visual gauge |
| AnalogueSlider | `source/ui/sliders/ProbabilitySlider.h` | — | Continuous rotary for probability (0-100%) |
| ProbabilityGauge | `source/ui/ProbabilityGauge.h` | ![](tests/snapshots/probability_gauge.png) | 8×8 grid visualizing random probability distribution |

### Sequencer

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| SequencerStepsComponent | `source/ui/SequencerStepsComponent.h` | ![](tests/snapshots/sequencer_steps.png) | 8 visible steps (window into 64-step sequence) |
| StepButton | `source/ui/buttons/StepButton.h` | ![](tests/snapshots/step_button.png) | Individual step: shows note name, selection state, playhead |
| SequencerControlsComponent | `source/ui/SequencerControlsComponent.h` | ![](tests/snapshots/sequencer_controls.png) | 8-button row with normal/shift modes |
| SequenceOverviewComponent | `source/ui/SequenceOverviewComponent.h` | ![](tests/snapshots/sequence_overview.png) | 8×8 grid overview of entire 64-step sequence |
| SequenceGrid | `source/ui/SequenceGrid.h` | ![](tests/snapshots/sequence_grid.png) | Clickable grid for navigation and length adjustment |

### Sequencer Control Buttons

All inherit from `SequencerControlButton` (`source/ui/buttons/SequencerControlButton.h`).

**Normal Mode (Shift OFF):**

| Pos | Button | File | APVTS Param | Function |
|-----|--------|------|-------------|----------|
| 0 | RollButton | `buttons/RollButton.h` | SEQ_IS_RANDOM | Momentary random mode |
| 1 | PageLeftButton | `buttons/PageLeftButton.h` | INDEX_LEFT | Page window left 8 steps |
| 2 | PageRightButton | `buttons/PageRightButton.h` | INDEX_RIGHT | Page window right 8 steps |
| 3 | LengthPlusButton | `buttons/LengthPlusButton.h` | LENGTH_PLUS | Increase sequence length |
| 4 | LengthMinusButton | `buttons/LengthMinusButton.h` | LENGTH_MINUS | Decrease sequence length |
| 5 | NudgeLeftButton | `buttons/NudgeLeftButton.h` | NUDGE_LEFT | Shift all notes left |
| 6 | NudgeRightButton | `buttons/NudgeRightButton.h` | NUDGE_RIGHT | Shift all notes right |
| 7 | ShiftButton | `buttons/ShiftButton.h` | SEQ_IS_SHIFT | Toggle shift mode |

**Shift Mode (Shift ON) — buttons 1-6 swap:**

| Pos | Button | File | APVTS Param | Function |
|-----|--------|------|-------------|----------|
| 1 | DuplicateButton | `buttons/DuplicateButton.h` | DUPLICATE | Double length + copy |
| 2 | ClearWindowButton | `buttons/ClearWindowButton.h` | CLEAR_WINDOW | Clear visible 8 steps |
| 3 | ClearStepButton | `buttons/ClearStepButton.h` | CLEAR_STEP | Clear selected step |
| 4 | RandomizeAllButton | `buttons/RandomizeAllButton.h` | RANDOMIZE_ALL | Randomize all steps |
| 5 | RandomizeStepButton | `buttons/RandomizeStepButton.h` | RANDOMIZE_STEP | Randomize current step |
| 6 | ShuffleButton | `buttons/ShuffleButton.h` | SHUFFLE | Shuffle step order |

Shift-mode buttons share MIDI mappings with their normal-mode counterparts.

### Subsequence & Note Range

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| SubsequenceControlsComponent | `source/ui/SubsequenceControlsComponent.h` | ![](tests/snapshots/subsequence_controls.png) | Per-step substep editing (up to 8 variations) |
| NoteRangeControlsComponent | `source/ui/NoteRangeControlsComponent.h` | ![](tests/snapshots/note_range_controls.png) | MIDI note range window (offset + size sliders + gauge) |

### Settings

| Component | File | Snapshot | Description |
|-----------|------|----------|-------------|
| SettingsGroup | `source/ui/SettingsGroup.h` | ![](tests/snapshots/settings_group.png) | Reusable titled container for grouped settings |

### Base Classes & Utilities

| Class | File | Description |
|-------|------|-------------|
| StateListenerComponent | `source/ui/StateListenerComponent.h` | Base for all APVTS-observing components (thread-safe async updates) |
| Icons | `source/ui/Icons.h` | Centralized SVG path factory for all icons |
| UIConstants | `source/ui/UIConstants.h` | Global colour constants |
| MusicConstants | `source/ui/MusicConstants.h` | Note names, colours, octave labels |
| MidiLearnOverlayPainter | `source/ui/MidiLearnOverlayPainter.h` | Blue overlay drawn during MIDI learn mode |

---

## Backend Class Diagram

```mermaid
classDiagram
    direction TB

    %% ===== JUCE Base Classes =====
    class AudioProcessor {
        <<JUCE>>
    }
    class AudioProcessorEditor {
        <<JUCE>>
    }
    class ValueTree_Listener {
        <<JUCE>>
    }
    class APVTS_Listener {
        <<JUCE>>
    }

    %% ===== Plugin Core =====
    class PluginProcessor {
        -APVTS apvts
        -SequenceState sequenceState
        -InternalState internalState
        -Clock clock
        -DawSync dawSync
        -PresetManager presetManager
        -MidiLearnRegistry midiLearnRegistry
        -SynthEngine synthEngine
        +processBlock()
        +prepareToPlay()
        +togglePlayPause()
        +stopSequencer()
        +handleMidiMapping()
    }

    class PluginEditor {
        -PluginProcessor processorRef
        -ApplicationComponent applicationComponent
        +resized()
        +keyPressed()
        +getScaleFactor()
    }

    AudioProcessor <|-- PluginProcessor
    APVTS_Listener <|-- PluginProcessor
    AudioProcessorEditor <|-- PluginEditor
    PluginEditor --> PluginProcessor
    PluginEditor *-- ApplicationComponent

    %% ===== State Layer =====
    class SequenceState {
        -APVTS& apvts
        -InternalState& internalState
        -SPSCRingBuffer pendingNoteBuffer
        +getLength() / setLength()
        +getStepNote() / setStepNote()
        +tick() / tickStep()
        +quantizeNoteToGlobalScale()
        +getPlayProbability()
    }

    class InternalState {
        -ValueTree tree
        +getStepNote() / setStepNote()
        +getPlayProbability()
        +getNoteColour() / setNoteColour()
        +getState() / setState()
    }

    class Scale {
        -bool scaleNotes[12]
        -int rangeMin
        -int rangeMax
        +quantize()
        +isNoteInScale()
        +isNoteInRange()
    }

    ValueTree_Listener <|-- InternalState
    PluginProcessor *-- SequenceState
    PluginProcessor *-- InternalState
    SequenceState --> InternalState
    SequenceState --> Scale

    %% ===== Clock & Timing =====
    class Clock {
        -InternalClock internalClock
        -ClockMode mode
        +processInternalClock()
        +updateDawSync()
        +processExternalMidiClockPulse()
        +shouldTick()
    }

    class InternalClock {
        -double bpm
        -Subdivision subdivision
        -double sampleRate
        +processSamples() bool
        +start() / stop() / reset()
    }

    class DawSync {
        -Subdivision subdivision
        +update(PositionInfo)
        +calculatePulseCount()
    }

    PluginProcessor *-- Clock
    PluginProcessor *-- DawSync
    Clock *-- InternalClock

    %% ===== MIDI System =====
    class MidiLearnRegistry {
        -Map~MidiMessageIdentifier, MidiControlId~ userMappings
        -Map~MidiControlId, MappableControlInfo~ controls
        -bool learning
        +enterLearnMode() / exitLearnMode()
        +learnMidiMessage()
        +findUserMapping()
    }

    class MidiExporter {
        +createMidiFile(SequenceState)$
        +writeToFile()$
        +writeToTempFile()$
    }

    PluginProcessor *-- MidiLearnRegistry
    MidiExporter --> SequenceState

    %% ===== Presets =====
    class PresetManager {
        -Array~PresetInfo~ presets
        -String currentPresetName
        +savePreset() / loadPreset()
        +refreshPresetList()
        +deletePreset()
    }

    PluginProcessor *-- PresetManager

    %% ===== Synth Engine (conditional) =====
    class SynthEngine {
        -SynthVoice voices[8]
        -FDNReverb reverb
        -APVTS& apvts
        +prepare()
        +processBlock()
    }

    class SynthVoice {
        -VectorOscillator osc
        -ADSR ampEnv
        -ADSR filterEnv
        -LadderFilter filter
        +noteOn() / noteOff()
        +renderBlock()
    }

    class VectorOscillator {
        -double phase
        -float blend
        +setFrequency()
        +setBlend()
        +getNextSample()
    }

    class FDNReverb {
        -float fdnDelayBuffers[8]
        +prepare()
        +processBlock()
        +setParams()
    }

    PluginProcessor *-- SynthEngine
    SynthEngine *-- SynthVoice
    SynthEngine *-- FDNReverb
    SynthVoice *-- VectorOscillator

    %% ===== Enums =====
    class ClockMode {
        <<enum>>
        DAW_SYNC
        INTERNAL
        EXTERNAL_MIDI_CLOCK
    }

    class Subdivision {
        <<enum>>
        QUARTER
        EIGHTH
        SIXTEENTH
        THIRTY_SECOND
    }

    class MidiControlId {
        <<enum>>
        SCALE_NOTE_C...B
        OCTAVE_SLIDER
        NOTE_SLIDER
        STEP_0...7
        BUTTON_ROLL...SHIFT
        PLAY_PAUSE
        STOP
        SCALE_LATCH
        ~150+ controls
    }

    Clock --> ClockMode
    Clock --> Subdivision
    DawSync --> Subdivision
    InternalClock --> Subdivision
    MidiLearnRegistry --> MidiControlId
```

---

## Possible Dead Code

| Item | File(s) | Status | Notes |
|------|---------|--------|-------|
| `ScaleClearButton` class | `source/ui/ScaleClearButton.h` | **Orphaned** | Removed from NavBarComponent but file still exists. Not included anywhere |
| `CLEAR_SCALE` APVTS param | `source/data/Params.h:150`, `PluginProcessor.cpp:787` | **Orphaned** | Handler in PluginProcessor still active but no UI to trigger it |
| `createBarChartIcon()` | `source/ui/Icons.h` | **Unused** | Never called anywhere |
| `createCrossHairIcon()` | `source/ui/Icons.h` | **Unused** | Never called anywhere |
| `createKeyboardIcon()` | `source/ui/Icons.h` | **Unused** | Never called anywhere |
| `createRecordIcon()` | `source/ui/Icons.h` | **Unused** | Never called anywhere |
| `scale_clear_button.png` | `tests/snapshots/` | **Stale** | No corresponding test case in VisualSnapshotTests.cpp |
| `settings_window.png` | `tests/snapshots/` | **Stale** | No corresponding test case (SettingsWindowComponent test was removed) |
| `length_plus_sequence_overview.png` + ACTUAL/DIFF | `tests/snapshots/` | **Stale** | No corresponding test case in VisualSnapshotTests.cpp |

---

## Tests

```bash
cmake -B build && cmake --build build --target Tests && ./build/Tests
```

Visual snapshot tests live in `tests/VisualSnapshotTests.cpp`. Golden images are in `tests/snapshots/`.


#### Docs

 The .tex source is alongside it if you want to tweak anything. You can rebuild it with:                                                 
 `cd docs && pdflatex gambler-codebase-guide.tex && pdflatex gambler-codebase-guide.tex`
(two passes needed for the table of contents and cross-references)                                                                      
                                                                       
                                                                       ```