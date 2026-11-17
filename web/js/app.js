// Gambler UI. Renders the latest engine snapshot and turns gestures into
// bridge actions. It holds no sequencer logic of its own; the only local state
// is view state (which page is showing, what is being dragged).
//
// Keep every file under web/ ASCII-only (use \uXXXX escapes or HTML entities):
// some web views misread non-ASCII bytes in resources the plugin serves.
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

  let S = null; // latest engine snapshot
  const ui = { page: 0, drag: null, lenPreview: null, playedPc: -1, range: 0, view: 'scale' };
  try { ui.view = localStorage.getItem('gambler.view') === 'melody' ? 'melody' : 'scale'; } catch (e) { /* storage unavailable */ }

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
    { id: 'randomize-step', label: 'Random', hint: 'step', icon: 'random', run: () => send('randomize', { scope: 'step' }) },
    { id: 'randomize-all', label: 'Random', hint: 'all', icon: 'random', run: () => send('randomize', { scope: 'all' }) },
    { id: 'clear-step', label: 'Clear', hint: 'step', icon: 'clear', run: () => send('clear', { scope: 'step' }) },
    { id: 'clear-page', label: 'Clear', hint: 'page', icon: 'clear', run: () => send('clear', { scope: 'page' }) },
    { id: 'undo', label: 'Undo', icon: 'undo', run: () => send('undo') },
  ];
  const actionEls = {};
  ACTIONS.forEach((a) => {
    const btn = el('button', 'action', icons[a.icon] +
      '<span class="action-text"><b>' + a.label + '</b>' + (a.hint ? '<small>' + a.hint + '</small>' : '') + '</span>');
    if (a.id !== 'undo') btn.dataset.learn = a.id;
    $('#actions').appendChild(btn);
    actionEls[a.id] = btn;

    if (a.kind === 'momentary') {
      const set = (on) => () => send('roll', { on });
      btn.addEventListener('pointerdown', (e) => { btn.setPointerCapture(e.pointerId); set(true)(); });
      btn.addEventListener('pointerup', set(false));
      btn.addEventListener('pointercancel', set(false));
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
    if (ui.page !== ui.sentPage) {
      ui.sentPage = ui.page; // MIDI-mapped step buttons act on the visible page
      send('setPage', { page: ui.page });
    }
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
    const followNames = ['Chord off', 'Chord span', 'Chord exact'];
    $('#follow').textContent = followNames[S.rangeFollow || 0];
    $('#follow').classList.toggle('is-follow', !!S.rangeFollow);
    $('#bake-range').classList.toggle('is-ignored', !!S.rangeFollow && S.bakeRange);

    // ---- range
    drawRanges();

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
      slot.note.innerHTML = isAdd ? icons.plus : note >= 0 ? noteName(note) : '\u2013';
      root.setAttribute('aria-label', isAdd ? 'Add a note to this step\u2019s cycle' : 'Cycle note ' + (i + 1));
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
      pad.note.textContent = ghost ? '' : note >= 0 ? noteName(note) : '\u2013';
      pad.pips.innerHTML = !ghost && st.count > 1
        ? Array.from({ length: st.count }, (_, k) => '<i' + (k === shownSub(st) ? ' class="is-cur"' : '') + '></i>').join('')
        : '';
    });

    // ---- actions
    actionEls.roll.classList.toggle('is-on', S.roll);
    actionEls.undo.disabled = !S.canUndo;
    actionEls.duplicate.disabled = S.length >= 64;

    document.body.classList.toggle('view-melody', ui.view === 'melody');
    $$('#view-seg button').forEach((b) => b.classList.toggle('is-on', b.dataset.v === ui.view));
    if (ui.view === 'melody') roll.draw(len);

    drawLearn();
    if (!$('#sheet').hidden) drawSettings();
  }

  let rangeEls = [];
  let chordMarks = []; // the held chord drawn over the range track
  function drawRanges() {
    const track = $('#range-track');
    if (rangeEls.length !== S.ranges.length) {
      track.innerHTML = '';
      chordMarks = [];
      rangeEls = S.ranges.map(() => {
        const parts = { fill: el('div', 'range-fill'), min: el('div', 'range-handle'), max: el('div', 'range-handle') };
        track.append(parts.fill, parts.min, parts.max);
        return parts;
      });
    }
    ui.range = clamp(ui.range, 0, S.ranges.length - 1);
    S.ranges.forEach((r, i) => {
      const lo = (r.min / 127) * 100;
      const hi = (r.max / 127) * 100;
      const parts = rangeEls[i];
      parts.fill.style.left = lo + '%';
      parts.fill.style.width = hi - lo + '%';
      parts.min.style.left = lo + '%';
      parts.max.style.left = hi + '%';
      Object.values(parts).forEach((node) => node.classList.toggle('is-active', i === ui.range));
    });
    // a chord on MIDI input is setting the range: draw it over the manual ranges
    const chord = S.chord || [];
    const following = !!S.rangeFollow && chord.length > 0;
    $('#range').classList.toggle('is-following', following);
    const marks = following ? (S.rangeFollow === 1 ? 1 : chord.length) : 0;
    while (chordMarks.length > marks) chordMarks.pop().remove();
    while (chordMarks.length < marks) chordMarks.push(track.appendChild(el('div')));
    const pct = (n) => (n / 127) * 100 + '%';
    if (following && S.rangeFollow === 1) {
      chordMarks[0].className = 'range-chord';
      chordMarks[0].style.left = pct(chord[0]);
      chordMarks[0].style.width = ((chord[chord.length - 1] - chord[0]) / 127) * 100 + '%';
    } else {
      chordMarks.forEach((mark, i) => { mark.className = 'range-tick'; mark.style.left = pct(chord[i]); mark.style.width = ''; });
    }

    const active = S.ranges[ui.range];
    const label = $('#range-label');
    label.classList.toggle('is-following', following);
    label.textContent = !following ? noteName(active.min) + '\u2013' + noteName(active.max)
      : S.rangeFollow === 1 ? noteName(chord[0]) + '\u2013' + noteName(chord[chord.length - 1])
      : chord.length + (chord.length === 1 ? ' note' : ' notes');
    $('#range-add').hidden = S.ranges.length >= 3;
    $('#range-remove').hidden = S.ranges.length < 2;
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
    $('#prob-div').textContent = probDim + '\u00d7' + probDim;
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
    const canClear = !!(ui.learnFocus && S.learn.mappings[ui.learnFocus]);
    $('#learn-text').textContent = selected ? 'Move a knob or play a key\u2026' : 'Tap a control to map it';
    $('#learn-text').hidden = canClear && !selected;
    $('#learn-clear').hidden = !canClear || !!selected;
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
    const track = $('#range-track');
    let g = null;
    const noteAt = (e) => {
      const r = track.getBoundingClientRect();
      return clamp(Math.round(((e.clientX - r.left) / r.width) * 127), 0, 127);
    };
    range.addEventListener('pointerdown', (e) => {
      range.setPointerCapture(e.pointerId);
      const n = noteAt(e);
      const rect = track.getBoundingClientRect();
      const px = (note) => rect.left + (note / 127) * rect.width;
      // nearest handle across all ranges; the selected range wins ties
      let best = null;
      S.ranges.forEach((r, index) => ['min', 'max'].forEach((mode) => {
        const dist = Math.abs(e.clientX - px(r[mode])) - (index === ui.range ? 0.5 : 0);
        if (!best || dist < best.dist) best = { index, mode, dist };
      }));
      const inside = S.ranges.findIndex((r, i) => n > r.min && n < r.max && (i === ui.range || !(n > S.ranges[ui.range].min && n < S.ranges[ui.range].max)));
      const index = best.dist <= 12 || inside < 0 ? best.index : inside;
      const mode = best.dist <= 12 || inside < 0 ? best.mode : 'move';
      ui.range = index;
      g = { index, mode, from: n, min: S.ranges[index].min, max: S.ranges[index].max };
      if (mode !== 'move') drag(e);
      render();
    });
    function drag(e) {
      if (!g) return;
      const n = noteAt(e);
      const r = S.ranges[g.index];
      if (!r) return;
      if (g.mode === 'min') send('setRange', { index: g.index, min: Math.min(n, r.max), max: r.max });
      else if (g.mode === 'max') send('setRange', { index: g.index, min: r.min, max: Math.max(n, r.min) });
      else {
        const shift = clamp(n - g.from, -g.min, 127 - g.max);
        send('setRange', { index: g.index, min: g.min + shift, max: g.max + shift });
      }
    }
    range.addEventListener('pointermove', drag);
    const end = () => { g = null; };
    range.addEventListener('pointerup', end);
    range.addEventListener('pointercancel', end);
  })();

  $('#range-add').addEventListener('click', () => { ui.range = S.ranges.length; send('addRange'); });
  $('#range-remove').addEventListener('click', () => send('removeRange', { index: ui.range }));

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
  $('#follow').addEventListener('click', () => send('setParam', { id: 'rangeFollow', value: ((S.rangeFollow || 0) + 1) % 3 }));
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

  // ================================================================ melody view
  // The whole sequence as a grid: steps left to right, allowed notes bottom to top.
  // The note a step plays next is a solid block; its other cycle notes are ghosts.
  const roll = (function () {
    const panel = $('#roll');
    const canvas = $('#roll-canvas');
    const ctx = canvas.getContext('2d');
    const GUTTER = 48;   // note names
    const REST_H = 16;   // lane along the bottom for rests
    const MIN_ROW = 13;  // rows never get thinner than this; extra rows scroll
    const token = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();
    const C = { panel: token('--panel'), raise: token('--raise'), line: token('--line'), ink: token('--ink'),
      mute: token('--mute'), cream: token('--cream'), rest: token('--rest'), bg: token('--bg') };

    let geo = null;
    let scroll = null; // index of the top visible row; null until first layout

    // Rows: every note the engine allows right now (scale, ranges, a held chord),
    // plus any note already in the sequence.
    function rowNotes(len) {
      const notes = new Set(S.allowed);
      for (let i = 0; i < len; i++) {
        const st = S.steps[i];
        for (let k = 0; k < st.count; k++) if (st.notes[k] >= 0) notes.add(st.notes[k]);
      }
      return Array.from(notes).sort((a, b) => b - a);
    }

    function layout(len) {
      const w = panel.clientWidth;
      const h = panel.clientHeight;
      const dpr = window.devicePixelRatio || 1;
      if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
        canvas.width = Math.round(w * dpr);
        canvas.height = Math.round(h * dpr);
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

      const rows = rowNotes(len);
      const index = new Map(rows.map((n, i) => [n, i]));
      const gridH = h - REST_H;
      const visible = Math.max(1, Math.min(rows.length, Math.floor(gridH / MIN_ROW)));
      const maxScroll = Math.max(0, rows.length - visible);
      if (scroll == null) {
        // start centred on the notes in use
        const used = [];
        for (let i = 0; i < len; i++) { const n = S.steps[i].notes[shownSub(S.steps[i])]; if (n >= 0) used.push(index.get(n)); }
        used.sort((a, b) => a - b);
        scroll = used.length ? used[Math.floor(used.length / 2)] - Math.floor(visible / 2) : 0;
      }
      scroll = clamp(Math.round(scroll), 0, maxScroll);
      const cols = Math.min(64, Math.max(16, Math.ceil(len / 8) * 8));
      geo = { w, h, rows, index, gridH, visible, maxScroll, cols, len, rowH: gridH / visible, colW: (w - GUTTER) / cols };
    }

    function block(x, y, w, h, radius) {
      ctx.beginPath();
      if (ctx.roundRect) ctx.roundRect(x, y, w, h, radius); else ctx.rect(x, y, w, h);
    }

    function draw(len) {
      if (!panel.clientWidth) return;
      layout(len);
      const { w, h, rows, index, gridH, visible, cols, rowH, colW } = geo;
      const playing = S.transport !== 'stopped';
      const colX = (col) => GUTTER + col * colW;

      ctx.clearRect(0, 0, w, h);
      ctx.fillStyle = C.panel;
      ctx.fillRect(0, 0, w, h);

      // the eight steps the pads below are showing
      ctx.fillStyle = C.raise;
      ctx.fillRect(colX(ui.page * 8), 0, colW * 8, h);

      // rows and note names
      ctx.textBaseline = 'middle';
      ctx.font = '600 ' + Math.min(11, Math.max(8, rowH - 3)) + 'px Inter, system-ui, sans-serif';
      // nothing allowed (e.g. Latch off with no scale keys held): say so instead of an empty row
      if (!rows.length) {
        ctx.fillStyle = C.mute;
        ctx.textAlign = 'center';
        ctx.fillText('No notes allowed: turn on scale keys or hold a chord', GUTTER + (w - GUTTER) / 2, gridH / 2);
        ctx.textAlign = 'left';
      }
      for (let v = 0; v < Math.min(visible, rows.length); v++) {
        const note = rows[scroll + v];
        const y = v * rowH;
        if (note % 12 === 0) { ctx.fillStyle = 'rgba(255,255,255,0.045)'; ctx.fillRect(GUTTER, y, w - GUTTER, rowH); }
        ctx.fillStyle = S.colours[note % 12];
        ctx.fillRect(0, y + 1, 5, rowH - 2);
        ctx.fillStyle = note % 12 === 0 ? C.ink : C.mute;
        ctx.fillText(noteName(note), 10, y + rowH / 2 + 0.5);
        ctx.fillStyle = C.line;
        ctx.globalAlpha = 0.35;
        ctx.fillRect(GUTTER, Math.round(y + rowH) - 0.5, w - GUTTER, 1);
        ctx.globalAlpha = 1;
      }

      // column lines: faint per step, stronger per beat of four, strongest per page of eight
      for (let col = 0; col <= cols; col++) {
        ctx.fillStyle = C.line;
        ctx.globalAlpha = col % 8 === 0 ? 1 : col % 4 === 0 ? 0.6 : 0.25;
        ctx.fillRect(Math.round(colX(col)) - 0.5, 0, 1, h);
      }
      ctx.globalAlpha = 1;

      // rest lane
      ctx.fillStyle = C.bg;
      ctx.globalAlpha = 0.55;
      ctx.fillRect(GUTTER, gridH, w - GUTTER, REST_H);
      ctx.globalAlpha = 1;
      ctx.fillStyle = C.mute;
      ctx.fillText('rest', 10, gridH + REST_H / 2 + 0.5);

      // playhead column
      if (playing && S.currentStep >= 0 && S.currentStep < len) {
        ctx.fillStyle = C.cream;
        ctx.globalAlpha = 0.1;
        ctx.fillRect(colX(S.currentStep), 0, colW, h);
        ctx.globalAlpha = 1;
      }

      // notes: ghosts first so the solid "plays next" block sits on top
      const pad = Math.min(2, colW * 0.12);
      for (const pass of ['ghost', 'main']) {
        for (let step = 0; step < len; step++) {
          const st = S.steps[step];
          const shown = shownSub(st);
          for (let k = 0; k < st.count; k++) {
            const isMain = k === shown;
            if (isMain !== (pass === 'main')) continue;
            const note = st.notes[k];
            const selected = step === S.selectedStep && k === st.sel;
            const x = colX(step) + pad;
            const bw = colW - pad * 2;

            if (note < 0) {
              if (!isMain) continue;
              ctx.fillStyle = C.rest;
              block(x, gridH + 3, bw, REST_H - 6, 3);
              ctx.fill();
              if (selected) { ctx.strokeStyle = C.cream; ctx.lineWidth = 2; ctx.stroke(); }
              continue;
            }

            const v = index.get(note) - scroll;
            if (v < 0 || v >= visible) {
              if (!isMain) continue; // off-screen: a small marker at the edge it left through
              ctx.fillStyle = S.colours[note % 12];
              const top = v < 0;
              ctx.beginPath();
              ctx.moveTo(x + bw / 2, top ? 2 : gridH - 2);
              ctx.lineTo(x + bw / 2 - 4, top ? 8 : gridH - 8);
              ctx.lineTo(x + bw / 2 + 4, top ? 8 : gridH - 8);
              ctx.fill();
              continue;
            }

            const isPlaying = isMain && playing && step === S.currentStep;
            ctx.globalAlpha = isMain ? 1 : 0.36;
            ctx.fillStyle = isPlaying ? C.cream : S.colours[note % 12];
            block(x, v * rowH + 1.5, bw, rowH - 3, 3);
            ctx.fill();
            ctx.globalAlpha = 1;
            if (selected) { ctx.strokeStyle = C.cream; ctx.lineWidth = 2; ctx.stroke(); }
          }
        }
      }

      // steps beyond the sequence length
      if (len < cols) {
        ctx.fillStyle = C.bg;
        ctx.globalAlpha = 0.6;
        ctx.fillRect(colX(len), 0, w - colX(len), h);
        ctx.globalAlpha = 1;
      }

      // more rows above or below
      ctx.fillStyle = C.mute;
      if (scroll > 0) { ctx.beginPath(); ctx.moveTo(GUTTER - 12, 3); ctx.lineTo(GUTTER - 16, 9); ctx.lineTo(GUTTER - 8, 9); ctx.fill(); }
      if (scroll < geo.maxScroll) { ctx.beginPath(); ctx.moveTo(GUTTER - 12, gridH - 3); ctx.lineTo(GUTTER - 16, gridH - 9); ctx.lineTo(GUTTER - 8, gridH - 9); ctx.fill(); }
    }

    // ---- gestures
    function locate(e) {
      const r = canvas.getBoundingClientRect();
      const x = e.clientX - r.left;
      const y = e.clientY - r.top;
      const row = clamp(Math.floor(y / geo.rowH), 0, geo.visible - 1);
      return {
        y,
        gutter: x < GUTTER,
        col: Math.floor((x - GUTTER) / geo.colW),
        // null: over the grid but there are no rows to land on
        note: y >= geo.gridH ? -1 : geo.rows.length ? geo.rows[scroll + row] : null,
      };
    }

    let g = null;

    // Drawing: put the step's "plays next" note where the pointer is, one step at a time.
    function paint(at) {
      if (at.col < 0 || at.col >= geo.len) return;
      if (at.col === g.col && at.note === g.note) return;
      g.col = at.col;
      g.note = at.note;
      send('setNote', { step: at.col, sub: shownSub(S.steps[at.col]), note: at.note, undo: !g.changed });
      g.changed = true;
    }

    canvas.addEventListener('pointerdown', (e) => {
      if (e.button || !geo) return;
      canvas.setPointerCapture(e.pointerId);
      const at = locate(e);

      if (at.gutter) { g = { mode: 'scroll', y: at.y, from: scroll }; return; }
      if (at.col >= geo.len) { g = { mode: 'extend', col: at.col }; return; }
      if (at.note == null) return;

      const st = S.steps[at.col];
      const shown = shownSub(st);
      let sub = -1;
      if (at.note >= 0) {
        if (st.notes[shown] === at.note) sub = shown;
        else for (let k = 0; k < st.count && sub < 0; k++) if (st.notes[k] === at.note) sub = k;
      } else if (st.notes[shown] < 0) {
        sub = shown;
      }

      ui.page = Math.floor(at.col / 8);
      if (sub >= 0) {
        // grabbed an existing note: drag it, or just tap to select it
        g = { mode: 'move', step: at.col, sub, note: at.note, changed: false };
        send('selectStep', { step: at.col, audition: false });
        send('selectSub', { step: at.col, sub, audition: false });
      } else {
        g = { mode: 'draw', col: -1, note: null, changed: false };
        send('selectStep', { step: at.col, audition: false });
        paint(at);
      }
    });

    canvas.addEventListener('pointermove', (e) => {
      if (!g) return;
      const at = locate(e);
      if (g.mode === 'scroll') {
        scroll = clamp(g.from - Math.round((at.y - g.y) / geo.rowH), 0, geo.maxScroll);
        render();
      } else if (at.note == null) {
        return;
      } else if (g.mode === 'move') {
        if (at.note === g.note) return;
        g.note = at.note;
        send('setNote', { step: g.step, sub: g.sub, note: at.note, undo: !g.changed });
        g.changed = true;
      } else if (g.mode === 'draw') {
        paint(at);
      }
    });

    const end = (e) => {
      if (!g) return;
      if (e.type === 'pointerup') {
        if (g.mode === 'move' && !g.changed) send('selectSub', { step: g.step, sub: g.sub, audition: true });
        if (g.mode === 'draw' && g.col >= 0) { ui.page = Math.floor(g.col / 8); send('selectStep', { step: g.col, audition: false }); }
        if (g.mode === 'extend' && locate(e).col === g.col) send('setLength', { length: g.col + 1 });
      }
      g = null;
      render();
    };
    canvas.addEventListener('pointerup', end);
    canvas.addEventListener('pointercancel', end);
    canvas.addEventListener('wheel', (e) => {
      if (!geo || !geo.maxScroll) return;
      e.preventDefault();
      scroll = clamp(scroll + (e.deltaY > 0 ? 1 : -1), 0, geo.maxScroll);
      render();
    }, { passive: false });

    if (window.ResizeObserver) new ResizeObserver(() => render()).observe(panel);
    else window.addEventListener('resize', () => render());

    return { draw };
  })();

  $('#view-seg').addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (!b) return;
    ui.view = b.dataset.v;
    try { localStorage.setItem('gambler.view', ui.view); } catch (err) { /* storage unavailable */ }
    render();
  });

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
    // open towards whichever side of the window has room
    const left = a.left + a.width / 2 < window.innerWidth / 2 ? a.left : a.right - p.width;
    popover.style.left = clamp(left, 8, window.innerWidth - p.width - 8) + 'px';
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

  bridge.on('noteOff', () => {
    ui.playedPc = -1;
    render();
  });

  document.addEventListener('contextmenu', (e) => e.preventDefault());
  bridge.ready();
})();
