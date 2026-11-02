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
const read=name=>JSON.parse(fs.readFileSync(`build/cross-copy/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(from,to){
 const p=new Processor();const store=Object.fromEntries(Object.entries(read(`before-${from}-${to}`)).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/cross-copy/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
function prompt(p,from,to){
 tap(p,5);tap(p,1);turn(p,1,1); // sourceP2
 turn(p,0,1);turn(p,1,1); // destinationP3
 turn(p,0,1);turn(p,1,from); // sourcepart defaults destination to samepart
 turn(p,0,1);turn(p,1,to-from); // destinationpart relative to source
 turn(p,0,1);screen(p,`menu-${from}-${to}`);tap(p,1);screen(p,`confirm-${from}-${to}`);
}
for(const [from,to] of [[2,4],[2,6],[12,1],[23,24],[24,23]]){
 const p=await open(from,to);prompt(p,from,to);tap(p,1);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`after-${from}-${to}`),`crossroute${from}->${to}: native fullobject parity`);
 tap(p,6);tap(p,6);chord(p,6,11);tap(p,7);render(p,100);
 assert.deepEqual(stored(p),read(`before-${from}-${to}`),'buttonUndo restores allobjects');
 chord(p,6,10);tap(p,7);render(p,100);assert.deepEqual(stored(p),read(`after-${from}-${to}`),'buttonRedo exact');
 p.ex.web_boot();render(p,175);assert.deepEqual(stored(p),read(`after-${from}-${to}`),'savedreboot exact');
}
const p=await open(2,4);const original=stored(p);prompt(p,2,4);tap(p,6);tap(p,7);render(p,100);assert.deepEqual(stored(p),original,'Cancel unchanged');
console.log('CROSS PART COPY WASM PASS: snare->tom/rim,808->909,303A/B both ways via actualmenu/buttons; nativeobjectparity,Undo/Redo,reboot,Cancel');
