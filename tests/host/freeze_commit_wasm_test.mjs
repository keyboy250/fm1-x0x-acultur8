// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';
import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,ctor)=>{Processor=ctor;};await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]),dir='build/freeze-commit/';
const read=n=>JSON.parse(fs.readFileSync(dir+n+'.json'));
function drain(p){
 const n=p.ex.web_midi_batch(64);assert.ok(n>=0);
 for(let i=0;i<n;i++){const x=p.ex.web_midi_packet(i)>>>0;if(p.capture&&!(x&0x80000000))p.capture.push({frame:p.frame,st:(x>>>8)&255,note:(x>>>16)&127,vel:(x>>>24)&127});}
 assert.equal(p.ex.web_midi_ack(n),0);
}
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.frame=(p.frame??0)+128;p.process([],[[new Float32Array(128),new Float32Array(128)]]);drain(p);}}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,e,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(e,Math.sign(n));render(p);}}
function chord(p,h,b){p.ex.web_buttons(1<<h);render(p);p.ex.web_buttons((1<<h)|(1<<b));render(p);p.ex.web_buttons(1<<h);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,n){fs.writeFileSync(dir+n+'.rgb565',new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
for(let b=0;b<2;b++){
 const p=new Processor(),baseline=read(`baseline-${b}`);
 await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(baseline).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,175);
 assert.ok(p.messages.some(m=>m.type==='ready'));tap(p,4); // EDIT from HOME
 turn(p,1,b+2);tap(p,8);tap(p,8);tap(p,8);tap(p,8); // VARIATION
 turn(p,3,-1);turn(p,4,6);turn(p,5,-3);turn(p,6,42);
 tap(p,13);tap(p,1);tap(p,8);tap(p,8); // ARM then OPTIONS then CAPTURE
 screen(p,`capture-before-${b}`);assert.equal(p.ex.web_midi_ready(1),0);render(p,10);
 p.capture=[];const start=p.frame;tap(p,10);render(p,800);tap(p,13); // request during second pass
 screen(p,`capture-wait-${b}`);render(p,800);screen(p,`capture-held-${b}`);
 p.capture=[];render(p,2200);const attacks=p.capture.filter(e=>(e.st&240)==144&&e.vel&&(e.st&15)==b).map(e=>[e.note,e.vel]);
 const expected=read(`frozen-${b}`).notes;assert.ok(expected.length>4);assert.ok(attacks.length>expected.length*2);
 assert.ok(expected.some((_,offset)=>attacks.every((x,i)=>JSON.stringify(x)===JSON.stringify(expected[(i+offset)%expected.length]))),'frozen pitches/probability rests/accent repeat exactly as native captured pass');
 tap(p,10);assert.equal(p.ex.web_seq_playing(),0);p.capture=null;
 tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Freeze leaves every saved object unchanged');
 tap(p,12);screen(p,`store-confirm-${b}`);tap(p,6); // cancel
 tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Cancel writes nothing');
 tap(p,12);tap(p,1);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`committed-${b}`),'STORE TO P7 complete saved-object native/WASM parity');
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'button Undo restores destination and source');
 chord(p,6,10);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`committed-${b}`),'Redo restores committed destination');
 p.ex.web_boot();render(p,175);assert.equal(p.ex.web_seq_playing(),0);assert.deepEqual(stored(p),read(`committed-${b}`),'reboot keeps committed project');
}
console.log('FREEZE COMMIT WASM PASS: both303, actualARP/knobs/ARM/FREEZE/P7 STORE/Cancel/Undo/Redo/Save/reboot; native accepted-pitch/probability/accent repeat and whole saved-object parity');
