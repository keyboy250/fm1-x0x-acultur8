// SPDX-License-Identifier: GPL-3.0-only
// The browser build (build/emu/x0x.wasm) against the host simulator: boot, half a second, a kick
// on steps 1 and 9, PLAY, three seconds of it; the audio must match the simulator's WAV to its
// 16-bit rounding, and the screen and the lights must be drawn.
//   node tests/host/emu_test.mjs build/emu/x0x.wasm HOST.wav
import fs from "fs";
import path from "node:path";
import {spawnSync} from "node:child_process";
import assert from "node:assert/strict";
const [wasmPath, wavPath] = process.argv.slice(2);
let mem;
const clock = { clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; } };
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), { wasi_snapshot_preview1: clock });
const ex = instance.exports;
mem = ex.memory;
ex._initialize();
ex.web_boot();
const out = [];
// Feed the independent host the exact input timestamps used by the WASM simulator.
// web_render drains an audio ring, so requested sample durations can finish before
// the nominal millisecond deadline. An extra EDIT press exposes that old assumption.
const timeline=['wav emu_ref_clocked.wav'];let hostTime=0;
function input(command){const now=ex.web_now();assert.ok(now>=hostTime);if(now>hostTime)timeline.push(`wait ${now-hostTime}`);timeline.push(command);hostTime=now+2;}
function buttons(mask,name){input(`${mask?'press':'release'} ${name}`);ex.web_buttons(mask);}
function keys(mask,name){input(`key ${name} ${mask?'down':'up'}`);ex.web_keys(mask);}

const render = (ms) => {
  const n = Math.round(ms * 44.1);
  for (let d = 0; d < n; d += 128) {
    const k = Math.min(128, n - d);
    ex.web_render(k);
    out.push(...new Float32Array(mem.buffer, ex.web_out_l(), k));
  }
};
// as the host script's tapkey (60 ms held, 30 released) and tap (40, 40): kick, steps 1 and 9, PLAY
const tapkey = (k) => { keys(1 << k,String(k)); render(60); keys(0,String(k)); render(30); };
render(500);
// Startup is HOME; explicitly enter the editor before the kick-editing parity fixture.
buttons(1 << 4,"EDIT");render(40);buttons(0,"EDIT");render(40);
tapkey(1);
tapkey(0);
tapkey(14);
buttons(1 << 10,"PLAY");
render(40);
buttons(0,"PLAY");
render(3000);
const end=ex.web_now();if(end>hostTime)timeline.push(`wait ${end-hostTime}`);timeline.push('wavstop');
const dir=path.dirname(wavPath),script=path.join(dir,'emu_ref_clocked.x0x');fs.writeFileSync(script,timeline.join('\n')+'\n');
const replay=spawnSync(path.join(dir,'x0x_host'),[script,dir],{encoding:'utf8'});assert.equal(replay.status,0,replay.stdout+replay.stderr);
const wav = fs.readFileSync(path.join(dir,'emu_ref_clocked.wav'));
const host = new Int16Array(wav.buffer, wav.byteOffset + 44, (wav.length - 44) >> 1).filter((_, i) => i % 2 === 0);
const n = Math.min(host.length, out.length);
let diff = 0, peak = 0;
for (let i = 0; i < n; i++) { diff = Math.max(diff, Math.abs(host[i] / 32767 - out[i])); peak = Math.max(peak, Math.abs(out[i])); }
const fb = new Uint16Array(mem.buffer, ex.web_fb(), 240 * 240);
const drawn = fb.some((p) => p !== 0), lit = ex.web_lit_keys() !== 0;
console.log(`emu: ${n} samples, peak ${peak.toFixed(3)}, max diff from the host ${diff.toExponential(2)}, screen ${drawn}, lights ${lit}`);
process.exit(n > 160000 && peak > 0.05 && diff < 1e-4 && drawn && lit ? 0 : 1);
