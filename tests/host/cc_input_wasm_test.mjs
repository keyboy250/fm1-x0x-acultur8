// SPDX-License-Identifier: GPL-3.0-only
// Production worklet and compiled shared engine. MIDI packets are synthetic;
// this establishes no browser permission, physical device or timing guarantee.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
assert.ok(process.argv[2],'Usage: node tests/host/cc_input_wasm_test.mjs build/emu/x0x.wasm');
const wasm=fs.readFileSync(process.argv[2]);
const inventory=JSON.parse(fs.readFileSync('docs/ACULTUR8_CC_INVENTORY.json','utf8'));
const byId=new Map(inventory.entries.map(row=>[row.id,row]));
for(const [id,n,ch,cc] of [['selector.pattern',1010,12,74],['selector.strip',1069,13,13],
  ['selector.commit',1071,13,15],['lane.length',1128,13,104],['lane.rate',1129,13,105],['lane.mode',1131,13,107]]) {
  const row=byId.get(id);assert.equal(row.parameter_number,n);assert.deepEqual(row.address,{channel:ch,cc});
}
const packet=(status,cc,value)=>(11|(status<<8)|(cc<<16)|(value<<24))>>>0;
// Independent CC7 inverse for representable integer values, checked against the
// generated address inventory but not the production browser packing helper.
function control(id,value) {
  const row=byId.get(id),[min,max]=row.range;
  const raw=Array.from({length:128},(_,i)=>i).find(i=>min+Math.floor((i*(max-min)+63)/127)===value);
  assert.notEqual(raw,undefined,`${id}=${value} is exactly representable by CC7`);
  return packet(0xb0+row.address.channel-1,row.address.cc,raw);
}
const context=(pattern,lane,step=0)=>[control('selector.pattern',pattern),control('selector.strip',lane),
  control('selector.step',step),control('selector.commit',1)];
let Processor,serial=0,route=0;
globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(name,C)=>{assert.equal(name,'x0x');Processor=C;};
await import('../../web/emu/worklet.js');
const p=new Processor(),output=[[new Float32Array(128),new Float32Array(128)]];
function render(n=12){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],output);}}
function snap(section=0,target=0,voice=0){
  const ptr=p.ex.web_variant_snapshot(section,target,voice);assert.ok(ptr,`snapshot ${section}/${target}/${voice}`);
  const v=new Uint32Array(p.mem.buffer,ptr,20);return {revision:v[2],values:[...v.slice(4,4+v[3])]};
}
const lane=(pattern,voice)=>snap(10,pattern,voice).values.slice(1,3);
const stats=()=>Array.from({length:6},(_,i)=>p.ex.web_cc_stat(i)>>>0);
function frame(){p.publish();return p.messages.at(-1);}
async function send(m){await p.onMessage({...m,request:++serial});const reply=p.messages.at(-1);assert.equal(reply.request,serial);return reply;}
async function enable(){route++;const reply=await send({type:'cc-input-route',route,ready:true});assert.equal(reply.result,0);assert.equal(reply.ready,true);}
async function input(packets,{expected=packets.length,drain=true}={}){
  assert.ok(packets.length<=16);
  const reply=await send({type:'cc-input-batch',route,packets});assert.equal(reply.result,0);
  assert.equal(reply.prefix,expected,`ingress prefix must count C's accepted return 0; canonical mode=${snap(11).values}, stats=${stats()}`);
  if(drain){render();assert.equal(p.ex.web_cc_pending(),0);assert.equal(snap().values[3],snap().values[4]);}
  return reply;
}
async function command(op,{target=0,voice=0,index=0,value=0}={}){
  const reply=await send({type:'variant-command',abi:1,op,target,voice,index,value});assert.equal(reply.type,'variant-reply');return reply.result;
}
async function load(store){await p.onMessage({type:'load',wasm,store});render(35);assert.equal(p.ex.web_cc_version(),0x10001);assert.ok(snap().values[0]&1024);}
function telemetry(){
  const cc=frame().ccInput;assert.deepEqual(cc,{version:0x10001,route,ready:p.ccInput.ready,pending:p.ex.web_cc_pending(),source:Array.from({length:32},(_,i)=>p.ex.web_cc_source(2,i)>>>0),slice:p.sliceControl.state(),stats:stats()});
  return cc;
}
await load();assert.deepEqual(snap(11).values,[0,0,0,0,25,8,64]);await enable();
await input([control('lane.mode',1)]);assert.deepEqual(snap(11).values,[1,1,1,0,25,8,64]);
// First packet is an actual-ABI negative control: C returns 0 after accepting
// it. A bridge expecting 1 reports prefix0 even though canonical mode changed.
assert.equal(stats()[0],1);telemetry();

// Explicit source selectors never borrow the browser's selected editor row.
await p.onMessage({type:'lane-select',pattern:2,lane:3,page:0,viewRevision:79});
const expected=new Map();
for(const pattern of [0,7,31])for(let sound=0;sound<25;sound++){
  const length=[1,7,13,64][(pattern+sound)%4],rate=(pattern+sound)%8;
  await input([...context(pattern,sound),control('lane.length',length),control('lane.rate',rate)]);
  expected.set(`${pattern}:${sound}`,[length,rate]);
  assert.deepEqual(lane(pattern,sound),[length,rate]);
  assert.equal(p.ex.web_cc_source(2,0),pattern);assert.equal(p.ex.web_cc_source(2,10),sound);assert.equal(p.ex.web_cc_source(2,12),1);
}
for(const [key,values] of expected){const [pattern,sound]=key.split(':').map(Number);assert.deepEqual(lane(pattern,sound),values,'distant pattern/voice retains its own settings');}
assert.deepEqual(lane(2,3),[16,0]);
let view=frame().variant.laneView;assert.deepEqual([view.pattern,view.lane,view.viewRevision],[2,3,79]);assert.deepEqual(view.settings.values.slice(1,3),[16,0]);
telemetry();

// Ingress acceptance is deliberately separate from semantic application.
const before=lane(31,24),priorStats=stats();
await input([packet(0xbc,32,127)]);assert.equal(stats()[2],priorStats[2]+1,'reserved fine CC is accepted into ingress then classified unsupported');assert.deepEqual(lane(31,24),before);
const untouched=stats();
for(const packets of [[packet(0x90,60,100)],[packet(0xb0,120,0)],[control('lane.length',7),,control('lane.rate',7)],Array(17).fill(control('lane.rate',0))]){
  const reply=await send({type:'cc-input-batch',route,packets});assert.equal(reply.result,-1);assert.equal(reply.prefix,0);assert.deepEqual(stats(),untouched);assert.deepEqual(lane(31,24),before);
}
assert.equal((await send({type:'cc-input-batch',route:route+1,packets:[control('lane.length',7)]})).result,-3);
assert.deepEqual(stats(),untouched);

// Live CC uses the real main/audio service. Desired values become canonical,
// then actual lane timing latches at its next local step, observed by VS10.
await input([...context(0,0),control('lane.length',13),control('lane.rate',0)]);
await p.onMessage({type:'seq-tempo',bpm:240});assert.equal(p.ex.web_seq_bpm(),240);p.ex.web_seq_play(1);render(12);assert.equal(p.ex.web_seq_playing(),1);
await input([control('lane.length',7),control('lane.rate',7)],{drain:false});
// The receipt may precede canonical application of the second packet: main
// servicing preserves its order behind the first packet's audio marker.
assert.ok(p.ex.web_cc_pending()>0);telemetry();
let latched=false;for(let i=0;i<400;i++){render(1);const s=snap(10,0,0).values;if(s[1]===7&&s[2]===7&&s[6]===7&&!s[8]){latched=true;break;}}
assert.equal(latched,true,'audio-owned next-local-step state reaches requested rate within bounded musical time');
await input([control('lane.mode',0)],{drain:false});assert.equal(snap(11).values[1],0);
for(let i=0;i<500&&snap(11).values[2]!==0;i++)render(1);
assert.equal(snap(11).values[2],0,'prepared Off latches on a real bar');assert.deepEqual(lane(0,0),[7,7]);
p.ex.web_seq_play(0);render(35);assert.equal(p.ex.web_seq_playing(),0);
await input([control('lane.mode',1)]);

// One explicit panel gesture plus incoming CC shares the actual C journal.
// Undo restores both changes; a browser owner remains held throughout ingress.
assert.equal(await command(7),0);render();const originalStrip=snap(2).values[0],originalLane=lane(7,22);
assert.equal(await command(3),0);assert.equal(await command(2,{value:100}),1);render();
await input([...context(7,22),control('lane.length',11),control('lane.rate',4)]);
assert.equal(p.editOwner,'variant-0');assert.equal(snap(2).values[0],100);assert.deepEqual(lane(7,22),[11,4]);
assert.equal(await command(4),0);assert.equal(await command(5),0);render(35);
assert.equal(snap(2).values[0],originalStrip);assert.deepEqual(lane(7,22),originalLane,'CC and panel change undo together by documented shared policy');
assert.equal(await command(6),0);render(35);assert.equal(snap(2).values[0],100);assert.deepEqual(lane(7,22),[11,4]);

// Disable resets explicit source context. Already applied settings survive;
// stale route packets and values without a new selector commit cannot mutate.
route++;assert.equal((await send({type:'cc-input-route',route,ready:false})).result,0);assert.equal(p.ex.web_cc_source(2,12),0);assert.deepEqual(lane(7,22),[11,4]);
assert.equal((await send({type:'cc-input-batch',route:route-1,packets:[control('lane.length',64)]})).result,-3);
assert.equal((await send({type:'cc-input-route',route,ready:true})).result,-3);
await enable();const rejected=stats()[1];await input([control('lane.length',64)]);assert.equal(stats()[1],rejected+1);assert.deepEqual(lane(7,22),[11,4]);
await input([...context(7,22),control('lane.length',13)]);assert.deepEqual(lane(7,22),[13,4]);

// Save/reload uses actual stored engine objects, not a synthetic replacement.
assert.equal(await command(7),0);render(35);
const store={};for(let obj=0;obj<p.ex.web_nobj();obj++){const n=p.ex.web_store_len(obj);if(n>=0)store[obj]=new Uint8Array(p.mem.buffer,p.ex.web_store(obj),n).slice();}
const old=p.ex;await load(store);assert.notEqual(p.ex,old);assert.equal(p.ccInput.ready,false);assert.equal(p.ex.web_cc_source(2,12),0);assert.deepEqual(lane(7,22),[13,4]);
assert.equal((await send({type:'cc-input-route',route,ready:true})).result,-3);await enable();
const rejectedReload=stats()[1];await input([control('lane.length',64)]);assert.equal(stats()[1],rejectedReload+1);assert.deepEqual(lane(7,22),[13,4]);
await input([...context(31,24),control('lane.length',7)]);assert.deepEqual(lane(31,24),[7,expected.get('31:24')[1]]);

// A reset cancels only unreviewed packets. Here the first CC is already
// canonical and the second is accepted into ingress but awaits audio service.
await input([control('lane.length',9),control('lane.length',64)],{drain:false});assert.equal(lane(31,24)[0],9);
route++;assert.equal((await send({type:'cc-input-route',route,ready:false})).result,0);render(35);
assert.equal(lane(31,24)[0],9,'source reset retains the applied prefix and cancels its unreviewed suffix');assert.equal(p.ex.web_cc_pending(),0);
await enable();await input(context(31,24));

// Exercise the real bounded ingress result, including a partial prefix. CC
// has 240 of the 256 host ring entries available; the rest protect urgent MIDI.
const queue=Array.from({length:238},(_,i)=>control('lane.length',1+i%63));
for(let i=0;i<queue.length;i+=16)await input(queue.slice(i,i+16),{drain:false});
assert.equal(p.ex.web_cc_pending(),238);
const tail=[17,19,23,29].map(value=>control('lane.length',value));
await input(tail,{expected:2,drain:false});assert.equal(p.ex.web_cc_pending(),240);
await input(tail.slice(2),{expected:0,drain:false});assert.equal(p.ex.web_cc_pending(),240);
render(35);assert.equal(p.ex.web_cc_pending(),0);assert.equal(lane(31,24)[0],19,'only acknowledged prefix reaches canonical state');
await input(tail.slice(2));assert.equal(lane(31,24)[0],29);assert.ok(stats()[5]>=2);

// A real failed WebAssembly compilation resets the retained source context
// and route readiness without rolling back the retained engine's project.
const retained=p.ex;await assert.rejects(p.onMessage({type:'load',wasm:new Uint8Array([0])}),WebAssembly.CompileError);
assert.equal(p.ex,retained);assert.equal(p.ccInput.ready,false);assert.equal(p.ex.web_cc_source(2,12),0);assert.equal(lane(31,24)[0],29);
assert.equal((await send({type:'cc-input-route',route,ready:true})).result,-3);
await enable();await input([...context(31,24),control('lane.length',13)]);assert.equal(lane(31,24)[0],13);
telemetry();
console.log(JSON.stringify({test:'CC input production worklet + actual WASM',wasmBytes:wasm.length,
  wasmSha256:crypto.createHash('sha256').update(wasm).digest('hex'),source:2,patternLaneCases:75,all25Sounds:true,
  all8Rates:true,canonicalAndAudioReadback:true,ingressVsSemanticRejection:true,sharedUndoRedo:true,
  explicitContextReset:true,unreviewedSuffixCancelled:true,realRingPartialPrefix:true,savedReload:true,failedLoadRetainsProject:true,physicalMidi:false}));
