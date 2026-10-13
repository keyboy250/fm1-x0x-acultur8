// SPDX-License-Identifier: GPL-3.0-only
// X0X in an AudioWorklet: x0x.wasm is the whole FM-1 app (web/emu/x0x_web.c). Each render
// quantum asks it for 128 frames, which runs the device's clock forward; between quanta the page's
// input goes in and, about 30 times a second, the screen, the lights and any saved objects go out.

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
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (!m || typeof m !== 'object') return;
    if (m.type === 'variant-command') {
      const keys = ['abi', 'op', 'request', 'target', 'voice', 'index', 'value'];
      const valid = keys.every(k => Number.isInteger(m[k])) && m.request >= 0 && m.request <= 0xffffffff &&
        m.value >= -0x80000000 && m.value <= 0x7fffffff && m.abi >= 0 && m.abi <= 65535 &&
        m.op >= 0 && m.op <= 65535 && ['target','voice','index'].every(k => m[k] >= 0 && m[k] <= 255);
      const reply = {type:'variant-reply', request:m.request, result:valid ? -2 : -1, revision:0, ticket:0};
      if (valid && this.ex?.web_variant_command) {
        const p = this.ex.web_variant_command(...keys.map(k => m[k]));
        const view = new DataView(this.mem.buffer, p, 16);
        Object.assign(reply, {request:view.getUint32(0,true), revision:view.getUint32(4,true),
          ticket:view.getUint32(8,true), result:view.getInt32(12,true)});
      }
      this.port.postMessage(reply);
      return;
    }
    if (m.type === 'variant-select') {
      if (Number.isInteger(m.strip) && m.strip >= 0 && m.strip < 25) this.variantStrip = m.strip;
      return;
    }
    if (m.type === "load") {
      const now = () => BigInt(Math.round(currentTime * 1e9));
      const wasi = {
        clock_time_get: (id, prec, out) => {
          new DataView(this.mem.buffer).setBigUint64(out, now(), true);
          return 0;
        },
        fd_write: (fd, iov, n, out) => { new DataView(this.mem.buffer).setUint32(out, 0, true); return 0; },
        proc_exit: () => {},
      };
      const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
      const env = new Proxy({}, { get: () => () => 0 });
      const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
      this.ex = instance.exports;
      this.mem = this.ex.memory;
      if (this.ex._initialize) this.ex._initialize();
      const max = this.ex.web_store_max();
      for (const [obj, bytes] of Object.entries(m.store || {})) {
        const b = new Uint8Array(bytes).subarray(0, max);
        new Uint8Array(this.mem.buffer, this.ex.web_store(+obj), b.length).set(b);
        this.ex.web_store_set_len(+obj, b.length);
      }
      this.sentWrites = this.ex.web_store_writes();
      this.ex.web_master(m.master ?? 2800);
      this.ex.web_boot();
      this.port.postMessage({ type: "ready" });
    } else if (!this.ex) {
      return;
    } else if (m.type === "buttons") {
      this.ex.web_buttons(m.mask >>> 0);
    } else if (m.type === "keys") {
      this.ex.web_keys(m.mask >>> 0);
    } else if (m.type === "enc") {
      this.ex.web_enc(m.role, m.n | 0);
    } else if (m.type === "master") {
      this.ex.web_master(m.value | 0);
    } else if (m.type === "marked-load") {
      try {
        const ex=this.ex;
        if(m.frames<1||m.frames>1440000||m.channels<1||m.channels>2||m.pcm.length!==m.frames*m.channels||m.slices.length<1||m.slices.length>64)throw Error('Invalid slice audio.');
        ex.web_sync_off();ex.web_marked_stop();this.markedOn=false;this.syncOn=false;
        new Float32Array(this.mem.buffer,ex.web_marked_pcm(),m.pcm.length).set(m.pcm);
        const meta=new Uint32Array(this.mem.buffer,ex.web_marked_meta(),m.slices.length*3);
        m.slices.forEach((s,i)=>meta.set([s.start,s.end,s.time],i*3));
        const rc=ex.web_marked_load(m.frames,m.rate,m.channels,m.slices.length,m.length);
        if(rc)throw Error('Firmware marker validation failed: '+rc);
        this.port.postMessage({type:'marked-ready'});
      }catch(err){this.port.postMessage({type:'marked-error',message:err.message});}
    } else if (m.type === "marked-sync") {
      const rc=this.ex.web_sync_arm(Number(m.bpm));
      if(rc)this.port.postMessage({type:'marked-error',message:'Enter the original loop BPM (20–300). Sync supports the internal sequencer clock.'});
      else {this.syncOn=true;this.markedOn=false;this.markedLayer=true;}
    } else if (m.type === "seq-tempo") {
      this.ex.web_seq_set_bpm(Number(m.bpm));
    } else if (m.type === "marked-play") {
      this.ex.web_sync_off();this.syncOn=false;this.ex.web_marked_play(m.repeat?1:0);this.markedOn=true;
    } else if (m.type === "marked-pad") {
      this.ex.web_sync_off();this.syncOn=false;this.ex.web_marked_pad(m.slice);this.markedOn=true;
    } else if (m.type === "marked-stop") {
      this.ex.web_sync_off();this.ex.web_marked_stop();this.markedOn=false;this.syncOn=false;
    } else if (m.type === "marked-level") {
      this.markedGain=Math.max(0,Math.min(1,m.gain));this.markedLayer=!!m.layer;
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
    if (ex.web_variant_snapshot) {
      const read = (section,target) => {
        const p = ex.web_variant_snapshot(section,target,0);
        if (!p) return null;
        const v = new Uint32Array(this.mem.buffer,p,20);
        return {abi:v[0],section:v[1],revision:v[2],values:Array.from(v.subarray(4,4+Math.min(v[3],16)))};
      };
      msg.variant = {info:read(0,0),strip:this.variantStrip ?? 0,settings:read(2,this.variantStrip ?? 0),meters:read(3,this.variantStrip ?? 0)};
    }
    msg.sync={state:ex.web_sync_state(),bpm:ex.web_seq_bpm(),playing:!!ex.web_seq_playing(),beat:ex.web_seq_beat()};
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
    this.busy += clock() - t0;
    this.frames += n;
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
