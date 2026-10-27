// Production worklet, physical panel input and actual candidate WASM.
import fs from 'node:fs';import assert from 'node:assert/strict';
let Processor,request=0;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,c)=>{Processor=c;};await import('../../web/emu/worklet.js');
const wasm=fs.readFileSync('build/emu/x0x.wasm');
function render(p,n=25){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}}
async function open(bytes=wasm,store){const p=new Processor();await p.onMessage({type:'load',wasm:bytes,store});render(p,175);return p;}
function snap(p,section=10,target=0,voice=0){const ptr=p.ex.web_variant_snapshot(section,target,voice);assert(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return Array.from(v.subarray(4,4+v[3]));}
function tap(p,b){p.ex.web_buttons(1<<b);render(p);p.ex.web_buttons(0);render(p);}
function turn(p,enc,n){for(let i=0;i<Math.abs(n);i++){p.ex.web_enc(enc,Math.sign(n));render(p);}}
function screen(p,name){fs.writeFileSync(`build/straight-time/${name}.rgb565`,new Uint8Array(p.mem.buffer,p.ex.web_fb(),240*240*2));}
async function cmd(p,op,target=0,voice=0,index=0,value=0){const id=++request;await p.onMessage({type:'variant-command',abi:1,request:id,op,target,voice,index,value});const reply=p.messages.findLast(m=>m.type==='variant-reply'&&m.request===id);assert(reply&&reply.result>=0,JSON.stringify(reply));render(p);}
const objects=p=>Object.fromEntries(Array.from({length:p.ex.web_nobj()},(_,i)=>{const n=p.ex.web_store_len(i);return[i,n<0?null:Buffer.from(new Uint8Array(p.mem.buffer,p.ex.web_store(i),n))];}).filter(([,v])=>v));
const p=await open();tap(p,5);turn(p,0,JSON.parse(fs.readFileSync('build/straight-time/navigation.json')).gloMoves);turn(p,1,1);screen(p,'wasm-glo-straight');
assert.equal(snap(p)[13],1);assert.equal(snap(p)[14],0);assert.equal(snap(p,11)[5],29);
tap(p,6);tap(p,9); // HOME, SEQ; drum sequence RATE is K2.
const grid=[0,5,4,9,8,21,22,23,24,25,26,27,28];
for(let i=1;i<grid.length;i++){turn(p,4,1);assert.equal(snap(p)[2],grid[i]);}
screen(p,'wasm-seq-32bars');
for(let i=grid.length-2;i>=0;i--){turn(p,4,-1);assert.equal(snap(p)[2],grid[i]);}
// EDIT's final page is STRIP SYNC followed by STRIP DEFAULTS. Scan pages with
// real SELECT movement until a K1 turn changes the local override snapshot.
tap(p,4);let found=false;
for(let i=0;i<24;i++){
 turn(p,3,1);
 if(snap(p)[14]===1){found=true;screen(p,'wasm-strip-straight');break;}
 turn(p,3,-1);turn(p,0,1);
}
assert(found,'strip override must be reachable through EDIT pages');
turn(p,3,1);assert.equal(snap(p)[14],2);screen(p,'wasm-strip-triplet');
await cmd(p,7);const saved=objects(p);const reopened=await open(wasm,saved);
assert.equal(snap(reopened)[13],1);assert.equal(snap(reopened)[14],2);
// The previous LAN2-capable actual WASM must refuse LAN3 and preserve its bytes.
const old=await open(fs.readFileSync('../timing-calibration-candidate-20261025/build/emu/x0x.wasm'),saved);
assert.equal(snap(old,0)[2],1);assert.deepEqual(objects(old),saved);
assert(p.messages.every(m=>m.type!=='error'));
console.log('STRAIGHT_TIME_WASM_PASS: GLO and strip controls through physical input, exact 13-stop sweep both directions, saved reload, previous LAN2 firmware refuses LAN3');
