// SPDX-License-Identifier: GPL-3.0-only
// Actual production slice UI module and minimal DOM lifecycle surface.
// This is not browser drawing, AudioContext or genuine IndexedDB acceptance.
import assert from 'node:assert/strict';
class E{constructor(){this.disabled=false;this.value='';this.checked=false;this.textContent='';this.children=[];this.files=[];this.dataset={};this.classList={toggle(){}};}replaceChildren(...children){this.children=children;}querySelectorAll(){return this.children;}getContext(){return{fillRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){}};}}
const ids=['slice-status','slice-preview-play','slice-export','slice-save','slice-preview-stop','slice-sync-play','slice-wave','slice-list','slice-info','slice-audio','slice-markers','slice-import-button','slice-demo','slice-source-bpm','slice-repeat','slice-layer','slice-volume','slice-tempo-set','slice-seq-bpm','slice-reopen','slice-export-json','slice-export-output','slice-playhead','slice-sync-status'];
const elements=new Map(ids.map(id=>[id,new E()]));elements.get('slice-wave').width=100;elements.get('slice-wave').height=40;
globalThis.document={getElementById:id=>{assert.ok(elements.has(id),id);return elements.get(id);},createElement:()=>new E(),activeElement:null};
globalThis.window=new EventTarget();globalThis.location={search:''};const requests=[];window.fm1SliceSend=m=>requests.push(m);
await import('../../web/emu/slice-ui.js');const service=window.fm1SliceControl,$=id=>elements.get(id);
const event=m=>{const e=new Event('fm1-marked');e.detail=m;window.dispatchEvent(e);};
const frame=(extra={})=>event({type:'frame',ccInput:{slice:{version:0x10001,revision:1,loaded:true,count:6,repeat:true,sourceBpm:null,gain:.5,layer:false,gate:0,owned:false,...extra}}});
const flush=async()=>{for(let i=0;i<10;i++)await Promise.resolve();};
await $('slice-demo').onclick();assert.equal($('slice-list').children.length,6);assert.ok($('slice-list').children.every(b=>b.disabled),'prepared while powered off');
window.dispatchEvent(new Event('fm1-ready'));assert.ok($('slice-list').children.every(b=>b.disabled),'engine ready alone is not loaded');
frame({loaded:false});await flush();assert.equal(requests.at(-1).op,'load');assert.ok($('slice-list').children.every(b=>b.disabled),'restore in flight');
const request=requests.at(-1);event({type:'slice-reply',request:request.request,result:0,state:{version:0x10001,revision:2,loaded:true,count:6,repeat:true,gain:.5,layer:false,gate:0,owned:false}});await flush();assert.ok($('slice-list').children.every(b=>!b.disabled),'same retained model becomes auditionable after restore');
frame({owned:true});assert.ok($('slice-list').children.every(b=>b.disabled),'owner gates list');frame();assert.ok($('slice-list').children.every(b=>!b.disabled));
$('slice-list').children[0].onclick();await flush();assert.equal(requests.at(-1).op,'pad');assert.ok($('slice-list').children.every(b=>b.disabled),'operation in flight gates every list button');
const pad=requests.at(-1);event({type:'slice-reply',request:pad.request,result:0,state:service.state});await flush();assert.ok($('slice-list').children.every(b=>!b.disabled));
event({type:'cc-input-reset'});assert.ok($('slice-list').children.every(b=>b.disabled));frame();assert.ok($('slice-list').children.every(b=>!b.disabled),'failed replacement retained-engine frame restores list');
frame({gate:16});assert.ok($('slice-list').children.every(b=>b.disabled),'native job gate');frame({version:0});assert.ok($('slice-list').children.every(b=>b.disabled),'old ABI never enables new audition controls');
console.log('PASS actual slice UI DOM lifecycle: staged offline, first-ready/restore, same-model enable, busy/owner/native-job, failed reload and old ABI');
