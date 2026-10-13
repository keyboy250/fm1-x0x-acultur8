// SPDX-License-Identifier: GPL-3.0-only
// Compare identical browser builds with original/raw and packed font renderers.
// node tests/host/font_wasm_parity.mjs RAW.wasm RLE.wasm [REPORT.json]
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { createHash } from 'node:crypto';

const [rawPath, packedPath, reportPath] = process.argv.slice(2);
assert(rawPath && packedPath, 'Provide raw and packed WASM builds');
async function boot(path) {
  let memory;
  const wasi = { clock_time_get: (_id, _p, out) => {
    new DataView(memory.buffer).setBigUint64(out, 0n, true);
    return 0;
  } };
  const { instance } = await WebAssembly.instantiate(fs.readFileSync(path), { wasi_snapshot_preview1: wasi });
  const ex = instance.exports;
  memory = ex.memory;
  ex._initialize();
  ex.web_boot();
  return ex;
}
const pair = [await boot(rawPath), await boot(packedPath)];
const hashes = { audio: createHash('sha256'), screens: createHash('sha256') };
let frames = 0, chunks = 0, peak = 0, drawn = false;
function bytes(ex, start, length) { return Buffer.from(ex.memory.buffer, start, length); }
function render(ms) {
  const count = Math.round(ms * 44.1);
  for (let pos = 0; pos < count; pos += 128) {
    const n = Math.min(128, count - pos);
    for (const ex of pair) ex.web_render(n);
    const [a, b] = pair;
    for (const channel of ['web_out_l', 'web_out_r']) {
      const x = bytes(a, a[channel](), n * 4), y = bytes(b, b[channel](), n * 4);
      assert(x.equals(y), `Audio mismatch at frame ${frames}, ${channel}`);
      hashes.audio.update(x);
      for (const v of new Float32Array(a.memory.buffer, a[channel](), n)) peak = Math.max(peak, Math.abs(v));
    }
    const screen = bytes(a, a.web_fb(), 240 * 240 * 2);
    assert(screen.equals(bytes(b, b.web_fb(), screen.length)), `Screen mismatch at frame ${frames}`);
    hashes.screens.update(screen);
    drawn ||= screen.some(v => v !== 0);
    for (const field of ['web_lit_buttons', 'web_lit_keys', 'web_dim_keys', 'web_now', 'web_blits']) {
      assert.equal(a[field](), b[field](), `${field} at frame ${frames}`);
    }
    frames += n;
    chunks++;
  }
}
function invoke(name, value) { for (const ex of pair) ex[name](value); }
function tapKey(key) { invoke('web_keys', 1 << key); render(60); invoke('web_keys', 0); render(30); }
function tapButton(button) { invoke('web_buttons', 1 << button); render(40); invoke('web_buttons', 0); render(150); }
render(500);
tapKey(1); tapKey(0); tapKey(14); // kick at steps 1 and 9
tapButton(10); render(2200); // PLAY
for (const button of [0, 2, 3, 4, 5, 6, 8, 9, 6]) tapButton(button);
render(600);
assert(drawn && peak > 0.05 && frames > 200000, 'Exercise drawing and audible playback');
const report = { status: 'pass', frames, chunks, stereo_audio: 'bit-identical',
  framebuffer: 'pixel-identical after every render chunk', lights_and_clock: 'identical', peak,
  audio_sha256: hashes.audio.digest('hex'), framebuffer_stream_sha256: hashes.screens.digest('hex') };
if (reportPath) fs.writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify(report, null, 2));
