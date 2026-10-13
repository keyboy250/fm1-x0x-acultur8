// SPDX-License-Identifier: GPL-3.0-only
// Execute production UI/controller with fake DOM and MIDI; permission stays
// behind its explicit button. No hardware or compiled-WASM acceptance claim.
import fs from 'node:fs';import assert from 'node:assert/strict';
class Element extends EventTarget{constructor(){super();this.value='';this.disabled=false;this.children=[];this.textContent='';}replaceChildren(...items){this.children=items;this.value=items[0]?.value??'';}}
const html=fs.readFileSync('web/emu/cc-input-panel.html','utf8'),elements=new Map();
for(const m of html.matchAll(/\bid="([^"]+)"/g))elements.set(m[1],new Element());
globalThis.document={getElementById:id=>{assert.ok(elements.has(id),id);return elements.get(id);},createElement:()=>new Element()};
globalThis.window=new EventTarget();globalThis.CustomEvent=class extends Event{constructor(type,init){super(type);this.detail=init.detail;}};
const sent=[];window.fm1SliceSend=m=>sent.push(m);let permissions=0;
Object.defineProperty(globalThis,'navigator',{configurable:true,value:{requestMIDIAccess:async options=>{permissions++;assert.deepEqual(options,{sysex:false});throw Error('Permission denied');}}});
await import('../../web/emu/cc-input-ui.js');
const $=id=>elements.get(id),fire=(id,event='click')=>$(id).dispatchEvent(new Event(event));
const dispatch=m=>window.dispatchEvent(new CustomEvent('fm1-marked',{detail:m}));
const ticks=async()=>{for(let i=0;i<20;i++)await Promise.resolve();};
const frame=(value={version:0x10001,route:0,ready:false,pending:0,stats:[1,2,3,4,5,6]})=>dispatch({type:'frame',ccInput:value});
const ack=m=>dispatch({type:'cc-input-reply',request:m.request,route:m.route,result:0,prefix:0,ready:m.ready});
assert.equal(permissions,0);assert.equal(sent.length,0);assert.equal($('cc-input-enable').disabled,true);
frame(undefined);dispatch({type:'frame'});assert.match($('cc-input-availability').textContent,/unavailable/);assert.equal(permissions,0);
fire('cc-input-connect');await ticks();assert.equal(permissions,1);assert.match($('cc-input-status').textContent,/Permission denied/);
const access=new EventTarget(),port=new EventTarget();Object.assign(port,{id:'input-A',name:'Test input',state:'connected',opens:0,async open(){this.opens++;return this;},async close(){}});access.inputs=new Map([[port.id,port]]);
navigator.requestMIDIAccess=async options=>{permissions++;assert.deepEqual(options,{sysex:false});return access;};
fire('cc-input-connect');await ticks();assert.equal(permissions,2);assert.equal(port.opens,0);assert.equal($('cc-input-port').disabled,false);
$('cc-input-port').value=port.id;fire('cc-input-port','change');assert.equal($('cc-input-enable').disabled,true,'old engine remains disabled after permission');
frame();assert.equal($('cc-input-enable').disabled,false);assert.match($('cc-input-engine').textContent,/all sources: accepted 1.*overflow 6/);
fire('cc-input-enable');await ticks();let reset=sent.at(-1);assert.equal(reset.type,'cc-input-route');assert.equal(reset.ready,false);ack(reset);await ticks();
let enable=sent.at(-1);assert.equal(enable.ready,true);assert.equal(port.opens,1);assert.equal($('cc-input-retry').disabled,true);
port.state='disconnected';access.dispatchEvent(new Event('statechange'));ack(enable);await ticks();
assert.equal($('cc-input-enable').disabled,true);assert.match($('cc-input-status').textContent,/disconnected/);
port.state='connected';access.dispatchEvent(new Event('statechange'));await ticks();assert.equal(port.opens,1,'reconnect never autoopens');assert.equal($('cc-input-retry').disabled,false,'late ready ACK leaves explicit Retry reachable');
reset=sent.at(-1);ack(reset);await ticks();fire('cc-input-retry');await ticks();reset=sent.at(-1);ack(reset);await ticks();enable=sent.at(-1);ack(enable);await ticks();
assert.match($('cc-input-status').textContent,/CC input enabled/);assert.equal($('cc-input-retry').disabled,true);
const ignored=new Event('midimessage');ignored.data=new Uint8Array([0x90,60,90]);port.dispatchEvent(ignored);assert.match($('cc-input-counts').textContent,/ignored 1/);
const cc=new Event('midimessage');cc.data=new Uint8Array([0xb0,20,7]);port.dispatchEvent(cc);const batch=sent.at(-1);assert.equal(batch.type,'cc-input-batch');
dispatch({type:'cc-input-reply',request:batch.request,route:batch.route,result:0,prefix:1,ready:true});assert.match($('cc-input-counts').textContent,/ingress receipts 1/);
dispatch({type:'cc-input-reset'});assert.equal($('cc-input-enable').disabled,true);assert.equal($('cc-input-retry').disabled,true);
dispatch({type:'cc-input-reply',request:enable.request,route:enable.route,result:0,prefix:0,ready:true});assert.match($('cc-input-status').textContent,/Engine restarted/);
frame();assert.equal($('cc-input-retry').disabled,false);assert.equal(port.opens,2);assert.equal(permissions,2);
assert.match(html,/candidate/);assert.match(html,/partial runtime coverage/);assert.match(html,/physical-device delivery/);assert.match(html,/already accepted or applied settings remain/i);
window.dispatchEvent(new Event('pagehide'));await ticks();assert.equal(sent.at(-1).ready,false);
console.log('CC input UI: no startup permission,old-engine fallback,explicit permission/port/enable,lateACK Retry reachability,counters,load-reset fresh-frame requirement and candidate limitations PASS (fake DOM/API only)');
