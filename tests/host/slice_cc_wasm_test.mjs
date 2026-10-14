// SPDX-License-Identifier: GPL-3.0-only
// Production slice service/worklet + actual HOST WASM. Browser storage is
// qualified by slice_browser_test.html; this test makes no physical MIDI claim.
import fs from 'node:fs';import assert from 'node:assert/strict';import crypto from 'node:crypto';import path from 'node:path';import {pathToFileURL} from 'node:url';
import {SliceService} from '../../web/emu/slice-control.mjs';
const wasm=fs.readFileSync(process.argv[2]);let Processor,service;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>{this.messages.push(m);service?.onMessage(m);}};}};
globalThis.registerProcessor=(_,c)=>Processor=c;await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const p=new Processor();const render=(n=1)=>{let out;for(let i=0;i<n;i++){out=[[new Float32Array(128),new Float32Array(128)]];currentTime+=128/44100;p.process([],out);}return out[0][0];};
await p.onMessage({type:'load',wasm});render(180);
assert.equal(p.ex.web_slice_cc_version?.(),0x10001,'new HOST slice ABI required; missing/old WASM fails');
service=new SliceService({send:m=>p.onMessage(m)});service.powered=true;service.state=p.sliceControl.state();
const frames=4410,candidate={name:'Synthetic constant original',audio:{frames,rate:44100,channels:1,pcm:new Float32Array(frames).fill(.2)},markers:{length:frames,slices:[{start:0,end:frames,time:0,label:'One'}],bpm:120,loopDuration:.1}};
await service.replace(candidate);assert.equal(service.state.count,1);await service.perform(1,1);await service.perform(0,20);await service.perform(2,0);await service.perform(5,1);
const signal=render();assert.ok(signal.some(v=>v>.039&&v<.041),'real worklet gain yields original .2 × .2');
const cursor=p.ex.web_marked_cursor(),triggers=p.ex.web_marked_triggers();await service.perform(1,0);assert.equal(p.ex.web_marked_cursor(),cursor);assert.equal(p.ex.web_marked_triggers(),triggers);render(36);assert.equal(p.ex.web_marked_playing(),0,'repeat release reaches existing loop boundary');
await service.perform(4,0);assert.equal(p.ex.web_marked_triggers(),triggers+1);assert.ok(render().some(v=>v>.039));await service.perform(6,1);assert.equal(p.ex.web_marked_playing(),0);
const old=service.imported;p.ex.web_seq_play(1);await assert.rejects(service.perform(9,1),/transport-pending/);render();await assert.rejects(service.perform(9,1),/transport-playing/);assert.equal(service.imported,old);p.ex.web_seq_play(0);render(5);
// Native context/arm survives exact browser consumption; all three chunks are
// fed through actual source2 ingress, not injected JavaScript source state.
await p.onMessage({type:'cc-input-route',request:1,route:7,ready:true});
const inv=JSON.parse(fs.readFileSync('docs/ACULTUR8_CC_INVENTORY.json','utf8')),rows=new Map(inv.entries.map(r=>[r.parameter_number,r]));
const packet=(num,v)=>{const r=rows.get(num),[lo,hi]=r.range;const raw=Array.from({length:128},(_,i)=>i).find(i=>Math.round(lo+i*(hi-lo)/127)===v);assert.notEqual(raw,undefined);return(11|((0xaf+r.address.channel)<<8)|(r.address.cc<<16)|(raw<<24))>>>0;};
const exact=220123,pk=[packet(1010,7),packet(1011,0),packet(1132,19),packet(1071,1),packet(1133,2),packet(1072,1107&127),packet(1073,1107>>7),packet(1074,exact&127),packet(1075,(exact>>7)&127),packet(1130,exact>>14)];
await p.onMessage({type:'cc-input-batch',request:2,route:7,packets:pk});render(35);assert.equal(p.ex.web_cc_pending(),0);const before=Array.from({length:32},(_,i)=>p.ex.web_cc_source(2,i));
assert.equal((await service.applyPacket(0,{route:7,parameter:1107,exact:{parameter:1107,value:exact}})).status,'applied');assert.equal(service.state.bpmMilli,exact);
const after=Array.from({length:32},(_,i)=>p.ex.web_cc_source(2,i));assert.equal(after[14],0);before[14]=0;assert.deepEqual(after,before,'browser exact consumes only assembly validity, retains arm/selectors/native edges');
await service.perform(7,1);render(900);assert.equal(p.ex.web_sync_state(),2,'actual C starts slices at musical bar');assert.ok(p.ex.web_marked_triggers()>triggers+1);await service.perform(6,1);assert.equal(p.ex.web_sync_state(),0);p.ex.web_seq_play(0);render(5);
const bad=structuredClone(candidate);bad.audio.pcm[0]=NaN;await assert.rejects(service.replace(bad),/invalid-audio/);assert.equal(service.imported,old);
await p.onMessage({type:'variant-command',abi:1,request:30,op:3,target:0,voice:0,index:0,value:0});await assert.rejects(service.perform(0,50),/owned-gesture/);await service.perform(6,1);await p.onMessage({type:'variant-command',abi:1,request:31,op:4,target:0,voice:0,index:0,value:0});
console.log(JSON.stringify({test:'actual HOST WASM slice control',sha256:crypto.createHash('sha256').update(wasm).digest('hex'),gainPCM:true,repeatBoundary:true,audition:true,realSyncBar:true,exact21ContextPreserved:true,pendingPlayingOwnerNoReplace:true,browserIndexedDB:false,physicalMidi:false}));
