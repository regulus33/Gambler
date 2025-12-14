# UI ↔ Backend Connection Plan

This document outlines the action plan for connecting the Gambler sequencer UI to the backend domain layer in a thread-safe manner, with DAW automation support for key parameters.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           PRESENTATION LAYER                                 │
│  PluginEditor                                                                │
│  ├── PianoRollComponent          → Scale (12 chromatic notes)               │
│  ├── NoteControlsComponent       → Note/Octave sliders (APVTS)              │
│  ├── ProbabilityControlsComponent → Probability slider (APVTS)              │
│  ├── NoteRangeControlsComponent  → Min/Max range sliders (APVTS)            │
│  ├── SequencerStepsComponent     → 8-step window into Sequence              │
│  ├── SequencerControlsComponent  → Roll, Shift, Index, Length, Nudge        │
│  ├── SubsequenceControlsComponent → StepNotes length/selection              │
│  └── SequenceOverviewComponent   → 64-step grid visualization               │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           APPLICATION LAYER                                  │
│  PluginProcessor                                                             │
│  ├── APVTS (AudioProcessorValueTreeState)                                   │
│  │   ├── DAW-automatable parameters (note, octave, probability, range)      │
│  │   └── UI state parameters (buttons, selections)                          │
│  ├── Clock (owns InternalClock, uses DawSync)                               │
│  ├── MidiOutput handling                                                     │
│  └── Sync methods: parameterChanged(), syncStateToApvts(), syncApvtsToState()│
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                             DOMAIN LAYER                                     │
│  Sequence                                                                    │
│  ├── Step[64]                                                               │
│  │   ├── StepNotes (up to 8 substeps)                                       │
│  │   │   └── StepNote (noteNumber, selected)                                │
│  │   └── StepModulations (7 slots)                                          │
│  │       └── StepModulation (CC/velocity data)                              │
│  ├── Scale (12 chromatic notes, rangeMin, rangeMax)                         │
│  ├── playProbability[64]                                                    │
│  └── tick() → returns MIDI note to play                                     │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Thread Safety Strategy

### Audio Thread (processBlock)
- **Reads**: Clock state, Sequence state for tick()
- **Writes**: Clock pulse detection, Sequence currentIndex advancement
- **Protection**: Use atomics for simple flags, lock-free queues for complex state changes

### Message Thread (UI)
- **Reads**: Sequence state for display
- **Writes**: User interactions (note changes, button clicks)
- **Protection**: APVTS handles thread-safe parameter changes; use AsyncUpdater for UI refresh

### Synchronization Pattern
```cpp
// In PluginProcessor
std::atomic<bool> sequenceNeedsRepaint{false};
juce::SpinLock sequenceLock;  // For complex multi-field updates

// Audio thread sets flag
sequenceNeedsRepaint.store(true);

// UI thread checks and repaints
if (sequenceNeedsRepaint.exchange(false)) {
    repaint();
}
```

---

## Component Action Plans

### 1. PianoRollComponent → Scale

**Current State**: UI-only, `onNoteChanged` callback logs to DBG

**Backend Connection**:
- PianoRoll toggles affect `Sequence::scale.scaleNotes[0-11]`
- Scale is NOT an APVTS parameter (too complex, stored in Sequence XML)

**Action Items**:
1. Add `Scale& getScale()` accessor to Sequence (already exists)
2. In PluginEditor constructor, wire up callback:
   ```cpp
   pianoRoll.onNoteChanged = [this]() {
       auto toggleStates = pianoRoll.getToggleStates();  // Add this method
       auto& scale = processorRef.getSequence().getScale();
       for (int i = 0; i < 12; i++) {
           scale.setScaleNote(i, toggleStates[i] ? 1 : 0);
       }
   };
   ```
3. Add `std::array<bool, 12> getToggleStates()` to PianoRollComponent
4. Add `void setToggleStates(const std::array<bool, 12>&)` for loading presets
5. On preset load, sync Scale → PianoRoll UI

**Thread Safety**: Scale is only modified from message thread (UI), read from audio thread during `tick()`. Use SpinLock for reads during randomization.

---

### 2. NoteControlsComponent → Current Step Note/Octave

**Current State**: NotchedSliders for Note (0-11) and Octave (-4 to +4), no backend connection

**Backend Connection**:
- These control the **currently selected step's** note value
- Must be APVTS parameters for DAW automation

**APVTS Parameters** (already defined in Params.h):
- `note` (0-127 MIDI note)
- `octave` (-4 to +4)

**Action Items**:
1. Create `SliderAttachment` in PluginEditor for both sliders:
   ```cpp
   std::unique_ptr<APVTS::SliderAttachment> noteAttachment;
   std::unique_ptr<APVTS::SliderAttachment> octaveAttachment;
   ```
2. Expose internal `juce::Slider&` from NoteControlsComponent's NotchedSliders
3. In `parameterChanged()`, update the selected step:
   ```cpp
   if (parameterID == Params::Keys::ID::NOTE) {
       int stepIdx = getSelectedStepIndex();
       sequence.getStep(stepIdx).updateCurrentNoteNumber(newValue);
   }
   ```
4. When step selection changes, update APVTS from step's current note
5. Add `selectedStepIndex` atomic to track which step is being edited

**Thread Safety**: APVTS handles thread-safe parameter changes. Step modification uses SpinLock.

---

### 3. ProbabilityControlsComponent → Step Probability

**Current State**: AnalogueSlider (0-100%), updates ProbabilityGauge visually

**Backend Connection**:
- Controls `Sequence::playProbability[selectedStepIndex]`
- Must be APVTS parameter for DAW automation

**APVTS Parameter** (already defined):
- `probability` (0-100)

**Action Items**:
1. Create `SliderAttachment` for probability slider
2. Expose `juce::Slider&` from AnalogueSlider (already has `getSlider()`)
3. In `parameterChanged()`:
   ```cpp
   if (parameterID == Params::Keys::ID::PROBABILITY) {
       int stepIdx = getSelectedStepIndex();
       bool shouldPlay = (newValue >= randomThreshold);  // Or use probability directly
       sequence.setPlayProbability(stepIdx, shouldPlay);
   }
   ```
4. Note: Current backend uses bool array, may need to change to float array for true probability

**Thread Safety**: Same as note controls.

---

### 4. NoteRangeControlsComponent → Scale Range

**Current State**: Two rotary sliders (min/max 0-127), updates NoteRangeGauge visually

**Backend Connection**:
- Controls `Sequence::scale.rangeMin` and `scale.rangeMax`
- Must be APVTS parameters for DAW automation

**APVTS Parameters** (already defined):
- `note_range_min` (0-127)
- `note_range_max` (0-127)

**Action Items**:
1. Create `SliderAttachment` for both range sliders
2. Expose sliders from NoteRangeControlsComponent:
   ```cpp
   juce::Slider& getMinRangeSlider() { return minRangeSlider; }
   juce::Slider& getMaxRangeSlider() { return maxRangeSlider; }
   ```
3. In `parameterChanged()`:
   ```cpp
   if (parameterID == Params::Keys::ID::NOTE_RANGE_MIN) {
       sequence.getScale().setRangeMin(static_cast<int>(newValue));
   }
   if (parameterID == Params::Keys::ID::NOTE_RANGE_MAX) {
       sequence.getScale().setRangeMax(static_cast<int>(newValue));
   }
   ```

**Thread Safety**: Scale range is read during `tick()` randomization. Use SpinLock.

---

### 5. SequencerStepsComponent → Sequence Display & Selection

**Current State**: Displays 8 steps from Sequence, has `onStepClicked` callback

**Backend Connection**:
- Reads step data from Sequence for display (note number, color)
- Click selects step for editing (updates selectedStepIndex)
- Active step indicator shows current playback position

**Action Items**:
1. Add `std::atomic<int> selectedStepIndex{0}` to PluginProcessor
2. Add `std::atomic<int> playbackStepIndex{-1}` to PluginProcessor
3. Wire up `onStepClicked`:
   ```cpp
   sequencerSteps.onStepClicked = [this](int stepIndex) {
       int absoluteIndex = windowOffset + stepIndex;
       processorRef.setSelectedStepIndex(absoluteIndex);
       // Update APVTS note/octave/probability from this step
       syncStepToApvts(absoluteIndex);
   };
   ```
4. Use Timer or AsyncUpdater to refresh display during playback
5. Add `windowOffset` tracking for 8-step window position

**Thread Safety**: 
- `playbackStepIndex` written by audio thread, read by UI
- `selectedStepIndex` written by UI, read by audio thread (for parameter sync)
- Both use atomics

---

### 6. SequencerControlsComponent → Sequence Operations

**Current State**: 8 buttons with `onButtonClicked` callback

**Backend Connection**:
| Button | Backend Method |
|--------|----------------|
| Roll | `sequence.setIsRandom(toggle)` |
| Shift | Modifier key (UI state only) |
| Index ← | `windowOffset--` (UI state) |
| Index → | `windowOffset++` (UI state) |
| Length + | `sequence.increaseLength()` |
| Length - | `sequence.decreaseLength()` |
| Nudge ← | `sequence.nudgeLeft()` |
| Nudge → | `sequence.nudgeRight()` |

**Action Items**:
1. Wire up each button callback to appropriate Sequence method
2. Roll button needs toggle state (use APVTS bool parameter)
3. Shift button is UI-only modifier
4. Index buttons modify `windowOffset` in PluginEditor
5. Length/Nudge buttons call Sequence methods directly

**Thread Safety**: Sequence modifications use SpinLock. Roll toggle uses atomic.

---

### 7. SubsequenceControlsComponent → StepNotes

**Current State**: Bar display (8 slots), +/- buttons, `onBarClicked` callback

**Backend Connection**:
- Displays/edits `Step::stepNotes` for selected step
- +/- buttons change `stepNotes.length`
- Bar click selects substep for editing

**Action Items**:
1. Add `std::atomic<int> selectedSubstepIndex{0}` to PluginProcessor
2. Wire up callbacks:
   ```cpp
   subsequenceControls.onBarClicked = [this](int index) {
       processorRef.setSelectedSubstepIndex(index);
   };
   subsequenceControls.onPlusClicked = [this]() {
       auto& stepNotes = getSelectedStep().getStepNotes();
       stepNotes.increaseLength();
   };
   ```
3. Update bar display when selected step changes
4. Add methods to SubsequenceControlsComponent for setting bar states

**Thread Safety**: StepNotes modifications use SpinLock.

---

### 8. SequenceOverviewComponent → 64-Step Grid

**Current State**: 8x8 grid display, Export/Save/Load buttons

**Backend Connection**:
- Grid shows all 64 steps with colors from note values
- Clicking grid cell could select that step
- Export/Save/Load handle preset management

**Action Items**:
1. Add `void updateFromSequence(const Sequence&)` to SequenceGrid
2. Wire up grid cell clicks to step selection
3. Implement Export button (MIDI file export)
4. Implement Save/Load buttons (preset files)
5. Use Timer to refresh grid during playback

**Thread Safety**: Read-only from Sequence, use SpinLock for consistent snapshot.

---

## Clock & MIDI Output Integration

### Clock Setup in PluginProcessor

```cpp
// Members
Clock clock;
DawSync dawSync;
std::atomic<bool> isPlaying{false};

// In prepareToPlay()
clock.setSampleRate(sampleRate);

// In processBlock()
if (clock.getMode() == ClockMode::DAW_SYNC) {
    auto position = getPlayHead()->getPosition();
    int pulseCount = dawSync.update(position);
    clock.updateDawSync(pulseCount);
    isPlaying.store(dawSync.getIsPlaying());
} else {
    clock.processSamples(buffer.getNumSamples());
    isPlaying.store(clock.isRunning());
}

if (clock.shouldTick()) {
    int midiNote = sequence.tick();
    if (midiNote >= 0) {
        // Send MIDI note on
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, midiNote, (uint8)100), 0);
        // Schedule note off (based on gate length)
    }
    playbackStepIndex.store(sequence.getCurrentIndex());
}
```

### Standalone MIDI Output

For USB MIDI testing:
1. Use `juce::MidiOutput::getAvailableDevices()` to list devices
2. Open selected device with `juce::MidiOutput::openDevice()`
3. Send MIDI messages directly instead of to buffer

---

## Implementation Order

### Phase 1: Core Parameter Binding (DAW Automation)
1. [ ] Add slider accessors to UI components
2. [ ] Create APVTS SliderAttachments in PluginEditor
3. [ ] Implement `parameterChanged()` for note, octave, probability, range
4. [ ] Add selectedStepIndex tracking

### Phase 2: Sequence Operations
5. [ ] Wire up SequencerControlsComponent buttons
6. [ ] Implement window offset for step display
7. [ ] Wire up SubsequenceControlsComponent

### Phase 3: Clock & Playback
8. [ ] Integrate Clock into processBlock
9. [ ] Add playback step indicator
10. [ ] Implement MIDI output

### Phase 4: Display Sync
11. [ ] Timer-based UI refresh during playback
12. [ ] PianoRoll ↔ Scale sync
13. [ ] SequenceOverview grid updates

### Phase 5: Persistence
14. [ ] Implement syncStateToApvts() for preset saving
15. [ ] Implement syncApvtsToState() for preset loading
16. [ ] Export/Save/Load button functionality

---

## Testing Strategy

### Standalone Mode Testing
1. Build standalone app
2. Connect USB MIDI device
3. Test internal clock playback
4. Verify MIDI output on each step

### DAW Integration Testing
1. Load as VST3/AU in DAW
2. Test DAW sync mode
3. Verify parameter automation recording
4. Test preset save/load

### Thread Safety Testing
1. Rapid UI interaction during playback
2. Parameter automation while playing
3. Preset switching during playback

