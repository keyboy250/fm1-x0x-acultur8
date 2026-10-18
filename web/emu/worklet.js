// SPDX-License-Identifier: GPL-3.0-only
// X0X in an AudioWorklet: x0x.wasm is the whole FM-1 app (web/emu/x0x_web.c). Each render
// quantum asks it for 128 frames, which runs the device's clock forward; between quanta the page's
// input goes in and, about 30 times a second, the screen, the lights and any saved objects go out.
// The screen reader (fm1-reader's protocol): the page's reader SysEx goes in through web_sysex, and
// what the device says (whole F0..F7 frames from web_said_buf) goes out as soon as it is said.

import { ClockWorkletBridge } from './clock-worklet.mjs';
import { MidiWorkletBridge } from './midi-worklet.mjs';
import { CcInputWorkletBridge } from './cc-input-worklet.mjs';
import { SliceWorkletControl, validateMarked } from './slice-worklet.mjs';
const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();

class X0X extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.input = null;
    this.sentBlits = -1;
    this.sentWrites = -1;
    this.lastFrame = 0;
    this.busy = 0;
    this.frames = 0;
    this.variantStrip = 0;
    this.variantViewRevision = 0;
    this.laneView = {pattern:0,lane:0,page:0,viewRevision:0};
    // C has one external undo gesture. Browser panels must not join or finish
    // another panel's gesture; their request namespaces identify the caller.
    this.editOwner = null;
    this.rejectedGestures = new Set();
    this.loadGeneration = 0;
    this.legacyMaster = 2800; // older binaries lack remote MASTER/readback
    this.loading = false;
    this.midi = new MidiWorkletBridge(() => this.loading ? null : this.ex, m => this.port.postMessage(m));
    this.ccInput = new CcInputWorkletBridge(() => this.loading ? null : this.ex, m => this.port.postMessage(m));
    this.performanceClock=new ClockWorkletBridge(()=>this.loading?null:this.ex,m=>this.port.postMessage(m),()=>this.mem);
    this.sliceControl = new SliceWorkletControl(this,m=>this.port.postMessage(m));
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (!m || typeof m !== 'object') return;
    if (this.performanceClock.message(m)) return;
    if (this.midi.message(m)) return;
    if (this.ccInput.message(m)) return;
    if (this.sliceControl.message(m)) return;
    if (m.type === 'variant-command' || m.type === 'lane-command') {
      const keys = ['abi', 'op', 'request', 'target', 'voice', 'index', 'value'];
      const valid = keys.every(k => Number.isInteger(m[k])) && m.request >= 0 && m.request <= 0xffffffff &&
        m.value >= -0x80000000 && m.value <= 0x7fffffff && m.abi >= 0 && m.abi <= 65535 &&
        m.op >= 0 && m.op <= 65535 && ['target','voice','index'].every(k => m[k] >= 0 && m[k] <= 255);
      const reply = {type:m.type === 'lane-command' ? 'lane-reply' : 'variant-reply', request:m.request, result:valid ? -2 : -1, revision:0, ticket:0};
      if (valid) {
        const owner = m.type === 'lane-command' ? 'lane' : `variant-${m.request >>> 30}`;
        if (this.loading || this.rejectedGestures.has(owner) || (this.editOwner !== null && (this.editOwner !== owner || m.op === 7))) {
          reply.result = -3;
          // Legacy panels post BEGIN/value/END without waiting for BEGIN's
          // reply. Reject the whole refused gesture, even if its value arrives
          // after the original owner has ended. Foreign END never reaches C.
          // The lane panel waits for BEGIN ACK and sends no value/END after a
          // refusal. Only the four legacy variant panels need suffix quarantine.
          if (m.op === 3 && m.type === 'variant-command') this.rejectedGestures.add(owner);
          if (m.op === 4) this.rejectedGestures.delete(owner);
          // SAVE closes C's external gesture before storage can fail. Require
          // the owner to END first, keeping C and browser ownership consistent.
        } else if (this.ex?.web_variant_command) {
          const p = this.ex.web_variant_command(...keys.map(k => m[k]));
          const view = new DataView(this.mem.buffer, p, 16);
          Object.assign(reply, {request:view.getUint32(0,true), revision:view.getUint32(4,true),
            ticket:view.getUint32(8,true), result:view.getInt32(12,true)});
          if (reply.result >= 0) {
            if (m.op === 20) this.sentWrites = -1; // Replay bank for durable browser acknowledgement, including native no-op saves.
            if (m.op === 3) this.editOwner = owner;
            else if ([4,5,6].includes(m.op)) this.editOwner = null;
          }
        }
      }
      this.port.postMessage(reply);
      return;
    }
    if (m.type === 'lane-select') {
      if (Number.isInteger(m.pattern) && m.pattern >= 0 && m.pattern < 32 &&
          Number.isInteger(m.lane) && m.lane >= 0 && m.lane < 25 &&
          Number.isInteger(m.page) && m.page >= 0 && m.page < 2 &&
          Number.isInteger(m.viewRevision) && m.viewRevision >= 0 && m.viewRevision <= 0xffffffff) {
        this.laneView = {pattern:m.pattern,lane:m.lane,page:m.page,viewRevision:m.viewRevision};
      }
      return;
    }
    if (m.type === 'variant-select') {
      if (Number.isInteger(m.strip) && m.strip >= 0 && m.strip < 25 &&
          (m.viewRevision === undefined || (Number.isInteger(m.viewRevision) && m.viewRevision >= 0 && m.viewRevision <= 0xffffffff))) {
        this.variantStrip = m.strip;
        this.variantViewRevision = m.viewRevision ?? 0;
      }
      return;
    }
    if (m.type === "load") {
      // Keep the original MIDI producer alive until its selected route and
      // every posted/latched/queued prefix have completed cleanup.
      if(this.midi.ready||this.midi.flight||this.midi.latched||
         (this.ex?.web_midi_pending&&this.ex.web_midi_pending()))
        throw new Error('Disconnect the MIDI output and complete cleanup before replacing the engine.');
      const generation = ++this.loadGeneration;
      const requestedMaster=this.ex?.web_master_state ? this.ex.web_master_state()&8191 : this.ex ? this.legacyMaster : m.master ?? 2800;
      const masterQ12=Number.isInteger(requestedMaster)&&requestedMaster>=0&&requestedMaster<=4096?requestedMaster:2800;
      // Reset while the old exports are still visible. Already applied edits
      // remain; only unreviewed source-2 ingress and its selectors are cleared.
      this.ccInput.reset();
      this.port.postMessage({type:'cc-input-reset'});
      this.loading = true;
      let memory;
      const now = () => BigInt(Math.round(currentTime * 1e9));
      const wasi = {
        clock_time_get: (id, prec, out) => {
          new DataView(memory.buffer).setBigUint64(out, now(), true);
          return 0;
        },
        fd_write: (fd, iov, n, out) => { new DataView(memory.buffer).setUint32(out, 0, true); return 0; },
        proc_exit: () => {},
      };
      const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
      const env = new Proxy({}, { get: () => () => 0 });
      try {
        const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
        if (generation !== this.loadGeneration) return;
        const ex = instance.exports;
        memory = ex.memory;
        if (ex._initialize) ex._initialize();
        const max = ex.web_store_max();
        for (const [obj, bytes] of Object.entries(m.store || {})) {
          const b = new Uint8Array(bytes).subarray(0, max);
          new Uint8Array(memory.buffer, ex.web_store(+obj), b.length).set(b);
          ex.web_store_set_len(+obj, b.length);
        }
        const writes = ex.web_store_writes();
        ex.web_master(masterQ12);
        this.legacyMaster=masterQ12;
        ex.web_boot();
        this.ex = ex;this.sliceControl.reset();
        this.midi=new MidiWorkletBridge(()=>this.loading?null:this.ex,m=>this.port.postMessage(m));
        this.mem = memory;
        this.sentWrites = writes;
        this.editOwner = null;
        // Refused legacy BEGIN suffixes belong to their original callers even
        // across replacement. Only their refused END drains the quarantine.
        this.loading = false;
        this.ccInput.reset();
        this.port.postMessage({ type: "ready" });
      } catch (error) {
        if (generation !== this.loadGeneration) return;
        // Failed replacement keeps the old engine and its gesture ownership.
        this.loading = false;
        this.ccInput.reset();
        throw error;
      }
    } else if (!this.ex) {
      return;
    } else if (m.type === "buttons") {
      if(this.ex.web_buttons_tagged&&Number.isInteger(m.inputTicket)&&m.inputTicket>=1&&m.inputTicket<=0xffffffff)
        this.ex.web_buttons_tagged(m.mask>>>0,m.inputTicket>>>0);
      else this.ex.web_buttons(m.mask >>> 0);
    } else if (m.type === "keys") {
      this.ex.web_keys(m.mask >>> 0);
    } else if (m.type === "enc") {
      this.ex.web_enc(m.role, m.n | 0);
    } else if (m.type === "master") {
      if(!this.loading&&Number.isInteger(m.value)&&m.value>=0&&m.value<=4096){this.ex.web_master(m.value);this.legacyMaster=m.value;}
    } else if(m.type === "master-step") {
      if(!this.loading&&Number.isInteger(m.delta)&&m.delta>=-4096&&m.delta<=4096){
        if(this.ex.web_master_step)this.ex.web_master_step(m.delta);
        else {this.legacyMaster=Math.max(0,Math.min(4096,this.legacyMaster+m.delta));this.ex.web_master(this.legacyMaster);}
      }
    } else if (m.type === "marked-load") {
      try {
        const ex=this.ex;
        const prepared=validateMarked(m);
        ex.web_sync_off();ex.web_marked_stop();this.markedOn=false;this.syncOn=false;
        new Float32Array(this.mem.buffer,ex.web_marked_pcm(),m.pcm.length).set(m.pcm);
        const meta=new Uint32Array(this.mem.buffer,ex.web_marked_meta(),m.slices.length*3);
        m.slices.forEach((s,i)=>meta.set([s.start,s.end,s.time],i*3));
        const rc=ex.web_marked_load(m.frames,m.rate,m.channels,m.slices.length,m.length);
        if(rc)throw Error('Firmware marker validation failed: '+rc);
        this.sliceControl.loaded=prepared;this.sliceControl.bpm=m.sourceBpm??null;this.sliceControl.revision++;
        this.port.postMessage({type:'marked-ready'});
      }catch(err){this.port.postMessage({type:'marked-error',message:err.message});}
    } else if (m.type === "marked-sync") {
      const rc=this.ex.web_sync_arm(Number(m.bpm));
      if(rc)this.port.postMessage({type:'marked-error',message:'Enter the original loop BPM (20–300). Sync supports the internal sequencer clock.'});
      else {this.syncOn=true;this.markedOn=false;this.markedLayer=true;this.sliceControl.bpm=Number(m.bpm);}
    } else if (m.type === "seq-tempo") {
      this.ex.web_seq_set_bpm(Number(m.bpm));
    } else if (m.type === "marked-play") {
      this.ex.web_sync_off();this.syncOn=false;this.ex.web_marked_play(m.repeat?1:0);this.sliceControl.repeat=!!m.repeat;this.markedOn=true;
    } else if (m.type === "marked-pad") {
      this.ex.web_sync_off();this.syncOn=false;this.ex.web_marked_pad(m.slice);this.markedOn=true;
    } else if (m.type === "marked-stop") {
      this.ex.web_sync_off();this.ex.web_marked_stop();this.markedOn=false;this.syncOn=false;
    } else if (m.type === "marked-level") {
      this.markedGain=Math.max(0,Math.min(1,m.gain));this.markedLayer=!!m.layer;
    } else if (m.type === "sysex") {
               // a reader's SysEx, the bytes between F0 and F7
      if(!this.ex.web_sysex_in || !this.ex.web_sysex || !Array.isArray(m.bytes) || m.bytes.length>64 || !m.bytes.every(v=>Number.isInteger(v)&&v>=0&&v<128))return;
      const b = m.bytes.slice();
      new Uint8Array(this.mem.buffer, this.ex.web_sysex_in(), b.length).set(b);
      this.ex.web_sysex(b.length);
    } else if (m.type === "slot") {                // a user break slot: {k, name, zones: [{pcm, rate}]}
      const ex = this.ex, name = new Uint8Array(this.mem.buffer, ex.web_slot_namebuf(), 9);
      name.fill(0);
      const nm = (m.name || "").slice(0, 8);         // (no TextEncoder in an AudioWorklet)
      for (let i = 0; i < nm.length; i++) name[i] = nm.charCodeAt(i) & 0x7F;
      ex.web_slot_clear(m.k);
      let used = 0;
      const max = ex.web_slot_max();
      (m.zones || []).forEach((zn, z) => {
        const n = Math.min(zn.pcm.length, max - used);
        if (n <= 0) return;
        new Int16Array(this.mem.buffer, ex.web_slot_pcm(), n).set(zn.pcm.subarray(0, n));
        if (ex.web_slot_zone(m.k, z, n, zn.rate) === 0) used += n;
      });
      ex.web_slots_changed();
    }
  }

  publish() {
    const ex = this.ex;
    const blits = ex.web_blits();
    const msg = { type: "frame", buttons: ex.web_lit_buttons() >>> 0, keys: ex.web_lit_keys() >>> 0, dim: ex.web_dim_keys() >>> 0 };
    if(ex.web_input_ticket)msg.inputTicket=ex.web_input_ticket()>>>0;
    if(ex.web_master_state){const s=ex.web_master_state()>>>0;msg.master={q12:s&8191,owner:(s>>>13)&7,pickupPending:!!(s&65536)};}
    else if(ex.web_master)msg.master={q12:this.legacyMaster,owner:1,pickupPending:false};
    if (ex.web_variant_snapshot) {
      const read = (section,target,voice=0) => {
        const p = ex.web_variant_snapshot(section,target,voice);
        if (!p) return null;
        const v = new Uint32Array(this.mem.buffer,p,20);
        return {abi:v[0],section:v[1],revision:v[2],values:Array.from(v.subarray(4,4+Math.min(v[3],16)))};
      };
      const arrangement = read(5,0);
      msg.variant = {info:read(0,0),strip:this.variantStrip ?? 0,viewRevision:this.variantViewRevision,settings:read(2,this.variantStrip ?? 0),meters:read(3,this.variantStrip ?? 0),
        arrangement,block:arrangement ? read(4,arrangement.values[2]) : null,chord:read(6,0),bass:[read(7,0),read(7,1)],pad:read(8,0)};
      if (msg.variant.info?.values[0] & 1024) {
        const view = this.laneView;
        msg.variant.laneView = {...view,mode:read(11,0),settings:read(10,view.pattern,view.lane),
          breaks:view.lane === 24 ? read(12,view.pattern,view.page) : null};
      }
    }
    msg.sync={state:ex.web_sync_state(),bpm:ex.web_seq_bpm(),playing:!!ex.web_seq_playing(),beat:ex.web_seq_beat()};
    if (this.ccInput.supported() && typeof ex.web_cc_pending === 'function' && typeof ex.web_cc_stat === 'function') {
      msg.ccInput = {version:ex.web_cc_version() >>> 0,route:this.ccInput.route,ready:this.ccInput.ready,
        pending:ex.web_cc_pending() >>> 0,source:typeof ex.web_cc_source==='function'?Array.from({length:32},(_,i)=>ex.web_cc_source(2,i)>>>0):null,slice:this.sliceControl.state(),stats:Array.from({length:6},(_,i)=>ex.web_cc_stat(i) >>> 0)};
    }
    if(ex.web_marked_cursor)msg.marked={cursor:ex.web_marked_cursor(),triggers:ex.web_marked_triggers(),slice:ex.web_marked_last(),playing:!!ex.web_marked_playing()||(ex.web_sync_state()===2&&!!ex.web_seq_playing())};
    if (this.frames >= 44100) {                       // the share of real time spent running the device
      msg.load = Math.round(100 * this.busy / (this.frames / 44.1));
      this.busy = this.frames = 0;
    }
    const transfer = [];
    if (blits !== this.sentBlits) {
      this.sentBlits = blits;
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
      transfer.push(msg.fb.buffer);
    }
    const writes = ex.web_store_writes();
    if (writes !== this.sentWrites) {
      this.sentWrites = writes;
      msg.store = {};
      for (let o = 0; o < ex.web_nobj(); o++) {
        const n = ex.web_store_len(o);
        if (n >= 0) msg.store[o] = new Uint8Array(this.mem.buffer, ex.web_store(o), n).slice();
      }
    }
    this.port.postMessage(msg, transfer);
  }

  process(inputs, outputs) {
    if (!this.ex) return true;
    const out = outputs[0];
    const n = out[0].length;
    const t0 = clock();
    this.ex.web_render(n);
    this.midi.tick();
    this.busy += clock() - t0;
    this.frames += n;
    const said = this.ex.web_said_len ? this.ex.web_said_len() : 0;
    if (said) {                                       // what the device said for the reader
      const bytes = new Uint8Array(this.mem.buffer, this.ex.web_said_buf(), said).slice();
      this.ex.web_said_clear();
      this.port.postMessage({ type: "said", bytes }, [bytes.buffer]);
    }
    out[0].set(new Float32Array(this.mem.buffer, this.ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
    if(this.markedOn||this.syncOn){
      if(this.markedOn)this.ex.web_marked_render(n);
      const l=new Float32Array(this.mem.buffer,this.syncOn?this.ex.web_sync_l():this.ex.web_marked_l(),n),r=new Float32Array(this.mem.buffer,this.syncOn?this.ex.web_sync_r():this.ex.web_marked_r(),n),g=this.markedGain??0.5;
      for(let i=0;i<n;i++){out[0][i]=Math.max(-1,Math.min(1,(this.markedLayer?out[0][i]:0)+l[i]*g));if(out[1])out[1][i]=Math.max(-1,Math.min(1,(this.markedLayer?out[1][i]:0)+r[i]*g));}
    }
    if (currentTime - this.lastFrame >= 1 / 30) {
      this.lastFrame = currentTime;
      this.publish();
    }
    return true;
  }
}

registerProcessor("x0x", X0X);
