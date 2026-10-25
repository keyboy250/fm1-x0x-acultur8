// SPDX-License-Identifier: GPL-3.0-only
// Desired, effective and held-note state comes only from C snapshots.
const $=id=>document.getElementById(id);
const inputs=[...document.querySelector('.bass-lab').querySelectorAll('[data-part]')];
const modes=['Off','Root only','Chord tones only','Scale adherence'];
const notes=['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','B'];
const pending=new Map(),tickets=new Set(),holds=new Map(),drafts=new Map();
let counter=0,available=false,protectedProject=false,playing=false,caps=0,read=0,error=false;
let snapshots=[null,null],chord=null,info=null;
const say=(text,failed=false)=>{error=failed;$('bass-status').textContent=text;$('bass-status').classList.toggle('bass-error',failed);};
function busy(){return pending.size>0||tickets.size>0||holds.size>0;}
function controls(){
  const locked=!available||protectedProject||playing||busy();
  for(let part=0;part<2;part++)$('bass-controls-'+part).disabled=locked;
  for(const input of inputs)input.disabled=locked||(drafts.size>0&&!drafts.has(input));
  $('bass-undo').disabled=$('bass-redo').disabled=locked||drafts.size>0||!(caps&4);
  $('bass-save').disabled=locked||drafts.size>0||!(caps&8);
}
function send(op,target=0,index=0,value=0,hold=false){
  if(!available||protectedProject||typeof window.fm1SliceSend!=='function')return null;
  // Two-bit namespaces: sound 00, bass 01, arrangement 10, future external 11.
  counter=(counter+1)&0x3fffffff;const request=(0x40000000|counter)>>>0;
  pending.set(request,{op,target,index,value});
  if(hold)holds.set(target,{request,revision:null});
  controls();
  if(op!==3&&op!==4)say(op===7?'Requesting project save…':'Waiting for engine acknowledgement…');
  window.fm1SliceSend({type:'variant-command',abi:1,op,request,target,voice:0,index,value});
  return request;
}
function edit(input,text){
  if(!available||protectedProject||busy()){render();return;}
  if(playing){say('Stop playback using the instrument controls before changing bass follow.',true);render();return;}
  const part=Number(input.dataset.part),index=Number(input.dataset.field),value=Number(text);
  if(text===''||!Number.isInteger(value)||value<0||value>(index===0?3:127)){
    say(index===0?'Choose a supported follow mode.':'Enter a whole MIDI-note number from 0 to 127.',true);render();return;
  }
  send(3);send(13,part,index,value,true);send(4);
  render(); // Restore acknowledged values until a fresh C snapshot arrives.
}
function commit(input){if(!drafts.has(input))return;const text=drafts.get(input);drafts.delete(input);edit(input,text);controls();}
for(const input of inputs){
  if(input.type==='number'){
    input.addEventListener('input',()=>{drafts.set(input,input.value);controls();});
    input.addEventListener('change',()=>commit(input));
    input.addEventListener('blur',()=>{commit(input);render();});
  }else input.addEventListener('change',()=>edit(input,input.value));
}
for(const [id,op] of [['bass-undo',5],['bass-redo',6],['bass-save',7]])$(id).addEventListener('click',()=>{if(!busy()&&!drafts.size&&!playing)send(op);});
function render(){
  for(const input of inputs){
    const values=snapshots[Number(input.dataset.part)]?.values;
    if(values&&!drafts.has(input))input.value=String(values[Number(input.dataset.field)]);
  }
  for(let part=0;part<2;part++){
    const v=snapshots[part]?.values,id='bass-state-'+part;
    if(!available||!v){$(id).textContent=`303 ${part?'B':'A'} state unavailable.`;continue;}
    const effective=modes[v[3]]??'Unavailable';
    const held=v[5]===255?'No synth note held':`Synth MIDI note ${v[5]}`;
    const midi=v[6]===255?'No owned MIDI note':`Owned MIDI note ${v[6]} · channel ${v[7]===255?'unavailable':v[7]+1}`;
    const behavior=v[9]?'Strict range failure · no note emitted; a new NOTE is required.':v[3]===3?'Scale adherence is active, including transposition and preview notes.':!v[3]?'Follow is off or arrangement is disabled; legacy pitches apply.':!v[4]?'No playing chord · legacy absolute pitches apply.':'Playing chord follow is active.';
    $(id).textContent=`Effective: ${effective}. ${behavior} ${held}. ${midi}. Engine MIDI transport ${v[8]?'ready':'not ready'}; external receiver delivery pending.`;
  }
  const values=chord?.values;
  $('bass-chord').textContent=values?.length===7&&values[0]?`Playing chord: ${values.slice(1,4).map(pc=>notes[pc]??'?').join(' · ')} · MIDI ${values.slice(4,7).join(', ')}.`:'No playing chord metadata.';
  if(info)$('bass-project').textContent=`Project ${info.values[1]?'has unsaved changes':'matches its saved state'} · revision ${info.revision}`;
  controls();
}
window.addEventListener('fm1-ready',()=>{
  pending.clear();tickets.clear();holds.clear();drafts.clear();snapshots=[null,null];chord=info=null;
  available=protectedProject=playing=error=false;caps=read=0;
  $('bass-availability').textContent='Checking bass-follow support…';$('bass-project').textContent='';
  render();say('Checking the shared engine…');
});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;
  if(m.type==='variant-reply'){
    const sent=pending.get(m.request);if(!sent)return;pending.delete(m.request);
    const hold=holds.get(sent.target),latest=hold?.request===m.request;
    if(m.result<0){
      if(latest)holds.delete(sent.target);
      const failures={'-1':'Invalid bass edit. Use MIDI notes 0–127 with lowest no higher than highest.','-2':'This engine does not support bass-follow editing.','-3':sent.op===7?'Save is busy. Stop playback using the instrument controls, then press Save project again.':'Stop playback using the instrument controls before changing bass follow. The engine may also be busy.','-4':'Project is protected. Load a supported project before editing.','-5':'Storage or history capacity is full.','-6':'Project save failed.'};
      if(m.result===-4)protectedProject=true;
      say(failures[m.result]??`Engine rejected the operation (${m.result}).`,true);render();return;
    }
    if(latest)hold.revision=m.revision>>>0;
    if(m.result===1){tickets.add(m.ticket>>>0);if(!error)say('Queued · waiting for the audio boundary.');}
    else if(sent.op===7)say('Project save acknowledged by the engine.');
    else if(sent.op===5||sent.op===6)say(`${sent.op===5?'Undo':'Redo'} acknowledged by the engine.`);
    else if(sent.op!==3&&sent.op!==4&&!error)say('Edit acknowledged · waiting for the C snapshot.');
    render();return;
  }
  if(m.type!=='frame')return;
  info=m.variant?.info?.abi===1?m.variant.info:null;
  caps=info?.values[0]??0;protectedProject=!!info?.values[2];read=(info?.values[4]??0)>>>0;playing=!!m.sync?.playing;
  const rows=m.variant?.bass;
  // Field9 is the engine's strictBlocked flag; never infer harmony/range in JS.
  available=!!(caps&128)&&Array.isArray(rows)&&rows.length===2&&rows.every(row=>row?.abi===1&&row.section===7&&row.values.length===10);
  snapshots=available?rows:[null,null];chord=m.variant?.chord??null;
  $('bass-availability').textContent=protectedProject?'Project is protected. Load a supported project to edit.':!available?'Bass follow is unavailable in this engine. Existing bass notes remain unchanged.':playing?'Bass-follow state is available. Stop playback to edit mode or note bounds.':'Bass-follow settings are available. Choose a mode and MIDI-note bounds while stopped.';
  for(const ticket of tickets)if(((read-ticket)|0)>=0)tickets.delete(ticket);
  for(const [part,hold] of holds)if(hold.revision!==null&&snapshots[part]&&((snapshots[part].revision-hold.revision)|0)>=0)holds.delete(part);
  if(!error&&!busy()&&available&&/^(Checking|Queued|Edit acknowledged)/.test($('bass-status').textContent))say('Bass settings loaded from the engine.');
  render();
});
