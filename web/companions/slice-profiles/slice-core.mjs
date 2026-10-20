// SPDX-License-Identifier: GPL-3.0-only
// Analysis only. PCM is never modified; boundaries use original source frames.
export const PRESETS = [
  ['Full-band',0,0,-22,90,4,5,2], ['Kick',90,130,-20,140,5,7,3],
  ['Snare Body',400,450,-22,110,5,6,2], ['Snare Attack',2200,2400,-20,90,4,5,2],
  ['Hi-hat',7500,6500,-22,65,3,4,1], ['Percussion',1200,1600,-22,85,4,5,2]
].map(([name,frequency,bandwidth,thresholdDb,spacingMs,preRollMs,zeroMs,fadeMs])=>({name,frequency,bandwidth,thresholdDb,spacingMs,preRollMs,zeroMs,fadeMs,minDb:-60,lengthMs:0,holdMs:60,decayMs:90,levelDb:0,envelopeOn:false}));
const ranges={frequency:[0,18000],bandwidth:[20,20000],thresholdDb:[-60,-3],spacingMs:[20,2000],preRollMs:[0,50],zeroMs:[0,30],fadeMs:[0,20],minDb:[-96,0],lengthMs:[0,30000],holdMs:[0,30000],decayMs:[0,30000],levelDb:[-60,0]};
const addedDefaults={minDb:-60,lengthMs:0,holdMs:60,decayMs:90,levelDb:0};
export function validateProfile(p) {
  const out={name:String(p.name||'Custom').trim().slice(0,60)};
  for(const [k,[lo,hi]] of Object.entries(ranges)) {
    const v=Number(p[k]===undefined?addedDefaults[k]:p[k]);
    if(!Number.isFinite(v)||(k==='bandwidth'&&Number(p.frequency)===0?v!==0&&(v<lo||v>hi):v<lo||v>hi))throw Error('Invalid '+k);
    out[k]=v;
  }
  if(p.envelopeOn!==undefined&&typeof p.envelopeOn!=='boolean')throw Error('Invalid envelopeOn');
  out.envelopeOn=p.envelopeOn??false;
  return out;
}
export function playbackSettings(p={}){
  const result={envelopeOn:p.envelopeOn??false,holdMs:p.holdMs??60,decayMs:p.decayMs??90,levelDb:p.levelDb??0};
  if(typeof result.envelopeOn!=='boolean')throw Error('Invalid slice envelope flag.');
  for(const key of ['holdMs','decayMs','levelDb'])if(typeof result[key]!=='number'||!Number.isFinite(result[key])||result[key]<ranges[key][0]||result[key]>ranges[key][1])throw Error('Invalid slice '+key+'.');
  return result;
}
// Source remains immutable. Slice settings are copied when a profile is applied;
// later profile changes cannot rewrite existing instrument hits.
export function renderSlice(audio,slice,fadeMs=2){
  const p=playbackSettings(slice.playback),available=slice.end-slice.start;
  const frames=p.envelopeOn?Math.min(available,Math.round((p.holdMs+p.decayMs)*audio.rate/1000)):available;
  const pcm=new Float32Array(frames*audio.channels),fade=Math.min(Math.round(Math.max(0,fadeMs)*audio.rate/1000),Math.floor(frames/2));
  const level=10**(p.levelDb/20);
  for(let i=0;i<frames;i++){
    const t=i/audio.rate*1000;
    let gain=level;
    if(p.envelopeOn&&t>p.holdMs)gain*=p.decayMs?Math.max(0,1-(t-p.holdMs)/p.decayMs):0;
    if(fade)gain*=Math.min(1,i/fade,(frames-1-i)/fade);
    for(let ch=0;ch<audio.channels;ch++)pcm[i*audio.channels+ch]=audio.pcm[(slice.start+i)*audio.channels+ch]*gain;
  }
  return {...audio,pcm,frames,duration:frames/audio.rate};
}
export function copySliceSettings(audio,slice){return {active:slice.active,lengthMs:(slice.end-slice.start)/audio.rate*1000,playback:playbackSettings(slice.playback)};}
export function pasteSliceSettings(audio,slices,index,settings){
  if(typeof settings.active!=='boolean')throw Error('Invalid Active flag in copied settings.');
  const p=playbackSettings(settings.playback),next=setSliceLength(audio,slices,index,settings.lengthMs);
  next[index]={...next[index],active:settings.active,playback:p};return next;
}
export function resetSliceSettings(audio,slices,index,profile){
  const p=validateProfile(profile),next=validateSlices(slices,audio);
  if(!next[index])throw Error('Select a slice first.');
  const s=next[index],end=Math.min(next[index+1]?.start??audio.frames,p.lengthMs?s.start+Math.max(1,Math.round(p.lengthMs*audio.rate/1000)):audio.frames);
  next[index]={...s,end,active:true,playback:playbackSettings(p)};return next;
}
export function filterAudio(audio, profile) {
  const p=validateProfile(profile), {frames,channels,rate,pcm}=audio;
  if(p.frequency && p.frequency>=rate*.45)throw Error('Target frequency must be below 45% of the WAV sample rate. Lower the target frequency.');
  const filtered=new Float32Array(pcm.length);
  const w=2*Math.PI*p.frequency/rate, q=Math.max(.1,p.frequency/Math.max(20,p.bandwidth)), alpha=Math.sin(w)/(2*q), a0=1+alpha;
  const b0=alpha/a0,b2=-alpha/a0,a1=-2*Math.cos(w)/a0,a2=(1-alpha)/a0;
  // Separate channel energy prevents opposite-polarity stereo hits cancelling.
  for(let ch=0;ch<channels;ch++) {
    let x1=0,x2=0,y1=0,y2=0;
    for(let i=0;i<frames;i++) {
      const x=pcm[i*channels+ch],y=p.frequency?b0*x+b2*x2-a1*y1-a2*y2:x;
      x2=x1;x1=x;y2=y1;y1=y;filtered[i*channels+ch]=y;
    }
  }
  return {...audio,pcm:filtered};
}
export function bandEnergy(audio, profile) {
  const filtered=filterAudio(audio,profile), {frames,rate,channels}=audio;
  const hop=Math.max(1,Math.round(rate*.002)),count=Math.ceil(frames/hop),energy=new Float64Array(count);
  for(let i=0;i<frames;i++)for(let ch=0;ch<channels;ch++)energy[Math.floor(i/hop)]+=filtered.pcm[i*channels+ch]**2/channels;
  for(let j=0;j<count;j++)energy[j]=Math.sqrt(energy[j]/Math.min(hop,frames-j*hop));
  return {energy,hop};
}
export function refineStart(audio, onset, profile) {
  const {frames,rate,pcm,channels}=audio;
  const anchor=Math.max(0,Math.min(frames-1,Math.floor(onset-profile.preRollMs*rate/1000)));
  const lo=Math.max(0,anchor-Math.round(profile.zeroMs*rate/1000));
  let best=anchor, bestEnergy=Infinity, crossing=false;
  for(let i=anchor;i>=lo;i--) {
    let e=0;for(let ch=0;ch<channels;ch++)e+=pcm[i*channels+ch]**2;
    const cross=i===0 || pcm[i*channels]===0 || pcm[i*channels]*pcm[(i-1)*channels]<0;
    if(cross&&!crossing){crossing=true;bestEnergy=Infinity;}
    if((cross||!crossing)&&e<bestEnergy){best=i;bestEnergy=e;}
  }
  return best;
}
export function analyseDetection(audio, profile) {
  const p=validateProfile(profile),{energy,hop}=bandEnergy(audio,p), novelty=new Float64Array(energy.length);
  let slow=0, peak=0;
  for(let i=0;i<energy.length;i++) {
    novelty[i]=Math.max(0,energy[i]-slow*1.25);peak=Math.max(peak,novelty[i]);slow=.88*slow+.12*energy[i];
  }
  const threshold=peak*10**(p.thresholdDb/20),floor=10**(p.minDb/20),candidates=[],belowFloor=[];
  if(peak>=1e-5)for(let i=0;i<novelty.length;i++)if(novelty[i]>=threshold&&novelty[i]>(novelty[i-1]??-1)&&novelty[i]>=(novelty[i+1]??-1)) {
    if(energy[i]>=floor)candidates.push({i,strength:novelty[i]});else belowFloor.push(i);
  }
  const picked=[], spacing=p.spacingMs*audio.rate/1000;
  for(const c of candidates.sort((a,b)=>b.strength-a.strength))if(picked.every(v=>Math.abs(v.i-c.i)*hop>=spacing))picked.push(c);
  if(picked.length>64)throw Error('More than 64 onsets found. Increase minimum spacing or reduce sensitivity; no markers were replaced.');
  const starts=[...new Set(picked.sort((a,b)=>a.i-b.i).map(c=>refineStart(audio,c.i*hop,p)))];
  return {profile:p,energy,novelty,hop,floor,threshold,belowFloor,
    matches:picked.sort((a,b)=>a.i-b.i).map(c=>({frame:c.i*hop,db:toDb(energy[c.i])})),
    slices:starts.map((start,i)=>({start,end:Math.min(starts[i+1]??audio.frames,p.lengthMs?start+Math.max(1,Math.round(p.lengthMs*audio.rate/1000)):audio.frames),active:true,label:'Slice '+(i+1),playback:playbackSettings(p)}))};
}
export function setSliceLength(audio,slices,index,ms){
  const valid=validateSlices(slices,audio);
  if(!valid[index]||!Number.isFinite(ms)||ms<1||ms>30000)throw Error('Choose a slice length from 1 to 30000 ms.');
  const s=valid[index],end=Math.min(audio.frames,valid[index+1]?.start??audio.frames,s.start+Math.max(1,Math.round(ms*audio.rate/1000)));
  return valid.map((slice,i)=>i===index?{...slice,end}:slice);
}
export function detectSlices(audio,profile){return analyseDetection(audio,profile).slices;}
export const toDb=value=>20*Math.log10(Math.max(1e-12,value));

// Hann-windowed, averaged channel power spectrum; normalized to full-scale peak
// sine amplitude (not SPL). FFT length 4096; no stereo phase cancellation.
export function spectrum(audio,start=0,end=audio.frames) {
  start=Math.max(0,Math.floor(start));end=Math.min(audio.frames,Math.floor(end));
  if(end<=start)throw Error('Select an audio region for its spectrum.');
  const size=4096, power=new Float64Array(size/2+1),span=end-start;
  const count=Math.min(12,Math.max(1,Math.ceil(span/size)));
  for(let ch=0;ch<audio.channels;ch++)for(let win=0;win<count;win++) {
    const re=new Float64Array(size),im=new Float64Array(size);
    const offset=start+Math.round(win*Math.max(0,span-size)/Math.max(1,count-1));
    const used=Math.min(size,end-offset);let sum=0;
    for(let i=0;i<used;i++){const w=used===1?1:.5-.5*Math.cos(2*Math.PI*i/(used-1));re[i]=audio.pcm[(offset+i)*audio.channels+ch]*w;sum+=w;}
    for(let i=1,j=0;i<size;i++){let bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)[re[i],re[j]]=[re[j],re[i]];}
    for(let len=2;len<=size;len*=2){const angle=-2*Math.PI/len;
      for(let i=0;i<size;i+=len)for(let j=0;j<len/2;j++){const c=Math.cos(angle*j),s=Math.sin(angle*j),a=i+j,b=a+len/2,tr=re[b]*c-im[b]*s,ti=re[b]*s+im[b]*c;re[b]=re[a]-tr;im[b]=im[a]-ti;re[a]+=tr;im[a]+=ti;}}
    for(let i=0;i<power.length;i++)power[i]+=(re[i]**2+im[i]**2)*(i===0||i===size/2?1:4)/Math.max(1e-12,sum**2)/(count*audio.channels);
  }
  const bins=Array.from(power,(v,i)=>({hz:i*audio.rate/size,db:10*Math.log10(Math.max(1e-24,v))}));
  return {bins,resolutionHz:audio.rate/size,start,end};
}
export function validateSlices(slices,audio) {
  if(!Array.isArray(slices)||slices.length>64)throw Error('Expected up to 64 slices.');
  return slices.map((s,i)=>{
    if(!Number.isInteger(s.start)||!Number.isInteger(s.end)||s.start<0||s.end>audio.frames||s.start>=s.end||i&&s.start<=slices[i-1].start)throw Error('Invalid or unordered slice boundaries.');
    if(typeof s.active!=='boolean')throw Error('Each slice needs an Active flag.');
    return {start:s.start,end:s.end,active:s.active,label:String(s.label??i+1).slice(0,48),playback:playbackSettings(s.playback)};
  });
}
export function exportActive(audio,slices) {
  const active=validateSlices(slices,audio).filter(s=>s.active);
  if(!active.length)throw Error('Activate at least one slice before exporting markers.');
  return {version:1,sampleRate:audio.rate,audioFrames:audio.frames,channels:audio.channels,loopDuration:audio.duration,
    slices:active.map(s=>({label:s.label,startSample:s.start,endSample:s.end,time:s.start/audio.rate}))};
}
export function demoAudio(rate=44100) {
  const frames=rate*4,pcm=new Float32Array(frames);
  for(let h=0;h<8;h++)for(let j=0;j<rate*.22;j++) {
    const t=j/rate,i=Math.round((.2+h*.45)*rate)+j;
    if(i<frames)pcm[i]=.65*Math.sin(2*Math.PI*(h%2?400:90)*t)*Math.exp(-t*35)*Math.min(1,t/.001);
  }
  return {pcm,frames,rate,channels:1,duration:frames/rate};
}
