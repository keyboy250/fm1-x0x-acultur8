// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>Processor=c;await import('../../web/emu/worklet.js');
const p=new Processor();await p.onMessage({type:'load',wasm:fs.readFileSync(process.argv[2]),store:{}});
const render=n=>{for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}};
render(175);assert.equal(typeof p.ex.web_said_len,'function');
const store=()=>Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return n<0?null:Array.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n));});
const before=store();p.messages.length=0;
await p.onMessage({type:'sysex',bytes:[0x7d,0x53,0x52,1,1]});render(10);
await p.onMessage({type:'sysex',bytes:[0x7d,0x53,0x52,3]});render(100);
const frames=p.messages.filter(m=>m.type==='said').flatMap(m=>Array.from(m.bytes));assert.ok(frames.length>0);
let hello=0,say=0;for(let i=0;i<frames.length;i++)if(frames[i]===0xf0&&frames[i+1]===0x7d&&frames[i+2]===0x53&&frames[i+3]===0x52){if(frames[i+4]===1)hello++;if(frames[i+4]===16)say++;}
assert.ok(hello&&say);assert.deepEqual(store(),before,'Reading screen must not save or change flash');
await p.onMessage({type:'sysex',bytes:[0x7d,0x53,0x52,2]});render(10);p.messages.length=0;render(200);assert.equal(p.messages.filter(m=>m.type==='said').length,0);
await p.onMessage({type:'sysex',bytes:[256]});assert.deepEqual(store(),before);
console.log('PASS actual worklet/WASM screen-reader HELLO/WHERE/SAY/BYE, no storage changes, malformed input refusal');
