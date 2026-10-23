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
const read=name=>JSON.parse(fs.readFileSync(`build/copy-review/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(scope){
 const p=new Processor();
 const store=Object.fromEntries(Object.entries(read(`before-${scope}`)).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);
 assert.ok(p.messages.some(m=>m.type==='ready'));return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/copy-review/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
function prompt(p,scope){
 tap(p,5);screen(p,`wasm-global-${scope}`); // GLO: top-level submenu above CLOCK OUT
 tap(p,1); // enter COPY/PASTE SEQUENCES
 turn(p,0,read('menu-moves').sourceMoves);
 turn(p,1,1); // COPY FROM P2
 turn(p,0,1);turn(p,1,1); // PASTE INTO P3
 turn(p,0,1);turn(p,1,scope); // scope
 turn(p,0,1);screen(p,`wasm-menu-${scope}`);
 tap(p,1);screen(p,`wasm-confirm-${scope}`); // SEL -> confirmation
}
for(let scope=0;scope<25;scope++){
 const p=await open(scope);prompt(p,scope);tap(p,1); // confirm
 tap(p,7);render(p,100); // SAVE, using the real button handler
 assert.deepEqual(stored(p),read(`after-${scope}`),`scope ${scope}: complete saved object parity with independently exercised native transaction`);
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`before-${scope}`),`scope ${scope}: physical-button Undo restores original project objects`);
 chord(p,6,10);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`after-${scope}`),`scope ${scope}: physical-button Redo restores pasted project objects`);
 // Reboot uses those actual objects, rather than a test-only mutation export.
 p.ex.web_boot();render(p,175);assert.deepEqual(stored(p),read(`after-${scope}`));
}
const cancelled=await open(1);const original=stored(cancelled);prompt(cancelled,1);tap(cancelled,6);tap(cancelled,7);render(cancelled,100);
assert.deepEqual(stored(cancelled),original,'Cancel must leave all saved objects unchanged');
console.log('PATTERN COPY WASM PASS: nested GLO submenu and 25 scopes via select/algorithm/SEL, native saved-object parity, button Undo/Redo, reboot and Cancel');
