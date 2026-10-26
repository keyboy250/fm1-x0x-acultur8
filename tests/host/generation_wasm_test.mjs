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
const read=name=>JSON.parse(fs.readFileSync(`build/generation/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(bass,mode){
 const p=new Processor();const store=Object.fromEntries(Object.entries(read(`before-${bass}-${mode}`)).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));tap(p,4);return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/generation/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
function prompt(p,bass,mode){
 turn(p,1,bass+2);turn(p,2,1); // selected303 and P2
 tap(p,8);tap(p,8);tap(p,8); // ARP pages: GENERATE,SCALE,CONSTRAINED
 turn(p,3,mode); // K1 note constraint, defaultGEN
 screen(p,`page-${bass}-${mode}`);tap(p,12);screen(p,`confirm-${bass}-${mode}`); // OCT- repeat saved seed
}
for(let bass=0;bass<2;bass++)for(let mode=0;mode<5;mode++){
 const p=await open(bass,mode);prompt(p,bass,mode);tap(p,1);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`after-${bass}-${mode}`),`native saved-object parity ${bass}/${mode}`);
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`before-${bass}-${mode}`),'Undo restores complete store');
 chord(p,6,10);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${bass}-${mode}`),'Redo exact');
 p.ex.web_boot();render(p,175);assert.deepEqual(stored(p),read(`after-${bass}-${mode}`),'saved restart exact');
}
const p=await open(0,2);const before=stored(p);prompt(p,0,2);tap(p,6);tap(p,7);render(p,100);assert.deepEqual(stored(p),before,'Cancel unchanged');
console.log('CONSTRAINED GENERATION WASM PASS: both303s/all5modes; actualARP/K1/OCT-/SEL buttons, complete native-object parity,Undo/Redo,Cancel and saved restart');
