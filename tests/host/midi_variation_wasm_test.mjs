// SPDX-License-Identifier: GPL-3.0-only
// Public wire plans through production worklet;independent native accepted-event traces.
import fs from 'node:fs';import assert from 'node:assert/strict';import {pathToFileURL} from 'node:url';import path from 'node:path';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};globalThis.registerProcessor=(_,ctor)=>Processor=ctor;
await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const {ccPacket}=await import(pathToFileURL(path.resolve('web/emu/cc-input-worklet.mjs')).href);
const wasm=fs.readFileSync(process.argv[2]),read=n=>JSON.parse(fs.readFileSync('build/midi-variation/'+n));let ticket=0,route=0,cases=0;
function drain(p){if(!p.collect)return;const n=p.ex.web_midi_batch(64);for(let i=0;i<n;i++){
 const packet=p.ex.web_midi_packet(i)>>>0,st=(packet>>>8)&255,note=(packet>>>16)&127,vel=(packet>>>24)&127;
 if((packet&0x80000000)||st!==(0x90|p.oracle.channel)||!vel||(p.lane<22&&note!==p.oracle.note))continue;
 const ptr=p.ex.web_runtime_snapshot(p.lane);assert(ptr);const d=new Uint32Array(p.mem.buffer,ptr,52),cy=d[32],step=d[31];
 if(cy<8&&step<16){p.trace[cy][step].push(p.lane<22?vel:note);}
 }p.ex.web_midi_ack(n);}
function render(p,n=20){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);drain(p);}}
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,180);p.route=++route;await p.onMessage({type:'cc-input-route',route:p.route,ready:true,request:++ticket});return p;}
function state(p,page=4){const ptr=p.ex.web_generation_snapshot(2,page,0);assert(ptr,'page'+page);const n=new Uint32Array(p.mem.buffer,ptr,1)[0];return [...new Uint32Array(p.mem.buffer,ptr+4,n)];}
async function input(p,events){for(let i=0;i<events.length;i+=8){const packets=events.slice(i,i+8).map(ccPacket),request=++ticket;assert(packets.every(x=>x!==null));await p.onMessage({type:'cc-input-batch',route:p.route,request,packets});const r=p.messages.findLast(x=>x.type==='cc-input-reply'&&x.request===request);assert.equal(r.result,0);assert.equal(r.prefix,packets.length);render(p);assert.equal(p.ex.web_cc_pending(),0);}}
const direct=(p,n,v)=>{const ch=Math.floor(n/88),cc=n%88;return input(p,[[176+ch,cc<32?cc:cc+32,v]]);};
function objects(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=[...new Uint8Array(p.mem.buffer,p.ex.web_store(i),n)];}return o;}
async function save(p){await direct(p,985,0);await direct(p,985,127);return objects(p);}
for(let lane=0;lane<24;lane++){
 const before=read(`state-before-${lane}.json`),on=read(`plan-${lane}-on.json`),off=read(`plan-${lane}-off.json`),p=await open(before);
 assert.equal(state(p,0)[7],63);assert.equal(state(p,0)[9],9000128);
 await input(p,on.events.slice(0,-3));assert.deepEqual(objects(p),before);assert.equal(state(p,8)[8],42);assert.equal(state(p,8)[14],1);
 await input(p,on.events.slice(-3));assert.equal(state(p)[6],1);assert.equal(state(p)[8],3);assert.equal(state(p,9)[5],lane);assert.equal(state(p,9)[6],1);assert.equal(state(p,9)[17],1);
 assert.deepEqual(await save(p),before,'variation saves no project changes');await input(p,on.events.slice(-2));assert.equal(state(p)[8],3);
 await input(p,off.events);assert.equal(state(p)[6],2);assert.equal(state(p,9)[5],255);assert.deepEqual(await save(p),before,'OFF leaves all saved objects unchanged');cases++;
}
for(const lane of [0,11,21,22,23]){
 const before=read(`before-${lane}.json`),p=await open(before);p.lane=lane;p.oracle=read(`trace-${lane}.json`);p.trace=Array.from({length:8},()=>Array.from({length:16},()=>[]));
 // HOME P2 chooses playback explicitly;MIDI target remains explicit and independent.
 p.ex.web_buttons(1<<6);render(p);p.ex.web_keys(1<<2);render(p);p.ex.web_keys(0);render(p);p.ex.web_buttons(0);render(p);
 assert.deepEqual(await save(p),before,'explicit HOME cue changes no stored project bytes');
 await input(p,read(`plan-${lane}-on.json`).events);assert.equal(state(p,9)[5],lane);p.ex.web_midi_ready(1);
 p.collect=true;await direct(p,1119,127);render(p,Math.ceil(88200*8/128)+10);await direct(p,1120,127);p.collect=false;
 assert.equal(p.ex.web_seq_playing(),0);assert.deepEqual(p.trace,p.oracle.trace,`actual accepted MIDI trace/cadence equals native8-cycle oracle lane${lane}`);assert.deepEqual(await save(p),before,'playback preserves all saved bytes');cases++;
}
const p=await open(read('state-before-22.json')),on=read('plan-22-on.json');await input(p,on.events.slice(0,-3));await input(p,[on.events.at(-3)]);await direct(p,1234,127);await input(p,on.events.slice(-2));assert.equal(state(p)[8],7);assert.equal(state(p,9)[5],255);
console.log(`VARIATION MIDI WASM PASS: ${cases} cases;all24 explicit lanes,public wire plans,staged/live/result readback,Arm/OFF/dedup/Cancel,unchanged saved objects;5 independent actual8-cycle MIDI/cadence/pitch traces.`);
