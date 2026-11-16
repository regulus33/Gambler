// Bundles web/ into one self-contained HTML fragment (styles and scripts inlined)
// so the UI can be shared as a single page for review outside the plugin.
// Usage: node web/tools/build-prototype.mjs <output.html>
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const read = (p) => readFileSync(join(root, p), 'utf8');

const html = read('index.html');
const body = html.split('<!--APP-->')[1].split('<!--/APP-->')[0];
const css = read('css/style.css').replace(/@font-face\s*{[^}]*}\s*/, '');
const scripts = [...html.matchAll(/<script src="([^"]+)"><\/script>/g)].map((m) => read(m[1]));

writeFileSync(process.argv[2], [
  '<title>Gambler UI Prototype</title>',
  '<meta name="claude:viewport-fit-sides" content="cover">',
  '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Inter:wght@400..800&display=swap">',
  '<style>' + css + '</style>',
  body,
  ...scripts.map((s) => '<script>' + s + '</script>'),
].join('\n'));
