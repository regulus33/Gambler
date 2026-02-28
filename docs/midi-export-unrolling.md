# MIDI Export: Substep Unrolling & Clip Length Analysis

## How Unrolling Works

Each Gambler step can have 1-8 **substeps**. In the sequencer, all substeps share one
step slot — the sequencer cycles through them across repeated passes. On export, every
substep becomes its own **full-length MIDI step** so the user gets the complete melody
laid out linearly.

**Formula for total MIDI steps:**

```
totalMidiSteps = sum( substepCount[i] ) for i in 0..seqLength-1
```

Each MIDI step has a duration of `ticksPerStep` (e.g. 120 ticks for 16th notes at 480 PPQ).

---

## Examples (all at 16th note subdivision = 120 ticks/step)

### Case 1: Simple — All substep counts = 1

**Gambler:** 4 steps, each with 1 substep

```
Gambler Steps:  [ S0 ]  [ S1 ]  [ S2 ]  [ S3 ]
Substeps:         1       1       1       1

MIDI Output:    [ S0 ]  [ S1 ]  [ S2 ]  [ S3 ]
Tick:             0      120     240     360
Total:          4 MIDI steps = 480 ticks (1 quarter note)
```

No change — 4 steps in, 4 steps out.

---

### Case 2: One step has 2 substeps

**Gambler:** 4 steps, step 1 has 2 substeps

```
Gambler Steps:  [ S0 ]  [  S1   ]  [ S2 ]  [ S3 ]
Substeps:         1      2 (a, b)     1       1

MIDI Output:    [ S0 ]  [ S1a ] [ S1b ] [ S2 ]  [ S3 ]
Tick:             0      120     240     360      480
Total:          5 MIDI steps = 600 ticks
```

Step 1's two substeps unroll into two full-length 16th notes.

---

### Case 3: Mixed substep counts

**Gambler:** 8 steps with varying substep counts

```
Gambler Steps:  [ S0 ] [ S1 ] [ S2 ] [ S3 ] [ S4 ] [ S5 ] [ S6 ] [ S7 ]
Substeps:         1      1      3      1      1      2      1      1

MIDI Output:
  [ S0 ] [ S1 ] [ S2a ] [ S2b ] [ S2c ] [ S3 ] [ S4 ] [ S5a ] [ S5b ] [ S6 ] [ S7 ]
Tick:
    0     120     240     360     480     600    720     840     960    1080   1200
Total:  11 MIDI steps = 1320 ticks
```

The 8 Gambler steps (with substeps summing to 11) become 11 MIDI steps.

---

### Case 4: Edge case — 7 steps, one with 7 substeps, one with 3

**Gambler:** 7 steps

```
Step:       S0   S1   S2        S3   S4   S5           S6
Substeps:    1    1    7         1    1    3             1
                       (a-g)                (a,b,c)
```

**Unrolled MIDI output:**

```
 S0   S1   S2a  S2b  S2c  S2d  S2e  S2f  S2g  S3   S4   S5a  S5b  S5c  S6
  0  120   240  360  480  600  720  840  960  1080 1200 1320 1440 1560 1680
```

```
Total MIDI steps: 1 + 1 + 7 + 1 + 1 + 3 + 1 = 15
Total ticks:      15 * 120 = 1800
```

The 7 Gambler steps produce 15 MIDI steps.

---

### Case 5: Edge case — All substeps maxed out (8 each)

**Gambler:** 2 steps, each with 8 substeps

```
Step:       S0 (8 substeps)                       S1 (8 substeps)
Substeps:   a  b  c  d  e  f  g  h               a  b  c  d  e  f  g  h

MIDI:
 S0a S0b S0c S0d S0e S0f S0g S0h S1a S1b S1c S1d S1e S1f S1g S1h
  0  120 240 360 480 600 720 840 960 1080 ...                    1800
```

```
Total MIDI steps: 8 + 8 = 16
Total ticks:      16 * 120 = 1920 (4 quarter notes = 1 bar at 4/4)
```

---

### Case 6: Edge case — Rest substeps (note = -1)

**Gambler:** 3 steps, step 1 has 3 substeps where substep b is a rest

```
Step:       S0       S1 (3 substeps)        S2
Substeps:    1       a=C4  b=REST  c=E4      1

MIDI Output (rests are skipped, but time still advances):
  [S0=D4]  [S1a=C4]  [      ]  [S1c=E4]  [S2=G4]
     0       120       240       360        480
```

```
Total MIDI steps: 5 (time slots)
Note events:      4 (rest has no note-on/off, but the tick still advances)
```

Rests produce silence — no MIDI events — but the time slot is preserved
so the melody stays in rhythm.

---

## Clip Length Issue

### What determines MIDI clip length?

The MIDI file's effective length = the timestamp of the **last event** in the track.
JUCE automatically appends an End-of-Track meta event at that timestamp.

With the current code, the last event is typically a **note-off** for the final step:

```
lastNoteOff = (totalMidiSteps - 1) * ticksPerStep + (ticksPerStep * noteLength)
```

For a simple 8-step sequence at 16th notes with `noteLength = 0.9`:

```
Last note-on:    7 * 120 = 840
Last note-off:   840 + (120 * 0.9) = 840 + 108 = 948
Expected end:    8 * 120 = 960  (2 quarter notes)
```

The note-off at 948 is **before** the logical end at 960, so the clip should be
slightly shorter than 2 beats. But DAWs typically snap clip boundaries to the
nearest beat or bar, so Bitwig may show it as a full 2-beat or even 1-bar clip.

### Potential issue: noteLength > 1.0

The `noteLength` parameter range is `0.1 - 2.0` (from Params.h). If set above 1.0,
the note-off would extend **past** the next step boundary:

```
noteLength = 1.5, ticksPerStep = 120
Last note-on:    840
Last note-off:   840 + 180 = 1020  (extends 60 ticks past the logical end at 960)
```

This would make the clip appear longer than expected. This is actually correct
behavior (overlapping/legato notes), but it means the MIDI file's end-of-track
marker sits at 1020 instead of 960, and the DAW displays a longer clip.

### Recommendation

If the clip appears consistently longer than expected regardless of `noteLength`,
it's most likely Bitwig snapping to bar boundaries. A 15-step sequence at 16th notes
= 15/4 = 3.75 beats, which Bitwig would display as a 4-beat (1 bar) clip.
