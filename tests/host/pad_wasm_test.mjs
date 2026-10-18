// SPDX-License-Identifier: GPL-3.0-only
// Production worklet/WASM, real bounded C producer, independent accepted bass PCM.
import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const PIN='b643f1775215cdf0c73e0e15812dbf1783107865';
assert.ok(process.argv[2]&&process.argv[3]&&process.argv[4],'candidate plus independent pinned bass oracle required');
assert.equal(fs.readFileSync(process.argv[4],'utf8').trim(),PIN);assert.notEqual(fs.realpathSync(process.argv[2]),fs.realpathSync(process.argv[3]));
assert.ok(path.resolve(process.argv[3]).includes('pad-oracle'));const candidate=fs.readFileSync(process.argv[2]),reference=fs.readFileSync(process.argv[3]);
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');assert.notEqual(hash(candidate),hash(reference));
let Processor,request=0;globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
function render(p,n=1,accept=true){const audio=new Float32Array(n*256);p.accepted??=[];for(let i=0;i<n;i++){currentTime+=128/44100;const out=[[new Float32Array(128),new Float32Array(128)]];p.process([],out);for(let k=0;k<128;k++){audio[i*256+k*2]=out[0][0][k];audio[i*256+k*2+1]=out[0][1][k];}
 if(accept&&p.midi?.flight){const f=p.midi.flight,b=p.messages.findLast(m=>m.type==='midi-batch'&&m.batch===f.batch);assert.ok(b.packets.length<=16);p.accepted.push(...b.packets);p.midi.message({type:'midi-ack',protocol:0x00020001,route:p.midi.route,batch:f.batch,epoch:f.epoch,prefix:f.count});}}
 return audio;}
async function open(store,wasm=candidate){const p=new Processor();await p.onMessage({type:'load',wasm,store});render(p,175);return p;}
function snap(p,section=0,target=0){const ptr=p.ex.web_variant_snapshot(section,target,0);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return{revision:v[2],values:Array.from(v.subarray(4,4+v[3]))};}
async function cmd(p,op,target=0,index=0,value=0,voice=0){await p.onMessage({type:'variant-command',abi:1,op,request:++request,target,voice,index,value});const r=p.messages.findLast(m=>m.type==='variant-reply'&&m.request===request);assert.ok(r);return r;}
async function good(...args){const r=await cmd(...args);assert.equal(r.result,[3,4,5,6,7,11].includes(args[1])?0:1);return r;}
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n))];}).filter(([,b])=>b));
const pcmEqual=(a,b,label)=>assert.ok(Buffer.from(a.buffer).equals(Buffer.from(b.buffer)),label);
async function route(p){await p.onMessage({type:'midi-route',protocol:0x00020001,route:1,ready:true,request:1});render(p,3);assert.equal(snap(p,8).values[15],1);}
{
 const replaced=await open();await route(replaced);const old=replaced.ex,bridge=replaced.midi;
 await assert.rejects(()=>replaced.onMessage({type:'load',wasm:candidate}),/complete cleanup/);
 assert.equal(replaced.ex,old);assert.equal(replaced.midi,bridge);assert.equal(bridge.ready,true);
 await replaced.onMessage({type:'midi-route',protocol:0x00020001,route:1,ready:false,request:2});render(replaced,4);
 assert.equal(bridge.ready,false);assert.equal(old.web_midi_pending(),0);
 const previousCaps=replaced.messages.filter(m=>m.type==='midi-output-capability'&&m.supported).length;
 await replaced.onMessage({type:'load',wasm:candidate});render(replaced,4);
 assert.notEqual(replaced.ex,old);assert.notEqual(replaced.midi,bridge);assert.equal(replaced.midi.ready,false);
 assert.equal(replaced.messages.filter(m=>m.type==='midi-output-capability'&&m.supported).length,previousCaps+1);
 const instantiate=WebAssembly.instantiate;let release,entered;
 const gate=new Promise(resolve=>release=resolve),started=new Promise(resolve=>entered=resolve);
 WebAssembly.instantiate=async(...args)=>{entered();await gate;return instantiate(...args);};
 try{
  const loading=replaced.onMessage({type:'load',wasm:candidate});await started;
  assert.equal(replaced.loading,true);const producer=replaced.ex;
  await replaced.onMessage({type:'midi-route',protocol:0x00020001,route:2,ready:true,request:3});
  const reply=replaced.messages.findLast(m=>m.type==='midi-route-reply'&&m.request===3);
  assert.equal(reply.result,-2);assert.equal(replaced.midi.ready,false);assert.equal(snap(replaced,8).values[15],0);
  release();await loading;assert.notEqual(replaced.ex,producer);render(replaced,4);assert.equal(replaced.midi.ready,false);
 }finally{release();WebAssembly.instantiate=instantiate;}
}
function start(p,accept=true){p.ex.web_seq_play(1);render(p,4,accept);assert.equal(p.ex.web_seq_playing(),1);}
function stop(p,accept=true){p.ex.web_seq_play(0);render(p,4,accept);assert.equal(p.ex.web_seq_playing(),0);}
const STREAM=[2,3,5,6],LEGACY=29572,VARIANT=LEGACY,ARR=VARIANT+208,SONG=528*32;
function crc32(bytes){let c=0xffffffff;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return(c^0xffffffff)>>>0;}
function unpack(store){const h=store[2],length=h.readUInt32LE(4),packed=h.readUInt32LE(12);const data=Buffer.concat(STREAM.map((id,i)=>store[id].subarray(i?4:16))).subarray(0,packed);const out=Buffer.alloc(length);let i=0,n=0;while(i<data.length){const c=data[i++];if(c<128){data.copy(out,n,i,i+c+1);i+=c+1;n+=c+1;}else{const count=(c&127)+3,d=data.readUInt16LE(i)+1;i+=2;for(let k=0;k<count;k++){out[n]=out[n-d];n++;}}}assert.equal(n,length);assert.equal(crc32(out),h.readUInt32LE(8));return out;}
// Independent simple LZ encoder, using latest exact three-byte match and runs.
function pack(raw){const result=[],seen=new Map();let i=0,literal=[];const flush=()=>{while(literal.length){const part=literal.splice(0,128);result.push(part.length-1,...part);}};while(i<raw.length){let best=0,dist=0;if(i+2<raw.length){const key=raw.readUIntLE(i,3),previous=seen.get(key);seen.set(key,i);for(const pos of [previous,i-1]){if(pos===undefined||pos<0)continue;let n=0;while(n<130&&i+n<raw.length&&raw[pos+n]===raw[i+n])n++;if(n>best){best=n;dist=i-pos;}}}if(best>=3){flush();result.push(128|(best-3),(dist-1)&255,(dist-1)>>8);i+=best;}else{literal.push(raw[i++]);if(literal.length===128)flush();}}flush();return Buffer.from(result);}
function encode(base,raw,format=4){const store=Object.fromEntries(Object.entries(base).map(([id,b])=>[id,Buffer.from(b)])),packed=pack(raw),max=3840;assert.ok(packed.length<=4*(max-4)-12);let offset=0;for(let k=0;k<4;k++){const size=Math.min(max-(k?4:16),packed.length-offset),b=Buffer.alloc((k?4:16)+size);b.writeUInt32LE(format);if(!k){b.writeUInt32LE(raw.length,4);b.writeUInt32LE(crc32(raw),8);b.writeUInt32LE(packed.length,12);}packed.copy(b,k?4:16,offset,offset+size);offset+=size;store[STREAM[k]]=b;}assert.equal(offset,packed.length);return store;}

const initial=await open();assert.equal(snap(initial).values[0],3583);assert.equal(snap(initial).values[5],5);assert.equal(snap(initial,8).values.length,16);assert.equal(snap(initial,8).values[15],0);
await good(initial,10,0,0,4);await good(initial,9,0,0,1);render(initial,4);await good(initial,7);
const template=objects(initial),templateRaw=unpack(template);assert.equal(templateRaw.length,LEGACY+412);
function fixture({schema=4,pad=8,notes=true,degree=1,mode=0,key=0,parent=255,root=60,low=48,high=84,follow=[1,2]}={}){
 const raw=Buffer.from(templateRaw);for(let p=0;p<32;p++){const off=p*528;raw.fill(0,off,off+200);raw.writeUInt32LE(0,off+488);for(let b=0;b<2;b++){const o=off+200+b*144;raw.fill(0,o,o+128);raw[o]=b?67:64;raw[o+1]=13;raw[o+128]=1;raw[o+129]=0;raw[o+130]=0;raw[o+131]=24;}}
 raw.writeUInt16LE(schema,VARIANT+4);raw[ARR]=1|(follow[0]<<1)|(follow[1]<<3)|((schema>=4&&notes)?32:0);raw[ARR+1]=key;raw[ARR+2]=mode;raw[ARR+3]=parent;
 raw[ARR+8]=pad;raw[ARR+9]=root;raw[ARR+10]=low;raw[ARR+11]=high;raw.fill(0,ARR+12,ARR+204);for(let i=0;i<4;i++)raw[ARR+12+i]=[degree,6,4,5][i];
 const store=encode(template,raw);store[0]=Buffer.from(store[0]);store[0][15]=notes?1:0;store[0].writeUInt16LE(1200,16);store[0][20]=1;return{raw,store};
}
// Off pads preserve internal PCM against a fresh, pinned accepted bass build.
for(const enabled of [false,true]){const f=fixture({schema:3,pad:255,notes:false});if(!enabled){f.raw[ARR]=0;f.store=encode(f.store,f.raw);}const a=await open(f.store),b=await open(f.store,reference);start(a);start(b);pcmEqual(render(a,800),render(b,800),'independent accepted bass PCM, pads disabled');}
const controls=await open(fixture({pad:255,notes:false}).store),before=snap(controls,8);
for(const args of [[14,1,0,8],[14,0,4,8],[14,0,0,16],[14,0,0,9],[14,0,1,128],[14,0,2,85],[15,1,0,1],[15,0,1,1],[15,0,0,2]]){assert.equal((await cmd(controls,...args)).result,-1);assert.deepEqual(snap(controls,8),before);}
await good(controls,3);await good(controls,14,0,0,8);await good(controls,15,0,0,1);await good(controls,4);render(controls,4);assert.equal(snap(controls,8).values[4],1);
await good(controls,5);render(controls,4);assert.deepEqual(snap(controls,8).values.slice(0,5),[255,60,48,84,0]);await good(controls,6);render(controls,4);assert.equal(snap(controls,8).values[0],8);assert.equal(snap(controls,8).values[4],1);
await good(controls,7);const reopened=await open(objects(controls));assert.deepEqual(snap(reopened,8).values.slice(0,5),[8,60,48,84,1]);assert.equal(snap(reopened,8).values[15],0,'route is never restored from storage');
controls.ex.web_seq_play(1);assert.equal((await cmd(controls,14,0,1,72)).result,-3);assert.equal((await cmd(controls,15,0,0,0)).result,-3);render(controls);stop(controls);
let accepted=0;for(let i=0;i<1100;i++){const old=snap(controls,8),info=snap(controls);const r=await cmd(controls,14,0,1,i%128);if(r.result===-3){assert.deepEqual(snap(controls,8),old);assert.deepEqual(snap(controls),info);break;}assert.equal(r.result,1);accepted++;}assert.equal(accepted,1024);render(controls,4);
// Production packets, sustained unchanged triad and C-Am-F-G current latch.
const music=await open(fixture().store);await route(music);start(music);assert.deepEqual(snap(music,8).values.slice(7,14),[8,60,64,67,60,64,67]);
const padOns=p=>p.accepted.filter(x=>((x>>>8)&255)===0x98&&((x>>>24)&127)).map(x=>(x>>>16)&127);
assert.deepEqual(padOns(music),[60,64,67]);const seen=[padOns(music)],roots=[57,65,55];
for(const root of roots){const old=snap(music,5).values[3];let guard=0;while(snap(music,5).values[3]===old&&guard++<900)render(music);assert.ok(guard<900);assert.equal(snap(music,8).values[8],root);seen.push(snap(music,8).values.slice(8,11));}
assert.deepEqual(seen,[[60,64,67],[57,60,64],[65,69,72],[55,59,62]]);stop(music);assert.deepEqual(snap(music,8).values.slice(7,11),[255,255,255,255]);
// Posted in-flight batch cancellation before partial ACK retains Offs/clock and no replay.
const queued=await open(fixture().store);await route(queued);start(queued,false);assert.ok(queued.midi.flight);const flight={...queued.midi.flight};const oldBatch=queued.messages.findLast(m=>m.type==='midi-batch'&&m.batch===flight.batch);
const oldEpoch=queued.ex.web_midi_epoch();stop(queued,false);assert.ok(queued.ex.web_midi_epoch()!==oldEpoch);for(let i=0;i<flight.count;i++){const now=queued.ex.web_midi_packet(i)>>>0;if(((now>>>8)&240)===144&&((now>>>24)&127))assert.ok(now&0x80000000);}
queued.midi.message({type:'midi-ack',protocol:0x00020001,route:1,batch:flight.batch,epoch:flight.epoch,prefix:1});assert.equal(queued.midi.latched,flight.count-1);assert.ok(queued.ex.web_midi_pending()>=oldBatch.packets.length-1+3,'only acknowledged prefix consumed; owed triad Offs retained');
queued.midi.message({type:'midi-drain',protocol:0x00020001,route:1});render(queued,4);assert.equal(queued.ex.web_seq_playing(),0);assert.equal(snap(queued,8).values[7],255);
// Tagged PLAY is acknowledged only after scan and subsequent actual render.
const ticket=0xffffffff;reopened.ex.web_buttons_tagged(1<<10,ticket);assert.notEqual(reopened.ex.web_input_ticket()>>>0,ticket);
let wait=0;while((reopened.ex.web_input_ticket()>>>0)!==ticket&&wait++<30)render(reopened);assert.ok(wait<30);assert.equal(reopened.ex.web_seq_playing(),1);
reopened.ex.web_buttons_tagged(0,1);wait=0;while((reopened.ex.web_input_ticket()>>>0)!==1&&wait++<30)render(reopened);assert.ok(wait<30);
// Old schema pad bytes never enable, and malformed new flags/fields protect storage.
for(const schema of [2,3]){const f=fixture({schema,pad:8,follow:schema===2?[0,0]:[1,2]});const p=await open(f.store);assert.equal(snap(p,8).values[0],255);assert.equal(snap(p,8).values[4],1);assert.equal(snap(p).values[5],5);}
for(const bad of ['flags','oldFlags','channel','range','hint','schema','crc']){const f=fixture();if(bad==='flags')f.raw[ARR]|=64;if(bad==='oldFlags'){f.raw.writeUInt16LE(3,VARIANT+4);f.raw[ARR]|=32;}if(bad==='channel')f.raw[ARR+8]=16;if(bad==='range')f.raw[ARR+10]=85;if(bad==='hint')f.raw[ARR+9]=128;if(bad==='schema')f.raw.writeUInt16LE(6,VARIANT+4);const store=encode(f.store,f.raw);if(bad==='crc')store[2][8]^=1;const p=await open(store);assert.equal(snap(p).values[2],1);assert.equal((await cmd(p,14)).result,-4);assert.equal((await cmd(p,15)).result,-4);assert.deepEqual(objects(p),store);}
console.log(JSON.stringify({realProductionWorkletWasm:true,candidateSha256:hash(candidate),referenceCommit:PIN,referenceSha256:hash(reference),independentPadDisabledPcm:true,commands14and15:true,schema4Same412:true,outputUndoRedo:true,queuePressure:accepted,CAmFG:true,atomicTriadPackets:true,partialAckCancellation:true,taggedInput:true,oldPadDisabledMigration:true,protectedMalformed:true,physicalReceiver:'pending'}));
