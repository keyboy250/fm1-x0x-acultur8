// SPDX-License-Identifier: GPL-3.0-only

import {readWav,parseMarkers} from './wav-import.mjs';

import {PRESETS,validateProfile,detectSlices,validateSlices,exportActive,analyseDetection,bandEnergy,filterAudio,spectrum,toDb,setSliceLength,playbackSettings,renderSlice,copySliceSettings,pasteSliceSettings,resetSliceSettings} from './slice-core.mjs';

import {drawSpectrum,drawEnergy,drawMatches} from './diagnostics.mjs';

const $=id=>document.getElementById(id), storageKey='acultur8.slice-profiles.v1';

const fields=[['frequency','Target frequency (Hz)',0,18000,10],['bandwidth','Bandwidth (Hz)',0,20000,10],['thresholdDb','Attack sensitivity (dB below peak)',-60,-3,1],['minDb','Minimum band level (dBFS)',-96,0,1],['spacingMs','Minimum spacing (ms)',20,2000,1],['preRollMs','Pre-roll (ms)',0,50,1],['zeroMs','Zero-crossing search (ms)',0,30,1],['fadeMs','Audition fade (ms)',0,20,.5],['lengthMs','Maximum slice length (ms; 0 = off)',0,30000,1],['holdMs','Default Hold (ms)',0,30000,1],['decayMs','Default Decay (ms)',0,30000,1],['levelDb','Default Level (dB)',-60,0,.5]];

let custom=[],audio=null,slices=[],pending=null,selected=0,sourceName='',fingerprint='',context=null,voices=[],generation=0,playGeneration=0;

let settingsClipboard=null;

let analysis=null,analysisKey='',spectrumData=null,spectrumKey='',lastDetection=null;

const status=message=>$('status').textContent=message;

try {const saved=JSON.parse(localStorage.getItem(storageKey)||'[]');if(!Array.isArray(saved)||saved.length>100)throw Error('Invalid profile library');custom=saved.map(validateProfile);}catch{status('Saved profiles could not be read. Factory presets are available.');}

for(const [key,label,min,max,step] of fields){const el=document.createElement('label');el.textContent=label;const input=document.createElement('input');Object.assign(input,{id:key,type:'number',min,max,step});el.append(input);$('settings').append(el);}

function profile(){return validateProfile({envelopeOn:$('profile-envelope').checked,name:$('profile-name').value||$('profile').selectedOptions[0].textContent,...Object.fromEntries(fields.map(([k])=>[k,$(k).value]))});}

function showProfile(p){for(const [k]of fields)$(k).value=p[k];$('profile-name').value=p.name;$('profile-envelope').checked=p.envelopeOn;settingsChanged();}

function profileOptions(index=0){$('profile').replaceChildren();[...PRESETS,...custom].forEach((p,i)=>{const o=document.createElement('option');o.value=i;o.textContent=p.name+(i>=PRESETS.length?' · custom':'');$('profile').append(o);});$('profile').value=index;}

profileOptions();showProfile(PRESETS[0]);

$('profile').onchange=()=>showProfile([...PRESETS,...custom][Number($('profile').value)]);

function saveCustom(p){if(!p.name)throw Error('Give the custom profile a name.');const next=[...custom,p];if(next.length>100)throw Error('Profile library limit is 100; export profiles to keep additional copies.');localStorage.setItem(storageKey,JSON.stringify(next));custom=next;profileOptions(PRESETS.length+custom.length-1);showProfile(p);status('Custom profile saved in this browser. Factory presets are unchanged.');}

function guarded(fn){return async(...args)=>{try{await fn(...args);}catch(e){status(e.message);}};}

function download(name,data){const url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}

const base=()=>sourceName.replace(/\.wav$/i,'').replace(/[^a-zA-Z0-9_-]/g,'_')||'slices';

$('save-profile').onclick=guarded(()=>saveCustom(profile()));

$('export-profile').onclick=guarded(()=>download('slice-profile.json',{type:'acultur8-slice-profile',version:2,profile:profile()}));

async function jsonFile(input){const f=input.files[0];if(!f)throw Error('Choose a JSON file.');if(f.size>262144)throw Error('JSON exceeds 256 KiB.');return JSON.parse(await f.text());}

$('import-profile').onchange=guarded(async()=>{const data=await jsonFile($('import-profile'));if(data.type!=='acultur8-slice-profile'||![1,2].includes(data.version))throw Error('Choose a Slice Profile export.');saveCustom(validateProfile(data.profile));});

function stop(){playGeneration++;for(const s of voices){try{s.stop();}catch{}}voices=[];}

$('stop').onclick=()=>{stop();status('Audio stopped.');};

async function hashAudio(a){const bytes=new TextEncoder().encode(`${a.rate}:${a.channels}:`),pcm=new Uint8Array(a.pcm.buffer,a.pcm.byteOffset,a.pcm.byteLength),all=new Uint8Array(bytes.length+pcm.length);all.set(bytes);all.set(pcm,bytes.length);const hash=await crypto.subtle.digest('SHA-256',all);return [...new Uint8Array(hash)].map(v=>v.toString(16).padStart(2,'0')).join('');}

async function loadAudio(a,name,token){const hash=await hashAudio(a);if(token!==generation)return;stop();audio=a;sourceName=name;fingerprint=hash;analysis=null;analysisKey='';spectrumKey='';lastDetection=null;$('inspect-start').value=0;$('inspect-end').value=Math.min(400,a.duration*1000);slices=[];pending=null;selected=0;$('source').textContent=`${name} · ${a.duration.toFixed(2)} s · ${a.rate} Hz · ${a.channels} channel(s). Original audio unchanged.`;$('detect').disabled=false;$('export-project').disabled=false;$('import-project').disabled=false;status('Source loaded. Choose a profile and preview detection.');render();}

$('demo').onclick=guarded(async()=>{const token=++generation,response=await fetch('./demo.wav');if(!response.ok)throw Error('Demo WAV could not be loaded.');await loadAudio(readWav(await response.arrayBuffer()),'demo.wav',token);});

$('wav').onchange=guarded(async()=>{const token=++generation,f=$('wav').files[0];if(!f)return;if(f.size>16*1024*1024)throw Error('Choose a WAV below 16 MiB.');const a=readWav(await f.arrayBuffer());await loadAudio(a,f.name,token);});

$('detect').onclick=guarded(()=>{lastDetection=analyseDetection(audio,profile());analysis=lastDetection;analysisKey=JSON.stringify(profile());pending=lastDetection.slices;render();status(`${pending.length} proposed slices. Current markers and Active flags are unchanged until you choose Use proposed markers.`);});

$('apply').onclick=()=>{stop();slices=pending;pending=null;selected=0;render();status(`${slices.length} slices accepted. Review and deactivate unwanted hits.`);};

$('discard').onclick=()=>{pending=null;render();status('Current markers and edits kept.');};

function choose(i){selected=Math.max(0,Math.min(slices.length-1,i));const hit=slices[selected];if(hit){$('inspect-start').value=hit.start/audio.rate*1000;$('inspect-end').value=Math.min(hit.end,hit.start+audio.rate*.15)/audio.rate*1000;}render();$('strip').children[selected]?.scrollIntoView({block:'nearest',inline:'nearest'});}

$('previous').onclick=()=>choose(selected-1);$('next').onclick=()=>choose(selected+1);

$('strip').onkeydown=e=>{if(e.key==='ArrowRight'||e.key==='ArrowLeft'){e.preventDefault();choose(selected+(e.key==='ArrowRight'?1:-1));$('strip').children[selected]?.focus();}};

$('active').onchange=()=>{stop();slices[selected].active=$('active').checked;render();};

$('label').onchange=()=>{slices[selected].label=$('label').value.slice(0,48);render();};

$('bounds').onclick=guarded(()=>{const start=Math.round(Number($('start').value)*audio.rate/1000),end=Math.round(Number($('end').value)*audio.rate/1000),next=slices.map((s,i)=>i===selected?{...s,start,end}:s);validateSlices(next,audio);stop();slices=next;render();status('Boundaries updated. Original audio unchanged.');});

$('set-length').onclick=guarded(()=>{const next=setSliceLength(audio,slices,selected,Number($('slice-length').value));stop();slices=next;render();status('Selected slice length set to '+((slices[selected].end-slices[selected].start)/audio.rate*1000).toFixed(1)+' ms, bounded by the next slice or source end. Trigger timing unchanged.');});

function wave(id,lo,hi,markers){const canvas=$(id),g=canvas.getContext('2d'),w=canvas.width,h=canvas.height;g.clearRect(0,0,w,h);if(!audio)return;for(const m of markers)if(Number.isFinite(m.end)){g.fillStyle=m.color+'24';g.fillRect((m.start-lo)/(hi-lo)*w,0,(m.end-m.start)/(hi-lo)*w,h);}g.strokeStyle='#587b8c';g.beginPath();for(let x=0;x<w;x++){let min=0,max=0;const a=Math.floor(lo+x*(hi-lo)/w),b=Math.min(hi,Math.max(a+1,Math.floor(lo+(x+1)*(hi-lo)/w)));for(let i=a;i<b;i++)for(let ch=0;ch<audio.channels;ch++){const v=audio.pcm[i*audio.channels+ch];min=Math.min(min,v);max=Math.max(max,v);}g.moveTo(x,h/2-min*h*.44);g.lineTo(x,h/2-max*h*.44);}g.stroke();for(const m of markers){g.strokeStyle=m.color;g.beginPath();const x=(m.start-lo)/(hi-lo)*w;g.moveTo(x,0);g.lineTo(x,h);g.stroke();}}

function render(){const count=slices.filter(s=>s.active).length,s=slices[selected];$('pending').hidden=pending===null;$('pending-count').textContent=pending===null?'':`${pending.length} proposed markers (orange). Applying replaces current boundaries and Active choices.`;$('apply').disabled=!pending?.length;$('strip').replaceChildren();slices.forEach((slice,i)=>{const b=document.createElement('button');b.className='slice'+(slice.active?'':' inactive');b.setAttribute('aria-pressed',String(i===selected));b.textContent=`${i+1} · ${slice.label}`;const small=document.createElement('small');small.textContent=(slice.active?'Active':'Inactive')+` · ${(slice.start/audio.rate).toFixed(2)} s`;b.append(small);const duration=document.createElement('small');duration.textContent=((slice.end-slice.start)/audio.rate*1000).toFixed(0)+' ms';b.append(duration);b.onclick=()=>choose(i);$('strip').append(b);});$('edit').disabled=!s;$('previous').disabled=!s||selected===0;$('next').disabled=!s||selected===slices.length-1;$('audition').disabled=!s;$('play').disabled=!count;$('export-markers').disabled=!count;if(s){$('selected').textContent=`Slice ${selected+1} of ${slices.length} · ${count} active`;$('active').checked=s.active;$('label').value=s.label;$('start').value=(s.start/audio.rate*1000).toFixed(3);$('end').value=(s.end/audio.rate*1000).toFixed(3);$('slice-length').value=((s.end-s.start)/audio.rate*1000).toFixed(3);const sound=playbackSettings(s.playback);$('slice-envelope').checked=sound.envelopeOn;$('slice-hold').value=sound.holdMs;$('slice-decay').value=sound.decayMs;$('slice-level').value=sound.levelDb;$('paste-settings').disabled=!settingsClipboard;$('slice-sound-info').textContent=sound.envelopeOn?'Hold '+sound.holdMs+' ms, then '+sound.decayMs+' ms linear decay; '+sound.levelDb+' dB. Source length may end sooner.':'Envelope off · full source length at '+sound.levelDb+' dB.';}else $('selected').textContent='No slice selected';wave('overview',0,audio?.frames||1,[...slices.map(x=>({...x,color:x.active?'#9ce5c6':'#72818c'})),...(pending||[]).map(x=>({...x,color:'#ffbe75'}))]);wave('detail',s?.start||0,s?.end||audio?.frames||1,[]);if(audio&&lastDetection)drawMatches($('overview'),audio,lastDetection.matches);updateDiagnostics();}

async function play(items,originalTiming,source=audio){

  stop();const token=playGeneration;context??=new AudioContext();await context.resume();if(token!==playGeneration)return;

  const p=profile(),when=context.currentTime+.05;

  for(const s of items){

    const rendered=renderSlice(source,s,p.fadeMs);if(!rendered.frames)continue;

    const buffer=context.createBuffer(source.channels,rendered.frames,source.rate);

    for(let ch=0;ch<source.channels;ch++){const a=buffer.getChannelData(ch);for(let i=0;i<rendered.frames;i++)a[i]=rendered.pcm[i*source.channels+ch];}

    const src=context.createBufferSource(),gain=context.createGain(),start=when+(originalTiming?s.start/source.rate:0);src.buffer=buffer;src.connect(gain);gain.connect(context.destination);gain.gain.setValueAtTime(Number($('level').value),start);src.start(start);

    src.onended=()=>{src.disconnect();gain.disconnect();voices=voices.filter(v=>v!==src);};voices.push(src);

  }

  status(originalTiming?'Playing active slices with their own sound settings at original trigger positions.':'Auditioning selected slice with its own sound settings.');

}

$('audition').onclick=guarded(()=>play([slices[selected]],false));$('play').onclick=guarded(()=>play(slices.filter(s=>s.active),true));

$('export-markers').onclick=guarded(()=>{const markers=exportActive(audio,slices);parseMarkers(JSON.stringify(markers),'markers.json',audio);download(base()+'_markers.json',markers);status('Cuts-only markers exported. Markers omit sound settings. Export a review project and import it with the original WAV in the emulator to play Hold, Decay and Level.');});

$('export-project').onclick=guarded(()=>download(base()+'_slice-review.json',{type:'acultur8-slice-review',version:2,audio:{name:sourceName,sha256:fingerprint,rate:audio.rate,channels:audio.channels,frames:audio.frames},profile:profile(),slices:validateSlices(slices,audio)}));

$('import-project').onchange=guarded(async()=>{const data=await jsonFile($('import-project'));if(data.type!=='acultur8-slice-review'||![1,2].includes(data.version))throw Error('Choose a Slice Review project.');if(data.audio?.sha256!==fingerprint||data.audio?.rate!==audio.rate||data.audio?.channels!==audio.channels||data.audio?.frames!==audio.frames)throw Error('This project belongs to different audio. Load its original WAV first; existing edits are unchanged.');const p=validateProfile(data.profile),restored=validateSlices(data.slices,audio);if(slices.length&&!window.confirm('Replace current slice edits with this review project?'))return;stop();showProfile(p);slices=restored;pending=null;selected=0;render();status('Review project reopened, including inactive slices.');});

$('apply-sound').onclick=guarded(()=>{

  const sound=playbackSettings({envelopeOn:$('slice-envelope').checked,holdMs:Number($('slice-hold').value),decayMs:Number($('slice-decay').value),levelDb:Number($('slice-level').value)});

  stop();slices=slices.map((s,i)=>i===selected?{...s,playback:sound}:s);render();status('Selected slice sound saved in this review. Other slices are unchanged.');

});

$('copy-settings').onclick=()=>{settingsClipboard=copySliceSettings(audio,slices[selected]);render();status('Slice settings copied: Active, Length, Hold, Decay and Level.');};

$('paste-settings').onclick=guarded(()=>{if(!settingsClipboard)throw Error('Copy a slice first.');stop();slices=pasteSliceSettings(audio,slices,selected,settingsClipboard);render();status('Settings pasted. Destination start and label are unchanged; length is bounded by its next slice/source end.');});

$('reset-settings').onclick=guarded(()=>{stop();slices=resetSliceSettings(audio,slices,selected,profile());render();status('Selected slice reset to current profile defaults and Active. Start and label kept.');});

function inspectRange(){

  if(!audio)return {start:0,end:1};

  const start=Math.round(Number($('inspect-start').value)*audio.rate/1000),end=Math.round(Number($('inspect-end').value)*audio.rate/1000);

  if(!Number.isFinite(start)||!Number.isFinite(end)||start<0||end>audio.frames||end<=start)throw Error('Choose an inspection region inside this WAV, with End after Start.');

  return {start,end};

}

function settingsChanged(){

  analysis=null;analysisKey='';lastDetection=null;pending=null;

  if(audio){render();status('Detection settings changed. Preview detection to see new magenta matches; accepted edits are kept.');}

}

for(const [key]of fields)$(key).addEventListener('change',settingsChanged);

$('profile-envelope').addEventListener('change',settingsChanged);

function updateDiagnostics(){

  for(const id of ['spectrum','energy']){const c=$(id),width=Math.max(240,Math.round(c.clientWidth));if(c.width!==width)c.width=width;}

  for(const id of ['inspect','hear-band','hear-band-loop'])$(id).disabled=!audio;

  if(!audio){drawSpectrum($('spectrum'),null);drawEnergy($('energy'),null);return;}

  try{

    const p=profile(),range=inspectRange(),key=JSON.stringify(p);

    if(analysisKey!==key||!analysis){analysis={...bandEnergy(audio,p),profile:p};analysisKey=key;}

    const skey=range.start+':'+range.end;

    if(spectrumKey!==skey){spectrumData=spectrum(audio,range.start,range.end);spectrumKey=skey;}

    drawSpectrum($('spectrum'),spectrumData,p,audio.rate);

    drawEnergy($('energy'),audio,analysis,range,Boolean(lastDetection));

    const peaks=spectrumData.bins.filter((b,i,bins)=>b.hz>=20&&b.db>-90&&b.db>(bins[i-1]?.db??-240)&&b.db>=(bins[i+1]?.db??-240)).sort((a,b)=>b.db-a.db).slice(0,3);

    $('spectrum-info').textContent=`Original region ${(range.start/audio.rate).toFixed(3)}–${(range.end/audio.rate).toFixed(3)} s · FFT spacing ${spectrumData.resolutionHz.toFixed(1)} Hz · `+(peaks.length?'Prominent peaks: '+peaks.map(b=>Math.round(b.hz)+' Hz ('+b.db.toFixed(1)+' dBFS)').join(', '):'No significant peaks in this region.');

    const maxDb=toDb(Math.max(...analysis.energy));

    $('energy-info').textContent=`Band RMS peak ${maxDb.toFixed(1)} dBFS · minimum ${p.minDb} dBFS · `+(lastDetection?`${lastDetection.matches.length} magenta matches; ${lastDetection.belowFloor.length} attack candidates below the minimum level. `:'Preview detection to show matched attacks. ')+`Energy is stereo-averaged RMS in 2 ms windows, not loudness or SPL.`;

  }catch(e){$('spectrum-info').textContent=e.message;}

}

$('inspect').onclick=guarded(()=>{inspectRange();updateDiagnostics();});

$('overview').addEventListener('click',event=>{

  if(!audio)return;const rect=$('overview').getBoundingClientRect(),frame=Math.max(0,Math.min(audio.frames-1,Math.round((event.clientX-rect.left)/rect.width*audio.frames)));

  const start=Math.max(0,frame-Math.round(audio.rate*.01)),end=Math.min(audio.frames,start+Math.round(audio.rate*.15));

  $('inspect-start').value=start/audio.rate*1000;$('inspect-end').value=end/audio.rate*1000;updateDiagnostics();

});

async function hearBand(full){const range=full?{start:0,end:audio.frames}:inspectRange(),source=filterAudio(audio,profile());await play([range],false,source);status(full?'Hearing the detection band for the full loop. No normalization; original audio unchanged.':'Hearing the detection band for the inspected region. No normalization; original audio unchanged.');}

$('hear-band').onclick=guarded(()=>hearBand(false));$('hear-band-loop').onclick=guarded(()=>hearBand(true));

window.addEventListener('resize',()=>requestAnimationFrame(updateDiagnostics));

window.addEventListener('pagehide',stop);render();

