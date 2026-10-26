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
const read=name=>JSON.parse(fs.readFileSync(`build/repeating-variation/${name}.json`));
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);drain(p);}}
async function open(){
 const p=new Processor();const store=Object.fromEntries(Object.entries(read("baseline")).map(([k,v])=>[k,Uint8Array.from(v)]));
 await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));return p;
}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,encoder,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(encoder,Math.sign(n));render(p);}}
function chord(p,held,b){p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons((1<<held)|(1<<b));render(p);p.ex.web_buttons(1<<held);render(p);p.ex.web_buttons(0);render(p);}
function stored(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));}return o;}
function screen(p,name){fs.writeFileSync(`build/repeating-variation/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}


const black=[1,3,5,8,10,13,15,17,20,22,25],gm=[[36,38,41,45,50,37,39,42,46,49,51],[36,38,41,45,50,37,39,56,49,46,42]];
function key(p,k){p.ex.web_keys(1<<k);render(p);p.ex.web_keys(0);render(p);}
function drain(p){
 if(!p.ex?.web_midi_pending)return;
 const n=p.ex.web_midi_batch(64);assert.ok(n>=0);
 for(let i=0;i<n;i++){const packet=p.ex.web_midi_packet(i)>>>0;if(p.capture&&!(packet&0x80000000))p.capture.push({frame:p.frame??0,st:(packet>>>8)&255,note:(packet>>>16)&127,vel:(packet>>>24)&127});}
 assert.equal(p.ex.web_midi_ack(n),0);
}
function prompt(p,lane,options=0){
 turn(p,1,Math.floor(lane/11));tap(p,8);tap(p,8);key(p,black[lane%11]); // second ARP page
 turn(p,4,1);turn(p,6,42); // changes3, seed43 (raw42)
 if(options){tap(p,8);turn(p,3,1);turn(p,4,1);turn(p,5,2);} // ADD,EVOLVE,LAST8
 screen(p,`menu-${lane}-${options}`);tap(p,13);screen(p,`confirmation-${lane}-${options}`);
}
function run(p,lane,options=0,baseOnly=false){
 assert.equal(p.ex.web_midi_ready(1),0);render(p,10);p.capture=[];p.frame=0;
 p.ex.web_buttons(1<<10); // actual PLAY; keep it held through UI service
 for(let i=0;i<Math.ceil((88200*8+4096)/128);i++){p.frame=i;render(p,1);if(i==24)p.ex.web_buttons(0);}
 p.ex.web_buttons(1<<10);render(p,25);p.ex.web_buttons(0);render(p,25);assert.equal(p.ex.web_seq_playing(),0);
 const events=p.capture;p.capture=null;assert.equal(p.ex.web_midi_ready(0),0);render(p,25);
 const on=events.filter(e=>(e.st&240)==144&&e.vel>0);assert.ok(on.length>100,`actual MIDI output emitted: ${on.length} attacks / ${events.length} total packets`);
 const first=on[0].frame;const rows=Array.from({length:22},()=>Array(8).fill(0));
 for(const e of on){const k=(e.st&15)-9;assert.ok(k==0||k==1);const v=gm[k].indexOf(e.note);assert.ok(v>=0);const absolute=Math.round((e.frame-first)*128/5512.5),cycle=Math.floor(absolute/16),step=absolute%16;
  if(cycle<0||cycle>=8)continue;rows[k*11+v][cycle]|=1<<step;assert.equal(e.vel,(0x9292>>>step&1)?127:88,'base accent preserved');
 }
 const expected=read(`expected-${lane}-${options}`).rows;
 if(baseOnly){for(let l=0;l<22;l++)for(let cy=0;cy<8;cy++)assert.equal(rows[l][cy],expected[l][0],'boot/off plays base every cycle');}
 else assert.deepEqual(rows,expected,'native/WASM full8cycle hit parity');
 return rows;
}
for(let lane=0;lane<22;lane++){
 const p=await open();const baseline=stored(p);prompt(p,lane);tap(p,1);run(p,lane);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'live variation leaves every saved object unchanged');
 tap(p,12);tap(p,1);run(p,lane,0,true);tap(p,7);render(p,100);assert.deepEqual(stored(p),baseline,'Off restores base with saved bytes intact');
 if(lane==1){tap(p,13);tap(p,1);p.ex.web_boot();render(p,175);run(p,lane,0,true);assert.deepEqual(stored(p),baseline,'boot disables runtime overlay; saved base intact');}
}
const evolved=await open();prompt(evolved,1,1);tap(evolved,1);run(evolved,1,1);tap(evolved,7);render(evolved,100);assert.deepEqual(stored(evolved),read('baseline'),'ADD/evolving/fill-region saved data unchanged');
const cancelled=await open();const baseline=stored(cancelled);prompt(cancelled,1);tap(cancelled,6);run(cancelled,1,0,true);assert.deepEqual(stored(cancelled),baseline,'Cancel leaves base-only playback');
console.log('REPEATING VARIATION WASM PASS: all22 lanes actualARP/black-key/knobs/ARM/SEL/PLAY/OFF; full8cycle native/MIDI/accent parity; ADD/EVOLVE/LAST8; unchanged saved bytes, Cancel and boot reset');
