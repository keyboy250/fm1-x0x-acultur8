// SPDX-License-Identifier: GPL-3.0-only
// One latched prefix and one in-flight message. Only explicit accepted/cancelled
// prefix acknowledgements consume C packets; output devices are never visible here.
export class MidiWorkletBridge {
  constructor(exports, post) {
    this.exports=exports; this.post=post; this.route=0; this.ready=false;
    this.latched=0; this.flight=null; this.serial=0; this.epoch=null;this.paused=false;
  }
  get ex(){return this.exports();}
  supported(){return ['web_midi_batch','web_midi_packet','web_midi_ack','web_midi_pending','web_midi_epoch','web_midi_cancel','web_midi_ready','web_seq_playing'].every(k=>typeof this.ex?.[k]==='function');}
  state(){
    if(!this.supported())return;
    this.epoch=this.ex.web_midi_epoch()>>>0;
    this.post({type:'midi-state',route:this.route,epoch:this.epoch,
      pending:this.ex.web_midi_pending(),ready:this.ready,inflight:!!this.flight});
  }
  tick(){
    if(this.supported()&&(this.ex.web_midi_epoch()>>>0)!==this.epoch)this.state();
    if(this.supported()&&this.ready&&!this.paused&&!this.flight&&this.ex.web_midi_pending())this.drain(false);
  }
  drain(resume=true){
    if(!this.supported()||this.flight)return;
    if(resume)this.paused=false;
    if((this.ex.web_midi_epoch()>>>0)!==this.epoch)this.state();
    if(!this.latched){const n=this.ex.web_midi_batch(16);if(n<0){this.post({type:'midi-bridge-error',message:'MIDI prefix already latched.'});return;}this.latched=n;}
    if(!this.latched){this.state();return;}
    const packets=Array.from({length:this.latched},(_,i)=>this.ex.web_midi_packet(i)>>>0);
    this.serial=(this.serial+1)>>>0;
    this.flight={batch:this.serial,epoch:this.epoch,count:packets.length};
    this.post({type:'midi-batch',route:this.route,...this.flight,packets,
      postedAt:typeof performance==='object'?performance.now():null,
      audioTime:typeof currentTime==='number'?currentTime:null});
  }
  message(m){
    if(!['midi-route','midi-cancel','midi-drain','midi-ack'].includes(m.type))return false;
    const valid=Number.isInteger(m.route)&&m.route>=0&&m.route<=0xffffffff;
    const result=(value)=>this.post({type:'midi-route-reply',request:m.request,route:this.route,result:value,ready:this.ready});
    if(!this.supported()){if(m.type!=='midi-ack'&&m.type!=='midi-drain')result(-2);return true;}
    if(!valid){result(-1);return true;}
    if(m.type==='midi-route'){
      if(typeof m.ready!=='boolean'){result(-1);return true;}
      if(m.route!==this.route){
        if(!m.ready||this.ready||this.flight||this.latched||this.ex.web_midi_pending()){result(-3);return true;}
        if(((m.route-this.route)|0)<=0){result(-1);return true;}
      }
      if(m.ready&&this.ex.web_seq_playing()){result(-3);return true;}
      const rc=this.ex.web_midi_ready(m.ready?1:0);
      if(!rc){this.route=m.route;this.ready=m.ready;}
      this.state();result(rc);return true;
    }
    if(m.route!==this.route)return true;
    if(m.type==='midi-cancel'){this.ex.web_midi_cancel();this.state();result(0);return true;}
    if(m.type==='midi-drain'){this.drain();return true;}
    if(!this.flight||m.batch!==this.flight.batch||m.epoch!==this.flight.epoch||
      !Number.isInteger(m.prefix)||m.prefix<0||m.prefix>this.flight.count)return true;
    if(this.ex.web_midi_ack(m.prefix)!==0)return true;
    this.latched-=m.prefix;this.paused=this.latched>0;this.flight=null;this.state();
    this.post({type:'midi-acknowledged',route:this.route,batch:m.batch,prefix:m.prefix});
    return true;
  }
}

