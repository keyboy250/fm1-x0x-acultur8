// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';import {pathToFileURL} from 'node:url';import path from 'node:path';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};globalThis.registerProcessor=(_,c)=>Processor=c;
await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
const {ccPacket}=await import(pathToFileURL(path.resolve('web/emu/cc-input-worklet.mjs')).href);
const wasm=fs.readFileSync(process.argv[2]),read=n=>JSON.parse(fs.readFileSync('build/midi-drum/'+n));let ticket=0,route=0,assertions=0;
function render(p,n=20){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
const packet=(n,v)=>{let ch=Math.floor(n/88),cc=n%88;return ccPacket([176+ch,cc<32?cc:cc+32,v]);};
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,180);p.route=++route;await p.onMessage({type:'cc-input-route',route:p.route,ready:true,request:++ticket});assert.equal(p.ex.web_cc_mode(),0);return p;}
function state(p,page=4){const ptr=p.ex.web_generation_snapshot(2,page,0);assert(ptr,'Missing generation snapshot page'+page);const n=new Uint32Array(p.mem.buffer,ptr,1)[0];return [...new Uint32Array(p.mem.buffer,ptr+4,n)];}
async function input(p,events,raw=false){for(let i=0;i<events.length;i+=8){const packets=events.slice(i,i+8).map(e=>raw?e:ccPacket(e)),request=++ticket;await p.onMessage({type:'cc-input-batch',route:p.route,request,packets});const r=p.messages.findLast(x=>x.type==='cc-input-reply'&&x.request===request);assert(r);assert.equal(r.result,0);assert.equal(r.prefix,packets.length);render(p);assert.equal(p.ex.web_cc_pending(),0);}}
async function direct(p,n,v){await input(p,[packet(n,v)],true);}
function objects(p){const o={};for(let i=0;i<p.ex.web_nobj();i++){const n=p.ex.web_store_len(i);if(n>=0)o[i]=[...new Uint8Array(p.mem.buffer,p.ex.web_store(i),n)];}return o;}
async function save(p){await direct(p,985,0);await direct(p,985,127);return objects(p);}
for(let kit=0;kit<2;kit++)for(let voice=0;voice<11;voice++){
 const before=read(`before-${kit}-${voice}.json`),after=read(`after-${kit}-${voice}.json`),plan=read(`plan-${kit}-${voice}.json`),p=await open(before);
 assert.equal(state(p,0)[7],63);assert.equal(state(p,0)[9],9000128);
 await input(p,plan.events.slice(0,-3));assert.deepEqual(objects(p),before);assert.equal(state(p,3)[8],255);assert.equal(state(p,3)[9],256);
 assert.equal(p.ex.web_generation_snapshot(2,2,0),0,'303 recipe unavailable while drum family staged');
 await input(p,plan.events.slice(-3));assert.equal(state(p)[5],1);assert.equal(state(p)[6],1);assert.equal(state(p)[8],3);assert.equal(state(p)[9],0);
 assert.deepEqual(await save(p),after,'public wire plan equals native full-object oracle');
 await input(p,plan.events.slice(-2));assert.equal(state(p)[8],3);assert.deepEqual(await save(p),after,'same request does not regenerate');
 await direct(p,1117,0);await direct(p,1117,127);assert.deepEqual(await save(p),before,'Undo whole-object parity');
 await direct(p,1118,0);await direct(p,1118,127);assert.deepEqual(await save(p),after,'Redo whole-object parity');
 const reopened=await open(after);assert.deepEqual(objects(reopened),after);assert.equal(state(reopened,1)[11],0);assertions+=9;
}
const before=read('before-0-0.json'),plan=read('plan-0-0.json'),p=await open(before);
await input(p,plan.events.slice(0,-3));await input(p,[plan.events.at(-3)]);await direct(p,1234,127);await input(p,plan.events.slice(-2));assert.equal(state(p)[8],7);assert.deepEqual(await save(p),before);
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,e,n){p.ex.web_enc(e,n);render(p,30);}
tap(p,5);turn(p,0,1);fs.writeFileSync('build/midi-drum/glo-full.rgb565',new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));turn(p,1,1);assert.equal(p.ex.web_cc_mode(),1,'actual GLO selector changes CC MODE to SIMPLE');assert.equal(state(p,7)[5],1);assert.equal(state(p,1)[8],0);
fs.writeFileSync('build/midi-drum/glo-simple.rgb565',new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));
await direct(p,1010,127);assert.equal(state(p,1)[8],0,'SIMPLE blocks selector');await direct(p,1210,127);assert.deepEqual(objects(p),before);
const unsupported=state(p,7)[9];await direct(p,0,64);assert.equal(state(p,7)[9],unsupported,'SIMPLE allows direct sound');
turn(p,1,1);assert.equal(p.ex.web_cc_mode(),2);fs.writeFileSync('build/midi-drum/glo-off.rgb565',new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));const rejected=state(p,7)[9];await direct(p,0,127);assert.equal(state(p,7)[9],rejected+1,'OFF blocks direct CC');
/* Route-aware browser slice CC cannot bypass native CC mode; manual companion controls remain separate. */
await p.onMessage({type:'slice-command',abi:1,request:++ticket,op:'check',value:0,ccRoute:p.route});assert.equal(p.messages.at(-1).reason,'cc-mode-disabled');
assert.equal(p.ex.web_midi_ingest(0x0000fa0f,2),0);render(p);assert.equal(p.ex.web_seq_playing(),1,'CC OFF preserves MIDI Start');
assert.equal(p.ex.web_midi_ingest(0x0000fc0f,2),0);render(p);assert.equal(p.ex.web_seq_playing(),0,'CC OFF preserves MIDI Stop');
turn(p,1,-1);assert.equal(p.ex.web_cc_mode(),1);p.ex.web_boot();render(p,180);assert.equal(p.ex.web_cc_mode(),0,'reboot defaults FULL');
console.log(`DRUM MIDI WASM PASS: ${assertions} whole-object assertions;22 lanes,public wire plans,native oracle,Undo/Redo/save/reopen,Cancel,actual GLO FULL/SIMPLE/OFF,readback and MIDI Start/Stop unaffected.`);
