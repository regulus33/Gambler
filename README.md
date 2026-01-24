# Gambler - MIDI Step Sequencer Plugin

A 64-step MIDI sequencer plugin with probability-based note generation, substep sequences, and comprehensive MIDI learn support.

## Architecture Overview

The application follows a clean separation between backend domain logic, UI components, and the JUCE plugin framework. State is backed by JUCE's AudioProcessorValueTreeState (APVTS) for thread-safe access and DAW automation support.

---

## Class Reference

### Core Plugin Classes

#### `PluginProcessor`
The main audio processor and application controller. Owns the APVTS (single source of truth for all state), Clock, DawSync, PresetManager, and MidiLearnRegistry. Handles audio processing, MIDI input/output, command queue execution, and coordinates between UI and audio threads. This is the central hub of the application.

#### `PluginEditor`
JUCE AudioProcessorEditor that creates and manages the UI. Handles window resizing with aspect ratio constraints, keyboard shortcuts, and hosts the ApplicationComponent. Provides access to the Melatonin Inspector for UI debugging.

---

### Backend Classes

#### `Clock`
Master timing mechanism that determines when the sequencer should advance. Supports two modes: DAW_SYNC (synchronizes with host tempo) and INTERNAL (uses user-specified BPM). Delegates to InternalClock for internal timing and receives DAW position from DawSync.

#### `ClockMode` (enum + Subdivision enum)
Simple enums defining clock modes (DAW_SYNC, INTERNAL) and note subdivisions (QUARTER, EIGHTH, SIXTEENTH, THIRTY_SECOND). Also provides a helper function for pulses-per-quarter-note conversion.

#### `DawSync`
Handles DAW synchronization by interfacing with JUCE's AudioPlayHead. Converts PPQ position to pulse counts based on subdivision. Detects playhead jumps and transport state changes. Not a pure domain object - contains JUCE-specific code.

#### `InternalClock`
Pure domain object for internal timing based on BPM and sample counts. Accumulates samples and triggers pulses at calculated intervals. No JUCE dependencies.

#### `Sequence`
**⚠️ POTENTIALLY REDUNDANT**: Master orchestrator containing 64 Steps, Scale, and play probability array. Provides tick(), nudge, shuffle, and randomize operations. However, most of this functionality is now duplicated in `SequenceState` which is the actual source of truth. Consider deprecating this class and using SequenceState exclusively.

#### `SequenceState`
APVTS-backed sequencer state accessor providing the same interface as Sequence but reading/writing directly to APVTS parameters. This is the actual single source of truth for all sequencer state. Thread-safe and supports DAW automation. **This is the preferred class for state access.**

#### `Step`
A single step in the sequence containing StepNotes (substep sequence) and StepModulations (CC/velocity data). Coordinates tick behavior and note randomization. **⚠️ POTENTIALLY REDUNDANT**: With APVTS-backed state, this class may be unnecessary as SequenceState handles step data directly.

#### `StepNotes`
Sub-sequence of up to 8 notes within a Step that cycle on each trigger. Creates evolving pattern variations. **⚠️ POTENTIALLY REDUNDANT**: SequenceState now manages substep data directly via APVTS parameters.

#### `StepNote`
Simple data container for a single MIDI note with selection state. Supports rest values (-1). **⚠️ POTENTIALLY REDUNDANT**: Data is now stored in APVTS parameters.

#### `StepModulations`
Container for 7 modulation slots (CC/velocity) per step. First slot defaults to velocity, rest to CC 1-6. **⚠️ POTENTIALLY REDUNDANT**: SequenceState manages modulation data directly.

#### `StepModulation`
Individual modulation data (CC or Velocity) with type, CC number, value, and selection state.

#### `Scale`
Handles note quantization and range limiting. Manages which chromatic notes are enabled and provides quantization for random note generation. Used by both Sequence and SequenceState.

#### `PresetManager`
Handles preset storage, loading, and saving as XML files. Stores presets in the user's application data directory. Provides preset listing, save, load, and delete operations.

#### `ProcessorCommand` / `ProcessorCommandQueue`
Command pattern for thread-safe UI-to-audio-thread communication. Commands are queued by UI thread and executed in processBlock(). Supports completion callbacks that run on the message thread after command execution.

#### `MidiLearnRegistry`
Central registry for MIDI learn mode and user-configurable MIDI mappings. Manages learn state machine, user mappings, and control registration. Supports XML serialization for preset persistence.

#### `MidiMapping` / `MidiControlId` (enum)
Data structures for MIDI mapping system. MidiControlId enumerates all ~150 controllable UI elements. MidiMapping maps MIDI messages to controls with value transformation support.

#### `MidiCommandFactory`
Creates ProcessorCommands for MIDI input that match UI component commands. Ensures MIDI and UI follow the same code path for state changes.

#### `MidiUIUpdateCallbacks`
Registry of UI update callbacks for MIDI-controlled elements. When MIDI changes state, these callbacks update UI to reflect changes (button animations, slider positions).

#### `UserPreferences`
Stores user customization settings (preview notes on edit, follow step, note length, MIDI clock output). Persisted with plugin state but separate from domain model.

#### `Params` (namespace)
Comprehensive APVTS parameter definitions. Contains ~2500 parameter IDs and labels for all sequencer state. Provides createDefaultParameterLayout() for APVTS initialization.

---

### UI Components

#### `ApplicationComponent`
Top-level component orchestrating screens (MainScreen, SettingsScreen). Handles processor callbacks, async updates, preset management, and MIDI learn mode toggling. Implements AsyncUpdater for thread-safe UI updates.

#### `MainScreenComponent`
The main sequencer screen displaying NavBar, PianoRoll/MidiDataView, SequencerSteps, SequencerControls, MainControls, SequenceOverview, SubsequenceControls, and NoteRangeControls. Wires up all callbacks between components and processor.

#### `StateListenerComponent`
Base class for UI components that observe APVTS state. Provides automatic parameter listening with thread-safe UI updates via AsyncUpdater. Batches multiple rapid changes into single repaints.

#### `MidiMappableComponent`
Base class for MIDI-mappable UI components. Provides automatic registration with MidiLearnRegistry, overlay rendering during learn mode, and visual feedback for mapped controls.

#### `NavBarComponent`
Top navigation bar with logo, preset selector, view toggle, play/pause/stop controls, MIDI learn button, and settings button.

#### `PianoRollComponent`
Piano keyboard for scale selection. Displays 12 chromatic keys that users can toggle to define the scale. Shows active note indicator.

#### `SequencerStepsComponent`
Displays 8 visible steps (window into 64-step sequence). Contains 8 StepButton components. Handles step click callbacks.

#### `StepButton`
Individual step button showing note color, selection state, and playback position. Supports MIDI learn overlay. Listens to APVTS parameters for automatic updates.

#### `SequencerControlsComponent`
Container for sequencer control buttons (Roll, Page Left/Right, Length +/-, Nudge, Shift, and shift-mode buttons).

#### `SequencerControlButton`
Base class for APVTS-driven sequencer control buttons. mouseDown/mouseUp set APVTS parameters; visual state is driven by parameter listening.

#### `MainControlsComponent`
Container for NoteControlsComponent and ProbabilityControlsComponent.

#### `NoteControlsComponent`
Note and octave sliders for editing the selected step's note.

#### `ProbabilityControlsComponent`
Probability slider and ProbabilityGauge for controlling random note generation.

#### `ProbabilityGauge`
8x8 grid of colored circles visualizing probability distribution. Highlights the randomly selected circle during roll mode.

#### `SequenceOverviewComponent`
Container for SequenceGrid and SequenceOverviewControls. Provides the 8x8 overview of the full 64-step sequence.

#### `SequenceGrid`
8x8 grid of colored squares representing the full sequence. Each square shows note color, playback position (border), and selection state. Rows outside the current viewing window are darkened.

#### `SubsequenceControlsComponent`
Controls for editing substep sequences within a step. Contains 8 SubstepButton components for the substep sequence.

#### `SubstepButton`
Individual substep button within a step's substep sequence. Shows note color and selection state.

#### `NoteRangeControlsComponent`
Min/max note range sliders for constraining random note generation.

#### `SettingsScreenComponent`
Settings screen with clock mode, BPM, subdivision, and user preference controls.

#### `AnalogueSlider`
Continuous rotary slider with vertical label, hollow knob, value indicator, and percentage display.

#### `NotchedSlider`
Discrete rotary slider with colored dots around the knob for each option. Used for note and octave selection.

#### `ToggleIconButton`
Generic toggleable icon button that displays different icons based on toggle state. Used for settings button (gear/X), view toggle, etc.

#### `Icons` (static class)
Centralized icon system providing static methods to create juce::Path icons. Contains ~50 icon definitions (play, pause, stop, dice, gear, etc.) with configurable size and color.

#### `LogoComponent`
Displays the Gambler logo in the navigation bar.

#### `PresetSelectorComponent`
Preset management UI with dropdown selector, save button, and delete functionality.

#### `PresetDropdownComponent`
Dropdown menu for selecting presets from the available list.

#### `SavePresetDialogComponent`
Modal dialog for entering a name when saving a new preset.

#### `SettingsWindowComponent`
Floating window container for settings content. Provides window chrome and positioning.

#### `MidiDataPerStepSelectionComponent`
UI for viewing/editing MIDI CC and velocity data per step. Alternative view to the piano roll.

#### `VerticalBarSlider`
Vertical bar-style slider used for modulation/velocity editing in MidiDataPerStepSelectionComponent.

#### `UIConstants` (namespace)
Shared UI constants (colors, dimensions, spacing) used across components.

#### `MusicConstants` (namespace)
Music theory constants including note names, colors for chromatic notes, and octave definitions.

---

### Button Components (source/ui/buttons/)

All button classes extend `SequencerControlButton` and implement specific sequencer actions:

#### `RollButton`
Toggles roll/probability mode for the selected step.

#### `PageLeftButton` / `PageRightButton`
Navigate the 8-step viewing window through the 64-step sequence.

#### `LengthPlusButton` / `LengthMinusButton`
Adjust the sequence length (1-64 steps).

#### `NudgeLeftButton` / `NudgeRightButton`
Shift all notes in the sequence left or right by one step.

#### `ShiftButton`
Modifier button that changes behavior of other buttons when held.

#### `ShuffleButton`
Randomizes the order of existing notes in the sequence.

#### `RandomizeStepButton`
Generates a random note for the selected step based on scale and range settings.

#### `RandomizeAllButton`
Generates random notes for all steps in the sequence.

#### `ClearStepButton`
Clears the selected step (sets to rest).

#### `ClearWindowButton`
Clears all 8 steps in the current viewing window.

#### `DuplicateButton`
Duplicates the current 8-step window to the next window position.

#### `StepButton`
Individual step button in the sequencer. Shows note color, selection state, playback position, and substep indicator.

---

### Utility Classes

#### `ComponentSnapshotUtility`
Development utility for capturing screenshots of UI components for documentation.

---

### Data Flow

1. **UI → Processor**: UI components create `ProcessorCommand` objects and queue them via `ProcessorCommandQueue`. Commands execute in `processBlock()` on the audio thread.

2. **Processor → UI**: State changes update APVTS parameters. `StateListenerComponent` subclasses listen to parameters and trigger `AsyncUpdater` for thread-safe UI updates.

3. **MIDI → Processor**: MIDI input is processed in `processBlock()`. `MidiLearnRegistry` maps MIDI messages to controls. `MidiCommandFactory` creates commands matching UI behavior.

4. **MIDI → UI**: `MidiUIUpdateCallbacks` registry notifies UI components of MIDI-triggered state changes for visual feedback.

---

### Key Design Patterns

- **Observer Pattern**: StateListenerComponent provides automatic APVTS parameter observation with batched UI updates
- **Command Pattern**: ProcessorCommand/ProcessorCommandQueue for thread-safe UI-to-audio communication
- **Single Source of Truth**: APVTS holds all state; SequenceState provides typed access
- **Composition over Inheritance**: UI components compose smaller components rather than deep inheritance hierarchies

---

### Build & Development

Built with JUCE 8.x framework. Uses CMake for build configuration.

```bash
# Configure
cmake -B build -G Ninja

# Build
cmake --build build

# Run standalone
./build/Gambler_artefacts/Standalone/Gambler.app/Contents/MacOS/Gambler
```

#### VS Code Setup

For a CLion-like experience in VS Code, install these extensions:

- **CMake Tools** (`ms-vscode.cmake-tools`) - GUI building, target selection, debugging
- **clangd** (`llvm-vs-code-extensions.vscode-clangd`) - Fast indexing and Go to Definition

System dependencies (Ubuntu/Debian):
```bash
sudo apt install clangd ninja-build
```

**First-time setup:**
```bash
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Building from the GUI:**
- Use the status bar at the bottom of VS Code:
  - Click the **Build** button (or press `F7`)
  - Click the **target selector** to choose `Gambler2_Standalone`, `Gambler2_VST3`, etc.
  - Click the **build variant** to switch between Debug/Release
- Or use Command Palette (`Ctrl+Shift+P`):
  - "CMake: Build" - build current target
  - "CMake: Build Target" - pick a specific target
  - "CMake: Set Build Target" - change default target

**Go to Definition / Indexing:**
- clangd uses `build/compile_commands.json` for indexing
- First indexing may take a few minutes (JUCE is large)
- Subsequent loads are fast due to background indexing
- If intellisense breaks, run "CMake: Configure" to regenerate compile_commands.json

**Tips:**
- Disable Microsoft C++ IntelliSense when using clangd (VS Code should prompt you)
- Use `Ctrl+Click` or `F12` for Go to Definition
- Use `Ctrl+Shift+O` for symbol search in current file
- Use `Ctrl+T` for workspace-wide symbol search

---

### File Organization

```
source/
├── backend/           # Domain logic (Clock, Sequence, Scale, MIDI, etc.)
├── data/              # Data structures and APVTS parameter definitions
├── ui/                # UI components
│   ├── buttons/       # Button components (SequencerControlButton, etc.)
│   └── ...            # Other UI components
├── PluginProcessor.cpp/h
└── PluginEditor.cpp/h
```
