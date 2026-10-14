// SPDX-License-Identifier: GPL-3.0-only
// Production worklet + real candidate C. Synthetic ingress/storage bytes only;
// not physical MIDI, flash power-loss or hardware deadline acceptance.
import fs from 'node:fs';import assert from 'node:assert/strict';import crypto from 'node:crypto';
import path from 'node:path';import {pathToFileURL} from 'node:url';
assert.ok(process.argv[2],'Usage: node tests/host/cc_actions_wasm_test.mjs build/emu/x0x.wasm');
const wasm=fs.readFileSync(process.argv[2]);
const inv=JSON.parse(fs.readFileSync('docs/ACULTUR8_CC_INVENTORY.json','utf8')),rows=new Map(inv.entries.map(r=>[r.id,r]));
for(const[id,num]of[['selector.destination_pattern',1132],['action_control.arm',1133],['action_control.cancel',1134]])assert.equal(rows.get(id).parameter_number,num);
let Processor,request=0,route=0;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
function render(p,n=35){const out=[[new Float32Array(128),new Float32Array(128)]];for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],out);}}
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store});render(p,175);await enable(p);return p;}
async function enable(p){await p.onMessage({type:'cc-input-route',route:++route,ready:true,request:++request});assert.equal(p.messages.at(-1).result,0);p.testRoute=route;}
function snap(p,s=0,t=0,v=0){const ptr=p.ex.web_variant_snapshot(s,t,v);assert.ok(ptr);const a=new Uint32Array(p.mem.buffer,ptr,20);return{revision:a[2],values:[...a.slice(4,4+a[3])]};}
const reason=(p,source=2)=>p.ex.web_cc_source(source,31)>>>0;
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n))];}).filter(([,v])=>v));
const source=(p,field,s=2)=>p.ex.web_cc_source(s,field)>>>0;
function packet(id,value,raw=false){const r=rows.get(id);assert.ok(r,id);assert.ok(r.address,id);const[lo,hi]=r.range;
 const v=raw?value:Array.from({length:128},(_,i)=>i).find(i=>lo+Math.floor((i*(hi-lo)+63)/127)===value);assert.notEqual(v,undefined,`${id} ${value} representable`);
 return (11|((0xaf+r.address.channel)<<8)|(r.address.cc<<16)|(v<<24))>>>0;}
async function input(p,packets,s=2){if(s===2){const q=++request;await p.onMessage({type:'cc-input-batch',route:p.testRoute,request:q,packets});const r=p.messages.at(-1);assert.equal(r.result,0);assert.equal(r.prefix,packets.length);}else for(const v of packets)assert.equal(p.ex.web_midi_ingest(v,s),0);render(p);assert.equal(p.ex.web_cc_pending(),0);}
async function ctl(p,id,value,s=2){await input(p,[packet(id,value)],s);}
async function context(p,pat=7,part=0,dest=19,block=0,s=2){await input(p,[packet('selector.pattern',pat),packet('selector.part',part),packet('selector.drum',0),packet('selector.kit',part<2?part:0),packet('selector.bass',part===3?1:0),packet('selector.step',0),packet('selector.block_low',block&127),packet('selector.block_high',block>>7),packet('selector.destination_pattern',dest),packet('selector.commit',1)],s);assert.equal(source(p,0,s),pat);assert.equal(source(p,1,s),part);assert.equal(source(p,30,s),dest);}
async function arm(p,act,s=2){await ctl(p,'action_control.arm',act,s);assert.equal(reason(p,s),0);assert.equal(source(p,29,s),act);}
async function action(p,act,{s=2,expected=0,raw=1}={}){await ctl(p,`action.${act}`,raw,s);assert.equal(reason(p,s),expected,`action${act} reason`);if(!expected)assert.ok(source(p,28,s)&(1<<(act-1)),`action${act} accepted edge`);}
async function cmd(p,op,{target=0,voice=0,index=0,value=0,result=[3,4,5,6,7].includes(op)?0:1}={}){const q=++request;await p.onMessage({type:'variant-command',abi:1,op,request:q,target,voice,index,value});const r=p.messages.findLast(m=>m.type==='variant-reply'&&m.request===q);assert.ok(r);assert.equal(r.result,result);render(p);}
const STREAM=[2,3,5,6];
function unpack(store){const h=store[2],length=h.readUInt32LE(4),packed=h.readUInt32LE(12),data=Buffer.concat(STREAM.map((id,i)=>store[id].subarray(i?4:16))).subarray(0,packed),out=Buffer.alloc(length);let i=0,n=0;
 while(i<data.length){const c=data[i++];if(c<128){data.copy(out,n,i,i+c+1);i+=c+1;n+=c+1;}else{const count=(c&127)+3,d=data.readUInt16LE(i)+1;i+=2;for(let k=0;k<count;k++){out[n]=out[n-d];n++;}}}assert.equal(n,length);return out;}
function pattern(store,pat){const h=store[2],raw=unpack(store),offset=h.readUInt32LE(0)===5?1344:0;return Buffer.from(raw.subarray(offset+528*pat,offset+528*(pat+1)));}
async function save(p){await context(p);await ctl(p,'action.1',0);await action(p,1);assert.ok(p.ex.web_store_writes());return objects(p);}
async function musicalFixture(){const p=await open();await cmd(p,19,{value:1});await cmd(p,18,{target:7,voice:0,value:13});await cmd(p,18,{target:7,voice:0,index:1,value:7});await cmd(p,18,{target:7,voice:24,value:64});await cmd(p,18,{target:7,voice:24,index:2,value:63|(1<<6)|(8<<7)});
 await context(p,7,0);await ctl(p,'step.drum_hit',1);await context(p,7,2);await ctl(p,'step.bass_note',69);await ctl(p,'step.bass_gate',1);await context(p,7,1);await ctl(p,'step.drum_hit',1);const store=await save(p);return{p,store};}

// ClearPart/Pattern/Copy operate on source selectors rather than editor focus,
// retain25 timing cells and BREAK64 metadata, and form one true Undo unit.
for(const act of[2,3,4]){const{p,store}=await musicalFixture();await context(p,7,0,19);await arm(p,act);await action(p,act,{raw:127});const accepted=source(p,28);await action(p,act,{raw:64});assert.equal(source(p,28),accepted,'held action edge no repeat');
 await cmd(p,7);const changed=objects(p),src=pattern(changed,7),before=pattern(store,7);
 if(act===4){assert.deepEqual(pattern(changed,19),before);assert.deepEqual(src,before);assert.deepEqual(snap(p,10,19,0).values.slice(1,3),[13,7]);assert.equal(snap(p,12,19,1).values[0]>>>31,1);assert.equal(snap(p,12,19,1).values[4]>>>28,8);}
 else{assert.equal(src.readUInt32LE(0),0);if(act===2)assert.equal(src.readUInt32LE(100),1,'other kit preserved');else assert.equal(src.readUInt32LE(100),0);assert.deepEqual(snap(p,10,7,0).values.slice(1,3),[13,7]);}
 await ctl(p,'editor.undo',1);await cmd(p,7);assert.deepEqual(pattern(objects(p),7),before,'one Undo restores source pattern');if(act===4)assert.deepEqual(pattern(objects(p),19),pattern(store,19));
}

// Song row structure: chord/duration/mute/pattern arrays move together.
for(const act of[7,8,9]){const p=await open();await cmd(p,10,{value:3});for(let row=0;row<3;row++)await cmd(p,8,{target:row,voice:row|((row+1)<<3),index:row,value:row+4});await context(p,0,0,1,1);const before=snap(p,4,1).values;await arm(p,act);await action(p,act);assert.equal(snap(p,5).values[1],act===7?4:act===8?2:0);if(act===7)assert.deepEqual(snap(p,4,2).values,before);if(act===8)assert.equal(snap(p,4,1).values[7],3);await ctl(p,'editor.undo',1);assert.equal(snap(p,5).values[1],3);assert.deepEqual(snap(p,4,1).values,before);}

// Every source requires its own committed context/arm. Cancel/reset/stale
// context cannot consume a destructive edge or change saved project.
for(const s of[0,1,2]){const{p,store}=await musicalFixture();await context(p,7,0,19,0,s);await arm(p,2,s);await ctl(p,'action_control.cancel',1,s);await action(p,2,{s,expected:4});assert.equal(source(p,28,s)&2,0);await arm(p,2,s);p.ex.web_cc_reset_source(s);await context(p,7,0,19,0,s);await action(p,2,{s,expected:4});await arm(p,2,s);await ctl(p,'selector.pattern',8,s);await ctl(p,'selector.commit',1,s);await action(p,2,{s,expected:4});assert.deepEqual(objects(p),store);await context(p,7,0,19,0,s);await arm(p,2,s);await action(p,2,{s,raw:s===0?1:s===1?64:127});await cmd(p,7);assert.equal(pattern(objects(p),7).readUInt32LE(0),0,'each actual C ingress source executes its own rearmed action');}
const{p:guarded,store:guardedStore}=await musicalFixture();await context(guarded);await arm(guarded,2);guarded.ex.web_seq_play(1);render(guarded);await input(guarded,[packet('action.2',127),packet('transport.stop',1)]);assert.equal(reason(guarded),1);assert.equal(guarded.ex.web_seq_playing(),0);assert.deepEqual(objects(guarded),guardedStore);await action(guarded,2,{expected:4});
for(let act=1;act<=11;act++)await ctl(guarded,`action.${act}`,0);await cmd(guarded,3);const ownedRevision=snap(guarded).revision,ownedEdges=source(guarded,28),ownedWrites=guarded.ex.web_store_writes();for(let act=1;act<=11;act++)await action(guarded,act,{expected:2});await ctl(guarded,'action_control.arm',2);assert.equal(reason(guarded),2);assert.equal(snap(guarded).revision,ownedRevision);assert.equal(source(guarded,28),ownedEdges);assert.equal(guarded.ex.web_store_writes(),ownedWrites);assert.equal(guarded.editOwner,'variant-0');await cmd(guarded,4);
await context(guarded);await arm(guarded,2);await cmd(guarded,2,{index:2,value:12});await action(guarded,2,{expected:4});assert.equal(snap(guarded,2).values[2],12,'stale action never undoes newer parameter intent');

// SAVE+RESET: verified pre-reset checkpoint remains recoverable, no reset
// storage write/autosave, and actual reopen restores saved musical settings.
const{p:reset,store:resetStore}=await musicalFixture();await context(reset);await arm(reset,5);const writes=reset.ex.web_store_writes();await action(reset,5);assert.equal(reset.ex.web_store_writes(),writes);render(reset,1300);assert.equal(reset.ex.web_store_writes(),writes);assert.deepEqual(objects(reset),resetStore);const recovered=await open(resetStore);assert.deepEqual(snap(recovered,10,7,0).values.slice(1,3),[13,7]);assert.equal(snap(recovered,12,7,1).values[0]>>>31,1);

// About/Perf use the real C framebuffer; benchmark runs then restores the
// project on native completion/Stop. No browser-only success flag substitutes.
for(const act of[6,10]){const p=await open();await context(p,0,0,1);const ptr=p.ex.web_fb(),fb=Buffer.from(new Uint8Array(p.mem.buffer,ptr,240*240*2));await action(p,act);render(p,100);assert.notDeepEqual(Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2)),fb);}
const bench=await open();const benchStore=await save(bench);await context(bench,0,0,1);await action(bench,11);render(bench,500);assert.equal(bench.ex.web_seq_playing(),1,'native benchmark actually runs transport');await ctl(bench,'transport.stop',1);render(bench,250);assert.equal(bench.ex.web_seq_playing(),0);await context(bench,0,0,1);await action(bench,6);await cmd(bench,7);assert.deepEqual(unpack(objects(bench)),unpack(benchStore),'benchmark Stop restores actual project');

const complete=await open();await cmd(complete,2,{index:2,value:7});const completeStore=await save(complete);await context(complete,0,0,1);await action(complete,11);render(complete,500);assert.equal(complete.ex.web_seq_playing(),1);render(complete,7200);assert.equal(complete.ex.web_seq_playing(),0,'native benchmark completes bounded three-scene job');await action(complete,6);assert.equal(snap(complete,2).values[2],7);await cmd(complete,7);assert.deepEqual(unpack(objects(complete)),unpack(completeStore),'full benchmark restores real saved settings/project');

// Malformed persisted sound/stream cross-check creates actual C protection.
// SAVE/destructive refusal preserves all input bytes and performs no write.
const corrupted=Object.fromEntries(Object.entries(resetStore).map(([k,v])=>[k,Buffer.from(v)]));corrupted[1][4]^=1;const protectedP=await open(corrupted);assert.equal(snap(protectedP).values[2],1);await context(protectedP);const pw=protectedP.ex.web_store_writes();await action(protectedP,1,{expected:5});await action(protectedP,2,{expected:5});assert.equal(protectedP.ex.web_store_writes(),pw);assert.deepEqual(objects(protectedP),corrupted);
console.log(JSON.stringify({test:'native actions production worklet+actual WASM',wasmBytes:wasm.length,wasmSHA256:crypto.createHash('sha256').update(wasm).digest('hex'),actions11:true,selectors1132_1133_1134:true,source0_1_2:true,oneUndo:true,saveResetReopen:true,playingStopAndArm:true,all11OwnerRefusal:true,staleRevision:true,protectedNoWrites:true,physicalMidi:false,flashWriteFailureInjection:false,storageCrashAtomicity:false,fullBenchmarkCompletion:true}));
