// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};globalThis.registerProcessor=(_,C)=>Processor=C;
await import('../../web/emu/worklet.js');const wasm=fs.readFileSync(process.argv[2]);let request=0;
const render=(p,n=4)=>{for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}};
const open=async store=>{const p=new Processor();await p.onMessage({type:'load',wasm,store});render(p,200);return p;};
const snap=(p,s=0,lane=0)=>{const ptr=p.ex.web_variant_snapshot(s,lane,0);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return Array.from(v.subarray(4,4+v[3]));};
const command=async(p,op,lane=0,index=0,value=0)=>{await p.onMessage({type:'variant-command',abi:1,op,request:++request,target:lane,voice:0,index,value});return p.messages.at(-1);};
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>[i,p.ex.web_store_len(i)]).filter(([,n])=>n>=0).map(([i,n])=>[i,new Uint8Array(p.mem.buffer,p.ex.web_store(i),n).slice()]));
const configs=new Map([[0,[63,17000,9,11,173,15]],[7,[400,14000,3,6,119,13]],[24,[21,18000,24,18,250,7]]]);
const p=await open();assert.equal(p.ex.web_nobj(),8);assert.ok(snap(p)[0]&2048);
for(const [lane,values] of configs){assert.equal((await command(p,21,lane)).result,-7);for(let i=0;i<6;i++)assert.equal((await command(p,2,lane,i,values[i])).result,1);render(p);assert.deepEqual(snap(p,2,lane),values);assert.equal((await command(p,20,lane)).result,0);render(p);}
let store=objects(p);assert.equal(store[7].length,212);const writes=p.ex.web_store_writes();assert.equal((await command(p,20,24)).result,0);assert.equal(p.ex.web_store_writes(),writes,'same default avoids flash wear');p.publish();assert.ok(p.messages.at(-1).store[7],'no-op save replays bank for browser durability ACK');
const reopened=await open(store);for(const [lane,values] of configs){assert.notDeepEqual(snap(reopened,2,lane),values);const before=Array.from({length:25},(_,i)=>snap(reopened,2,i));assert.equal((await command(reopened,21,lane)).result,0);render(reopened);for(let i=0;i<25;i++)assert.deepEqual(snap(reopened,2,i),i===lane?values:before[i]);}
assert.equal((await command(reopened,7)).result,0);const project=await open(objects(reopened));for(const [lane,values] of configs)assert.deepEqual(snap(project,2,lane),values);
project.ex.web_seq_play(1);render(project);const stable=objects(project);assert.equal((await command(project,20)).result,-3);assert.equal((await command(project,21)).result,-3);assert.deepEqual(objects(project),stable);project.ex.web_seq_play(0);render(project);
store=objects(project);store[7][4]=2;const future=await open(store);const preserved=objects(future);assert.equal((await command(future,20)).result,-2);assert.equal((await command(future,21)).result,-2);assert.deepEqual(objects(future),preserved);
console.log('Actual WASM/worklet defaults: distinct BD/CH/BREAK, durable bank replay, no-op writes, explicit isolated recall, project save/reopen, busy/future refusal PASS');
