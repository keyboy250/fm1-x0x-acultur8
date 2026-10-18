// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const wasm=fs.readFileSync(process.argv[2]||'build/emu/x0x.wasm');let Processor,request=0,route=0;
globalThis.currentTime=0;globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(name,C)=>{assert.equal(name,'x0x');Processor=C;};await import('../../web/emu/worklet.js');
const p=new Processor(),out=[[new Float32Array(128),new Float32Array(128)]];
function render(n=12){for(let i=0;i<n;i++){currentTime+=128/44100;p.process([],out);}}
const state=()=>{const s=p.ex.web_master_state()>>>0;return {q12:s&8191,owner:(s>>>13)&7,pickupPending:!!(s&65536)};};
const packet=raw=>(11|(0xbc<<8)|(76<<16)|(raw<<24))>>>0;
async function send(m){await p.onMessage({...m,request:++request});const r=p.messages.at(-1);assert.equal(r.request,request);return r;}
async function enable(){route++;assert.equal((await send({type:'cc-input-route',route,ready:true})).result,0);}
async function cc(raw){const r=await send({type:'cc-input-batch',route,packets:[packet(raw)]});assert.equal(r.prefix,1);render();assert.equal(state().q12,Math.floor((raw*4096+63)/127));assert.equal(state().owner,4);}
await p.onMessage({type:'load',wasm,master:2800});render(35);assert.deepEqual(state(),{q12:2800,owner:1,pickupPending:false});await enable();
// Actual index listeners/readback; only DOM plumbing is replaced.
const html=fs.readFileSync('web/emu/index.html','utf8');const input=html.slice(html.indexOf('// ---------------------------------------------------------------- state and input'),html.indexOf('// ---------------------------------------------------------------- screen and lights'));
const element=()=>({attrs:new Map(),listeners:new Map(),style:{},classList:{toggle(){}},setAttribute(k,v){this.attrs.set(k,String(v));},getAttribute(k){return this.attrs.get(k);},hasAttribute(k){return this.attrs.has(k);},removeAttribute(k){this.attrs.delete(k);},getBBox(){return {x:0,y:0,width:20,height:20};},insertBefore(){},addEventListener(k,v){this.listeners.set(k,v);},setPointerCapture(){},append(){}});
const knobs={},queue=[];
const el=(tag,attrs)=>{const e=element();for(const [name,value] of Object.entries(attrs))e.setAttribute(name,value);return e;};
const ctx=vm.createContext({device:element(),controls:element(),descs:element(),nDesc:0,$:()=>({checked:true,addEventListener(){}}),prefs:{},savePrefs(){},document:{createElement:element,getElementById:element},el,text(){},TRAY:'tray',KNOB:'knob',LABEL:'label',buttons:{},keys:[],knobs,WHITE_K:[],BLACK_K:[],BTN:{},performance:{now:()=>0},setTimeout(){},addEventListener(){},post:m=>queue.push(p.onMessage(m))});
const constructor=html.slice(html.indexOf('function knob(x, y, r, label, role)'),html.indexOf('knob(105, 100, 24, "MASTER", "master");'));
assert.ok(constructor.startsWith('function knob(x, y, r, label, role)'),'production knob constructor is present');
vm.runInContext(constructor+'\nknob(105,100,24,"MASTER","master");',ctx);const knob=knobs.MASTER;
const hit=html.slice(html.indexOf('function hit(c, kind, name, does)'),html.indexOf('for (const n of ["MASTER", "SELECT", "PRESETS", "ALGORITHM"]) hit'));
assert.ok(hit.startsWith('function hit(c, kind, name, does)'),'production touch target constructor is present');
vm.runInContext(hit+'\nhit(knobs.MASTER,"knob","MASTER","Master gain");',ctx);
assert.equal(knob.hit.attrs.get('role'),'slider');assert.equal(knob.hit.attrs.get('aria-label'),'MASTER');assert.equal(knob.hit.tabIndex,0);
assert.equal(knob.g.attrs.get('aria-hidden'),'true');for(const name of ['role','aria-label','tabindex'])assert.equal(knob.g.attrs.has(name),false,'drawing relinquishes accessible control attributes');
vm.runInContext(input,ctx);vm.runInContext('turn(knobs.MASTER,1)',ctx);assert.equal(vm.runInContext('master',ctx),2864,'power-off knob still sets next boot level');vm.runInContext('node={port:{postMessage:post}}',ctx);
const ui=()=>vm.runInContext('master',ctx);
function sync(){p.publish();const frame=p.messages.at(-1);assert.deepEqual(frame.master,state());ctx.latest=frame.master;vm.runInContext('syncMaster(latest)',ctx);assert.equal(ui(),state().q12);assert.equal(knob.hit.attrs.get('aria-valuenow'),String(state().q12));assert.equal(knob.hit.attrs.get('aria-valuemin'),'0');assert.equal(knob.hit.attrs.get('aria-valuemax'),'4096');for(const name of ['aria-valuenow','aria-valuemin','aria-valuemax'])assert.equal(knob.g.attrs.has(name),false,'hidden drawing is not the readback target');}
async function turn(n){ctx.n=n;vm.runInContext('turn(knobs.MASTER,n)',ctx);await Promise.all(queue.splice(0));}
sync();for(const raw of [0,127,31,94,64]){await cc(raw);sync();}
// Pointer adds to fresh C level, never its stale displayed angle.
const displayed=ui();await cc(31);assert.equal(ui(),displayed);const actual=state().q12;await turn(1);assert.equal(state().q12,actual+64);assert.equal(state().owner,1);assert.equal(ui(),displayed);sync();
await turn(-100);assert.equal(state().q12,0);sync();await turn(100);assert.equal(state().q12,4096);sync();
for(const message of [{type:'master',value:-1},{type:'master',value:4097},{type:'master',value:NaN},{type:'master-step',delta:4097},{type:'master-step',delta:1.5}]){const before=state();await p.onMessage(message);assert.deepEqual(state(),before);}
for(const invalid of [{q12:4097,owner:1,pickupPending:false},{q12:0,owner:7,pickupPending:false},{q12:0,owner:1,pickupPending:1}]){ctx.latest=invalid;const before=ui();vm.runInContext('syncMaster(latest)',ctx);assert.equal(ui(),before);}
await cc(52);const retained=state().q12;route++;assert.equal((await send({type:'cc-input-route',route,ready:false})).result,0);assert.deepEqual(state(),{q12:retained,owner:5,pickupPending:false});sync();
assert.equal((await send({type:'cc-input-batch',route:route-1,packets:[packet(0)]})).result,-3);assert.equal(state().q12,retained);await enable();await cc(100);
// Replacement retains current C level despite stale UI load argument; input
// during asynchronous instantiate cannot change the captured level.
const before=state().q12,old=p.ex,loading=p.onMessage({type:'load',wasm,master:123});assert.equal(p.loading,true);
await p.onMessage({type:'master-step',delta:-4096});await p.onMessage({type:'master',value:0});assert.equal((old.web_master_state()>>>0)&8191,before);
await loading;render();assert.notEqual(p.ex,old);assert.equal(state().q12,before);assert.equal(p.ccInput.ready,false);sync();
await enable();await cc(48);const q=state().q12,engine=p.ex;await assert.rejects(p.onMessage({type:'load',wasm:new Uint8Array([0]),master:0}),WebAssembly.CompileError);assert.equal(p.ex,engine);assert.deepEqual(state(),{q12:q,owner:5,pickupPending:false});sync();
await turn(1);assert.equal(state().q12,q+64);sync();
console.log(JSON.stringify({test:'MASTER isolated production worklet/WASM + real index pointer/readback',wasmBytes:wasm.length,wasmSha256:crypto.createHash('sha256').update(wasm).digest('hex'),coarseCcAndCanonicalReadback:true,stalePointerUsesCurrentC:true,relativeBoundsAndMalformed:true,detachRetainsLevel:true,successFailedLoadPreserveLevel:true,loadingInputGuard:true,physicalMidi:false,actualBrowserDOM:false}));

// A real older engine has no master CC/readback; retain its pointer control.
if(process.argv[3]){
 const legacy=new Processor(),oldWasm=fs.readFileSync(process.argv[3]);
 await legacy.onMessage({type:'load',wasm:oldWasm,master:1000});assert.equal(legacy.ex.web_master_state,undefined);
 await legacy.onMessage({type:'master-step',delta:64});legacy.publish();assert.equal(legacy.messages.at(-1).master.q12,1064);
 await legacy.onMessage({type:'master',value:2048});legacy.publish();assert.equal(legacy.messages.at(-1).master.q12,2048);
 await legacy.onMessage({type:'load',wasm:oldWasm,master:99});legacy.publish();assert.equal(legacy.messages.at(-1).master.q12,2048);
 console.log('Actual older engine pointer/readback compatibility PASS (remote MASTER unsupported in that binary)');
}
