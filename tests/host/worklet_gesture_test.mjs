// SPDX-License-Identifier: GPL-3.0-only
// Actual C undo semantics through the production worklet. Requires real WASM;
// the direct-export negative control proves nested BEGIN is otherwise shared.
import fs from 'node:fs';import assert from 'node:assert/strict';
assert.ok(process.argv[2],'Usage: node tests/host/worklet_gesture_test.mjs build/emu/x0x.wasm');
const wasm=fs.readFileSync(process.argv[2]);let Processor;
globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,C)=>Processor=C;
await import('../../web/emu/worklet.js');
const owners=['sound','bass','arrangement','pad','lane'];const requests=new Map(owners.map(x=>[x,0]));
const message=(owner,op,value=0,extra={})=>{
 const serial=requests.get(owner)+1;requests.set(owner,serial);
 return {type:owner==='lane'?'lane-command':'variant-command',abi:1,op,
  request:owner==='lane'?serial:((owners.indexOf(owner)<<30)|serial)>>>0,target:0,voice:0,index:0,value,...extra};
};
const render=(p,n=35)=>{for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);}};
const snap=(p,section=0,target=0,voice=0)=>{const ptr=p.ex.web_variant_snapshot(section,target,voice);assert.ok(ptr);const v=new Uint32Array(p.mem.buffer,ptr,20);return {revision:v[2],values:[...v.slice(4,4+v[3])]};};
const fields=p=>({strip:snap(p,2).values[0],lane:snap(p,10).values[1]});
const call=async(p,owner,op,value=0,extra={})=>{const m=message(owner,op,value,extra);await p.onMessage(m);const reply=p.messages.at(-1);assert.equal(reply.type,owner==='lane'?'lane-reply':'variant-reply');assert.equal(reply.request,m.request);return reply.result;};
const open=async()=>{const p=new Processor();await p.onMessage({type:'load',wasm});render(p);assert.ok(snap(p).values[0]&1024);return p;};
const initialize=async p=>{assert.equal(await call(p,'lane',3),0);assert.equal(await call(p,'lane',19,1),1);assert.equal(await call(p,'lane',4),0);render(p);assert.equal(await call(p,'lane',7),0);render(p);};
const direct=(p,owner,op,value=0)=>{const m=message(owner,op,value);const ptr=p.ex.web_variant_command(...['abi','op','request','target','voice','index','value'].map(k=>m[k]));return new DataView(p.mem.buffer,ptr,16).getInt32(12,true);};

// Negative control: bypass only the new worklet ownership gate, not C history.
const raw=await open();await initialize(raw);assert.deepEqual(fields(raw),{strip:20,lane:16});
assert.equal(direct(raw,'sound',3),0);assert.equal(direct(raw,'sound',2,100),1);render(raw);
assert.equal(direct(raw,'lane',3),0,'C nested BEGIN is an idempotent global no-op');assert.equal(direct(raw,'lane',18,7),1);assert.equal(direct(raw,'lane',4),0);render(raw);
assert.equal(direct(raw,'sound',4),0);assert.equal(direct(raw,'sound',5),0);render(raw);assert.deepEqual(fields(raw),{strip:20,lane:16},'negative control merges both changes into one undo');

const p=await open();await initialize(p);
assert.equal(await call(p,'sound',3),0);assert.equal(await call(p,'sound',3),0,'same-owner BEGIN remains idempotent');
assert.equal(await call(p,'sound',2,100),1);render(p);assert.deepEqual(fields(p),{strip:100,lane:16});
for(const foreign of owners.slice(1)){
 const before=snap(p);
 for(const op of [3,foreign==='lane'?18:2,4,5,6,7])assert.equal(await call(p,foreign,op,7),-3,`${foreign} op${op} cannot join/end/history/save sound's gesture`);
 assert.deepEqual(snap(p),before,'rejected commands never enter C or advance revision/ticket');assert.deepEqual(fields(p),{strip:100,lane:16});
}
// Refused legacy BEGIN retains a bounded caller quarantine through its late
// suffix, including the gap after the actual owner has successfully ended.
assert.equal(await call(p,'bass',3),-3);assert.equal(await call(p,'sound',4),0);assert.equal(p.editOwner,null);
assert.equal(await call(p,'bass',2,300),-3);assert.equal(await call(p,'bass',5),-3);assert.equal(await call(p,'bass',7),-3);assert.equal(await call(p,'bass',4),-3);assert.equal(p.rejectedGestures.size,0);
// The acknowledged lane controller sends no suffix after rejected BEGIN, so
// it must be able to retry without an END that it never opened.
assert.equal(await call(p,'lane',3),0);assert.equal(await call(p,'lane',18,7),1);render(p);assert.equal(await call(p,'lane',4),0);
assert.deepEqual(fields(p),{strip:100,lane:7});assert.equal(await call(p,'lane',5),0);render(p);assert.deepEqual(fields(p),{strip:100,lane:16},'one Undo changes only the lane');
assert.equal(await call(p,'sound',5),0);render(p);assert.deepEqual(fields(p),{strip:20,lane:16},'second Undo changes only the strip');
assert.equal(await call(p,'sound',6),0);render(p);assert.deepEqual(fields(p),{strip:100,lane:16});assert.equal(await call(p,'lane',6),0);render(p);assert.deepEqual(fields(p),{strip:100,lane:7});

// Every directional pair is exclusive, including four distinct legacy request
// namespaces. A failed owner value never prevents that owner from sending END.
for(const owner of owners)for(const foreign of owners.filter(x=>x!==owner)){
 assert.equal(await call(p,owner,3),0);assert.equal(await call(p,foreign,3),-3);
 assert.equal(await call(p,foreign,2,200),-3);assert.equal(await call(p,foreign,4),-3);assert.equal(await call(p,owner,2,-1),-1);
 assert.equal(await call(p,owner,4),0);assert.equal(p.editOwner,null);assert.equal(p.rejectedGestures.size,0);
}
// Invalid scalar fields fail before ownership, even while a different caller
// owns C. They cannot acquire, release or clear a refused gesture quarantine.
for(const extra of [{request:-1},{request:2**32},{request:.5},{target:256},{abi:65536},{op:NaN},{value:2**31}]){
 assert.equal(await call(p,'sound',3,0,extra),-1);assert.equal(p.editOwner,null);
}
assert.equal(await call(p,'sound',3,0,{abi:2}),-2);assert.equal(p.editOwner,null,'C-rejected BEGIN never acquires');
assert.equal(await call(p,'sound',3),0);assert.equal(await call(p,'lane',4,0,{value:2**31}),-1);assert.equal(p.editOwner,'variant-0');
assert.equal(await call(p,'bass',3),-3);assert.equal(await call(p,'bass',4,0,{target:256}),-1);assert.ok(p.rejectedGestures.has('variant-1'));
assert.equal(await call(p,'sound',4),0);assert.equal(await call(p,'bass',2,500),-3);assert.equal(await call(p,'bass',4),-3);
assert.equal(await call(p,'pad',3),0);const preSave=snap(p),preSaveWrites=p.ex.web_store_writes(),actualExports=p.ex;
// Observe the actual C-export boundary: SAVE must not get far enough to close
// C's gesture, irrespective of whether the subsequent storage operation fails.
p.ex={...actualExports,web_variant_command:(...args)=>{assert.notEqual(args[1],7,'owned Save must never enter C');return actualExports.web_variant_command(...args);}};
assert.equal(await call(p,'pad',7),-3);p.ex=actualExports;assert.equal(p.editOwner,'variant-3','Save requires explicit END even when storage would succeed');assert.deepEqual(snap(p),preSave);assert.equal(p.ex.web_store_writes(),preSaveWrites);
assert.equal(await call(p,'pad',4),0);assert.equal(await call(p,'pad',7),0);assert.equal(p.editOwner,null,'END then Save matches current production UI');
assert.equal(await call(p,'pad',3),0);p.ex.web_seq_play(1);render(p);assert.equal(await call(p,'pad',7),-3);assert.equal(p.editOwner,'variant-3','busy Save retains ownership');
assert.equal(await call(p,'lane',3),-3);assert.equal(await call(p,'pad',4),0);p.ex.web_seq_play(0);render(p);
for(const op of [5,6]){assert.equal(await call(p,'sound',3),0);assert.equal(await call(p,'sound',op),0);assert.equal(p.editOwner,null);render(p);}

// Same-processor replacement: suspend commands while instantiate is pending,
// keep the original engine/owner on failure, and ignore stale completions.
const instantiate=WebAssembly.instantiate,gate=()=>{let resolve,reject;const promise=new Promise((a,b)=>{resolve=a;reject=b;});return {promise,resolve,reject};};
try{
 assert.equal(await call(p,'sound',3),0);const original=p.ex,first=gate();WebAssembly.instantiate=()=>first.promise;
 const loading=p.onMessage({type:'load',wasm});assert.equal(p.loading,true);assert.equal(await call(p,'lane',3),-3);assert.equal(await call(p,'sound',4),-3);assert.equal(p.editOwner,'variant-0');
 first.reject(Error('synthetic instantiate failure'));await assert.rejects(loading,/synthetic/);assert.equal(p.loading,false);assert.equal(p.ex,original);assert.equal(p.editOwner,'variant-0');assert.equal(await call(p,'sound',4),0);
 assert.equal(await call(p,'sound',3),0);const second=gate();WebAssembly.instantiate=()=>second.promise;const replacement=p.onMessage({type:'load',wasm});
 WebAssembly.instantiate=instantiate;second.resolve(await instantiate(wasm,{wasi_snapshot_preview1:new Proxy({},{get:()=>()=>0}),env:new Proxy({},{get:()=>()=>0})}));await replacement;
 assert.notEqual(p.ex,original);assert.equal(p.editOwner,null);assert.equal(p.rejectedGestures.size,0);assert.equal(p.loading,false);render(p);
 assert.equal(await call(p,'lane',3),0);assert.equal(await call(p,'lane',4),0);
 for(const oldFails of [false,true]){
  const old=gate(),fresh=gate();let count=0;WebAssembly.instantiate=()=>++count===1?old.promise:fresh.promise;
  const obsolete=p.onMessage({type:'load',wasm}),current=p.onMessage({type:'load',wasm});
  WebAssembly.instantiate=instantiate;fresh.resolve(await instantiate(wasm,{wasi_snapshot_preview1:new Proxy({},{get:()=>()=>0}),env:new Proxy({},{get:()=>()=>0})}));await current;render(p);
  const engine=p.ex,ready=p.messages.filter(m=>m.type==='ready').length;assert.equal(await call(p,'arrangement',3),0);
  if(oldFails)old.reject(Error('obsolete failure'));else old.resolve(await instantiate(wasm,{wasi_snapshot_preview1:new Proxy({},{get:()=>()=>0}),env:new Proxy({},{get:()=>()=>0})}));
  await obsolete;assert.equal(p.ex,engine);assert.equal(p.editOwner,'variant-2');assert.equal(p.loading,false);assert.equal(p.messages.filter(m=>m.type==='ready').length,ready);assert.equal(await call(p,'arrangement',4),0);
 }
 // A rejected BEGIN during initial/replacement load must reject its delayed
 // value/history/save suffix after either success or failure. Check all four
 // legacy caller namespaces against real C snapshots and storage counters.
 for(const outcome of ['success','failure','initial-success']){
  WebAssembly.instantiate=instantiate;const q=outcome==='initial-success'?new Processor():await open();
  const delayed=gate();WebAssembly.instantiate=()=>delayed.promise;const loading=q.onMessage({type:'load',wasm});
  assert.equal(await call(q,'sound',3,0,{request:-1}),-1);assert.equal(q.rejectedGestures.size,0,'loading still validates scalars before ownership');
  for(const owner of owners.slice(0,4))assert.equal(await call(q,owner,3),-3);
  assert.equal(q.rejectedGestures.size,4,'loading BEGIN is tracked even before first engine exists');
  assert.equal(await call(q,'sound',4,0,{target:256}),-1);assert.equal(q.rejectedGestures.size,4,'malformed END cannot clear a loading quarantine');
  if(outcome==='failure'){delayed.reject(Error('refused-load regression'));await assert.rejects(loading,/refused-load/);}
  else {WebAssembly.instantiate=instantiate;delayed.resolve(await instantiate(wasm,{wasi_snapshot_preview1:new Proxy({},{get:()=>()=>0}),env:new Proxy({},{get:()=>()=>0})}));await loading;render(q);}
  assert.equal(q.loading,false);assert.equal(q.editOwner,null);assert.equal(q.rejectedGestures.size,4,'completion cannot erase refused suffixes');
  const before=snap(q),strip=snap(q,2),writes=q.ex.web_store_writes();
  for(const owner of owners.slice(0,4)){
   for(const op of [2,5,6,7])assert.equal(await call(q,owner,op,100),-3,`${outcome}: refused ${owner} suffix op${op}`);
   assert.deepEqual(snap(q),before);assert.deepEqual(snap(q,2),strip);assert.equal(q.ex.web_store_writes(),writes);
   assert.equal(await call(q,owner,4),-3,'refused END drains caller quarantine without reaching C');
   assert.deepEqual(snap(q),before);assert.deepEqual(snap(q,2),strip);assert.equal(q.ex.web_store_writes(),writes);
  }
  assert.equal(q.rejectedGestures.size,0);assert.equal(await call(q,'sound',3),0);assert.equal(await call(q,'sound',2,100),1);assert.equal(await call(q,'sound',4),0);render(q);assert.equal(snap(q,2).values[0],100,'a fresh gesture works after explicit suffix drain');
 }
 // Initial failure has no C exports. Quarantined callers must still be able
 // to drain END, then start fresh after the first successful engine load.
 const initial=new Processor(),initialGate=gate();WebAssembly.instantiate=()=>initialGate.promise;
 const failedInitial=initial.onMessage({type:'load',wasm});
 for(const owner of owners.slice(0,4))assert.equal(await call(initial,owner,3),-3);
 initialGate.reject(Error('first engine unavailable'));await assert.rejects(failedInitial,/first engine/);
 assert.equal(initial.ex,null);assert.equal(initial.loading,false);assert.equal(initial.rejectedGestures.size,4);
 for(const owner of owners.slice(0,4)){
  for(const op of [2,5,6,7])assert.equal(await call(initial,owner,op,100),-3,'refused suffix stays BUSY with no C engine');
  assert.equal(await call(initial,owner,4,0,{target:256}),-1,'scalar guard remains first without C');
  assert.equal(await call(initial,owner,4),-3,'valid END drains quarantine without C');
 }
 assert.equal(initial.rejectedGestures.size,0);assert.equal(await call(initial,'sound',3),-2,'unquarantined caller correctly sees unavailable engine');
 WebAssembly.instantiate=instantiate;await initial.onMessage({type:'load',wasm});render(initial);
 for(const [i,owner] of owners.slice(0,4).entries()){
  assert.equal(await call(initial,owner,3),0);assert.equal(await call(initial,owner,2,100+i),1);assert.equal(await call(initial,owner,4),0);render(initial);assert.equal(snap(initial,2).values[0],100+i);
 }
}finally{WebAssembly.instantiate=instantiate;}
console.log(JSON.stringify({realWasmBytes:wasm.length,negativeControlMerged:true,separateUndoRedo:true,ownerPairs:20,blindSuffixRejected:true,scalarFirst:true,failedEditAllowsEnd:true,saveRequiresEnd:true,replacementGeneration:true,refusedLoadSuffixes:16,initialFailureCleanup:true}));
