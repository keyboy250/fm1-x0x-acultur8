// SPDX-License-Identifier: GPL-3.0-only
// This component never keeps a second project mirror: desired values come from C snapshots.
const panel=document.querySelector('.variant-lab');
const $=id=>document.getElementById(id);
const parameters=[...panel.querySelectorAll('[data-index]')];
const flags=[...panel.querySelectorAll('[data-flag]')];
const pending=new Map(), tickets=new Set();
// Uncommitted text and outstanding field requests are presentation state only.
// Neither updates settings; only an acknowledged C snapshot does that.
const drafts=new Map(), fieldWaiting=new Map();
let request=0, available=false, protectedProject=false, caps=0, settings=null, read=0;
let gesture=null, error=false;
const voices=['909 BD','909 SD','909 LT','909 MT','909 HT','909 RS','909 CP','909 CH','909 OH','909 CR','909 RD','808 BD','808 SD','808 LT','808 MT','808 HT','808 RS / Clave','808 CP / Maracas','808 CB','808 CY','808 OH','808 CH','303 A','303 B','BREAK (mono)'];
const selector=$('variant-strip');
let selectedStrip=0, viewRevision=0, processingMask=0;
for(const [index,name] of voices.entries()){const option=document.createElement('option');option.value=index;option.textContent=name;option.disabled=index!==0;selector.append(option);}
function selectionBusy(){return !!gesture||pending.size>0||tickets.size>0||drafts.size>0||fieldWaiting.size>0||flagWaiting;}
function lockSelector(){selector.disabled=selectionBusy();}
function selectView(){settings=null;available=false;viewRevision=(viewRevision+1)>>>0;$('variant-controls').disabled=true;$('variant-meter').textContent='Gain reduction unavailable.';$('variant-availability').textContent='Checking the selected sound strip…';$('variant-title').textContent=selectedStrip===0?'909 kick · sound strip':`${voices[selectedStrip]} · sound strip`;$('variant-legend').textContent=selectedStrip===0?'Kick processing':`${voices[selectedStrip]} processing`;window.fm1SliceSend?.({type:'variant-select',strip:selectedStrip,viewRevision});}
selector.addEventListener('change',()=>{const next=Number(selector.value);if(selectionBusy()||!Number.isInteger(next)||next<0||next>=25||!(processingMask&(1<<next))){selector.value=selectedStrip;return;}selectedStrip=next;selectView();say('Checking the selected sound strip…');});
let flagWaiting=false, flagRevision=null;
const say=(text,isError=false)=>{error=isError;$('variant-status').textContent=text;$('variant-status').classList.toggle('variant-error',isError);};
const passed=ticket=>((read-(ticket>>>0))|0)>=0;
function command(op,index=0,value=0){
  if(typeof window.fm1SliceSend!=='function')return;
  request=(request+1)&0x3fffffff;
  pending.set(request,{op,index,value,target:selectedStrip});lockSelector();
  window.fm1SliceSend({type:'variant-command',abi:1,op,request,target:selectedStrip,voice:0,index,value});
  if(op!==3&&op!==4)say(op===7?'Requesting project save…':'Waiting for engine acknowledgement…');
  return request;
}
function begin(input){if(!available||protectedProject)return;if(gesture&&gesture!==input)end();if(!gesture){gesture=input;command(3);command(2,4,80);}}
function end(){if(gesture){gesture=null;command(4);}lockSelector();}
function converted(input){const i=Number(input.dataset.index),n=Number(input.value);return i<2&&input.type==='range'?Math.round(20*Math.pow(1000,n/1000)):i===3?-n:n;}
function display(input,value){const i=Number(input.dataset.index);input.value=i<2&&input.type==='range'?Math.round(1000*Math.log(value/20)/Math.log(1000)):i===3?-value:value;if(input.type==='range')input.setAttribute('aria-valuetext',`${i<2?value:i===3?-value:value} ${i<2?'Hz':i===3?'dBFS':'dB'}`);}
function render(forceIndex=-1){
  if(!settings)return;
  for(const input of parameters){
    const index=Number(input.dataset.index);
    if(index===forceIndex||(!drafts.has(input)&&!fieldWaiting.has(index)&&document.activeElement!==input&&gesture!==input))display(input,settings[index]);
  }
  for(const input of flags){input.checked=!!(settings[5]&Number(input.dataset.flag));input.disabled=flagWaiting;}
}
function edit(input){
  if(!available||protectedProject)return;
  const i=Number(input.dataset.index),value=converted(input);
  if(input.value===''||!Number.isInteger(value)||(i<2?(value<20||value>20000):i===2?(value<0||value>24):(value<0||value>48))){say('Enter a whole-number value within the shown range.',true);return;}
  begin(input);
  const sent=command(2,i,value);
  fieldWaiting.set(i,{request:sent,revision:null});
  // Update paired display only during this user gesture; snapshots remain authoritative.
  for(const other of parameters)if(other!==input&&Number(other.dataset.index)===i)display(other,value);
}
function commitNumber(input){
  if(!drafts.has(input))return;
  input.value=drafts.get(input);drafts.delete(input);
  edit(input);end();
}
for(const input of parameters){
  if(input.type==='range'){
    input.addEventListener('pointerdown',()=>begin(input));
    input.addEventListener('keydown',e=>{if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Home','End','PageUp','PageDown'].includes(e.key))begin(input);});
    input.addEventListener('input',()=>edit(input));
    input.addEventListener('keyup',end);
  }else{
    input.addEventListener('input',()=>{drafts.set(input,input.value);lockSelector();});
    input.addEventListener('change',()=>commitNumber(input));
  }
  input.addEventListener('blur',()=>{if(input.type==='number')commitNumber(input);end();render();});
}
for(const input of flags)input.addEventListener('change',()=>{
  if(!available||protectedProject||flagWaiting){render();return;}
  begin(input);
  const bit=Number(input.dataset.flag);
  const value=input.checked?settings[5]|bit:settings[5]&~bit;
  // One full flags word at a time: the next toggle must use a fresh C snapshot,
  // never a prior frame or an optimistic/rejected checkbox combination.
  flagWaiting=true;flagRevision=null;
  for(const item of flags)item.disabled=true;
  command(2,5,value);end();
});
for(const input of flags)input.addEventListener('blur',()=>render());
for(const event of ['pointerup','pointercancel','blur','pagehide'])window.addEventListener(event,end);
document.addEventListener('visibilitychange',()=>{if(document.hidden)end();});
for(const [id,op] of [['variant-undo',5],['variant-redo',6],['variant-save',7]])$(id).addEventListener('click',()=>{end();command(op);});
window.addEventListener('fm1-ready',()=>{
  end();pending.clear();tickets.clear();drafts.clear();fieldWaiting.clear();settings=null;available=false;error=false;flagWaiting=false;flagRevision=null;
  $('variant-controls').disabled=true;
  $('variant-meter').textContent='Gain reduction unavailable.';
  $('variant-project').textContent='';
  selectedStrip=0;selector.value=0;processingMask=0;for(const option of selector.options)option.disabled=Number(option.value)!==0;selectView();lockSelector();say('Checking the shared engine…');
});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;
  if(m.type==='variant-reply'){
    const sent=pending.get(m.request);if(!sent)return;pending.delete(m.request);lockSelector();
    if(sent.target!==selectedStrip)return;
    const field=sent.op===2?fieldWaiting.get(sent.index):null;
    const latestField=field?.request===m.request;
    if(latestField){
      if(m.result<0)fieldWaiting.delete(sent.index);
      else field.revision=m.revision>>>0;
    }
    if(sent.op===2&&sent.index===5){
      if(m.result<0){flagWaiting=false;flagRevision=null;}
      else flagRevision=m.revision>>>0;
    }
    if(m.result<0){
      const failures={'-1':'Invalid edit.','-2':'This engine does not support the operation.','-3':sent.op===7?'Save is busy. Stop playback using the instrument controls, then press Save project again.':'Engine is busy. Try the edit again.','-4':'Project is protected. Load a supported project before editing.','-5':'Storage or history capacity is full.','-6':'Project save failed.'};
      say(failures[m.result]||`Engine rejected the operation (${m.result}).`,true);
      // A rejected ticket is never counted as an accepted edit.
      if(sent.op===3){gesture=null;command(4);}
      if(m.result===-4){protectedProject=true;$('variant-controls').disabled=true;}
      if(latestField)for(const input of parameters)if(Number(input.dataset.index)===sent.index)drafts.delete(input);
      render(latestField?sent.index:-1);lockSelector();return;
    }
    if(m.result===1){tickets.add(m.ticket>>>0);say('Queued · waiting for the audio boundary.');}
    else if(sent.op===7)say('Project save acknowledged by the engine.');
    else if(sent.op===5||sent.op===6)say(`${sent.op===5?'Undo':'Redo'} acknowledged by the engine.`);
    lockSelector();return;
  }
  if(m.type!=='frame')return;
  const variant=m.variant;
  // View identity is transport state, not a project mirror. Ignore frames queued for a previous selection.
  if(variant&&(variant.strip!==selectedStrip||(variant.viewRevision!==undefined&&variant.viewRevision!==viewRevision)))return;
  if(!variant?.info||variant.info.abi!==1){available=false;$('variant-controls').disabled=true;$('variant-availability').textContent='Sound-strip audio is unavailable in this engine.';$('variant-meter').textContent='Gain reduction unavailable.';return;}
  const info=variant.info.values;
  caps=info[0];protectedProject=!!info[2];read=info[4]>>>0;
  processingMask=(caps&16)?(info[7]>>>0):0;
  for(const option of selector.options)option.disabled=!(processingMask&(1<<Number(option.value)));
  available=!!(caps&16)&&!!(processingMask&(1<<selectedStrip))&&variant.strip===selectedStrip&&variant.settings?.abi===1&&variant.settings.values.length===6;
  $('variant-controls').disabled=!available||protectedProject;
  $('variant-undo').disabled=$('variant-redo').disabled=!(caps&4);
  $('variant-save').disabled=!(caps&8);
  $('variant-availability').textContent=protectedProject?'Project is protected. Load a supported project to edit.':available?`${selectedStrip===0?'909 kick':voices[selectedStrip]} processing is available. Enable the strip and choose its stages to hear changes.${processingMask===1?' This engine processes only the 909 kick; other selectors are unavailable.':''}`:'Sound-strip audio is unavailable for this voice in this engine. Strip configuration alone does not enable processing.';
  $('variant-project').textContent=`Project ${info[1]?'has unsaved changes':'matches its saved state'} · revision ${variant.info.revision}`;
  if(available){
    if(!error&&$('variant-status').textContent.startsWith('Checking '))say('Selected sound settings loaded from the engine.');
    settings=variant.settings.values.slice();
    for(const [index,field] of fieldWaiting)if(field.revision!==null&&((variant.settings.revision-field.revision)|0)>=0)fieldWaiting.delete(index);
    if(flagWaiting&&flagRevision!==null&&((variant.settings.revision-flagRevision)|0)>=0){flagWaiting=false;flagRevision=null;}
    render();
  }
  for(const ticket of tickets)if(passed(ticket))tickets.delete(ticket);
  lockSelector();
  if(!error&&!tickets.size&&![...pending.values()].some(p=>p.op!==3&&p.op!==4)&&$('variant-status').textContent.startsWith('Queued'))say('Applied at the audio boundary.');
  const meters=variant.meters?.values;
  if(!available||!meters||meters.length<3||!(settings[5]&1))$('variant-meter').textContent='Gain reduction unavailable · strip bypassed or no meter data.';
  else{
    const gain=meters[2]/1e6, reduction=gain>0?Math.max(0,-20*Math.log10(gain)):Infinity;
    $('variant-meter').textContent=`Gain reduction: ${Number.isFinite(reduction)?reduction.toFixed(1):'∞'} dB · input peak ${peak(meters[0])} · output peak ${peak(meters[1])}`;
  }
});
function peak(value){return value>0?`${(20*Math.log10(value/1e6)).toFixed(1)} dBFS`:'−∞ dBFS';}
