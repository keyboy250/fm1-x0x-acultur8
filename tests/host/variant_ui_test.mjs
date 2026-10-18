// SPDX-License-Identifier: GPL-3.0-only
// Production sound-strip UI: release controls and acknowledged edit semantics.
import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';
class Element extends EventTarget{constructor(attrs={}){super();Object.assign(this,{value:'',disabled:false,textContent:'',dataset:{},type:'',options:[],checked:false,classList:{toggle(){}}},attrs);}append(o){this.options.push(o);}setAttribute(k,v){this[k]=v;}}
const html=fs.readFileSync('web/emu/variant-panel.html','utf8'),elements=new Map(),parameters=[],flags=[];
for(const m of html.matchAll(/<input[^>]*>|<(?:button|fieldset|select|h2|legend|p)[^>]*id="[^"]+"[^>]*>/g)){
 const tag=m[0],id=tag.match(/id="([^"]+)"/)?.[1],e=new Element({type:tag.match(/type="([^"]+)"/)?.[1]||''});
 if(id)elements.set(id,e);
 if(tag.includes('data-index=')){e.dataset.index=tag.match(/data-index="([^"]+)"/)[1];parameters.push(e);}
 if(tag.includes('data-flag=')){e.dataset.flag=tag.match(/data-flag="([^"]+)"/)[1];flags.push(e);}
}
const panel={querySelectorAll:s=>s==='[data-index]'?parameters:flags},sent=[],window=new EventTarget();window.fm1SliceSend=m=>sent.push(m);
const document={activeElement:null,hidden:false,querySelector:()=>panel,getElementById:id=>{assert.ok(elements.has(id),id);return elements.get(id);},createElement:()=>new Element(),addEventListener(){}};
const context=vm.createContext({window,document,console,Map,Set,Math,Number});vm.runInContext(fs.readFileSync('web/emu/variant-ui.js','utf8'),context);
const $=id=>elements.get(id);const fire=(id,n)=>$(id).dispatchEvent(new Event(n));
const values=[73,12345,6,9,173,15];let revision=1,read=0,protectedProject=0,caps=1535,playing=false,reject=0,bank=null;
const frame=()=>window.dispatchEvent(new CustomEvent('fm1-marked',{detail:{type:'frame',sync:{playing},variant:{strip:0,viewRevision:1,info:{abi:1,revision,values:[caps,0,protectedProject,read,read,0,0,33554431]},settings:{abi:1,revision,values:values.slice()},meters:{values:[0,0,1000000]}}}}));
const pump=()=>{for(const m of sent.splice(0)){
 if(m.type!=='variant-command')continue;
 let result=reject;
 if(!result){
  if(m.op===2)values[m.index]=m.value;
  if(m.op===20){bank=new Uint8Array(212);const v=new DataView(bank.buffer);v.setUint32(0,0x31464453,true);v.setUint16(4,1,true);v.setUint16(6,212,true);v.setUint32(8,1,true);v.setUint16(12,values[0],true);v.setUint16(14,values[1],true);for(let i=2;i<6;i++)bank[14+i]=values[i];}
  if(m.op===21){if(!bank)result=-7;else{const v=new DataView(bank.buffer);values[0]=v.getUint16(12,true);values[1]=v.getUint16(14,true);for(let i=2;i<6;i++)values[i]=bank[14+i];}}
 }
 revision++;window.dispatchEvent(new CustomEvent('fm1-marked',{detail:{type:'variant-reply',request:m.request,result,revision,ticket:read}}));
}frame();};
const persist=saved=>window.dispatchEvent(new CustomEvent('fm1-store',{detail:{saved,store:{7:bank}}}));
window.dispatchEvent(new Event('fm1-ready'));frame();assert.equal($('variant-release-number').value,173);
const edit=(id,value)=>{$(id).value=String(value);fire(id,'input');fire(id,'change');fire(id,'blur');};
edit('variant-hp-number',81);assert.equal(sent.filter(m=>m.type==='variant-command'&&m.op===2).length,1);assert.equal(sent.find(m=>m.op===2).index,0);pump();assert.equal(values[4],173,'HP edit must preserve release');
for(const value of [10,250,119]){edit('variant-release-number',value);assert.equal(sent.find(m=>m.op===2).index,4);assert.equal(sent.find(m=>m.op===2).value,value);pump();assert.equal(values[4],value);}
for(const value of [9,251,'',12.5]){edit('variant-release-number',value);assert.equal(sent.filter(m=>m.op===2).length,0,'invalid release does not edit');pump();}
fire('variant-save','click');assert.equal(sent.find(m=>m.type==='variant-command').op,7);pump();assert.equal(values[4],119);
protectedProject=1;frame();edit('variant-release-number',100);assert.equal(sent.filter(m=>m.op===2).length,0);assert.equal(values[4],119);
console.log('Sound-strip UI: release10..250, invalid bounds, unrelated-edit preservation, save and protected refusal PASS');

protectedProject=0;frame();assert.equal($('variant-default-save').disabled,true,'older engine cannot advertise defaults');
caps=3583;frame();assert.equal($('variant-default-save').disabled,false);
fire('variant-default-apply','click');pump();assert.match($('variant-status').textContent,/No saved default/);assert.equal($('variant-strip').disabled,false);
fire('variant-default-save','click');pump();assert.equal($('variant-strip').disabled,true);assert.doesNotMatch($('variant-status').textContent,/^Saved default/);persist(false);assert.match($('variant-status').textContent,/Browser storage failed/);assert.equal($('variant-strip').disabled,false);
fire('variant-default-save','click');pump();persist(true);assert.match($('variant-status').textContent,/Saved default for 909 BD/);
const saved=values.slice();values.splice(0,6,150,9000,0,2,220,1);frame();fire('variant-default-apply','click');pump();assert.deepEqual(values,saved);assert.match($('variant-status').textContent,/Applied saved default/);
for(const result of [-2,-3,-4,-6]){reject=result;fire('variant-default-save','click');pump();assert.equal($('variant-strip').disabled,false);if(result===-4){protectedProject=0;frame();}}
reject=0;playing=true;frame();const count=sent.length;fire('variant-default-save','click');fire('variant-default-apply','click');assert.equal(sent.length,count);playing=false;frame();
protectedProject=1;frame();fire('variant-default-save','click');assert.equal(sent.length,count);
console.log('Sound-strip defaults UI: legacy fallback, missing default, durable ACK, storage failure/retry, full recall, C refusals and playing/protected guards PASS');
