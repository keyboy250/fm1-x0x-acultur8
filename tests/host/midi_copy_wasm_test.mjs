// SPDX-License-Identifier: GPL-3.0-only
// Public wire plans through production worklet;compare full native UI stores.
import fs from 'node:fs';import assert from 'node:assert/strict';import path from 'node:path';import {pathToFileURL} from 'node:url';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};globalThis.registerProcessor=(_,p)=>Processor=p;
await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const {ccPacket}=await import(pathToFileURL(path.resolve('web/emu/cc-input-worklet.mjs')).href);
const wasm=fs.readFileSync(process.argv[2]),read=n=>JSON.parse(fs.readFileSync('build/midi-copy/'+n));let ticket=0,route=0,cases=0;
function render(p,n=20){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,180);p.route=++route;await p.onMessage({type:'cc-input-route',route:p.route,ready:true,request:++ticket});await direct(p,1071,127);return p;}
function state(p,page=4){const ptr=p.ex.web_generation_snapshot(2,page,0);assert(ptr,`page${page}`);const n=new Uint32Array(p.mem.buffer,ptr,1)[0];return [...new Uint32Array(p.mem.buffer,ptr+4,n)];}
async function input(p,events){for(let i=0;i<events.length;i+=8){const packets=events.slice(i,i+8).map(ccPacket),request=++ticket;assert(packets.every(x=>x!==null));await p.onMessage({type:'cc-input-batch',route:p.route,request,packets});const r=p.messages.findLast(x=>x.type==='cc-input-reply'&&x.request===request);assert.equal(r.result,0);assert.equal(r.prefix,packets.length);render(p);assert.equal(p.ex.web_cc_pending(),0);}}
const direct=(p,n,v)=>{const ch=Math.floor(n/88),cc=n%88;return input(p,[[176+ch,cc<32?cc:cc+32,v]]);};
function objects(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=[...new Uint8Array(p.mem.buffer,p.ex.web_store(i),n)];}return o;}
async function save(p){await direct(p,985,0);await direct(p,985,127);return objects(p);}
for(let from=0;from<25;from++)for(let to=0;to<25;to++){
 if((from===0?0:from<23?1:2)!==(to===0?0:to<23?1:2))continue;
 const before=read(`before-${from}-${to}.json`),after=read(`after-${from}-${to}.json`),plan=read(`plan-${from}-${to}.json`),p=await open(before);
 assert.equal(state(p,0)[7],63);assert.equal(state(p,0)[9],9000128);
 await input(p,plan.events.slice(0,plan.preview_event_count));assert.equal(state(p)[6],1);assert.equal(state(p)[8],1);
 const preview=state(p,10);assert.deepEqual(preview.slice(5,11),[1,2,from,to,15,1]);assert.deepEqual(objects(p),before,'preview never saves');assert.deepEqual(state(p,10),preview,'readback has no side effects');
 await input(p,plan.events.slice(plan.preview_event_count));assert.equal(state(p)[8],3);assert.equal(state(p,10)[10],0);
 assert.deepEqual(await save(p),after,`complete native UI golden ${from}->${to}`);
 await input(p,plan.events.slice(-2));assert.equal(state(p)[8],3);assert.deepEqual(await save(p),after,'dedup preserves all saved objects');
 await direct(p,1117,0);await direct(p,1117,127);assert.deepEqual(await save(p),before,'Undo full byte recall');
 await direct(p,1118,0);await direct(p,1118,127);assert.deepEqual(await save(p),after,'Redo full byte recall');
 if(from===0||from===23||from===24){const rebooted=await open(after);assert.deepEqual(await save(rebooted),after,'restart byte recall');}
 cases++;
}
const before=read('before-2-4.json'),p=await open(before),plan=read('plan-2-4.json');await input(p,plan.events.slice(0,7));await direct(p,1234,127);await input(p,plan.events.slice(-2));assert.equal(state(p)[8],7);assert.deepEqual(await save(p),before);
console.log(`COPY MIDI WASM PASS: ${cases} compatible routes;public preview/execute plans,receipt and page10,whole-store native parity,dedup,Cancel,Undo/Redo/save/restart.`);
