# Thread Safety Architecture

## Overview

Gambler uses a **command queue pattern** for thread-safe communication between the UI thread and audio thread. This prevents race conditions and ensures all domain state modifications happen on the audio thread.

## Communication Patterns

### UI Thread → Audio Thread (State Modifications)

**Problem**: UI callbacks were directly modifying domain objects (Sequence, Clock, Scale) that are also read by the audio thread in `processBlock()`, causing race conditions.

**Solution**: Use `ProcessorCommandQueue` to queue commands that execute on the audio thread.

```cpp
// UI thread queues a command
processorRef.queueCommand(ProcessorCommand([this]() {
    // This lambda executes on the audio thread
    processorRef.getSequence().increaseLength();
}));
```

**Implementation**:
- `ProcessorCommand.h` - Command pattern with `std::function<void()>`
- `ProcessorCommandQueue` - Lock-free FIFO using `juce::AbstractFifo`
- Commands are queued by UI thread via `PluginProcessor::queueCommand()`
- Commands are executed in `processBlock()` via `commandQueue.executeAll()`

### Audio Thread → UI Thread (Notifications)

**Pattern**: `AsyncUpdater` callback (already implemented)

```cpp
// Audio thread triggers notification
if (onSequenceTick) {
    onSequenceTick();  // Calls triggerAsyncUpdate()
}

// UI thread handles update on message thread
void PluginEditor::handleAsyncUpdate() {
    // Safe to read state and update UI here
    int currentStep = processorRef.getSequence().getCurrentIndex();
    setActiveStepAndNote(currentStep);
}
```

**Why this works**: `AsyncUpdater` posts a message to the message thread's queue. The UI thread then reads the current state when it's ready. No data is passed directly.

## State Modifications Using Command Queue

All these UI callbacks now use the command queue:

### Sequence Controls
- **Note/Octave changes** - `step.updateCurrentNoteNumber()`
- **Index navigation** - `seq.indexLeft()`, `seq.indexRight()`
- **Length changes** - `seq.increaseLength()`, `seq.decreaseLength()`
- **Nudge** - `seq.nudgeLeft()`, `seq.nudgeRight()`
- **Shift mode** - `seq.setIsShift()`
- **Random mode** - `seq.setIsRandom()` (Roll button)
- **Step selection** - `seq.setCurrentIndex()`
- **Sequence reset** - `seq.reset()`

### Substep Controls
- **Substep selection** - `stepNotes.setCurrentIndex()`
- **Substep length** - `stepNotes.increaseLength()`, `stepNotes.decreaseLength()`

### Scale Controls
- **Piano roll** - `state.setStepScaleNotes()` (per-step scales)
- **Note range** - `state.setScaleRangeMin()`, `state.setScaleRangeMax()` (global range)

### Clock Controls
- **Mode changes** - `clock.setMode()`
- **Tempo changes** - `clock.setBpm()`
- **Start/Stop** - `clock.start()`, `clock.stop()`

### User Preferences
- **Preview notes** - `userPreferences.previewNotesOnEdit` (direct access is safe, it's only read by UI thread)

## MIDI Preview (Special Case)

Preview MIDI (when user edits notes) uses a separate queue because MIDI messages need to be sent immediately in DAW mode:

```cpp
// UI thread queues preview MIDI
previewMidiFifo.prepareToWrite(1, start1, size1, start2, size2);
previewMidiBuffer[start1] = { note, true, velocity };
previewMidiFifo.finishedWrite(1);

// Audio thread consumes and sends MIDI
previewMidiFifo.prepareToRead(...);
midiMessages.addEvent(noteOn, 0);
previewMidiFifo.finishedRead(...);
```

**Why separate queue**: MIDI messages are time-sensitive and need to be sent in the next audio buffer, not just "eventually".

## Reading State from UI Thread

**Safe**: Reading state from UI thread is generally safe because:
1. Reads are atomic for simple types (int, bool, etc.)
2. We only read for display purposes, not for critical logic
3. Worst case: UI shows slightly stale data for one frame

**Examples**:
```cpp
// Safe to read for UI updates
int currentStep = processorRef.getSequence().getCurrentIndex();
sequencerSteps.repaint();
```

## Performance Considerations

- **Command queue size**: 128 commands (should be more than enough for UI interactions)
- **Preview MIDI queue size**: 32 messages
- **Overhead**: Minimal - commands are just `std::function` wrappers
- **Latency**: Commands execute in the next `processBlock()` call (~10ms at 48kHz with 512 buffer)

## Future Improvements

1. **Command batching**: Group related commands to reduce queue pressure
2. **Priority commands**: Separate queue for time-critical operations
3. **Command validation**: Check queue capacity and warn if dropping commands
4. **Metrics**: Track command queue usage for debugging

## Testing

To verify thread safety:
1. Run with Thread Sanitizer (TSan) to detect race conditions
2. Test rapid UI interactions (button mashing, slider dragging)
3. Test in both standalone and DAW modes
4. Monitor debug output for dropped commands

## References

- JUCE `AbstractFifo` documentation: https://docs.juce.com/master/classAbstractFifo.html
- JUCE `AsyncUpdater` documentation: https://docs.juce.com/master/classAsyncUpdater.html

