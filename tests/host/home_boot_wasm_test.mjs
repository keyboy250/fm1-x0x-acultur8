// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';
import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,ctor)=>{Processor=ctor;};await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]),saved=JSON.parse(fs.readFileSync('build/home-boot/saved.json'));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function title(p){const a=new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2),rows=[];for(let y=0;y<24;y++)rows.push(...a.slice(y*480,y*480+120));return Buffer.from(rows);}
const expected=fs.readFileSync('build/home-boot/home-title.rgb565');
for(const store of [{},saved]){
 const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,175);
 assert.ok(p.messages.some(m=>m.type==='ready'));assert.deepEqual(title(p),expected,'real initial render says HOME');assert.equal(p.ex.web_seq_playing(),0);
 const baseline=stored(p);tap(p,4);assert.notDeepEqual(title(p),expected,'EDIT opens editor');tap(p,6);assert.deepEqual(title(p),expected,'HOME returns to overview');
 p.ex.web_boot();render(p,175);assert.deepEqual(title(p),expected,'reboot opens HOME');assert.equal(p.ex.web_seq_playing(),0);assert.deepEqual(stored(p),baseline,'boot/navigation does not save or alter objects');
 fs.writeFileSync('build/home-boot/home-wasm.rgb565',new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));
}
console.log('HOME BOOT WASM PASS: fresh and saved project startup/reboot render HOME; native title pixel parity, EDIT/HOME navigation, stopped transport and unchanged stored objects');
