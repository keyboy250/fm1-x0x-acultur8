
"use strict";
const NS = "http://www.w3.org/2000/svg";
const BTN = { FX: 0, SEL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, "OCT-": 12, "OCT+": 13 };
const ENC = { SELECT: 0, ALGORITHM: 1, PRESETS: 2, KNOB1: 3, KNOB2: 4, KNOB3: 5, KNOB4: 6 };
const WHITE_K = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];
const BLACK_K = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];
const BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14];
const BODY = "#3a3d42", TRAY = "#26282c", KEY_W = "#d9dadc", KEY_B = "#c9cacd", KNOB = "#1f2124", BTNC = "#2e3135", LABEL = "#e8e8ea";
const STORE_KEY = "acultur8-fm1-preview-032-store-v1";

const svg = document.getElementById("panel");
function el(tag, attrs, parent = svg) {
  const e = document.createElementNS(NS, tag);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v);
  parent.appendChild(e);
  return e;
}
function text(parent, x, y, s, size, fill = LABEL, weight = 600) {
  const t = el("text", { x, y, "text-anchor": "middle", "font-family": "'Barlow Semi Condensed', 'Arial Narrow', Arial, sans-serif",
                         "font-size": size, "font-weight": weight, fill, "letter-spacing": ".05em", class: "btn", "pointer-events": "none" }, parent);
  t.textContent = s;
  return t;
}

// ---------------------------------------------------------------- the panel
el("rect", { x: 4, y: 4, width: 992, height: 632, rx: 44, fill: BODY });
const knobs = {};
function knob(x, y, r, label, role) {
  const g = el("g", { class: "ctl knob", tabindex: 0, role: "slider", "aria-label": label });
  el("circle", { cx: x, cy: y, r: r + 5, fill: TRAY }, g);
  el("circle", { cx: x, cy: y, r, fill: KNOB, stroke: "#44474c", "stroke-width": 2 }, g);
  const ind = el("line", { x1: x, y1: y - r + 5, x2: x, y2: y - r + 15, stroke: LABEL, "stroke-width": 3, "stroke-linecap": "round" }, g);
  text(g, x, y - r - 12, label, 17);
  el("circle", { cx: x, cy: y, r: r + 14, fill: "transparent" }, g);
  knobs[label] = { g, ind, x, y, angle: 0, role };
  return knobs[label];
}
knob(105, 100, 24, "MASTER", "master");
knob(215, 100, 24, "SELECT", ENC.SELECT);
knob(105, 205, 24, "PRESETS", ENC.PRESETS);
knob(215, 205, 24, "ALGORITHM", ENC.ALGORITHM);
for (let i = 0; i < 4; i++) knob(610 + i * 104, 100, 22, `KNOB${i + 1}`, ENC[`KNOB${i + 1}`]);

const buttons = {};
function button(x, y, w, h, label, two) {
  const g = el("g", { class: "ctl", role: "button", "aria-label": label });
  el("rect", { x, y, width: w, height: h, rx: 9, fill: BTNC, stroke: "#4a4d52", "stroke-width": 1.5, class: "btnface" }, g);
  if (two) { text(g, x + w / 2, y + h / 2 - 1, label, 12); text(g, x + w / 2, y + h / 2 + 12, two, 12); }
  else text(g, x + w / 2, y + h / 2 + 5, label, 15);
  buttons[label] = { g, bit: BTN[label] };
}
el("rect", { x: 58, y: 258, width: 210, height: 58, rx: 14, fill: TRAY });
button(72, 268, 82, 38, "OCT-"); button(170, 268, 82, 38, "OCT+");
el("rect", { x: 300, y: 44, width: 236, height: 236, rx: 26, fill: "#111214" });
el("rect", { x: 568, y: 160, width: 400, height: 140, rx: 18, fill: TRAY });
const ROW1 = ["FX", "SEL", "ENV", "LFO", "EDIT", "GLO"], ROW2 = ["HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"];
for (let i = 0; i < 6; i++) {
  button(582 + i * 63, 174, 54, 48, ROW1[i]);
  if (ROW2[i] === "PLAY") button(582 + i * 63, 236, 54, 48, "PLAY", "STOP"); else button(582 + i * 63, 236, 54, 48, ROW2[i]);
}

const keys = [];
(function keybed(x0, y0, w, h) {
  el("rect", { x: x0, y: y0, width: w, height: h, rx: 18, fill: TRAY });
  const pitch = (w - 24) / 16, kw = pitch * 0.78;
  for (let i = 0; i < 16; i++) {
    const g = el("g", { class: "ctl", role: "button", "aria-label": `white key ${i + 1}` });
    el("rect", { x: x0 + 12 + i * pitch + (pitch - kw) / 2, y: y0 + h * 0.43, width: kw, height: h * 0.52, rx: kw / 2, fill: KEY_W, class: "keyface white" }, g);
    keys[WHITE_K[i]] = { g };
  }
  BLACK_AFTER.forEach((a, k) => {
    const g = el("g", { class: "ctl", role: "button", "aria-label": `black key ${k + 1}` });
    el("rect", { x: x0 + 12 + (a + 1) * pitch - kw / 2, y: y0 + h * 0.06, width: kw, height: h * 0.33, rx: kw / 2, fill: KEY_B, class: "keyface black" }, g);
    keys[BLACK_K[k]] = { g };
  });
})(36, 340, 928, 270);

// ---------------------------------------------------------------- touch targets
// The drawing is only a picture (pointer-events: none). Each control gets an invisible HTML target
// over it: browsers (iOS Safari above all) honour touch-action and touchstart on HTML elements but
// not on SVG ones, so the controls keep their touches while a finger anywhere else on the device
// scrolls the page natively.
const device = document.getElementById("device");
function hit(c, cls) {
  const b = c.g.getBBox(), d = document.createElement("div");
  d.className = "hit" + (cls ? " " + cls : "");
  Object.assign(d.style, { left: `${b.x / 10}%`, top: `${b.y / 6.4}%`, width: `${b.width / 10}%`, height: `${b.height / 6.4}%` });
  for (const a of ["role", "aria-label", "tabindex"]) if (c.g.hasAttribute(a)) { d.setAttribute(a, c.g.getAttribute(a)); c.g.removeAttribute(a); }
  c.g.setAttribute("aria-hidden", "true");
  d.addEventListener("touchstart", (e) => e.preventDefault(), { passive: false });
  device.insertBefore(d, document.getElementById("start"));
  c.hit = d;
}
for (const k of Object.values(knobs)) hit(k, "knob");
for (const b of Object.values(buttons)) hit(b);
keys.forEach((k) => hit(k));

// ---------------------------------------------------------------- state and input
let node = null, btnMask = 0, keyMask = 0, latched = 0, master = 2800;
const btnHeldBy = new Map(), keyHeldBy = new Map();      // pointer / key id -> bit
function send(m) { if (node) node.port.postMessage(m); }
function syncButtons() {
  let m = latched;
  for (const b of btnHeldBy.values()) m |= 1 << b;
  if (m !== btnMask) { btnMask = m; send({ type: "buttons", mask: m }); }
  for (const b of Object.values(buttons)) {
    b.g.classList.toggle("held", !!(m & (1 << b.bit)));
    b.g.classList.toggle("latched", !!(latched & (1 << b.bit)));
    b.hit.classList.toggle("latched", !!(latched & (1 << b.bit)));
    b.hit.setAttribute("aria-pressed", m & (1 << b.bit) ? "true" : "false");
  }
}
function syncKeys() {
  let m = 0;
  for (const k of keyHeldBy.values()) m |= 1 << k;
  if (m !== keyMask) { keyMask = m; send({ type: "keys", mask: m }); }
  keys.forEach((k, i) => k.g.classList.toggle("held", !!(m & (1 << i))));
}

const buttonPresses = new Map(); // Preserve short clicks through the firmware input scan.
for (const b of Object.values(buttons)) {
  b.hit.addEventListener("contextmenu", (e) => e.preventDefault());
  b.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    if (e.shiftKey || e.button === 2) { latched ^= 1 << b.bit; syncButtons(); return; }
    if (latched & (1 << b.bit)) { latched &= ~(1 << b.bit); syncButtons(); return; }
    b.hit.setPointerCapture(e.pointerId);
    buttonPresses.set("p" + e.pointerId, {at: performance.now()});
    btnHeldBy.set("p" + e.pointerId, b.bit);
    syncButtons();
  });
}
keys.forEach((k, i) => {
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    k.hit.setPointerCapture(e.pointerId);
    keyHeldBy.set("p" + e.pointerId, i);
    syncKeys();
  });
});
function pointerUp(e) {
  const id = "p" + e.pointerId;
  const press=buttonPresses.get(id);
  const release=()=>{if(buttonPresses.get(id)!==press)return;buttonPresses.delete(id);if(btnHeldBy.delete(id))syncButtons();};
  if(press && e.type!=="pointercancel")setTimeout(release,Math.max(0,60-(performance.now()-press.at)));
  else release();
  if (keyHeldBy.delete(id)) syncKeys();
}
device.addEventListener("pointerup", pointerUp);
device.addEventListener("pointercancel", pointerUp);
device.addEventListener("lostpointercapture", pointerUp);

function drawKnob(k) {
  k.ind.setAttribute("transform", `rotate(${k.angle} ${k.x} ${k.y})`);
}
function turn(k, n) {
  if (!n) return;
  if (k.role === "master") {
    master = Math.max(0, Math.min(4096, master + n * 64));
    k.angle = -135 + 270 * master / 4096;
    send({ type: "master", value: master });
  } else {
    k.angle = (k.angle + n * 18) % 360;
    send({ type: "enc", role: k.role, n });
  }
  drawKnob(k);
}
for (const k of Object.values(knobs)) {
  if (k.role === "master") { k.angle = -135 + 270 * master / 4096; drawKnob(k); }
  let drag = null, wheel = 0;
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    k.hit.setPointerCapture(e.pointerId);
    drag = { y: e.clientY, acc: 0 };
  });
  k.hit.addEventListener("pointermove", (e) => {
    if (!drag) return;
    drag.acc += drag.y - e.clientY;
    drag.y = e.clientY;
    const step = k.role === "master" ? 3 : 7;
    const n = Math.trunc(drag.acc / step);
    if (n) { drag.acc -= n * step; turn(k, n); }
  });
  const end = () => { drag = null; };
  k.hit.addEventListener("pointerup", end);
  k.hit.addEventListener("pointercancel", end);
  k.hit.addEventListener("wheel", (e) => {
    e.preventDefault();
    const dy = e.deltaMode === 1 ? e.deltaY * 33 : e.deltaY;
    if (Math.abs(dy) >= 50) { turn(k, dy < 0 ? 1 : -1); wheel = 0; return; }   // a mouse wheel notch: one detent
    wheel -= dy;                                                                   // a trackpad: by distance
    const n = Math.trunc(wheel / 25);
    if (n) { wheel -= n * 25; turn(k, n); }
  }, { passive: false });
  k.hit.addEventListener("keydown", (e) => {
    if (e.key === "ArrowUp" || e.key === "ArrowRight") { turn(k, 1); e.preventDefault(); e.stopPropagation(); }
    if (e.key === "ArrowDown" || e.key === "ArrowLeft") { turn(k, -1); e.preventDefault(); e.stopPropagation(); }
  });
}

const KEYMAP = {};
"qwertyui".split("").forEach((c, i) => KEYMAP["Key" + c.toUpperCase()] = { key: WHITE_K[i] });
"asdfghjk".split("").forEach((c, i) => KEYMAP["Key" + c.toUpperCase()] = { key: WHITE_K[8 + i] });
"1234567890".split("").forEach((c, i) => KEYMAP["Digit" + c] = { key: BLACK_K[i] });
KEYMAP.Minus = { key: BLACK_K[10] };
Object.assign(KEYMAP, { Space: { btn: BTN.PLAY }, Enter: { btn: BTN.REC }, ShiftLeft: { btn: BTN.SEL }, ShiftRight: { btn: BTN.SEL },
                        Escape: { btn: BTN.HOME }, KeyZ: { btn: BTN["OCT-"] }, KeyX: { btn: BTN["OCT+"] } });
addEventListener("keydown", (e) => {
  if (!node || e.metaKey || e.ctrlKey || e.altKey) return;
  // Text/sliders and auxiliary editor controls keep their normal keyboard behavior.
  if (e.target.closest?.("input, textarea, select, [contenteditable], .variant-lab, .arrangement-lab, .slice-lab, .breaks")) return;
  if (e.code === "ArrowUp" || e.code === "ArrowDown") { turn(knobs.SELECT, e.code === "ArrowUp" ? 1 : -1); e.preventDefault(); return; }
  const m = KEYMAP[e.code];
  if (!m) return;
  e.preventDefault();
  if (e.repeat) return;
  if (m.key !== undefined) { keyHeldBy.set("k" + e.code, m.key); syncKeys(); }
  else { btnHeldBy.set("k" + e.code, m.btn); syncButtons(); }
});
addEventListener("keyup", (e) => {
  const id = "k" + e.code;
  if (keyHeldBy.delete(id)) syncKeys();
  if (btnHeldBy.delete(id)) syncButtons();
});
addEventListener("blur", () => {
  for (const id of [...btnHeldBy.keys()]) if (id[0] === "k") btnHeldBy.delete(id);
  for (const id of [...keyHeldBy.keys()]) if (id[0] === "k") keyHeldBy.delete(id);
  syncButtons(); syncKeys();
});

// ---------------------------------------------------------------- screen and lights
const ctx = document.getElementById("screen").getContext("2d");
const img = ctx.createImageData(240, 240);
function drawScreen(fb) {
  const d = img.data;
  for (let i = 0; i < 240 * 240; i++) {
    const v = fb[i], p = ((v >> 8) | (v << 8)) & 0xFFFF;
    d[i * 4] = ((p >> 11) & 31) * 255 / 31;
    d[i * 4 + 1] = ((p >> 5) & 63) * 255 / 63;
    d[i * 4 + 2] = (p & 31) * 255 / 31;
    d[i * 4 + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}
function drawLights(b, k, d = 0) {
  for (const x of Object.values(buttons)) x.g.classList.toggle("lit", !!(b & (1 << x.bit)));
  keys.forEach((x, i) => {
    x.g.classList.toggle("lit", !!(k & (1 << i)));
    x.g.classList.toggle("dim", !(k & (1 << i)) && !!(d & (1 << i)));
  });
}

// ---------------------------------------------------------------- storage
function loadStore() {
  try {
    const raw = JSON.parse(localStorage.getItem(STORE_KEY) || "{}"), out = {};
    for (const [o, b64] of Object.entries(raw)) out[o] = Uint8Array.from(atob(b64), (c) => c.charCodeAt(0)).buffer;
    return out;
  } catch { return {}; }
}
function saveStore(store) {
  try {
    const raw = {};
    for (const [o, u8] of Object.entries(store)) { let s = ""; for (const c of u8) s += String.fromCharCode(c); raw[o] = btoa(s); }
    localStorage.setItem(STORE_KEY, JSON.stringify(raw));
  } catch { /* storage unavailable: this session only */ }
}

// ---------------------------------------------------------------- your breaks
// A file becomes mono 22050 Hz 16-bit here (the browser decodes and resamples it), peak 0.95 as
// tools/import_breaks.py leaves it, and goes to the worklet, which IMA-encodes it into the slot as
// an upload to the FM-1 would. Slots are kept in IndexedDB (they are too big for localStorage).
const SLOT_RATE = 22050, SLOT_MAX = 2 * (0x14000 - 512), SLOT_ZONES = 16;
const slots = [null, null, null];                 // {name, zones: [{pcm: Int16Array, rate}]}
function idb() {
  return new Promise((res, rej) => {
    const r = indexedDB.open("acultur8-fm1-preview-032-loops-v1", 1);
    r.onupgradeneeded = () => r.result.createObjectStore("slots");
    r.onsuccess = () => res(r.result);
    r.onerror = () => rej(r.error);
  });
}
async function slotsLoad() {
  try {
    const db = await idb();
    await Promise.all([0, 1, 2].map((k) => new Promise((res) => {
      const q = db.transaction("slots").objectStore("slots").get(k);
      q.onsuccess = () => { if (q.result) slots[k] = { name: q.result.name, zones: q.result.zones.map((z) => ({ pcm: new Int16Array(z.pcm), rate: z.rate })) }; res(); };
      q.onerror = () => res();
    })));
  } catch { /* no IndexedDB: this session only */ }
}
async function slotSave(k) {
  try {
    const db = await idb(), st = db.transaction("slots", "readwrite").objectStore("slots");
    if (slots[k]) st.put({ name: slots[k].name, zones: slots[k].zones.map((z) => ({ pcm: z.pcm.buffer.slice(0), rate: z.rate })) }, k);
    else st.delete(k);
  } catch { /* this session only */ }
}
function slotSend(k) { send({ type: "slot", k, name: slots[k] ? slots[k].name : "", zones: slots[k] ? slots[k].zones : [] }); }
async function decode(file) {
  const ctx = new OfflineAudioContext(1, 1, SLOT_RATE);
  const buf = await ctx.decodeAudioData(await file.arrayBuffer());
  const off = new OfflineAudioContext(1, Math.max(1, Math.ceil(buf.duration * SLOT_RATE)), SLOT_RATE);
  const src = off.createBufferSource();
  src.buffer = buf;
  src.connect(off.destination);
  src.start();
  const x = (await off.startRendering()).getChannelData(0);
  let pk = 1e-9;
  for (const v of x) pk = Math.max(pk, Math.abs(v));
  const g = 0.95 / pk, pcm = new Int16Array(x.length);
  for (let i = 0; i < x.length; i++) pcm[i] = Math.round(Math.max(-1, Math.min(1, x[i] * g)) * 32767);
  return pcm;
}
const slotName = (file) => (file.name.replace(/\.[^.]*$/, "").toUpperCase().replace(/[^A-Z0-9]/g, "") || "LOOP").slice(0, 8);
async function slotFill(k, files) {
  files = [...files].filter((f) => f && f.size).slice(0, SLOT_ZONES);
  if (!files.length) return;
  const row = document.querySelectorAll(".slot")[k];
  row.querySelector(".sname").textContent = "Reading...";
  const zones = [];
  let used = 0, skipped = 0;
  for (const f of files) {
    try {
      const pcm = await decode(f);
      if (used + pcm.length > SLOT_MAX) { skipped++; continue; }
      zones.push({ pcm, rate: SLOT_RATE });
      used += pcm.length;
    } catch { skipped++; }
  }
  slots[k] = zones.length ? { name: slotName(files[0]), zones } : null;
  slotDraw(k, skipped);
  slotSend(k);
  slotSave(k);
}
function slotDraw(k, skipped = 0) {
  const row = document.querySelectorAll(".slot")[k], s = slots[k], n = row.querySelector(".sname");
  if (!s) { n.textContent = skipped ? "Could not read that file" : "Empty"; return; }
  const sec = s.zones.reduce((a, z) => a + z.pcm.length, 0) / SLOT_RATE;
  n.innerHTML = "";
  const b = document.createElement("strong");
  b.textContent = s.name;
  n.append(b, ` · ${s.zones.length} loop${s.zones.length > 1 ? "s" : ""} · ${sec.toFixed(1)} of ${(SLOT_MAX / SLOT_RATE).toFixed(1)} s` +
           (skipped ? ` · ${skipped} did not fit or could not be read` : ""));
}
(function slotsUi() {
  const box = document.getElementById("slots");
  for (let k = 0; k < 3; k++) {
    const row = document.createElement("div");
    row.className = "slot";
    row.innerHTML = `<b>Slot ${k + 1}</b><span class="sname">Empty</span>` +
      `<label>Choose files<input type="file" multiple accept="audio/*,.wav,.aif,.aiff,.mp3,.flac"></label><button type="button">Clear</button>`;
    row.querySelector("input").addEventListener("change", (e) => { slotFill(k, e.target.files); e.target.value = ""; });
    row.querySelector("button").addEventListener("click", () => { slots[k] = null; slotDraw(k); slotSend(k); slotSave(k); });
    row.addEventListener("dragover", (e) => { e.preventDefault(); row.classList.add("over"); });
    row.addEventListener("dragleave", () => row.classList.remove("over"));
    row.addEventListener("drop", (e) => { e.preventDefault(); row.classList.remove("over"); slotFill(k, e.dataTransfer.files); });
    box.appendChild(row);
  }
  slotsLoad().then(() => [0, 1, 2].forEach((k) => slotDraw(k)));
})();

// ---------------------------------------------------------------- switching on
const status = document.getElementById("status"), cpu = document.getElementById("cpu");
async function switchOn() {
  const go = document.getElementById("go");
  go.disabled = true;
  go.textContent = "Starting...";
  try {
    const ac = new AudioContext({ sampleRate: 44100, latencyHint: "interactive" });
    const [wasm] = await Promise.all([fetch("x0x.wasm").then((r) => { if (!r.ok) throw new Error(`x0x.wasm: ${r.status}`); return r.arrayBuffer(); }),
                                      ac.audioWorklet.addModule("worklet.js?v=032-preview-1")]);
    node = new AudioWorkletNode(ac, "x0x", { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
    node.connect(ac.destination);
    const ready = new Promise((res) => {
      node.port.onmessage = (e) => {
        const m = e.data;
        window.dispatchEvent(new CustomEvent("fm1-marked", {detail:m}));
        if (m.type === "ready") res();
        else if (m.type === "frame") {
          if (m.fb) drawScreen(m.fb);
          drawLights(m.buttons, m.keys, m.dim);
          window.x0xLast = m;
          if (m.load !== undefined) cpu.textContent = `X0X uses ${m.load}% of this computer's time`;
          if (m.store) saveStore(m.store);
        }
      };
    });
    node.port.postMessage({ type: "load", wasm, store: loadStore(), master }, [wasm]);
    await ready;
    [0, 1, 2].forEach((k) => { if (slots[k]) slotSend(k); });   // the loops kept in this browser
    await ac.resume();
    document.getElementById("start").hidden = true;
    status.textContent = ac.sampleRate === 44100 ? "On." : `On (the browser runs audio at ${ac.sampleRate} Hz).`;
    syncButtons(); syncKeys();
    window.dispatchEvent(new Event("fm1-ready"));
  } catch (err) {
    go.disabled = false;
    go.textContent = "Switch on";
    status.textContent = "Could not start: " + (err && err.message || err) + ". Chrome, Edge, Firefox and Safari should all work.";
  }
}
document.getElementById("go").addEventListener("click", switchOn);
document.getElementById("reset").addEventListener("click", () => {
  if (document.getElementById("reset").dataset.armed) {
    try { localStorage.removeItem(STORE_KEY); } catch {}
    location.reload();
  } else {
    document.getElementById("reset").dataset.armed = "1";
    document.getElementById("reset").textContent = "Click again to forget everything";
  }
});

window.fm1SliceSend = send;
