// SPDX-License-Identifier: GPL-3.0-only
// Production AudioWorklet and real application/clock/storage; no sequencer mock.
import fs from 'node:fs'; import assert from 'node:assert/strict';
let Processor; globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class {constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};
await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]);let request=0;
const snap=(p,section,target=0)=>{const ptr=p.ex.web_variant_snapshot(section,target,0);if(!ptr)return null;
  const v=new Uint32Array(p.mem.buffer,ptr,20);return {revision:v[2],values:Array.from(v.subarray(4,4+v[3]))};};
const render=(p,blocks=1)=>{for(let i=0;i<blocks;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}};
const open=async(store)=>{const p=new Processor();await p.onMessage({type:'load',wasm,store});render(p,175);return p;};
const command=async(p,op,target=0,voice=0,index=0,value=0)=>{
  await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice,index,value});
  const r=p.messages.at(-1);assert.equal(r.type,'variant-reply');assert.equal(r.request,request);return r;};
const good=async(...args)=>{const r=await command(...args);assert.equal(r.result,args[1]===11||args[1]===7||args[1]===3||args[1]===4||args[1]===5||args[1]===6?0:1);return r;};
const block=async(p,index,degree,duration=1,patterns=[0,0,0,0,0],mutes=0)=>
  good(p,8,index,(duration-1)|(degree<<3),mutes,patterns.reduce((v,n,i)=>v|(n<<(i*5)),0));
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{
  const n=p.ex.web_store_len(i);return [i,n<0?null:new Uint8Array(p.mem.buffer,p.ex.web_store(i),n).slice()];}).filter(([,v])=>v));
const play=async(p)=>{await p.onMessage({type:'buttons',mask:1<<10});render(p,14);await p.onMessage({type:'buttons',mask:0});render(p,14);};
const p=await open();
assert.equal(snap(p,0).values[0],1535);assert.equal(snap(p,0).values[5],5);
assert.deepEqual(snap(p,5).values.slice(3,6),[255,255,255]);
// Fixture produced by the native test using the independent schema-1 raw length/header.
const fixture=fs.readFileSync(process.argv[3]??'build/host/arrangement_schema1_store.bin');
const storeMax=p.ex.web_store_max(), oldStore={};
assert.equal(fixture.length,p.ex.web_nobj()*(storeMax+4));
for(let o=0,offset=0;o<p.ex.web_nobj();o++,offset+=storeMax+4) {
  const length=fixture.readInt32LE(offset);
  if(length>=0)oldStore[o]=new Uint8Array(fixture.subarray(offset+4,offset+4+length));
}
const migrated=await open(oldStore);
assert.equal(snap(migrated,0).values[2],0);
assert.equal(snap(migrated,0).values[5],5);
assert.equal(snap(migrated,5).values[0],0);
assert.equal(snap(migrated,4,2).values[7],0);
assert.equal(snap(migrated,4,2).values[6],8,'legacy repeat bits survive migration');
assert.equal(snap(migrated,2,24).values[2],9,'nondefault schema1 strip config migrates unchanged');
await good(p,10,0,0,0,192);
for(const [i,d] of [1,6,4,5].entries())await block(p,i,d);
await block(p,191,0,8,[31,30,29,28,27],31);
await good(p,9,0,0,0,1);render(p,6);
const selected=await good(p,11,191);p.publish();
const frame=p.messages.at(-1);
assert.equal(frame.variant.arrangement.values[2],191);
assert.deepEqual(frame.variant.block.values,[31,30,29,28,27,31,8,0]);
assert.equal(frame.variant.block.revision,selected.revision);
const before=snap(p,5),savedBefore=objects(p);
for(const args of [[8,192,0,0,0],[8,0,64,0,0],[8,0,0,32,0],[8,0,0,0,2**25],
  [9,0,0,1,12],[9,0,0,2,9],[10,0,0,0,193],[11,192,0,0,0],[12,192,0,0,0]]) {
  assert.equal((await command(p,...args)).result,-1);assert.deepEqual(snap(p,5),before);assert.deepEqual(objects(p),savedBefore);
}
await good(p,10,0,0,0,4);render(p,4);
await good(p,7);const saved=objects(p);
const first=Object.values(saved).find(b=>b.length>=16&&new DataView(b.buffer,b.byteOffset).getUint32(0,true)===4);
assert.ok(first,'nondefault arrangement uses format4');
const reopened=await open(saved);assert.deepEqual(snap(reopened,4,191).values,[31,30,29,28,27,31,8,0]);
assert.deepEqual(snap(reopened,4,1).values,[0,0,0,0,0,0,1,6]);
await play(reopened);assert.equal(snap(reopened,5).values[3],0);
assert.deepEqual(snap(reopened,6).values.slice(0,4),[1,0,4,7]);
await good(reopened,11,191);assert.equal(snap(reopened,5).values[3],0,'editing does not queue/play another row');
assert.equal((await command(reopened,10,0,0,0,4)).result,-3);
assert.equal((await command(reopened,9,0,0,0,0)).result,-3);
// Playing intent remains immutable; a desired degree edit takes effect on its next occurrence.
await block(reopened,0,0,8);assert.equal(snap(reopened,5).values[11],1);
assert.equal(snap(reopened,5).values[15],1);
await good(reopened,9,0,0,1,2);
assert.equal(snap(reopened,5).values[8],2);assert.equal(snap(reopened,5).values[12],0);
await good(reopened,9,0,0,1,0);
await good(reopened,12,3);render(reopened,2);assert.equal(snap(reopened,5).values[4],3);
let quanta=0;
while(snap(reopened,5).values[3]===0 && quanta++<800)render(reopened);
assert.ok(quanta<800);assert.equal(snap(reopened,5).values[3],3);
assert.equal(snap(reopened,5).values[4],255);
assert.deepEqual(snap(reopened,6).values.slice(0,4),[1,7,11,2],'queued degree V is resolved in C');
quanta=0;while(snap(reopened,5).values[3]===3&&quanta++<800)render(reopened);
assert.equal(snap(reopened,5).values[3],0);
assert.equal(snap(reopened,5).values[11],0);assert.equal(snap(reopened,5).values[15],8);
assert.deepEqual(snap(reopened,6).values,[0,0,0,0,0,0,0]);
// Stopped undo/redo includes both the packed block record and degree extension.
await play(reopened);assert.equal(reopened.ex.web_seq_playing(),0);
await good(reopened,7);const degreeBefore=snap(reopened,4,2).values[7];
await good(reopened,3);await block(reopened,2,7);await good(reopened,4);render(reopened,4);
await good(reopened,5);render(reopened,4);assert.equal(snap(reopened,4,2).values[7],degreeBefore);
await good(reopened,6);render(reopened,4);assert.equal(snap(reopened,4,2).values[7],7);
// Saturate the real queue; BUSY must leave seven canonical bytes/revision untouched.
render(reopened,4);let accepted=0;
for(let i=0;i<2048;i++) {
  const prior=snap(reopened,4,191),info=snap(reopened,0);
  const r=await command(reopened,8,191,(i%8)|((i%8)<<3),i%32,i%32);
  if(r.result===-3){assert.deepEqual(snap(reopened,4,191),prior);assert.deepEqual(snap(reopened,0),info);break;}
  assert.equal(r.result,1);accepted++;
}
assert.equal(accepted,1024);render(reopened,4);
// Corruption preserves the entire stored image and locks autosave/editor mutations.
const corrupt=structuredClone(saved);const header=Object.values(corrupt).find(b=>b.length>=16&&new DataView(b.buffer,b.byteOffset).getUint32(0,true)===4);header[8]^=1;
const protectedProject=await open(corrupt);assert.equal(snap(protectedProject,0).values[2],1);
assert.equal((await command(protectedProject,8,0,0,0,0)).result,-4);
assert.equal((await command(protectedProject,7)).result,-4);render(protectedProject,1800);
assert.deepEqual(objects(protectedProject),corrupt);
// Regression: disabled-playing REDO must not restore enabled arrangement via a journal swap.
const hazard=await open();
await good(hazard,3);await good(hazard,9,0,0,0,1);await good(hazard,4);render(hazard,4);
await good(hazard,5);render(hazard,4);assert.equal(snap(hazard,5).values[0],0);
await play(hazard);assert.equal(hazard.ex.web_seq_playing(),1);
const hazardRevision=snap(hazard,0).revision, hazardBlock=snap(hazard,4), hazardStore=objects(hazard);
assert.equal((await command(hazard,6)).result,-3);
assert.equal(snap(hazard,0).revision,hazardRevision);assert.deepEqual(snap(hazard,4),hazardBlock);
assert.deepEqual(objects(hazard),hazardStore);
// Physical HOME+PLAY takes the same preflight and keeps the redo entry intact.
await hazard.onMessage({type:'buttons',mask:1<<6});render(hazard,14);
await hazard.onMessage({type:'buttons',mask:(1<<6)|(1<<10)});render(hazard,14);
await hazard.onMessage({type:'buttons',mask:0});render(hazard,14);
assert.equal(snap(hazard,5).values[0],0);assert.equal(snap(hazard,0).revision,hazardRevision);
assert.equal(hazard.ex.web_seq_playing(),1);
await play(hazard);assert.equal(hazard.ex.web_seq_playing(),0);
await good(hazard,6);render(hazard,4);assert.equal(snap(hazard,5).values[0],1,'rejected live redo retained the exact entry');
// The start request itself is busy, before an audio quantum makes playing=true.
const pendingStart=await open();await good(pendingStart,3);
const pendingInfo=snap(pendingStart,0), pendingArr=snap(pendingStart,5),pendingStore=objects(pendingStart);
const pendingWrites=pendingStart.ex.web_store_writes();
pendingStart.ex.web_seq_play(1);assert.equal(pendingStart.ex.web_seq_playing(),0);
for(const args of [[7],[10,0,0,0,4],[9,0,0,0,1]])assert.equal((await command(pendingStart,...args)).result,-3);
assert.deepEqual(snap(pendingStart,0),pendingInfo);assert.deepEqual(snap(pendingStart,5),pendingArr);
assert.deepEqual(objects(pendingStart),pendingStore);assert.equal(pendingStart.ex.web_store_writes(),pendingWrites);
pendingStart.ex.web_seq_play(0);render(pendingStart,4);await good(pendingStart,4);
console.log(JSON.stringify({realWasmWorklet:true,blocks:192,saveFormat4:true,authoritativeSelection:true,
  queuedBarTransition:true,playingIntentLatched:true,noChord:true,sharedChordResolver:true,
  schema1Migration:true,stoppedUndoRedo:true,queueBackpressure:accepted,corruptStoreProtected:true,
  prospectiveRedoEnableBlocked:true,physicalRedoSamePreflight:true,pendingStartNoMutation:true}));
