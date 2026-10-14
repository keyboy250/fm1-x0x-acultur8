// SPDX-License-Identifier: GPL-3.0-only
// The real router and worklet bridge, with bounded fake C transport and WebMIDI ports.
// Physical receiver delivery and actual WASM producer tests are separate.
import assert from 'node:assert/strict';
import {MidiRouter,midiBytes,noteOn} from '../../web/emu/midi-router.mjs';
import {MidiWorkletBridge} from '../../web/emu/midi-worklet.mjs';
const packet=(status,a=0,b=0,cin=status>=248?15:status>>4)=>(cin|(status<<8)|(a<<16)|(b<<24))>>>0;
const on=packet(0x92,60,90),off=packet(0x82,60,0),clock=packet(0xf8),zero=packet(0x92,60,0);
assert.deepEqual(midiBytes(on),[0x92,60,90]);assert.deepEqual(midiBytes(zero),[0x92,60,0]);
assert.equal(noteOn(zero),false);assert.deepEqual(midiBytes(clock),[248]);
assert.deepEqual(midiBytes(packet(0xc3,12)),[0xc3,12]);
assert.deepEqual(midiBytes(packet(0xd3,13)),[0xd3,13]);
assert.deepEqual(midiBytes(packet(0xb3,7,99)),[0xb3,7,99]);
assert.equal(midiBytes(packet(240,0,0,4)),null);
for(const bad of [NaN,Infinity,-1,0x100000000,'123',1.5])assert.equal(midiBytes(bad),null);
class Core {
 constructor(){this.ring=[];this.latched=0;this.epoch=0;this.ready=0;this.playing=0;}
 web_seq_playing(){return this.playing;}
 web_midi_epoch(){return this.epoch;}
 web_midi_pending(){return this.ring.length;}
 web_midi_ready(v){if(this.ready&&!v)this.web_midi_cancel();this.ready=v;return 0;}
 web_midi_cancel(){this.epoch++;this.ring=this.ring.map(p=>noteOn(p)?(p|0x80000000)>>>0:p);return this.epoch;}
 web_midi_batch(n){if(this.latched)return -1;return this.latched=Math.min(n,this.ring.length);}
 web_midi_packet(i){return this.ring[i]??0;}
 web_midi_ack(n){if(n<0||n>this.latched)return -1;this.ring.splice(0,n);this.latched-=n;return 0;}
 put(...p){if(!this.ready||this.ring.length+p.length>64)return false;this.ring.push(...p);return true;}
}
class Port {
 constructor(id){this.id=this.name=id;this.state='connected';this.connection='closed';this.sent=[];this.failAt=null;this.opens=0;}
 async open(){this.opens++;this.connection='open';return this;}
 async close(){this.connection='closed';}
 send(bytes){if(this.state!=='connected'||this.failAt===this.sent.length)throw Error('synthetic send failure');this.sent.push([...bytes]);}
}
function frame({playing=false,owned=false,cap=511}={}){
 return {type:'frame',sync:{playing,bpm:120},variant:{info:{abi:1,values:[cap]},pad:{abi:1,section:8,values:[2,60,48,84,1,0,0,owned?2:255,...(owned?[60,64,67]:[255,255,255]),60,64,67,owned?1:0,1]},bass:[{values:[0,0,0,0,0,255,255]},{values:[0,0,0,0,0,255,255]}]}};
}
function rig(){
 const a=new Port('A'),b=new Port('B'),access=new EventTarget();access.outputs=new Map([['A',a],['B',b]]);
 let requests=0,time=100,hold=null,core=new Core();const wire=[],held=[],out=[];
 const router=new MidiRouter({send:m=>wire.push(['to',m]),navigator:{requestMIDIAccess:async opts=>{requests++;assert.deepEqual(opts,{sysex:false});return access;}},now:()=>time++});
 const makeBridge=c=>new MidiWorkletBridge(()=>c,m=>{out.push(m);wire.push(['from',m]);});
 let bridge=makeBridge(core);
 const flush=async()=>{
   for(let i=0;i<200;i++){await Promise.resolve();if(wire.length){const [d,m]=wire.shift();if(hold?.(d,m))held.push([d,m]);else if(d==='to')bridge.message(m);else router.receive(m);}else {for(let n=0;n<16;n++)await Promise.resolve();if(!wire.length)return;}}
   throw Error('message bridge failed to settle');
 };
 const act=async fn=>{let done=false,error;fn().then(()=>done=true,e=>{done=true;error=e;});for(let i=0;!done&&i<100;i++)await flush();assert.ok(done);await flush();if(error)throw error;};
 router.updateFrame(frame());
 return {a,b,access,get core(){return core;},router,get bridge(){return bridge;},wire,out,flush,act,held,
   replaceEngine:()=>{const old={core,bridge};wire.length=0;core=new Core();bridge=makeBridge(core);router.engineReset();return old;},
   hold:predicate=>{hold=predicate;},release:()=>{hold=null;wire.push(...held.splice(0));},get requests(){return requests;}};
}
const deferred=()=>{let resolve;const promise=new Promise(r=>resolve=r);return {promise,resolve};};
const watch=promise=>{const state={done:false,error:null};promise.then(()=>state.done=true,error=>{state.done=true;state.error=error;});return state;};
async function settle(r,state){for(let i=0;!state.done&&i<100;i++)await r.flush();assert.ok(state.done,'bounded async operation settled');await r.flush();}
function disconnectReconnect(r,port){
 port.state='disconnected';r.access.dispatchEvent(new Event('statechange'));
 assert.equal(r.router.active,false,'disconnect immediately halts routing');
 port.state='connected';r.access.dispatchEvent(new Event('statechange'));
 assert.equal(r.router.active,false,'reconnect never activates automatically');
}
function retryAvailable(r,port){
 assert.equal(r.router.output,port,'original output is retained');
 assert.equal(r.router.active,false);assert.equal(r.router.locked,false,'late reply releases control lock');
 assert.equal(r.core.ready,0,'late success cannot leave the worklet ready');
 // These are the route-specific gates used by the real Retry button.
 assert.ok(r.router.output&&r.router.engine&&!r.router.playing&&!r.router.locked&&!r.router.active);
}
{
 // A statechange during the first open must invalidate even if the port has
 // reconnected before open() resolves. It was not yet an active output.
 const r=rig();await r.act(()=>r.router.connect());const gate=deferred();
 r.a.open=async()=>{r.a.opens++;await gate.promise;r.a.connection='open';return r.a;};
 const operation=watch(r.router.select('A'));await r.flush();
 assert.equal(r.router.output,r.a);disconnectReconnect(r,r.a);gate.resolve();await settle(r,operation);
 assert.match(operation.error?.message,/interrupted/);retryAvailable(r,r.a);assert.deepEqual(r.a.sent,[]);
 r.a.open=Port.prototype.open;await r.act(()=>r.router.retry());assert.equal(r.router.active,true);
}
for(const reconnectBeforeReply of [false,true]){
 // Reproduce the reported blocker: ready succeeds in C, its reply is delayed,
 // then A disconnects/reconnects. The late success must not set active=true.
 const r=rig();await r.act(()=>r.router.connect());
 r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request>0);
 const operation=watch(r.router.select('A'));await r.flush();assert.equal(r.held.length,1);assert.equal(r.core.ready,1);
 r.core.put(on);r.bridge.tick();await r.flush();assert.deepEqual(r.a.sent,[],'activation-await attacks cannot escape');
 if(reconnectBeforeReply)disconnectReconnect(r,r.a);
 else {r.a.state='disconnected';r.access.dispatchEvent(new Event('statechange'));}
 await r.flush();r.release();await settle(r,operation);
 assert.match(operation.error?.message,/interrupted/);retryAvailable(r,r.a);assert.deepEqual(r.a.sent,[]);
 if(!reconnectBeforeReply){r.a.state='connected';r.access.dispatchEvent(new Event('statechange'));await r.flush();retryAvailable(r,r.a);}
 await r.act(()=>r.router.retry());assert.equal(r.router.active,true);assert.deepEqual(r.a.sent,[]);
}
{
 // A new MIDIPort with the same ID is not authority for an in-flight old open.
 const r=rig();await r.act(()=>r.router.connect());
 r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request>0);
 const operation=watch(r.router.select('A'));await r.flush();
 const replacement=new Port('A');r.access.outputs.set('A',replacement);r.access.dispatchEvent(new Event('statechange'));
 await r.flush();r.release();await settle(r,operation);retryAvailable(r,r.a);
 assert.match(operation.error?.message,/interrupted/);assert.equal(replacement.opens,0);
 await r.act(()=>r.router.retry());assert.equal(r.router.output,replacement);assert.equal(r.router.active,true);
}
for(const boundary of ['open','ready','cancel']){
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.core.put(on);r.router.pump();await r.flush();r.router.halt('synthetic failure');await r.flush();
 assert.equal(r.router.debt,1);const gate=deferred();
 if(boundary==='open')r.a.open=async()=>{r.a.opens++;await gate.promise;return r.a;};
 else {const request=r.router.request+(boundary==='ready'?1:2);r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request===request);}
 const operation=watch(r.router.retry());await r.flush();
 if(boundary!=='open')assert.equal(r.held.length,1);
 // Owed Off + new On remain in the C ring while retry is interrupted. No
 // accepted prefix is replayed; cleanup still belongs to A after reconnect.
 if(boundary!=='open'){assert.ok(r.core.put(off,packet(0x92,64,90)));}
 disconnectReconnect(r,r.a);await r.flush();gate.resolve();r.release();await settle(r,operation);
 assert.match(operation.error?.message,/interrupted/,boundary);retryAvailable(r,r.a);
 assert.deepEqual(r.a.sent,[[146,60,90]],boundary+' cannot leak an On or erase accepted-note debt');assert.equal(r.router.debt,1);
 if(boundary!=='open'){assert.equal(r.core.ring[0],off);assert.ok(r.core.ring[1]>>>31,'unsent attack is cancelled, not replayed');}
 r.a.open=Port.prototype.open;await r.act(()=>r.router.retry());await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,90],[130,60,0]]);assert.equal(r.router.debt,0);assert.equal(r.core.ring.length,0);
 assert.equal(r.router.active,true);assert.equal(r.router.output,r.a);
}
for(const boundary of ['cancel','disable','close','new-open','new-ready']){
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.core.put(on);r.router.pump();await r.flush();const gate=deferred();
 const oldBoundary=['cancel','disable','close'].includes(boundary),port=oldBoundary?r.a:r.b;
 if(boundary==='close')r.a.close=async()=>{await gate.promise;r.a.connection='closed';};
 else if(boundary==='new-open')r.b.open=async()=>{r.b.opens++;await gate.promise;return r.b;};
 else {const offset={cancel:1,disable:2,'new-ready':3}[boundary],request=r.router.request+offset;r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request===request);}
 const operation=watch(r.router.select('B'));await r.flush();
 if(!['close','new-open'].includes(boundary)){
   for(let i=0;!r.held.length&&i<100;i++)await r.flush();
   assert.equal(r.held.length,1,boundary);
 }else for(let i=0;r.router.output!==port&&i<100;i++)await r.flush();
 assert.equal(r.router.output,port,boundary);disconnectReconnect(r,port);await r.flush();
 gate.resolve();r.release();await settle(r,operation);
 for(let i=0;r.router.locked&&i<100;i++)await r.flush();retryAvailable(r,port);
 if(boundary==='cancel')assert.match(operation.error?.message,/interrupted/);
 if(oldBoundary)assert.equal(r.b.opens,0,'old-route interruption cannot open the requested new port');
 assert.deepEqual(r.b.sent,[]);assert.equal(r.a.sent.filter(bytes=>bytes[0]===146&&bytes[2]>0).length,1);
 r.a.close=Port.prototype.close;r.b.open=Port.prototype.open;
 await r.act(()=>r.router.retry());await r.flush();
 assert.equal(r.router.output,r.b);assert.equal(r.router.active,true);
 assert.deepEqual(r.a.sent,[[146,60,90],[130,60,0]],'original A receives its own cleanup before B activates');
 assert.deepEqual(r.b.sent,[]);assert.equal(r.router.debt,0);
}
for(const boundary of ['first-open','first-ready','retry-open','retry-ready','retry-cancel','switch-cancel','switch-disable']){
 const r=rig();await r.act(()=>r.router.connect());const first=boundary.startsWith('first-');
 if(!first){await r.act(()=>r.router.select('A'));r.core.put(on);r.router.pump();await r.flush();}
 if(boundary.startsWith('retry-')){r.router.halt('synthetic failure');await r.flush();}
 const gate=deferred(),open=boundary.endsWith('open');
 if(open)r.a.open=async()=>{r.a.opens++;await gate.promise;return r.a;};
 else {const offset=boundary==='retry-cancel'||boundary==='switch-disable'?2:1,request=r.router.request+offset;
   r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request===request);}
 const operation=watch(first?r.router.select('A'):boundary.startsWith('retry-')?r.router.retry():r.router.select('B'));
 await r.flush();if(!open)assert.equal(r.held.length,1,boundary);
 const oldRoute=r.router.route,oldBridgeRoute=r.bridge.route,oldRequest=r.router.request,oldDebt=r.router.debt,accepted=r.a.sent.length;
 const oldReplies=r.held.map(([,m])=>m),old=r.replaceEngine();
 assert.notEqual(r.router.route,oldRoute);assert.equal(r.router.rpcs.size,0);assert.equal(r.router.active,false);
 assert.equal(r.router.engine,false);assert.equal(r.router.playing,true);assert.equal(r.router.output,r.a);
 assert.equal(r.router.debt,oldDebt);assert.equal(r.router.pending,0);assert.equal(r.router.flight,null);
 await assert.rejects(()=>r.router.retry(),/Stop playback/,'a fresh frame is required after replacement');
 r.router.updateFrame(frame());r.a.open=Port.prototype.open;
 const freshRequest=oldRequest+1;r.hold((d,m)=>d==='from'&&m.type==='midi-route-reply'&&m.request===freshRequest);
 const fresh=watch(r.router.retry());await r.flush();await settle(r,operation);
 if(boundary!=='switch-disable')assert.match(operation.error?.message,/Engine replaced/,boundary);
 assert.equal(r.router.rpcs.size,1);assert.equal(r.router.locked,true,'old finalizer cannot clear the new retry lock');
 assert.equal(fresh.done,false);assert.ok(r.held.some(([,m])=>m.request===freshRequest));
 // Request numbers and route generations survive reset. Neither late control
 // replies nor old packet/state/ack messages may alter the replacement route.
 for(const m of oldReplies)r.router.receive(m);
 r.router.receive({type:'midi-state',route:oldRoute,epoch:999,pending:64,inflight:true});
 r.router.receive({type:'midi-batch',route:oldRoute,batch:999,epoch:0,count:1,packets:[on]});
 r.router.receive({type:'midi-acknowledged',route:oldRoute,batch:999});
 gate.resolve();await r.flush();
 assert.equal(r.router.rpcs.size,1);assert.equal(r.router.active,false);assert.equal(r.router.locked,true);
 assert.equal(r.a.sent.length,accepted,'late open/reply/batch cannot send a note or clear cleanup debt');
 assert.equal(r.router.debt,oldDebt);assert.equal(r.router.pending,0);assert.equal(old.bridge.route,oldBridgeRoute);
 r.release();await settle(r,fresh);assert.equal(fresh.error,null);await r.flush();
 assert.equal(r.router.output,r.a,'reset cancels a previous request to switch to B');assert.equal(r.router.active,true);
 assert.equal(r.b.opens,0);assert.deepEqual(r.b.sent,[]);assert.equal(r.router.debt,0);assert.equal(r.core.ring.length,0);
 assert.deepEqual(r.a.sent,first?[]:[[146,60,90],[130,60,0]],'only original-output Off cleanup survives replacement');
}
for(const stage of ['before-acceptance','awaiting-ack']){
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.core.put(on);r.router.pump();await r.flush();const oldRoute=r.router.route;
 r.core.put(packet(0x92,64,90),clock);r.bridge.drain();const batch=r.out.filter(m=>m.type==='midi-batch').at(-1);
 if(stage==='awaiting-ack'){
   r.hold((d,m)=>d==='from'&&m.type==='midi-acknowledged');await r.flush();assert.ok(r.router.flight);
 }
 const debt=r.router.debt;r.replaceEngine();r.router.updateFrame(frame());
 assert.equal(r.router.flight,null);assert.equal(r.router.drainWaiting,false);assert.equal(r.router.lastBatch,null);
 assert.equal(r.router.debt,debt);r.router.receive(batch);
 r.router.receive({type:'midi-acknowledged',route:oldRoute,batch:batch.batch});r.release();await r.flush();
 assert.equal(r.router.flight,null);assert.equal(r.router.debt,debt);
 await r.act(()=>r.router.retry());assert.equal(r.router.debt,0);assert.equal(r.router.active,true);
 assert.equal(r.a.sent.filter(bytes=>(bytes[0]&240)===144&&bytes[2]>0).length,stage==='awaiting-ack'?2:1,'only API-accepted attacks acquire cleanup debt');
 assert.equal(r.a.sent.filter(bytes=>(bytes[0]&240)===128).length,debt);
}
{
 // A close already submitted to WebMIDI must settle before explicit reopen;
 // otherwise a late physical close could close a newly activated route.
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 const gate=deferred();r.a.close=async()=>{await gate.promise;r.a.connection='closed';};
 await r.act(()=>r.router.select('B'));assert.ok(r.router.portClosing);
 r.replaceEngine();r.router.updateFrame(frame());assert.equal(r.router.locked,true);
 await assert.rejects(()=>r.router.retry(),/control is busy/);assert.equal(r.b.opens,0);
 gate.resolve();await r.flush();retryAvailable(r,r.a);
 r.a.close=Port.prototype.close;await r.act(()=>r.router.retry());assert.equal(r.router.output,r.a);assert.equal(r.router.active,true);
}
{
 const r=rig();assert.equal(r.requests,0);assert.equal(r.core.ready,0);
 await r.act(()=>r.router.connect());assert.equal(r.requests,1);assert.equal(r.a.opens,0);
 await r.act(()=>r.router.select('A'));assert.equal(r.core.ready,1);
 const forged={type:'midi-batch',route:r.router.route,batch:123,epoch:0,count:1,packets:[on]};
 for(const patch of [{route:999},{packets:[-1]},{packets:[NaN]},{packets:[0x100000000]},{batch:1.5},{epoch:-1},{count:2}])r.router.receive({...forged,...patch});
 assert.equal(r.a.sent.length,0);
 r.core.put(on,clock,packet(0xc3,12),packet(0xd3,13),zero);
 r.router.pump();await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,90],[248],[195,12],[211,13],[146,60,0]]);
 assert.equal(r.router.debt,0);assert.equal(r.core.ring.length,0);
 assert.ok(r.router.rtt>=0);assert.equal(r.router.accepted,5);
 // Duplicate/unknown acknowledgements never consume fresh appends.
 r.core.put(on);r.bridge.message({type:'midi-ack',route:r.router.route,batch:999,epoch:0,prefix:1});
 assert.equal(r.core.ring.length,1);
 // Full queue and 16-packet flights, only one flight before explicit prefix ack.
 r.core.ring=[];assert.ok(r.core.put(...Array(64).fill(clock)));assert.equal(r.core.put(clock),false);
 r.router.pump();await r.flush();
 assert.equal(r.core.ring.length,0);assert.ok(r.out.filter(m=>m.type==='midi-batch').every(m=>m.packets.length<=16));
 const last=r.out.filter(m=>m.type==='midi-batch').at(-1),sent=r.a.sent.length;
 r.router.receive(last);await r.flush();assert.equal(r.a.sent.length,sent,'duplicate accepted batch never replays');
}
{
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.a.failAt=1;r.core.put(on,off,packet(0x92,64,90));r.router.pump();await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,90]]);assert.equal(r.router.active,false);
 assert.equal(r.core.ring.length,2);assert.equal(r.core.latched,2);
 assert.equal(r.core.ring[0],off);assert.ok(r.core.ring[1]>>>31);
 assert.equal(r.router.debt,1);
 // Disconnection keeps the original port; selecting B cannot release ownership.
 r.a.state='disconnected';r.access.dispatchEvent(new Event('statechange'));await r.flush();
 await assert.rejects(()=>r.router.select('B'),/original output/);
 r.a.state='connected';r.access.dispatchEvent(new Event('statechange'));await r.flush();
 assert.equal(r.router.active,false);assert.equal(r.core.ready,0);
 r.a.failAt=null;await r.act(()=>r.router.retry());await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,90],[130,60,0]]);
 assert.equal(r.core.ring.length,0);assert.equal(r.router.debt,0);
 assert.equal(r.router.cancelled,1);assert.ok(r.a.sent.every(bytes=>bytes.every(b=>b<256)));
}
{
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.core.put(on);r.router.pump();await r.flush();assert.equal(r.router.debt,1);
 // Switching must clean only old A, before enabling B; old queue On is cancelled.
 r.core.put(packet(0x92,67,90),off);
 await r.act(()=>r.router.select('B'));await r.flush();
 assert.equal(r.router.output.id,'B');assert.equal(r.router.active,true);
 assert.deepEqual(r.a.sent,[[146,60,90],[130,60,0]]);assert.deepEqual(r.b.sent,[]);
 r.core.put(packet(0x93,55,90));r.router.pump();await r.flush();
 assert.deepEqual(r.b.sent,[[147,55,90]]);
 r.router.pagehide();await r.flush();assert.equal(r.router.active,false);
 assert.deepEqual(r.b.sent.at(-1),[131,55,0]);
}
{
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.router.updateFrame(frame({owned:true}));r.core.put(on);r.router.pump();await r.flush();
 await r.act(()=>r.router.select('B'));assert.equal(r.router.output.id,'A');assert.equal(r.a.connection,'open');
 assert.equal(r.b.opens,0,'C ownership prevents routing to a new output');
 r.a.failAt=r.a.sent.length;r.router.updateFrame(frame());await r.flush();
 assert.equal(r.router.active,false);assert.equal(r.router.debt,1);assert.equal(r.b.opens,0,'rejected cleanup keeps original output');
 r.a.failAt=null;await r.act(()=>r.router.retry());r.router.updateFrame(frame());await r.flush();
 assert.equal(r.router.output.id,'B');assert.deepEqual(r.a.sent,[[146,60,90],[130,60,0]]);
}
{
 const c=new Core(),messages=[],b=new MidiWorkletBridge(()=>c,m=>messages.push(m));
 c.ready=1;c.put(on,off,zero,clock);
 b.route=1;b.ready=true;b.drain();const first=messages.find(m=>m.type==='midi-batch');
 b.drain();assert.equal(messages.filter(m=>m.type==='midi-batch').length,1);
 for(const patch of [{route:2},{batch:first.batch+1},{epoch:first.epoch+1},{prefix:5},{prefix:-1},{prefix:1.5}]){
   b.message({type:'midi-ack',route:1,batch:first.batch,epoch:first.epoch,prefix:1,...patch});
   assert.equal(c.ring.length,4,'invalid/stale acknowledgements cannot consume packets');
 }
 b.message({type:'midi-ack',route:1,batch:first.batch,epoch:first.epoch,prefix:1});
 const messagesBefore=messages.filter(m=>m.type==='midi-batch').length;b.tick();
 assert.equal(messages.filter(m=>m.type==='midi-batch').length,messagesBefore,'partial suffix waits for explicit drain');
 c.web_midi_cancel();c.put(packet(0x92,67,90));b.drain();
 const suffix=messages.filter(m=>m.type==='midi-batch').at(-1);
 assert.deepEqual(suffix.packets,[off,zero,clock]);assert.equal(c.latched,3);
 b.message({type:'midi-ack',route:1,batch:suffix.batch,epoch:suffix.epoch,prefix:3});b.drain();
 assert.deepEqual(messages.filter(m=>m.type==='midi-batch').at(-1).packets,[packet(0x92,67,90)]);
 // Stop epoch cancels a posted old attack, preserving a velocity-zero Off.
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.core.put(on,zero);r.bridge.drain();
 const batch=r.out.filter(m=>m.type==='midi-batch').at(-1);
 r.wire.length=0;r.core.web_midi_cancel();r.bridge.tick();
 r.router.receive(r.out.at(-1));r.router.receive(batch);await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,0]]);assert.equal(r.router.cancelled,1);
 const recent=r.out.findIndex(m=>m.type==='midi-batch');
 assert.ok(r.out.slice(0,recent).some(m=>m.type==='midi-state'&&m.epoch===0),'state precedes first batch');
}
{
 const r=rig();await r.act(()=>r.router.connect());await r.act(()=>r.router.select('A'));
 r.router.updateFrame({...frame({playing:true}),inputTicket:10});
 r.router.input({type:'buttons',mask:1024,inputTicket:11});
 r.core.put(on,zero);r.router.pump();await r.flush();
 assert.deepEqual(r.a.sent,[[146,60,0]],'local Stop intent cancels an old batch before epoch notice');
 r.router.updateFrame({...frame({playing:false}),inputTicket:10});assert.equal(r.router.attackHold,11,'old frame cannot acknowledge new intent');
 r.router.updateFrame({...frame({playing:true}),inputTicket:11});assert.equal(r.router.attackHold,null,'overlay/no-stop C acknowledgement releases guard');
 r.core.put(on);r.router.pump();await r.flush();assert.deepEqual(r.a.sent.at(-1),[146,60,90]);
 r.router.input({type:'buttons',mask:0,inputTicket:12});r.router.input({type:'buttons',mask:1024,inputTicket:13});
 r.router.updateFrame({...frame(),inputTicket:13});assert.equal(r.router.attackHold,null);
 r.router.input({type:'buttons',mask:0,inputTicket:14});r.router.input({type:'buttons',mask:1024,inputTicket:15});assert.equal(r.router.attackHold,null,'Start does not suppress attacks');
 r.router.updateFrame({...frame({playing:true}),inputTicket:15});
 r.router.input({type:'buttons',mask:0,inputTicket:0xfffffffe});r.router.input({type:'buttons',mask:1024,inputTicket:0xffffffff});
 r.router.updateFrame({...frame({playing:true}),inputTicket:0xfffffffe});assert.equal(r.router.attackHold,0xffffffff);
 r.router.updateFrame({...frame(),inputTicket:0});assert.equal(r.router.attackHold,0xffffffff,'reserved zero is never an applied ticket');
 r.router.updateFrame({...frame(),inputTicket:1});assert.equal(r.router.attackHold,null,'ticket acknowledgement crosses u32 wrap, skipping0');
}
{
 const absent=new MidiRouter({send:()=>{},navigator:{}});await absent.connect();assert.match(absent.status,/unavailable/);
 const denied=new MidiRouter({send:()=>{},navigator:{requestMIDIAccess:async()=>{throw Error('denied');}}});
 await denied.connect();assert.match(denied.status,/denied/);
 const noports=new EventTarget();noports.outputs=new Map();
 const empty=new MidiRouter({send:()=>{},navigator:{requestMIDIAccess:async()=>noports}});
 await empty.connect();assert.match(empty.status,/no connected outputs/);
 const r=rig();r.router.updateFrame(frame({cap:255}));assert.equal(r.router.engine,false);
 await r.act(()=>r.router.connect());await assert.rejects(()=>r.router.select('A'),/Switch on/);
 r.router.updateFrame(frame({playing:true}));await assert.rejects(()=>r.router.select('A'),/stop playback/);
}
{
 // Production worklet forwards tagged buttons and publishes only C's applied
 // watermark, never the most recently received JS message ticket.
 let Processor;globalThis.currentTime=0;
 globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
 globalThis.registerProcessor=(_,ctor)=>{Processor=ctor;};
 await import('../../web/emu/worklet.js');
 const p=new Processor(),c=new Core();let pending=0,applied=0,renders=0,untagged=0;
 p.ex=Object.fromEntries(Object.getOwnPropertyNames(Core.prototype).filter(k=>k!=='constructor').map(k=>[k,c[k].bind(c)]));
 p.mem={buffer:new ArrayBuffer(262144)};
 Object.assign(p.ex,{web_buttons_tagged:(mask,ticket)=>{pending=ticket;},web_buttons:()=>untagged++,web_input_ticket:()=>applied,
   web_render:()=>{if(++renders>=2)applied=pending;},web_out_l:()=>120000,web_out_r:()=>120512,
   web_blits:()=>0,web_lit_buttons:()=>0,web_lit_keys:()=>0,web_dim_keys:()=>0,web_sync_state:()=>0,
   web_seq_bpm:()=>120,web_seq_beat:()=>0,web_fb:()=>0,web_store_writes:()=>0,web_nobj:()=>0});
 await p.onMessage({type:'buttons',mask:1024,inputTicket:7});assert.equal(pending,7);
 const outputs=[[new Float32Array(128),new Float32Array(128)]];
 currentTime=.04;p.process([],outputs);assert.equal(p.messages.filter(m=>m.type==='frame').at(-1).inputTicket,0);
 currentTime=.08;p.process([],outputs);assert.equal(p.messages.filter(m=>m.type==='frame').at(-1).inputTicket,7);
 await p.onMessage({type:'buttons',mask:0,inputTicket:-1});assert.equal(untagged,1);
 p.midi.route=1;p.midi.ready=true;c.ready=1;c.put(on);currentTime=.12;p.process([],outputs);
 assert.equal(p.messages.filter(m=>m.type==='midi-batch').at(-1).packets[0],on,'fresh packets dispatch from audio render without UI polling');
}
console.log('MIDI router: permission, byte lengths, bounded flights, prefix ack, partial failure, cancellation, old-output cleanup, delayed open/ACK/close disconnect and engine-replacement races, explicit retry/switch and pagehide PASS');

