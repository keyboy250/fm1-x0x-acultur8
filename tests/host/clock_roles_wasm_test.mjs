// SPDX-License-Identifier: GPL-3.0-only
// Actual production worklet/C protocol; synthetic input, no physical delivery.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {CLOCK_PROTOCOL as protocol} from '../../web/emu/clock-worklet.mjs';
const wasm=fs.readFileSync(process.argv[2]);
let Processor,request=0;
globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class {constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,C)=>{Processor=C;};
await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const p=new Processor();
const out=[[new Float32Array(128),new Float32Array(128)]];
function render(n=8){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],out);}}
await p.onMessage({type:'load',wasm});render(180);
const snapshot=()=>p.performanceClock.snapshot();
async function send(type,fields={}){
 const id=++request;await p.onMessage({type,protocol,request:id,...fields});
 const reply=p.messages.findLast(m=>m.type==='clock-reply'&&m.request===id);
 assert.ok(reply);return reply;
}
async function mode(role,source){const r=await send('clock-command',{role,source,policy:0});assert.equal(r.result,0);render();const s=snapshot();assert.equal(s.generation,r.acceptedGeneration);assert.equal(s.role,role);assert.equal(s.source,source);assert.equal(p.ex.web_seq_playing(),0);return s;}
const packet=st=>(15|(st<<8))>>>0;
async function input(generation,...statuses){const r=await send('clock-input',{generation,source:2,packets:statuses.map(packet)});return r;}
let s=await mode(3,2);
assert.equal((await input(s.generation,250,248)).result,0);render();
assert.equal(p.ex.web_seq_playing(),1);assert.equal(snapshot().tickLo,s.tickLo+1);
assert.equal((await input(s.generation,252)).result,0);render();assert.equal(p.ex.web_seq_playing(),0);
assert.equal((await input(s.generation,251,248)).result,0);render();assert.equal(p.ex.web_seq_playing(),1);
const oldGeneration=s.generation;
const retired=await send('clock-source-retire',{source:2});assert.equal(retired.result,0);
render();assert.equal(snapshot().generation,retired.acceptedGeneration);assert.equal(p.ex.web_seq_playing(),0);
assert.equal((await input(oldGeneration,250,248)).result,-1);render();assert.equal(p.ex.web_seq_playing(),0);
s=snapshot();assert.equal((await input(s.generation,248)).result,0);render();assert.equal(p.ex.web_seq_playing(),0);
assert.equal((await input(s.generation,250,248)).result,0);render();assert.equal(p.ex.web_seq_playing(),1);
render(200);assert.equal(p.ex.web_seq_playing(),0);assert.ok(snapshot().generation>s.generation);
s=snapshot();assert.equal((await input(s.generation,248)).result,0);render();assert.equal(p.ex.web_seq_playing(),0);
assert.equal((await input(s.generation,251,248)).result,0);render();assert.equal(p.ex.web_seq_playing(),1);
await mode(2,0);p.ex.web_seq_play(1);render();assert.equal(p.ex.web_seq_playing(),1);
p.ex.web_seq_play(0);render();assert.equal(p.ex.web_seq_playing(),0);
await mode(1,0);p.ex.web_seq_play(1);render();assert.equal(p.ex.web_seq_playing(),1);
p.ex.web_seq_play(0);render();assert.equal(p.ex.web_seq_playing(),0);
console.log(JSON.stringify({test:'clock roles actual WASM and production worklet',wasmBytes:wasm.length,wasmSHA256:crypto.createHash('sha256').update(wasm).digest('hex'),follow:true,startStopContinue:true,sourceRetirement:true,staleGenerationRefused:true,ticksAloneDoNotRestart:true,lossFreezes:true,explicitRestart:true,leadStandalone:true,physicalMidi:false,irqInterleavings:'separate native fixture'}));
