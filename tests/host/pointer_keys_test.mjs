// SPDX-License-Identifier: GPL-3.0-only
// Real emulator input listeners, deterministic pointer/timer events, and the
// production AudioWorklet plus an existing WASM build's actual input scanner.
import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';
assert.ok(process.argv[2],'existing production WASM path is required');
const wasm=fs.readFileSync(process.argv[2]);
const html=fs.readFileSync('web/emu/index.html','utf8');
const input=html.slice(html.indexOf('// ---------------------------------------------------------------- state and input'),html.indexOf('// ---------------------------------------------------------------- screen and lights'));
assert.ok(input.includes('function pointerUp'));
const WHITE=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26],BLACK=[1,3,5,8,10,13,15,17,20,22,25];
function surface(send=()=>{}){
 let now=0,serial=0;const timers=new Map(),captures=new Map(),messages=[],globals=new Map();
 const element=()=>({listeners:new Map(),classList:{toggle(){}},setAttribute(){},focus(){},contains(){return true;},
   addEventListener(type,fn){this.listeners.set(type,fn);},setPointerCapture(id){captures.set(id,this);},hasPointerCapture(id){return captures.get(id)===this;}});
 const device=element(),keys=Array.from({length:27},()=>({g:element(),hit:element()}));
 const buttons={PLAY:{bit:10,g:element(),hit:element()}},context=vm.createContext({
   device,keys,buttons,knobs:{},WHITE_K:WHITE,BLACK_K:BLACK,BTN:{PLAY:10,REC:11,SEL:1,HOME:6,'OCT-':12,'OCT+':13},
   $:()=>({checked:true,addEventListener(){}}),prefs:{},savePrefs(){},setState(){},document:{activeElement:{}},clearTimeout:id=>timers.delete(id),
   performance:{now:()=>now},setTimeout:(fn,delay)=>{timers.set(++serial,{fn,at:now+delay});return serial;},
   addEventListener:(type,fn)=>globals.set(type,fn),post:m=>{messages.push(m);send(m);}});
 vm.runInContext(input,context);vm.runInContext('node={port:{postMessage:post}}',context);
 const event=(type,index,id=1)=>({type,pointerId:id,target:keys[index].hit,preventDefault(){},button:0});
 const down=(index,id=1)=>keys[index].hit.listeners.get('pointerdown')(event('pointerdown',index,id));
 const up=(index,id=1,type='pointerup')=>{const e=event(type,index,id);if(type==='lostpointercapture')captures.delete(id);device.listeners.get(type)(e);};
 const advance=ms=>{const target=now+ms;for(let n=0;n<1000;n++){
   const due=[...timers].filter(([,v])=>v.at<=target).sort((a,b)=>a[1].at-b[1].at)[0];
   if(!due){now=target;return;}timers.delete(due[0]);now=due[1].at;due[1].fn();}throw Error('unbounded pointer timer loop');};
 return {keys,buttons,device,captures,context,messages,down,up,advance,
   mask:()=>vm.runInContext('keyMask',context),blur:()=>globals.get('blur')(),
   staleLost:(index,id=1)=>device.listeners.get('lostpointercapture')(event('lostpointercapture',index,id)),
   keyboard:(type,code)=>globals.get(type)({code,target:{},preventDefault(){}})};
}
{
 const s=surface();s.down(0);s.up(0);assert.equal(s.mask(),1);s.advance(59);assert.equal(s.mask(),1);s.advance(1);assert.equal(s.mask(),0);
 s.down(0);s.up(0);s.advance(10);s.up(0,1,'pointercancel');assert.equal(s.mask(),0);s.advance(100);assert.equal(s.mask(),0,'cancel after up invalidates the delayed release safely');
 // Different keys with the same mouse ID retain independent minimum holds.
 s.down(0);s.up(0);s.advance(10);s.down(7);s.up(7);assert.equal(s.mask(),1|(1<<7));
 s.advance(50);assert.equal(s.mask(),1<<7);s.advance(10);assert.equal(s.mask(),0);
 // An old release cannot clear a newer held press on the very same key/ID.
 s.down(0);s.up(0);s.advance(10);s.down(0);s.staleLost(0);s.advance(100);assert.equal(s.mask(),1);
 s.up(0);s.advance(0);assert.equal(s.mask(),0);
 // A delayed old-target lost-capture cannot release a new target's capture.
 s.down(0);s.up(0);s.advance(10);s.down(7);s.staleLost(0);s.advance(100);assert.equal(s.mask(),1<<7);
 s.up(7);s.advance(0);assert.equal(s.mask(),0);
 // Independent touch contacts and computer-keyboard ownership remain additive.
 s.down(0,1);s.down(7,2);s.keyboard('keydown','KeyQ');s.up(0,1,'pointercancel');assert.equal(s.mask(),1|(1<<7));
 s.keyboard('keyup','KeyQ');s.advance(59);assert.equal(s.mask(),1|(1<<7));s.advance(1);assert.equal(s.mask(),1<<7);s.up(7,2,'pointercancel');assert.equal(s.mask(),0);
 s.down(0,1);s.down(0,2);s.up(0,1,'pointercancel');assert.equal(s.mask(),1);s.up(0,2,'pointercancel');assert.equal(s.mask(),0);
 // Lost capture without a replacement still retains a short valid press.
 s.down(0);s.up(0,1,'lostpointercapture');s.advance(59);assert.equal(s.mask(),1);s.advance(1);assert.equal(s.mask(),0);
 // Blur releases physical pointers, keyboard and pending clicks immediately;
 // later old timers cannot clear a new press, and deliberate latches remain.
 s.buttons.PLAY.hit.listeners.get('pointerdown')({pointerId:8,target:s.buttons.PLAY.hit,button:0,preventDefault(){}});
 assert.equal(vm.runInContext('btnMask',s.context),1<<10);s.blur();assert.equal(vm.runInContext('btnMask',s.context),0);
 s.down(0);s.up(0);s.keyboard('keydown','KeyW');vm.runInContext('latched=1<<10;syncButtons()',s.context);s.blur();
 assert.equal(s.mask(),0);assert.equal(vm.runInContext('btnMask',s.context),1<<10);
 s.down(7);s.advance(100);assert.equal(s.mask(),1<<7);s.up(7,1,'pointercancel');assert.equal(s.mask(),0);
}
let Processor;globalThis.currentTime=0;
globalThis.AudioWorkletProcessor=class{constructor(){this.messages=[];this.port={postMessage:m=>this.messages.push(m)};}};
globalThis.registerProcessor=(_,ctor)=>{Processor=ctor;};await import('../../web/emu/worklet.js');
async function instrument(){
 const p=new Processor();await p.onMessage({type:'load',wasm});const pending=[],s=surface(m=>pending.push(m));
 const render=async blocks=>{for(let i=0;i<blocks;i++){
   s.advance(128000/44100);while(pending.length)await p.onMessage(pending.shift());
   currentTime+=128/44100;p.process([],[[new Float32Array(128),new Float32Array(128)]]);
 }};
 await render(175);return {p,s,render};
}
const whiteMask=WHITE.reduce((mask,k)=>mask|(1<<k),0),pattern=p=>p.ex.web_lit_keys()&whiteMask;
{
 const {p,render}=await instrument();assert.equal(pattern(p),0);
 // Reproduce the old immediate down/up loss before any actual C input scan.
 await p.onMessage({type:'keys',mask:1});await p.onMessage({type:'keys',mask:0});await render(30);
 assert.equal(pattern(p),0,'control: overwritten held mask is not a firmware keypress');
}
{
 const {p,s,render}=await instrument();assert.equal(pattern(p),0);
 // The reported white 1/5/9/13 rapid mouse clicks all happen before a render,
 // and reuse the same pointer. C must record every step, not just the last one.
 const wanted=[0,4,8,12].map(i=>WHITE[i]);
 for(const key of wanted){s.down(key);s.up(key);s.advance(3);}
 await render(45);assert.equal(s.mask(),0);
 assert.equal(pattern(p),wanted.reduce((mask,k)=>mask|(1<<k),0),'real C sequencer retains all four short pointer clicks');
 // A later normal-duration click toggles exactly one existing step off.
 s.down(WHITE[8]);await render(30);s.up(WHITE[8]);await render(20);
 assert.equal(pattern(p),(1<<WHITE[0])|(1<<WHITE[4])|(1<<WHITE[12]));
}
{
 const {p,s,render}=await instrument();s.down(WHITE[0],1);s.down(WHITE[4],2);await render(25);
 s.up(WHITE[0],1,'pointercancel');assert.equal(s.mask(),1<<WHITE[4]);await render(3);
 s.blur();await render(10);assert.equal(s.mask(),0);assert.equal(pattern(p),(1<<WHITE[0])|(1<<WHITE[4]));
}
console.log('Pointer keys: 60 ms capture, independent reused-pointer holds, stale releases/capture, touch/keyboard polyphony, cancel/blur, and real worklet/WASM four-step input scan PASS');
