// SPDX-License-Identifier: GPL-3.0-only
// Exercise the real firmware menu through the production worklet and WASM.
import fs from 'node:fs';
import assert from 'node:assert/strict';
let Processor;
globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,ctor)=>{Processor=ctor;};
await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]);
const read=name=>JSON.parse(fs.readFileSync(`build/drum-generation/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(bass,mode){
 const p=new Processor();const store=Object.fromEntries(Object.entries(read(`before-${bass}-${mode}`)).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/drum-generation/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}

const black=[1,3,5,8,10,13,15,17,20,22,25];
function key(p,k){p.ex.web_keys(1<<k);render(p);p.ex.web_keys(0);render(p);}
function prompt(p,kit,voice){
 turn(p,1,kit);turn(p,2,1); // kit and P2
 tap(p,8);key(p,black[voice]);turn(p,5,1); // ARP, black-key lane, K3 BEAT anchor
 screen(p,`page-${kit}-${voice}`);tap(p,12);screen(p,`confirm-${kit}-${voice}`);
}
for(let kit=0;kit<2;kit++)for(let voice=0;voice<11;voice++){
 const p=await open(kit,voice);prompt(p,kit,voice);tap(p,1);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`after-${kit}-${voice}`),`native saved-object parity ${kit}/${voice}`);
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`before-${kit}-${voice}`),'Undo restores complete store');
 chord(p,6,10);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${kit}-${voice}`),'Redo exact');
 if(kit==0&&voice==1){tap(p,8);tap(p,12);tap(p,1);screen(p,"same-rhythm");tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${kit}-${voice}`),'same-seed no-op exact');}
 p.ex.web_boot();render(p,175);assert.deepEqual(stored(p),read(`after-${kit}-${voice}`),'saved restart exact');
}
const p=await open(0,1);const before=stored(p);prompt(p,0,1);tap(p,6);tap(p,7);render(p,100);assert.deepEqual(stored(p),before,'Cancel unchanged');
console.log('DRUM GENERATION WASM PASS: all22 lanes; actualARP/black-key/K3/OCT-/SEL buttons, complete native-object parity,Undo/Redo,Cancel and saved restart');
