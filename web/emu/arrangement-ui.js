// SPDX-License-Identifier: GPL-3.0-only
// Editing state is read from bounded C snapshots. No project or chord-theory mirror.
const $=id=>document.getElementById(id);
const parts=['909','808','303 A','303 B','Break'];
const notes=['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','B'];
const modes=['Ionian','Dorian','Phrygian','Lydian','Mixolydian','Aeolian','Locrian','Major pentatonic','Minor pentatonic','Chromatic'];
const degrees=['None','I','II','III','IV','V','VI','VII'];
const pending=new Map(), tickets=new Set(), holds=new Map();
let request=0x80000000, page=0, available=false, harmony=false, protectedProject=false;
let arrangement=null, block=null, read=0, lengthDraft=null, error=false;
const say=(text,failed=false)=>{error=failed;$('arrangement-status').textContent=text;$('arrangement-status').classList.toggle('arrangement-error',failed);};
function options(select,labels,start=0){select.replaceChildren(...labels.map((label,i)=>{const option=document.createElement('option');option.value=String(i+start);option.textContent=label;return option;}));}
options($('arrangement-key'),notes);options($('arrangement-mode'),modes);
options($('arrangement-parent'),['None']);$('arrangement-parent').firstChild.value='255';
for(let i=0;i<7;i++){const option=document.createElement('option');option.value=String(i);option.textContent=modes[i];$('arrangement-parent').append(option);}
options($('arrangement-duration'),Array.from({length:32},(_,i)=>String(i+1)),1);
options($('arrangement-degree'),degrees);
const patternInputs=[],muteInputs=[];
for(let i=0;i<5;i++){
  const container=document.createElement('div');container.className='arrangement-part';
  const label=document.createElement('label');label.textContent=parts[i]+' pattern ';
  const select=document.createElement('select');select.id='arrangement-pattern-'+i;select.setAttribute('aria-label',parts[i]+' pattern');options(select,Array.from({length:32},(_,p)=>String(p+1)));label.append(select);
  const muteLabel=document.createElement('label'),mute=document.createElement('input');mute.type='checkbox';mute.id='arrangement-mute-'+i;muteLabel.append(mute,document.createTextNode('Mute '+parts[i]));
  container.append(label,muteLabel);$('arrangement-parts').append(container);patternInputs.push(select);muteInputs.push(mute);
  select.addEventListener('change',()=>editBlock('pattern',i,Number(select.value)));
  mute.addEventListener('change',()=>editBlock('mute',i,mute.checked));
}
const rows=[];
for(let i=0;i<16;i++){
  const row=document.createElement('div');row.className='arrangement-row';
  const title=document.createElement('strong'),tags=document.createElement('div');tags.className='arrangement-tags';
  const actions=document.createElement('div');actions.className='arrangement-row-actions';
  const edit=document.createElement('button'),queue=document.createElement('button');edit.type=queue.type='button';edit.textContent='Edit';queue.textContent='Queue';
  edit.addEventListener('click',()=>send(11,page*16+i,0,0,0,'selection'));
  queue.addEventListener('click',()=>send(12,page*16+i));
  actions.append(edit,queue);row.append(title,tags,actions);$('arrangement-rows').append(row);rows.push({title,tags,edit,queue});
}
function send(op,target=0,voice=0,index=0,value=0,hold=null){
  if(!available||protectedProject||typeof window.fm1SliceSend!=='function')return;
  request=0x80000000|((request+1)&0x3fffffff);request>>>=0;
  const sent={op,target,voice,index,value,hold};pending.set(request,sent);
  if(hold)holds.set(hold,{request,revision:null,target});
  window.fm1SliceSend({type:'variant-command',abi:1,op,request,target,voice,index,value});
  if(op!==3&&op!==4)say(op===7?'Requesting project save…':'Waiting for engine acknowledgement…');
  controls();return request;
}
function grouped(op,target,voice,index,value,hold){send(3);send(op,target,voice,index,value,hold);send(4);}
function editBlock(kind,index,value){
  if(!block||!arrangement||holds.has('block')||holds.has('selection')){render();return;}
  const patterns=block.slice(0,5);let mutes=block[5],duration=block[6],degree=block[7];
  if(kind==='pattern')patterns[index]=value;
  if(kind==='mute')mutes=value?mutes|(1<<index):mutes&~(1<<index);
  if(kind==='duration')duration=value;
  if(kind==='degree')degree=value;
  let packed=0;for(let i=0;i<5;i++)packed|=patterns[i]<<(i*5);
  grouped(8,arrangement[2],((duration-1)&7)|(degree<<3)|(((duration-1)>>3)<<6),mutes,packed,'block');
}
$('arrangement-duration').addEventListener('change',()=>editBlock('duration',0,Number($('arrangement-duration').value)));
$('arrangement-degree').addEventListener('change',()=>editBlock('degree',0,Number($('arrangement-degree').value)));
for(const [id,index] of [['arrangement-enabled',0],['arrangement-key',1],['arrangement-mode',2],['arrangement-parent',3]])$(id).addEventListener('change',()=>{
  if(holds.has('global')){render();return;}
  const value=index===0?Number($(id).checked):Number($(id).value);
  grouped(9,0,0,index,value,'global');
});
$('arrangement-length').addEventListener('input',()=>{lengthDraft=$('arrangement-length').value;});
function commitLength(){
  if(lengthDraft===null)return;const text=lengthDraft;lengthDraft=null;
  const value=Number(text);
  if(text===''||!Number.isInteger(value)||value<0||value>192){say('Length must be a whole number from 0 to 192 blocks.',true);render();return;}
  grouped(10,0,0,0,value,'length');
}
$('arrangement-length').addEventListener('change',commitLength);
$('arrangement-length').addEventListener('blur',()=>{commitLength();render();});
$('arrangement-previous').addEventListener('click',()=>{page=Math.max(0,page-1);render();});
$('arrangement-next').addEventListener('click',()=>{page=Math.min(11,page+1);render();});
for(const [id,op] of [['arrangement-undo',5],['arrangement-redo',6],['arrangement-save',7]])$(id).addEventListener('click',()=>send(op));
// Every form change is one atomic, closed gesture. Also close on interrupted UI.
for(const event of ['pointercancel','blur','pagehide'])window.addEventListener(event,()=>{if(available)send(4);});
function controls(){
  const locked=!available||protectedProject;
  $('arrangement-global').disabled=locked||holds.has('global')||holds.has('length');
  for(const id of ['arrangement-key','arrangement-mode','arrangement-parent'])$(id).disabled=!harmony;
  $('arrangement-block').disabled=locked||!block||holds.has('block')||holds.has('selection');
  $('arrangement-degree').disabled=!harmony;
  $('arrangement-previous').disabled=locked||page===0;$('arrangement-next').disabled=locked||page===11;
  for(const [i,row] of rows.entries()){
    const target=page*16+i;row.edit.disabled=locked||holds.has('block')||holds.has('selection');row.queue.disabled=locked||!arrangement||!(arrangement[0]&1)||target>=arrangement[1];
    row.queue.title=!arrangement||!(arrangement[0]&1)?'Enable the arrangement before selecting a start or queued block.':target>=arrangement[1]?'This block is outside the arrangement length.':'Select at the next musical bar, or choose the start block while stopped.';
  }
  const caps=available?lastCaps:0;
  $('arrangement-undo').disabled=$('arrangement-redo').disabled=locked||!(caps&4);
  $('arrangement-save').disabled=locked||!(caps&8);
}
let lastCaps=0;
function render(){
  $('arrangement-page-label').textContent=`Blocks ${page*16+1}–${page*16+16} of 192`;
  for(const [i,row] of rows.entries()){
    const target=page*16+i;row.title.textContent='Block '+(target+1);
    const tags=[];if(arrangement){if(target===arrangement[2])tags.push('Editing');if(target===arrangement[3])tags.push('Playing');if(target===arrangement[4])tags.push('Queued');if(target>=arrangement[1])tags.push('Outside length');}
    row.tags.textContent=tags.join(' · ');row.edit.setAttribute('aria-label',`Edit block ${target+1}`);row.queue.setAttribute('aria-label',`Queue block ${target+1}`);
  }
  if(arrangement){
    const position=value=>value===255?'—':String(value+1);
    $('arrangement-position').textContent=`Editing ${position(arrangement[2])} · Playing ${position(arrangement[3])} · Queued ${position(arrangement[4])} · Start ${position(arrangement[5])} · tick ${arrangement[6]}/95 · ${arrangement[7]} bars remaining`;
    $('arrangement-editing').textContent='Editing block '+(arrangement[2]+1);
    if(!holds.has('global')){
      $('arrangement-enabled').checked=!!(arrangement[0]&1);$('arrangement-key').value=String(arrangement[8]);$('arrangement-mode').value=String(arrangement[9]);$('arrangement-parent').value=String(arrangement[10]);
    }
    if(lengthDraft===null&&!holds.has('length')&&document.activeElement!==$('arrangement-length'))$('arrangement-length').value=arrangement[1];
    if(block&&!holds.has('block')&&!holds.has('selection')){
      patternInputs.forEach((input,i)=>{input.value=String(block[i]);});muteInputs.forEach((input,i)=>{input.checked=!!(block[5]&(1<<i));});$('arrangement-duration').value=String(block[6]);$('arrangement-degree').value=String(block[7]);
    }
    for(const option of $('arrangement-degree').options)option.disabled=Number(option.value)>0&&(arrangement[9]===9||(arrangement[9]>=7&&arrangement[10]===255));
  }
  controls();
}
window.addEventListener('fm1-ready',()=>{
  pending.clear();tickets.clear();holds.clear();lengthDraft=null;arrangement=block=null;available=harmony=false;protectedProject=false;lastCaps=0;page=0;error=false;
  $('arrangement-chord').textContent='Playing chord metadata unavailable.';$('arrangement-project').textContent='';render();say('Checking the shared engine…');
});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;
  if(m.type==='variant-reply'){
    const sent=pending.get(m.request);if(!sent)return;pending.delete(m.request);
    const hold=sent.hold?holds.get(sent.hold):null,latest=hold?.request===m.request;
    if(latest){if(m.result<0)holds.delete(sent.hold);else hold.revision=m.revision>>>0;}
    if(m.result<0){
      const failures={'-1':'Invalid arrangement edit. Check the block, mode and parent values.','-2':'This engine does not support this operation.','-3':sent.op===7?'Save is busy. Stop playback using the instrument controls, then press Save project again.':sent.op===10||(sent.op===9&&sent.index===0)?'Stop playback using the instrument controls before changing length or enabling the arrangement.':sent.op===5||sent.op===6?'Stop arrangement playback using the instrument controls before Undo or Redo.':'Engine is busy. Try the edit again.','-4':'Project is protected. Load a supported project before editing.','-5':'Storage or history capacity is full.','-6':'Project save failed.'};
      if(m.result===-4)protectedProject=true;
      if(sent.hold==='length'){$('arrangement-length').value=arrangement?.[1]??0;lengthDraft=null;}
      const failure=m.result===-1&&sent.op===9&&sent.index===2?'Mode change refused by the engine. Choose a parent before switching to pentatonic; clear chord degrees before using chromatic.':failures[m.result];
      render();say(failure||`Engine rejected the operation (${m.result}).`,true);return;
    }
    if(m.result===1){tickets.add(m.ticket>>>0);say('Queued · waiting for the audio boundary.');}
    else if(sent.op===7)say('Project save acknowledged by the engine.');
    else if(sent.op===5||sent.op===6)say(`${sent.op===5?'Undo':'Redo'} acknowledged by the engine.`);
    else if(sent.op===12)say(`Block ${sent.target+1} selection acknowledged. Playing and Queued show the engine state.`);
    else if(sent.op!==3&&sent.op!==4)say('Edit acknowledged · waiting for the C snapshot.');
    controls();return;
  }
  if(m.type!=='frame')return;
  const v=m.variant,info=v?.info;
  lastCaps=info?.abi===1?info.values[0]:0;protectedProject=!!info?.values[2];read=info?.values[4]>>>0;
  available=!!(lastCaps&32)&&v?.arrangement?.abi===1&&v.arrangement.values.length===16;
  harmony=available&&!!(lastCaps&64);
  $('arrangement-availability').textContent=protectedProject?'Project is protected. Load a supported project to edit.':available?'Arrangement editing is available. Editing selects a row; Queue selects the next musical-bar transition or stopped start block.':'Arrangement editing is unavailable in this engine.';
  if(!available){$('arrangement-chord').textContent='Playing chord metadata unavailable.';controls();return;}
  arrangement=v.arrangement.values.slice();block=v.block?.abi===1&&v.block.values.length===8?v.block.values.slice():null;
  for(const [name,hold] of holds){
    if(hold.revision===null)continue;
    const snapshot=name==='block'?v.block:v.arrangement;
    if(!snapshot||((snapshot.revision-hold.revision)|0)<0)continue;
    if((name==='block'||name==='selection')&&arrangement[2]!==hold.target)continue;
    holds.delete(name);
  }
  for(const ticket of tickets)if(((read-ticket)|0)>=0)tickets.delete(ticket);
  if(!error&&!holds.size&&!tickets.size&&!pending.size&&/^(Queued|Edit acknowledged)/.test($('arrangement-status').textContent))say('Applied · C snapshot received.');
  $('arrangement-project').textContent=`Project ${info.values[1]?'has unsaved changes':'matches its saved state'} · revision ${info.revision}`;
  const chord=v.chord?.abi===1?v.chord.values:null;
  const follow=(lastCaps&128)?'Enabled bass follow uses this shared chord. External accompaniment remains pending.':'Bass-follow and external accompaniment remain pending.';
  $('arrangement-chord').textContent=harmony&&chord?.length===7&&chord[0]?`Playing chord metadata: ${chord.slice(1,4).map(pc=>notes[pc]??'?').join(' · ')} · MIDI ${chord.slice(4,7).join(', ')}. ${follow}`:(lastCaps&128)?'No playing chord metadata. Bass follow keeps legacy pitches when no chord is available. External accompaniment remains pending.':'No playing chord metadata. Bass-follow and external accompaniment remain pending.';
  render();
});
render();
