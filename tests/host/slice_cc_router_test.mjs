// SPDX-License-Identifier: GPL-3.0-only
// Production mixed router ordering. Explicit engine/API doubles only.
import assert from 'node:assert/strict';
import {CcInputRouter} from '../../web/emu/cc-input-router.mjs';
const sent=[],applied=[],timers=new Map();let n=0;
const router=new CcInputRouter({send:m=>sent.push(m),schedule:(fn,ms)=>{const id=++n;timers.set(id,{fn,ms});return id;},cancel:id=>timers.delete(id),browserApply:async(p,c)=>{applied.push({p,c});return{status:'applied',reason:'applied'};}});
router.active=true;router.route=7;router.supported=true;router.fresh=true;
const packet=(cc,v=127)=>[0xbc,cc,v];
const frame=(pending=0,gate=0,source=Array(32).fill(0))=>router.engineState({version:0x10001,route:7,ready:true,pending,source,slice:{version:0x10001,gate},stats:Array(6).fill(0)});
const tick=async()=>{for(let i=0;i<8;i++)await Promise.resolve();};
const retry=()=>{const t=[...timers].find(([,v])=>v.ms===20);if(t){timers.delete(t[0]);t[1].fn();}};
frame();router.receive(packet(10));router.receive(packet(89));assert.equal(sent.length,1);assert.equal(applied.length,0);
const flight=router.flight;router.message({type:'cc-input-reply',request:flight.request,route:7,prefix:1,result:0,ready:true});retry();await tick();assert.equal(applied.length,0,'ACK alone cannot release browser operation');
frame(1);await tick();assert.equal(applied.length,0);frame(0,2);await tick();assert.equal(applied.length,0,'pending transport holds order');frame();await tick();assert.equal(applied.length,1);
assert.equal(router.accepted,1);assert.equal(router.browserApplied,1);assert.equal(router.queue.length,0);
// Native exact assembly stays native until fresh source readback; canonical
// browser value uses all21bits. The next native parameter remains native.
router.receive(packet(48,80));router.receive(packet(53));const f=router.flight;
router.message({type:'cc-input-reply',request:f.request,route:7,prefix:1,result:0,ready:true});retry();
const source=Array(32).fill(0);source[12]=1;source[13]=1107;source[14]=7;source[15]=220123;frame(0,0,source);await tick();assert.equal(applied.at(-1).c.exact.value,220123);assert.equal(router.queue.length,0);
router.receive(packet(10,17));assert.equal(router.flight.count,1);
// Route invalidation cannot apply an old queued browser item or count its receipt.
let resolve;router.browserApply=()=>new Promise(r=>resolve=r);router.clearTraffic();router.active=true;router.receive(packet(89));await tick();const count=router.browserApplied;router.invalidate('route changed');resolve({status:'applied',reason:'late'});await tick();assert.equal(router.browserApplied,count);assert.equal(router.queue.length,0);
console.log('PASS production mixed router: native ACK/frame barrier, pending gates, exact21bit, next-native routing, stale receipt; engine double only');
