const localReaderWords = {"4a492b07c2.json":{"format":1,"words":[["BD","bass drum"],["SD","snare"],["LT","low tom"],["MT","mid tom"],["HT","high tom"],["RS","rimshot"],["CP","clap"],["CH","closed hat"],["OH","open hat"],["CR","crash"],["RD","ride"],["CB","cowbell"],["CY","cymbal"],["909","nine oh nine"],["808","eight oh eight"],["303A","three oh three A"],["303B","three oh three B"],["303","three oh three"],["BRK","break"],["TB-3PO","T B three P O"],["P#-#","patterns # to #"],["P#","pattern #"],["PAGES: 1-16 17-32 33-48 49-64","pages: 1 to 16, 17 to 32, 33 to 48, 49 to 64"],["S#","song bar #"],["STEP #: #:#","step #: loop # of every #"],["LENGTH # st","length # steps"],["TRANSP # st","transpose # semitones"],["TRANSP","transpose"],["DIRECTION REV","direction reverse"],["NEW LINE *","new line"],["BATTERY ...","battery checking"],["BATTERY 1/3","battery one third"],["BATTERY 2/3","battery two thirds"],["A LEN","loop A length"],["B LEN","loop B length"],["RTG#X","retrigger times #"],["P.DPTH","pitch depth"],["ACCDEC","accent decay"],["DRVTYP","drive type"],["ENVMOD","envelope mod"],["RELEAS","release"],["DENS","density"],["OCTS","octaves"],["CMPLX","complexity"],["RV","reverb"],["DL","delay"],["BFZ","fuzz"],["PDIST","crunch"],["Sqr","square"],["909 GR","nine oh nine groove"],["909 FL","nine oh nine fill"],["DOR","dorian"],["PHRY","phrygian"],["HARM","harmonic minor"],["MPEN","minor pentatonic"],["PING-PONG","ping-pong"],["PING","ping-pong"],["CHN","chain"],["SEL","S E L"],["RETRIG","retrigger"],["X0X","X zero X"],["PAN C","pan center"],["PAN L#","pan left #"],["PAN R#","pan right #"],["FX","effects"],["REV","reverb"],["DLY","delay"],["ENV","envelope"],["LFO","L F O"],["GLO","global"],["OCT+","octave up"],["OCT-","octave down"],["OCT","octave"],["REC","record"],["SEQ","sequence"],["ALGO","algorithm"],["ARP","arp"],["USB","U S B"],["MIDI CH #","MIDI channel #"],["MIDI","midi"],["dB","decibels"],["Hz","hertz"],["kHz","kilohertz"],["ms","milliseconds"],["bpm","B P M"],["CC #","controller #"],["CC","controller"],["INF","infinity"],["PCT","percent"],["2ND","second"],["FWD","forward"],["RND","random"],["MEM","memory"],["CPU","C P U"],["P-LOCK","p-lock"],["MIN","minor"],["MAJ","major"],["ATK","attack"],["DCY","decay"],["SUS","sustain"],["REL","release"],["RES","resonance"],["RESO","resonance"],["FREQ","frequency"],["AMT","amount"],["LVL","level"],["VOL","volume"],["DIST","distortion"],["SAT","saturation"],["COMP","compressor"],["THRESH","threshold"],["PERF","performance"],["HP","high-pass"],["LP","low-pass"],["BP","band-pass"]]},"c8a6bb1555.json":{"format":1,"words":[["BD","bass drum"],["SD","snare"],["LT","low tom"],["MT","mid tom"],["HT","high tom"],["RS","rimshot"],["CP","clap"],["CH","closed hat"],["OH","open hat"],["CR","crash"],["RD","ride"],["CB","cowbell"],["CY","cymbal"],["909","nine oh nine"],["808","eight oh eight"],["303A","three oh three A"],["303B","three oh three B"],["303","three oh three"],["BRK","break"],["TB-3PO","T B three P O"],["P#-#","patterns # to #"],["P#","pattern #"],["PAGES: 1-16 17-32 33-48 49-64","pages: 1 to 16, 17 to 32, 33 to 48, 49 to 64"],["S#","song bar #"],["STEP #: #:#","step #: loop # of every #"],["LENGTH # st","length # steps"],["TRANSP # st","transpose # semitones"],["TRANSP","transpose"],["DIRECTION REV","direction reverse"],["NEW LINE *","new line"],["BATTERY ...","battery checking"],["BATTERY 1/3","battery one third"],["BATTERY 2/3","battery two thirds"],["A LEN","loop A length"],["B LEN","loop B length"],["RTG#X","retrigger times #"],["P.DPTH","pitch depth"],["ACCDEC","accent decay"],["DRVTYP","drive type"],["ENVMOD","envelope mod"],["RELEAS","release"],["DENS","density"],["OCTS","octaves"],["CMPLX","complexity"],["RV","reverb"],["DL","delay"],["BFZ","fuzz"],["PDIST","crunch"],["Sqr","square"],["909 GR","nine oh nine groove"],["909 FL","nine oh nine fill"],["DOR","dorian"],["PHRY","phrygian"],["HARM","harmonic minor"],["MPEN","minor pentatonic"],["PING-PONG","ping-pong"],["PING","ping-pong"],["CHN","chain"],["SEL","S E L"],["RETRIG","retrigger"],["X0X","X zero X"],["PAN C","pan center"],["PAN L#","pan left #"],["PAN R#","pan right #"],["FX","effects"],["REV","reverb"],["DLY","delay"],["ENV","envelope"],["LFO","L F O"],["GLO","global"],["OCT+","octave up"],["OCT-","octave down"],["OCT","octave"],["REC","record"],["SEQ","sequence"],["ALGO","algorithm"],["ARP","arp"],["USB","U S B"],["MIDI CH #","MIDI channel #"],["MIDI","midi"],["dB","decibels"],["Hz","hertz"],["kHz","kilohertz"],["ms","milliseconds"],["bpm","B P M"],["CC","C C"],["INF","infinity"],["PCT","percent"],["2ND","second"],["FWD","forward"],["RND","random"],["MEM","memory"],["CPU","C P U"],["P-LOCK","p-lock"],["MIN","minor"],["MAJ","major"],["ATK","attack"],["DCY","decay"],["SUS","sustain"],["REL","release"],["RES","resonance"],["RESO","resonance"],["FREQ","frequency"],["AMT","amount"],["LVL","level"],["VOL","volume"],["DIST","distortion"],["SAT","saturation"],["COMP","compressor"],["THRESH","threshold"],["PERF","performance"],["HP","high-pass"],["LP","low-pass"],["BP","band-pass"]]}};

// The screen reader's words: words.js is fm1-reader's (web/emu/words.js, a copy), the words file X0X's own
// (../words/, the one its HELLO names). The page's voice (reader/tts.js and espeak) is copied in from fm1-reader
// by web/emu/build.sh, and loaded only when it is chosen.
// SPDX-License-Identifier: GPL-3.0-only
// (A copy of fm1-reader's web/words.js, github.com/charlesvestal/fm1-reader, for the emulator's screen reader:
// web/emu/build.sh says when it no longer matches ../fm1-reader's.)
// The screen's text in words: the firmware's own list first (its words file, fetched from the link it gives:
// PROTOCOL.md), then the common way to say numbers and signs. One copy, used by the page, the audit and the
// tests.
//   spoken('BD (1/2): TUNE 34', words) -> 'bass drum, page 1 of 2: tune 34'
//   words: [[screen, said], ...] from a words file ({ format: 1, words: [...] }); see tools/gen_words.py
//
// A list entry matches whole words, capitals or not. "#" in both takes a number (digits, a sign, a decimal
// point), up to three, in order; "*" in the screen's text takes a word and drops it. The longest match wins;
// an earlier entry when they match as much (the firmware's own come before the common ones).

const al = (c) => /[A-Za-z]/.test(c || '');
const dg = (c) => /[0-9]/.test(c || '');
const an = (c) => al(c) || dg(c);
const NOTE = { 2: 'half', 4: 'quarter', 8: 'eighth', 16: 'sixteenth', 32: 'thirty-second' };
// after a capital A, these make it the note, not the article
const NOTE_WORD = /^(MINOR|MAJOR|MIN|MAJ|SHARP|FLAT|NATURAL|DORIAN|PHRYGIAN|LYDIAN|MIXOLYDIAN|LOCRIAN|AEOLIAN|IONIAN|PENTATONIC|BLUES|HARMONIC|MELODIC|CHROMATIC|SEVENTH|SIXTH|NINTH|ELEVENTH|THIRTEENTH|DIMINISHED|AUGMENTED|SUSPENDED|ADD|DOMINANT|POWER|minor|major|sharp|flat|seventh|diminished|augmented|suspended)\b/;

function match(s, i, k) {                         // length matched at s[i], and the numbers
    const caps = [];
    let p = i;
    for (let j = 0; j < k.length; j++) {
        const c = k[j];
        if (c === '#') {
            const b = p;
            if (s[p] === '-' || s[p] === '+') p++;
            while (dg(s[p]) || (s[p] === '.' && dg(s[p + 1]))) p++;
            if (p === b || !dg(s[p - 1])) return null;
            caps.push(s.slice(b, p));
        } else if (c === '*') {
            const b = p;
            while (an(s[p])) p++;
            if (p === b) return null;
        } else {
            if ((s[p] || '').toLowerCase() !== c.toLowerCase()) return null;
            p++;
        }
    }
    if (an(s[p]) && an(s[p - 1])) return null;    // whole: not the start of a longer word
    return { len: p - i, caps };
}

function spoken(s, words = []) {
    let o = '';
    let i = 0;
    const last = () => o[o.length - 1];
    while (i < s.length) {
        const c = s[i];
        const start = i === 0 || !an(s[i - 1]);
        if (start && an(c)) {                      // a word: the list
            let best = null, said = '';
            for (const [k, v] of words) {
                const m = match(s, i, k);
                if (m && (!best || m.len > best.len)) { best = m; said = v; }
            }
            if (best) {
                let n = 0;
                o += said.replace(/#/g, () => best.caps[n++] ?? '');
                i += best.len;
                continue;
            }
        }
        if (start && c === '1' && s[i + 1] === '/') {        // 1/16, 1/8T (a triplet), 1/8. (dotted: the dot ends it)
            let q = i + 2, d = '';
            while (dg(s[q])) d += s[q++];
            if (NOTE[d]) {
                const trip = s[q] === 'T', dot = s[q] === '.' && (q + 1 >= s.length || s[q + 1] === ',' || s[q + 1] === '.');
                if ((trip || dot) ? !an(s[q + 1]) : !an(s[q])) {
                    o += (dot ? 'dotted ' : '') + NOTE[d] + (trip ? ' triplet' : '');
                    i = q + (trip || dot ? 1 : 0);
                    continue;
                }
            }
        }
        if (start && dg(c)) {                                  // 4:1 a ratio, 1-16 a range, 1/3 a share
            let q = i;
            while (dg(s[q]) || (s[q] === '.' && dg(s[q + 1]))) q++;
            if ((s[q] === ':' || s[q] === '-' || s[q] === '/') && dg(s[q + 1])) {
                let e = q + 1;
                while (dg(s[e])) e++;
                if (!an(s[e])) {
                    o += s.slice(i, q) + (s[q] === '/' ? ' of ' : ' to ');
                    i = q + 1;
                    continue;
                }
            }
        }
        if (start && c >= 'A' && c <= 'G' && s[i + 1] === '#') {   // C#3: C sharp 3
            o += c + ' sharp' + (dg(s[i + 2]) || s[i + 2] === '-' ? ' ' : '');
            i += 2;
            continue;
        }
        if (start && c === 'x' && dg(s[i + 1])) {                  // x2: 2 times
            let q = i + 1;
            while (dg(s[q])) q++;
            if (!an(s[q])) {
                const n = s.slice(i + 1, q);
                o += n + (n === '1' ? ' time' : ' times');
                i = q;
                continue;
            }
        }
        if (start && c === 'A' && s[i + 1] === ' ' && al(s[i + 2]) && al(s[i + 3]) &&
            !NOTE_WORD.test(s.slice(i + 2))) {             // A NEW LINE: the article (A MINOR, A FLAT: the note)
            o += 'a';
            i++;
            continue;
        }
        if (start && al(c)) {                                      // a word in capitals: a word, not letters
            let q = i, caps = 0, lower = 0;
            while (al(s[q]) || s[q] === "'") {
                if (/[A-Z]/.test(s[q])) caps++;
                if (/[a-z]/.test(s[q])) lower++;
                q++;
            }
            const w = s.slice(i, q);
            o += caps >= 2 && !lower && !dg(s[q]) ? w.toLowerCase() : w;
            i = q;
            continue;
        }
        if (c === '(' && dg(s[i + 1])) {                           // (1/2): page 1 of 2
            const m = /^\((\d+)\/(\d+)\)/.exec(s.slice(i));
            if (m) {
                o = o.replace(/ +$/, '') + `, page ${m[1]} of ${m[2]}`;
                i += m[0].length;
                continue;
            }
        }
        if (c === '%') { o += (o && last() !== ' ' ? ' ' : '') + 'percent'; i++; continue; }
        if (c === '+' && (i === 0 || s[i - 1] === ' ' || s[i - 1] === '(') && s[i + 1] === ' ') { o += 'plus'; i++; continue; }
        if (c === '/' && i > 0 && s[i - 1] === ' ' && s[i + 1] === ' ') { o += 'or'; i++; continue; }
        if (c === ' ') {                                           // one space; none before a mark or at the end
            while (s[i + 1] === ' ') i++;
            if (!o || last() === ' ' || ',.:'.includes(s[i + 1] || '.') ) { i++; continue; }
        }
        if (c === '.' && o && ',:?!.'.includes(last())) {           // "DELAY,." / "PATTERN?.": one mark
            if (last() === ',' || last() === ':') o = o.slice(0, -1) + '.';
            i++;
            continue;
        }
        o += c;
        i++;
    }
    return o;
}

// a words file, checked: { format: 1, words: [[screen, said], ...] }
function wordsOf(json) {
    if (!json || json.format !== 1 || !Array.isArray(json.words)) throw new Error('not a words file (format 1)');
    return json.words.filter((w) => Array.isArray(w) && typeof w[0] === 'string' && typeof w[1] === 'string' && w[0]);
}


const NS = "http://www.w3.org/2000/svg";
const BTN = { FX: 0, SEL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, "OCT-": 12, "OCT+": 13 };
const ENC = { SELECT: 0, ALGORITHM: 1, PRESETS: 2, KNOB1: 3, KNOB2: 4, KNOB3: 5, KNOB4: 6 };
const WHITE_K = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];
const BLACK_K = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];
const BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14];
const BODY = "#3a3d42", TRAY = "#26282c", KEY_W = "#d9dadc", KEY_B = "#c9cacd", KNOB = "#1f2124", BTNC = "#2e3135", LABEL = "#e8e8ea";
const STORE_KEY = "acultur8-fm1-beta-0127-store-v1", PREFS_KEY = "x0x-emu-prefs";
// what each control does (the manual's panel drawing), for its description
const DOES = { MASTER: "Volume.", SELECT: "Page.", PRESETS: "Pattern.", ALGORITHM: "Part.", KNOB: "One of the page's four values.",
  "OCT-": "16 steps back.", "OCT+": "16 steps forward.", FX: "Effects.", SEL: "List all.", ENV: "Hold: accent.", LFO: "Mixer.",
  EDIT: "Sound.", GLO: "Settings.", HOME: "Pattern, song.", SAVE: "Save; hold: copy.", ARP: "TB-3PO.", SEQ: "Steps.",
  PLAY: "Play and stop.", REC: "Record." };
const TRACKS = ["bass drum", "snare", "low tom", "mid tom", "high tom", "rimshot", "clap", "closed hat", "open hat", "crash", "ride"];
const SPOKEN = { "OCT-": "OCT minus", "OCT+": "OCT plus" };

const $ = (id) => document.getElementById(id);
const prefs = (() => { try { return JSON.parse(localStorage.getItem(PREFS_KEY) || "{}"); } catch { return {}; } })();
const savePrefs = () => { try { localStorage.setItem(PREFS_KEY, JSON.stringify(prefs)); } catch { /* private window */ } };

const svg = $("panel");
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

// ---------------------------------------------------------------- the panel (a picture: the controls are over it)
el("rect", { x: 4, y: 4, width: 992, height: 632, rx: 44, fill: BODY });
const knobs = {};
function knob(x, y, r, label, role) {
  const g = el("g", { class: "ctl knob" });
  el("circle", { cx: x, cy: y, r: r + 5, fill: TRAY }, g);
  el("circle", { cx: x, cy: y, r, fill: KNOB, stroke: "#44474c", "stroke-width": 2 }, g);
  const ind = el("line", { x1: x, y1: y - r + 5, x2: x, y2: y - r + 15, stroke: LABEL, "stroke-width": 3, "stroke-linecap": "round" }, g);
  text(g, x, y - r - 12, label, 17);
  el("circle", { cx: x, cy: y, r: r + 14, fill: "transparent" }, g);
  knobs[label] = { g, ind, x, y, angle: 0, role, label };
  return knobs[label];
}
knob(105, 100, 24, "MASTER", "master");
knob(215, 100, 24, "SELECT", ENC.SELECT);
knob(105, 205, 24, "PRESETS", ENC.PRESETS);
knob(215, 205, 24, "ALGORITHM", ENC.ALGORITHM);
for (let i = 0; i < 4; i++) knob(610 + i * 104, 100, 22, `KNOB${i + 1}`, ENC[`KNOB${i + 1}`]);

const buttons = {};
function button(x, y, w, h, label, two) {
  const g = el("g", { class: "ctl" });
  el("rect", { x, y, width: w, height: h, rx: 9, fill: BTNC, stroke: "#4a4d52", "stroke-width": 1.5, class: "btnface" }, g);
  if (two) { text(g, x + w / 2, y + h / 2 - 1, label, 12); text(g, x + w / 2, y + h / 2 + 12, two, 12); }
  else text(g, x + w / 2, y + h / 2 + 5, label, 15);
  buttons[label] = { g, bit: BTN[label], label };
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
    const g = el("g", { class: "ctl" });
    el("rect", { x: x0 + 12 + i * pitch + (pitch - kw) / 2, y: y0 + h * 0.43, width: kw, height: h * 0.52, rx: kw / 2, fill: KEY_W, class: "keyface white" }, g);
    keys[WHITE_K[i]] = { g, label: `White key ${i + 1}`, does: `Step ${i + 1} (on HOME, pattern ${i + 1}).` };
  }
  BLACK_AFTER.forEach((a, k) => {
    const g = el("g", { class: "ctl" });
    el("rect", { x: x0 + 12 + (a + 1) * pitch - kw / 2, y: y0 + h * 0.06, width: kw, height: h * 0.33, rx: kw / 2, fill: KEY_B, class: "keyface black" }, g);
    keys[BLACK_K[k]] = { g, label: `Black key ${k + 1}`, does: `Drum track ${k + 1}, ${TRACKS[k]}; on the break, slice ${k + 1}.` };
  });
})(36, 340, 928, 270);

// ---------------------------------------------------------------- the controls
// The drawing is only a picture (pointer-events: none). Each control gets an invisible HTML control over it, in
// reading order: a knob a slider (MASTER) or a button you turn (the endless ones), a button or key a <button>.
// Browsers (iOS Safari above all) honour touch-action and touchstart on HTML elements but not on SVG ones, so
// the controls keep their touches while a finger anywhere else on the device scrolls the page natively.
const device = $("device"), controls = $("controls"), descs = $("descs");
const HOW = {
  enc: "Endless knob: turn it with the arrow keys, faster with Page Up and Page Down.",
  btn: "Space or Enter presses it for as long as you hold the key. Shift+Space keeps it held down until you press it again.",
  key: "Space or Enter presses it for as long as you hold the key.",
};
for (const [id, t] of Object.entries(HOW)) { const d = document.createElement("span"); d.id = "how-" + id; d.textContent = t; descs.append(d); }
let nDesc = 0;
function hit(c, kind, name, does) {
  const b = c.g.getBBox(), d = document.createElement(kind === "knob" ? "div" : "button");
  d.className = "hit" + (kind === "knob" ? " knob" : "");
  Object.assign(d.style, { left: `${b.x / 10}%`, top: `${b.y / 6.4}%`, width: `${b.width / 10}%`, height: `${b.height / 6.4}%` });
  if (kind !== "knob") d.type = "button";
  const own = document.createElement("span");
  own.id = `does-${nDesc++}`;
  own.textContent = does;
  descs.append(own);
  d.setAttribute("aria-label", name);
  if (kind === "knob" && c.role === "master") {
    Object.assign(d, { tabIndex: 0 });
    d.setAttribute("role", "slider");
    d.setAttribute("aria-valuemin", "0");
    d.setAttribute("aria-valuemax", "4096");
    d.setAttribute("aria-describedby", own.id);
  } else if (kind === "knob") {
    d.tabIndex = 0;
    d.setAttribute("role", "button");
    d.setAttribute("aria-describedby", `${own.id} how-enc`);
  } else {
    d.setAttribute("aria-describedby", `${own.id} how-${kind}`);
  }
  d.addEventListener("touchstart", (e) => e.preventDefault(), { passive: false });
  controls.append(d);
  c.g.setAttribute("aria-hidden","true");
  c.hit = d;
  c.name = name;
}
for (const n of ["MASTER", "SELECT", "PRESETS", "ALGORITHM"]) hit(knobs[n], "knob", n, DOES[n]);
for (const n of ["OCT-", "OCT+"]) hit(buttons[n], "btn", SPOKEN[n], DOES[n]);
for (let i = 1; i <= 4; i++) hit(knobs[`KNOB${i}`], "knob", `KNOB ${i}`, DOES.KNOB);
for (const n of [...ROW1, ...ROW2]) hit(buttons[n], "btn", n, DOES[n]);
for (const k of BLACK_K) hit(keys[k], "key", keys[k].label, keys[k].does);
for (const k of WHITE_K) hit(keys[k], "key", keys[k].label, keys[k].does);

// a control's name says its light: "PLAY, lit", "White key 5, dim"; a latched button "held down"
function nameOf(c) { return c.name + (c.lit === 2 ? ", lit" : c.lit === 1 ? ", dim" : "") + (c.latched ? ", held down" : ""); }
function setState(c, lit, latched = false) {
  if (c.lit === lit && !!c.latched === latched) return;
  c.lit = lit;
  c.latched = latched;
  c.hit.setAttribute("aria-label", nameOf(c));
}

// ---------------------------------------------------------------- state and input
let node = null, gain = null, btnMask = 0, keyMask = 0, latched = 0, master = 2800;
const btnHeldBy = new Map(), keyHeldBy = new Map();      // pointer / key id -> bit
let uiInputTicket = 0;
window.addEventListener("fm1-ready", () => { uiInputTicket = 0; });
function send(m) { if (node) { if (m.type === "buttons") { m = {...m, inputTicket: uiInputTicket = ((uiInputTicket + 1) >>> 0) || 1}; window.dispatchEvent(new CustomEvent("fm1-input", {detail:m})); } node.port.postMessage(m); } }
let litButtons = 0;
function syncButtons() {
  let m = latched;
  for (const b of btnHeldBy.values()) m |= 1 << b;
  if (m !== btnMask) { btnMask = m; send({ type: "buttons", mask: m }); }
  for (const b of Object.values(buttons)) {
    b.g.classList.toggle("held", !!(m & (1 << b.bit)));
    b.g.classList.toggle("latched", !!(latched & (1 << b.bit)));
    b.hit.classList.toggle("latched", !!(latched & (1 << b.bit)));
    setState(b, litButtons & (1 << b.bit) ? 2 : 0, !!(latched & (1 << b.bit)));
  }
}
function syncKeys() {
  let m = 0;
  for (const k of keyHeldBy.values()) m |= 1 << k;
  if (m !== keyMask) { keyMask = m; send({ type: "keys", mask: m }); }
  keys.forEach((k, i) => k.g.classList.toggle("held", !!(m & (1 << i))));
}
// a press from a screen reader or a click without a pointer (no keydown, no pointerdown): a short tap
function tap(map, id, bit, sync) {
  map.set(id, bit);
  sync();
  setTimeout(() => { map.delete(id); sync(); }, 120);
}
const pointerAt = { t: -1e9 }, keyAt = { t: -1e9 };

const buttonPresses = new Map(); // Preserve short clicks through the firmware input scan.
const keyPresses = new Map();
function releasePointerKey(id, press) {
  if (!press) return;
  if (keyPresses.get(id) === press) keyPresses.delete(id);
  // Each press owns a separate hold: an older timer cannot release a newer
  // press, and quick clicks on different keys with one mouse can overlap.
  if (keyHeldBy.delete(press)) syncKeys();
}
for (const b of Object.values(buttons)) {
  b.hit.addEventListener("contextmenu", (e) => e.preventDefault());
  b.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    b.hit.focus({ preventScroll: true });
    pointerAt.t = performance.now();
    shiftCancel();
    if (e.shiftKey || e.button === 2) { latched ^= 1 << b.bit; syncButtons(); return; }
    if (latched & (1 << b.bit)) { latched &= ~(1 << b.bit); syncButtons(); return; }
    b.hit.setPointerCapture(e.pointerId);
    buttonPresses.set("p" + e.pointerId, {at: performance.now()});
    btnHeldBy.set("p" + e.pointerId, b.bit);
    syncButtons();
  });
  b.hit.addEventListener("click", () => {
    const now = performance.now();
    if (now - pointerAt.t < 800 || now - keyAt.t < 800) return;
    if (latched & (1 << b.bit)) { latched &= ~(1 << b.bit); syncButtons(); return; }
    tap(btnHeldBy, "c" + b.bit, b.bit, syncButtons);
  });
}
keys.forEach((k, i) => {
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    k.hit.focus({ preventScroll: true });
    pointerAt.t = performance.now();
    k.hit.setPointerCapture(e.pointerId);
    const id = "p" + e.pointerId, old = keyPresses.get(id);
    if (old && !old.released) releasePointerKey(id, old);
    const press = {at: performance.now(), target: k.hit, released: false};
    keyPresses.set(id, press);
    keyHeldBy.set(press, i);
    syncKeys();
  });
  k.hit.addEventListener("click", () => {
    const now = performance.now();
    if (now - pointerAt.t < 800 || now - keyAt.t < 800) return;
    tap(keyHeldBy, "c" + i, i, syncKeys);
  });
});
function pointerUp(e) {
  const id = "p" + e.pointerId;
  const press=buttonPresses.get(id);
  const release=()=>{if(buttonPresses.get(id)!==press)return;buttonPresses.delete(id);if(btnHeldBy.delete(id))syncButtons();};
  if(press && e.type!=="pointercancel")setTimeout(release,Math.max(0,60-(performance.now()-press.at)));
  else release();
  const keyPress = keyPresses.get(id);
  if (!keyPress || keyPress.target !== e.target) return;
  if (e.type === "lostpointercapture" && keyPress.target.hasPointerCapture(e.pointerId)) return;
  if (e.type === "pointercancel") releasePointerKey(id, keyPress);
  else if (!keyPress.released) {
    keyPress.released = true;
    setTimeout(() => releasePointerKey(id, keyPress), Math.max(0, 60 - (performance.now() - keyPress.at)));
  }
}
device.addEventListener("pointerup", pointerUp);
device.addEventListener("pointercancel", pointerUp);
device.addEventListener("lostpointercapture", pointerUp);
device.addEventListener("pointerdown", (e) => { if (e.target === device || e.target === controls) device.focus({ preventScroll: true }); });

function drawKnob(k) {
  k.ind.setAttribute("transform", `rotate(${k.angle} ${k.x} ${k.y})`);
}
function syncMaster(state) {
  if(!state || !Number.isInteger(state.q12) || state.q12<0 || state.q12>4096 ||
     !Number.isInteger(state.owner) || state.owner<0 || state.owner>5 || typeof state.pickupPending!=="boolean")return;
  master=state.q12;const k=knobs.MASTER;k.angle=-135+270*master/4096;drawKnob(k);
  k.hit.setAttribute("aria-valuemin",0);k.hit.setAttribute("aria-valuemax",4096);masterValue(k);
}
function masterValue(k) {
  const pct = Math.round(master * 100 / 4096);
  k.hit.setAttribute("aria-valuenow", String(master));
  k.hit.setAttribute("aria-valuetext", `${pct} percent`);
}
function turn(k, n) {
  if (!n) return;
  if (k.role === "master") {
    if(!node){master=Math.max(0,Math.min(4096,master+n*64));k.angle=-135+270*master/4096;}
    else send({ type: "master-step", delta: Math.max(-4096,Math.min(4096,n*64)) });
  } else {
    k.angle = (k.angle + n * 18) % 360;
    send({ type: "enc", role: k.role, n });
  }
  drawKnob(k);
}
for (const k of Object.values(knobs)) {
  if (k.role === "master") { k.angle = -135 + 270 * master / 4096; drawKnob(k); masterValue(k); }
  let drag = null, wheel = 0;
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    k.hit.focus({ preventScroll: true });
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
    const n = { ArrowUp: 1, ArrowRight: 1, ArrowDown: -1, ArrowLeft: -1, PageUp: 8, PageDown: -8 }[e.key] ??
              (k.role === "master" ? { Home: -64, End: 64 }[e.key] : undefined);
    if (n === undefined) return;
    turn(k, n);
    e.preventDefault();
    e.stopPropagation();
  });
}

// keyboard: a focused button or key is pressed with Space or Enter; the playing shortcuts work only while the FM-1
// (the panel) has focus, and can be turned off. Shift is SEL, but only once something else is pressed with it or
// after a moment: Shift+Tab moves back without pressing SEL.
const KEYMAP = {};
"qwertyui".split("").forEach((c, i) => KEYMAP["Key" + c.toUpperCase()] = { key: WHITE_K[i] });
"asdfghjk".split("").forEach((c, i) => KEYMAP["Key" + c.toUpperCase()] = { key: WHITE_K[8 + i] });
"1234567890".split("").forEach((c, i) => KEYMAP["Digit" + c] = { key: BLACK_K[i] });
KEYMAP.Minus = { key: BLACK_K[10] };
Object.assign(KEYMAP, { Space: { btn: BTN.PLAY }, Enter: { btn: BTN.REC }, Escape: { btn: BTN.HOME }, KeyZ: { btn: BTN["OCT-"] }, KeyX: { btn: BTN["OCT+"] } });
const shortcuts = $("shortcuts");
if (prefs.shortcuts === false) shortcuts.checked = false;
shortcuts.addEventListener("change", () => { prefs.shortcuts = shortcuts.checked; savePrefs(); });
const inPanel = () => device.contains(document.activeElement);
let shift = null;                                  // {code, timer, down}
function shiftCommit() {
  if (!shift || shift.down) return;
  clearTimeout(shift.timer);
  shift.down = true;
  btnHeldBy.set("k" + shift.code, BTN.SEL);
  syncButtons();
}
function shiftCancel() {
  if (!shift) return;
  clearTimeout(shift.timer);
  if (shift.down && btnHeldBy.delete("k" + shift.code)) syncButtons();
  shift = null;
}
function controlOf(t) {
  for (const b of Object.values(buttons)) if (b.hit === t) return { btn: b.bit };
  const i = keys.findIndex((k) => k && k.hit === t);
  return i >= 0 ? { key: i } : null;
}
addEventListener("keydown", (e) => {
  if (!node || e.metaKey || e.ctrlKey || e.altKey || !inPanel()) return;
  if (e.target.closest?.("input, textarea, select, [contenteditable], .variant-lab, .arrangement-lab, .slice-lab, .breaks")) return;
  if (e.key === "Tab") { shiftCancel(); return; }
  const own = (e.code === "Space" || e.code === "Enter" || e.code === "NumpadEnter") ? controlOf(e.target) : null;
  if (own) {                                       // the focused button or key
    e.preventDefault();
    keyAt.t = performance.now();
    if (e.repeat) return;
    if (own.btn !== undefined && e.shiftKey) {       // Shift+Space: held down until pressed again
      if (shift && !shift.down) { clearTimeout(shift.timer); shift = null; }
      latched ^= 1 << own.btn;
      syncButtons();
      return;
    }
    shiftCommit();
    downAt.set("k" + e.code, {at:performance.now()});
    if (own.key !== undefined) { keyHeldBy.set("k" + e.code, own.key); syncKeys(); }
    else if (latched & (1 << own.btn)) { latched &= ~(1 << own.btn); syncButtons(); }
    else { btnHeldBy.set("k" + e.code, own.btn); syncButtons(); }
    return;
  }
  if (!shortcuts.checked) return;
  if (e.code === "ShiftLeft" || e.code === "ShiftRight") {
    if (!e.repeat && !shift) shift = { code: e.code, timer: setTimeout(shiftCommit, 300), down: false };
    return;
  }
  if (e.code === "ArrowUp" || e.code === "ArrowDown") { shiftCommit(); turn(knobs.SELECT, e.code === "ArrowUp" ? 1 : -1); e.preventDefault(); return; }
  const m = KEYMAP[e.code];
  if (!m) return;
  e.preventDefault();
  if (e.repeat) return;
  shiftCommit();
  downAt.set("k" + e.code, {at:performance.now()});
  if (m.key !== undefined) { keyHeldBy.set("k" + e.code, m.key); syncKeys(); }
  else { btnHeldBy.set("k" + e.code, m.btn); syncButtons(); }
});
// a key let go at once (a screen reader's, or a fast typist's) is still held for 60 ms: the device reads its
// inputs between audio blocks, and a press shorter than that would go unseen
const downAt = new Map();
function letGo(id) {
  const press=downAt.get(id);
  const wait = Math.max(0, 60 - (performance.now() - (press?.at || 0)));
  setTimeout(() => {
    if(downAt.get(id)!==press)return;
    downAt.delete(id);
    if (keyHeldBy.delete(id)) syncKeys();
    if (btnHeldBy.delete(id)) syncButtons();
  }, wait);
}
addEventListener("keyup", (e) => {
  if (shift && e.code === shift.code) {
    if (!shift.down) {                              // Shift on its own, quickly: a SEL press
      clearTimeout(shift.timer);
      shift = null;
      tap(btnHeldBy, "kshift", BTN.SEL, syncButtons);
    } else shiftCancel();
    return;
  }
  if (keyHeldBy.has("k" + e.code) || btnHeldBy.has("k" + e.code)) letGo("k" + e.code);
  if (e.code === "Space" && controlOf(e.target)) e.preventDefault();
});
function letGoKeys() {
  if (shift) { clearTimeout(shift.timer); shift = null; }
  for (const id of [...btnHeldBy.keys()]) if (id[0] === "k") btnHeldBy.delete(id);
  for (const id of [...keyHeldBy.keys()]) if (id[0] === "k") keyHeldBy.delete(id);
  downAt.clear();
  syncButtons(); syncKeys();
}
addEventListener("blur", () => {
  letGoKeys();buttonPresses.clear();keyPresses.clear();
  btnHeldBy.clear();keyHeldBy.clear();syncButtons();syncKeys();
});
device.addEventListener("focusout", (e) => { if (!device.contains(e.relatedTarget)) letGoKeys(); });

// ---------------------------------------------------------------- screen and lights
const ctx = $("screen").getContext("2d");
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
  litButtons = b;
  for (const x of Object.values(buttons)) {
    x.g.classList.toggle("lit", !!(b & (1 << x.bit)));
    setState(x, b & (1 << x.bit) ? 2 : 0, !!(latched & (1 << x.bit)));
  }
  keys.forEach((x, i) => {
    const lit = !!(k & (1 << i)), dim = !lit && !!(d & (1 << i));
    x.g.classList.toggle("lit", lit);
    x.g.classList.toggle("dim", dim);
    setState(x, lit ? 2 : dim ? 1 : 0);
  });
}

// ---------------------------------------------------------------- the screen reader (fm1-reader's PROTOCOL.md)
// The page is the reader: HELLO every 2 s while someone reads, and the device's SAY frames in its own words.
const SR = [0x7D, 0x53, 0x52], R_HELLO = 1, R_BYE = 2, R_WHERE = 3, R_SAY = 16;
const rmode = () => document.querySelector('input[name="rmode"]:checked').value;
if (prefs.rmode) { const r = document.querySelector(`input[name="rmode"][value="${prefs.rmode}"]`); if (r) r.checked = true; }
let words = null, wordsReady = null, helloTimer = 0, saidAny = false, lastScreen = "", lastFlags = 0, liveFlip = false, tts = null, ttsLoad = null;
const sendReader = (...b) => send({ type: "sysex", bytes: [...SR, ...b] });
async function loadWords(link) {
  // the words file the device names, from this site first (../words/), else from its link
  const name = (link.match(/[^/]+\.json$/) || [""])[0];
  if (localReaderWords[name]) return wordsOf(localReaderWords[name]);
  for (const url of [name && new URL(`../words/${name}`, location.href).href, link].filter(Boolean)) {
    try {
      const r = await fetch(url);
      if (r.ok) return wordsOf(await r.json());
    } catch { /* the next */ }
  }
  return [];                                       // the screen's text, read as it is (the common rules only)
}
function readerOn() {
  const on = !!node && rmode() !== "off";
  $("where").disabled = $("again").disabled = !on;
  clearInterval(helloTimer);
  if (!on) { if (node) sendReader(R_BYE); saidAny = false; return; }
  sendReader(R_HELLO, 1);
  helloTimer = setInterval(() => sendReader(R_HELLO, 1), 2000);
}
function speak(screen, flags) {
  const now = !!(flags & 1), keep = !!(flags & 2), said = spoken(screen, words || []);
  lastScreen = screen;
  lastFlags = flags;
  $("now").textContent = said;
  if (rmode() === "voice") {
    if (tts) tts.speak(said, { rate: 260, interrupt: now && !keep });
  } else if (rmode() === "sr") {
    const live = now || keep ? $("live-a") : $("live-p");
    liveFlip = !liveFlip;                            // the same words twice: some screen readers skip them otherwise
    live.textContent = said + (liveFlip ? "" : " ");
  }
}
function heard(bytes) {
  for (let i = 0; i < bytes.length; i++) {
    if (bytes[i] !== 0xF0) continue;
    let j = i + 1;
    while (j < bytes.length && bytes[j] !== 0xF7) j++;
    const f = bytes.subarray(i, j + 1);
    i = j;
    if (f.length < 6 || f[1] !== SR[0] || f[2] !== SR[1] || f[3] !== SR[2]) continue;
    const body = String.fromCharCode(...f.subarray(6, f.length - 1).filter((c) => c < 128));
    if (f[4] === R_HELLO && !wordsReady) {
      const link = body.split("\0")[1] || "";
      wordsReady = loadWords(link).then((w) => { words = w; });
      setTimeout(() => { if (!saidAny && rmode() !== "off") sendReader(R_WHERE); }, 400);
    } else if (f[4] === R_SAY && rmode() !== "off") {
      saidAny = true;
      const flags = f[5], text = body.replace(/\0/g, "");
      (wordsReady || Promise.resolve()).then(() => speak(text, flags));
    }
  }
}
async function useVoice() {
  if (!ttsLoad) {
    ttsLoad = import("./reader/tts.js").then((m) => m.createTTS(new URL("reader/", location.href).href)).then((t) => (tts = t));
  }
  try {
    await ttsLoad;
    tts.unlock();
  } catch {
    // Portable packages can use the browser's voice without optional reader assets.
    if (globalThis.speechSynthesis && globalThis.SpeechSynthesisUtterance) {
      tts = {unlock(){},stop(){speechSynthesis.cancel();},speak(text, options={}){
        if(options.interrupt)speechSynthesis.cancel();
        const utterance=new SpeechSynthesisUtterance(text);
        utterance.rate=Math.max(0.5,Math.min(2,(options.rate||180)/180));
        speechSynthesis.speak(utterance);
      }};
      ttsLoad=Promise.resolve(tts);return;
    }
    ttsLoad = null;
    document.querySelector('input[name="rmode"][value="sr"]').checked = true;
    prefs.rmode = "sr"; savePrefs();
    $("alert").textContent = "The page's voice could not load. Your own screen reader reads the screen.";
  }
}
document.querySelectorAll('input[name="rmode"]').forEach((r) => r.addEventListener("change", () => {
  prefs.rmode = rmode(); savePrefs();
  if (rmode() !== "voice" && tts) tts.stop();
  if (rmode() === "voice") useVoice();
  readerOn();
}));
$("where").addEventListener("click", () => sendReader(R_WHERE));
$("again").addEventListener("click", () => { if (lastScreen) speak(lastScreen, lastFlags | 1); });

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
    return true;
  } catch { return false; /* storage unavailable: this session only */ }
}

// ---------------------------------------------------------------- your breaks
// A file becomes mono 22050 Hz 16-bit here (the browser decodes and resamples it), peak 0.95 as
// tools/import_breaks.py leaves it, and goes to the worklet, which IMA-encodes it into the slot as
// an upload to the FM-1 would. Slots are kept in IndexedDB (they are too big for localStorage).
const SLOT_RATE = 22050, SLOT_MAX = 2 * (0x14000 - 512), SLOT_ZONES = 16;
const slots = [null, null, null];                 // {name, zones: [{pcm: Int16Array, rate}]}
function idb() {
  return new Promise((res, rej) => {
    const r = indexedDB.open("acultur8-fm1-beta-0127-loops-v1", 1);
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
  const box = $("slots");
  for (let k = 0; k < 3; k++) {
    const row = document.createElement("div");
    row.className = "slot";
    row.innerHTML = `<b>Slot ${k + 1}</b><span class="sname" aria-live="polite">Empty</span>` +
      `<button type="button" class="pick" aria-label="Choose files for slot ${k + 1}">Choose files</button>` +
      `<input type="file" multiple hidden aria-label="Files for slot ${k + 1}" accept="audio/*,.wav,.aif,.aiff,.mp3,.flac">` +
      `<button type="button" class="clear" aria-label="Clear slot ${k + 1}">Clear</button>`;
    const input = row.querySelector("input");
    row.querySelector(".pick").addEventListener("click", () => input.click());
    input.addEventListener("change", (e) => { slotFill(k, e.target.files); e.target.value = ""; });
    row.querySelector(".clear").addEventListener("click", () => { slots[k] = null; slotDraw(k); slotSend(k); slotSave(k); });
    row.addEventListener("dragover", (e) => { e.preventDefault(); row.classList.add("over"); });
    row.addEventListener("dragleave", () => row.classList.remove("over"));
    row.addEventListener("drop", (e) => { e.preventDefault(); row.classList.remove("over"); slotFill(k, e.dataTransfer.files); });
    box.appendChild(row);
  }
  slotsLoad().then(() => [0, 1, 2].forEach((k) => slotDraw(k)));
})();

// ---------------------------------------------------------------- switching on
const status = $("status"), cpu = $("cpu");
let muted = true;
$("mute").addEventListener("click", () => {
  muted = !muted;
  $("mute").setAttribute("aria-pressed", String(muted));
  $("mute").textContent = muted ? "Unmute" : "Mute";
  if (gain) gain.gain.value = muted ? 0 : 1;
});
async function switchOn() {
  const go = $("go");
  go.disabled = true;
  go.textContent = "Starting...";
  $("alert").textContent = "";
  try {
    const ac = new AudioContext({ sampleRate: 44100, latencyHint: "interactive" });
    const [wasm] = await Promise.all([fetch("x0x.wasm").then((r) => { if (!r.ok) throw new Error(`x0x.wasm: ${r.status}`); return r.arrayBuffer(); }),
                                      ac.audioWorklet.addModule("worklet.js")]);
    node = new AudioWorkletNode(ac, "x0x", { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
    gain = ac.createGain();
    gain.gain.value = muted ? 0 : 1;
    node.connect(gain).connect(ac.destination);
    const ready = new Promise((res) => {
      node.port.onmessage = (e) => {
        const m = e.data;
        window.dispatchEvent(new CustomEvent("fm1-marked", {detail:m}));
        if (m.type === "ready") res();
        else if (m.type === "said") heard(m.bytes);
        else if (m.type === "frame") {
          syncMaster(m.master);
          if (m.fb) drawScreen(m.fb);
          drawLights(m.buttons, m.keys, m.dim);
          window.x0xLast = m;
          if (m.load !== undefined) cpu.textContent = `X0X uses ${m.load}% of this computer's time`;
          if (m.store) { const saved=saveStore(m.store); window.dispatchEvent(new CustomEvent("fm1-store", {detail:{saved,store:m.store}})); }
        }
      };
    });
    node.port.postMessage({ type: "load", wasm, store: loadStore(), master }, [wasm]);
    await ready;
    [0, 1, 2].forEach((k) => { if (slots[k]) slotSend(k); });   // the loops kept in this browser
    await ac.resume();
    $("start").hidden = true;
    controls.inert = false;
    device.focus({ preventScroll: true });          // the Switch on button is gone: the FM-1 has the focus
    status.textContent = ac.sampleRate === 44100 ? "On." : `On (the browser runs audio at ${ac.sampleRate} Hz).`;
    syncButtons(); syncKeys();
    window.dispatchEvent(new Event("fm1-ready"));
    if (rmode() === "voice") useVoice();
    readerOn();
  } catch (err) {
    go.disabled = false;
    go.textContent = "Switch on";
    $("alert").textContent = "Could not start: " + (err && err.message || err) + ". Chrome, Edge, Firefox and Safari should all work.";
  }
}
$("go").addEventListener("click", switchOn);
addEventListener("pagehide", () => { if (node && rmode() !== "off") sendReader(R_BYE); });
$("reset").addEventListener("click", () => {
  if ($("reset").dataset.armed) {
    try { localStorage.removeItem(STORE_KEY); } catch {}
    location.reload();
  } else {
    $("reset").dataset.armed = "1";
    $("reset").textContent = "Click again to forget everything";
  }
});

window.fm1SliceSend = send;
