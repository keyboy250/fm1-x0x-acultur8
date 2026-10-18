// SPDX-License-Identifier: GPL-3.0-only
// Contract test with bounded fake C ingress. Actual compiled-WASM integration
// is separately required; this test cannot establish canonical application.
import assert from 'node:assert/strict';
import {ccPacket, CcInputWorkletBridge} from '../../web/emu/cc-input-worklet.mjs';
const output=[], accepted=[];
let capacity=3, resets=0, version=0x00010001, ingressCalls=0;
let ex={web_cc_version:()=>version, web_cc_reset_source:s=>{assert.equal(s,2);resets++;},
  web_midi_ingest:(p,s)=>{ingressCalls++;assert.equal(s,2);if(accepted.length>=capacity)return -3;accepted.push(p);return 0;}};
const bridge=new CcInputWorkletBridge(()=>ex,m=>output.push(m));
const send=m=>{assert.equal(bridge.message(m),true);return output.pop();};
const route=(n,ready=true)=>send({type:'cc-input-route',request:1,route:n,ready});
const batch=(n,packets)=>send({type:'cc-input-batch',request:2,route:n,packets});
const packets=Array.from({length:6},(_,i)=>ccPacket([0xb0+i,10,i]));
assert.equal(bridge.message({type:'midi-route'}),false,'output namespace untouched');
for(const value of [[0x90,60,90],[0xf8],[0xb0,120,1],[0xb0,127,1],[0xb0,10,128],[0xb0,-1,1],[0xb0,10,.5],[0xb0,10,1,0],null])assert.equal(ccPacket(value),null);
for(const value of [{length:3},{0:0xb0,1:10,2:1,length:3},new DataView(new ArrayBuffer(3)),new Float32Array([0xb0,1,1.5]),new BigInt64Array([176n,1n,1n])])assert.equal(ccPacket(value),null,'unsupported or invalid array shape is rejected without throwing');
assert.equal(ccPacket(new Uint8Array([0xbf,119,127])),0x7f77bf0b);
assert.equal(batch(1,packets).result,-3);assert.equal(accepted.length,0);
assert.equal(route(1).result,0);assert.equal(resets,1);
assert.equal(route(1).result,0);assert.equal(resets,1,'duplicate route does not clear context');
assert.deepEqual(batch(1,packets),{type:'cc-input-reply',request:2,route:1,result:0,prefix:3,ready:true});
assert.deepEqual(accepted,packets.slice(0,3));
assert.equal(batch(1,packets.slice(3)).prefix,0);capacity=6;
assert.equal(batch(1,packets.slice(3)).prefix,3);assert.deepEqual(accepted,packets);
for(const invalid of [[],Array(17).fill(packets[0]),[packets[0],0xffffffff],[packets[0] | 0x10],[ccPacket([0xb0,1,1])+1],Array(1),Array(16),[packets[0],,packets[1]],[,packets[0]],[packets[0],,]]){
  const before=[accepted.length,ingressCalls,resets,bridge.route,bridge.ready];assert.equal(batch(1,invalid).result,-1);
  assert.deepEqual([accepted.length,ingressCalls,resets,bridge.route,bridge.ready],before,'whole invalid/sparse batch rejected before any C ingress, reset or route mutation');
}
for(const patch of [{request:-1},{request:2**32},{route:-1},{route:1.5},{ready:1}]){
  const n=resets;assert.equal(send({type:'cc-input-route',request:1,route:1,ready:true,...patch}).result,-1);assert.equal(resets,n);
}
assert.equal(route(2,false).result,0);assert.equal(batch(1,[packets[0]]).result,-3);
assert.equal(route(1).result,-3);assert.equal(route(3).result,0);assert.equal(resets,3);
bridge.reset();assert.equal(bridge.ready,false);assert.equal(batch(3,[packets[0]]).result,-3);
assert.equal(route(3).result,-3,'engine replacement requires a new route generation');
assert.equal(route(4).result,0);assert.equal(batch(3,[packets[0]]).result,-3);
ex=null;bridge.reset();assert.equal(route(5).result,-2);assert.equal(route(5,false).result,0);
ex={web_cc_version:()=>version,web_cc_reset_source:()=>resets++,web_midi_ingest:()=>0};
version=0x00020001;assert.equal(route(6).result,-2);version=0x00010001;
assert.equal(route(6).result,0);assert.equal(batch(6,[packets[0]]).prefix,1);
for(const result of [-1,-2,-3,1,undefined]){
  let calls=0;ex.web_midi_ingest=()=>++calls===1?0:result;
  assert.equal(batch(6,packets.slice(0,3)).prefix,1,'only exact CC_APPLIED=0 acknowledges the accepted prefix');
  assert.equal(calls,2,'rejected/busy/unknown return never advances to the suffix');
}
bridge.route=0xffffffff;bridge.ready=false;assert.equal(route(0).result,0,'u32 route wraps forward');
assert.equal(route(0xffffffff).result,-3,'old generation rejected across wrap');
console.log('CC input worklet contract: bounded prefix receipts, channel-mode exclusion, route generations, context reset and no output echo PASS (fake C only)');
