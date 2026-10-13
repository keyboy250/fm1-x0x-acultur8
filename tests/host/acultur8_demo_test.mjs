// SPDX-License-Identifier: GPL-3.0-only
// Validate the actual published pair with its actual published parser and WASM.
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const site = path.resolve(process.argv[2]);
// The build directory has no package.json. Import the self-contained ES module as data.
const parserText = fs.readFileSync(path.join(site, 'emu/slice-import.js'), 'utf8');
const {readWav, parseMarkers} = await import('data:text/javascript;base64,' + Buffer.from(parserText).toString('base64'));
const wav = fs.readFileSync(path.join(site, 'demo/acultur8-demo_slices.wav'));
const text = fs.readFileSync(path.join(site, 'demo/acultur8-demo_markers.json'), 'utf8');
const raw = JSON.parse(text);
assert.equal(createHash('sha256').update(wav).digest('hex'), raw.audioSha256);
const audio = readWav(wav.buffer.slice(wav.byteOffset, wav.byteOffset + wav.byteLength));
const markers = parseMarkers(text, 'demo.json', audio);
assert.equal(markers.slices.length, 6);
assert.equal(markers.bpm, 120);
assert.equal(markers.length, 88200);
for (const bpm of [90, 120, 180]) {
  let mem;
  const {instance} = await WebAssembly.instantiate(fs.readFileSync(path.join(site, 'emu/x0x.wasm')), {
    wasi_snapshot_preview1: {clock_time_get: (a, b, p) => { new DataView(mem.buffer).setBigUint64(p, 0n, true); return 0; }}
  });
  const e = instance.exports; mem = e.memory; e._initialize(); e.web_boot();
  new Float32Array(mem.buffer, e.web_marked_pcm(), audio.pcm.length).set(audio.pcm);
  new Uint32Array(mem.buffer, e.web_marked_meta(), markers.slices.length * 3).set(markers.slices.flatMap(s => [s.start, s.end, s.time]));
  assert.equal(e.web_marked_load(audio.frames, audio.rate, audio.channels, markers.slices.length, markers.length), 0);
  e.web_seq_set_bpm(bpm); assert.equal(e.web_sync_arm(120), 0);
  let peak = 0;
  for (let n = 0; n < Math.ceil(audio.rate * 60 / bpm * 8); n += 128) {
    e.web_render(128);
    for (const sample of new Float32Array(mem.buffer, e.web_sync_l(), 128)) peak = Math.max(peak, Math.abs(sample));
  }
  assert.ok(e.web_marked_triggers() >= 12, 'all demo slices repeat at ' + bpm);
  assert.ok(peak > .1, 'demo produces audio at ' + bpm);
}
assert.ok(!fs.readFileSync(path.join(site, 'emu/slice-ui.js'), 'utf8').includes('/api/rex/'));
const manifest = JSON.parse(fs.readFileSync(path.join(site, 'release-manifest.json'), 'utf8'));
for (const [name, expected] of Object.entries(manifest.files)) {
  assert.equal(createHash('sha256').update(fs.readFileSync(path.join(site, name))).digest('hex'), expected, name);
}
console.log('Published demo: exact parser, six original slices, sync at 90/120/180, manifest checksums and static REX boundary passed.');
