// SPDX-License-Identifier: GPL-3.0-only
export const CLOCK_PROTOCOL=0x00010001;
const u32=n=>Number.isInteger(n)&&n>=0&&n<=0xffffffff;
// This bounded synthetic bridge carries no event timestamps: browser timing
// leadership is still unqualified. It never connects a port or starts a role.
export class ClockWorkletBridge {
 constructor(get,post,getMemory){this.get=get;this.post=post;this.getMemory=getMemory;}
 supported(){const e=this.get();return !!e&&['web_clock_version','web_clock_command','web_clock_retire','web_clock_expected','web_clock_snapshot','web_midi_ingest','web_midi_output_version','web_midi_transport_epoch'].every(k=>typeof e[k]==='function')&&e.web_clock_version()===CLOCK_PROTOCOL&&e.web_midi_output_version()===0x00020001;}
 snapshot(){if(!this.supported())return null;const p=this.get().web_clock_snapshot(CLOCK_PROTOCOL),mem=this.getMemory();if(!Number.isInteger(p)||p<=0||p%4||!mem||p+40>mem.buffer.byteLength)return null;const v=Array.from(new Uint32Array(mem.buffer,p,10));if(v[0]!==CLOCK_PROTOCOL||v[2]>3||v[3]>2||v[4]>4)return null;return{protocol:v[0],generation:v[1],role:v[2],source:v[3],state:v[4],tickLo:v[5],tickHi:v[6],ageSamples:v[7],flags:v[8],pending:v[9]};}
 message(m){if(!['clock-command','clock-source-retire','clock-input','clock-status'].includes(m?.type))return false;
  const reply={type:'clock-reply',protocol:CLOCK_PROTOCOL,request:m.request,result:-1,acceptedGeneration:null,status:null};
  const valid=u32(m.request)&&m.protocol===CLOCK_PROTOCOL;
  if(valid&&this.supported()){
   const e=this.get(),status=this.snapshot();reply.status=status;
   if(status){
    if(m.type==='clock-status')reply.result=0;
    else if(m.type==='clock-command'&&Number.isInteger(m.role)&&m.role>=0&&m.role<=3&&Number.isInteger(m.source)&&m.source>=0&&m.source<=2&&m.policy===0){reply.result=e.web_clock_command(CLOCK_PROTOCOL,m.role,m.source,0);if(reply.result===0)reply.acceptedGeneration=e.web_clock_expected()>>>0;}
    else if(m.type==='clock-source-retire'&&m.source===2){reply.result=e.web_clock_retire(CLOCK_PROTOCOL,2);if(reply.result===0)reply.acceptedGeneration=e.web_clock_expected()>>>0;}
    else if(m.type==='clock-input'&&m.source===2&&u32(m.generation)&&m.generation===status.generation&&status.generation!==0xffffffff&&status.role===3&&status.source===2&&!status.pending&&!(status.flags&1)&&Array.isArray(m.packets)&&m.packets.length>0&&m.packets.length<=32&&m.packets.every(p=>u32(p)&&(p&15)===15&&[248,250,251,252].includes((p>>>8)&255)&&(p&0xffff0000)===0)){
      reply.result=0;reply.accepted=0;
      for(const p of m.packets){const rc=e.web_midi_ingest(p,2);if(rc!==0){reply.result=rc;break;}reply.accepted++;}
      // Receipt count is distinct from later applied pulse/transport status.
    }
   }
  }
  this.post(reply);return true;
 }
}
