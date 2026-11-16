// Inline SVG icons (24px grid, stroked with currentColor) and piano key shapes.
(function () {
  'use strict';
  const G = (window.Gambler = window.Gambler || {});

  const svg = (body, filled) =>
    '<svg class="icon" viewBox="0 0 24 24" aria-hidden="true" fill="' + (filled ? 'currentColor' : 'none') +
    '" stroke="' + (filled ? 'none' : 'currentColor') +
    '" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' + body + '</svg>';

  const dot = (x, y) => '<circle cx="' + x + '" cy="' + y + '" r="1.3" fill="currentColor" stroke="none"/>';

  G.icons = {
    dice: svg('<rect x="3" y="3" width="18" height="18" rx="3.5"/>' + dot(8, 8) + dot(16, 8) + dot(12, 12) + dot(8, 16) + dot(16, 16)),
    arrowL: svg('<path d="M19 12H5M11 6l-6 6 6 6"/>'),
    arrowR: svg('<path d="M5 12h14M13 6l6 6-6 6"/>'),
    duplicate: svg('<rect x="8" y="8" width="12" height="12" rx="2"/><path d="M16 8V6a2 2 0 0 0-2-2H6a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h2"/>'),
    shuffle: svg('<path d="M3 7h3c5 0 7 10 12 10h3M3 17h3c1.6 0 2.9-1 4.1-2.4M13.9 9.4C15.1 8 16.4 7 18 7h3M18 4l3 3-3 3M18 14l3 3-3 3"/>'),
    random: svg('<path d="M12 3v4M12 17v4M3 12h4M17 12h4M6 6l2.5 2.5M15.5 15.5 18 18M18 6l-2.5 2.5M8.5 15.5 6 18"/>'),
    clear: svg('<path d="M9 5h10a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H9l-6-7 6-7zM12 10l4 4M16 10l-4 4"/>'),
    undo: svg('<path d="M8 5 3 10l5 5M3 10h11a5.5 5.5 0 0 1 0 11h-4"/>'),
    play: svg('<path d="M7 4l13 8-13 8z"/>', true),
    pause: svg('<rect x="5" y="4" width="5" height="16" rx="1"/><rect x="14" y="4" width="5" height="16" rx="1"/>', true),
    stop: svg('<rect x="5" y="5" width="14" height="14" rx="2"/>', true),
    more: svg('<circle cx="5" cy="12" r="1.8"/><circle cx="12" cy="12" r="1.8"/><circle cx="19" cy="12" r="1.8"/>', true),
    save: svg('<path d="M5 3h11l4 4v12a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM7 3v6h8V3M7 21v-7h10v7"/>'),
    chevL: svg('<path d="M15 5l-7 7 7 7"/>'),
    chevR: svg('<path d="M9 5l7 7-7 7"/>'),
    chevDown: svg('<path d="M5 9l7 7 7-7"/>'),
    download: svg('<path d="M12 3v12M7 10l5 5 5-5M4 20h16"/>'),
    learn: svg('<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="2"/><path d="M12 2v4M12 18v4M2 12h4M18 12h4"/>'),
    settings: svg('<path d="M4 7h10M18 7h2M4 17h2M10 17h10"/><circle cx="16" cy="7" r="2"/><circle cx="8" cy="17" r="2"/>'),
    sound: svg('<path d="M4 9v6h4l5 4V5L8 9H4zM16.5 8.5a5 5 0 0 1 0 7"/>'),
    close: svg('<path d="M6 6l12 12M18 6 6 18"/>'),
    plus: svg('<path d="M12 5v14M5 12h14"/>'),
  };

  // ---- Piano keys: the original Figma outlines, as SVG path data ----
  const n = (v) => +v.toFixed(2);
  const P = (x, cmds) =>
    cmds.map((c) => (typeof c === 'string' ? c : n(c[0] + x) + ' ' + c[1])).join(' ').replace(/ ([MLCZ])/g, '$1');

  const rightNotch = (x) => P(x, ['M', [57.1, 127.7], 'C', [57.1, 124.9], [54.9, 122.7], [52.1, 122.7], 'L', [46.3, 122.7],
    'C', [43.6, 122.7], [41.3, 120.4], [41.3, 117.7], 'L', [41.3, 5.5], 'C', [41.3, 2.7], [39.1, 0.5], [36.3, 0.5], 'L', [5.5, 0.5],
    'C', [2.7, 0.5], [0.5, 2.7], [0.5, 5.5], 'L', [0.5, 194.5], 'C', [0.5, 197.3], [2.7, 199.5], [5.5, 199.5], 'L', [52.1, 199.5],
    'C', [54.9, 199.5], [57.1, 197.3], [57.1, 194.5], 'Z']);

  const bothNotches = (x) => P(x, ['M', [60.7, 127.7], 'C', [60.7, 124.9], [58.4, 122.7], [55.7, 122.7], 'L', [47.5, 122.7],
    'C', [44.7, 122.7], [42.5, 120.4], [42.5, 117.7], 'L', [42.5, 5.5], 'C', [42.5, 2.7], [40.3, 0.5], [37.5, 0.5], 'L', [23.7, 0.5],
    'C', [20.9, 0.5], [18.7, 2.7], [18.7, 5.5], 'L', [18.7, 117.7], 'C', [18.7, 120.4], [16.5, 122.7], [13.7, 122.7], 'L', [5.5, 122.7],
    'C', [2.7, 122.7], [0.5, 124.9], [0.5, 127.7], 'L', [0.5, 194.5], 'C', [0.5, 197.3], [2.7, 199.5], [5.5, 199.5], 'L', [55.7, 199.5],
    'C', [58.4, 199.5], [60.7, 197.3], [60.7, 194.5], 'Z']);

  const leftNotch = (x) => P(x, ['M', [0.5, 127.7], 'C', [0.5, 124.9], [2.7, 122.7], [5.5, 122.7], 'L', [11.3, 122.7],
    'C', [14.1, 122.7], [16.3, 120.4], [16.3, 117.7], 'L', [16.3, 5.5], 'C', [16.3, 2.7], [18.5, 0.5], [21.3, 0.5], 'L', [52.1, 0.5],
    'C', [54.9, 0.5], [57.1, 2.7], [57.1, 5.5], 'L', [57.1, 194.5], 'C', [57.1, 197.3], [54.9, 199.5], [52.1, 199.5], 'L', [5.5, 199.5],
    'C', [2.7, 199.5], [0.5, 197.3], [0.5, 194.5], 'Z']);

  const black = (x) => P(x, ['M', [5.5, 0.5], 'C', [2.7, 0.5], [0.5, 2.8], [0.5, 5.5], 'L', [1.1, 117.5],
    'C', [1.1, 120.2], [3.3, 122.4], [5.9, 122.5], 'L', [18.1, 122.9], 'L', [35.5, 122.8],
    'C', [38.3, 122.8], [40.5, 120.6], [40.5, 117.8], 'L', [40.5, 5.5], 'C', [40.5, 2.7], [38.3, 0.5], [35.5, 0.5], 'Z']);

  // pitch class -> shape, x offset and label centre (viewBox units)
  const W = 408;
  const H = 200;
  const key = (pc, isBlack, d, cx) => ({ pc, isBlack, d, cx: cx / W, cy: (isBlack ? 104 : 170) / H });
  G.keys = {
    viewBox: '0 0 ' + W + ' ' + H,
    list: [
      key(0, false, rightNotch(0), 28.8),
      key(2, false, bothNotches(56.6), 87.2),
      key(4, false, leftNotch(116.8), 145.6),
      key(5, false, rightNotch(173.4), 202.2),
      key(7, false, bothNotches(230), 260.6),
      key(9, false, bothNotches(290.2), 320.8),
      key(11, false, leftNotch(350.4), 379.2),
      key(1, true, black(38.3), 58.8),
      key(3, true, black(96.7), 117.2),
      key(6, true, black(211.7), 232.2),
      key(8, true, black(270.1), 290.6),
      key(10, true, black(330.3), 350.8),
    ],
  };
})();
