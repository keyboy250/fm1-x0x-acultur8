// SPDX-License-Identifier: GPL-3.0-only
// Production AudioWorklet lifecycle + CC router, with controlled instantiate,
// C exports and MIDI ports. Compiled C/WASM application is a separate check.
import assert from 'node:assert/strict';
import {CcInputRouter} from '../../web/emu/cc-input-router.mjs';
import {ccPacket} from '../../web/emu/cc-input-worklet.mjs';

let Worklet,permissionRequests=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(name,C)=>{assert.equal(name,'x0x');Worklet=C;};
globalThis.currentTime=0;
Object.defineProperty(globalThis,'navigator',{configurable:true,value:{requestMIDIAccess:()=>{permissionRequests++;throw Error('Unexpected permission');}}});
await import('../../web/emu/worklet.js');
const deferred=()=>{let resolve,reject;const promise=new Promise((yes,no)=>{resolve=yes;reject=no;});return {promise,resolve,reject};};
const pending=[];
const instantiate=WebAssembly.instantiate;
WebAssembly.instantiate=()=>{const item=deferred();pending.push(item);return item.promise;};
function engine(name,{supported=true,stats=[1,2,3,4,5,-1]}={}){
  const memory=new WebAssembly.Memory({initial:3});
  const calls=[],ingress=[],unreviewed=[];let owner=false,resets=0;
  const ex={memory,web_store_max:()=>0,web_store_writes:()=>0,web_master:()=>{},web_boot:()=>calls.push('boot'),
    web_blits:()=>0,web_lit_buttons:()=>0,web_lit_keys:()=>0,web_dim_keys:()=>0,
    web_fb:()=>8192,web_nobj:()=>0,web_sync_state:()=>0,web_seq_bpm:()=>120,web_seq_playing:()=>0,web_seq_beat:()=>0,
    web_variant_command:(abi,op,request)=>{
      calls.push({op,request});if(op===3)owner=true;if([4,5,6,7].includes(op))owner=false;
      const p=64,v=new DataView(memory.buffer,p,16);v.setUint32(0,request,true);v.setUint32(4,1,true);v.setUint32(8,0,true);v.setInt32(12,0,true);return p;
    }};
  if(supported)Object.assign(ex,{
    web_cc_version:()=>0x10001,
    web_cc_reset_source:source=>{assert.equal(source,2);resets++;unreviewed.length=0;calls.push('reset');},
    web_midi_ingest:(packet,source)=>{assert.equal(source,2);ingress.push(packet);unreviewed.push(packet);return 0;},
    web_cc_pending:()=>unreviewed.length,web_cc_stat:i=>stats[i],
  });
  return {name,ex,calls,ingress,unreviewed,resets:()=>resets,owner:()=>owner};
}
const startLoad=worker=>{const loading=worker.onMessage({type:'load',wasm:new ArrayBuffer(0)});return {loading,gate:pending.shift()};};
const load=async(worker,next)=>{const {loading,gate}=startLoad(worker);gate.resolve({instance:{exports:next.ex}});await loading;};
const command=(op,request=1,type='variant-command')=>({type,abi:1,op,request,target:0,voice:0,index:0,value:0});
const message=async(worker,m)=>{await worker.onMessage(m);return worker.messages.at(-1);};
const route=(ready,id=1,request=id)=>({type:'cc-input-route',request,route:id,ready});
const batch=(id=1,request=50,packets=[ccPacket([0xb0,7,99])])=>({type:'cc-input-batch',request,route:id,packets});
const frame=worker=>{worker.publish();return worker.messages.at(-1);};

try {
  const worker=new Worklet(),a=engine('A');
  assert.equal(permissionRequests,0);
  await load(worker,a);
  assert.deepEqual(a.calls,['boot','reset'],'new engine reset follows boot and precedes ready');
  assert.deepEqual(worker.messages.map(m=>m.type),['cc-input-reset','ready']);
  assert.deepEqual(frame(worker).ccInput,{version:0x10001,route:0,ready:false,pending:0,stats:[1,2,3,4,5,0xffffffff]});
  assert.equal((await message(worker,route(true))).result,0);
  assert.equal((await message(worker,batch())).prefix,1);
  assert.equal(frame(worker).ccInput.pending,1);

  // CC follows C's shared undo policy: it must neither acquire nor release a
  // browser panel owner. It is not quarantined as a foreign panel gesture.
  await message(worker,command(3));assert.equal(worker.editOwner,'variant-0');
  assert.equal((await message(worker,command(3,2,'lane-command'))).result,-3);
  assert.equal((await message(worker,batch())).prefix,1);
  assert.equal(worker.editOwner,'variant-0');assert.equal(a.owner(),true);
  const count=a.calls.length;
  assert.equal((await message(worker,command(7))).result,-3,'SAVE cannot close an owned gesture');
  assert.equal(a.calls.length,count);
  await message(worker,route(false,2));assert.equal(worker.editOwner,'variant-0');assert.equal(a.owner(),true);
  await message(worker,command(4));assert.equal(worker.editOwner,null);

  // Initial reset must see the old exports, before loading hides them. While
  // instantiate is pending, no route or ingress can reach the retained engine.
  await message(worker,route(true,3));await message(worker,batch(3));
  const resets=a.resets(),received=a.ingress.length,replacing=startLoad(worker);
  assert.equal(worker.loading,true);assert.equal(a.resets(),resets+1);assert.equal(a.unreviewed.length,0);
  assert.equal(worker.messages.at(-1).type,'cc-input-reset');
  assert.equal(frame(worker).ccInput,undefined,'loading never advertises the old CC API');
  assert.equal((await message(worker,route(true,4))).result,-2);
  assert.equal((await message(worker,batch(3))).result,-3);assert.equal(a.ingress.length,received);
  assert.equal((await message(worker,command(3,0x40000001))).result,-3);
  const b=engine('B');replacing.gate.resolve({instance:{exports:b.ex}});await replacing.loading;
  assert.equal(b.resets(),1);assert.equal(worker.ccInput.ready,false);assert.equal(worker.ccInput.route,3);
  assert.equal((await message(worker,route(true,3))).result,-3,'old route cannot reactivate after replacement');
  assert.equal((await message(worker,command(8,0x40000002))).result,-3,'refused legacy suffix remains quarantined after success');
  assert.equal((await message(worker,command(4,0x40000003))).result,-3);
  assert.equal((await message(worker,command(3,0x40000004))).result,0);
  await message(worker,command(4,0x40000005));
  assert.equal((await message(worker,route(true,4))).result,0);

  // Latest failure retains old C/gesture but clears source 2 again. Never infer
  // rollback of values that C has already accepted/applied.
  await message(worker,command(3));const failed=startLoad(worker),failure=assert.rejects(failed.loading,/bad replacement/);
  assert.equal((await message(worker,command(3,0x80000001))).result,-3);
  failed.gate.reject(Error('bad replacement'));await failure;
  assert.equal(worker.ex,b.ex);assert.equal(worker.editOwner,'variant-0');assert.equal(b.owner(),true);
  assert.equal(b.resets(),4);assert.equal(worker.ccInput.ready,false);assert.equal(frame(worker).ccInput.ready,false);
  assert.equal((await message(worker,command(8,0x80000002))).result,-3);
  assert.equal((await message(worker,command(4,0x80000003))).result,-3);
  await message(worker,command(4));assert.equal(worker.editOwner,null);

  // Older successes/failures cannot reset the latest engine or route.
  for(const fail of [false,true]){
    const older=startLoad(worker),newer=startLoad(worker),next=engine(fail?'D':'C'),obsolete=engine('obsolete');
    newer.gate.resolve({instance:{exports:next.ex}});await newer.loading;
    const nextRoute=worker.ccInput.route+1;await message(worker,route(true,nextRoute));
    const before=next.resets(),readyCount=worker.messages.filter(m=>m.type==='ready').length;
    if(fail)older.gate.reject(Error('obsolete failure'));else older.gate.resolve({instance:{exports:obsolete.ex}});
    await older.loading;
    assert.equal(worker.ex,next.ex);assert.equal(next.resets(),before);assert.equal(worker.ccInput.ready,true);
    assert.equal(obsolete.calls.length,0);assert.equal(worker.messages.filter(m=>m.type==='ready').length,readyCount);
  }
  const current=worker.ex,ingressCalls=worker.ccInput.ex.web_cc_pending(),sparse=[ccPacket([0xb0,7,1]),,ccPacket([0xb0,7,2])];
  assert.equal((await message(worker,batch(worker.ccInput.route,100,sparse))).result,-1);
  assert.equal(current.web_cc_pending(),ingressCalls);assert.equal(worker.ccInput.ready,true);
  await load(worker,engine('old',{supported:false}));assert.equal(frame(worker).ccInput,undefined);
  assert.equal((await message(worker,route(true,worker.ccInput.route+1))).result,-2);
  const missingStats=engine('missing stats');delete missingStats.ex.web_cc_stat;
  await load(worker,missingStats);assert.equal(frame(worker).ccInput,undefined);

  // Actual router <-> actual processor exchange. The only permission call is
  // explicit connect; load reset stops old callbacks and pending ACKs, and
  // surviving replacement failure still requires fresh-frame + explicit retry.
  const integrated=new Worklet(),first=engine('integrated A'),inbox=[],timers=new Map();let timer=0,permissions=0;
  const input=new EventTarget();Object.assign(input,{id:'A',state:'connected',opens:0,async open(){this.opens++;return this;},async close(){}});
  const access=new EventTarget();access.inputs=new Map([[input.id,input]]);
  const router=new CcInputRouter({send:m=>inbox.push(m),requestAccess:async()=>{permissions++;return access;},
    schedule:fn=>{const id=++timer;timers.set(id,fn);return id;},cancel:id=>timers.delete(id)});
  const deliver=m=>{if(m.type==='cc-input-reset'||m.type==='ready')router.engineReset();else if(m.type==='frame')router.engineState(m.ccInput);else router.message(m);};
  const flush=async()=>{for(let i=0;i<30;i++){while(inbox.length)await integrated.onMessage(inbox.shift());while(integrated.messages.length)deliver(integrated.messages.shift());await Promise.resolve();}};
  const sendFrame=async()=>{frame(integrated);await flush();};
  const activate=async()=>{const result=router.activate('A');await flush();assert.equal(await result,true);};
  await load(integrated,first);await flush();await sendFrame();assert.equal(permissions,0);
  await router.connect();await activate();assert.equal(permissions,1);
  const oldHandler=router.handler;oldHandler({data:[0xb0,20,99]});
  await integrated.onMessage(inbox.shift());const late=integrated.messages.shift();assert.equal(late.prefix,1);
  const reload=startLoad(integrated);await flush();assert.equal(router.active,false);assert.equal(router.flight,null);assert.equal(router.fresh,false);
  router.message(late);assert.equal(router.accepted,0,'unknown old prefix is never counted/replayed');
  oldHandler({data:[0xb0,20,100]});assert.equal(inbox.length,0);
  const replacement=engine('integrated B');reload.gate.resolve({instance:{exports:replacement.ex}});await reload.loading;await flush();
  assert.equal(router.active,false);assert.equal(await router.retry(),false,'fresh frame required after actual ready');
  await sendFrame();await activate();assert.equal(input.opens,2);assert.equal(replacement.ingress.length,0);
  oldHandler({data:[0xb0,20,101]});await flush();assert.equal(replacement.ingress.length,0);
  router.handler({data:[0xb0,20,102]});await flush();assert.deepEqual(replacement.ingress,[ccPacket([0xb0,20,102])]);
  const latest=startLoad(integrated),rejected=assert.rejects(latest.loading,/cannot instantiate/);await flush();
  latest.gate.reject(Error('cannot instantiate'));await rejected;await sendFrame();assert.equal(router.active,false);
  await activate();assert.equal(replacement.ingress.length,1,'failed replacement does not replay previously received data');
  assert.equal(permissions,1);assert.equal(permissionRequests,0);router.pagehide();await flush();
  assert.equal(integrated.ccInput.ready,false);assert.equal(timers.size,0);
  console.log('CC input production worklet/router integration: load-start/success/failure/stale-generation resets, unsigned frames, old-engine fallback, shared panel owner policy, suffix quarantine, malformed batch and explicit retry PASS (controlled C exports/API only)');
} finally {
  WebAssembly.instantiate=instantiate;
}
