import fs from 'node:fs';import assert from 'node:assert/strict';import crypto from 'node:crypto';
let Processor,request=0;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync('build/emu/x0x.wasm');
function render(p,n=1){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(store){const p=new Processor();await p.onMessage({type:'load',wasm,store});render(p,175);return p;}
function snap(p,section=0,target=0,voice=0){const ptr=p.ex.web_variant_snapshot(section,target,voice);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return Array.from(v.subarray(4,4+v[3]));}
async function cmd(p,op,target=0,voice=0,index=0,value=0){const id=++request;await p.onMessage({type:'variant-command',abi:1,request:id,op,target,voice,index,value});const reply=p.messages.findLast(m=>m.type==='variant-reply'&&m.request===id);assert.ok(reply);assert.ok(reply.result>=0,JSON.stringify({op,target,voice,index,value,reply}));return reply;}
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n))];}).filter(([,v])=>v));
const p=await open();await cmd(p,19,0,0,0,1);render(p,4);
await cmd(p,3);
const old=[];
for(let pat=0;pat<32;pat++){old[pat]=[];for(let lane=0;lane<25;lane++){const length=1+(pat*7+lane*3)%64,rate=(pat+lane)%8;await cmd(p,18,pat,lane,0,length);await cmd(p,18,pat,lane,1,rate);old[pat][lane]=[length,rate];if(lane%8===7)render(p);}}
await cmd(p,4);render(p,4);await cmd(p,7);const legacy=objects(p);assert.equal(legacy[2].readUInt32LE(0),5);
await cmd(p,3);await cmd(p,18,0,0,8,1);await cmd(p,4);render(p,4);assert.equal(snap(p,11)[5],21);
for(let pat=0;pat<32;pat++)for(let lane=0;lane<25;lane++){const view=snap(p,10,pat,lane);assert.equal(view.length,13);assert.deepEqual(view.slice(1,3),old[pat][lane]);assert.deepEqual(view.slice(9),[0,0,0,1]);}
await cmd(p,3);
for(let lane=0;lane<25;lane++){await cmd(p,18,0,lane,4,3);await cmd(p,18,0,lane,5,lane%2);await cmd(p,18,0,lane,6,lane%3);for(let rate=0;rate<21;rate++){await cmd(p,18,0,lane,3,rate);render(p);assert.equal(snap(p,10,0,lane)[2],rate);}}
await cmd(p,4);render(p,4);await cmd(p,7);const saved=objects(p);assert.equal(saved[2].readUInt32LE(0),6);
const reopened=await open(saved);
for(let lane=0;lane<25;lane++)assert.deepEqual(snap(reopened,10,0,lane).slice(9),[3,lane%2,lane%3,1]);
assert.equal(snap(reopened,10,0,24)[2],20);
await cmd(reopened,3);await cmd(reopened,18,0,24,3,11);await cmd(reopened,4);render(reopened,4);await cmd(reopened,5);render(reopened,4);assert.equal(snap(reopened,10,0,24)[2],20);await cmd(reopened,6);render(reopened,4);assert.equal(snap(reopened,10,0,24)[2],11);
// Production CC ingress/source2, with independently fixed family bins.
const inventory=JSON.parse(fs.readFileSync('docs/ACULTUR8_CC_INVENTORY.json','utf8'));
const byId=new Map(inventory.entries.map(row=>[row.id,row]));
function control(id,value){const row=byId.get(id),[min,max]=row.range,raw=Array.from({length:128},(_,i)=>i).find(i=>min+Math.floor((i*(max-min)+63)/127)===value);assert.notEqual(raw,undefined);return (11|((0xb0+row.address.channel-1)<<8)|(row.address.cc<<16)|(raw<<24))>>>0;}
const ordered=[[8,9,4,5,0,2,10],[11,8,12,9,13,4,14,5,15,0,16,2,17,10],[8,18,9,19,4,6,5,3,0,1,2,7,10,20],[11,8,12,18,9,13,19,4,14,6,5,15,3,0,16,1,2,17,7,10,20]];
await p.onMessage({type:'cc-input-route',route:1,ready:true,request:++request});assert.equal(p.messages.at(-1).result,0);
async function input(packets){await p.onMessage({type:'cc-input-batch',route:1,request:++request,packets});assert.equal(p.messages.at(-1).prefix,packets.length);let guard=20;do{render(p);}while((p.ex.web_cc_pending()||snap(p)[3]!==snap(p)[4])&&--guard);assert.ok(guard);}
const rejected=p.ex.web_cc_stat(1);
for(const lane of [0,7,24])for(let family=0;family<4;family++){
 await input([control('selector.pattern',0),control('selector.strip',lane),control('selector.commit',1),control('rate.family',family),control('rate.hysteresis',0)]);
 for(let v=0;v<256;v++){const value=v<128?v:255-v,expected=ordered[family][Math.floor(value*ordered[family].length/128)];await input([control('rate.sweep_v1',value)]);assert.equal(snap(p,10,0,lane)[2],expected);}
}
assert.equal(p.ex.web_cc_stat(1),rejected);
// Independently pinned older actual firmware refuses the new stored format.
assert.ok(process.argv[2]&&process.argv[3],'old-firmware oracle is mandatory');
assert.equal(fs.readFileSync(process.argv[3],'utf8').trim(),'ad3237199ef275abda5d388b849c5a0f8771dfd0');
const oldProcessor=new Processor();await oldProcessor.onMessage({type:'load',wasm:fs.readFileSync(process.argv[2]),store:saved});render(oldProcessor,175);assert.equal(snap(oldProcessor)[2],1);assert.deepEqual(objects(oldProcessor),saved);
console.log(JSON.stringify({test:'expanded rates actual WASM production worklet',wasmSha256:crypto.createHash('sha256').update(wasm).digest('hex'),migration32x25:true,all21RatesAll25Lanes:true,format6SaveReopen:true,policyPersistence:true,undoRedo:true,actualIngressFamilySweepsBothDirections:true,olderFirmwareRefusesFormat6:true,physicalMidi:false}));
