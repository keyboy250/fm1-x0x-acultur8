// SPDX-License-Identifier: GPL-3.0-only
// Execute the production pad UI against a minimal DOM and actual C-ABI-shaped messages.
import fs from 'node:fs';import assert from 'node:assert/strict';
class Element extends EventTarget {
 constructor(){super();this.value='';this.checked=false;this.disabled=false;this.dataset={};this.children=[];this.textContent='';this.classList={toggle(){}};this.type='';}
 append(...items){this.children.push(...items);}
 replaceChildren(...items){this.children=items;this.value=items[0]?.value??'';}
}
const html=fs.readFileSync('web/emu/pad-panel.html','utf8'),elements=new Map();
for(const tag of html.matchAll(/<[^>]+\bid="([^"]+)"[^>]*>/g)){
 const element=new Element();element.type=/type="([^"]+)"/.exec(tag[0])?.[1]??'';
 const field=/data-pad-field="(\d+)"/.exec(tag[0]);if(field)element.dataset.padField=field[1];
 elements.set(tag[1],element);
}
const input=[...elements.values()].filter(e=>e.dataset.padField!==undefined);
globalThis.document={getElementById:id=>{assert.ok(elements.has(id),id);return elements.get(id);},createElement:()=>new Element(),querySelector:()=>({querySelectorAll:()=>input})};
globalThis.window=new EventTarget();const sent=[];window.fm1SliceSend=m=>sent.push(m);
let permissions=0;Object.defineProperty(globalThis,'navigator',{configurable:true,value:{requestMIDIAccess:async()=>{permissions++;throw Error('test denial');}}});
globalThis.CustomEvent=class extends Event{constructor(type,init){super(type);this.detail=init.detail;}};
await import('../../web/emu/pad-ui.js');
const $=id=>elements.get(id),dispatch=m=>window.dispatchEvent(new CustomEvent('fm1-marked',{detail:m}));
const pad=[2,60,48,84,1,1,0,2,60,64,67,60,64,67,0,1];
const frame=(cap=511,read=0,revision=1,values=pad,playing=false)=>({type:'frame',sync:{playing,bpm:120},variant:{info:{abi:1,revision,values:[cap,1,0,read,read]},pad:{abi:1,section:8,revision,values},chord:{values:[1,0,4,7,60,64,67]},bass:[]}});
assert.equal(permissions,0);assert.equal($('pad-controls').disabled,true);
dispatch(frame(255));assert.equal($('pad-controls').disabled,true);assert.match($('pad-availability').textContent,/unavailable/);
dispatch({type:'midi-state',protocol:0x00020001,route:0,epoch:0,transportEpoch:0,pending:0,ready:false});
dispatch(frame());assert.equal($('pad-controls').disabled,false);
assert.equal($('pad-channel').value,'2');assert.equal($('pad-root').value,'60');assert.equal($('pad-low').value,'48');assert.equal($('pad-high').value,'84');assert.equal($('pad-notes-out').checked,true);
assert.match($('pad-engine-state').textContent,/60, 64, 67 on channel 3/);assert.match($('pad-engine-state').textContent,/enqueue acceptance, not physical delivery/);
const diagnostic=[2,61,0,127,0,0,5,0,0,4,7,12,16,19,1,0];dispatch(frame(511,0,1,diagnostic));
assert.match($('pad-engine-state').textContent,/MIDI note output disabled/);assert.match($('pad-engine-state').textContent,/Effective eligible: false/);
assert.match($('pad-engine-state').textContent,/0, 4, 7 on channel 1/);assert.match($('pad-engine-state').textContent,/Desired triad 12, 16, 19/);
assert.match($('pad-engine-state').textContent,/Pending cleanup\/chord update: true/);assert.match($('pad-engine-state').textContent,/transport ready: false/);dispatch(frame());
$('pad-channel').value='3';$('pad-channel').dispatchEvent(new Event('change'));
assert.deepEqual(sent.slice(-3).map(m=>m.op),[3,14,4]);assert.equal(sent.at(-2).value,3);
assert.ok(sent.at(-2).request>=0xc0000000);assert.equal($('pad-controls').disabled,true);
assert.equal($('pad-channel').value,'2','no optimistic setting readback');
for(const m of sent.slice(-3))dispatch({type:'variant-reply',request:m.request,result:m.op===14?1:0,ticket:99,revision:2});
dispatch(frame(511,98,2));assert.equal($('pad-controls').disabled,true,'queued ticket keeps controls locked');
const changed=[...pad];changed[0]=3;dispatch(frame(511,99,2,changed));assert.equal($('pad-controls').disabled,false);assert.equal($('pad-channel').value,'3');
$('pad-notes-out').checked=false;$('pad-notes-out').dispatchEvent(new Event('change'));
assert.equal(sent.at(-2).op,15);assert.equal(sent.at(-2).value,0);
for(const m of sent.slice(-3))dispatch({type:'variant-reply',request:m.request,result:m.op===15?-3:0,ticket:99,revision:2});
assert.match($('pad-edit-status').textContent,/Stop playback/);assert.equal($('pad-notes-out').checked,true);
dispatch(frame(511,99,2,pad,true));const n=sent.length;$('pad-channel').value='4';$('pad-channel').dispatchEvent(new Event('change'));assert.equal(sent.length,n);
dispatch(frame());$('pad-root').value='128';$('pad-root').dispatchEvent(new Event('input'));$('pad-root').dispatchEvent(new Event('change'));assert.equal(sent.length,n);assert.match($('pad-edit-status').textContent,/whole MIDI-note/);
assert.equal(permissions,0);$('pad-connect').dispatchEvent(new Event('click'));await Promise.resolve();await Promise.resolve();
assert.equal(permissions,1);assert.match($('pad-route-status').textContent,/test denial/);
{
 // Exercise the production button predicate after a late ready ACK, rather
 // than assuming router.active=false alone makes the actual Retry reachable.
 const access=new EventTarget(),port={id:'A',name:'A',state:'connected',opens:0,
   async open(){this.opens++;return this;},async close(){},send(){throw Error('No MIDI sends expected in this race');}};
 access.outputs=new Map([['A',port]]);navigator.requestMIDIAccess=async()=>{permissions++;return access;};
 const ticks=async()=>{for(let i=0;i<12;i++)await Promise.resolve();};
 $('pad-connect').dispatchEvent(new Event('click'));await ticks();
 $('pad-output').value='A';$('pad-output').dispatchEvent(new Event('change'));
 $('pad-route').dispatchEvent(new Event('click'));await ticks();
 const ready=sent.filter(m=>m.type==='midi-route'&&m.ready).at(-1);assert.ok(ready);
 assert.equal($('pad-retry').disabled,true,'activation is awaiting acknowledgement');
 port.state='disconnected';access.dispatchEvent(new Event('statechange'));
 dispatch({type:'midi-route-reply',protocol:0x00020001,request:ready.request,route:ready.route,result:0,ready:true});await ticks();
 assert.equal($('pad-retry').disabled,false,'late success does not strand the real Retry button');
 port.state='connected';access.dispatchEvent(new Event('statechange'));await ticks();
 assert.equal(port.opens,1,'reconnect does not reopen');assert.equal($('pad-retry').disabled,false);
 assert.equal($('pad-disconnect').disabled,true,'failed route remains inactive');
 $('pad-retry').dispatchEvent(new Event('click'));await ticks();
 assert.equal(port.opens,2,'only explicit Retry opens the original port again');
 const retry=sent.filter(m=>m.type==='midi-route'&&m.ready).at(-1);assert.ok(retry.request>ready.request);
 assert.equal(retry.route,ready.route,'retry retains the original route for cleanup');
 // Resolve this final request as rejected so the test leaves no pending RPC.
 dispatch({type:'midi-route-reply',protocol:0x00020001,request:retry.request,route:retry.route,result:-3,ready:false});await ticks();
 assert.equal($('pad-retry').disabled,false);assert.equal(permissions,2);
 for(const boundary of ['ready','cancel','open']){
   let release;const gate=new Promise(resolve=>release=resolve);
   port.open=boundary==='open'?async()=>{port.opens++;await gate;return port;}:async()=>{port.opens++;return port;};
   $('pad-retry').dispatchEvent(new Event('click'));await ticks();
   let obsolete=sent.filter(m=>m.type==='midi-route'&&m.ready).at(-1);
   if(boundary==='cancel'){
     dispatch({type:'midi-route-reply',protocol:0x00020001,request:obsolete.request,route:obsolete.route,result:0,ready:true});await ticks();
     obsolete=sent.filter(m=>m.type==='midi-cancel').at(-1);assert.ok(obsolete);
   }
   assert.equal($('pad-retry').disabled,true);const opens=port.opens;
   window.dispatchEvent(new Event('fm1-ready'));
   dispatch({type:'midi-output-capability',protocol:0x00020001,supported:true});await ticks();
   assert.equal($('pad-retry').disabled,true,'replacement needs a fresh supported stopped frame');
   dispatch({type:'midi-route-reply',protocol:0x00020001,request:obsolete.request,route:obsolete.route,result:0,ready:true});await ticks();
   assert.equal($('pad-disconnect').disabled,true,'old reply cannot enable the replacement route');
   dispatch(frame());await ticks();assert.equal($('pad-retry').disabled,false,boundary+' cannot strand Retry after reset');
   assert.equal(port.opens,opens,'engine replacement never reopens automatically');
   port.open=async()=>{port.opens++;return port;};$('pad-retry').dispatchEvent(new Event('click'));await ticks();
   const fresh=sent.filter(m=>m.type==='midi-route'&&m.ready).at(-1);
   assert.ok(fresh.request>obsolete.request);assert.notEqual(fresh.route,obsolete.route);
   dispatch({type:'midi-route-reply',protocol:0x00020001,request:obsolete.request,route:obsolete.route,result:0,ready:true});release();await ticks();
   assert.equal($('pad-retry').disabled,true,'late open/reply cannot clear a new retry lock');
   assert.equal($('pad-disconnect').disabled,true);
   dispatch({type:'midi-route-reply',protocol:0x00020001,request:fresh.request,route:fresh.route,result:-3,ready:false});await ticks();
   assert.equal($('pad-retry').disabled,false);assert.equal(permissions,2);
 }
}
console.log('Pad UI: all16 snapshot fields, cap255 disabled, commands14/15, request namespace, stopped/ticket/revision locks, rejection readback, explicit permission, late-ACK and engine-reset Retry reachability PASS');

