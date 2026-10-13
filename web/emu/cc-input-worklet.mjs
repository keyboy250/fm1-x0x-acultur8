// SPDX-License-Identifier: GPL-3.0-only
// CC automation input only. Receipts acknowledge bounded C ingress, not a
// parameter application or physical MIDI deadline. Never echo to MIDI output.
export function ccPacket(bytes) {
  if (!(Array.isArray(bytes) || ArrayBuffer.isView(bytes)) || bytes.length !== 3) return null;
  const status=bytes[0],control=bytes[1],value=bytes[2];
  if (![status,control,value].every(Number.isInteger)) return null;
  if (status < 0xb0 || status > 0xbf || control < 0 || control >= 120 || value < 0 || value > 127) return null;
  return (11 | status << 8 | control << 16 | value << 24) >>> 0;
}
const u32 = n => Number.isInteger(n) && n >= 0 && n <= 0xffffffff;
const validPacket = p => u32(p) && (p & 255) === 11 &&
  ((p >>> 8) & 255) >= 0xb0 && ((p >>> 8) & 255) <= 0xbf &&
  ((p >>> 16) & 255) < 120 && (p >>> 24) <= 127;

export class CcInputWorkletBridge {
  constructor(exports, post) {
    this.exports = exports; this.post = post; this.route = 0; this.ready = false;
  }
  get ex() { return this.exports(); }
  supported() {
    return ['web_midi_ingest', 'web_cc_reset_source', 'web_cc_version'].every(k => typeof this.ex?.[k] === 'function') &&
      this.ex.web_cc_version() === 0x00010001;
  }
  reset() {
    // Replacing the engine never reactivates an input. Preserve route generation
    // so queued messages from the previous selected port remain inadmissible.
    this.ready = false;
    if (this.supported()) this.ex.web_cc_reset_source(2);
  }
  message(m) {
    if (!m || !['cc-input-route', 'cc-input-batch'].includes(m.type)) return false;
    const reply = (result, prefix = 0) => this.post({type:'cc-input-reply', request:m.request,
      route:this.route, result, prefix, ready:this.ready});
    if (!u32(m.request) || !u32(m.route)) { reply(-1); return true; }
    if (m.type === 'cc-input-route') {
      if (typeof m.ready !== 'boolean') { reply(-1); return true; }
      if (m.route !== this.route && ((m.route - this.route) | 0) <= 0) { reply(-3); return true; }
      if (m.ready && !this.ready && m.route === this.route) { reply(-3); return true; }
      if (m.ready && !this.supported()) { reply(-2); return true; }
      // Duplicate route acknowledgements must not erase selectors already sent
      // by the DAW after the first acknowledgement.
      if ((m.route !== this.route || m.ready !== this.ready) && this.supported()) this.ex.web_cc_reset_source(2);
      this.route = m.route; this.ready = m.ready; reply(0); return true;
    }
    if (!Array.isArray(m.packets) || m.packets.length < 1 || m.packets.length > 16 || !Array.from(m.packets).every(validPacket)) {
      reply(-1); return true;
    }
    if (!this.ready || m.route !== this.route) { reply(-3); return true; }
    if (!this.supported()) { this.ready = false; reply(-2); return true; }
    let prefix = 0;
    for (const packet of m.packets) {
      if (this.ex.web_midi_ingest(packet, 2) !== 1) break;
      prefix++;
    }
    reply(0, prefix); return true;
  }
}
