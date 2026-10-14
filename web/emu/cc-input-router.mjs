// SPDX-License-Identifier: GPL-3.0-only
// Explicit CC-only input. The worklet receipts acknowledge ingress, never
// canonical application or a physical timing guarantee. No MIDI output calls.
import {ccPacket} from './cc-input-worklet.mjs';
const VERSION=0x00010001,MAX_QUEUE=64,MAX_FLIGHT=16;
const u32=n=>Number.isInteger(n)&&n>=0&&n<=0xffffffff;
export class CcInputRouter {
  constructor({send,change=()=>{},requestAccess=()=>globalThis.navigator?.requestMIDIAccess?.({sysex:false}),
    schedule=(fn,ms)=>setTimeout(fn,ms),cancel=id=>clearTimeout(id)}={}) {
    this.send=send;this.change=change;this.requestAccess=requestAccess;this.schedule=schedule;this.cancel=cancel;
    this.access=null;this.input=null;this.selected='';this.active=false;this.supported=false;this.fresh=false;
    this.route=0;this.serial=0;this.generation=0;this.queue=[];this.flight=null;this.control=null;this.operation=null;
    this.handler=null;this.retryTimer=null;this.zeroProgress=0;this.closing=new Map();this.connecting=false;
    this.received=0;this.accepted=0;this.ignored=0;this.discarded=0;this.pending=null;this.stats=[];
    this.status='No MIDI permission requested. CC input is inactive.';
  }
  changed(){this.change(this);}
  ports(){return this.access?[...this.access.inputs.values()]:[];}
  current(port){return !!port&&port.state==='connected'&&this.access?.inputs.get(port.id)===port;}
  request(){this.serial=(this.serial+1)>>>0;if(!this.serial)this.serial=1;return this.serial;}
  nextRoute(){this.route=(this.route+1)>>>0;return this.route;}
  async connect(){
    if(this.connecting)return;this.connecting=true;this.changed();
    try{
      const access=await this.requestAccess();
      if(!access?.inputs||typeof access.addEventListener!=='function')throw Error('Web MIDI is unavailable in this browser.');
      if(this.access!==access){this.access?.removeEventListener('statechange',this.stateHandler);this.access=access;
        this.stateHandler=()=>{if(this.input&&!this.current(this.input)&&(this.active||this.operation))this.pause('Selected MIDI input disconnected. Reconnect it and explicitly retry.');this.changed();};
        access.addEventListener('statechange',this.stateHandler);
        if(this.input&&!this.current(this.input)&&(this.active||this.operation))this.pause('MIDI access changed. Select an input and explicitly retry.');
      }
      this.status=this.ports().length?'Choose an input, then enable CC automation.':'MIDI permission granted, but no input ports are available.';
    }catch(error){this.status=`MIDI connection unavailable: ${error.message||error}`;}
    finally{this.connecting=false;this.changed();}
  }
  detach(){if(this.input&&this.handler)this.input.removeEventListener('midimessage',this.handler);this.handler=null;}
  clearTraffic(){
    this.discarded+=this.queue.length;this.queue=[];
    if(this.flight)this.cancel(this.flight.timer);this.flight=null;
    if(this.retryTimer!==null)this.cancel(this.retryTimer);this.retryTimer=null;this.zeroProgress=0;
  }
  invalidate(reason){
    this.generation++;this.active=false;this.detach();this.clearTraffic();
    if(this.control){const c=this.control;this.control=null;this.cancel(c.timer);c.reject(Error(reason));}
    if(this.operation){const op=this.operation;this.operation=null;op.reject(Error(reason));}
  }
  routeRequest(ready){
    if(this.control){const c=this.control;this.control=null;this.cancel(c.timer);c.reject(Error('CC route request superseded.'));}
    const request=this.request(),route=this.route;
    return new Promise((resolve,reject)=>{
      const timer=this.schedule(()=>{if(this.control?.request!==request)return;this.control=null;reject(Error('CC route acknowledgement timed out.'));},1000);
      this.control={request,route,ready,resolve,reject,timer};
      try{this.send({type:'cc-input-route',request,route,ready});}catch(error){this.cancel(timer);this.control=null;reject(error);}
    });
  }
  resetSource(){
    const generation=this.generation;this.nextRoute();
    // Failures keep input paused; a later explicit activation uses a newer
    // generation and resets the source again. No discarded packet is replayed.
    this.routeRequest(false).catch(error=>{if(generation===this.generation){this.status+=` Source reset unconfirmed: ${error.message}`;this.changed();}});
  }
  pause(reason){
    this.invalidate(reason);this.status=`${reason} Input paused; queued input discarded. Already accepted settings remain. Use explicit retry.`;
    this.resetSource();this.changed();
  }
  closePort(port){
    if(!port)return Promise.resolve();if(this.closing.has(port))return this.closing.get(port);
    const closing=Promise.resolve().then(()=>port.close()).catch(()=>{}).finally(()=>{if(this.closing.get(port)===closing)this.closing.delete(port);this.changed();});
    this.closing.set(port,closing);return closing;
  }
  async activate(id){
    if(this.operation||!this.supported||!this.fresh){this.status='Wait for a fresh supported engine frame before enabling CC input.';this.changed();return false;}
    const port=this.access?.inputs.get(id);
    if(!this.current(port)){this.status='Choose a connected MIDI input.';this.changed();return false;}
    const previous=this.input;this.invalidate('New CC input route selected.');this.selected=id;
    const generation=this.generation;let reject;const cancelled=new Promise((_,no)=>reject=no);
    const op={generation,port,reject};this.operation=op;this.input=port;
    const valid=()=>this.operation===op&&this.generation===generation&&this.input===port&&this.current(port)&&this.supported&&this.fresh;
    const check=()=>{if(!valid())throw Error('CC input activation was interrupted.');};
    try{
      this.status='Opening the selected CC input…';this.changed();
      // Reset the old source before enabling another route, even when its
      // acknowledgement was lost. New-route ACK is required before listening.
      this.nextRoute();await Promise.race([this.routeRequest(false),cancelled]);check();
      if(previous&&previous!==port)await Promise.race([this.closePort(previous),cancelled]);check();
      if(this.closing.has(port))await Promise.race([this.closing.get(port),cancelled]);check();
      const opened=Promise.resolve().then(()=>port.open());
      opened.then(()=>{if(!valid()&&this.input!==port)this.closePort(port);},()=>{});
      await Promise.race([opened,cancelled]);check();
      this.nextRoute();await Promise.race([this.routeRequest(true),cancelled]);check();
      const route=this.route;
      this.handler=event=>{if(!this.active||generation!==this.generation||route!==this.route||this.input!==port||!this.current(port))return;this.receive(event.data);};
      port.addEventListener('midimessage',this.handler);this.active=true;
      this.status='CC input enabled. Send automation after this acknowledgement. Notes and channel-mode messages are ignored.';
      return true;
    }catch(error){
      if(this.operation===op){this.invalidate(error.message);this.status=`CC input activation failed: ${error.message} Already accepted settings remain. Retry explicitly.`;this.resetSource();}
      return false;
    }finally{if(this.operation===op)this.operation=null;this.changed();}
  }
  retry(){return this.activate(this.selected);}
  async disconnect(){
    const port=this.input;this.invalidate('CC input disconnected.');this.input=null;this.status='CC input disabled. Already accepted settings remain.';this.resetSource();this.changed();await this.closePort(port);
  }
  engineReset(){
    this.invalidate('Engine restarted.');this.supported=false;this.fresh=false;this.pending=null;this.stats=[];
    this.nextRoute();this.status='Engine restarted. Await a fresh supported frame, then explicitly enable CC input.';this.changed();
  }
  engineState(state){
    this.fresh=true;this.supported=state?.version===VERSION;
    this.pending=u32(state?.pending)?state.pending:null;this.stats=Array.isArray(state?.stats)&&state.stats.length===6&&Array.from(state.stats).every(u32)?state.stats.slice():[];
    if(!this.supported&&(this.active||this.operation))this.pause('This engine does not support the CC input protocol.');
    else if(this.active&&state.route===this.route&&state.ready===false)this.pause('The engine disabled this CC input route.');
    this.changed();
  }
  receive(bytes){
    if(!this.active)return;
    const packet=ccPacket(bytes);if(packet===null){this.ignored++;this.changed();return;}
    this.received++;
    if(this.queue.length>=MAX_QUEUE){this.discarded++;this.pause('CC input overflow (64 queued messages).');return;}
    this.queue.push(packet);this.drain();this.changed();
  }
  drain(){
    if(!this.active||this.flight||this.retryTimer!==null||!this.queue.length)return;
    const request=this.request(),route=this.route,packets=this.queue.slice(0,MAX_FLIGHT);
    const timer=this.schedule(()=>{if(this.flight?.request===request)this.pause('CC receipt timed out; acceptance of that batch is unknown.');},1000);
    this.flight={request,route,count:packets.length,timer};
    try{this.send({type:'cc-input-batch',request,route,packets});}catch(error){this.pause(`CC message send failed: ${error.message}`);}
  }
  message(m){
    if(m?.type!=='cc-input-reply'||!u32(m.request)||!u32(m.route)||!Number.isInteger(m.result)||m.result< -3||m.result>0||typeof m.ready!=='boolean'||!u32(m.prefix))return false;
    const control=this.control;
    if(control?.request===m.request){
      if(m.route!==control.route||m.prefix!==0)return false;
      this.control=null;this.cancel(control.timer);
      if(m.result===0&&m.ready===control.ready)control.resolve();else control.reject(Error(`Engine refused CC route (${m.result}).`));
      return true;
    }
    const flight=this.flight;
    if(!flight||m.request!==flight.request||m.route!==flight.route||m.route!==this.route)return false;
    if(m.prefix>flight.count||m.result!==0||!m.ready){this.pause('Invalid or rejected CC receipt; acceptance is uncertain.');return true;}
    this.cancel(flight.timer);this.flight=null;
    this.queue.splice(0,m.prefix);this.accepted+=m.prefix;
    this.zeroProgress=m.prefix?0:this.zeroProgress+1;
    if(this.zeroProgress>=32){this.pause('CC engine ingress remained full.');return true;}
    if(this.queue.length){const generation=this.generation;this.retryTimer=this.schedule(()=>{this.retryTimer=null;if(generation===this.generation)this.drain();},20);}
    this.changed();return true;
  }
  pagehide(){this.invalidate('Page closing.');this.nextRoute();try{this.send({type:'cc-input-route',request:this.request(),route:this.route,ready:false});}catch{}this.closePort(this.input);}
}
