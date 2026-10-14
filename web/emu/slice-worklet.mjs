// SPDX-License-Identifier: GPL-3.0-only
// Browser-only correlated marked-player controls. C remains the audio authority.
export const SLICE_CC_VERSION=0x00010001;
const integer=(v,a,b)=>Number.isInteger(v)&&v>=a&&v<=b;
const fail=reason=>{throw Error(reason);};
export function overlapCheck(info,loopSeconds=info.length/44100){
  const events=[];for(const s of info.slices){const duration=(s.end-s.start)/info.rate,t=s.time/44100;if(duration>loopSeconds*8)fail('overlap-capacity');
    for(let cycle=-8;cycle<=1;cycle++){events.push([t+cycle*loopSeconds,1]);events.push([t+cycle*loopSeconds+duration,-1]);}}
  events.sort((a,b)=>a[0]-b[0]||a[1]-b[1]);let active=0;for(const[t,d]of events){active+=d;if(t>=0&&t<loopSeconds&&active>8)fail('overlap-capacity');}
}
export function validateMarked(m){
  if(!integer(m.frames,1,1440000)||!integer(m.rate,8000,48000)||!integer(m.channels,1,2)||m.frames/m.rate>30||
    !(m.pcm instanceof Float32Array)||m.pcm.length!==m.frames*m.channels||!integer(m.length,1,2646000)||!Array.isArray(m.slices)||!integer(m.slices.length,1,64))fail('invalid-import');
  for(const v of m.pcm)if(!Number.isFinite(v)||Math.abs(v)>1)fail('invalid-audio');
  for(let i=0;i<m.slices.length;i++){const s=m.slices[i];if(!integer(s.start,0,m.frames-1)||!integer(s.end,s.start+1,m.frames)||!integer(s.time,0,m.length-1)||
    (i&&(s.start<=m.slices[i-1].start||s.time<m.slices[i-1].time)))fail('invalid-markers');}
  overlapCheck(m);
  return {...m,pcm:m.pcm.slice(),slices:m.slices.map(({start,end,time})=>({start,end,time}))};
}
export class SliceWorkletControl {
  constructor(owner,post){this.owner=owner;this.post=post;this.loaded=null;this.revision=0;this.repeat=true;this.bpm=null;}
  supported(){const e=this.owner.ex;return e?.web_slice_cc_version?.()===SLICE_CC_VERSION&&['web_cc_take_exact','web_slice_control_state','web_marked_repeat'].every(k=>typeof e[k]==='function');}
  reset(){this.loaded=null;this.repeat=true;this.bpm=null;this.revision++;}
  state(){const o=this.owner;return{version:this.supported()?SLICE_CC_VERSION:0,revision:this.revision,loaded:!!this.loaded,count:this.loaded?.slices.length||0,
    repeat:this.repeat,sourceBpm:this.bpm,bpmMilli:this.bpm===null?null:Math.round(this.bpm*1000),gain:o.markedGain??0.5,layer:!!o.markedLayer,
    gate:this.supported()?o.ex.web_slice_control_state()>>>0:null,owned:o.editOwner!==null};}
  write(info){const e=this.owner.ex;new Float32Array(this.owner.mem.buffer,e.web_marked_pcm(),info.pcm.length).set(info.pcm);
    const meta=new Uint32Array(this.owner.mem.buffer,e.web_marked_meta(),info.slices.length*3);info.slices.forEach((s,i)=>meta.set([s.start,s.end,s.time],i*3));
    return e.web_marked_load(info.frames,info.rate,info.channels,info.slices.length,info.length);}
  message(m){if(m?.type!=='slice-command')return false;
    let result=-1,reason='invalid-request';const o=this.owner,e=o.ex;
    try{
      if(!integer(m.request,1,0xffffffff)||m.abi!==1)fail(reason);
      if(o.loading||!e)fail('engine-not-ready');if(!this.supported())fail('engine-slice-abi-unavailable');
      if(m.ccRoute!==undefined&&(!o.ccInput.ready||m.ccRoute!==o.ccInput.route))fail('stale-route');
      const gate=e.web_slice_control_state()>>>0;
      if(!['stop','consume'].includes(m.op)&&(o.editOwner!==null||(gate&8)))fail('owned-gesture');
      if(!['stop','consume'].includes(m.op)&&(gate&4))fail('reserved');
      if(!['stop','consume'].includes(m.op)&&(gate&16))fail('native-job');
      if(m.op==='load'&&e.web_marked_playing())fail('slice-playing');
      if(m.op==='load'&&(gate&63))fail(gate&1?'transport-playing':gate&2?'transport-pending':gate&4?'reserved':gate&16?'native-job':'native-queue');
      if(!['load','stop','check','consume'].includes(m.op)&&!this.loaded)fail('slice-not-ready');
      if(m.op==='load'){if(m.sourceBpm!==null&&m.sourceBpm!==undefined&&(!Number.isFinite(m.sourceBpm)||m.sourceBpm<20||m.sourceBpm>300))fail('invalid-source-bpm');m={...m,prepared:validateMarked(m)};}
      if(m.op==='level'&&!integer(m.value,0,100)||['repeat','layer','play'].includes(m.op)&&!integer(m.value,0,1)||m.op==='pad'&&!integer(m.value,0,(this.loaded?.slices.length||0)-1)||['bpm','sync'].includes(m.op)&&!integer(m.value,20000,300000))fail('invalid-value');
      const sourceBpm=m.op==='sync'&&m.sourceBpm!==undefined?m.sourceBpm:m.value/1000;if(['bpm','sync'].includes(m.op)&&(!Number.isFinite(sourceBpm)||sourceBpm<20||sourceBpm>300))fail('invalid-source-bpm');
      if(['bpm','sync'].includes(m.op)&&(m.op==='sync'||o.syncOn)){const loop=(this.loaded.length/44100)*sourceBpm/e.web_seq_bpm();
        overlapCheck({...this.loaded,slices:this.loaded.slices.map(s=>({...s,time:Math.round(s.time*sourceBpm/e.web_seq_bpm())}))},loop);}
      if(!['load','level','repeat','layer','bpm','pad','play','stop','sync','check','consume'].includes(m.op))fail('invalid-operation');
      if(m.exact){if(!integer(m.exact.parameter,1104,1116)||!integer(m.exact.value,0,0x1fffff))fail('invalid-exact');
        const p=m.exact.parameter,v=m.exact.value,scalar={level:1104,repeat:1105,layer:1106,bpm:1107,pad:1108,play:1109,stop:1110,sync:1111};
        if(m.op==='consume'){if(p<1109||v>1)fail('invalid-exact-operation');}
        else if(m.op==='load'){if(![1112,1113].includes(p)||v!==1)fail('invalid-exact-operation');}
        else if(m.op==='check'){if(p<1114||p>1116||v!==1)fail('invalid-exact-operation');}
        else if(scalar[m.op]!==p||(['sync','stop','play'].includes(m.op)?v!==1:v!==m.value))fail('invalid-exact-operation');
        if(e.web_cc_take_exact(p,v)!==0)fail('exact-context-mismatch');}
      switch(m.op){
        case'load':{const previous=this.loaded;const rc=this.write(m.prepared);if(rc){if(previous&&this.write(previous))fail('import-rollback-failed');fail('c-marker-validation');}
          this.loaded=m.prepared;this.bpm=m.sourceBpm??null;this.revision++;e.web_sync_off();o.syncOn=o.markedOn=false;e.web_marked_repeat(this.repeat?1:0);break;}
        case'level':o.markedGain=m.value/100;break;
        case'layer':o.markedLayer=!!m.value;break;
        case'repeat':if(e.web_marked_repeat(m.value))fail('repeat-unavailable');this.repeat=!!m.value;break;
        case'bpm':if(o.syncOn&&e.web_sync_arm(m.value/1000))fail('sync-clock-unavailable');this.bpm=m.value/1000;break;
        case'pad':e.web_sync_off();o.syncOn=false;if(e.web_marked_pad(m.value))fail('audition-unavailable');o.markedOn=true;break;
        case'play':e.web_sync_off();o.syncOn=false;e.web_marked_play(this.repeat?1:0);o.markedOn=true;break;
        case'stop':e.web_sync_off();e.web_marked_stop();o.markedOn=o.syncOn=false;break;
        case'sync':if(e.web_sync_arm(sourceBpm))fail('sync-clock-unavailable');this.bpm=sourceBpm;o.syncOn=true;o.markedOn=false;o.markedLayer=true;break;
      }
      result=0;reason='applied';
    }catch(error){reason=error.message||String(error);}
    this.post({type:'slice-reply',request:m.request,result,reason,ccRoute:m.ccRoute,state:this.state()});return true;
  }
}
