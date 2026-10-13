// SPDX-License-Identifier: GPL-3.0-only
// Every production UI-triggered sound, actual worklet/C engine, independent pre-all25 WASM.
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
assert.ok(process.argv[2]&&process.argv[3],"Current and independent pinned baseline WASM are mandatory");
const current=fs.readFileSync(process.argv[2]),baseline=fs.readFileSync(process.argv[3]);let request=0;
const snap=(p,section,target=0)=>{const ptr=p.ex.web_variant_snapshot(section,target,0);if(!ptr)return null;const v=new Uint32Array(p.mem.buffer,ptr,20);return Array.from(v.subarray(4,4+v[3]));};
const render=(p,n=1)=>{const samples=new Float32Array(n*256);for(let i=0;i<n;i++){currentTime+=128/44100;const out=[[new Float32Array(128),new Float32Array(128)]];p.process([],out);for(let k=0;k<128;k++){samples[256*i+2*k]=out[0][0][k];samples[256*i+2*k+1]=out[0][1][k];}}return samples;};
const cmd=async(p,op,target=0,index=0,value=0)=>{await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice:0,index,value});const r=p.messages.at(-1);assert.equal(r.type,'variant-reply');assert.equal(r.result,op<=2?1:0);return r;};
const open=async(bytes=current,store)=>{const p=new Processor();await p.onMessage({type:'load',wasm:bytes,store});render(p,175);return p;};
const black=[1,3,5,8,10,13,15,17,20,22,25];
const select=async(p,strip)=>{const part=strip<22?Math.floor(strip/11):strip-20;for(let i=0;i<part;i++){await p.onMessage({type:'enc',role:1,n:1});render(p,14);}await p.onMessage({type:'buttons',mask:1<<4});render(p,14);await p.onMessage({type:'buttons',mask:0});render(p,14);if(strip===22||strip===23){for(let i=0;i<2;i++){await p.onMessage({type:'buttons',mask:1<<9});render(p,14);await p.onMessage({type:'buttons',mask:0});render(p,14);}await p.onMessage({type:'enc',role:3,n:1});render(p,14);}};
const configure=async(p,strip,flags)=>{for(const[index,value]of [[2,24],[3,48],[5,flags]])await cmd(p,2,strip,index,value);render(p,40);};
const hit=async(p,strip)=>{const key=strip<22?black[strip%11]:strip<24?0:1;await p.onMessage({type:'keys',mask:1<<key});let peak=0,outPeak=0,gain=1;const chunks=[];for(let i=0;i<124;i++){if(i===24)await p.onMessage({type:'keys',mask:0});chunks.push(render(p));const m=snap(p,3,strip);if(m){peak=Math.max(peak,m[0]);outPeak=Math.max(outPeak,m[1]);gain=Math.min(gain,m[2]);}}return{audio:Float32Array.from(chunks.flatMap(c=>Array.from(c))),peak,outPeak,gain};};
const capture=async(strip,flags,bytes=current)=>{const p=await open(bytes);await select(p,strip);if(flags!==undefined)await configure(p,strip,flags);else render(p,40);return hit(p,strip);};
const equal=(a,b,msg)=>assert.ok(Buffer.from(a.buffer).equals(Buffer.from(b.buffer)),msg);
const energy=a=>a.reduce((s,x)=>s+x*x,0);
for(let strip=0;strip<25;strip++){
 const original=await capture(strip,undefined,baseline),dry=await capture(strip,0),active=await capture(strip,9);
 equal(original.audio,dry.audio,`strip ${strip} independent default/disabled bypass PCM`);
 assert.ok(energy(dry.audio)>1e-8,`strip ${strip} actually sounds`);
 assert.ok(active.peak>1,`strip ${strip} active input meter`);
 assert.ok(active.outPeak<=3982 && active.gain<1000000,`strip ${strip} prepan limiter bound/meter`);
 assert.ok(energy(active.audio)<energy(dry.audio)*.5,`strip ${strip} audible limiting`);
}
// Every meter is exposed only after the complete audio job; original schema/caps persist.
const p=await open();assert.equal(snap(p,0)[7],0x01ffffff);assert.equal(snap(p,0)[5],2);
for(let i=0;i<25;i++)assert.deepEqual(snap(p,3,i),[0,0,1000000]);
await cmd(p,3);for(const strip of [11,22,24])await configure(p,strip,9);await cmd(p,4);await cmd(p,7);
const objects=Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:new Uint8Array(p.mem.buffer,p.ex.web_store(i),n).slice()];}).filter(([,v])=>v));
const restored=await open(current,objects);for(const strip of [11,22,24])assert.equal(snap(restored,2,strip)[5],9);
await cmd(p,5);render(p,4);for(const strip of [11,22,24])assert.equal(snap(p,2,strip)[5],0);
await cmd(p,6);render(p,4);for(const strip of [11,22,24])assert.equal(snap(p,2,strip)[5],9);
// Undo/reopen are checked through audio, not just matching stored configuration fields.
for(const strip of [11,22,24]) {
 const q=await open();await select(q,strip);await cmd(q,3);await configure(q,strip,9);await cmd(q,4);await cmd(q,7);
 const saved=Object.fromEntries(Array.from({length:q.ex.web_nobj()},(_,i)=>{const n=q.ex.web_store_len(i);return[i,n<0?null:new Uint8Array(q.mem.buffer,q.ex.web_store(i),n).slice()];}).filter(([,v])=>v));
 const limited=await hit(q,strip);render(q,200);await cmd(q,5);render(q,40);const dry=await hit(q,strip);
 assert.ok(energy(limited.audio)<energy(dry.audio)*.5,`strip ${strip} undo audibly removes limiting`);
 render(q,200);await cmd(q,6);render(q,40);const redone=await hit(q,strip);
 assert.ok(energy(redone.audio)<energy(dry.audio)*.5,`strip ${strip} redo audibly reapplies limiting`);
 const reopened=await open(current,saved);await select(reopened,strip);render(reopened,40);const restoredAudio=await hit(reopened,strip);
 assert.ok(energy(restoredAudio.audio)<energy(dry.audio)*.5 && restoredAudio.outPeak<=3982,`strip ${strip} reopened processing remains audible and bounded`);
}
console.log(JSON.stringify({realWorkletWasm:true,sounds:25,configuredBypassBitwise:true,independentBypassBitwise:true,allAudibleLimiters:true,allMeters:true,savedConfigs:true,undoRedo:true,audibleUndoRedoReopen:true,processingMask:0x01ffffff}));
