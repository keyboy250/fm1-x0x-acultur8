// SPDX-License-Identifier: GPL-3.0-only
import {readWav,parseMarkers,exportMarkers} from './slice-import.js';
export const SLICE_IDS=['level_percent','repeat','layer','source_bpm_milli','audition_index','play','stop','arm_sync','import','demo','save_import','reopen_import','export'];
const ranges=[[0,100],[0,1],[0,1],[20000,300000],[0,63],[0,1],[0,1],[0,1],[0,1],[0,1],[0,1],[0,1],[0,1]];
const err=reason=>Object.assign(Error(reason),{reason});
export function sliceWire(packet){const status=(packet>>>8)&255,cc=(packet>>>16)&255;return status===0xbc&&cc>=80&&cc<=92?cc-80:null;}
export function sliceScale(index,raw){const[a,b]=ranges[index];return Math.round(a+raw*(b-a)/127);}
export function syntheticImport(){const rate=44100,frames=rate*2,pcm=new Float32Array(frames),starts=[0,.13,.51,.78,1.17,1.63];
 starts.forEach((t,k)=>{const p=Math.round(t*rate);for(let i=0;i<rate*.07;i++)pcm[p+i]=.24*Math.sin(2*Math.PI*(110+k*85)*i/rate)*(1-i/(rate*.07));});
 const audio={rate,frames,channels:1,duration:2,pcm},markers=parseMarkers(JSON.stringify({sampleRate:rate,loopDuration:2,slices:starts.map((t,i)=>({start:t,label:'Unequal '+(i+1)}))}),'demo.json',audio);return{name:'Unequal timing demo (synthetic)',audio,markers};}
export async function selectedImport(wav,marker){if(!wav||!marker)throw err('select-matching-files-first');if(wav.size>16*1024*1024||marker.size>262144)throw err('file-size-limit');
 const [bytes,text]=await Promise.all([wav.arrayBuffer(),marker.text()]);
 if(marker.name.toLowerCase().endsWith('.json')){const meta=JSON.parse(text);if(meta.audioFile&&meta.audioFile!==wav.name)throw err('audio-file-mismatch');if(meta.audioSha256){
  const hash=[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(v=>v.toString(16).padStart(2,'0')).join('');if(hash!==meta.audioSha256)throw err('audio-checksum-mismatch');}}
 const audio=readWav(bytes),markers=parseMarkers(text,marker.name,audio);if(markers.bpm!==null&&(markers.bpm<20||markers.bpm>300))throw err('invalid-source-bpm');return{name:wav.name,audio,markers};}
function database(){return new Promise((resolve,reject)=>{const q=indexedDB.open('acultur8-fm1-beta-0127-marked-imports-v1',1);q.onupgradeneeded=()=>q.result.createObjectStore('imports');q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);});}
export class SliceService {
 constructor({send,files=()=>[],notify=()=>{},changed=()=>{},download=()=>false,databaseOpen=database}={}){Object.assign(this,{send,files,notify,changed,download,databaseOpen});this.imported=null;this.powered=false;this.state=null;this.frame=null;this.serial=0;this.pending=new Map();this.busy=false;this.edges=new Map();this.epoch=0;}
 update(){this.changed(this);}
 ready(){return this.powered&&this.state?.version===0x10001;}
 engineReset(){this.epoch++;this.powered=false;this.edges.clear();for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(err('engine-reset'));}this.pending.clear();this.state=null;this.update();}
 onMessage(m){if(m?.type==='frame'){this.frame=m;if(m.ccInput?.slice)this.state=m.ccInput.slice;this.update();return;}
  if(m?.type!=='slice-reply')return;const p=this.pending.get(m.request);if(!p)return;clearTimeout(p.timer);this.pending.delete(m.request);this.state=m.state;
  if(m.result===0&&p.candidate){this.imported=p.candidate;this.update();} // accepted even if route invalidates immediately after receipt
  if(m.result===0)p.resolve(m);else p.reject(err(m.reason));this.update();}
 command(op,value,ctx={},candidate=null){if(!this.ready())return Promise.reject(err('engine-slice-abi-unavailable'));const request=this.serial=((this.serial+1)>>>0)||1;
  return new Promise((resolve,reject)=>{const timer=setTimeout(()=>{this.pending.delete(request);reject(err('slice-receipt-timeout'));},5000);this.pending.set(request,{resolve,reject,timer,candidate});
   try{this.send({type:'slice-command',abi:1,request,op,value,...(ctx.route!==undefined?{ccRoute:ctx.route}:{}),...(ctx.exact?{exact:ctx.exact}:{}),...(op==='sync'&&ctx.sourceBpm!==undefined?{sourceBpm:ctx.sourceBpm}:{}),...(candidate?{sourceBpm:candidate.markers.bpm,...candidate.audio,pcm:candidate.audio.pcm,slices:candidate.markers.slices,length:candidate.markers.length}: {})});}catch(e){clearTimeout(timer);this.pending.delete(request);reject(e);}});}
 requireCurrent(ctx,epoch){if(epoch!==this.epoch||ctx.signal?.aborted||ctx.current&& !ctx.current())throw err('stale-route');}
 async storage(write,data,ctx,epoch){const db=await this.databaseOpen();try{this.requireCurrent(ctx,epoch);return await new Promise((resolve,reject)=>{
   const tx=db.transaction('imports',write?'readwrite':'readonly'),store=tx.objectStore('imports');let result;const abort=()=>{try{tx.abort();}catch{}};ctx.signal?.addEventListener('abort',abort,{once:true});
   const q=write?store.put(data,'current'):store.get('current');q.onsuccess=()=>{result=q.result;};
   const finish=()=>ctx.signal?.removeEventListener('abort',abort);tx.oncomplete=()=>{finish();resolve(result);};tx.onabort=()=>{finish();reject(err(ctx.signal?.aborted?'stale-route':'storage-aborted'));};tx.onerror=()=>{finish();reject(err('storage-failed'));};
  });}finally{db.close();}}
 async load(candidate,ctx,epoch){this.requireCurrent(ctx,epoch);await this.command('load',0,ctx,candidate);this.requireCurrent(ctx,epoch);}
 async replace(candidate,ctx={}){if(this.busy)throw err('slice-operation-busy');this.busy=true;this.update();const epoch=this.epoch;try{await this.load(candidate,ctx,epoch);}finally{this.busy=false;this.update();}}
 async perform(index,value,ctx={}){
  if(!Number.isInteger(index)||index<0||index>=13||!Number.isInteger(value)||value<ranges[index][0]||value>ranges[index][1])throw err('invalid-value');
  if(this.busy)throw err('slice-operation-busy');if(!this.powered)throw err('engine-not-powered');if(!this.ready())throw err('engine-slice-abi-unavailable');
  const epoch=this.epoch;this.requireCurrent(ctx,epoch);this.busy=true;this.update();try{
   const op=['level','repeat','layer','bpm','pad','play','stop','sync'][index];
   if(index<8){if(index>=5&&!value)return{status:'noop',reason:'zero-rearm'};
    if(index===7){const bpm=this.imported?.markers.bpm;if(!Number.isFinite(bpm)||bpm<20||bpm>300)throw err('set-source-bpm-first');value=Math.round(bpm*1000);ctx={...ctx,sourceBpm:bpm};}
    const reply=await this.command(op,value,ctx);if(index===3&&this.imported)this.imported.markers.bpm=value/1000;
    this.update();return{status:'applied',reason:'applied',revision:reply.state.revision};}
   if(!value)return{status:'noop',reason:'zero-rearm'};
   if(index===8||index===9){const candidate=index===9?syntheticImport():await selectedImport(...this.files());this.requireCurrent(ctx,epoch);await this.load(candidate,ctx,epoch);return{status:'applied',reason:'import-loaded',revision:this.state.revision};}
   if(index===10){if(!this.imported)throw err('slice-not-ready');await this.command('check',0,ctx);const saved={name:this.imported.name,audio:{...this.imported.audio,pcm:this.imported.audio.pcm.slice()},markers:structuredClone(this.imported.markers)};
    await this.storage(true,saved,ctx,epoch);return{status:'applied',reason:'browser-storage-committed'};}
   if(index===11){await this.command('check',0,ctx);const saved=await this.storage(false,null,ctx,epoch);this.requireCurrent(ctx,epoch);if(!saved)throw err('no-saved-import');
    const audio={...saved.audio,pcm:new Float32Array(saved.audio.pcm)};if(audio.pcm.length!==audio.frames*audio.channels)throw err('saved-audio-incomplete');
    const markers=parseMarkers(JSON.stringify(exportMarkers(audio,saved.markers)),'saved.json',audio);if(markers.bpm!==null&&(markers.bpm<20||markers.bpm>300))throw err('invalid-source-bpm');await this.load({name:saved.name,audio,markers}, {...ctx,exact:null},epoch);return{status:'applied',reason:'saved-import-loaded',revision:this.state.revision};}
   if(index===12){if(!this.imported)throw err('slice-not-ready');await this.command('check',0,ctx);this.requireCurrent(ctx,epoch);const json=JSON.stringify(exportMarkers(this.imported.audio,this.imported.markers),null,2);const requested=!!this.download(json,!!ctx.cc);
    return{status:'applied',reason:requested?'json-prepared-download-requested':'json-prepared-download-not-requested',downloadDelivered:false};}
  }finally{this.busy=false;this.update();}
 }
 async applyPacket(packet,ctx={}){const index=ctx.parameter!==undefined?ctx.parameter-1104:sliceWire(packet);if(index===null||index<0||index>=13)return{status:'rejected',reason:'not-browser-slice'};
  const value=ctx.exact?ctx.exact.value:sliceScale(index,packet>>>24),key=`${ctx.route}:${index}`;
  if(!Number.isInteger(value)||value<ranges[index][0]||value>ranges[index][1])return{status:'rejected',reason:'invalid-value'};
  if(index>=5&&(!value||this.edges.has(key))){try{if(ctx.exact)await this.command('consume',0,ctx);}catch(e){return{status:'rejected',reason:e.reason||e.message};}if(!value)this.edges.delete(key);return{status:'noop',reason:!value?'zero-rearm':'held-edge'};}
  try{const result=await this.perform(index,value,{...ctx,cc:true});if(index>=5&&result.status==='applied')this.edges.set(key,true);this.notify(`${SLICE_IDS[index]}: ${result.reason}`);return result;}
  catch(error){const result={status:'rejected',reason:error.reason||error.message||String(error)};this.notify(`${SLICE_IDS[index]} rejected: ${result.reason}`,true);return result;}
 }
}
