// SPDX-License-Identifier: GPL-3.0-only
// Deterministic production controller/service tests; C is a bounded test double.
// Actual C and actual browser IndexedDB qualification run separately.
import assert from 'node:assert/strict';
import {SliceService,syntheticImport,sliceScale} from '../../web/emu/slice-control.mjs';
import {SliceWorkletControl,validateMarked} from '../../web/emu/slice-worklet.mjs';
let gate=0,playing=0,repeat=1,clock=0,loadError=0,exact=null,pads=[],writes=0,stored=null,failStorage=false;
const mem={buffer:new ArrayBuffer(16000000)},owner={mem,loading:false,editOwner:null,markedGain:.5,markedLayer:false,ccInput:{ready:true,route:7}};
owner.ex={web_slice_cc_version:()=>0x10001,web_slice_control_state:()=>gate,web_marked_playing:()=>playing,
 web_marked_pcm:()=>0,web_marked_meta:()=>12000000,web_marked_load:()=>{writes++;return loadError;},web_marked_repeat:v=>{repeat=v;return 0;},
 web_sync_off:()=>clock=0,web_marked_stop:()=>playing=0,web_marked_play:()=>playing=1,web_marked_pad:i=>{pads.push(i);return 0;},
 web_seq_bpm:()=>120,web_sync_arm:b=>{clock=b;return 0;},web_cc_take_exact:(p,v)=>{if(exact?.parameter!==p||exact.value!==v)return -2;exact=null;return 0;}};
const databaseOpen=async()=>({close(){},transaction(_,mode){const tx={objectStore:()=>({put(value){const q={};queueMicrotask(()=>{if(failStorage){tx.onerror();return;}stored=structuredClone(value);q.result='current';q.onsuccess?.();tx.oncomplete();});return q;},get(){const q={};queueMicrotask(()=>{q.result=structuredClone(stored);q.onsuccess?.();tx.oncomplete();});return q;}}),abort(){tx.onabort?.();}};return tx;}});
let service;const control=new SliceWorkletControl(owner,m=>service.onMessage(m));
service=new SliceService({send:m=>control.message(m),databaseOpen,download:(json,cc)=>{assert.ok(JSON.parse(json).slices.length);return !cc;}});
service.powered=true;service.state=control.state();
await service.perform(9,1);assert.equal(service.imported.markers.slices.length,6);const original=service.imported;
for(let index=0;index<4;index++)await service.perform(index,[37,0,1,123456][index]);
assert.equal(owner.markedGain,.37);assert.equal(repeat,0);assert.equal(owner.markedLayer,true);assert.equal(service.imported.markers.bpm,123.456);
await service.perform(4,5);assert.deepEqual(pads,[5]);await assert.rejects(service.perform(4,6),/invalid-value/);
await service.perform(5,1);assert.equal(playing,1);await service.perform(6,1);assert.equal(playing,0);
await service.perform(7,1);assert.equal(clock,123.456);await service.perform(6,1);
await service.perform(10,1);const saved=structuredClone(stored);await service.perform(9,1);await service.perform(11,1);assert.equal(service.imported.markers.bpm,123.456);
assert.equal((await service.perform(12,1,{cc:true})).reason,'json-prepared-download-not-requested');
assert.equal((await service.perform(12,1)).downloadDelivered,false);
const old=service.imported;failStorage=true;await assert.rejects(service.perform(10,1),/storage-failed/);assert.deepEqual(stored,saved);failStorage=false;
service.files=()=>[];await assert.rejects(service.perform(8,1),/select-matching-files-first/);assert.equal(service.imported,old);
for(const bits of[1,2,4,8,16,32]){gate=bits;const before=writes;await assert.rejects(service.perform(9,1));assert.equal(writes,before);assert.equal(service.imported,old);}gate=0;
owner.editOwner='lane-0';await assert.rejects(service.perform(0,40),/owned-gesture/);await service.perform(6,1);owner.editOwner=null;
const cc=(i,v)=>((v<<24)|((80+i)<<16)|(0xbc<<8)|0x0b)>>>0;
assert.equal(sliceScale(5,63),0);assert.equal(sliceScale(5,64),1);
assert.equal((await service.applyPacket(cc(5,127),{route:7})).status,'applied');assert.equal((await service.applyPacket(cc(5,64),{route:7})).reason,'held-edge');
assert.equal((await service.applyPacket(cc(5,0),{route:7})).reason,'zero-rearm');assert.equal((await service.applyPacket(cc(5,127),{route:7})).status,'applied');await service.perform(6,1);
exact={parameter:1107,value:220123};assert.equal((await service.applyPacket(cc(3,127),{route:7,exact,parameter:1107})).status,'applied');assert.equal(exact,null);assert.equal(control.bpm,220.123);
const candidate=syntheticImport();candidate.audio.pcm[10]=NaN;assert.throws(()=>validateMarked({...candidate.audio,...candidate.markers}),/invalid-audio/);assert.equal(service.imported,old);
owner.ex.web_slice_cc_version=()=>0;service.state=control.state();await assert.rejects(service.perform(9,1),/engine-slice-abi-unavailable/);assert.equal(service.imported,old);
console.log('PASS production slice service/controller:13 paths, exact, guard, recovery, action rearm; C/IndexedDB doubles only');
