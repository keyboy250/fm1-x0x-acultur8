// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';let Processor;
globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,ctor)=>Processor=ctor;await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]),dir='build/random-step-options/';
const read=n=>JSON.parse(fs.readFileSync(dir+n+'.json'));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,e,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(e,Math.sign(n));render(p);}}
function chord(p,h,b){p.ex.web_buttons(1<<h);render(p);p.ex.web_buttons((1<<h)|(1<<b));render(p);p.ex.web_buttons(1<<h);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
for(let b=0;b<2;b++)for(let single=0;single<2;single++){
 const p=new Processor(),baseline=read(`before-${b}-${single}`);
 await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(baseline).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));
 tap(p,4);turn(p,1,b+2);turn(p,2,1);tap(p,8);tap(p,8);tap(p,8);
 turn(p,single?6:5,single?7:32);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'selectors alone never alter stored pattern');
 fs.writeFileSync(dir+`options-${b}-${single}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));
 tap(p,12);fs.writeFileSync(dir+`confirm-${b}-${single}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));tap(p,6);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Cancel leaves every object unchanged');
 tap(p,12);tap(p,1);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${b}-${single}`),'actual STEP7/32STEPS button commit matches native whole object fixture');
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Undo restores base');
 chord(p,6,10);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${b}-${single}`),'Redo exact');
 p.ex.web_boot();render(p,175);assert.deepEqual(stored(p),read(`after-${b}-${single}`),'generated result persists; selectors transient');
}
console.log('RANDOM OPTIONS WASM PASS: both303,actual32STEPS/STEP7 controls,selectors no write,Cancel,full native saved-object parity,Undo/Redo/save/reboot');
