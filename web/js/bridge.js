// Bridge: the single seam between the UI and whatever runs the sequencer.
// Inside the plugin that is the JUCE WebBrowserComponent backend; in a plain
// browser it is the mock engine. The UI never talks to either directly.
//
// ---------------------------------------------------------------- CONTRACT
// Engine -> UI events
//   state  full snapshot, pushed (coalesced) after anything changes:
//     length, selectedStep, currentStep, transport ('stopped'|'playing'|'paused'), roll
//     steps[64]   { count, cur, sel, notes[8] }  notes are quantized for display, -1 = rest
//     scale[12]   bool per pitch class;  latch, bakeScale, bakeRange
//     ranges[1..3] { min, max } MIDI notes; a note is allowed if it is inside any range
//     prob[64]    bool grid;  probDim (8|4|2), probPercent, probTick
//     colours[12] '#rrggbb'
//     bpm, subdivision (0..3), clockMode (0 internal, 1 host, 2 MIDI clock)
//     channel, noteLength, previewNotes, followStep
//     presets[] names, presetName
//     learn       { active, selected, mappings: { controlId: 'CC 21' } }
//     canUndo
//   tick   { step, sub, note, probTick }  once per sequencer step
//   noteOff  the sounding note ended (clears the key highlight)
//   toast  string
//
// UI -> engine actions: send(action, payload)
//   selectStep {step, audition}      selectSub {step, sub, audition}   jumpTo {step}
//   shiftNote {step, sub, delta}     delta in scale degrees, staying inside the note ranges
//   setNote {step, sub, note, undo}  absolute pitch (-1 = rest); undo: true on the first change of a gesture
//   addSub {step}                    removeSub {step, sub}
//   setLength {length}               roll {on}     setPage {page}  (which 8 steps are visible)
//   nudge {dir}  duplicate  shuffle  randomize {scope}  clear {scope}  undo
//                                    scope: 'step' | 'page' (clear only) | 'all'
//   setScaleNote {pc, on}            setRange {index, min, max}  addRange  removeRange {index}
//   setProbPercent {percent}         probPaint {cell, on}  probReseed  probDivision
//   playPause  stop
//   setParam {id, value}             ids: bpm subdivision clockMode channel noteLength
//                                         previewNotes followStep latch bakeScale bakeRange
//   setColour {pc, colour}           resetColours
//   savePreset {name}  loadPreset {name}  deletePreset {name}  cyclePreset {dir}
//   learnToggle  learnSelect {id}  learnClear {id}
//   exportMidi
// -------------------------------------------------------------------------
(function () {
  'use strict';
  const G = (window.Gambler = window.Gambler || {});

  const handlers = { state: [], tick: [], noteOff: [], toast: [] };
  const emit = (type, data) => (handlers[type] || []).forEach((fn) => fn(data));

  const native = window.__JUCE__ && window.__JUCE__.backend;
  let send;
  let ready; // called by the UI once its handlers are registered; triggers the first state push

  if (native) {
    Object.keys(handlers).forEach((type) => native.addEventListener(type, (data) => emit(type, data)));
    send = (action, payload) => native.emitEvent('action', { action, payload: payload || {} });
    ready = () => send('ready');
  } else {
    const engine = G.createMockEngine();
    send = engine.dispatch;
    ready = () => engine.on(emit);
  }

  G.bridge = {
    isMock: !native,
    on: (type, fn) => handlers[type].push(fn),
    send,
    ready,
  };
})();
