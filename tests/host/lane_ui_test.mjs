// SPDX-License-Identifier: GPL-3.0-only
// Production DOM controller + production AudioWorklet bridge; fake bounded C ABI.
// Actual C/WASM timing behavior is tested separately by the lane engine suite.
import fs from 'node:fs';import assert from 'node:assert/strict';
class Element extends EventTarget{
 constructor(){super();this.value='';this.disabled=false;this.hidden=false;this.children=[];this.textContent='';this.attributes={};this.classes=new Set();this.classList={toggle:(k,on)=>on?this.classes.add(k):this.classes.delete(k)};}
 append(...items){this.children.push(...items);}replaceChildren(...items){this.children=items;this.value=items[0]?.value??'';}setAttribute(k,v){this.attributes[k]=v;}
}
const html=fs.readFileSync('web/emu/lane-panel.html','utf8'),elements=new Map();
for(const tag of html.matchAll(/<[^>]+\bid="([^"]+)"[^>]*>/g))elements.set(tag[1],new Element());
globalThis.document={getElementById:id=>{assert.ok(elements.has(id),id);return elements.get(id);},createElement:()=>new Element()};
globalThis.window=new EventTarget();globalThis.CustomEvent=class extends Event{constructor(type,init){super(type);this.detail=init.detail;}};
let permissions=0;Object.defineProperty(globalThis,'navigator',{configurable:true,value:{requestMIDIAccess:()=>{permissions++;throw Error('Must not request MIDI');}}});
const sent=[],out=[],commands=[],snapshots=[];
window.fm1SliceSend=m=>sent.push(m);
let Worklet;globalThis.AudioWorkletProcessor=class{constructor(){this.port={postMessage:m=>out.push(m)};}};globalThis.registerProcessor=(name,C)=>{assert.equal(name,'x0x');Worklet=C;};
globalThis.currentTime=0;await import('../../web/emu/worklet.js');await import('../../web/emu/lane-ui.js');
const worker=new Worklet(),memory=new WebAssembly.Memory({initial:3});worker.mem=memory;worker.sentBlits=worker.sentWrites=0;
let caps=1535,protectedProject=0,playing=false,revision=1,written=0,read=0,initialized=0,desiredMode=0,effectiveMode=0,modePending=0,localPending=0,effectiveRate=0;
let lengths=new Map(),rates=new Map(),gates=[0x80000001,0x80000001],slices=[[0x87654321,0,0,0x80000000],[0x12345678,0,0,0x80000000]];
let rejectOp=null,rejectResult=-3,gesture=false;
const snapshot=(section,target,voice)=>{
 snapshots.push([section,target,voice]);let values;
 if(section===0)values=[caps,1,protectedProject,written,read,5,25,0];
 else if(section===11)values=[initialized,desiredMode,effectiveMode,modePending,25,8,64];
 else if(section===10)values=[initialized,lengths.get(`${target}:${voice}`)??16,rates.get(`${target}:${voice}`)??0,3,playing?2:255,7,effectiveRate,effectiveMode,localPending];
 else if(section===12&&initialized)values=[gates[voice],...slices[voice]];
 else return 0;
 const p=64,v=new Uint32Array(memory.buffer,p,20);v.fill(0);v.set([1,section,revision,values.length,...values]);return p;
};
worker.ex={memory,web_variant_snapshot:snapshot,web_variant_command:(abi,op,request,target,voice,index,value)=>{
 commands.push({abi,op,request,target,voice,index,value});let result=0;
 if(op===rejectOp)result=rejectResult;
 else if(op===3)gesture=true;
 else if(op===4)gesture=false;
 else if(op===18){assert.equal(gesture,true,'value follows acknowledged BEGIN');result=1;written=(written+1)>>>0;revision=(revision+1)>>>0;
  if(index===0)lengths.set(`${target}:${voice}`,value);else if(index===1)rates.set(`${target}:${voice}`,value);else{const step=value&63,p=step>>>5,i=step&31;gates[p]=((gates[p]&~(1<<i))|(((value>>>6)&1)<<i))>>>0;const group=i>>>3,shift=(i&7)*4;slices[p][group]=((slices[p][group]&~(15<<shift))|((value>>>7)<<shift))>>>0;}
 }else if(op===19){assert.equal(gesture,true);initialized=1;desiredMode=value;modePending=playing?1:0;if(!playing)effectiveMode=value;result=1;written=(written+1)>>>0;revision=(revision+1)>>>0;}
 else revision=(revision+1)>>>0;
 const p=256,v=new DataView(memory.buffer,p,16);v.setUint32(0,request,true);v.setUint32(4,revision,true);v.setUint32(8,written,true);v.setInt32(12,result,true);return p;
},web_blits:()=>0,web_lit_buttons:()=>0,web_lit_keys:()=>0,web_dim_keys:()=>0,web_store_writes:()=>0,
 web_sync_state:()=>0,web_seq_bpm:()=>120,web_seq_playing:()=>playing?1:0,web_seq_beat:()=>0};
const $=id=>elements.get(id),fire=(id,event='click')=>$(id).dispatchEvent(new Event(event));
const dispatch=m=>window.dispatchEvent(new CustomEvent('fm1-marked',{detail:m}));
const frame=()=>{worker.publish();const m=out.pop();assert.equal(m.type,'frame');dispatch(m);return m;};
async function pump(){let count=0;while(sent.length){assert.ok(count++<20,'bounded message exchange');await worker.onMessage(sent.shift());while(out.length)dispatch(out.shift());}}
const settle=async()=>{await pump();read=written;frame();};
const select=async(id,value)=>{$(id).value=String(value);fire(id,'change');await pump();frame();};
const editLength=value=>{$('lane-length').value=String(value);fire('lane-length','input');fire('lane-length','change');};

assert.equal($('lane-timing').disabled,true);assert.equal(permissions,0);
window.dispatchEvent(new Event('fm1-ready'));await pump();caps=511;frame();assert.equal($('lane-mode').disabled,true);assert.match($('lane-availability').textContent,/unavailable/);assert.equal(worker.laneView.viewRevision,1);
caps=1535;playing=true;frame();assert.equal($('lane-mode').disabled,true,'first initialization requires stopped');
playing=false;frame();assert.equal($('lane-mode').disabled,false);assert.equal($('lane-timing').disabled,true);
fire('lane-mode');assert.equal(sent.length,1);assert.equal(sent[0].op,3,'BEGIN is acknowledged before value dispatch');await pump();assert.deepEqual(commands.slice(-3).map(m=>m.op),[3,19,4]);assert.equal(gesture,false);
frame();assert.equal($('lane-mode').disabled,true,'queued mode cannot unlock on revision alone');read=written;frame();assert.equal($('lane-timing').disabled,false);assert.equal($('lane-mode').disabled,false);
assert.equal($('lane-sound').children.length,25);assert.equal($('lane-pattern').children.length,32);assert.equal($('lane-rate').children.length,8);assert.equal($('lane-grid').children.length,32);
for(let sound=0;sound<25;sound++){await select('lane-sound',sound);editLength(16);await settle();assert.equal(commands.at(-2).voice,sound);assert.equal(commands.at(-2).target,0);}

await select('lane-pattern',31);await select('lane-sound',23);assert.match($('lane-position').textContent,/Editing pattern 31 · 303 B · Playing pattern 3/);
assert.match($('lane-timing-state').textContent,/Prepared settings/);
$('lane-length').value='6';fire('lane-length','input');frame();assert.equal($('lane-length').value,'6','draft survives frames');assert.equal($('lane-selection').disabled,true);assert.equal($('lane-rate').disabled,true);assert.equal($('lane-save').disabled,true);
$('lane-pattern').value='2';fire('lane-pattern','change');assert.equal($('lane-pattern').value,'31','selection cannot steal a draft');fire('lane-length','change');
await pump();assert.deepEqual(commands.slice(-3).map(m=>m.op),[3,18,4]);assert.deepEqual([commands.at(-2).target,commands.at(-2).voice,commands.at(-2).index,commands.at(-2).value],[31,23,0,6]);
frame();assert.equal($('lane-timing').disabled,true);read=written;frame();assert.equal($('lane-timing').disabled,false);assert.equal($('lane-length').value,'6');
for(const value of ['',0,65,1.5,'abc']){const n=commands.length;editLength(value);await pump();assert.equal(commands.length,n);assert.match($('lane-status').textContent,/whole number/);assert.equal($('lane-length').value,'6');}
for(const value of [1,64]){editLength(value);await settle();assert.equal($('lane-length').value,String(value));}
editLength(32);await pump();read=written;const oldRevision=revision;revision=(revision-1)>>>0;frame();assert.equal($('lane-timing').disabled,true,'applied ticket still needs the acknowledged revision');revision=oldRevision;frame();assert.equal($('lane-timing').disabled,false);
for(let rate=0;rate<8;rate++){$('lane-rate').value=String(rate);fire('lane-rate','change');await settle();assert.equal(commands.at(-2).index,1);assert.equal(commands.at(-2).value,rate);assert.equal($('lane-rate').value,String(rate));}
// Actual lane controller must recover from another panel's open C gesture. It
// waits for BEGIN ACK, so refusal sends no value/END and needs no suffix drain.
await worker.onMessage({type:'variant-command',abi:1,op:3,request:42,target:0,voice:0,index:0,value:0});dispatch(out.shift());
const ownedCalls=commands.length;editLength(33);await pump();assert.equal(commands.length,ownedCalls);assert.match($('lane-status').textContent,/busy/);assert.equal(worker.editOwner,'variant-0');assert.equal(gesture,true);
await worker.onMessage({type:'variant-command',abi:1,op:4,request:43,target:0,voice:0,index:0,value:0});dispatch(out.shift());
editLength(34);await settle();assert.equal($('lane-length').value,'34');assert.equal(worker.editOwner,null);assert.equal(gesture,false);

// Replies in other panels' namespaces, duplicates and malformed replies cannot
// advance a lane gesture even when their request scalar happens to be equal.
editLength(12);const begin=sent[0];dispatch({type:'variant-reply',request:begin.request,result:0,revision,ticket:written});assert.equal(sent.length,1);
dispatch({type:'lane-reply',request:begin.request,result:0,revision:-1,ticket:written});assert.equal(sent.length,1);
dispatch({type:'lane-reply',request:begin.request,result:2,revision,ticket:written});assert.equal(sent.length,1);
await pump();read=written;const saved=frame();assert.equal($('lane-length').value,'12');
dispatch({type:'lane-reply',request:begin.request,result:0,revision,ticket:written});assert.equal(sent.length,0,'duplicate cannot reopen gesture');
// Selection revision and target must both match; a delayed frame never enables
// editing the previous sound under the newly selected label.
$('lane-sound').value='0';fire('lane-sound','change');dispatch(saved);assert.equal($('lane-timing').disabled,true);await pump();const fresh=frame();assert.equal($('lane-timing').disabled,false);
dispatch({...fresh,variant:{...fresh.variant,laneView:{...fresh.variant.laneView,pattern:4}}});assert.equal($('lane-timing').disabled,true);frame();
for(const patch of [[1,0],[2,8],[3,32],[4,64],[6,8],[8,2]]){
 const bad=structuredClone(fresh);bad.variant.laneView.settings.values[patch[0]]=patch[1];dispatch(bad);assert.equal($('lane-timing').disabled,true,'malformed semantic lane snapshot stays locked');frame();
}
const malformed=structuredClone(fresh);malformed.variant.laneView.mode.values[4]=24;dispatch(malformed);assert.equal($('lane-mode').disabled,true);frame();

// BEGIN rejection emits no value or END, rejected values still close BEGIN.
rejectOp=3;const n=commands.length;editLength(32);await pump();assert.equal(commands.length,n+1);assert.equal(gesture,false);assert.match($('lane-status').textContent,/busy/);
rejectOp=18;editLength(32);await settle();assert.deepEqual(commands.slice(-3).map(m=>m.op),[3,18,4]);assert.equal(gesture,false);assert.equal($('lane-length').value,'16');assert.match($('lane-status').textContent,/busy/);rejectOp=null;
editLength(32);window.dispatchEvent(new Event('blur'));await settle();assert.deepEqual(commands.slice(-2).map(m=>m.op),[3,4]);assert.equal(gesture,false);assert.equal($('lane-length').value,'16');
$('lane-length').value='27';fire('lane-length','input');window.dispatchEvent(new Event('pagehide'));assert.equal($('lane-length').value,'16');

// Live edit proof stops at authoritative pending/local-boundary readbacks.
await select('lane-pattern',3);playing=true;localPending=1;frame();editLength(7);await settle();assert.equal($('lane-timing').disabled,false);assert.match($('lane-timing-state').textContent,/waiting.*next local step/);
localPending=0;effectiveRate=7;frame();assert.match($('lane-timing-state').textContent,/effective step rate 1\/32 triplet/);
fire('lane-mode');await settle();assert.equal(desiredMode,0);assert.equal(effectiveMode,1);assert.equal($('lane-mode').disabled,true);assert.match($('lane-mode-state').textContent,/Pending next musical bar/);
modePending=0;effectiveMode=0;frame();assert.equal($('lane-mode').disabled,false);assert.equal($('lane-timing').disabled,false,'initialized grouped mode remains editable');assert.match($('lane-timing-state').textContent,/BD conductor/);
const liveActions=commands.length;fire('lane-save');fire('lane-undo');fire('lane-redo');await pump();assert.equal(commands.length,liveActions);

// Both BREAK pages, bit31 gates and high packed slice nibbles are unsigned.
await select('lane-sound',24);assert.equal($('lane-break-panel').hidden,false);
const grid=$('lane-grid').children;assert.equal(grid[31].children[1].attributes['aria-pressed'],'true');assert.equal(grid[31].children[2].value,'8');
grid[31].children[1].dispatchEvent(new Event('click'));await settle();assert.equal(commands.at(-2).value,31|(8<<7));assert.equal(grid[31].children[1].attributes['aria-pressed'],'false');
fire('lane-next');await pump();frame();assert.match($('lane-page-label').textContent,/33–64/);assert.ok(snapshots.some(v=>v.join(':')==='12:3:1'));assert.equal(grid[31].children[1].attributes['aria-pressed'],'true');
grid[31].children[2].value='8';grid[31].children[2].dispatchEvent(new Event('change'));await settle();assert.equal(commands.at(-2).value,1151);assert.equal(commands.at(-2).voice,24);assert.equal(commands.at(-2).index,2);
grid[0].children[2].value='0';grid[0].children[2].dispatchEvent(new Event('change'));await settle();assert.equal(commands.at(-2).value,32|64);
assert.ok(grid[31].classes.has('lane-outside'));assert.ok(!grid[31].classes.has('lane-playing'));
const badBreak=frame();badBreak.variant.laneView.breaks.values[1]=9;dispatch(badBreak);assert.equal($('lane-break-controls').disabled,true);frame();assert.equal($('lane-break-controls').disabled,false);

playing=false;frame();for(const [id,op] of [['lane-undo',5],['lane-redo',6],['lane-save',7]]){fire(id);await settle();assert.equal(commands.at(-1).op,op);}
protectedProject=1;frame();assert.equal($('lane-timing').disabled,true);assert.equal($('lane-mode').disabled,true);const protectedN=commands.length;editLength(8);fire('lane-mode');await pump();assert.equal(commands.length,protectedN);protectedProject=0;frame();

// Ticket wrap and old-engine replies: counters remain monotonic on ready;
// selected generation must be read before replacement controls unlock.
written=0xfffffffe;read=0xfffffffe;frame();editLength(13);await pump();frame();assert.equal($('lane-timing').disabled,true);read=0xffffffff;frame();assert.equal($('lane-timing').disabled,false);
editLength(14);await pump();frame();assert.equal($('lane-timing').disabled,true);read=0;frame();assert.equal($('lane-timing').disabled,false);
editLength(15);const obsolete=sent.shift();window.dispatchEvent(new Event('fm1-ready'));await pump();dispatch({type:'lane-reply',request:obsolete.request,result:0,revision,ticket:written});assert.equal(sent.length,0);assert.equal($('lane-timing').disabled,true);frame();assert.equal($('lane-timing').disabled,false);
editLength(16);assert.ok(sent[0].request>obsolete.request);await settle();

// Worklet scalar/address guard and browser reply separation; no C call on bad input.
const command={type:'lane-command',abi:1,op:18,request:55,target:0,voice:0,index:0,value:1};
for(const patch of [{request:-1},{request:2**32},{request:1.5},{abi:65536},{op:-1},{target:256},{voice:-1},{index:NaN},{value:2**31},{value:-(2**31)-1}]){
 const before=commands.length;await worker.onMessage({...command,...patch});assert.equal(commands.length,before);assert.equal(out.pop().result,-1);
}
for(const type of ['variant-command','lane-command']){await worker.onMessage({...command,type,op:4});const result=out.pop();assert.equal(result.type,type==='lane-command'?'lane-reply':'variant-reply');assert.equal(result.request,55);}
const view={...worker.laneView};for(const patch of [{pattern:32},{lane:25},{page:2},{viewRevision:-1},{viewRevision:2**32},{lane:.5}]){await worker.onMessage({type:'lane-select',...view,...patch});assert.deepEqual(worker.laneView,view);}
caps=511;snapshots.length=0;worker.publish();assert.equal(out.pop().variant.laneView,undefined);assert.ok(!snapshots.some(v=>v[0]>=10),'old capability never calls new snapshot sections');
assert.equal(permissions,0);
console.log('Lane UI/worklet: 25 sounds,32 patterns,8 rates,64 BREAK steps; stopped initialization/live pending modes; ACK/gesture/readback/ticket wrap; drafts/protection/history; namespaces/scalars/stale selection/reset; no MIDI permission PASS');
