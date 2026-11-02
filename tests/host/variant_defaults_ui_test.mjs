// SPDX-License-Identifier: GPL-3.0-only
// Independent read-only production-UI review. No browser storage or engine mutation.
import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';
const root=process.argv[2]||'.';
class Element extends EventTarget{constructor(a={}){super();Object.assign(this,{value:'',disabled:false,textContent:'',dataset:{},type:'',options:[],checked:false,classList:{toggle(){}}},a);}append(o){this.options.push(o);}setAttribute(k,v){this[k]=v;}}
function setup(){
 const html=fs.readFileSync(root+'/web/emu/variant-panel.html','utf8'),elements=new Map(),parameters=[],flags=[];
 for(const m of html.matchAll(/<input[^>]*>|<(?:button|fieldset|select|h2|legend|p)[^>]*id="[^"]+"[^>]*>/g)){
  const tag=m[0],id=tag.match(/id="([^"]+)"/)?.[1],e=new Element({type:tag.match(/type="([^"]+)"/)?.[1]||''});if(id)elements.set(id,e);
  if(tag.includes('data-index=')){e.dataset.index=tag.match(/data-index="([^"]+)"/)[1];parameters.push(e);}if(tag.includes('data-flag=')){e.dataset.flag=tag.match(/data-flag="([^"]+)"/)[1];flags.push(e);}
 }
 const sent=[],window=new EventTarget();window.fm1SliceSend=m=>sent.push(m);
 const document={activeElement:null,hidden:false,querySelector:()=>({querySelectorAll:s=>s==='[data-index]'?parameters:flags}),getElementById:id=>elements.get(id),createElement:()=>new Element(),addEventListener(){}};
 vm.runInContext(fs.readFileSync(root+'/web/emu/variant-ui.js','utf8'),vm.createContext({window,document,console,Map,Set,Math,Number}));
 const $=id=>elements.get(id),fire=(id,type)=>$(id).dispatchEvent(new Event(type));
 const emit=m=>window.dispatchEvent(new CustomEvent('fm1-marked',{detail:m}));
 const frame=(strip,viewRevision,values=[80,12000,3,6,173,15],{caps=3583,playing=false,protectedProject=0,read=100,revision=10}={})=>emit({type:'frame',sync:{playing},variant:{strip,viewRevision,info:{abi:1,revision,values:[caps,0,protectedProject,read,read,0,0,33554431]},settings:{abi:1,revision,values},meters:{values:[0,0,1000000]}}});
 const reply=(m,result=0,ticket=0)=>emit({type:'variant-reply',request:m.request,result,revision:10,ticket});
 const ready=()=>window.dispatchEvent(new Event('fm1-ready'));
 const store=(strip,saved=true)=>{const bank=new Uint8Array(212),v=new DataView(bank.buffer);v.setUint32(0,0x31464453,true);v.setUint16(4,1,true);v.setUint16(6,212,true);v.setUint32(8,1<<strip,true);window.dispatchEvent(new CustomEvent('fm1-store',{detail:{saved,store:{7:bank}}}));};
 return{$,fire,emit,frame,reply,ready,store,sent};
}
{
 const h=setup();h.ready();let view=1;h.frame(0,view);h.sent.length=0;
 for(let strip=0;strip<25;strip++){
  if(strip){h.$('variant-strip').value=String(strip);h.fire('variant-strip','change');const sel=h.sent.pop();assert.equal(sel.type,'variant-select');assert.equal(sel.strip,strip);view=sel.viewRevision;
   h.frame(strip-1,view-1);assert.equal(h.$('variant-controls').disabled,true,'previous selection snapshot cannot enable controls');
   const before=h.sent.length;h.fire('variant-default-save','click');assert.equal(h.sent.length,before,'unread selection cannot save');}
  h.frame(strip,view,[60+strip,12000+strip,strip%25,strip%49,100+strip,15]);
  h.fire('variant-default-save','click');const save=h.sent.pop();assert.equal(save.op,20);assert.equal(save.target,strip);
  h.store(strip);assert.equal(h.$('variant-strip').disabled,true,'store before native ACK cannot finish save');
  h.reply(save);h.store((strip+1)%25);assert.equal(h.$('variant-strip').disabled,true,'other voice bank cannot finish save');
  h.store(strip);assert.equal(h.$('variant-strip').disabled,false);assert.match(h.$('variant-status').textContent,/^Saved default for /);
  h.fire('variant-default-apply','click');const apply=h.sent.pop();assert.equal(apply.op,21);assert.equal(apply.target,strip);h.reply(apply,0,110);
  h.frame(strip,view,undefined,{read:109});assert.equal(h.$('variant-strip').disabled,true,'apply waits for readback ticket');
  h.frame(strip,view,undefined,{read:110});assert.equal(h.$('variant-strip').disabled,false);assert.match(h.$('variant-status').textContent,/^Applied saved default/);
 }
 console.log('25 UI targets: explicit save/apply addressing; matching selection, native ACK, voice bank and audio-ticket gates PASS');
}
{
 const h=setup();h.ready();h.frame(0,1);h.sent.length=0;h.fire('variant-default-save','click');const old=h.sent.pop();h.reply(old);h.ready();
 const view=h.sent.findLast(m=>m.type==='variant-select').viewRevision;h.sent.length=0;
 h.reply(old);h.store(0);h.frame(0,view-1);assert.equal(h.$('variant-controls').disabled,true);assert.doesNotMatch(h.$('variant-status').textContent,/^Saved default/);
 h.frame(0,view);h.fire('variant-default-save','click');const fresh=h.sent.pop();assert.notEqual(fresh.request,old.request);
 h.reply(old);h.store(0);assert.equal(h.$('variant-strip').disabled,true,'old reply/store before new native ACK cannot finish fresh save');
 h.reply(fresh);h.store(0,false);assert.match(h.$('variant-status').textContent,/Browser storage failed/);assert.equal(h.$('variant-strip').disabled,false);
 h.fire('variant-default-save','click');const retry=h.sent.pop();h.reply(retry);h.store(0,true);assert.match(h.$('variant-status').textContent,/^Saved default/);
 for(const result of [-2,-3,-4,-5,-6]){h.frame(0,view);h.fire('variant-default-save','click');const req=h.sent.pop();h.reply(req,result);assert.equal(h.$('variant-strip').disabled,false);assert.doesNotMatch(h.$('variant-status').textContent,/^Saved default/);}
 console.log('Reset/stale requests, old view isolation, storage failure/retry and negative native ACK release PASS');
}
