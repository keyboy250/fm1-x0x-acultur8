// SPDX-License-Identifier: GPL-3.0-only
// Audible vertical slice: production worklet -> real command queue -> 909 summed strip.
// node tests/host/kick_wasm_test.mjs CURRENT.wasm [PRE_ACTIVATION.wasm]
import fs from 'node:fs';
import assert from 'node:assert/strict';
let Processor;
globalThis.currentTime = 0;
globalThis.AudioWorkletProcessor = class {
  constructor() { this.messages=[]; this.port={postMessage:m=>this.messages.push(m)}; }
};
globalThis.registerProcessor = (_,ctor) => {Processor=ctor;};
await import('../../web/emu/worklet.js');
const bytes = fs.readFileSync(process.argv[2]);
const baselineBytes = process.argv[3] ? fs.readFileSync(process.argv[3]) : bytes;
let request=0;
const render = (p, blocks) => {
  const samples = new Float32Array(blocks*256);
  const meters=[];
  for(let i=0;i<blocks;i++) {
    currentTime+=128/44100;
    const out=[[new Float32Array(128),new Float32Array(128)]];
    p.process([],out);
    for(let k=0;k<128;k++) {samples[256*i+2*k]=out[0][0][k]; samples[256*i+2*k+1]=out[0][1][k];}
    const m=snapshot(p,3);
    if(m) meters.push(m);
  }
  return {samples,meters};
};
const snapshot = (p, section, target=0) => {
  const ptr=p.ex.web_variant_snapshot(section,target,0);
  if(!ptr)return null;
  const v=new Uint32Array(p.mem.buffer,ptr,20);
  return Array.from(v.subarray(4,4+v[3]));
};
const command = async(p,op,index=0,value=0,target=0) => {
  await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice:0,index,value});
  const r=p.messages.at(-1);
  assert.equal(r.type,'variant-reply'); assert.equal(r.request,request);
  assert.equal(r.result,op===1 || op===2?1:0);
  return r;
};
const open = async(store,wasm=bytes) => {
  const p=new Processor(); await p.onMessage({type:'load',wasm,store}); render(p,175);
  if (!store) await command(p,1,3,24); // lower kick Level pot to leave master headroom for boost comparison
  return p;
};
const config = async(p,fields={}) => {
  for(const [index,value] of Object.entries(fields))await command(p,2,+index,value);
  render(p,40); // apply queue and finish stage/control transitions before the first hit
};
const hit = async(p,key=1) => {
  await p.onMessage({type:'keys',mask:1<<key});
  const first=render(p,22);
  await p.onMessage({type:'keys',mask:0});
  const rest=render(p,178);
  return {samples:Float32Array.from([...first.samples,...rest.samples]),meters:[...first.meters,...rest.meters]};
};
const objects = p => Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=> {
  const n=p.ex.web_store_len(i);
  return [i,n<0?null:new Uint8Array(p.mem.buffer,p.ex.web_store(i),n).slice()];
}).filter(([,v])=>v!==null));
const energy = a => a.reduce((sum,x)=>sum+x*x,0);
const peak = a => a.reduce((max,x)=>Math.max(max,Math.abs(x)),0);
const equalAudio = (a,b,why) => assert.ok(Buffer.from(a.buffer).equals(Buffer.from(b.buffer)),why);
const capture = async(fields={},key=1,wasm=bytes) => {
  const p=await open(undefined,wasm); await config(p,fields); return {p,...await hit(p,key)};
};
const dry=await capture();
assert.ok(peak(dry.samples)>.01,'stopped panel kick must actually sound');
const original=await capture({},1,baselineBytes);
equalAudio(dry.samples,original.samples,'default strip must preserve pre-activation PCM bitwise');
const configuredBypass=await capture({0:1000,1:20,2:24,3:36,5:14});
equalAudio(dry.samples,configuredBypass.samples,'disabled configuration must leave PCM bitwise intact');
const highpass=await capture({0:1000,5:3});
const lowpass=await capture({1:20,5:5});
const boost=await capture({2:6,5:1});
const e=energy(dry.samples), hp=energy(highpass.samples)/e, lp=energy(lowpass.samples)/e;
assert.ok(hp<.25,`1000 Hz HPF attenuates kick energy: ${hp}`);
assert.ok(lp<.7,`20 Hz LPF attenuates kick energy: ${lp}`);
assert.ok(energy(boost.samples)>e*1.4,'+6 dB boost audibly increases kick energy');
const limited=await capture({2:24,3:36,5:9});
const maxInput=Math.max(...limited.meters.map(m=>m[0]))/1e6;
const maxOutput=Math.max(...limited.meters.map(m=>m[1]))/1e6;
const minGain=Math.min(...limited.meters.map(m=>m[2]))/1e6;
assert.ok(maxInput>.01 && maxOutput>0);
assert.ok(maxOutput<=10**(-36/20)+2e-6,'summed kick meter obeys requested ceiling');
assert.ok(minGain<.1,'meter exposes real gain reduction');
assert.ok(peak(limited.samples)<peak(dry.samples)*.2,'limiter materially bounds the audible signal');
assert.equal(snapshot(limited.p,0)[7],1,'only 909 BD processing is advertised');
assert.equal(snapshot(limited.p,3,1),null,'unsupported strip cannot expose an invented meter');
limited.p.publish();
assert.ok(limited.p.messages.at(-1).variant.meters,'production frame publishes actual meters');
const snare=await capture({},3), unaffected=await capture({2:24,3:36,5:9},3);
assert.ok(peak(snare.samples)>.01);
equalAudio(snare.samples,unaffected.samples,'kick strip cannot change the snare');
// Save/reopen persisted enabled settings, then undo/redo changes and compare actual output.
const savedSource=await open();
await config(savedSource,{0:1000,5:3}); await command(savedSource,7);
const saved=objects(savedSource);
const reopened=await open(saved); await config(reopened);
assert.deepEqual(snapshot(reopened,2),[1000,20000,0,1,80,3]);
const reopenedAudio=await hit(reopened);
equalAudio(reopenedAudio.samples,highpass.samples,'saved HPF must reopen with the same audible output');
const undone=await open(saved), redone=await open(saved), expectedDry=await open(saved), expectedHP=await open(saved);
for(const p of [undone,redone,expectedDry]) {
  await command(p,3); await command(p,2,5,0); await command(p,4);
}
await command(undone,5);
await command(redone,5); render(redone,4); await command(redone,6);
// Equalize render time while stopped so comparison includes deterministic source phase.
render(undone,44); render(redone,40); render(expectedDry,44);
render(expectedHP,44);
const undoAudio=await hit(undone), redoAudio=await hit(redone), expectedDryAudio=await hit(expectedDry), expectedHPAudio=await hit(expectedHP);
equalAudio(undoAudio.samples,expectedHPAudio.samples,'undo restores audible HPF');
equalAudio(redoAudio.samples,expectedDryAudio.samples,'redo restores audible bypass');
assert.ok(energy(undoAudio.samples)<energy(redoAudio.samples)*.25);
console.log(JSON.stringify({realWorkletWasm:true,independentBaseline:!!process.argv[3],
  bypassBitwise:true,highpassEnergyRatio:hp,lowpassEnergyRatio:lp,
  boostEnergyRatio:energy(boost.samples)/e,limiter:{maxInput,maxOutput,minGain},
  savedAudibleReopen:true,audibleUndoRedo:true,snareIsolationBitwise:true}));
