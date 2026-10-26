// SPDX-License-Identifier: GPL-3.0-only
// Production worklet,public MIDI wire plans and independent native UI goldens.
import fs from 'node:fs';import assert from 'node:assert/strict';import path from 'node:path';import {pathToFileURL} from 'node:url';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};globalThis.registerProcessor=(_,p)=>Processor=p;
await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const {ccPacket}=await import(pathToFileURL(path.resolve('web/emu/cc-input-worklet.mjs')).href);
const wasm=fs.readFileSync(process.argv[2]),read=n=>JSON.parse(fs.readFileSync('build/midi-freeze/'+n));let ticket=0,route=0,cases=0;
function render(p,n=20){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,180);p.route=++route;await p.onMessage({type:'cc-input-route',route:p.route,ready:true,request:++ticket});return p;}
function state(p,page=4,index=0){const ptr=p.ex.web_generation_snapshot(2,page,index);assert(ptr,`page${page} index${index}`);const n=new Uint32Array(p.mem.buffer,ptr,1)[0];return [...new Uint32Array(p.mem.buffer,ptr+4,n)];}
async function input(p,events){for(let i=0;i<events.length;i+=8){const packets=events.slice(i,i+8).map(ccPacket),request=++ticket;assert(packets.every(x=>x!==null));await p.onMessage({type:'cc-input-batch',route:p.route,request,packets});const r=p.messages.findLast(x=>x.type==='cc-input-reply'&&x.request===request);assert.equal(r.result,0);assert.equal(r.prefix,packets.length);render(p);assert.equal(p.ex.web_cc_pending(),0);}}
const direct=(p,n,v)=>{const ch=Math.floor(n/88),cc=n%88;return input(p,[[176+ch,cc<32?cc:cc+32,v]]);};
function objects(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=[...new Uint8Array(p.mem.buffer,p.ex.web_store(i),n)];}return o;}
async function save(p){await direct(p,985,0);await direct(p,985,127);return objects(p);}
for(let lane=0;lane<24;lane++){
 const before=read(`before-${lane}.json`),after=read(`after-${lane}.json`),expected=read(`capture-${lane}.json`),p=await open(before);
 assert.equal(state(p,0)[7],63);assert.equal(state(p,0)[9],9000128);
 // Explicit playback cue,holding HOME avoids entering SONG editing.
 p.ex.web_buttons(1<<6);render(p);p.ex.web_keys(1<<2);render(p);p.ex.web_keys(0);render(p);p.ex.web_buttons(0);render(p);
 assert.deepEqual(await save(p),before,'playback setup preserves all stored bytes');
 await input(p,read(`variation-${lane}.json`).events);
 await direct(p,1119,127);render(p,Math.ceil(100000/128));assert.equal(p.ex.web_seq_playing(),1);
 assert.equal(p.ex.web_generation_snapshot(2,6,0),0,'mutable partial captured notes unavailable');
 const freeze=read(`freeze-${lane}.json`);await input(p,freeze.events);assert.equal(state(p)[8],2,'PENDING receipt before pass end');
 const pending=state(p),h0=state(p,5);assert.equal(h0[12],0);assert.equal(h0[5],0);assert.deepEqual(state(p),pending,'readback never completes/mutates receipt');
 render(p,Math.ceil(90000/128));assert.equal(state(p)[6],2);assert.equal(state(p)[8],3,'APPLIED only after complete held pass');
 const header=state(p,5);assert.equal(header[5],expected.capture_id);assert.equal(header[6],lane);assert.equal(header[7],1);assert.equal(header[8],2);assert.equal(header[9],16);assert.equal(header[11],16);assert.equal(header[12],1);
 for(let step=0;step<16;step++){const row=state(p,6,step);assert.equal(row[5],expected.capture_id);assert.equal(row[6],step);assert.deepEqual([row[7],row[11]],expected.steps[step],`actual captured note/flags lane${lane} step${step}`);assert.deepEqual(row.slice(0,5),header.slice(0,5),'coherent read-only capture scan');}
 assert.deepEqual(state(p,5),header);assert.equal(p.ex.web_generation_snapshot(2,6,16),0);
 await input(p,freeze.events.slice(-2));assert.equal(state(p)[8],3,'Freeze repeated request deduplicates');
 await direct(p,1120,127);assert.equal(p.ex.web_seq_playing(),0);assert.deepEqual(await save(p),before,'Freeze is runtime-only');
 const commit=read(`commit-${lane}.json`);await input(p,commit.events.slice(0,-3));const staged=state(p,5);assert.equal(staged[16],6);assert.equal(staged[17],1);assert.equal(staged[19],expected.capture_id);
 assert.deepEqual(objects(p),before,'Commit staging never writes project');await input(p,commit.events.slice(-3));assert.equal(state(p)[6],3);assert.equal(state(p)[8],3);
 assert.deepEqual(await save(p),after,'public Commit equals independent native UI golden,whole saved object set');
 await input(p,commit.events.slice(-2));assert.equal(state(p)[8],3);assert.deepEqual(await save(p),after,'Commit retry deduplicates');
 await direct(p,1117,0);await direct(p,1117,127);assert.deepEqual(await save(p),before,'Commit Undo restores entire stored project');
 await direct(p,1118,0);await direct(p,1118,127);assert.deepEqual(await save(p),after,'Commit Redo restores native golden');
 const rebooted=await open(after);assert.equal(state(rebooted,9)[5],255);assert.equal(state(rebooted,5)[12],0);assert.equal(rebooted.ex.web_generation_snapshot(2,6,0),0);assert.deepEqual(await save(rebooted),after,'committed project survives restart;capture does not');cases++;
}
// Cancel an armed Freeze:identical public fixture,never executes musical change.
const p=await open(read('before-22.json'));p.ex.web_buttons(1<<6);render(p);p.ex.web_keys(1<<2);render(p);p.ex.web_keys(0);render(p);p.ex.web_buttons(0);render(p);await input(p,read('variation-22.json').events);await input(p,read('freeze-22.json').events.slice(0,-3));await input(p,[read('freeze-22.json').events.at(-3)]);await direct(p,1234,127);await input(p,read('freeze-22.json').events.slice(-2));assert.equal(state(p)[8],7);assert.deepEqual(await save(p),read('before-22.json'));
console.log(`FREEZE MIDI WASM PASS: ${cases} lanes;public variation/Freeze/Commit plans,PENDING/full-pass APPLIED,actual captured notes/flags,coherent scans,explicit token/destination,whole-object native parity,Undo/Redo/dedup/Cancel/save/restart.`);
