// Checks the translations of the watch app in resources/lang: every text
// the C code translates has a translation in every language, texts that
// go into fixed-size buffers fit them, and no two texts share a hash.
// Run with: node test/i18n.test.js
const assert = require('assert');
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const langDir = path.join(root, 'resources', 'lang');
const srcDir = path.join(root, 'src', 'c');

// "English = translation" lines, as i18n.c reads them
function readLanguage(file) {
  const texts = new Map();
  for (const line of fs.readFileSync(file, 'utf8').split('\n')) {
    const separator = line.indexOf(' = ');
    if (line.startsWith('#') || separator < 0) continue;
    const english = line.slice(0, separator);
    assert.ok(!texts.has(english), `${file}: "${english}" twice`);
    texts.set(english, line.slice(separator + 3));
  }
  return texts;
}

// Texts the C code passes to tr(), directly or as an empty_text
const used = new Set();
for (const file of fs.readdirSync(srcDir).filter(f => f.endsWith('.c'))) {
  const code = fs.readFileSync(path.join(srcDir, file), 'utf8');
  for (const m of code.matchAll(
           /(?:\btr\(|\.empty_text = )"((?:[^"\\]|\\.)*)"/g)) {
    used.add(m[1]);
  }
}
assert.ok(used.size > 50, 'texts found in src/c');

// i18n.c keeps only this hash of each English text (FNV-1a over the
// UTF-8 bytes), so all texts need different hashes
function hash(text) {
  let h = 2166136261;
  for (const byte of Buffer.from(text.replace(/\\n/g, '\n'))) {
    h = Math.imul(h ^ byte, 16777619) >>> 0;
  }
  return h;
}
const hashes = new Map();
for (const english of used) {
  const h = hash(english);
  assert.ok(!hashes.has(h), `"${english}" and "${hashes.get(h)}" share a hash`);
  hashes.set(h, english);
}

// Buffers in main.c the formatted texts are written into, with the
// largest number they show
const BUFFERS = {
  'None saved': 16,
  '%d saved': 16,  // at most DRAFTS_MAX (3) drafts
  'No open todos': 32,
  '1 open todo': 32,
  '%d open todos': 32
};
const LARGEST_NUMBER = {
  '%d saved': '3',
  '%d open todos': '9999'
};

const files = fs.readdirSync(langDir).filter(f => f.endsWith('.txt'));
assert.deepStrictEqual(files.sort(), ['de.txt', 'es.txt', 'fr.txt', 'it.txt']);
for (const file of files) {
  const texts = readLanguage(path.join(langDir, file));
  for (const english of used) {
    assert.ok(texts.has(english), `${file}: missing "${english}"`);
  }
  for (const [english, text] of texts) {
    assert.ok(used.has(english), `${file}: "${english}" is not used`);
    assert.ok(text.trim() !== '', `${file}: "${english}" is empty`);
    assert.strictEqual(
        (text.match(/%d/g) || []).length, (english.match(/%d/g) || []).length,
        `${file}: "${english}" needs the same %d`);
    if (english in BUFFERS) {
      const shown = text.replace('%d', LARGEST_NUMBER[english] || '');
      assert.ok(
          Buffer.byteLength(shown) < BUFFERS[english],
          `${file}: "${text}" does not fit ${BUFFERS[english]} bytes`);
    }
  }
}
console.log('ALL OK');
