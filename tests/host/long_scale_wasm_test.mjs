// SPDX-License-Identifier: GPL-3.0-only
// Production worklet, actual native serializer fixtures and firmware note output.
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor,request=0;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync(process.argv[2]);
function render(p,n=1){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(file){const p=new Processor();const store=Object.fromEntries(Object.entries(JSON.parse(fs.readFileSync(file))).map(([k,v])=>[k,Uint8Array.from(v)]));await p.onMessage({type:'load',wasm,store});render(p,175);assert.ok(p.messages.some(m=>m.type==='ready'));return p;}
function snap(p,section,target=0){const ptr=p.ex.web_variant_snapshot(section,target,0);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return Array.from(v.subarray(4,4+v[3]));}
async function command(p,op,target=0,voice=0,index=0,value=0){await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice,index,value});const r=p.messages.at(-1);assert.equal(r.type,'variant-reply');assert.equal(r.result,[3,4,5,6,7].includes(op)?0:1);render(p,3);}
const p=await open('build/long-scale-review/long-store.json');
assert.equal(snap(p,4)[6],32);assert.equal(snap(p,5)[1],2);
p.ex.web_seq_play(1);render(p,4);assert.equal(snap(p,5)[3],0);assert.equal(snap(p,5)[7],32);
render(p,32*689-4);assert.equal(snap(p,5)[3],0);render(p,4);assert.equal(snap(p,5)[3],1);
p.ex.web_seq_play(0);render(p,3);
await command(p,3);await command(p,8,0,0,0,0);await command(p,4);assert.equal(snap(p,4)[6],1);
await command(p,5);assert.equal(snap(p,4)[6],32);await command(p,6);assert.equal(snap(p,4)[6],1);
const latched=await open('build/long-scale-review/long-store.json');latched.ex.web_seq_play(1);render(latched,4);
await command(latched,3);await command(latched,8,0,0,0,0);await command(latched,4);
assert.equal(snap(latched,4)[6],1);assert.equal(snap(latched,5)[15],32,'current pass keeps its latched duration');
latched.ex.web_seq_play(0);render(latched,3);
const s=await open('build/long-scale-review/scale-store.json');
assert.equal(snap(s,7)[0],3);assert.equal(snap(s,7)[3],3);assert.equal(snap(s,5)[0]&1,0);
s.ex.web_seq_play(1);render(s,4);assert.equal(snap(s,7)[5],62);s.ex.web_seq_play(0);render(s,4);
await command(s,13,0,0,0,0);s.ex.web_seq_play(1);render(s,4);assert.equal(snap(s,7)[5],63);s.ex.web_seq_play(0);render(s,4);
await command(s,13,0,0,0,3);await command(s,13,0,0,1,61);await command(s,13,0,0,2,61);
s.ex.web_seq_play(1);render(s,4);assert.equal(snap(s,7)[5],255);assert.equal(snap(s,7)[9],1);
console.log('LONG SCALE WASM PASS: 32-bar boundary, stored unequal lane, Undo/Redo, Scale without arrangement/chord, transposed note62 vs Off63, impossible range silent');
