// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';
const wasm=fs.readFileSync(process.argv[2]);
async function setup(bpm=120,length=88200,count=8,span=1024){
 let mem;const {instance}=await WebAssembly.instantiate(wasm,{wasi_snapshot_preview1:{clock_time_get:(a,b,p)=>{new DataView(mem.buffer).setBigUint64(p,0n,true);return 0;}}});
 const e=instance.exports;mem=e.memory;e._initialize();e.web_boot();e.web_seq_set_bpm(bpm);
 const pcm=new Float32Array(mem.buffer,e.web_marked_pcm(),count*span);
 for(let j=0;j<count;j++)for(let i=0;i<span;i++)pcm[j*span+i]=i===0?1:Math.sin(2*Math.PI*440*i/44100)*.1;
 new Uint32Array(mem.buffer,e.web_marked_meta(),count*3).set(Array.from({length:count},(_,i)=>[i*span,(i+1)*span,Math.round(i*length/count)]).flat());
 assert.equal(e.web_marked_load(count*span,44100,1,count,length),0);
 const render=(n)=>{let samples=[];while(n){const k=Math.min(128,n);e.web_render(k);samples.push(...new Float32Array(mem.buffer,e.web_sync_l(),k));n-=k;}return samples;};
 return {e,render,pcm:Array.from(pcm)};
}
const evidence=[];
for(const bpm of [90,120,180]){
 const {e,render,pcm}=await setup(bpm);assert.equal(e.web_sync_arm(120),0);
 const samples=render(Math.ceil(44100*60/bpm*4)+2048);
 const onsets=samples.flatMap((v,i)=>v>.9?[i]:[]);
 assert.ok(onsets.length>=9);
 for(let i=0;i<1024;i++)assert.ok(Math.abs(samples[onsets[0]+i]-pcm[i])<1e-7,'unchanged 440 Hz waveform at '+bpm);
 assert.ok(Math.abs(onsets[8]-onsets[0]-44100*60/bpm*4)<100,'sequencer loop interval at '+bpm);
 evidence.push({bpm,onsets:onsets.slice(0,9),pitchSamplesUnchanged:true});
}
{
 const {e,render}=await setup();e.web_seq_play(1);render(44100);assert.equal(e.web_sync_arm(120),0);
 assert.equal(e.web_sync_state(),1);assert.equal(e.web_marked_triggers(),0);
 render(20000);assert.equal(e.web_sync_state(),1);assert.equal(e.web_marked_triggers(),0);
 render(26000);assert.equal(e.web_sync_state(),2);assert.ok(e.web_seq_beat()>=4);
 const before=e.web_marked_triggers();e.web_seq_set_bpm(180);render(15000);assert.ok(e.web_marked_triggers()>before);
 e.web_seq_play(0);const tail=render(2048);assert.equal(e.web_seq_playing(),0);assert.ok(tail.slice(-1024).every(v=>v===0));
 e.web_seq_play(1);render(1024);assert.equal(e.web_marked_triggers(),1,'restart from slice one');
 e.web_sync_off();assert.ok(render(2048).slice(-1024).every(v=>v===0));assert.equal(e.web_seq_playing(),1,'stop slices keeps pattern running');
 evidence.push({joinNextBar:true,liveTempoChange:true,stopRestart:true,independentSliceStop:true});
}
{
 const {e,render}=await setup(120,77175,7);e.web_sync_arm(120);
 const samples=render(80000),onsets=samples.flatMap((v,i)=>v>.9?[i]:[]);
 assert.ok(Math.abs(onsets[7]-onsets[0]-77175)<100,'7/8 retains 3.5 beats');
 evidence.push({sevenEighths:true,loopSamples:onsets[7]-onsets[0]});
}
{
 const {e,render}=await setup(275,4410,8,4410);e.web_sync_arm(120);render(12000);assert.equal(e.web_sync_state(),-1,'overlap rejected without stealing voices');
 evidence.push({overlapGuard:true});
}
console.log(JSON.stringify(evidence,null,2));
