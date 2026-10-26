// SPDX-License-Identifier: GPL-3.0-only
// Production worklet and real WASM/MIDI ingress, plus native whole-object oracle.
import fs from 'node:fs';import assert from 'node:assert/strict';import path from 'node:path';import {pathToFileURL} from 'node:url';
const wasm=fs.readFileSync(process.argv[2]);let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import(pathToFileURL(path.resolve('web/emu/worklet.js')).href);
function render(p,n=30){let out=[[new Float32Array(128),new Float32Array(128)]];for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],out);}}
let request=0,route=0;
const fixture=(name,b)=>JSON.parse(fs.readFileSync(`build/midi-303/${name}-${b}.json`));
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store:Object.fromEntries(Object.entries(store).map(([k,v])=>[k,Uint8Array.from(v)]))});render(p,180);await p.onMessage({type:'cc-input-route',route:++route,ready:true,request:++request});p.testRoute=route;return p;}
function state(p,page=4,source=2){let ptr=p.ex.web_generation_snapshot(source,page,0);assert.ok(ptr);let count=new Uint32Array(p.mem.buffer,ptr,1)[0];return [...new Uint32Array(p.mem.buffer,ptr+4,count)];}
const packet=(n,v)=>{let ch=Math.floor(n/88),cc=n%88;if(cc>=32)cc+=32;return(11|((176+ch)<<8)|(cc<<16)|(v<<24))>>>0;};
async function input(p,packets){const q=++request;await p.onMessage({type:'cc-input-batch',packets,route:p.testRoute,request:q});const reply=p.messages.at(-1);assert.equal(reply.result,0);assert.equal(reply.prefix,packets.length);render(p);assert.equal(p.ex.web_cc_pending(),0);}
const direct=(p,n,v)=>input(p,[packet(n,v)]);
async function exact(p,n,v){await input(p,[packet(1072,n&127),packet(1073,Math.round((n>>7)*127/15)),packet(1074,v&127),packet(1075,(v>>7)&127),packet(1130,(v>>14)&127),packet(1077,127)]);}
const MAX=new Map([[1010,31],[1011,4],[1013,1],[1069,24],[1071,1]]);
async function context(p,bass,pat=1){for(const[n,v]of[[1010,pat],[1011,2+bass],[1013,bass],[1069,22+bass],[1071,1]])await direct(p,n,Math.round(v*127/MAX.get(n)));}
async function recipe(p,{mode=4,length=32,step=0,token=1,seed=0x12345678}={}){for(const[n,v]of[[1203,mode],[1204,0],[1223,length],[1224,step]])await exact(p,n,v);for(let i=0;i<4;i++)await exact(p,1227+i,(seed>>>(8*i))&255);await direct(p,1231,token&127);await direct(p,1232,token>>7);}
async function arm(p){await direct(p,1233,36);assert.equal(state(p)[8],1,JSON.stringify({recipe:state(p,2),context:state(p,1),result:state(p)}));}
async function execute(p){await direct(p,1205,0);await direct(p,1205,127);}
async function cmd(p,op,{target=0,voice=0,index=0,value=0}={}){const q=++request;await p.onMessage({type:'variant-command',abi:1,op,request:q,target,voice,index,value});const reply=p.messages.findLast(x=>x.type==='variant-reply'&&x.request===q);assert.ok(reply);assert.ok(reply.result>=0);render(p);}
function objects(p){const result={};for(let i=0;i<p.ex.web_nobj();i++){let n=p.ex.web_store_len(i);if(n>=0)result[i]=[...new Uint8Array(p.mem.buffer,p.ex.web_store(i),n)];}return result;}
async function save(p){await direct(p,985,0);await direct(p,985,127);return objects(p);}
for(const bass of[0,1]){
 const before=fixture('before',bass),whole=fixture('whole',bass),single=fixture('single',bass),p=await open(before);
 await context(p,bass);await recipe(p);assert.deepEqual(objects(p),before,'staging never writes storage');
 await arm(p);await execute(p);assert.equal(state(p)[8],3);assert.equal(state(p)[6],1);
 const generated=await save(p);assert.deepEqual(generated,whole,'actual CC generation matches native musical/object oracle');
 await direct(p,1205,127);assert.equal(state(p)[8],3);assert.deepEqual(await save(p),whole,'held-high does not execute twice');
 await direct(p,1117,0);await direct(p,1117,127);assert.deepEqual(await save(p),before,'Undo restores all saved objects');
 await direct(p,1118,0);await direct(p,1118,127);assert.deepEqual(await save(p),whole,'Redo restores all saved objects');
 await recipe(p,{length:64,step:7,token:2});await arm(p);await execute(p);assert.equal(state(p)[8],3);
 assert.deepEqual(await save(p),single,'single NOTE changes exactly native expected pitch/object bytes');
 const reopened=await open(single);assert.deepEqual(objects(reopened),single,'save/reopen parity');
 assert.equal(state(reopened,2)[9],0,'remote recipe seed resets after reboot');
 p.ex.web_boot();render(p,180);assert.equal(state(p,2)[9],0,'same-instance reboot clears remote recipe');
}
const p=await open(fixture('before',0));await context(p,0);await recipe(p);await arm(p);await direct(p,1234,127);await execute(p);assert.equal(state(p)[8],7);assert.deepEqual(await save(p),fixture('before',0),'Cancel never mutates project');
await context(p,0);await recipe(p,{token:2});await arm(p);await cmd(p,2,{index:2,value:19});await execute(p);assert.equal(state(p)[9],2,'stale project refused');
await context(p,0);await recipe(p,{token:3});await arm(p);p.ex.web_cc_reset_source(2);await execute(p);assert.equal(state(p)[8],5,'source reset invalidates arm');
await context(p,0);await recipe(p,{token:4});await arm(p);assert.equal(p.ex.web_midi_ingest(0x0000f80f,2),0);render(p);await execute(p);await direct(p,1120,127);assert.equal(p.ex.web_seq_playing(),0);
console.log('MIDI303 WASM PASS: both303 production worklet/MIDI, native complete object parity, exact single-NOTE, Undo/Redo/save/reopen, Cancel/stale/reset and Stop.');
