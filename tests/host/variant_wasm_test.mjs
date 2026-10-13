// SPDX-License-Identifier: GPL-3.0-only
// Real shared C application through WASM and the production AudioWorklet transport.
import fs from 'node:fs';
import assert from 'node:assert/strict';
let Processor;
globalThis.currentTime = 0;
globalThis.AudioWorkletProcessor = class {
  constructor() { this.messages = []; this.port = {postMessage:m => this.messages.push(m)}; }
};
globalThis.registerProcessor = (_, ctor) => { Processor = ctor; };
await import('../../web/emu/worklet.js');
const wasm = fs.readFileSync(process.argv[2]);
const open = async store => {
  const p = new Processor();
  await p.onMessage({type:'load', wasm, store});
  assert.ok(p.messages.some(m => m.type === 'ready'));
  return p;
};
const render = (p, blocks = 1) => {
  for (let i = 0; i < blocks; i++) {
    currentTime += 128 / 44100;
    p.process([], [[new Float32Array(128), new Float32Array(128)]]);
  }
};
const snap = (p, section = 0, target = 0, voice = 0) => {
  const ptr = p.ex.web_variant_snapshot(section,target,voice);
  if (!ptr) return null;
  const v = new Uint32Array(p.mem.buffer,ptr,20);
  assert.ok(v[3] <= 16);
  return {abi:v[0], section:v[1], revision:v[2], values:Array.from(v.subarray(4,4+v[3]))};
};
let request = 0;
const command = async (p, op, target = 0, index = 0, value = 0, extra = {}) => {
  await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice:0,index,value,...extra});
  const reply = p.messages.at(-1);
  assert.equal(reply.type,'variant-reply');
  assert.equal(reply.request,extra.request ?? request);
  return reply;
};
const objects = p => Object.fromEntries(Array.from({length:p.ex.web_nobj()}, (_,i) => {
  const n = p.ex.web_store_len(i);
  return [i,n < 0 ? null : new Uint8Array(p.mem.buffer,p.ex.web_store(i),n).slice()];
}).filter(([,v]) => v !== null));
const p = await open(); render(p,30);
const info = snap(p);
assert.equal(info.abi,1);
assert.equal(info.values[0],127,'kick processing capability advertised separately');
assert.equal(info.values[7],1,'only the first 909 kick processes audio');
assert.equal(info.values[5],2); assert.equal(info.values[6],25);
assert.deepEqual(snap(p,2).values,[20,20000,0,1,80,0]);
assert.equal(snap(p,1,0,0).values.length > 0,true);
for (const args of [[1,-1,0],[1,0,255],[2,25,0],[2,0,1],[99,0,0]]) assert.equal(snap(p,...args),null);
// Invalid fields must be rejected before integer narrowing, without queue/revision/dirty mutation.
const before = snap(p);
for (const extra of [{abi:2},{op:99},{target:25},{voice:1},{index:6},{value:25},
  {target:256},{target:-256},{voice:256},{index:256},{abi:65537},{op:65538},
  {value:2**32},{value:1.5},{request:-1},{request:2**32}]) {
  assert.ok((await command(p,2,0,2,4,extra)).result < 0);
  assert.deepEqual(snap(p),before);
}
// Direct export callers also cannot wrap transport fields into valid commands.
for (const [abi,op,t,v,i] of [[65537,2,0,0,2],[1,65538,0,0,2],[1,2,-256,0,2],
  [1,2,256,0,2],[1,2,0,256,2],[1,2,0,0,258]]) {
  const ptr = p.ex.web_variant_command(abi,op,0xffffffff,t,v,i,8);
  const r = new DataView(p.mem.buffer,ptr,16);
  assert.equal(r.getUint32(0,true),0xffffffff); assert.equal(r.getInt32(12,true),-1);
  assert.equal(r.getUint32(4,true),before.revision);
  assert.equal(r.getUint32(8,true),before.values[3]);
  assert.deepEqual(snap(p),before);
}
assert.equal((await command(p,2,0,2,8,{abi:2})).result,-2);
// Fill the actual bounded C queue without rendering; BUSY does not alter desired state.
let accepted = 0, last;
for (let i = 0; i < 4096; i++) {
  const prior = snap(p); const strip = snap(p,2);
  const r = await command(p,2,0,2,i%25);
  if (r.result === -3) {
    assert.deepEqual(snap(p),prior); assert.deepEqual(snap(p,2),strip); break;
  }
  assert.equal(r.result,1); assert.ok(r.ticket > prior.values[3]); last = r; accepted++;
}
assert.ok(accepted > 0 && accepted < 4096);
const fullInfo = snap(p), fullSound = snap(p,1,0,0);
assert.equal((await command(p,1,0,0,0)).result,-3);
assert.deepEqual(snap(p),fullInfo); assert.deepEqual(snap(p,1,0,0),fullSound);
assert.ok(snap(p).values[4] < last.ticket,'accepted is not yet applied');
render(p,4); assert.equal(snap(p).values[4],last.ticket);
// Drain/group multiple edits with pauses into one undo step; save ends open gestures.
assert.equal((await command(p,7)).result,0); render(p,4);
const initial = snap(p,2).values[2];
assert.equal((await command(p,3)).result,0);
assert.equal((await command(p,3)).result,0);
const gestureWrites = p.ex.web_store_writes();
assert.equal((await command(p,2,0,2,7)).result,1); render(p,900);
assert.equal((await command(p,2,0,2,11)).result,1); render(p,900);
assert.equal(p.ex.web_store_writes(),gestureWrites,'open gesture defers autosave through idle pauses');
assert.equal((await command(p,4)).result,0);
assert.equal((await command(p,5)).result,0); render(p,4);
assert.equal(snap(p,2).values[2],initial);
assert.equal((await command(p,6)).result,0); render(p,4);
assert.equal(snap(p,2).values[2],11);
assert.equal((await command(p,7)).result,0);
assert.equal(snap(p).values[1],0);
const saved = objects(p);
// A sound pot edit uses the same acknowledged queue and persists through reopen.
const soundBefore = snap(p,1,0,0).values[0];
const soundValue = soundBefore === 0 ? 1 : 0;
assert.equal((await command(p,1,0,0,soundValue)).result,1);
render(p,4); assert.equal(snap(p,1,0,0).values[0],soundValue);
assert.equal((await command(p,7)).result,0);
const soundReopened = await open(objects(p)); render(soundReopened,30);
assert.equal(snap(soundReopened,1,0,0).values[0],soundValue);
const reopened = await open(saved); render(reopened,30);
assert.equal(snap(reopened,2).values[2],11);
assert.equal(snap(reopened).values[2],0);
// SAVE while playing or the asynchronous stop request is active must not write.
reopened.ex.web_seq_play(1); render(reopened,4);
assert.equal(reopened.ex.web_seq_playing(),1);
const writes = reopened.ex.web_store_writes();
assert.equal((await command(reopened,7)).result,-3);
reopened.ex.web_seq_play(0);
assert.equal((await command(reopened,7)).result,-3);
assert.equal(reopened.ex.web_store_writes(),writes);
render(reopened,4); assert.equal((await command(reopened,7)).result,0);
await reopened.onMessage({type:'variant-select',strip:24}); reopened.publish();
let frame = reopened.messages.at(-1);
assert.equal(frame.variant.strip,24); assert.equal(frame.variant.settings.section,2);
await reopened.onMessage({type:'variant-select',strip:256}); reopened.publish();
assert.equal(reopened.messages.at(-1).variant.strip,24);
// Corrupt an actual packed format header; reopen protects stored data from editor/save writes.
const corrupt = objects(reopened);
const packed = Object.values(corrupt).find(b => b.length >= 16 && new DataView(b.buffer,b.byteOffset,b.byteLength).getUint32(0,true) === 4);
assert.ok(packed); packed[8] ^= 1;
const rejected = await open(corrupt); render(rejected,30);
assert.equal(snap(rejected).values[2],1);
const rejectedWrites = rejected.ex.web_store_writes();
assert.equal((await command(rejected,2,0,2,3)).result,-4);
assert.equal((await command(rejected,7)).result,-4);
render(rejected,1900); assert.equal(rejected.ex.web_store_writes(),rejectedWrites);
assert.deepEqual(objects(rejected),corrupt);
console.log(JSON.stringify({realWasm:true,transportBounds:true,acceptedQueueCommands:accepted,
  appliedTickets:true,gestureUndoRedo:true,saveReopen:true,playingStopBusy:true,corruptProtection:true,capabilities:127}));
