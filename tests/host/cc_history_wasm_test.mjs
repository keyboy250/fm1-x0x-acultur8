// SPDX-License-Identifier: GPL-3.0-only
// Actual production worklet + compiled C. Synthetic ingress is not physical MIDI proof.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
assert.ok(process.argv[2],'Usage: node tests/host/cc_history_wasm_test.mjs build/emu/x0x.wasm');
const wasm=fs.readFileSync(process.argv[2]);
const inventory=JSON.parse(fs.readFileSync('docs/ACULTUR8_CC_INVENTORY.json','utf8'));
const rows=new Map(inventory.entries.map(r=>[r.id,r]));
for(const [id,n,cc] of [['editor.undo',1117,93],['editor.redo',1118,94]]) {
  assert.equal(rows.get(id).parameter_number,n);assert.deepEqual(rows.get(id).address,{channel:13,cc});
}
const packet=(cc,raw)=>(11|(0xbc<<8)|(cc<<16)|(raw<<24))>>>0;
function control(id,value){
  const row=rows.get(id),[lo,hi]=row.range;
  const raw=Array.from({length:128},(_,i)=>i).find(i=>lo+Math.floor((i*(hi-lo)+63)/127)===value);
  assert.notEqual(raw,undefined,`${id} ${value} is CC7 representable`);
  return (11|((0xaf+row.address.channel)<<8)|(row.address.cc<<16)|(raw<<24))>>>0;
}
let Processor,serial=0,route=0;
globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(name,C)=>{assert.equal(name,'x0x');Processor=C;};
await import('../../web/emu/worklet.js');
const p=new Processor(),out=[[new Float32Array(128),new Float32Array(128)]];
function render(n=35){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],out);}}
function snap(section=0,target=0,voice=0){const ptr=p.ex.web_variant_snapshot(section,target,voice);assert.ok(ptr);const a=new Uint32Array(p.mem.buffer,ptr,20);return [...a.slice(4,4+a[3])];}
const hp=()=>snap(2)[0],lane=()=>snap(10,7,22).slice(1,3),stat=()=>Array.from({length:6},(_,i)=>p.ex.web_cc_stat(i)>>>0);
async function send(m){const request=++serial;await p.onMessage({...m,request});const r=p.messages.at(-1);assert.equal(r.request,request);return r;}
async function enable(){route++;assert.equal((await send({type:'cc-input-route',route,ready:true})).result,0);}
async function input(packets,{drain=true}={}){
  const r=await send({type:'cc-input-batch',route,packets});assert.equal(r.result,0);assert.equal(r.prefix,packets.length,'receipt is ingress acceptance, not semantic application');
  if(drain){render();assert.equal(p.ex.web_cc_pending(),0);assert.equal(snap()[3],snap()[4]);}return r;
}
async function cmd(op,{type='variant-command',namespace=0,target=0,voice=0,index=0,value=0}={}){
  const request=((namespace<<30)|(++serial))>>>0;
  await p.onMessage({type,abi:1,op,request,target,voice,index,value});const r=p.messages.at(-1);assert.equal(r.request,request);return r.result;
}
async function fresh(){await p.onMessage({type:'load',wasm});render();assert.equal(p.ex.web_cc_version(),0x10001);await enable();}
async function edit(value){assert.equal(await cmd(3),0);assert.equal(await cmd(2,{value}),1);render();assert.equal(await cmd(4),0);}

await fresh();const original=hp();await edit(81);await edit(100);
await input([packet(93,63)]);assert.equal(hp(),100);
await input([packet(93,64)]);assert.equal(hp(),81,'positive boolean edge must perform actual native Undo');
let prior=stat();await input([packet(93,127)]);assert.equal(hp(),81);assert.equal(stat()[1],prior[1]);
await input([packet(93,0),packet(93,127)]);assert.equal(hp(),original);
await input([packet(94,127)]);assert.equal(hp(),81);
await input([packet(94,127)]);assert.equal(hp(),81);
await input([packet(94,0),packet(94,127)]);assert.equal(hp(),100);
// Reset rearms edges, but never rolls back edits or reuses a retired route.
route++;assert.equal((await send({type:'cc-input-route',route,ready:false})).result,0);
assert.equal((await send({type:'cc-input-batch',route:route-1,packets:[packet(93,127)]})).result,-3);assert.equal(hp(),100);
await enable();await input([packet(93,127)]);assert.equal(hp(),81);

// Every legacy browser namespace owns the same C gesture. CC history must
// reject without END, without consuming its edge, and without invalidating Redo.
for(let namespace=0;namespace<4;namespace++){
  await fresh();await edit(81);await edit(100);await input([packet(93,127),packet(93,0)]);assert.equal(hp(),81);
  assert.equal(await cmd(3,{namespace}),0);assert.equal(await cmd(2,{namespace,value:120}),1);render();
  prior=stat();await input([packet(93,127),packet(94,127)]);
  assert.equal(stat()[1],prior[1]+2);assert.equal(hp(),120);assert.equal(p.editOwner,`variant-${namespace}`);
  assert.equal(await cmd(4,{namespace}),0);await input([packet(93,127)]);assert.equal(hp(),81,'refused edge remains retryable after actual END');
  await input([packet(94,127)]);assert.equal(hp(),120,'newly committed owned edit is the next Redo unit');
}

// Lane owner + ordinary CC is intentionally one shared journal group. The
// history CC is refused while owned; subsequent Undo/Redo restores both edits.
await fresh();await input([control('lane.mode',1)]);assert.equal(await cmd(7),0);render();
const initialLane=lane(),initialBoost=snap(2)[2];
assert.equal(await cmd(3,{type:'lane-command'}),0);
assert.equal(await cmd(18,{type:'lane-command',target:7,voice:22,value:7}),1);render();
await input([control('strip.0.boost',7)]);
assert.equal(snap(2)[2],7);assert.deepEqual(lane(),[7,initialLane[1]]);
prior=stat();await input([packet(93,127),packet(94,127)]);assert.equal(stat()[1],prior[1]+2);assert.equal(p.editOwner,'lane');
assert.equal(await cmd(4,{type:'lane-command'}),0);
await input([packet(93,127)]);assert.equal(snap(2)[2],initialBoost);assert.deepEqual(lane(),initialLane);
await input([packet(94,127)]);assert.equal(snap(2)[2],7);assert.deepEqual(lane(),[7,initialLane[1]]);

// Terminal history refusal must not stall a following Stop in the same real
// ingress batch; a pending transport request also rejects before mutation.
await fresh();await edit(81);p.ex.web_seq_play(1);render();assert.equal(p.ex.web_seq_playing(),1);
prior=stat();await input([packet(93,127),control('transport.stop',1)]);
assert.equal(hp(),81);assert.equal(stat()[1],prior[1]+1);assert.equal(p.ex.web_seq_playing(),0);
await input([packet(93,127)]);assert.equal(hp(),original);
await edit(100);await input([packet(93,0)]);p.ex.web_seq_play(1);
prior=stat();await input([packet(93,127),control('transport.stop',0),control('transport.stop',1)]);
assert.equal(hp(),100);assert.equal(stat()[1],prior[1]+1);assert.equal(p.ex.web_seq_playing(),0);
await input([packet(93,127)]);assert.equal(hp(),original);

// The real record flag is set by native ingress, not a fake worklet property.
await fresh();await edit(81);await input([control('transport.record',1)]);
prior=stat();await input([packet(93,127),packet(94,127)]);assert.equal(stat()[1],prior[1]+2);assert.equal(hp(),81);
assert.equal(p.editOwner,null);

// Failed engine replacement retains the owned gesture and edited project,
// resets the CC source, and requires a fresh route. History still cannot END it.
await fresh();await edit(81);assert.equal(await cmd(3),0);assert.equal(await cmd(2,{value:100}),1);render();
const old=p.ex;await assert.rejects(p.onMessage({type:'load',wasm:new Uint8Array([0])}),WebAssembly.CompileError);
assert.equal(p.ex,old);assert.equal(p.editOwner,'variant-0');assert.equal(p.ccInput.ready,false);assert.equal(hp(),100);
await enable();prior=stat();await input([packet(93,127)]);assert.equal(stat()[1],prior[1]+1);assert.equal(p.editOwner,'variant-0');
assert.equal(await cmd(4),0);await input([packet(93,127)]);assert.equal(hp(),81);
await fresh();assert.equal(p.editOwner,null);await edit(100);await input([packet(93,127)]);assert.equal(hp(),original);

console.log(JSON.stringify({test:'CC Undo/Redo through production worklet + actual WASM',wasmBytes:wasm.length,
  wasmSha256:crypto.createHash('sha256').update(wasm).digest('hex'),source:2,edgeAndSourceReset:true,
  allFourLegacyOwners:true,laneOwnerAndSharedJournal:true,terminalRejectionAllowsStop:true,pendingStart:true,
  recordingRefusal:true,failedLoadRetainsOwner:true,successLoadResetsOwner:true,
  globalSettingsHistory:false,physicalMidi:false}));
