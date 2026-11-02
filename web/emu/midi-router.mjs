// SPDX-License-Identifier: GPL-3.0-only
// Local-only WebMIDI adapter. Device IDs are never saved. Successful send() means
// API queue acceptance, never receiver delivery. No permission request at startup.
export function midiBytes(packet){
  if(!Number.isInteger(packet)||packet<0||packet>0xffffffff)return null;
  const p=packet>>>0,cin=p&15,status=(p>>>8)&255;
  const lengths={2:2,3:3,5:1,8:3,9:3,10:3,11:3,12:2,13:2,14:3,15:1},n=lengths[cin];
  if(!n||status<128||status===240||status===247)return null;
  if((cin===2&&status!==241&&status!==243)||(cin===3&&status!==242)||
    (cin===5&&status!==246)||(cin===15&&status<248))return null;
  if(cin>=8&&cin<=14&&status>>4!==cin)return null;
  const bytes=[status,(p>>>16)&255,(p>>>24)&127].slice(0,n);
  if(bytes.slice(1).some(x=>x>127))return null;
  return bytes;
}
export const noteOn=p=>(((p>>>8)&240)===144&&((p>>>24)&127)>0);
const OUTPUT_PROTOCOL=0x00020001;
export class MidiRouter {
  constructor({send,navigator=globalThis.navigator,now=()=>performance.now(),changed=()=>{}}){
    this.send=send;this.navigator=navigator;this.now=now;this.changed=changed;
    this.access=null;this.output=null;this.route=0;this.epoch=0;this.active=false;
    this.transportEpoch=0;
    this.outputProtocol=0;
    this.playing=true;this.engine=false;this.owned=true;this.pending=0;
    this.drainWaiting=false;this.flight=null;this.lastBatch=null;this.request=0;
    this.rpcs=new Map();this.ledger=new Uint8Array(16*128);
    this.closing=false;this.switching=false;this.nextId=null;this.opening=false;
    this.openingOp=null;this.switchingOp=null;this.portClosing=null;this.openWaits=new Set();
    this.invalidation=0;
    this.status='MIDI is disconnected. Click Connect MIDI to request access.';
    this.accepted=0;this.cancelled=0;this.ackCount=0;this.rtt=0;this.maxRtt=0;
    this.clockAt=null;this.clockDeviation=0;this.bpm=120;
    this.buttonMask=0;this.attackHold=null;
  }
  report(text){if(text)this.status=text;this.changed(this);}
  get debt(){return this.ledger.reduce((sum,n)=>sum+n,0);}
  get locked(){return this.opening||this.switching||!!this.portClosing||this.rpcs.size>0;}
  ports(){return this.access?[...this.access.outputs.values()].filter(p=>p.state==='connected'):[];}
  connected(port){return port?.state==='connected'&&this.access?.outputs.get(port.id)===port;}
  operation(){return {port:this.output,route:this.route,invalidation:this.invalidation};}
  checkOperation(op){
    // A disconnect followed by reconnection must still invalidate an old await.
    // Object identity also prevents a replacement MIDIPort from inheriting it.
    if(op.invalidation!==this.invalidation||op.route!==this.route||op.port!==this.output||
      !this.connected(op.port)||!this.engine||this.playing)
      throw Error('Output operation interrupted. Reconnect the original output if needed, stop playback, then Retry cleanup.');
  }
  failOperation(op,text){
    if(op.port===this.output&&op.route===this.route)this.halt(text);
  }
  async openPort(op){
    let reject;
    const cancelled=new Promise((_,r)=>{reject=r;}),wait={reject};
    this.openWaits.add(wait);
    try{await Promise.race([op.port.open(),cancelled]);}
    finally{this.openWaits.delete(wait);}
  }
  engineReset(){
    // fm1-ready identifies a replacement producer, not a Stop in the same C
    // ring. Old requests/flights cannot complete on it. API-owned notes remain
    // assigned to the original output until an explicit cleanup retry.
    this.invalidation++;this.route=(this.route+1)>>>0;
    this.active=false;this.engine=false;this.playing=true;this.owned=true;
    this.epoch=0;this.transportEpoch=0;this.outputProtocol=0;this.pending=0;this.flight=null;this.lastBatch=null;this.drainWaiting=false;
    this.closing=false;this.nextId=null;this.clockAt=null;this.buttonMask=0;this.attackHold=null;
    if(this.openingOp){this.opening=false;this.openingOp=null;}
    this.switching=false;this.switchingOp=null;
    const error=Error('Engine replaced. Stop playback and Retry cleanup on the original output.');
    const requests=[...this.rpcs.values()];this.rpcs.clear();
    for(const wait of requests)wait.reject(error);
    for(const wait of this.openWaits)wait.reject(error);
    this.openWaits.clear();
    // A MIDIPort.close() already in progress cannot be cancelled. Its separate
    // lock prevents reopening that port until the actual close has settled.
    this.report(this.output?error.message:'Checking the replacement engine. MIDI output remains disconnected.');
  }
  async connect(){
    if(this.access){this.report('Choose an output explicitly. Reconnection does not activate it.');return;}
    if(!this.navigator?.requestMIDIAccess){this.report('WebMIDI is unavailable in this browser or context.');return;}
    if(this.opening)return;
    this.opening=true;this.report('Waiting for MIDI permission…');
    try{
      this.access=await this.navigator.requestMIDIAccess({sysex:false});
      this.access.addEventListener('statechange',()=>{
        if(this.output&&!this.connected(this.output))this.halt('Output disconnected. Cleanup remains assigned to '+this.output.name+'. Reconnect it, stop playback, then Retry cleanup.');
        else this.report('MIDI devices changed. Reconnection needs explicit output selection or Retry cleanup.');
      });
      this.report(this.ports().length?'Choose an output; no route is active.':'MIDI access granted, but no connected outputs are available.');
    }catch(error){this.report('MIDI permission/access failed: '+(error?.message||error));}
    finally{this.opening=false;this.report();}
  }
  rpc(type,fields={}){
    if(this.rpcs.size>=4)return Promise.reject(Error('MIDI control queue is busy.'));
    const request=++this.request;
    return new Promise((resolve,reject)=>{
      this.rpcs.set(request,{resolve,reject,route:this.route});
      this.send({type,protocol:OUTPUT_PROTOCOL,request,route:this.route,...fields});
      this.report();
    });
  }
  async activate(id){
    const output=this.access?.outputs.get(id);
    if(!this.connected(output))throw Error('The selected output is disconnected.');
    // Retain the candidate before open(), including a failed first activation,
    // so a statechange during that await requires an explicit original-port retry.
    this.output=output;this.route=(this.route+1)>>>0;
    const op=this.operation();
    this.opening=true;this.openingOp=op;this.report('Opening '+(output.name||'MIDI output')+'…');
    try{
      await this.openPort(op);
      this.checkOperation(op);
      await this.rpc('midi-route',{ready:true});
      this.checkOperation(op);
      this.active=true;this.closing=false;this.nextId=null;this.clockAt=null;
      this.report('Route active on '+(output.name||'MIDI output')+'. Physical receiver delivery is unverified.');
      this.pump();
    }catch(error){
      this.failOperation(op,'Route could not activate: '+error.message);
      throw error;
    }finally{if(this.openingOp===op){this.opening=false;this.openingOp=null;}this.report();}
  }
  async select(id){
    if(!this.engine||this.playing)throw Error('Switch on the engine and stop playback before selecting an output.');
    if(this.locked)throw Error('Wait for MIDI control acknowledgement.');
    if(!this.access)throw Error('Click Connect MIDI first.');
    if(this.output){
      if(!this.active)throw Error('Retry cleanup on the original output before selecting another.');
      const op=this.operation();
      this.closing=true;this.nextId=id||null;this.report('Cleaning up the original output before switching…');
      try{
        await this.rpc('midi-cancel');this.checkOperation(op);this.pump();this.maybeSwitch();
      }catch(error){this.failOperation(op,'Output switch stopped: '+error.message);throw error;}
      return;
    }
    if(id)await this.activate(id);
  }
  halt(text){
    this.invalidation++;
    this.active=false;this.clockAt=null;this.drainWaiting=false;
    // The old route remains selected, with both C and API-accepted ownership.
    if(this.output)this.send({type:'midi-route',protocol:OUTPUT_PROTOCOL,route:this.route,ready:false,request:0});
    this.report(text);
  }
  async retry(){
    if(!this.engine||this.playing)throw Error('Stop playback before retrying cleanup.');
    if(!this.output||this.locked)throw Error('No original output is available, or control is busy.');
    const port=this.access?.outputs.get(this.output.id);
    if(!this.connected(port))throw Error('Reconnect the original output first.');
    this.output=port;this.opening=true;
    const op=this.operation();
    this.openingOp=op;
    this.report('Opening the original output for cleanup…');
    try{
      await this.openPort(op);
      this.checkOperation(op);
      // Re-enable only this original route so the engine can enqueue owed Offs.
      await this.rpc('midi-route',{ready:true});
      this.checkOperation(op);
      this.active=true;this.closing=true;
      if(this.nextId===null)this.nextId=port.id;
      await this.rpc('midi-cancel');
      this.checkOperation(op);
      this.report('Retrying cleanup on the original output…');this.pump();
    }catch(error){this.failOperation(op,'Cleanup retry stopped: '+error.message);throw error;}
    finally{if(this.openingOp===op){this.opening=false;this.openingOp=null;}this.report();}
  }
  input(message){
    if(message.type!=='buttons'||!Number.isInteger(message.inputTicket)||message.inputTicket<1||message.inputTicket>0xffffffff)return;
    const mask=message.mask>>>0,rising=(mask&1024)&&!(this.buttonMask&1024);
    this.buttonMask=mask;
    if(rising&&this.engine&&this.playing)this.attackHold=message.inputTicket>>>0;
  }
  updateFrame(frame){
    const v=frame.variant?.pad,info=frame.variant?.info;
    this.engine=this.outputProtocol===OUTPUT_PROTOCOL&&!!(info?.values[0]&256)&&v?.abi===1&&v.section===8&&v.values.length===16;
    this.playing=!!frame.sync?.playing;this.bpm=frame.sync?.bpm||120;
    // C reports this only after scanning buttons and consuming transport requests
    // at a subsequent engine render. Even an overlay that consumes PLAY can ack.
    if(this.attackHold!==null&&Number.isInteger(frame.inputTicket)&&frame.inputTicket>=1&&frame.inputTicket<=0xffffffff&&
      (((frame.inputTicket>>>0)-this.attackHold)|0)>=0)this.attackHold=null;
    const bass=frame.variant?.bass??[];
    this.owned=!this.engine||v.values[14]!==0||v.values.slice(8,11).some(n=>n!==255)||
      bass.some(row=>row?.values[6]!==255);
    if(!this.engine&&this.active)this.halt('This engine does not support the MIDI route.');
    if(this.active){this.maybeSwitch();if(!this.locked)this.pump();}
    this.report();
  }
  pump(){
    if(!this.active||this.drainWaiting||this.flight||!this.output)return;
    this.drainWaiting=true;
    this.send({type:'midi-drain',protocol:OUTPUT_PROTOCOL,route:this.route});
  }
  track(bytes){
    const kind=bytes[0]&240,key=(bytes[0]&15)*128+(bytes[1]||0);
    if(kind===144&&bytes[2]>0)this.ledger[key]=1;
    if(kind===128||(kind===144&&bytes[2]===0))this.ledger[key]=0;
    if(bytes[0]===248){
      const now=this.now();if(this.clockAt!==null)this.clockDeviation=Math.max(this.clockDeviation,Math.abs(now-this.clockAt-60000/(24*this.bpm)));
      this.clockAt=now;
    }
  }
  acceptBatch(m){
    if(m.protocol!==OUTPUT_PROTOCOL||this.outputProtocol!==OUTPUT_PROTOCOL){this.active=false;this.engine=false;this.report('MIDI output protocol mismatch. Original-output cleanup remains owned; load matching engine/worklet/router before reconnecting.');return;}
    if(m.route!==this.route||!Array.isArray(m.packets)||m.packets.length>16||m.count!==m.packets.length||
      !m.packets.length||m.packets.some(p=>!Number.isInteger(p)||p<0||p>0xffffffff)||
      !Number.isInteger(m.batch)||m.batch<0||m.batch>0xffffffff||!Number.isInteger(m.epoch)||m.epoch<0||m.epoch>0xffffffff||
      this.flight||this.lastBatch===m.batch)return;
    this.drainWaiting=false;this.flight=m;this.lastBatch=m.batch;
    let prefix=0,failure=null;
    for(const packet of m.packets){
      // Candidate: stale transport/attack epochs cancel; Note Off stays owned.
      const realtime=[248,250,251,252].includes((packet>>>8)&255);
      if((packet>>>31)||(noteOn(packet)&&(m.epoch!==this.epoch||!this.active||this.closing||this.attackHold!==null))||
         (realtime&&(((m.epoch-this.transportEpoch)|0)<0||!this.active||
           (((packet>>>8)&255)!==252&&(this.closing||this.attackHold!==null))))){prefix++;this.cancelled++;continue;}
      if(!this.active||!this.output||this.output.state!=='connected')break;
      const bytes=midiBytes(packet);
      if(!bytes){failure='Invalid MIDI packet retained for inspection.';break;}
      try{this.output.send(bytes);this.track(bytes);this.accepted++;prefix++;}
      catch(error){failure='MIDI send failed; unsent suffix retained on '+this.output.name+': '+error.message;break;}
    }
    this.flight.ackAt=this.now();
    this.flight.ackPrefix=prefix;
    this.send({type:'midi-ack',protocol:OUTPUT_PROTOCOL,route:this.route,batch:m.batch,epoch:m.epoch,prefix});
    if(failure)this.halt(failure);
    this.report();
  }
  receive(m){
    // A replacement engine starts with route0. Its capability must be learned
    // before a fresh frame can authorize a new route; old state/ACK still uses
    // the retained route generation and cannot inherit cleanup ownership.
    if(m.type==='midi-output-capability'&&m.supported===true){
      this.outputProtocol=m.protocol===OUTPUT_PROTOCOL?OUTPUT_PROTOCOL:0;
      if(!this.outputProtocol){this.engine=false;this.active=false;}
      this.report();return;
    }
    if(m.type==='midi-route-reply'){
      const wait=this.rpcs.get(m.request);if(!wait)return;
      if(m.protocol!==OUTPUT_PROTOCOL){this.rpcs.delete(m.request);wait.reject(Error('MIDI output protocol mismatch.'));return;}
      if(!Number.isInteger(m.result)||(m.result===0&&m.route!==wait.route))return;
      this.rpcs.delete(m.request);m.result===0?wait.resolve(m):wait.reject(Error('Engine route rejected ('+m.result+'); stop playback and retain the original output.'));
      this.report();return;
    }
    if(m.type==='midi-output-capability'&&m.supported===false){this.outputProtocol=0;this.engine=false;this.active=false;this.report('This engine lacks the required MIDI transport cancellation protocol. Output remains inactive; original cleanup ownership is retained.');return;}
    if(m.route!==this.route)return;
    if(m.type==='midi-state'){
      if(m.protocol!==OUTPUT_PROTOCOL||!Number.isInteger(m.transportEpoch)||m.transportEpoch<0||m.transportEpoch>0xffffffff||!Number.isInteger(m.epoch)||m.epoch<0||m.epoch>0xffffffff||!Number.isInteger(m.pending)||m.pending<0||m.pending>64){this.outputProtocol=0;this.engine=false;this.active=false;this.report('MIDI output protocol mismatch; output remains inactive.');return;}
      this.outputProtocol=OUTPUT_PROTOCOL;
      this.epoch=m.epoch>>>0;this.pending=m.pending;
      this.transportEpoch=m.transportEpoch>>>0;
      if(!m.inflight&&!this.flight)this.drainWaiting=false;
      this.maybeSwitch();
      this.report();return;
    }
    if(m.type==='midi-batch'){this.acceptBatch(m);return;}
    if(m.type==='midi-acknowledged'&&m.protocol===OUTPUT_PROTOCOL&&this.flight?.batch===m.batch&&this.flight.route===m.route&&this.flight.epoch===m.epoch&&this.flight.ackPrefix===m.prefix){
      this.rtt=Math.max(0,this.now()-this.flight.ackAt);this.maxRtt=Math.max(this.maxRtt,this.rtt);this.ackCount++;
      this.flight=null;
      // Do not spin-retry a failed suffix. An active route can drain next prefix.
      if(this.active&&this.pending)this.pump();
      this.maybeSwitch();this.report();
    }
  }
  cleanupLocal(max=16){
    let sent=0;
    for(let key=0;key<this.ledger.length&&sent<max;key++){
      if(!this.ledger[key])continue;
      try{this.output.send([128+(key>>7),key&127,0]);this.ledger[key]=0;sent++;this.accepted++;}
      catch(error){this.halt('Cleanup send failed on the original output: '+error.message);return false;}
    }
    return this.debt===0;
  }
  async maybeSwitch(){
    if(!this.closing||!this.active||this.playing||this.owned||this.pending||this.flight||this.drainWaiting||this.locked)return;
    if(!this.cleanupLocal())return;
    this.switching=true;
    const next=this.nextId,old=this.output,op=this.operation();
    this.switchingOp=op;
    try{
      await this.rpc('midi-route',{ready:false});this.checkOperation(op);this.active=false;
      this.portClosing=op;
      try{await old.close();}finally{if(this.portClosing===op)this.portClosing=null;}
      this.checkOperation(op);this.output=null;this.closing=false;this.nextId=null;
      if(next)await this.activate(next);
      else this.report('MIDI route closed after API-accepted cleanup. Receiver delivery remains unverified.');
    }catch(error){this.failOperation(op,'Output switch stopped: '+error.message);}
    finally{if(this.switchingOp===op){this.switching=false;this.switchingOp=null;}this.report();}
  }
  pagehide(){
    // Best effort only: disappearing devices/pages cannot promise delivery.
    this.halt('Page closing: attempting original-output cleanup; physical delivery cannot be confirmed.');
    if(this.output?.state==='connected')this.cleanupLocal(2048);
  }
}

