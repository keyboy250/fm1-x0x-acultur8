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
const read=name=>JSON.parse(fs.readFileSync(`build/303-variation/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);drain(p);}}
async function open(bass,mode){
 const p=new Processor();const store=Object.fromEntries(Object.entries(read(`baseline-${bass}-${mode}`)).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/303-variation/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
function drain(p){
 if(!p.ex?.web_midi_pending)return;
 const n=p.ex.web_midi_batch(64);assert.ok(n>=0);
 for(let i=0;i<n;i++){const packet=p.ex.web_midi_packet(i)>>>0;if(p.capture&&!(packet&0x80000000))p.capture.push({frame:p.frame??0,st:(packet>>>8)&255,note:(packet>>>16)&127,vel:(packet>>>24)&127});}
 assert.equal(p.ex.web_midi_ack(n),0);
}
function prompt(p,bass,mode,pitch){
 turn(p,1,bass+2);tap(p,8);tap(p,8);tap(p,8);turn(p,3,mode); // CONSTRAINED
 tap(p,8);turn(p,3,-1);turn(p,4,6);turn(p,5,-2);turn(p,6,42); // every2,8 candidate positions,BEAT,seed43
 tap(p,8);turn(p,3,pitch);screen(p,`menu-${bass}-${mode}-${pitch}`);tap(p,13);screen(p,`confirm-${bass}-${mode}-${pitch}`);
}
function run(p,bass,mode,pitch,baseOnly=false){
 assert.equal(p.ex.web_midi_ready(1),0);render(p,10);p.capture=[];p.frame=0;p.ex.web_buttons(1<<10);
 for(let i=0;i<Math.ceil((88200*8+4096)/128);i++){p.frame=i;render(p,1);if(i==24)p.ex.web_buttons(0);}
 p.ex.web_buttons(1<<10);render(p,25);p.ex.web_buttons(0);render(p,25);assert.equal(p.ex.web_seq_playing(),0);
 const all=p.capture;p.capture=null;assert.equal(p.ex.web_midi_ready(0),0);render(p,25);
 const held=new Set();let onCount=0,offCount=0;
 for(const e of all){const ch=e.st&15;if(ch>1)continue;const id=`${ch}:${e.note}`;
  if((e.st&240)==144&&e.vel){assert.ok(!held.has(id));held.add(id);onCount++;}
  if((e.st&240)==128||((e.st&240)==144&&!e.vel)){assert.ok(held.has(id));held.delete(id);offCount++;}
 }
 assert.equal(held.size,0);assert.equal(onCount,offCount);
 const attacks=all.filter(e=>(e.st&240)==144&&e.vel&&(e.st&15)<2);
 const expected=read(`events-${bass}-${mode}-${pitch}`).events;assert.ok(attacks.length>=expected.length);
 const origin=attacks[0].frame;
 expected.forEach((native,i)=>{
  const e=attacks[i],step=native[3],note=baseOnly?60+(step%3==1?4:step%3==2?7:0):native[4];
  assert.equal(e.st&15,native[2]);assert.equal(e.note,note,`303 pitch b${bass} mode${mode} pitch${pitch} event${i}`);
  assert.equal(e.vel,native[5]?127:90,'unchanged accent');assert.ok(Math.abs((e.frame-origin)*128-native[0])<=256,'native/WASM event timing');
 });
}
for(let bass=0;bass<2;bass++)for(let mode=0;mode<4;mode++)for(let pitch=0;pitch<3;pitch++){
 const p=await open(bass,mode),baseline=stored(p);prompt(p,bass,mode,pitch);tap(p,1);run(p,bass,mode,pitch);
 tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'all saved objects unchanged after variation playback');
 tap(p,12);tap(p,1);run(p,bass,mode,pitch,true);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Off restores base; save unchanged');
 if(mode==0&&pitch==0){tap(p,13);tap(p,1);p.ex.web_boot();render(p,175);run(p,bass,mode,pitch,true);assert.deepEqual(stored(p),baseline,'boot defaults Off');}
}
const cancelled=await open(0,0);const baseline=stored(cancelled);prompt(cancelled,0,0,0);tap(cancelled,6);run(cancelled,0,0,0,true);assert.deepEqual(stored(cancelled),baseline,'Cancel leaves base');
console.log('303 VARIATION WASM PASS: both303/all4 constraints/all3 pitch modes actualARP/knobs/ARM/SEL/PLAY/OFF; 24 native8cycle MIDI pitch/timing/accent fixtures; note ownership/Stop/Cancel/boot/unchanged saved objects');


