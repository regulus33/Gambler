// Mock engine: a browser-only stand-in for the C++ backend so the UI can be
// developed and tried without the plugin. It implements the same contract as
// the native bridge (see bridge.js) and makes sound with WebAudio.
// Nothing in here ships as plugin logic; the real behaviour lives in C++.
(function () {
  'use strict';
  const G = (window.Gambler = window.Gambler || {});

  const MAX_STEPS = 64;
  const MAX_SUBS = 8;
  const MAX_RANGES = 3;
  const PPQ = [1, 2, 4, 8]; // quarter, eighth, sixteenth, thirty-second
  const DEFAULT_COLOURS = [
    '#ff6b6b', '#ff8e53', '#ffd93d', '#6bcb77', '#4ecdc4', '#45b7d1',
    '#5c7aea', '#9b59b6', '#e056fd', '#ff6b9d', '#ff9f43', '#feca57',
  ];
  const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

  G.createMockEngine = function () {
    const listeners = [];
    const emit = (type, data) => listeners.forEach((fn) => fn(type, data));

    const makeStep = (note) => ({
      notes: [note, ...Array(MAX_SUBS - 1).fill(-1)],
      count: 1,
      cur: -1,
      sel: 0,
    });

    const s = {
      length: 16,
      selectedStep: 0,
      currentStep: -1,
      transport: 'stopped',
      roll: false,
      steps: [],
      scale: [1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0].map(Boolean), // C minor pentatonic
      latch: true,
      ranges: [{ min: 48, max: 72 }], // up to MAX_RANGES; a note is allowed if it falls in any of them
      bakeScale: false,
      bakeRange: false,
      prob: Array(64).fill(true),
      probDim: 8,
      probTick: -1,
      colours: DEFAULT_COLOURS.slice(),
      bpm: 120,
      subdivision: 2,
      clockMode: 0,
      channel: 1,
      noteLength: 0.9,
      previewNotes: true,
      followStep: false,
      presets: [],
      presetName: '',
      learn: { active: false, selected: null, mappings: {} },
      sound: true,
    };

    const demo = [48, 55, 60, -1, 63, 60, 58, 55, 53, -1, 60, 65, 67, 63, 60, 58];
    for (let i = 0; i < MAX_STEPS; i++) s.steps.push(makeStep(i < demo.length ? demo[i] : 60));
    Object.assign(s.steps[2], { notes: [60, 63, 67, -1, -1, -1, -1, -1], count: 3 });
    Object.assign(s.steps[6], { notes: [58, 70, -1, -1, -1, -1, -1, -1], count: 2 });

    // ---------- scale / range (ported from ScaleQuantizer) ----------
    const inScale = (n) => s.scale[((n % 12) + 12) % 12];
    const anyScale = () => s.scale.some(Boolean);

    const inRange = (n) => s.ranges.some((r) => n >= r.min && n <= r.max);

    // Closest note that passes `ok`, preferring the lower one on a tie.
    function nearest(n, ok) {
      if (ok(n)) return n;
      for (let d = 1; d <= 127; d++) {
        if (n - d >= 0 && ok(n - d)) return n - d;
        if (n + d <= 127 && ok(n + d)) return n + d;
      }
      return -1;
    }

    // What a stored note sounds as: baked constraints are already in the data, so skip them.
    function quantize(n) {
      if (n < 0) return n;
      if (!s.bakeScale && !anyScale()) return -1;
      return nearest(n, (m) => (s.bakeRange || inRange(m)) && (s.bakeScale || inScale(m)));
    }

    function bake(n) {
      if (n < 0 || (!s.bakeRange && !s.bakeScale)) return n;
      if (s.bakeScale && !anyScale()) return -1;
      return nearest(n, (m) => (!s.bakeRange || inRange(m)) && (!s.bakeScale || inScale(m)));
    }

    function bakeAll() {
      s.steps.forEach((st) => (st.notes = st.notes.map(bake)));
    }

    function randomNote() {
      const pool = [];
      for (let n = 0; n <= 127; n++) if (inRange(n)) pool.push(n);
      return pool[Math.floor(Math.random() * pool.length)];
    }

    // ---------- probability ----------
    let probOrder = [];
    let probPercent = 100;

    function shuffled(n) {
      const a = Array.from({ length: n }, (_, i) => i);
      for (let i = n - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [a[i], a[j]] = [a[j], a[i]];
      }
      return a;
    }

    const underlying = (cell) => {
      const block = 8 / s.probDim;
      return Math.floor(cell / s.probDim) * block * 8 + (cell % s.probDim) * block;
    };

    function setProbCell(cell, on) {
      const block = 8 / s.probDim;
      const top = underlying(cell);
      for (let r = 0; r < block; r++) for (let c = 0; c < block; c++) s.prob[top + r * 8 + c] = on;
    }

    function applyProbPercent() {
      const cells = s.probDim * s.probDim;
      if (probOrder.length !== cells) probOrder = shuffled(cells);
      const n = Math.round((probPercent / 100) * cells);
      probOrder.forEach((cell, i) => setProbCell(cell, i < n));
    }

    function measuredProbPercent() {
      const cells = s.probDim * s.probDim;
      let on = 0;
      for (let i = 0; i < cells; i++) if (s.prob[underlying(i)]) on++;
      return Math.round((on / cells) * 100);
    }

    // ---------- undo ----------
    const undoStack = [];
    function pushUndo() {
      undoStack.push(JSON.stringify({ steps: s.steps, length: s.length }));
      if (undoStack.length > 20) undoStack.shift();
    }

    // ---------- presets ----------
    const presetData = {};
    function capturePreset() {
      const { presets, presetName, learn, transport, currentStep, sound, ...rest } = s;
      return JSON.stringify(rest);
    }
    function savePreset(name) {
      presetData[name] = capturePreset();
      if (!s.presets.includes(name)) s.presets.push(name);
      s.presets.sort();
      s.presetName = name;
    }
    function loadPreset(name) {
      if (!presetData[name]) return;
      Object.assign(s, JSON.parse(presetData[name]));
      s.presetName = name;
      resetPlayhead();
    }

    // ---------- sound ----------
    let ctx = null;
    function playNote(midi, ms) {
      if (!s.sound || midi < 0) return;
      try {
        ctx = ctx || new (window.AudioContext || window.webkitAudioContext)();
        if (ctx.state === 'suspended') ctx.resume();
        const t = ctx.currentTime;
        const dur = Math.max(0.05, ms / 1000);
        const osc = ctx.createOscillator();
        const gain = ctx.createGain();
        const filter = ctx.createBiquadFilter();
        osc.type = 'triangle';
        osc.frequency.value = 440 * Math.pow(2, (midi - 69) / 12);
        filter.type = 'lowpass';
        filter.frequency.value = 2400;
        gain.gain.setValueAtTime(0, t);
        gain.gain.linearRampToValueAtTime(0.22, t + 0.006);
        gain.gain.exponentialRampToValueAtTime(0.001, t + dur);
        osc.connect(filter).connect(gain).connect(ctx.destination);
        osc.start(t);
        osc.stop(t + dur + 0.02);
      } catch (e) {
        /* no audio available: stay silent */
      }
    }

    // ---------- clock ----------
    let timer = null;
    let nextAt = 0;
    const stepMs = () => 60000 / s.bpm / PPQ[s.subdivision];

    function advanceSub(st) {
      st.cur = (st.cur + 1) % Math.max(1, st.count);
      return st.cur;
    }

    function tick() {
      const idx = (s.currentStep + 1) % s.length;
      s.currentStep = idx;
      const cells = s.probDim * s.probDim;
      s.probTick = (s.probTick + 1) % cells;
      const st = s.steps[idx];
      const sub = advanceSub(st);
      let note;
      let changed = false;

      if (s.roll) {
        if (!s.prob[underlying(s.probTick)]) {
          st.notes[sub] = -1;
          note = -1;
        } else {
          const raw = randomNote();
          st.notes[sub] = bake(raw);
          note = quantize(st.notes[sub]);
        }
        changed = true;
      } else {
        note = quantize(st.notes[sub]);
      }

      playNote(note, stepMs() * s.noteLength);
      emit('tick', { step: idx, sub, note, probTick: s.probTick });
      if (changed) emitState();
    }

    function loop() {
      tick();
      nextAt += stepMs();
      timer = setTimeout(loop, Math.max(0, nextAt - performance.now()));
    }

    function startClock() {
      clearTimeout(timer);
      nextAt = performance.now();
      loop();
    }

    function resetPlayhead() {
      s.currentStep = -1;
      s.probTick = -1;
      s.steps.forEach((st) => (st.cur = -1));
    }

    // ---------- snapshot ----------
    function snapshot() {
      return {
        ...s,
        steps: s.steps.map((st) => ({
          count: st.count,
          cur: st.cur,
          sel: st.sel,
          notes: st.notes.map(quantize), // the UI only ever shows what will sound
        })),
        probPercent: measuredProbPercent(),
        canUndo: undoStack.length > 0,
        ranges: s.ranges.map((r) => ({ ...r })),
        learn: { ...s.learn, mappings: { ...s.learn.mappings } },
      };
    }

    let pending = false;
    function emitState() {
      if (pending) return;
      pending = true;
      queueMicrotask(() => {
        pending = false;
        emit('state', snapshot());
      });
    }

    // ---------- actions ----------
    const audition = (step, sub) => {
      if (s.previewNotes) playNote(quantize(s.steps[step].notes[sub]), 220);
    };

    const actions = {
      ready() {},

      selectStep({ step, audition: play }) {
        s.selectedStep = clamp(step, 0, MAX_STEPS - 1);
        const st = s.steps[s.selectedStep];
        if (play) audition(s.selectedStep, st.sel);
      },
      selectSub({ step, sub, audition: play }) {
        const st = s.steps[step];
        st.sel = clamp(sub, 0, st.count - 1);
        if (play) audition(step, st.sel);
      },
      jumpTo({ step }) {
        if (step < s.length) s.currentStep = step - 1;
      },
      // Move a note by whole scale degrees, staying inside the note range.
      shiftNote({ step, sub, delta }) {
        if (!anyScale()) return;
        const st = s.steps[step];
        let n = quantize(st.notes[sub]);
        const allowed = (m) => inScale(m) && inRange(m);
        if (n < 0) {
          n = nearest(Math.round((s.ranges[0].min + s.ranges[0].max) / 2), allowed);
          if (n < 0) return;
        } else {
          const dir = Math.sign(delta);
          for (let k = Math.abs(delta); k > 0; k--) {
            let m = n + dir;
            while (m >= 0 && m <= 127 && !allowed(m)) m += dir;
            if (m < 0 || m > 127) break;
            n = m;
          }
        }
        st.notes[sub] = n;
        audition(step, sub);
      },
      addSub({ step }) {
        const st = s.steps[step];
        if (st.count >= MAX_SUBS) return;
        pushUndo();
        st.notes[st.count] = st.notes[st.sel];
        st.sel = st.count;
        st.count++;
        audition(step, st.sel);
      },
      removeSub({ step, sub }) {
        const st = s.steps[step];
        if (st.count <= 1) return;
        pushUndo();
        st.notes.splice(sub, 1);
        st.notes.push(-1);
        st.count--;
        st.sel = clamp(st.sel, 0, st.count - 1);
        if (st.cur >= st.count) st.cur = -1;
      },

      setLength({ length }) {
        s.length = clamp(length, 1, MAX_STEPS);
        if (s.currentStep >= s.length) s.currentStep = -1;
      },
      setPage() {},
      roll({ on }) {
        s.roll = !!on;
      },
      nudge({ dir }) {
        pushUndo();
        const live = s.steps.slice(0, s.length);
        if (dir < 0) live.push(live.shift());
        else live.unshift(live.pop());
        s.steps.splice(0, s.length, ...live);
      },
      duplicate() {
        if (s.length >= MAX_STEPS) return;
        pushUndo();
        const newLen = Math.min(MAX_STEPS, s.length * 2);
        for (let i = s.length; i < newLen; i++) {
          const src = s.steps[i - s.length];
          s.steps[i] = { ...src, notes: src.notes.slice(), cur: -1 };
        }
        s.length = newLen;
      },
      shuffle() {
        pushUndo();
        const order = shuffled(s.length);
        const live = s.steps.slice(0, s.length);
        order.forEach((from, to) => (s.steps[to] = live[from]));
      },
      randomize({ scope }) {
        pushUndo();
        const targets = scope === 'all' ? s.steps.slice(0, s.length) : [s.steps[s.selectedStep]];
        targets.forEach((st) => {
          for (let i = 0; i < st.count; i++) st.notes[i] = bake(randomNote());
        });
        if (scope !== 'all') audition(s.selectedStep, s.steps[s.selectedStep].sel);
      },
      clear({ scope }) {
        pushUndo();
        if (scope === 'all') {
          for (let i = 0; i < s.length; i++) s.steps[i] = makeStep(-1);
        } else {
          const st = s.steps[s.selectedStep];
          st.notes[st.sel] = -1;
        }
      },
      undo() {
        const last = undoStack.pop();
        if (!last) return;
        const data = JSON.parse(last);
        s.steps = data.steps;
        s.length = data.length;
      },

      setScaleNote({ pc, on }) {
        s.scale[pc] = !!on;
        if (s.bakeScale) bakeAll();
      },
      setRange({ index, min, max }) {
        if (!s.ranges[index]) return;
        s.ranges[index] = { min: clamp(Math.min(min, max), 0, 127), max: clamp(Math.max(min, max), 0, 127) };
        if (s.bakeRange) bakeAll();
      },
      // New ranges start as the octave above the highest one, or below the lowest if there is no room.
      addRange() {
        if (s.ranges.length >= MAX_RANGES) return;
        const top = Math.max(...s.ranges.map((r) => r.max));
        const bottom = Math.min(...s.ranges.map((r) => r.min));
        if (top + 13 <= 127) s.ranges.push({ min: top + 2, max: top + 13 });
        else if (bottom - 13 >= 0) s.ranges.push({ min: bottom - 13, max: bottom - 2 });
        else s.ranges.push({ min: 60, max: 71 });
        if (s.bakeRange) bakeAll();
      },
      removeRange({ index }) {
        if (s.ranges.length <= 1 || !s.ranges[index]) return;
        s.ranges.splice(index, 1);
        if (s.bakeRange) bakeAll();
      },

      setProbPercent({ percent }) {
        probPercent = clamp(percent, 0, 100);
        applyProbPercent();
      },
      probPaint({ cell, on }) {
        setProbCell(cell, !!on);
        probPercent = measuredProbPercent();
      },
      probReseed() {
        probOrder = shuffled(s.probDim * s.probDim);
        applyProbPercent();
      },
      probDivision() {
        probPercent = measuredProbPercent();
        s.probDim = s.probDim === 8 ? 4 : s.probDim === 4 ? 2 : 8;
        s.probTick = -1;
        probOrder = [];
        applyProbPercent();
      },

      playPause() {
        if (s.transport === 'playing') {
          s.transport = 'paused';
          clearTimeout(timer);
        } else {
          s.transport = 'playing';
          startClock();
        }
      },
      stop() {
        s.transport = 'stopped';
        clearTimeout(timer);
        resetPlayhead();
      },

      setParam({ id, value }) {
        switch (id) {
          case 'bpm': s.bpm = clamp(Math.round(value), 20, 999); break;
          case 'subdivision': s.subdivision = clamp(value, 0, 3); break;
          case 'clockMode': s.clockMode = clamp(value, 0, 2); break;
          case 'channel': s.channel = clamp(value, 1, 16); break;
          case 'noteLength': s.noteLength = clamp(value, 0.1, 2); break;
          case 'previewNotes': s.previewNotes = !!value; break;
          case 'followStep': s.followStep = !!value; break;
          case 'latch': s.latch = !!value; break;
          case 'bakeScale': s.bakeScale = !!value; if (value) bakeAll(); break;
          case 'bakeRange': s.bakeRange = !!value; if (value) bakeAll(); break;
          case 'sound': s.sound = !!value; break;
        }
      },
      setColour({ pc, colour }) {
        s.colours[pc] = colour;
      },
      resetColours() {
        s.colours = DEFAULT_COLOURS.slice();
      },

      savePreset({ name }) {
        const clean = String(name || '').trim();
        if (!clean) return;
        savePreset(clean);
        emit('toast', 'Saved "' + clean + '"');
      },
      loadPreset({ name }) {
        loadPreset(name);
      },
      deletePreset({ name }) {
        delete presetData[name];
        s.presets = s.presets.filter((p) => p !== name);
        if (s.presetName === name) s.presetName = '';
      },
      cyclePreset({ dir }) {
        if (!s.presets.length) return;
        const i = s.presets.indexOf(s.presetName);
        const next = (i + dir + s.presets.length) % s.presets.length;
        loadPreset(s.presets[i < 0 ? 0 : next]);
      },

      learnToggle() {
        s.learn.active = !s.learn.active;
        s.learn.selected = null;
      },
      learnSelect({ id }) {
        s.learn.selected = id;
        // No MIDI in the browser: pretend a controller answered after a moment.
        setTimeout(() => {
          if (!s.learn.active || s.learn.selected !== id) return;
          s.learn.mappings[id] = 'CC ' + (20 + Object.keys(s.learn.mappings).length);
          s.learn.selected = null;
          emitState();
        }, 1200);
      },
      learnClear({ id }) {
        delete s.learn.mappings[id];
        if (s.learn.selected === id) s.learn.selected = null;
      },

      exportMidi() {
        emit('toast', 'MIDI export runs in the plugin, not in this prototype');
      },
    };

    savePreset('Night Bus');
    s.presetName = '';

    return {
      on(fn) {
        listeners.push(fn);
        emitState();
      },
      dispatch(action, payload) {
        const fn = actions[action];
        if (!fn) return console.warn('mock engine: unknown action', action);
        fn(payload || {});
        emitState();
      },
    };
  };
})();
