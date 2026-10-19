// SPDX-License-Identifier: GPL-3.0-only
import {readWav,parseMarkers,exportMarkers} from './slice-import.js';
const $=id=>document.getElementById(id);
let imported=null, powered=false, ready=false;
const say=(text,error=false)=>{ $('slice-status').textContent=text;$('slice-status').classList.toggle('slice-error',error); };
function controls(){for(const id of ['slice-preview-play','slice-export','slice-save'])$(id).disabled=!imported||(id==='slice-preview-play'&&!ready);$('slice-preview-stop').disabled=!ready;$('slice-sync-play').disabled=!ready;}
function send(message){window.fm1SliceSend(message);}
function loadEngine(){
  ready=false;controls();
  if(!powered||!imported)return;
  send({type:'marked-load',...imported.audio,pcm:imported.audio.pcm,slices:imported.markers.slices,length:imported.markers.length});
  $('slice-source-bpm').value=imported.markers.bpm||'';
  level();
}
function level(){if(powered)send({type:'marked-level',gain:Number($('slice-volume').value),layer:$('slice-layer').checked});}
function draw(){
  if(!imported)return;
  const {audio:a,markers:m}=imported;
  $('slice-info').textContent=`${imported.name} · ${m.slices.length} original slices · ${a.rate} Hz · ${a.channels===2?'stereo':'mono'} · ${m.loopDuration.toFixed(3)} s${m.bpm?' · '+m.bpm+' BPM':''}`;
  const canvas=$('slice-wave'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;
  ctx.fillStyle='#0b0e12';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#81b9bd';ctx.beginPath();
  for(let x=0;x<w;x++){
    const lo=Math.floor(x*a.frames/w),hi=Math.max(lo+1,Math.floor((x+1)*a.frames/w));let peak=0;
    for(let i=lo;i<hi;i++)for(let c=0;c<a.channels;c++)peak=Math.max(peak,Math.abs(a.pcm[i*a.channels+c]));
    ctx.moveTo(x,h/2-peak*(h/2-10));ctx.lineTo(x,h/2+peak*(h/2-10));
  }ctx.stroke();ctx.strokeStyle='#f0a043';
  for(const s of m.slices){const x=s.start/a.frames*w;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}
  $('slice-list').replaceChildren(...m.slices.map((s,i)=>{
    const b=document.createElement('button');b.type='button';b.dataset.slice=String(i);b.disabled=!ready;
    b.textContent=`${i+1} · ${s.label} · ${(s.start/a.rate).toFixed(3)}–${(s.end/a.rate).toFixed(3)} s · trigger ${(s.time/44100).toFixed(3)} s`;
    b.onclick=()=>{level();send({type:'marked-pad',slice:i});say('Auditioning slice '+(i+1)+' with its original boundaries.');};return b;
  }));controls();
}
$('slice-import-button').onclick=async()=>{
  try{
    const wav=$('slice-audio').files[0],markers=$('slice-markers').files[0];
    if(!wav||!markers)throw Error('Choose a WAV and its exact CSV or JSON marker file.');
    if(wav.size>16*1024*1024||markers.size>262144)throw Error('File exceeds the preview limit: 16 MiB WAV / 256 KiB markers.');
    const audio=readWav(await wav.arrayBuffer()),meta=parseMarkers(await markers.text(),markers.name,audio);
    imported={name:wav.name,audio,markers:meta};draw();loadEngine();
    say(powered?'Loading exact markers into the emulator…':'Imported. Switch on the FM1 panel, then play the original timing below.');
  }catch(e){say(e.message,true);}
};
$('slice-demo').onclick=()=>{
  const rate=44100,frames=rate*2,pcm=new Float32Array(frames),starts=[0,.13,.51,.78,1.17,1.63];
  starts.forEach((t,k)=>{const p=Math.round(t*rate);for(let i=0;i<rate*.07;i++)pcm[p+i]=0.24*Math.sin(2*Math.PI*(110+k*85)*i/rate)*(1-i/(rate*.07));});
  const audio={rate,frames,channels:1,duration:2,pcm},markers=parseMarkers(JSON.stringify({sampleRate:rate,loopDuration:2,slices:starts.map((t,i)=>({start:t,label:'Unequal '+(i+1)}))}),'demo.json',audio);
  imported={name:'Unequal timing demo (synthetic)',audio,markers};draw();loadEngine();say(powered?'Loading six unequal markers…':'Demo loaded. Switch on the FM1 panel to audition.');
};
$('slice-preview-play').onclick=()=>{level();send({type:'marked-play',repeat:$('slice-repeat').checked});say('Playing the original trigger positions; no step-grid quantization.');};
$('slice-preview-stop').onclick=()=>{send({type:'marked-stop'});say('Slice playback stopped.');};
$('slice-sync-play').onclick=()=>{const bpm=Number($('slice-source-bpm').value);if(!Number.isFinite(bpm)||bpm<20||bpm>300){say('Enter the original loop BPM from its source before syncing.',true);return;} imported.markers.bpm=bpm;$('slice-layer').checked=true;level();send({type:'marked-sync',bpm});say('Following the FM1 sequencer. Use instrument PLAY to stop or restart both.');};
$('slice-tempo-set').onclick=()=>{const bpm=Number($('slice-seq-bpm').value);if(!Number.isFinite(bpm)||bpm<20||bpm>275){say('FM1 tempo must be 20–275 BPM.',true);return;}if(!powered){say('Switch on the emulator first.',true);return;}send({type:'seq-tempo',bpm});};
$('slice-volume').oninput=level;$('slice-layer').onchange=level;
function database(){return new Promise((resolve,reject)=>{const q=indexedDB.open('acultur8-fm1-marked-imports-v1',1);q.onupgradeneeded=()=>q.result.createObjectStore('imports');q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);});}
$('slice-save').onclick=async()=>{try{const db=await database();await new Promise((resolve,reject)=>{const t=db.transaction('imports','readwrite');t.objectStore('imports').put(imported,'current');t.oncomplete=resolve;t.onerror=()=>reject(t.error);});db.close();say('Imported audio and exact markers saved in this browser.');}catch(e){say('Could not save this import: '+e.message,true);}};
$('slice-reopen').onclick=async()=>{try{const db=await database(),saved=await new Promise((resolve,reject)=>{const q=db.transaction('imports').objectStore('imports').get('current');q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);});db.close();if(!saved)throw Error('No saved slice import in this browser.');
  const audio=saved.audio;audio.pcm=new Float32Array(audio.pcm);if(audio.pcm.length!==audio.frames*audio.channels)throw Error('Saved audio is incomplete.');
  const markers=parseMarkers(JSON.stringify(exportMarkers(audio,saved.markers)),'saved.json',audio);imported={name:saved.name,audio,markers};draw();loadEngine();say('Reopened the saved audio and slice timing.');}catch(e){say(e.message,true);}};
$('slice-export').onclick=()=>{const json=JSON.stringify(exportMarkers(imported.audio,imported.markers),null,2);$('slice-export-json').value=json;$('slice-export-output').hidden=false;$('slice-export-output').open=true;const blob=new Blob([json],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='fm1-slice-markers.json';document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),2000);say('Marker JSON ready below; browser download requested.');};
window.addEventListener('fm1-ready',()=>{powered=true;loadEngine();});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;
  if(m.type==='frame'&&m.sync){const s=m.sync;$('slice-sync-status').textContent=s.state<0?'Sync stopped: more than eight slice tails overlap at this tempo. Lower the tempo and press Play with FM1 again.':`FM1 ${s.bpm.toFixed(1)} BPM · ${s.playing?'Playing':'Stopped'} · ${s.state===1?'Waiting for next 4-beat bar':s.state===2?'Slices follow sequencer · original pitch':'Slice sync off'}`;if(document.activeElement!==$('slice-seq-bpm'))$('slice-seq-bpm').value=Math.round(s.bpm*10)/10;}
  if(m.type==='marked-ready'){ready=true;draw();say('Ready: exact markers loaded. Play original timing or choose a slice.');}
  if(m.type==='marked-error'){ready=false;controls();say(m.message,true);}
  if(m.type==='frame'&&m.marked&&imported){
    const p=m.marked;$('slice-playhead').textContent=`${p.playing?'Playing':'Stopped'} · ${(p.cursor/44100).toFixed(3)} s · ${p.triggers} triggers`;
    $('slice-list').querySelectorAll('button').forEach(b=>b.classList.toggle('slice-active',Number(b.dataset.slice)===p.slice));
  }
});
controls();


if(new URLSearchParams(location.search).has('rex')) { say('Raw REX handoff is local-only. Import the matching WAV and JSON files here.', true); }
