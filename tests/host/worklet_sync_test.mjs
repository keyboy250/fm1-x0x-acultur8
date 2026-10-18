// SPDX-License-Identifier: GPL-3.0-only
// Exercise actual worklet messages and PCM mixing against the built WASM.
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(name,ctor)=>{Processor=ctor;};
await import('../../web/emu/worklet.js');
const p=new Processor();await p.onMessage({type:'load',wasm:fs.readFileSync(process.argv[2])});
assert.ok(p.messages.some(m=>m.type==='ready'));
const render=(frames)=>{const n=128,outputs=[[new Float32Array(n),new Float32Array(n)]];for(let i=0;i<frames;i++){currentTime+=n/44100;p.process([],outputs);}return outputs[0];};
const tap=async(k)=>{await p.onMessage({type:'keys',mask:1<<k});render(22);await p.onMessage({type:'keys',mask:0});render(11);};
render(175);await tap(1);await tap(0);await tap(14);
const span=1024,count=8,pcm=new Float32Array(span*count).fill(.15);
await p.onMessage({type:'marked-load',pcm,frames:pcm.length,rate:44100,channels:1,slices:Array.from({length:count},(_,i)=>({start:i*span,end:(i+1)*span,time:i*11025})),length:88200});
await p.onMessage({type:'marked-level',gain:.5,layer:true});await p.onMessage({type:'marked-sync',bpm:120});
let nativePeak=0;
for(let k=0;k<20;k++){
 const out=render(1),base=new Float32Array(p.mem.buffer,p.ex.web_out_l(),128),slice=new Float32Array(p.mem.buffer,p.ex.web_sync_l(),128);
 for(let i=0;i<128;i++){nativePeak=Math.max(nativePeak,Math.abs(base[i]));assert.ok(Math.abs(out[0][i]-Math.max(-1,Math.min(1,base[i]+slice[i]*.5)))<1e-7);}
}
assert.ok(nativePeak>.01,'FM1 pattern is audible in the synced mix');
await p.onMessage({type:'marked-level',gain:.5,layer:false});
const solo=render(1),slice=new Float32Array(p.mem.buffer,p.ex.web_sync_l(),128);
for(let i=0;i<128;i++)assert.ok(Math.abs(solo[0][i]-slice[i]*.5)<1e-7);
await p.onMessage({type:'seq-tempo',bpm:180});assert.equal(p.ex.web_seq_bpm(),180);
await p.onMessage({type:'marked-stop'});render(2);assert.equal(p.ex.web_sync_state(),0);assert.equal(p.syncOn,false);assert.equal(p.ex.web_seq_playing(),1);
await p.onMessage({type:'marked-play',repeat:false});render(2);assert.equal(p.ex.web_marked_playing(),1);assert.equal(p.syncOn,false);
await p.onMessage({type:'marked-pad',slice:2});render(2);await p.onMessage({type:'marked-stop'});
console.log(JSON.stringify({workletMessages:true,nativePatternPeak:nativePeak,layerMix:true,soloMix:true,liveTempo:true,stopSoloPad:true}));
