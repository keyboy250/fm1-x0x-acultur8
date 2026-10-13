// SPDX-License-Identifier: GPL-3.0-only
// Production router + worklet bridge, deterministic Web MIDI and bounded fake C.
// No physical device, browser permission or compiled-WASM application proof.
import assert from 'node:assert/strict';
import {CcInputRouter} from '../../web/emu/cc-input-router.mjs';
import {CcInputWorkletBridge,ccPacket} from '../../web/emu/cc-input-worklet.mjs';
const gate=()=>{let resolve,reject;const promise=new Promise((yes,no)=>{resolve=yes;reject=no;});return {promise,resolve,reject};};
class Port extends EventTarget{
 constructor(id){super();this.id=id;this.name=id;this.state='connected';this.opens=0;this.closes=0;this.openHook=null;}
 async open(){this.opens++;if(this.openHook)await this.openHook();return this;}
 async close(){this.closes++;}
 emit(data){const e=new Event('midimessage');e.data=data;this.dispatchEvent(e);}
 send(){throw Error('CC input must never send to MIDI output');}
}
function harness(){
 const access=new EventTarget(),A=new Port('A'),B=new Port('B');access.inputs=new Map([['A',A],['B',B]]);access.outputs=new Map();
 const inbox=[],replies=[],sent=[],ingress=[],timers=new Map();let serial=0,time=0,permissions=0,occupancy=0,capacity=64,resets=0;
 const ex={web_cc_version:()=>0x10001,web_cc_reset_source:s=>{assert.equal(s,2);resets++;occupancy=0;},web_midi_ingest:(p,s)=>{assert.equal(s,2);if(occupancy>=capacity)return 0;occupancy++;ingress.push(p);return 1;}};
 const bridge=new CcInputWorkletBridge(()=>ex,m=>replies.push(m));
 const router=new CcInputRouter({send:m=>{sent.push(m);inbox.push(m);},requestAccess:async()=>{permissions++;return access;},
  schedule:(fn,ms)=>{const id=++serial;timers.set(id,{fn,at:time+ms});return id;},cancel:id=>timers.delete(id)});
 const flush=async()=>{for(let n=0;n<30;n++){while(inbox.length)bridge.message(inbox.shift());while(replies.length)router.message(replies.shift());await Promise.resolve();}};
 const advance=async ms=>{const end=time+ms;let count=0;while(true){const next=[...timers].filter(([,v])=>v.at<=end).sort((a,b)=>a[1].at-b[1].at)[0];if(!next)break;assert.ok(count++<500,'bounded fake timer drain');time=next[1].at;timers.delete(next[0]);next[1].fn();await flush();}time=end;};
 const frame=()=>router.engineState({version:0x10001,route:bridge.route,ready:bridge.ready,pending:occupancy,stats:[0,0,0,0,0,0]});
 const activate=async(id='A')=>{const p=router.activate(id);await flush();assert.equal(await p,true);};
 const connect=async()=>{await router.connect();frame();await activate();};
 return {router,bridge,access,A,B,ex,sent,inbox,replies,ingress,timers,flush,advance,frame,activate,connect,
  permissions:()=>permissions,resets:()=>resets,capacity:n=>capacity=n,drain:()=>occupancy=0};
}
{
 const h=harness();assert.equal(h.permissions(),0);assert.equal(h.router.active,false);
 assert.equal(await h.router.activate('A'),false);assert.equal(h.permissions(),0);
 await h.connect();assert.equal(h.permissions(),1);assert.equal(h.A.opens,1);assert.equal(h.router.active,true);
 for(const invalid of [[0x90,60,90],[0x80,60,0],[0xf8],[0xb0,120,0],[0xbf,127,127],[0xb0,1,128],{length:3}])h.A.emit(invalid);
 await h.flush();assert.equal(h.ingress.length,0);assert.equal(h.router.ignored,7);
 const source=Array.from({length:24},(_,i)=>[0xb0+(i%16),i,10+i]);h.capacity(3);source.forEach(v=>h.A.emit(new Uint8Array(v)));
 assert.equal(h.router.queue.length,24);assert.equal(h.sent.filter(m=>m.type==='cc-input-batch').length,1,'one flight only');
 await h.flush();await h.advance(20);assert.deepEqual(h.ingress,source.slice(0,3).map(ccPacket));assert.equal(h.router.queue.length,21);
 const stale=h.sent.filter(m=>m.type==='cc-input-batch').at(-1);assert.equal(stale.packets.length,16);
 h.router.message({type:'cc-input-reply',request:stale.request,route:stale.route,result:0,prefix:2,ready:true});assert.equal(h.router.queue.length,21,'duplicate accepted-prefix receipt ignored');
 await h.advance(20);assert.equal(h.ingress.length,3,'full C queue returns zero prefix');
 h.capacity(4);for(let i=0;i<8&&h.router.queue.length;i++){h.drain();await h.advance(20);}
 assert.deepEqual(h.ingress,source.map(ccPacket),'partial retry never replays accepted prefix');assert.equal(h.router.queue.length,0);assert.equal(h.router.accepted,24);assert.equal(h.router.flight,null);
 assert.ok(h.sent.filter(m=>m.type==='cc-input-batch').every(m=>m.packets.length<=16));
 await h.router.disconnect();await h.flush();assert.equal(h.router.active,false);assert.equal(h.A.closes,1);assert.equal(h.permissions(),1);
}
{
 const h=harness();await h.connect();const oldHandler=h.router.handler,oldRoute=h.router.route;
 for(let i=0;i<65;i++)h.A.emit([0xb0,1,i%128]);
 assert.equal(h.router.active,false);assert.equal(h.router.queue.length,0);assert.match(h.router.status,/overflow/);assert.ok(h.router.route!==oldRoute);
 oldHandler({data:[0xb0,1,100]});assert.equal(h.router.queue.length,0,'stale port event cannot revive overflowed input');
 await h.flush();assert.equal(h.bridge.ready,false);const accepted=h.ingress.length;
 await h.activate();await h.flush();assert.equal(h.ingress.length,accepted,'explicit retry never replays discarded/ambiguous data');
 oldHandler({data:[0xb0,1,101]});await h.flush();assert.equal(h.ingress.length,accepted,'old callback remains invalid after the new route becomes active');
 h.A.emit([0xb0,1,7]);await h.flush();assert.equal(h.ingress.at(-1),ccPacket([0xb0,1,7]));
}
{
 const h=harness();await h.connect();h.A.emit([0xb0,3,7]);
 const batch=h.inbox.shift();h.bridge.message(batch);const lost=h.replies.shift();assert.equal(lost.prefix,1,'C accepted but main thread did not receive receipt');
 await h.advance(1000);assert.equal(h.router.active,false);assert.match(h.router.status,/acceptance.*unknown/);assert.equal(h.router.queue.length,0);assert.equal(h.bridge.ready,false);
 const total=h.ingress.length;h.router.message(lost);assert.equal(h.router.accepted,0,'late receipt does not revive old flight');await h.activate();assert.equal(h.ingress.length,total);
}
{
 const h=harness();await h.connect();h.capacity(0);h.A.emit([0xb0,3,7]);await h.flush();await h.advance(31*20);
 assert.equal(h.router.active,false);assert.match(h.router.status,/remained full/);assert.equal(h.bridge.ready,false);assert.equal(h.router.queue.length,0);
}
{
 const h=harness();await h.connect();h.A.emit([0xb0,4,7]);const packet=h.inbox.shift();
 for(const result of [1,-4,0.5,Infinity,Number.MAX_SAFE_INTEGER])assert.equal(h.router.message({type:'cc-input-reply',request:packet.request,route:packet.route,result,prefix:1,ready:true}),false,'invalid result scalar cannot acknowledge a flight');
 assert.equal(h.router.queue.length,1);assert.equal(h.router.accepted,0);
 for(const stats of [Array(6),[0,1,,3,4,5],[0,1,2,3,4,2**32]]){h.router.engineState({version:0x10001,route:h.router.route,ready:true,pending:0,stats});assert.deepEqual(h.router.stats,[],'malformed/sparse statistics rejected');}
 assert.equal(h.router.message({type:'cc-input-reply',request:packet.request,route:(packet.route+1)>>>0,result:0,prefix:1,ready:true}),false);
 assert.equal(h.router.message({type:'cc-input-reply',request:packet.request+1,route:packet.route,result:0,prefix:1,ready:true}),false);
 h.router.message({type:'cc-input-reply',request:packet.request,route:packet.route,result:0,prefix:2,ready:true});await h.flush();assert.equal(h.router.active,false);assert.match(h.router.status,/Invalid/);assert.equal(h.router.queue.length,0);
}
for(const boundary of ['reset','open','ready']){
 const h=harness();await h.router.connect();h.frame();const wait=gate();if(boundary==='open')h.A.openHook=()=>wait.promise;
 const activating=h.router.activate('A');
 if(boundary==='open')await h.flush();else if(boundary==='ready'){
  // Deliver reset ACK, then allow open to finish but retain the enable RPC.
  for(let i=0;i<12;i++){if(h.inbox[0]?.ready===false)h.bridge.message(h.inbox.shift());while(h.replies.length)h.router.message(h.replies.shift());await Promise.resolve();}
  assert.ok(h.inbox.some(m=>m.ready===true));
 }
 const oldHandler=h.router.handler;h.A.state='disconnected';h.access.dispatchEvent(new Event('statechange'));await h.flush();assert.equal(await activating,false);assert.equal(h.router.operation,null);assert.equal(h.router.active,false);
 h.A.state='connected';h.access.dispatchEvent(new Event('statechange'));await h.flush();assert.equal(h.A.opens,boundary==='reset'?0:1,'reconnect never opens automatically');
 h.A.openHook=null;await h.activate();wait.resolve();await h.flush();assert.equal(h.router.active,true);assert.equal(h.A.closes,0,'late old open cannot close the new same-port activation');
 if(oldHandler)oldHandler({data:[0xb0,1,1]});assert.equal(h.router.queue.length,0);
}
for(const boundary of ['reset','ready']){
 const h=harness();await h.router.connect();h.frame();const activation=h.router.activate('A');
 if(boundary==='ready')for(let i=0;i<12;i++){if(h.inbox[0]?.ready===false)h.bridge.message(h.inbox.shift());while(h.replies.length)h.router.message(h.replies.shift());await Promise.resolve();}
 const command=h.inbox.shift();assert.equal(command.ready,boundary==='ready');h.bridge.message(command);const lost=h.replies.shift();
 await h.advance(1000);assert.equal(await activation,false);assert.equal(h.router.operation,null);assert.equal(h.router.active,false);assert.equal(h.bridge.ready,false);
 h.router.message(lost);assert.equal(h.router.active,false,'late control success cannot reenable an acknowledged-timeout route');
 await h.activate();assert.equal(h.router.active,true);assert.equal(h.permissions(),1);
}
{
 const h=harness();await h.router.connect();h.frame();const wait=gate();h.A.openHook=()=>wait.promise;
 const first=h.router.activate('A');await h.flush();await h.router.disconnect();await h.flush();assert.equal(await first,false);
 await h.activate('B');const opens=h.B.opens;wait.resolve();await h.flush();assert.equal(h.router.input,h.B);assert.equal(h.router.active,true);assert.equal(h.B.opens,opens);assert.ok(h.A.closes>=1);
 h.A.emit([0xb0,1,1]);await h.flush();assert.equal(h.ingress.length,0,'old port never reaches new route');
 h.B.emit([0xb1,1,2]);await h.flush();assert.equal(h.ingress.at(-1),ccPacket([0xb1,1,2]));
}
for(const boundary of ['reset','open','ready']){
 const h=harness();await h.router.connect();h.frame();const wait=gate();if(boundary==='open')h.A.openHook=()=>wait.promise;
 const first=h.router.activate('A');
 if(boundary==='open')await h.flush();else if(boundary==='ready')for(let i=0;i<12;i++){if(h.inbox[0]?.ready===false)h.bridge.message(h.inbox.shift());while(h.replies.length)h.router.message(h.replies.shift());await Promise.resolve();}
 h.bridge.reset();h.router.engineReset();await h.flush();assert.equal(await first,false);assert.equal(h.router.operation,null);assert.equal(await h.router.retry(),false,'reset requires fresh frame');
 h.A.openHook=null;h.frame();await h.activate();wait.resolve();await h.flush();assert.equal(h.router.active,true);assert.equal(h.router.input,h.A);assert.equal(h.permissions(),1);
}
{
 const h=harness();await h.router.connect();h.frame();const wait=gate();h.A.openHook=()=>wait.promise;const activation=h.router.activate('A');await h.flush();
 const replacement=new Port('A');h.access.inputs.set('A',replacement);wait.resolve();await h.flush();assert.equal(await activation,false,'identity replacement is rejected after open without relying on statechange');assert.equal(h.router.active,false);
 await h.activate('A');assert.equal(h.router.input,replacement);assert.equal(replacement.opens,1);assert.equal(h.A.opens,1);
}
{
 const h=harness();await h.router.connect();h.frame();h.router.route=0xffffffff;h.bridge.route=0xffffffff;h.router.serial=0xffffffff;await h.activate();
 assert.equal(h.router.route,1);assert.ok(h.sent.every(m=>m.request!==0));
 h.router.pagehide();await h.flush();assert.equal(h.router.active,false);assert.equal(h.router.queue.length,0);assert.equal(h.bridge.ready,false);
}
for(const failure of [undefined,Error('Permission denied')]){
 let calls=0;const r=new CcInputRouter({send(){throw Error('No sends expected');},requestAccess:async()=>{calls++;if(failure)throw failure;return undefined;}});
 assert.equal(calls,0);await r.connect();assert.equal(calls,1);assert.equal(r.active,false);assert.match(r.status,/unavailable/);
}
{
 const h=harness();h.access.inputs.clear();await h.router.connect();assert.match(h.router.status,/no input ports/);assert.equal(h.router.active,false);
 h.router.engineState(undefined);assert.equal(h.router.supported,false);assert.equal(await h.router.activate('A'),false);
}
console.log('CC input router+bridge: explicit permission, CC-only/no echo,64/16 bounds,prefix retry/duplicates,overflow/timeout/reset,disconnect/open/lateACK/switch races,old-event rejection,wrap and initial/old-engine fallback PASS (fake C/API only)');
