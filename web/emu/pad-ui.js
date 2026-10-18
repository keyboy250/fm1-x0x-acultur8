// SPDX-License-Identifier: GPL-3.0-only
import {MidiRouter} from './midi-router.mjs';
const $=id=>document.getElementById(id);
const inputs=[...document.querySelector('.pad-lab').querySelectorAll('[data-pad-field]')];
const pending=new Map(),tickets=new Set(),drafts=new Map();
let count=0,available=false,protectedProject=false,playing=true,caps=0,snapshot=null,hold=null,error=false;
const names=['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','B'];
const reasons=['Eligible','No playing chord','Complete triad does not fit the range','Channel conflicts with a sequencer part','Transport disconnected','MIDI note output disabled'];
const router=new MidiRouter({send:m=>window.fm1SliceSend?.(m),changed:()=>renderRoute()});
for(let i=0;i<16;i++){const o=document.createElement('option');o.value=String(i);o.textContent=String(i+1);$('pad-channel').append(o);}
const say=(s,bad=false)=>{error=bad;$('pad-edit-status').textContent=s;$('pad-edit-status').classList.toggle('pad-error',bad);};
function busy(){return pending.size>0||tickets.size>0||hold!==null;}
function controls(){
  const locked=!available||protectedProject||playing||busy();
  $('pad-controls').disabled=locked;
  for(const input of inputs)input.disabled=locked||(drafts.size>0&&!drafts.has(input));
  $('pad-notes-out').disabled=locked||drafts.size>0;
  $('pad-undo').disabled=$('pad-redo').disabled=locked||drafts.size>0||!(caps&4);
  $('pad-save').disabled=locked||drafts.size>0||!(caps&8);
  $('pad-connect').disabled=router.opening;
  $('pad-output').disabled=!router.access||!available||playing||router.locked||busy();
  $('pad-route').disabled=!router.access||!available||playing||router.locked||busy()||!$('pad-output').value;
  $('pad-retry').disabled=!router.output||!available||playing||router.locked||busy()||router.active;
  $('pad-disconnect').disabled=!router.output||!router.active||!available||playing||router.locked||busy();
}
function renderRoute(){
  const selected=$('pad-output').value,ports=router.ports();
  const signature=ports.map(p=>p.id+'|'+p.name).join('\n');
  if($('pad-output').dataset.ports!==signature){
    $('pad-output').dataset.ports=signature;
    const blank=document.createElement('option');blank.value='';blank.textContent='Choose an output';
    $('pad-output').replaceChildren(blank,...ports.map(p=>{const o=document.createElement('option');o.value=p.id;o.textContent=p.name||'MIDI output';return o;}));
    if(ports.some(p=>p.id===selected))$('pad-output').value=selected;
  }
  $('pad-route-status').textContent=router.status;
  $('pad-route-detail').textContent=(router.output?'Original output: '+router.output.name+'. ':'')+
    'API-accepted packets '+router.accepted+'; cancelled attacks '+router.cancelled+
    '; API-owned notes awaiting cleanup '+router.debt+'; engine ring backlog '+router.pending+'. Receiver delivery is unverified.';
  $('pad-timing').textContent=router.ackCount?'Browser batch-acknowledgement round trip: last '+router.rtt.toFixed(2)+' ms, max '+router.maxRtt.toFixed(2)+
    ' ms. Maximum MIDI-clock API interval deviation: '+router.clockDeviation.toFixed(2)+' ms (includes batching). Physical arrival jitter and audio/MIDI alignment remain unmeasured.':
    'No MIDI batch has been measured yet. Physical arrival jitter and audio/MIDI alignment remain unmeasured.';
  controls();
}
function send(op,index=0,value=0,waitSnapshot=false){
  if(!available||protectedProject)return;
  const request=(0xc0000000|(++count&0x3fffffff))>>>0;
  pending.set(request,{op});if(waitSnapshot)hold={request,revision:null};
  window.fm1SliceSend({type:'variant-command',abi:1,op,request,target:0,voice:0,index,value});
  controls();return request;
}
function edit(index,text){
  if(!available||protectedProject||playing||busy()){render();return;}
  const value=Number(text),max=index===0?255:127;
  if(text===''||!Number.isInteger(value)||value<0||value>max||(index===0&&value>15&&value!==255)){
    say('Choose a valid channel or a whole MIDI-note number from 0 to 127.',true);render();return;
  }
  say('Waiting for engine acknowledgement…');send(3);send(14,index,value,true);send(4);render();
}
function commit(input){if(!drafts.has(input))return;const text=drafts.get(input);drafts.delete(input);edit(Number(input.dataset.padField),text);}
for(const input of inputs){
  if(input.type==='number'){
    input.addEventListener('input',()=>{drafts.set(input,input.value);controls();});
    input.addEventListener('change',()=>commit(input));
    input.addEventListener('blur',()=>{commit(input);render();});
  }else input.addEventListener('change',()=>edit(Number(input.dataset.padField),input.value));
}
$('pad-notes-out').addEventListener('change',()=>{
  if(!available||protectedProject||playing||busy()){render();return;}
  const value=$('pad-notes-out').checked?1:0;
  say('Waiting for note-output acknowledgement…');send(3);send(15,0,value,true);send(4);render();
});
for(const [id,op] of [['pad-undo',5],['pad-redo',6],['pad-save',7]])$(id).addEventListener('click',()=>{if(!playing&&!busy()&&!drafts.size)send(op);});
const routeAction=fn=>{Promise.resolve().then(fn).catch(e=>router.report(e.message));};
$('pad-connect').addEventListener('click',()=>router.connect());
$('pad-route').addEventListener('click',()=>routeAction(()=>router.select($('pad-output').value)));
$('pad-retry').addEventListener('click',()=>routeAction(()=>router.retry()));
$('pad-disconnect').addEventListener('click',()=>routeAction(()=>router.select('')));
$('pad-output').addEventListener('change',controls);
window.addEventListener('pagehide',()=>router.pagehide());
window.addEventListener('fm1-input',event=>router.input(event.detail));
function render(){
  const v=snapshot?.values;
  if(v){for(const input of inputs)if(!drafts.has(input))input.value=String(v[Number(input.dataset.padField)]);
    $('pad-notes-out').checked=!!v[4];
    const list=a=>a.every(n=>n===255)?'none':a.map(n=>n===255?'—':n).join(', ');
    $('pad-engine-state').textContent=(reasons[v[6]]??'Unknown engine reason')+'. Effective eligible: '+!!v[5]+
      '. Engine-owned notes '+list(v.slice(8,11))+' on channel '+(v[7]===255?'none':v[7]+1)+
      '. Desired triad '+list(v.slice(11,14))+'. Pending cleanup/chord update: '+!!v[14]+
      '. Engine transport ready: '+!!v[15]+'. Ownership is enqueue acceptance, not physical delivery.';
  }else $('pad-engine-state').textContent='External-pad snapshot unavailable.';
  controls();
}
window.addEventListener('fm1-ready',()=>{
  pending.clear();tickets.clear();drafts.clear();hold=null;snapshot=null;available=false;playing=true;
  router.engineReset();say('Checking the shared engine…');render();
});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;router.receive(m);
  if(m.type==='variant-reply'){
    const sent=pending.get(m.request);if(!sent)return;pending.delete(m.request);
    if(m.result<0){
      if(hold?.request===m.request)hold=null;
      const errors={'-1':'Invalid pad configuration or channel conflict. Use a dedicated channel and ordered note range.','-2':'External pad commands are unsupported.','-3':'Stop playback, wait for the engine queue, then retry.','-4':'The project is protected.','-5':'Project capacity is full.','-6':'Project save failed.'};
      if(m.result===-4)protectedProject=true;say(errors[m.result]??'Engine rejected this request.',true);
    }else{
      if(hold?.request===m.request)hold.revision=m.revision>>>0;
      if(m.result===1)tickets.add(m.ticket>>>0);
      if(sent.op===7)say('Project save acknowledged by the engine.');
    }
    render();return;
  }
  if(m.type!=='frame')return;
  const info=m.variant?.info,p=m.variant?.pad;
  caps=info?.values[0]??0;protectedProject=!!info?.values[2];playing=!!m.sync?.playing;
  available=!!(caps&256)&&p?.abi===1&&p.section===8&&p.values.length===16;snapshot=available?p:null;
  const read=(info?.values[4]??0)>>>0;
  for(const ticket of tickets)if(((read-ticket)|0)>=0)tickets.delete(ticket);
  if(hold?.revision!==null&&hold&&snapshot&&((snapshot.revision-hold.revision)|0)>=0)hold=null;
  $('pad-availability').textContent=!available?'External MIDI accompaniment is unavailable in this engine.':protectedProject?'Project protected.':playing?'Pad settings are locked during playback.':'Pad settings are available while stopped.';
  const chord=m.variant?.chord?.values;
  $('pad-chord').textContent=chord?.length===7&&chord[0]?'Playing chord: '+chord.slice(1,4).map(n=>names[n]??'?').join(' · ')+'.':'No playing chord metadata.';
  if(!busy()&&available&&!error)say('Settings read from the shared engine.');
  router.updateFrame(m);render();
});
renderRoute();render();

