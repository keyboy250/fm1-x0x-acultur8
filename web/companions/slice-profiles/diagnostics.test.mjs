import assert from 'node:assert/strict';
import {PRESETS,validateProfile,analyseDetection,bandEnergy,filterAudio,spectrum,demoAudio,toDb} from './slice-core.mjs';
const audio=demoAudio(),original=audio.pcm.slice(),p={...PRESETS[2],bandwidth:150},counts=[];
for(const minDb of [-96,-40,-25,-10,0])counts.push(analyseDetection(audio,{...p,minDb}).matches.length);
for(let i=1;i<counts.length;i++)assert.ok(counts[i]<=counts[i-1],'higher minimum floor cannot add matches on this fixture');
assert.equal(counts.at(-1),0);
const result=analyseDetection(audio,{...p,minDb:-25});
assert.ok(result.belowFloor.length>0);assert.equal(result.matches.length,result.slices.length);
assert.ok(result.matches.every(m=>m.db>=-25));
const filtered=filterAudio(audio,p),energy=bandEnergy(audio,p);
for(let w=0;w<energy.energy.length;w++){
 let sum=0,n=0;for(let i=w*energy.hop;i<Math.min(audio.frames,(w+1)*energy.hop);i++){sum+=filtered.pcm[i]**2;n++;}
 assert.ok(Math.abs(Math.sqrt(sum/n)-energy.energy[w])<1e-12,'audition PCM and plotted RMS use identical filter');
}
assert.deepEqual(audio.pcm,original);
const old={...PRESETS[0]};delete old.minDb;assert.equal(validateProfile(old).minDb,-60,'old profiles migrate with default minimum');
assert.throws(()=>validateProfile({...p,minDb:1}),/minDb/);
assert.throws(()=>validateProfile({...p,minDb:NaN}),/minDb/);
const rate=48000,frames=48000,pcm=new Float32Array(frames);
for(let i=0;i<frames;i++)pcm[i]=.5*Math.sin(2*Math.PI*1000*i/rate);
const sine={pcm,frames,rate,channels:1,duration:1},spec=spectrum(sine,0,4096),peak=spec.bins.reduce((a,b)=>a.db>b.db?a:b);
assert.ok(Math.abs(peak.hz-1000)<=spec.resolutionHz);assert.ok(Math.abs(peak.db-20*Math.log10(.5))<1,'Hann full scale amplitude calibration within bin leakage');
const e=bandEnergy(sine,PRESETS[0]);const rms=Array.from(e.energy).reduce((a,b)=>a+b,0)/e.energy.length;
assert.ok(Math.abs(toDb(rms)+9.03)<.05,'half scale sine RMS is -9 dBFS');
const short=spectrum(sine,0,100);assert.ok(short.bins.every(b=>Number.isFinite(b.db)));
const silent=spectrum({...sine,pcm:new Float32Array(frames)},0,4096);assert.ok(silent.bins.every(b=>b.db===-240));
const opposite=new Float32Array(frames*2);for(let i=0;i<frames;i++){opposite[2*i]=pcm[i];opposite[2*i+1]=-pcm[i];}
const stereoPeak=spectrum({...sine,pcm:opposite,channels:2},0,4096).bins.reduce((a,b)=>a.db>b.db?a:b);assert.ok(Math.abs(stereoPeak.db-peak.db)<1e-6);
console.log(JSON.stringify({status:'PASS',floorSweep:counts,checks:['absolute-RMS-floor','matches-meet-floor','below-floor-rejection','identical-analysis-audition-filter','original-PCM-preserved','old-profile-migration','invalid-minimum','FFT-frequency-and-amplitude','RMS-calibration','short-region','silent-spectrum','stereo-spectrum-no-cancellation']}));
