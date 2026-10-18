// SPDX-License-Identifier: GPL-3.0-only
// Production worklet + actual shared C WASM. The oracle is a separately built,
// pinned pre-lane source tree; no substitute or implicit baseline is accepted.
import fs from 'node:fs';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const PIN='ad3237199ef275abda5d388b849c5a0f8771dfd0';
assert.ok(process.argv[2]&&process.argv[3]&&process.argv[4]);assert.equal(fs.readFileSync(process.argv[4],'utf8').trim(),PIN);
const candidate=fs.readFileSync(process.argv[2]),reference=fs.readFileSync(process.argv[3]);
let Processor,request=0;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
function render(p,n=1){const samples=new Float32Array(n*256);for(let i=0;i<n;i++){currentTime+=128/44100;const out=[[new Float32Array(128),new Float32Array(128)]];p.process([],out);for(let j=0;j<128;j++){samples[i*256+j*2]=out[0][0][j];samples[i*256+j*2+1]=out[0][1][j];}}return samples;}
async function open(bytes=candidate,store){const p=new Processor();await p.onMessage({type:'load',wasm:bytes,store});render(p,175);return p;}
function snap(p,section=0,target=0,voice=0){const ptr=p.ex.web_variant_snapshot(section,target,voice);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return{revision:v[2],values:Array.from(v.subarray(4,4+v[3]))};}
async function cmd(p,op,target=0,voice=0,index=0,value=0,result=[3,4,5,6,7].includes(op)?0:1){const id=++request;await p.onMessage({type:'variant-command',abi:1,request:id,op,target,voice,index,value});const reply=p.messages.findLast(m=>m.type==='variant-reply'&&m.request===id);assert.ok(reply);assert.equal(reply.result,result);return reply;}
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n))];}).filter(([,v])=>v));
const STREAM=[2,3,5,6];
function crc32(raw){let c=0xffffffff;for(const b of raw){c^=b;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return(c^0xffffffff)>>>0;}
function unpack(store){const h=store[2],length=h.readUInt32LE(4),packed=h.readUInt32LE(12),data=Buffer.concat(STREAM.map((id,i)=>store[id].subarray(i?4:16))).subarray(0,packed),out=Buffer.alloc(length);let i=0,n=0;while(i<data.length){const c=data[i++];if(c<128){data.copy(out,n,i,i+c+1);i+=c+1;n+=c+1;}else{const count=(c&127)+3,d=data.readUInt16LE(i)+1;i+=2;for(let k=0;k<count;k++){out[n]=out[n-d];n++;}}}assert.equal(n,length);assert.equal(crc32(out),h.readUInt32LE(8));return out;}
function pack(raw){const result=[],seen=new Map();let i=0,literal=[];const flush=()=>{while(literal.length){const part=literal.splice(0,128);result.push(part.length-1,...part);}};while(i<raw.length){let best=0,dist=0;if(i+2<raw.length){const key=raw.readUIntLE(i,3),previous=seen.get(key);seen.set(key,i);for(const pos of[previous,i-1]){if(pos===undefined||pos<0)continue;let n=0;while(n<130&&i+n<raw.length&&raw[pos+n]===raw[i+n])n++;if(n>best){best=n;dist=i-pos;}}}if(best>=3){flush();result.push(128|(best-3),(dist-1)&255,(dist-1)>>8);i+=best;}else{literal.push(raw[i++]);if(literal.length===128)flush();}}flush();return Buffer.from(result);}
function encode(base,raw,format=2){const store=Object.fromEntries(Object.entries(base).map(([id,b])=>[id,Buffer.from(b)])),packed=pack(raw);assert.ok(packed.length<=15332);let offset=0;for(let k=0;k<4;k++){const start=k?4:16,size=Math.min(3840-start,packed.length-offset),b=Buffer.alloc(start+size);b.writeUInt32LE(format);if(!k){b.writeUInt32LE(raw.length,4);b.writeUInt32LE(crc32(raw),8);b.writeUInt32LE(packed.length,12);}packed.copy(b,start,offset,offset+size);offset+=size;store[STREAM[k]]=b;}assert.equal(offset,packed.length);return store;}
const p=await open();assert.equal(snap(p).values[0],3583);assert.equal(snap(p).values[5],5);assert.deepEqual(snap(p,11).values,[0,0,0,0,25,8,64]);
await cmd(p,7);const oldStore=objects(p),raw=unpack(oldStore);assert.equal(oldStore[2].readUInt32LE(0),2);
// Independent source oracle uses codec-matched assets; original-PCM codec loss is checked separately.
for(let kit=0;kit<2;kit++){for(let voice=0;voice<11;voice++){let mask=0;for(let step=0;step<16;step++)if((step*3+voice+kit)%7<2)mask|=1<<step;raw.writeUInt32LE(mask>>>0,kit*100+voice*8);}raw[kit*100+96]=16;}
for(let bass=0;bass<2;bass++)for(let step=0;step<16;step++){const off=200+bass*144+step*2;raw[off]=36+(step*5+bass*7)%36;raw[off+1]=step%7===2?2:step%9===0?0:1|(step%3===0?4:0)|(step%5===0?8:0);}
raw.writeUInt32LE(0xa529,488);raw[524]=37;
const fixture=encode(oldStore,raw),original=await open(reference,fixture),dry=await open(candidate,fixture);
original.ex.web_seq_play(1);dry.ex.web_seq_play(1);render(original,5);render(dry,5);
const oldPcm=render(original,1024),newPcm=render(dry,1024);assert.ok(Buffer.from(oldPcm.buffer).equals(Buffer.from(newPcm.buffer)),'independent default-Off patterned PCM exact');assert.ok(newPcm.some(x=>Math.abs(x)>1e-6));
await cmd(p,19,0,0,0,1);render(p,4);assert.deepEqual(snap(p,11).values,[1,1,1,0,25,8,64]);
const expected=Array.from({length:32},(_,pat)=>Array.from({length:25},(_,lane)=>[1+(pat*7+lane*3)%64,(pat+lane)%8]));
await cmd(p,3);for(let pat=0;pat<32;pat++)for(let lane=0;lane<25;lane++){await cmd(p,18,pat,lane,0,expected[pat][lane][0]);await cmd(p,18,pat,lane,1,expected[pat][lane][1]);if(lane%8===7)render(p);}
render(p,4);for(let pat=0;pat<32;pat++)for(let lane=0;lane<25;lane++)assert.deepEqual(snap(p,10,pat,lane).values.slice(1,3),expected[pat][lane]);
for(let step=0;step<64;step++)await cmd(p,18,31,24,2,step|((step%3===0?1:0)<<6)|((step%9)<<7));await cmd(p,4);render(p,4);
for(let page=0;page<2;page++){const v=snap(p,12,31,page).values;for(let s=0;s<32;s++){const step=page*32+s;assert.equal((v[0]>>>s)&1,step%3===0?1:0);assert.equal((v[1+Math.floor(s/8)]>>>((s%8)*4))&15,step%9);}}
await cmd(p,3);await cmd(p,18,1,7,0,64);await cmd(p,4);render(p,4);await cmd(p,5);render(p,4);assert.equal(snap(p,10,1,7).values[1],expected[1][7][0]);await cmd(p,6);render(p,4);assert.equal(snap(p,10,1,7).values[1],64);
await cmd(p,19,0,0,0,0);render(p,4);assert.deepEqual(snap(p,11).values.slice(0,4),[1,0,0,0]);await cmd(p,7);const saved=objects(p);assert.equal(saved[2].readUInt32LE(0),5);const reopened=await open(candidate,saved);assert.deepEqual(snap(reopened,11).values.slice(0,4),[1,0,0,0]);assert.equal(snap(reopened,10,1,7).values[1],64);
await cmd(reopened,19,0,0,0,1);render(reopened,4);for(let pat=0;pat<32;pat++)for(let lane=0;lane<25;lane++)assert.deepEqual(snap(reopened,10,pat,lane).values.slice(1,3),pat===1&&lane===7?[64,expected[pat][lane][1]]:expected[pat][lane]);
const before=snap(reopened).revision,writes=reopened.ex.web_store_writes();for(const args of [[32,0,0,1],[0,25,0,1],[0,0,0,0],[0,0,1,8],[0,0,2,64],[0,24,2,1152]])await cmd(reopened,18,...args,-1);assert.equal(snap(reopened).revision,before);assert.equal(reopened.ex.web_store_writes(),writes);
const corrupted=Object.fromEntries(Object.entries(saved).map(([id,b])=>[id,Buffer.from(b)]));corrupted[1][4]^=1;const protectedP=await open(candidate,corrupted);assert.equal(snap(protectedP).values[2],1);await cmd(protectedP,18,0,0,0,1,-4);assert.deepEqual(objects(protectedP),corrupted);
console.log(JSON.stringify({realProductionWorklet:true,pinnedCommit:PIN,candidateSha256:crypto.createHash('sha256').update(candidate).digest('hex'),pinnedOffPatternedPcm:true,codecMatchedOracleInputs:true,patterns32Lanes25Rates8:true,break64:true,settingsPreservingModes:true,undoRedoReopen:true,protectedCrossObjectMismatch:true}));
