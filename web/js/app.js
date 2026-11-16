// Gambler UI. Renders the latest engine snapshot and turns gestures into
// bridge actions. It holds no sequencer logic of its own; the only local state
// is view state (which page is showing, what is being dragged).
(function () {
  'use strict';
  const { bridge, icons, keys: keyGeom } = window.Gambler;
  const send = bridge.send;

  const $ = (sel, root) => (root || document).querySelector(sel);
  const $$ = (sel, root) => Array.from((root || document).querySelectorAll(sel));
  const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
  const el = (tag, cls, html) => {
    const node = document.createElement(tag);
    if (cls) node.className = cls;
    if (html != null) node.innerHTML = html;
    return node;
  };

  const NOTE_NAMES = ['C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab', 'A', 'Bb', 'B'];
  const SUBDIV_LABELS = ['1/4', '1/8', '1/16', '1/32'];
  const noteName = (n) => (n < 0 ? '' : NOTE_NAMES[n % 12] + (Math.floor(n / 12) - 1));

  const PITCH_PX = 16;    // vertical drag distance per scale degree
  const HOLD_MS = 550;    // hold time for "apply to all"

  let S = null; // latest engine snapshot
  const ui = { page: 0, drag: null, lenPreview: null, playedPc: -1 };

  // ================================================================ build
  $$('[data-icon]').forEach((node) => node.insertAdjacentHTML('afterbegin', icons[node.dataset.icon]));

  // ---- keys
  const keysSvg = $('#keys');
  keysSvg.setAttribute('viewBox', keyGeom.viewBox);
  keysSvg.innerHTML = keyGeom.list
    .map((k) => '<path d="' + k.d + '" data-pc="' + k.pc + '" data-learn="key-' + k.pc + '"></path>')
    .join('');
  const keyPaths = [];
  const keyLabels = [];
  keyGeom.list.forEach((k) => {
    keyPaths[k.pc] = $('[data-pc="' + k.pc + '"]', keysSvg);
    const label = el('span', 'key-label', NOTE_NAMES[k.pc]);
    label.style.left = k.cx * 100 + '%';
    label.style.top = k.cy * 100 + '%';
    $('#key-labels').appendChild(label);
    keyLabels[k.pc] = label;
  });
  const isBlackKey = (pc) => [1, 3, 6, 8, 10].includes(pc);

  // ---- pads
  const padsEl = $('#pads');
  const pads = Array.from({ length: 8 }, (_, i) => {
    const pad = el('div', 'pad', '<span class="pad-index"></span><span class="pad-note"></span><span class="pips"></span>');
    pad.dataset.learn = 'step-' + i;
    pad.setAttribute('role', 'button');
    padsEl.appendChild(pad);
    return { root: pad, index: $('.pad-index', pad), note: $('.pad-note', pad), pips: $('.pips', pad) };
  });

  // ---- cycle lane
  const cycleEl = $('#cycle');
  const slots = Array.from({ length: 8 }, (_, i) => {
    const slot = el('div', 'slot', '<span class="slot-note"></span><button class="slot-x" aria-label="Remove this note from the cycle">' + icons.close + '</button>');
    slot.dataset.learn = 'cycle-' + i;
    slot.setAttribute('role', 'button');
    cycleEl.appendChild(slot);
    return { root: slot, note: $('.slot-note', slot), x: $('.slot-x', slot) };
  });

  // ---- overview grid
  const gridEl = $('#seq-grid');
  const cells = Array.from({ length: 64 }, (_, i) => {
    const cell = el('div', 'seq-cell');
    cell.dataset.i = i;
    gridEl.appendChild(cell);
    return cell;
  });

  // ---- page dots
  const pagesEl = $('#pages');
  const pageDots = Array.from({ length: 8 }, (_, i) => {
    const dot = el('button', 'page-dot');
    dot.setAttribute('aria-label', 'Page ' + (i + 1));
    dot.addEventListener('click', () => { ui.page = i; render(); });
    pagesEl.appendChild(dot);
    return dot;
  });

  // ---- actions
  const ACTIONS = [
    { id: 'roll', label: 'Roll', hint: 'hold', icon: 'dice', kind: 'momentary' },
    { id: 'nudge-left', label: 'Nudge', icon: 'arrowL', run: () => send('nudge', { dir: -1 }) },
    { id: 'nudge-right', label: 'Nudge', icon: 'arrowR', run: () => send('nudge', { dir: 1 }) },
    { id: 'duplicate', label: 'Duplicate', icon: 'duplicate', run: () => send('duplicate') },
    { id: 'shuffle', label: 'Shuffle', icon: 'shuffle', run: () => send('shuffle') },
    { id: 'randomize', label: 'Random', hint: 'hold: all', icon: 'random', kind: 'scoped' },
    { id: 'clear', label: 'Clear', hint: 'hold: all', icon: 'clear', kind: 'scoped' },
    { id: 'undo', label: 'Undo', icon: 'undo', run: () => send('undo') },
  ];
  const actionEls = {};
  ACTIONS.forEach((a) => {
    const btn = el('button', 'action', icons[a.icon] +
      '<span class="action-text"><b>' + a.label + '</b>' + (a.hint ? '<small>' + a.hint + '</small>' : '') + '</span>');
    btn.dataset.learn = a.id;
    $('#actions').appendChild(btn);
    actionEls[a.id] = btn;

    if (a.kind === 'momentary') {
      const set = (on) => () => send('roll', { on });
      btn.addEventListener('pointerdown', (e) => { btn.setPointerCapture(e.pointerId); set(true)(); });
      btn.addEventListener('pointerup', set(false));
      btn.addEventListener('pointercancel', set(false));
    } else if (a.kind === 'scoped') {
      let timer = null;
      let fired = false;
      const reset = () => { clearTimeout(timer); timer = null; btn.classList.remove('is-holding'); };
      btn.addEventListener('pointerdown', (e) => {
        btn.setPointerCapture(e.pointerId);
        fired = false;
        btn.classList.add('is-holding');
        timer = setTimeout(() => {
          fired = true;
          reset();
          send(a.id, { scope: 'all' });
          btn.classList.add('is-fired');
          setTimeout(() => btn.classList.remove('is-fired'), 260);
        }, HOLD_MS);
      });
      btn.addEventListener('pointerup', () => { if (timer && !fired) send(a.id, { scope: 'step' }); reset(); });
      btn.addEventListener('pointercancel', reset);
    } else {
      btn.addEventListener('click', a.run);
    }
  });

  // ---- settings sheet statics
  const channelSel = $('#set-channel');
  for (let c = 1; c <= 16; c++) channelSel.appendChild(new Option(String(c), String(c)));
  const colourInputs = NOTE_NAMES.map((name, pc) => {
    const wrap = el('label', 'colour', '<input type="color" id="set-colour-' + pc + '" aria-label="Colour for ' + name + '">' + name);
    $('#set-colours').appendChild(wrap);
    const input = $('input', wrap);
    input.addEventListener('input', () => send('setColour', { pc, colour: input.value }));
    return input;
  });

  // ================================================================ render
  let renderQueued = false;
  function render() {
    if (renderQueued || !S) return;
    renderQueued = true;
    requestAnimationFrame(() => {
      renderQueued = false;
      draw();
    });
  }

  const shownSub = (st) => clamp(st.cur, 0, st.count - 1);

  function draw() {
    const len = ui.lenPreview != null ? ui.lenPreview : S.length;
    ui.page = clamp(ui.page, 0, Math.min(7, Math.floor(S.length / 8)));
    const playing = S.transport !== 'stopped';

    document.body.classList.toggle('is-playing', S.transport === 'playing');
    document.body.classList.toggle('is-learning', S.learn.active);

    // ---- bar
    $('#tempo-val').textContent = S.clockMode === 0 ? S.bpm : S.clockMode === 1 ? 'HOST' : 'MIDI';
    $('#tempo small').hidden = S.clockMode !== 0;
    $('#subdiv').textContent = SUBDIV_LABELS[S.subdivision];
    $('#preset-label').textContent = S.presetName || 'Presets';

    // ---- keys
    const selStep = S.steps[S.selectedStep];
    const selNote = selStep.notes[selStep.sel];
    const activePc = playing ? ui.playedPc : selNote >= 0 ? selNote % 12 : -1;
    for (let pc = 0; pc < 12; pc++) {
      keyPaths[pc].style.fill = S.scale[pc] ? S.colours[pc] : isBlackKey(pc) ? '#bababa' : '#ffffff';
      keyLabels[pc].classList.toggle('is-active', pc === activePc);
    }
    $('#latch').classList.toggle('is-on', S.latch);
    $('#bake-scale').classList.toggle('is-on', S.bakeScale);
    $('#bake-range').classList.toggle('is-on', S.bakeRange);

    // ---- range
    const lo = (S.rangeMin / 127) * 100;
    const hi = (S.rangeMax / 127) * 100;
    $('.range-fill').style.left = lo + '%';
    $('.range-fill').style.width = hi - lo + '%';
    $('[data-h="min"]').style.left = lo + '%';
    $('[data-h="max"]').style.left = hi + '%';
    $('#range-label').textContent = noteName(S.rangeMin) + '–' + noteName(S.rangeMax);

    // ---- chance
    drawProb();

    // ---- overview grid
    cells.forEach((cell, i) => {
      const st = S.steps[i];
      const ghost = i >= len;
      const note = st.notes[shownSub(st)];
      cell.className = 'seq-cell' +
        (ghost ? ' is-ghost' : note < 0 ? ' is-rest' : '') +
        (i === S.selectedStep ? ' is-selected' : '') +
        (!ghost && playing && i === S.currentStep ? ' is-playing' : '');
      cell.style.setProperty('--c', !ghost && note >= 0 ? S.colours[note % 12] : '');
    });
    gridEl.style.setProperty('--page', ui.page);

    // ---- cycle lane
    $('#lane-step').textContent = 'Step ' + (S.selectedStep + 1);
    slots.forEach((slot, i) => {
      const root = slot.root;
      const isNote = i < selStep.count;
      const isAdd = i === selStep.count && selStep.count < 8;
      root.hidden = !isNote && !isAdd;
      if (root.hidden) return;
      const note = isNote ? selStep.notes[i] : -1;
      root.className = 'slot' +
        (isAdd ? ' is-add' : note < 0 ? ' is-rest' : '') +
        (isNote && i === selStep.sel ? ' is-selected' : '') +
        (isNote && selStep.count > 1 ? ' can-remove' : '') +
        (isNote && selStep.count > 1 && i === shownSub(selStep) ? ' is-cur' : '');
      root.style.setProperty('--c', note >= 0 ? S.colours[note % 12] : '');
      slot.note.innerHTML = isAdd ? icons.plus : note >= 0 ? noteName(note) : '–';
      root.setAttribute('aria-label', isAdd ? 'Add a note to this step’s cycle' : 'Cycle note ' + (i + 1));
    });

    pageDots.forEach((dot, i) => {
      dot.classList.toggle('has-steps', i * 8 < S.length);
      dot.classList.toggle('is-current', i === ui.page);
    });
    $('#len-val').textContent = len;

    // ---- pads
    pads.forEach((pad, i) => {
      const abs = ui.page * 8 + i;
      const st = S.steps[abs];
      const ghost = abs >= len;
      const editing = ui.drag && ui.drag.step === abs;
      const note = ghost ? -1 : st.notes[editing ? ui.drag.sub : shownSub(st)];
      pad.root.className = 'pad' +
        (ghost ? ' is-ghost' : note < 0 ? ' is-rest' : '') +
        (abs === S.selectedStep && !ghost ? ' is-selected' : '') +
        (!ghost && playing && abs === S.currentStep ? ' is-playing' : '') +
        (editing ? ' is-dragging' : '');
      pad.root.style.setProperty('--c', note >= 0 ? S.colours[note % 12] : '');
      pad.index.textContent = abs + 1;
      pad.note.textContent = ghost ? '' : note >= 0 ? noteName(note) : '–';
      pad.pips.innerHTML = !ghost && st.count > 1
        ? Array.from({ length: st.count }, (_, k) => '<i' + (k === shownSub(st) ? ' class="is-cur"' : '') + '></i>').join('')
        : '';
    });

    // ---- actions
    actionEls.roll.classList.toggle('is-on', S.roll);
    actionEls.undo.disabled = !S.canUndo;
    actionEls.duplicate.disabled = S.length >= 64;

    drawLearn();
    if (!$('#sheet').hidden) drawSettings();
  }

  let probDim = 0;
  let probCells = [];
  function drawProb() {
    const grid = $('#prob-grid');
    if (probDim !== S.probDim) {
      probDim = S.probDim;
      grid.innerHTML = '';
      grid.style.setProperty('--dim', probDim);
      probCells = Array.from({ length: probDim * probDim }, (_, i) => {
        const cell = el('div', 'prob-cell');
        cell.dataset.cell = i;
        grid.appendChild(cell);
        return cell;
      });
    }
    const block = 8 / probDim;
    probCells.forEach((cell, i) => {
      const under = Math.floor(i / probDim) * block * 8 + (i % probDim) * block;
      cell.classList.toggle('is-on', S.prob[under]);
      cell.classList.toggle('is-tick', S.transport !== 'stopped' && i === S.probTick);
    });
    $('#prob-val').textContent = S.probPercent + '%';
    $('#prob-div').textContent = probDim + '×' + probDim;
    const slider = $('#prob-slider');
    if (document.activeElement !== slider || !slider.matches(':active')) slider.value = S.probPercent;
  }

  function drawLearn() {
    $('#learn-banner').hidden = !S.learn.active;
    if (!S.learn.active && !document.querySelector('[data-map]')) return;
    const selected = S.learn.selected;
    $$('[data-learn]').forEach((node) => {
      const id = node.dataset.learn;
      const mapped = S.learn.mappings[id];
      if (mapped) node.dataset.map = mapped;
      else delete node.dataset.map;
      node.classList.toggle('is-learn-selected', S.learn.active && id === selected);
    });
    $('#learn-text').textContent = selected
      ? 'Now move a knob or play a key on your controller…'
      : 'MIDI learn: tap a control, then move a knob or key on your controller.';
    $('#learn-clear').hidden = !(ui.learnFocus && S.learn.mappings[ui.learnFocus]);
  }

  function drawSettings() {
    const focused = document.activeElement;
    $$('#sheet .seg').forEach((seg) => {
      const value = S[seg.dataset.param];
      $$('button', seg).forEach((b) => b.classList.toggle('is-on', +b.dataset.v === value));
    });
    const setIfIdle = (input, value) => { if (input !== focused) input.value = value; };
    setIfIdle($('#set-bpm'), S.bpm);
    $('#set-bpm').disabled = S.clockMode !== 0;
    setIfIdle(channelSel, S.channel);
    setIfIdle($('#set-notelen'), Math.round(S.noteLength * 100));
    $('#set-notelen-val').textContent = Math.round(S.noteLength * 100) + '%';
    $('#set-preview').checked = S.previewNotes;
    $('#set-follow').checked = S.followStep;
    colourInputs.forEach((input, pc) => setIfIdle(input, S.colours[pc].toLowerCase()));
  }

  // ================================================================ gestures

  // Vertical drag = change pitch, tap = select, optional horizontal swipe.
  function pitchGesture(node, opts) {
    node.addEventListener('pointerdown', (e) => {
      if (e.button) return;
      const target = opts.target(e);
      if (!target) return;
      node.setPointerCapture(e.pointerId);
      const g = { id: e.pointerId, x: e.clientX, y: e.clientY, mode: null, applied: 0 };

      const move = (ev) => {
        if (ev.pointerId !== g.id) return;
        const dx = ev.clientX - g.x;
        const dy = ev.clientY - g.y;
        if (!g.mode) {
          if (Math.abs(dy) > 8 && Math.abs(dy) >= Math.abs(dx) && target.editable) {
            g.mode = 'pitch';
            g.y = ev.clientY;
            ui.drag = { step: target.step, sub: target.sub };
            opts.onEditStart(target);
          } else if (Math.abs(dx) > 14 && Math.abs(dx) > Math.abs(dy) && opts.onSwipe) {
            g.mode = 'swipe';
          } else {
            return;
          }
        }
        if (g.mode === 'pitch') {
          const want = Math.round((g.y - ev.clientY) / PITCH_PX);
          if (want !== g.applied) {
            send('shiftNote', { step: target.step, sub: target.sub, delta: want - g.applied });
            g.applied = want;
          }
        }
      };

      const end = (ev) => {
        if (ev.pointerId !== g.id) return;
        node.removeEventListener('pointermove', move);
        node.removeEventListener('pointerup', end);
        node.removeEventListener('pointercancel', end);
        if (ev.type === 'pointerup') {
          if (!g.mode) opts.onTap(target);
          else if (g.mode === 'swipe' && Math.abs(ev.clientX - g.x) > 40) opts.onSwipe(ev.clientX < g.x ? 1 : -1);
        }
        ui.drag = null;
        render();
      };

      node.addEventListener('pointermove', move);
      node.addEventListener('pointerup', end);
      node.addEventListener('pointercancel', end);
    });
  }

  // ---- pads
  pads.forEach((pad, i) => {
    const absOf = () => ui.page * 8 + i;
    pitchGesture(pad.root, {
      target: () => {
        const step = absOf();
        return { step, sub: S.steps[step].sel, editable: step < S.length };
      },
      onEditStart: (t) => send('selectStep', { step: t.step, audition: false }),
      onTap: (t) => {
        if (t.step >= S.length) send('setLength', { length: t.step + 1 }); // tap an empty slot to extend up to it
        else send('selectStep', { step: t.step, audition: true });
      },
      onSwipe: (dir) => { ui.page += dir; render(); },
    });

    let wheelAcc = 0;
    pad.root.addEventListener('wheel', (e) => {
      const step = absOf();
      if (step >= S.length) return;
      e.preventDefault();
      wheelAcc += e.deltaY;
      if (Math.abs(wheelAcc) < 40) return;
      send('selectStep', { step, audition: false });
      send('shiftNote', { step, sub: S.steps[step].sel, delta: wheelAcc < 0 ? 1 : -1 });
      wheelAcc = 0;
    }, { passive: false });
  });

  // ---- cycle slots
  slots.forEach((slot, i) => {
    pitchGesture(slot.root, {
      target: (e) => {
        if (e.target.closest('.slot-x')) return null;
        const st = S.steps[S.selectedStep];
        return { step: S.selectedStep, sub: i, editable: i < st.count, add: i >= st.count };
      },
      onEditStart: (t) => send('selectSub', { step: t.step, sub: t.sub, audition: false }),
      onTap: (t) => {
        if (t.add) send('addSub', { step: t.step });
        else send('selectSub', { step: t.step, sub: t.sub, audition: true });
      },
    });
    slot.x.addEventListener('click', () => send('removeSub', { step: S.selectedStep, sub: i }));
  });

  // ---- overview grid: tap = select + page, drag = set length, double tap = move playhead
  (function () {
    const cellAt = (e) => {
      const hit = document.elementFromPoint(e.clientX, e.clientY);
      const cell = hit && hit.closest('.seq-cell');
      return cell ? +cell.dataset.i : -1;
    };
    let g = null;
    gridEl.addEventListener('pointerdown', (e) => {
      const start = cellAt(e);
      if (start < 0) return;
      gridEl.setPointerCapture(e.pointerId);
      g = { start, dragged: false };
    });
    gridEl.addEventListener('pointermove', (e) => {
      if (!g) return;
      const at = cellAt(e);
      if (at < 0 || (!g.dragged && at === g.start)) return;
      g.dragged = true;
      ui.lenPreview = at + 1;
      render();
    });
    const end = (e) => {
      if (!g) return;
      if (e.type === 'pointerup') {
        if (g.dragged) send('setLength', { length: ui.lenPreview });
        else {
          ui.page = Math.floor(g.start / 8);
          if (g.start < S.length) send('selectStep', { step: g.start, audition: true });
        }
      }
      g = null;
      ui.lenPreview = null;
      render();
    };
    gridEl.addEventListener('pointerup', end);
    gridEl.addEventListener('pointercancel', end);
    gridEl.addEventListener('dblclick', (e) => {
      const at = cellAt(e);
      if (at >= 0) send('jumpTo', { step: at });
    });
  })();

  // ---- keys: latch on = tap toggles; latch off = in the scale only while held
  (function () {
    const wrap = $('.keys-wrap');
    const held = new Map(); // pointerId -> pitch class
    const pcAt = (e) => {
      const hit = document.elementFromPoint(e.clientX, e.clientY);
      return hit && hit.dataset && hit.dataset.pc != null ? +hit.dataset.pc : -1;
    };
    const press = (id, pc) => {
      if (S.latch) send('setScaleNote', { pc, on: !S.scale[pc] });
      else send('setScaleNote', { pc, on: true });
      held.set(id, pc);
    };
    const release = (id) => {
      if (!S.latch && held.has(id)) send('setScaleNote', { pc: held.get(id), on: false });
      held.delete(id);
    };
    wrap.addEventListener('pointerdown', (e) => {
      const pc = pcAt(e);
      if (pc < 0) return;
      wrap.setPointerCapture(e.pointerId);
      press(e.pointerId, pc);
    });
    wrap.addEventListener('pointermove', (e) => {
      if (!held.has(e.pointerId)) return;
      const pc = pcAt(e);
      if (pc < 0 || pc === held.get(e.pointerId)) return;
      release(e.pointerId);
      press(e.pointerId, pc);
    });
    wrap.addEventListener('pointerup', (e) => release(e.pointerId));
    wrap.addEventListener('pointercancel', (e) => release(e.pointerId));
  })();

  // ---- note range: drag a handle, or drag the filled part to move the whole range
  (function () {
    const range = $('#range');
    const track = $('.range-track');
    let g = null;
    const noteAt = (e) => {
      const r = track.getBoundingClientRect();
      return clamp(Math.round(((e.clientX - r.left) / r.width) * 127), 0, 127);
    };
    range.addEventListener('pointerdown', (e) => {
      range.setPointerCapture(e.pointerId);
      const n = noteAt(e);
      const r = track.getBoundingClientRect();
      const px = (note) => r.left + (note / 127) * r.width;
      const nearMin = Math.abs(e.clientX - px(S.rangeMin));
      const nearMax = Math.abs(e.clientX - px(S.rangeMax));
      let mode;
      if (Math.min(nearMin, nearMax) <= 16) mode = nearMin < nearMax || (nearMin === nearMax && n < S.rangeMin) ? 'min' : 'max';
      else if (n > S.rangeMin && n < S.rangeMax) mode = 'move';
      else mode = n <= S.rangeMin ? 'min' : 'max';
      g = { mode, from: n, min: S.rangeMin, max: S.rangeMax };
      if (mode !== 'move') drag(e);
    });
    function drag(e) {
      if (!g) return;
      const n = noteAt(e);
      if (g.mode === 'min') send('setRange', { min: Math.min(n, S.rangeMax), max: S.rangeMax });
      else if (g.mode === 'max') send('setRange', { min: S.rangeMin, max: Math.max(n, S.rangeMin) });
      else {
        const shift = clamp(n - g.from, -g.min, 127 - g.max);
        send('setRange', { min: g.min + shift, max: g.max + shift });
      }
    }
    range.addEventListener('pointermove', drag);
    const end = () => { g = null; };
    range.addEventListener('pointerup', end);
    range.addEventListener('pointercancel', end);
  })();

  // ---- chance: drag across the dots to paint them on or off
  (function () {
    const grid = $('#prob-grid');
    let paint = null;
    const cellAt = (e) => {
      const hit = document.elementFromPoint(e.clientX, e.clientY);
      const cell = hit && hit.closest('.prob-cell');
      return cell ? +cell.dataset.cell : -1;
    };
    const apply = (e) => {
      const cell = cellAt(e);
      if (cell < 0 || cell === paint.last) return;
      paint.last = cell;
      send('probPaint', { cell, on: paint.on });
    };
    grid.addEventListener('pointerdown', (e) => {
      const cell = cellAt(e);
      if (cell < 0) return;
      grid.setPointerCapture(e.pointerId);
      paint = { on: !probCells[cell].classList.contains('is-on'), last: -1 };
      apply(e);
    });
    grid.addEventListener('pointermove', (e) => { if (paint) apply(e); });
    const end = () => { paint = null; };
    grid.addEventListener('pointerup', end);
    grid.addEventListener('pointercancel', end);
  })();
  $('#prob-slider').addEventListener('input', (e) => send('setProbPercent', { percent: +e.target.value }));
  $('#prob-dice').addEventListener('click', () => send('probReseed'));
  $('#prob-div').addEventListener('click', () => send('probDivision'));

  // ---- length: drag sideways, or tap the left / right half
  (function () {
    const len = $('#len');
    let g = null;
    len.addEventListener('pointerdown', (e) => {
      len.setPointerCapture(e.pointerId);
      g = { x: e.clientX, from: S.length, dragged: false };
    });
    len.addEventListener('pointermove', (e) => {
      if (!g) return;
      const delta = Math.round((e.clientX - g.x) / 10);
      if (!g.dragged && delta === 0) return;
      g.dragged = true;
      const next = clamp(g.from + delta, 1, 64);
      if (next !== S.length) send('setLength', { length: next });
    });
    len.addEventListener('pointerup', (e) => {
      if (g && !g.dragged) {
        const r = len.getBoundingClientRect();
        send('setLength', { length: clamp(S.length + (e.clientX < r.left + r.width / 2 ? -1 : 1), 1, 64) });
      }
      g = null;
    });
    len.addEventListener('pointercancel', () => { g = null; });
  })();

  // ---- bar
  $('#play').addEventListener('click', () => send('playPause'));
  $('#stop').addEventListener('click', () => send('stop'));
  $('#subdiv').addEventListener('click', () => send('setParam', { id: 'subdivision', value: (S.subdivision + 1) % 4 }));
  $('#latch').addEventListener('click', () => send('setParam', { id: 'latch', value: !S.latch }));
  $('#bake-scale').addEventListener('click', () => send('setParam', { id: 'bakeScale', value: !S.bakeScale }));
  $('#bake-range').addEventListener('click', () => send('setParam', { id: 'bakeRange', value: !S.bakeRange }));
  $('#preset-prev').addEventListener('click', () => send('cyclePreset', { dir: -1 }));
  $('#preset-next').addEventListener('click', () => send('cyclePreset', { dir: 1 }));

  // tempo: drag up / down to change, tap to open settings
  (function () {
    const tempo = $('#tempo');
    let g = null;
    tempo.addEventListener('pointerdown', (e) => {
      tempo.setPointerCapture(e.pointerId);
      g = { y: e.clientY, from: S.bpm, dragged: false };
    });
    tempo.addEventListener('pointermove', (e) => {
      if (!g || S.clockMode !== 0) return;
      const delta = Math.round((g.y - e.clientY) / 3);
      if (!g.dragged && Math.abs(delta) < 2) return;
      g.dragged = true;
      send('setParam', { id: 'bpm', value: clamp(g.from + delta, 20, 999) });
    });
    tempo.addEventListener('pointerup', () => {
      if (g && !g.dragged) openSettings();
      g = null;
    });
    tempo.addEventListener('pointercancel', () => { g = null; });
  })();

  // ================================================================ popovers
  const popover = $('#popover');
  let popoverAnchor = null;
  function openPopover(anchor, build) {
    if (!popover.hidden && popoverAnchor === anchor) return closePopover();
    popoverAnchor = anchor;
    popover.innerHTML = '';
    build(popover);
    popover.hidden = false;
    const a = anchor.getBoundingClientRect();
    const p = popover.getBoundingClientRect();
    popover.style.top = Math.min(a.bottom + 6, window.innerHeight - p.height - 8) + 'px';
    popover.style.left = clamp(a.right - p.width, 8, window.innerWidth - p.width - 8) + 'px';
  }
  function closePopover() {
    popover.hidden = true;
    popoverAnchor = null;
  }
  document.addEventListener('pointerdown', (e) => {
    if (popover.hidden || popover.contains(e.target) || (popoverAnchor && popoverAnchor.contains(e.target))) return;
    closePopover();
  });

  const menuItem = (icon, text, onClick, extra) => {
    const item = el('button', 'menu-item', (icon ? icons[icon] : '') + '<span class="grow"></span>' + (extra ? '<small>' + extra + '</small>' : ''));
    $('.grow', item).textContent = text;
    item.addEventListener('click', onClick);
    return item;
  };

  $('#menu-btn').addEventListener('click', (e) => {
    openPopover(e.currentTarget, (root) => {
      root.appendChild(menuItem('download', 'Export MIDI file', () => { closePopover(); send('exportMidi'); }));
      root.appendChild(menuItem('learn', 'MIDI learn', () => { closePopover(); send('learnToggle'); }, S.learn.active ? 'on' : ''));
      root.appendChild(menuItem('settings', 'Settings', () => { closePopover(); openSettings(); }));
      if (bridge.isMock) {
        root.appendChild(el('div', 'menu-sep'));
        root.appendChild(menuItem('sound', 'Prototype sound', () => { closePopover(); send('setParam', { id: 'sound', value: !S.sound }); }, S.sound ? 'on' : 'off'));
      }
    });
  });

  $('#preset-name').addEventListener('click', (e) => {
    openPopover(e.currentTarget, (root) => {
      if (!S.presets.length) root.appendChild(el('div', 'menu-empty', 'No saved presets yet.'));
      S.presets.forEach((name) => {
        const item = menuItem(null, name, () => { closePopover(); send('loadPreset', { name }); });
        item.classList.toggle('is-on', name === S.presetName);
        const del = el('span', 'menu-del', icons.close);
        del.setAttribute('role', 'button');
        del.setAttribute('aria-label', 'Delete preset ' + name);
        del.addEventListener('click', (ev) => { ev.stopPropagation(); closePopover(); send('deletePreset', { name }); });
        item.appendChild(del);
        root.appendChild(item);
      });
      root.appendChild(el('div', 'menu-sep'));
      const row = el('form', 'menu-row', '<input id="preset-save-name" type="text" maxlength="40" placeholder="Preset name" aria-label="Preset name" autocomplete="off"><button class="chip solid" type="submit">Save</button>');
      $('input', row).value = S.presetName;
      row.addEventListener('submit', (ev) => {
        ev.preventDefault();
        const name = $('input', row).value.trim();
        if (!name) return $('input', row).focus();
        closePopover();
        send('savePreset', { name });
      });
      root.appendChild(row);
    });
  });

  // ================================================================ settings
  function openSettings() {
    $('#sheet').hidden = false;
    drawSettings();
  }
  $('#sheet-close').addEventListener('click', () => { $('#sheet').hidden = true; });
  $('#sheet').addEventListener('pointerdown', (e) => { if (e.target.id === 'sheet') $('#sheet').hidden = true; });
  $$('#sheet .seg').forEach((seg) => seg.addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (b) send('setParam', { id: seg.dataset.param, value: +b.dataset.v });
  }));
  $('#set-bpm').addEventListener('change', (e) => send('setParam', { id: 'bpm', value: +e.target.value || 120 }));
  channelSel.addEventListener('change', (e) => send('setParam', { id: 'channel', value: +e.target.value }));
  $('#set-notelen').addEventListener('input', (e) => send('setParam', { id: 'noteLength', value: +e.target.value / 100 }));
  $('#set-preview').addEventListener('change', (e) => send('setParam', { id: 'previewNotes', value: e.target.checked }));
  $('#set-follow').addEventListener('change', (e) => send('setParam', { id: 'followStep', value: e.target.checked }));
  $('#set-colours-reset').addEventListener('click', () => send('resetColours'));

  // ================================================================ MIDI learn
  // While learning, every control with data-learn is a target: one capture-phase
  // listener swallows the gesture and selects the control instead.
  ['pointerdown', 'click'].forEach((type) => document.addEventListener(type, (e) => {
    if (!S || !S.learn.active) return;
    const target = e.target.closest && e.target.closest('[data-learn]');
    if (!target) return;
    e.stopPropagation();
    e.preventDefault();
    if (type === 'pointerdown') {
      ui.learnFocus = target.dataset.learn;
      send('learnSelect', { id: ui.learnFocus });
    }
  }, true));
  $('#learn-done').addEventListener('click', () => { ui.learnFocus = null; send('learnToggle'); });
  $('#learn-clear').addEventListener('click', () => send('learnClear', { id: ui.learnFocus }));

  // ================================================================ engine events
  const toastEl = $('#toast');
  let toastTimer = null;
  bridge.on('toast', (text) => {
    toastEl.textContent = text;
    toastEl.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { toastEl.hidden = true; }, 2600);
  });

  bridge.on('state', (state) => {
    S = state;
    if (S.transport === 'stopped') ui.playedPc = -1;
    render();
  });

  bridge.on('tick', (t) => {
    if (!S) return;
    S.currentStep = t.step;
    S.probTick = t.probTick;
    S.steps[t.step].cur = t.sub;
    ui.playedPc = t.note >= 0 ? t.note % 12 : -1;
    if (S.followStep) ui.page = Math.floor(t.step / 8);
    render();
  });

  document.addEventListener('contextmenu', (e) => e.preventDefault());
  bridge.ready();
})();
