# Gambler

A MIDI sequencer plugin with probabilistic and generative features, inspired by modules like Turing Machine and Music Thing Modular's Turing Machine.

## Architecture

### Layered Architecture (Option 3)

```
┌─────────────────────────────────────┐
│  Presentation Layer (UI Thread)     │
│  - JUCE Components                  │
│  - Button/Slider listeners          │
└──────────────┬──────────────────────┘
               │ Commands/Queries
┌──────────────▼──────────────────────┐
│  Application Layer                  │
│  - AudioProcessor (Controller)      │
│  - Command handlers                 │
│  - APVTS (persistence adapter)      │
└──────────────┬──────────────────────┘
               │ Method calls
┌──────────────▼──────────────────────┐
│  Domain Layer (Thread-safe)         │
│  - Controls (aggregate root)        │
│  - Sequence, Note, etc.             │
│  - Pure C++, no JUCE dependencies   │
└─────────────────────────────────────┘
```

**Key Principles:**
- **Domain Layer**: Pure C++ business logic, fully testable without JUCE
- **Application Layer**: PluginProcessor acts as controller, syncing APVTS ↔ Controls
- **Presentation Layer**: JUCE UI components (to be implemented)
- **Persistence**: APVTS handles all parameter state saving/loading
- **Thread Safety**: Clear boundaries for adding locks/atomics when needed

## Sequencer Capabilities

### Current Features

#### 1. **Sequence Management**
- **64 steps** by default (configurable length)
- Each step contains a `Note` object with:
  - MIDI note value (0-127)
  - Subsequence of up to 8 notes for evolving patterns
  - 7 configurable slots for velocity/CC data

#### 2. **Note Subsequences**
- Each step can have **1-8 substep notes** that cycle through on each sequence pass
- Creates **evolving patterns** where steps change slightly on each loop
- Similar to Turing Machine's probabilistic evolution
- Substeps are triggered when their parent step triggers (not by faster clock divisions)

#### 3. **Roll (Randomization)**
- **Roll button** in Controls triggers note randomization
- When Roll is pressed, `Sequence.tick()` randomizes the current step's note (0-127)
- Allows for generative, unpredictable sequences
- Can be toggled on/off via Controls

#### 4. **Scale Quantization** (In Progress)
- `State` contains a 12-position scale array
- Each position represents a chromatic note: C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
- Values of 1 = note is in scale, 0 = note is excluded
- Example: C Major = `[1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1]`
- **Next step**: Implement quantization logic to constrain randomized notes to the active scale

#### 5. **State Integration**
- `State` object acts as the central domain state container
- Contains all application state:
  - 13 button states (Roll, Shift, Index navigation, etc.)
  - 3 main controls (Octave, Note, Probability)
  - MIDI mode (CC_TO_NOTE, NOTE_TO_CC)
  - Note range (min/max)
  - 8 step buttons (selected/highlighted states)
  - 7 CC slots (number + value for each)
  - 12-note scale array for quantization

#### 6. **Slot System**
- Each Note has **7 slots** for additional MIDI data
- Slot 0: Defaults to Velocity
- Slots 1-6: Default to CC (configurable CC numbers and values)
- Each slot can be configured for different CC types
- Supports CC values 0-127

### Architecture Details

#### Domain Objects

**`Sequence`**
- Manages 64 Note objects
- Handles sequence playback via `tick()`
- Integrates with State for Roll functionality and scale quantization
- Configurable sequence length with wrapping

**`Note`**
- Contains a vector of 1-8 MIDI note values (subsequence)
- Has its own internal index for cycling through substeps
- Contains 7 Slot objects for velocity/CC data
- Validates MIDI note values (0-127)

**`State`**
- Central domain state container for all application state
- Provides clean getter/setter API
- Validates all input values
- Contains scale quantization data (12-note array)
- No JUCE dependencies (pure C++)
- The actual UI controls interface will be closer to the UI layer

**`Slot`**
- Represents velocity or CC data
- Configurable type (Velocity or CC)
- Validates CC numbers (0-127) and values (0-127)

### Testing

All domain logic is fully tested using **Catch2** with the `--reporter rspec` option:

```bash
./cmake-build-debug/Tests --reporter rspec
```

**Test Coverage:**
- ✅ Note subsequence behavior and cycling
- ✅ Sequence length and wrapping
- ✅ Roll button randomization
- ✅ Scale configuration via State
- ✅ Slot configuration and validation
- ✅ State management
- ✅ MIDI note validation

### Next Steps

1. **Implement scale quantization logic** in Sequence.tick()
2. **Build UI components** for the Presentation Layer
3. **Add thread safety** (atomics/locks) for audio thread access
4. **Implement MIDI output** in AudioProcessor
5. **Add clock/tempo synchronization**
6. **Implement probability controls** for generative behavior

# State recall and persistence (basic not counting presets yet)
Basically we get callbacks sent to `PluginProcessor`
The main things to look at are what bitwig calls in save and load of projects
`PluginProcessor::getStateInformation`
`PluginProcessor::setStateInformation`

All we are doing is loading and saving state as xml.
... 