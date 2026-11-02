# Complete CC / MIDI map — 0.1.60-stoppedtrace

[Documentation](README.md) · [FAQ](FAQ.md) · [Comparison](FEATURE_COMPARISON.md) · [MIDI tutorial](tutorials/04_MIDI_CC.md)

**All 803 inventory rows**, map `draft-2-kick-presets-1`, kick ABI KP01. Channels below are numbered **1–16**. Parameter numbers and CC numbers are different namespaces. Numeric assignments are unchanged by this documentation update.

[Searchable HTML](CC_MIDI_MAP.html) · [Full-field CSV](CC_MIDI_MAP.csv) · [Original objects + runtime coverage JSON](CC_MIDI_MAP.json).

The runtime column retains source-owned status labels. “Native candidate; focused acceptance required” is not a promise of complete physical/live DAW acceptance. Unmapped/native-only rows have no direct CC address. Browser-only/planned entries stay labelled. `live_requirement` in CSV/JSON is a design requirement, not blanket permission to send guarded actions while playing. Optional CC14 entries are proposals where marked.

## Read the map before sending

- Back up your project, start stopped with MASTER down, and select the native GLO CC MODE deliberately.
- Musical destinations and structured transactions use explicit source-owned context; they do not follow whatever page happens to be visible.
- Raw CC is scaled to the stored range. Exact-value transactions use the existing source-owned protocol; do not assume displayed milliseconds are raw CC values.
- Preset STORE/RECALL are stopped-only, guarded actions. They need the committed 808 BD context, destination slot, request token, arm and execute sequence. See [kick protocol](KICK_PRESETS_0.1.36.md) for the retained ABI and [current MIDI tutorial](tutorials/04_MIDI_CC.md).
- Note input, MIDI clock and CC control are separate. Existing default drum-note routing can be changed by the project; check configured channels before testing.

## Full inventory

| ID | Name | Parameter | Channel | CC | Stored range | Context / guard | Runtime binding and status |
| --- | --- | ---: | ---: | ---: | --- | --- | --- |
| sound.909.BD.0 | Tune | 0 | 1 | 0 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.1 | Attack | 1 | 1 | 1 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.2 | Decay | 2 | 1 | 2 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.3 | Level | 3 | 1 | 3 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.4 | P.Dpth | 4 | 1 | 4 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.5 | Pitch | 5 | 1 | 5 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.6 | Drive | 6 | 1 | 6 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.7 | Dist | 7 | 1 | 7 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.8 | Rev | 8 | 1 | 8 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.9 | Dly | 9 | 1 | 9 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.BD.10 | Pan | 10 | 1 | 10 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.0 | Tune | 16 | 1 | 16 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.1 | Tone | 17 | 1 | 17 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.2 | Snappy | 18 | 1 | 18 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.3 | Decay | 19 | 1 | 19 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.4 | Level | 20 | 1 | 20 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.5 | Drive | 21 | 1 | 21 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.6 | Dist | 22 | 1 | 22 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.7 | Rev | 23 | 1 | 23 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.8 | Dly | 24 | 1 | 24 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.SD.9 | Pan | 25 | 1 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.0 | Tune | 32 | 1 | 64 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.1 | Decay | 33 | 1 | 65 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.2 | Level | 34 | 1 | 66 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.3 | Attack | 35 | 1 | 67 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.4 | Drive | 36 | 1 | 68 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.5 | Dist | 37 | 1 | 69 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.6 | Rev | 38 | 1 | 70 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.7 | Dly | 39 | 1 | 71 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.LT.8 | Pan | 40 | 1 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.0 | Tune | 48 | 1 | 80 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.1 | Decay | 49 | 1 | 81 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.2 | Level | 50 | 1 | 82 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.3 | Attack | 51 | 1 | 83 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.4 | Drive | 52 | 1 | 84 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.5 | Dist | 53 | 1 | 85 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.6 | Rev | 54 | 1 | 86 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.7 | Dly | 55 | 1 | 87 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.MT.8 | Pan | 56 | 1 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.0 | Tune | 64 | 1 | 96 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.1 | Decay | 65 | 1 | 97 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.2 | Level | 66 | 1 | 98 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.3 | Attack | 67 | 1 | 99 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.4 | Drive | 68 | 1 | 100 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.5 | Dist | 69 | 1 | 101 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.6 | Rev | 70 | 1 | 102 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.7 | Dly | 71 | 1 | 103 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.HT.8 | Pan | 72 | 1 | 104 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.0 | Level | 80 | 1 | 112 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.1 | Tune | 81 | 1 | 113 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.2 | Drive | 82 | 1 | 114 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.3 | Dist | 83 | 1 | 115 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.4 | Rev | 84 | 1 | 116 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.5 | Dly | 85 | 1 | 117 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RS.6 | Pan | 86 | 1 | 118 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.0 | Level | 96 | 2 | 8 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.1 | Tune | 97 | 2 | 9 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.2 | Tail | 98 | 2 | 10 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.3 | Drive | 99 | 2 | 11 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.4 | Dist | 100 | 2 | 12 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.5 | Rev | 101 | 2 | 13 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.6 | Dly | 102 | 2 | 14 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CP.7 | Pan | 103 | 2 | 15 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.0 | Decay | 112 | 2 | 24 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.1 | Level | 113 | 2 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.2 | Tune | 114 | 2 | 26 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.3 | Drive | 115 | 2 | 27 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.4 | Dist | 116 | 2 | 28 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.5 | Rev | 117 | 2 | 29 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.6 | Dly | 118 | 2 | 30 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CH.7 | Pan | 119 | 2 | 31 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.0 | Decay | 128 | 2 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.1 | Level | 129 | 2 | 73 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.2 | Tune | 130 | 2 | 74 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.3 | Drive | 131 | 2 | 75 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.4 | Dist | 132 | 2 | 76 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.5 | Rev | 133 | 2 | 77 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.6 | Dly | 134 | 2 | 78 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.OH.7 | Pan | 135 | 2 | 79 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.0 | Tune | 144 | 2 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.1 | Level | 145 | 2 | 89 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.2 | Decay | 146 | 2 | 90 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.3 | Drive | 147 | 2 | 91 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.4 | Dist | 148 | 2 | 92 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.5 | Rev | 149 | 2 | 93 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.6 | Dly | 150 | 2 | 94 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.CR.7 | Pan | 151 | 2 | 95 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.0 | Tune | 160 | 2 | 104 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.1 | Level | 161 | 2 | 105 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.2 | Decay | 162 | 2 | 106 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.3 | Drive | 163 | 2 | 107 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.4 | Dist | 164 | 2 | 108 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.5 | Rev | 165 | 2 | 109 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.6 | Dly | 166 | 2 | 110 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.RD.7 | Pan | 167 | 2 | 111 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.KIT.0 | Accent | 176 | 3 | 0 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.909.KIT.1 | Veloc | 177 | 3 | 1 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.0 | Level | 192 | 3 | 16 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.1 | Tone | 193 | 3 | 17 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.2 | Decay | 194 | 3 | 18 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.3 | Tune | 195 | 3 | 19 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.4 | Attack | 196 | 3 | 20 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.5 | Drive | 197 | 3 | 21 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.6 | Dist | 198 | 3 | 22 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.7 | Rev | 199 | 3 | 23 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.8 | Dly | 200 | 3 | 24 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.BD.9 | Pan | 201 | 3 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.0 | Level | 208 | 3 | 64 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.1 | Tone | 209 | 3 | 65 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.2 | Snappy | 210 | 3 | 66 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.3 | Tune | 211 | 3 | 67 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.4 | Decay | 212 | 3 | 68 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.5 | Drive | 213 | 3 | 69 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.6 | Dist | 214 | 3 | 70 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.7 | Rev | 215 | 3 | 71 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.8 | Dly | 216 | 3 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.SD.9 | Pan | 217 | 3 | 73 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.0 | Level | 224 | 3 | 80 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.1 | Tune | 225 | 3 | 81 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.2 | Decay | 226 | 3 | 82 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.3 | Sound | 227 | 3 | 83 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.4 | Drive | 228 | 3 | 84 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.5 | Dist | 229 | 3 | 85 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.6 | Rev | 230 | 3 | 86 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.7 | Dly | 231 | 3 | 87 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.LT.8 | Pan | 232 | 3 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.0 | Level | 240 | 3 | 96 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.1 | Tune | 241 | 3 | 97 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.2 | Decay | 242 | 3 | 98 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.3 | Sound | 243 | 3 | 99 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.4 | Drive | 244 | 3 | 100 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.5 | Dist | 245 | 3 | 101 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.6 | Rev | 246 | 3 | 102 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.7 | Dly | 247 | 3 | 103 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.MT.8 | Pan | 248 | 3 | 104 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.0 | Level | 256 | 3 | 112 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.1 | Tune | 257 | 3 | 113 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.2 | Decay | 258 | 3 | 114 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.3 | Sound | 259 | 3 | 115 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.4 | Drive | 260 | 3 | 116 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.5 | Dist | 261 | 3 | 117 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.6 | Rev | 262 | 3 | 118 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.7 | Dly | 263 | 3 | 119 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.HT.8 | Pan | 264 | 4 | 0 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.0 | Level | 272 | 4 | 8 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.1 | Tune | 273 | 4 | 9 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.2 | Decay | 274 | 4 | 10 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.3 | Sound | 275 | 4 | 11 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.4 | Drive | 276 | 4 | 12 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.5 | Dist | 277 | 4 | 13 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.6 | Rev | 278 | 4 | 14 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.7 | Dly | 279 | 4 | 15 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.RS.8 | Pan | 280 | 4 | 16 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.0 | Level | 288 | 4 | 24 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.1 | Tune | 289 | 4 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.2 | Decay | 290 | 4 | 26 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.3 | Attack | 291 | 4 | 27 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.4 | Sound | 292 | 4 | 28 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.5 | Drive | 293 | 4 | 29 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.6 | Dist | 294 | 4 | 30 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.7 | Rev | 295 | 4 | 31 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.8 | Dly | 296 | 4 | 64 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CP.9 | Pan | 297 | 4 | 65 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.0 | Level | 304 | 4 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.1 | Tune | 305 | 4 | 73 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.2 | Decay | 306 | 4 | 74 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.3 | Drive | 307 | 4 | 75 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.4 | Dist | 308 | 4 | 76 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.5 | Rev | 309 | 4 | 77 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.6 | Dly | 310 | 4 | 78 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CB.7 | Pan | 311 | 4 | 79 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.0 | Level | 320 | 4 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.1 | Decay | 321 | 4 | 89 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.2 | Tune | 322 | 4 | 90 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.3 | Drive | 323 | 4 | 91 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.4 | Dist | 324 | 4 | 92 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.5 | Rev | 325 | 4 | 93 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.6 | Dly | 326 | 4 | 94 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CY.7 | Pan | 327 | 4 | 95 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.0 | Level | 336 | 4 | 104 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.1 | Decay | 337 | 4 | 105 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.2 | Tune | 338 | 4 | 106 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.3 | Drive | 339 | 4 | 107 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.4 | Dist | 340 | 4 | 108 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.5 | Rev | 341 | 4 | 109 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.6 | Dly | 342 | 4 | 110 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.OH.7 | Pan | 343 | 4 | 111 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.0 | Level | 352 | 5 | 0 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.1 | Tune | 353 | 5 | 1 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.2 | Decay | 354 | 5 | 2 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.3 | Drive | 355 | 5 | 3 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.4 | Dist | 356 | 5 | 4 | [0, 6] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.5 | Rev | 357 | 5 | 5 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.6 | Dly | 358 | 5 | 6 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.CH.7 | Pan | 359 | 5 | 7 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.KIT.0 | Level | 368 | 5 | 16 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.808.KIT.1 | Accent | 369 | 5 | 17 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.0 | Cutoff | 384 | 5 | 64 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.1 | Reso | 385 | 5 | 65 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.2 | EnvMod | 386 | 5 | 66 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.3 | Decay | 387 | 5 | 67 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.4 | Accent | 388 | 5 | 68 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.5 | Wave | 389 | 5 | 69 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.6 | Tune | 390 | 5 | 70 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.7 | Volume | 391 | 5 | 71 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.8 | Drive | 392 | 5 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.9 | DrvTyp | 393 | 5 | 73 | [0, 2] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.10 | Slide | 394 | 5 | 74 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.A.11 | AccDec | 395 | 5 | 75 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.0 | Cutoff | 400 | 5 | 80 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.1 | Reso | 401 | 5 | 81 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.2 | EnvMod | 402 | 5 | 82 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.3 | Decay | 403 | 5 | 83 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.4 | Accent | 404 | 5 | 84 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.5 | Wave | 405 | 5 | 85 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.6 | Tune | 406 | 5 | 86 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.7 | Volume | 407 | 5 | 87 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.8 | Drive | 408 | 5 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.9 | DrvTyp | 409 | 5 | 89 | [0, 2] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.10 | Slide | 410 | 5 | 90 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.303.B.11 | AccDec | 411 | 5 | 91 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.BREAK.GLOBAL.0 | Level | 416 | 5 | 96 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.BREAK.GLOBAL.1 | Pitch | 417 | 5 | 97 | [0, 24] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.0 | Volume | 432 | 5 | 112 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.1 | Dist | 433 | 5 | 113 | [0, 7] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.2 | Drive | 434 | 5 | 114 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.3 | Comp | 435 | 5 | 115 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.4 | RvDec | 436 | 5 | 116 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.5 | RvTone | 437 | 5 | 117 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.6 | RvHPF | 438 | 5 | 118 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.7 | RvLvl | 439 | 5 | 119 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.8 | DlTime | 440 | 6 | 0 | [0, 12] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.9 | DlFdbk | 441 | 6 | 1 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.10 | DlTone | 442 | 6 | 2 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.11 | DlLvl | 443 | 6 | 3 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.12 | DlHPF | 444 | 6 | 4 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.13 | DlType | 445 | 6 | 5 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.14 | Wear | 446 | 6 | 6 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.FX.GLOBAL.15 | Ping | 447 | 6 | 7 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.909.0 | LEVEL | 464 | 6 | 24 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.909.1 | REV | 465 | 6 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.909.2 | DLY | 466 | 6 | 26 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.909.3 | PAN | 467 | 6 | 27 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.808.0 | LEVEL | 480 | 6 | 72 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.808.1 | REV | 481 | 6 | 73 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.808.2 | DLY | 482 | 6 | 74 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.808.3 | PAN | 483 | 6 | 75 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303A.0 | LEVEL | 496 | 6 | 88 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303A.1 | REV | 497 | 6 | 89 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303A.2 | DLY | 498 | 6 | 90 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303A.3 | PAN | 499 | 6 | 91 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303B.0 | LEVEL | 512 | 6 | 104 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303B.1 | REV | 513 | 6 | 105 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303B.2 | DLY | 514 | 6 | 106 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.303B.3 | PAN | 515 | 6 | 107 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.BREAK.0 | LEVEL | 528 | 7 | 0 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.BREAK.1 | REV | 529 | 7 | 1 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.BREAK.2 | DLY | 530 | 7 | 2 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MIX.BREAK.3 | PAN | 531 | 7 | 3 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.0 | THRESH | 544 | 7 | 16 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.1 | RATIO | 545 | 7 | 17 | [0, 9] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.2 | ATTACK | 546 | 7 | 18 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.3 | RELEAS | 547 | 7 | 19 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.4 | MAKEUP | 548 | 7 | 20 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.5 | MIX | 549 | 7 | 21 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.6 | PUMP | 550 | 7 | 22 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.7 | PUMP BY | 551 | 7 | 23 | [0, 2] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.8 | FILTER | 552 | 7 | 24 | [0, 3] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.9 | CUTOFF | 553 | 7 | 25 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.10 | RESO | 554 | 7 | 26 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.11 | LIMIT | 555 | 7 | 27 | [0, 1] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.12 | COMP | 556 | 7 | 28 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| sound.MASTER.GLOBAL.13 | FILTER | 557 | 7 | 29 | [0, 127] | {} | SOUND · native candidate; focused acceptance required |
| strip.0.highpass | highpass | 576 | 7 | 80 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.lowpass | lowpass | 577 | 7 | 81 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.boost | boost | 578 | 7 | 82 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.ceiling | ceiling | 579 | 7 | 83 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.release | release | 580 | 7 | 84 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.flags | flags | 581 | 7 | 85 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.enabled | enabled | 582 | 7 | 86 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.highpass_enabled | highpass_enabled | 583 | 7 | 87 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.lowpass_enabled | lowpass_enabled | 584 | 7 | 88 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.0.limiter_enabled | limiter_enabled | 585 | 7 | 89 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.highpass | highpass | 588 | 7 | 92 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.lowpass | lowpass | 589 | 7 | 93 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.boost | boost | 590 | 7 | 94 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.ceiling | ceiling | 591 | 7 | 95 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.release | release | 592 | 7 | 96 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.flags | flags | 593 | 7 | 97 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.enabled | enabled | 594 | 7 | 98 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.highpass_enabled | highpass_enabled | 595 | 7 | 99 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.lowpass_enabled | lowpass_enabled | 596 | 7 | 100 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.1.limiter_enabled | limiter_enabled | 597 | 7 | 101 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.highpass | highpass | 600 | 7 | 104 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.lowpass | lowpass | 601 | 7 | 105 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.boost | boost | 602 | 7 | 106 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.ceiling | ceiling | 603 | 7 | 107 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.release | release | 604 | 7 | 108 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.flags | flags | 605 | 7 | 109 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.enabled | enabled | 606 | 7 | 110 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.highpass_enabled | highpass_enabled | 607 | 7 | 111 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.lowpass_enabled | lowpass_enabled | 608 | 7 | 112 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.2.limiter_enabled | limiter_enabled | 609 | 7 | 113 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.highpass | highpass | 612 | 7 | 116 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.lowpass | lowpass | 613 | 7 | 117 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.boost | boost | 614 | 7 | 118 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.ceiling | ceiling | 615 | 7 | 119 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.release | release | 616 | 8 | 0 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.flags | flags | 617 | 8 | 1 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.enabled | enabled | 618 | 8 | 2 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.highpass_enabled | highpass_enabled | 619 | 8 | 3 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.lowpass_enabled | lowpass_enabled | 620 | 8 | 4 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.3.limiter_enabled | limiter_enabled | 621 | 8 | 5 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.highpass | highpass | 624 | 8 | 8 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.lowpass | lowpass | 625 | 8 | 9 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.boost | boost | 626 | 8 | 10 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.ceiling | ceiling | 627 | 8 | 11 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.release | release | 628 | 8 | 12 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.flags | flags | 629 | 8 | 13 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.enabled | enabled | 630 | 8 | 14 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.highpass_enabled | highpass_enabled | 631 | 8 | 15 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.lowpass_enabled | lowpass_enabled | 632 | 8 | 16 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.4.limiter_enabled | limiter_enabled | 633 | 8 | 17 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.highpass | highpass | 636 | 8 | 20 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.lowpass | lowpass | 637 | 8 | 21 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.boost | boost | 638 | 8 | 22 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.ceiling | ceiling | 639 | 8 | 23 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.release | release | 640 | 8 | 24 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.flags | flags | 641 | 8 | 25 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.enabled | enabled | 642 | 8 | 26 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.highpass_enabled | highpass_enabled | 643 | 8 | 27 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.lowpass_enabled | lowpass_enabled | 644 | 8 | 28 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.5.limiter_enabled | limiter_enabled | 645 | 8 | 29 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.highpass | highpass | 648 | 8 | 64 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.lowpass | lowpass | 649 | 8 | 65 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.boost | boost | 650 | 8 | 66 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.ceiling | ceiling | 651 | 8 | 67 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.release | release | 652 | 8 | 68 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.flags | flags | 653 | 8 | 69 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.enabled | enabled | 654 | 8 | 70 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.highpass_enabled | highpass_enabled | 655 | 8 | 71 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.lowpass_enabled | lowpass_enabled | 656 | 8 | 72 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.6.limiter_enabled | limiter_enabled | 657 | 8 | 73 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.highpass | highpass | 660 | 8 | 76 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.lowpass | lowpass | 661 | 8 | 77 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.boost | boost | 662 | 8 | 78 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.ceiling | ceiling | 663 | 8 | 79 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.release | release | 664 | 8 | 80 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.flags | flags | 665 | 8 | 81 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.enabled | enabled | 666 | 8 | 82 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.highpass_enabled | highpass_enabled | 667 | 8 | 83 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.lowpass_enabled | lowpass_enabled | 668 | 8 | 84 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.7.limiter_enabled | limiter_enabled | 669 | 8 | 85 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.highpass | highpass | 672 | 8 | 88 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.lowpass | lowpass | 673 | 8 | 89 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.boost | boost | 674 | 8 | 90 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.ceiling | ceiling | 675 | 8 | 91 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.release | release | 676 | 8 | 92 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.flags | flags | 677 | 8 | 93 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.enabled | enabled | 678 | 8 | 94 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.highpass_enabled | highpass_enabled | 679 | 8 | 95 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.lowpass_enabled | lowpass_enabled | 680 | 8 | 96 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.8.limiter_enabled | limiter_enabled | 681 | 8 | 97 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.highpass | highpass | 684 | 8 | 100 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.lowpass | lowpass | 685 | 8 | 101 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.boost | boost | 686 | 8 | 102 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.ceiling | ceiling | 687 | 8 | 103 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.release | release | 688 | 8 | 104 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.flags | flags | 689 | 8 | 105 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.enabled | enabled | 690 | 8 | 106 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.highpass_enabled | highpass_enabled | 691 | 8 | 107 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.lowpass_enabled | lowpass_enabled | 692 | 8 | 108 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.9.limiter_enabled | limiter_enabled | 693 | 8 | 109 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.highpass | highpass | 696 | 8 | 112 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.lowpass | lowpass | 697 | 8 | 113 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.boost | boost | 698 | 8 | 114 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.ceiling | ceiling | 699 | 8 | 115 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.release | release | 700 | 8 | 116 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.flags | flags | 701 | 8 | 117 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.enabled | enabled | 702 | 8 | 118 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.highpass_enabled | highpass_enabled | 703 | 8 | 119 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.lowpass_enabled | lowpass_enabled | 704 | 9 | 0 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.10.limiter_enabled | limiter_enabled | 705 | 9 | 1 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.highpass | highpass | 708 | 9 | 4 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.lowpass | lowpass | 709 | 9 | 5 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.boost | boost | 710 | 9 | 6 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.ceiling | ceiling | 711 | 9 | 7 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.release | release | 712 | 9 | 8 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.flags | flags | 713 | 9 | 9 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.enabled | enabled | 714 | 9 | 10 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.highpass_enabled | highpass_enabled | 715 | 9 | 11 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.lowpass_enabled | lowpass_enabled | 716 | 9 | 12 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.11.limiter_enabled | limiter_enabled | 717 | 9 | 13 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.highpass | highpass | 720 | 9 | 16 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.lowpass | lowpass | 721 | 9 | 17 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.boost | boost | 722 | 9 | 18 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.ceiling | ceiling | 723 | 9 | 19 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.release | release | 724 | 9 | 20 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.flags | flags | 725 | 9 | 21 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.enabled | enabled | 726 | 9 | 22 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.highpass_enabled | highpass_enabled | 727 | 9 | 23 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.lowpass_enabled | lowpass_enabled | 728 | 9 | 24 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.12.limiter_enabled | limiter_enabled | 729 | 9 | 25 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.highpass | highpass | 732 | 9 | 28 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.lowpass | lowpass | 733 | 9 | 29 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.boost | boost | 734 | 9 | 30 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.ceiling | ceiling | 735 | 9 | 31 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.release | release | 736 | 9 | 64 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.flags | flags | 737 | 9 | 65 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.enabled | enabled | 738 | 9 | 66 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.highpass_enabled | highpass_enabled | 739 | 9 | 67 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.lowpass_enabled | lowpass_enabled | 740 | 9 | 68 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.13.limiter_enabled | limiter_enabled | 741 | 9 | 69 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.highpass | highpass | 744 | 9 | 72 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.lowpass | lowpass | 745 | 9 | 73 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.boost | boost | 746 | 9 | 74 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.ceiling | ceiling | 747 | 9 | 75 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.release | release | 748 | 9 | 76 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.flags | flags | 749 | 9 | 77 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.enabled | enabled | 750 | 9 | 78 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.highpass_enabled | highpass_enabled | 751 | 9 | 79 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.lowpass_enabled | lowpass_enabled | 752 | 9 | 80 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.14.limiter_enabled | limiter_enabled | 753 | 9 | 81 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.highpass | highpass | 756 | 9 | 84 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.lowpass | lowpass | 757 | 9 | 85 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.boost | boost | 758 | 9 | 86 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.ceiling | ceiling | 759 | 9 | 87 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.release | release | 760 | 9 | 88 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.flags | flags | 761 | 9 | 89 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.enabled | enabled | 762 | 9 | 90 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.highpass_enabled | highpass_enabled | 763 | 9 | 91 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.lowpass_enabled | lowpass_enabled | 764 | 9 | 92 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.15.limiter_enabled | limiter_enabled | 765 | 9 | 93 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.highpass | highpass | 768 | 9 | 96 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.lowpass | lowpass | 769 | 9 | 97 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.boost | boost | 770 | 9 | 98 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.ceiling | ceiling | 771 | 9 | 99 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.release | release | 772 | 9 | 100 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.flags | flags | 773 | 9 | 101 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.enabled | enabled | 774 | 9 | 102 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.highpass_enabled | highpass_enabled | 775 | 9 | 103 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.lowpass_enabled | lowpass_enabled | 776 | 9 | 104 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.16.limiter_enabled | limiter_enabled | 777 | 9 | 105 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.highpass | highpass | 780 | 9 | 108 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.lowpass | lowpass | 781 | 9 | 109 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.boost | boost | 782 | 9 | 110 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.ceiling | ceiling | 783 | 9 | 111 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.release | release | 784 | 9 | 112 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.flags | flags | 785 | 9 | 113 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.enabled | enabled | 786 | 9 | 114 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.highpass_enabled | highpass_enabled | 787 | 9 | 115 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.lowpass_enabled | lowpass_enabled | 788 | 9 | 116 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.17.limiter_enabled | limiter_enabled | 789 | 9 | 117 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.highpass | highpass | 792 | 10 | 0 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.lowpass | lowpass | 793 | 10 | 1 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.boost | boost | 794 | 10 | 2 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.ceiling | ceiling | 795 | 10 | 3 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.release | release | 796 | 10 | 4 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.flags | flags | 797 | 10 | 5 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.enabled | enabled | 798 | 10 | 6 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.highpass_enabled | highpass_enabled | 799 | 10 | 7 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.lowpass_enabled | lowpass_enabled | 800 | 10 | 8 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.18.limiter_enabled | limiter_enabled | 801 | 10 | 9 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.highpass | highpass | 804 | 10 | 12 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.lowpass | lowpass | 805 | 10 | 13 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.boost | boost | 806 | 10 | 14 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.ceiling | ceiling | 807 | 10 | 15 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.release | release | 808 | 10 | 16 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.flags | flags | 809 | 10 | 17 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.enabled | enabled | 810 | 10 | 18 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.highpass_enabled | highpass_enabled | 811 | 10 | 19 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.lowpass_enabled | lowpass_enabled | 812 | 10 | 20 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.19.limiter_enabled | limiter_enabled | 813 | 10 | 21 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.highpass | highpass | 816 | 10 | 24 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.lowpass | lowpass | 817 | 10 | 25 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.boost | boost | 818 | 10 | 26 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.ceiling | ceiling | 819 | 10 | 27 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.release | release | 820 | 10 | 28 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.flags | flags | 821 | 10 | 29 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.enabled | enabled | 822 | 10 | 30 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.highpass_enabled | highpass_enabled | 823 | 10 | 31 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.lowpass_enabled | lowpass_enabled | 824 | 10 | 64 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.20.limiter_enabled | limiter_enabled | 825 | 10 | 65 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.highpass | highpass | 828 | 10 | 68 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.lowpass | lowpass | 829 | 10 | 69 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.boost | boost | 830 | 10 | 70 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.ceiling | ceiling | 831 | 10 | 71 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.release | release | 832 | 10 | 72 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.flags | flags | 833 | 10 | 73 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.enabled | enabled | 834 | 10 | 74 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.highpass_enabled | highpass_enabled | 835 | 10 | 75 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.lowpass_enabled | lowpass_enabled | 836 | 10 | 76 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.21.limiter_enabled | limiter_enabled | 837 | 10 | 77 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.highpass | highpass | 840 | 10 | 80 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.lowpass | lowpass | 841 | 10 | 81 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.boost | boost | 842 | 10 | 82 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.ceiling | ceiling | 843 | 10 | 83 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.release | release | 844 | 10 | 84 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.flags | flags | 845 | 10 | 85 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.enabled | enabled | 846 | 10 | 86 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.highpass_enabled | highpass_enabled | 847 | 10 | 87 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.lowpass_enabled | lowpass_enabled | 848 | 10 | 88 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.22.limiter_enabled | limiter_enabled | 849 | 10 | 89 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.highpass | highpass | 852 | 10 | 92 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.lowpass | lowpass | 853 | 10 | 93 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.boost | boost | 854 | 10 | 94 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.ceiling | ceiling | 855 | 10 | 95 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.release | release | 856 | 10 | 96 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.flags | flags | 857 | 10 | 97 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.enabled | enabled | 858 | 10 | 98 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.highpass_enabled | highpass_enabled | 859 | 10 | 99 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.lowpass_enabled | lowpass_enabled | 860 | 10 | 100 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.23.limiter_enabled | limiter_enabled | 861 | 10 | 101 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.highpass | highpass | 864 | 10 | 104 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.lowpass | lowpass | 865 | 10 | 105 | [20, 20000] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.boost | boost | 866 | 10 | 106 | [0, 24] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.ceiling | ceiling | 867 | 10 | 107 | [0, 48] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.release | release | 868 | 10 | 108 | [10, 250] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.flags | flags | 869 | 10 | 109 | [0, 15] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.enabled | enabled | 870 | 10 | 110 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.highpass_enabled | highpass_enabled | 871 | 10 | 111 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.lowpass_enabled | lowpass_enabled | 872 | 10 | 112 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| strip.24.limiter_enabled | limiter_enabled | 873 | 10 | 113 | [0, 1] | {} | STRIP · native candidate; focused acceptance required |
| sync.global_mode | SYNC RATE MODE |  | unmapped | unmapped | [0, 1] | {} | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| sync.strip_override | RATE MODE |  | unmapped | unmapped | [0, 3] | {"strip":[0,24]} | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| pref.SQ_SWING | SWING | 900 | 11 | 20 | [0, 100] | {"pattern":[0,31]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_DLEN | LENGTH | 901 | 11 | 21 | [0, 63] | {"pattern":[0,31],"kit":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_DRATE | RATE | 902 | 11 | 22 | [0, 3] | {"pattern":[0,31],"kit":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_BLEN | LENGTH | 903 | 11 | 23 | [0, 63] | {"pattern":[0,31],"bass":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_BRATE | RATE | 904 | 11 | 24 | [0, 3] | {"pattern":[0,31],"bass":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_BDIR | DIR | 905 | 11 | 25 | [0, 3] | {"pattern":[0,31],"bass":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_BTRANS | TRANSP | 906 | 11 | 26 | [0, 48] | {"pattern":[0,31],"bass":[0,1]} | SEQ · native candidate; focused acceptance required |
| pref.SQ_TEMPO | TEMPO | 907 | 11 | 27 | [0, 255] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_ACCENT | ACCENT | 908 | 11 | 28 | [0, 127] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_CLK | CLOCK OUT | 909 | 11 | 29 | [0, 1] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_NOTES | NOTES OUT | 910 | 11 | 30 | [0, 1] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_THEME | THEME | 911 | 11 | 31 | [0, 4] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_KEYLED | LIGHTS | 912 | 11 | 64 | [0, 2] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_KEYSOUND | KEY SOUND | 913 | 11 | 65 | [0, 1] | {} | SEQ · native candidate; focused acceptance required |
| pref.SQ_AUTOSAVE | AUTOSAVE | 914 | 11 | 66 | [0, 1] | {} | SEQ · native candidate; focused acceptance required |
| generator.0 | DENS | 920 | 11 | 72 | [0, 100] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.1 | ACCENT | 921 | 11 | 73 | [0, 100] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.2 | SLIDE | 922 | 11 | 74 | [0, 100] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.3 | OCTS | 923 | 11 | 75 | [0, 2] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.4 | ROOT | 924 | 11 | 76 | [0, 11] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.5 | SCALE | 925 | 11 | 77 | [0, 5] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.6 | OCTAVE | 926 | 11 | 78 | [0, 4] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| generator.7 | MUTATE | 927 | 11 | 79 | [0, 16] | {"pattern":[0,31],"bass":[0,1]} | GEN · native candidate; focused acceptance required |
| break.generator.0 | Cmplx | 936 | 11 | 88 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.1 | Anchor | 937 | 11 | 89 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.2 | Roll | 938 | 11 | 90 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.3 | Fill | 939 | 11 | 91 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.4 | Rtg2x | 940 | 11 | 92 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.5 | Rtg3x | 941 | 11 | 93 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.6 | Rtg4x | 942 | 11 | 94 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.7 | Rtg8x | 943 | 11 | 95 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.8 | Phrase | 944 | 11 | 96 | [0, 4] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.9 | BChnc | 945 | 11 | 97 | [0, 100] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.10 | A Len | 946 | 11 | 98 | [0, 5] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| break.generator.11 | B Len | 947 | 11 | 99 | [0, 5] | {"pattern":[0,31]} | BRKGEN · native candidate; focused acceptance required |
| part.channel.0 | 909 MIDI CH | 952 | 11 | 104 | [0, 16] | {} | CHANNEL · native candidate; focused acceptance required |
| part.channel.1 | 808 MIDI CH | 953 | 11 | 105 | [0, 16] | {} | CHANNEL · native candidate; focused acceptance required |
| part.channel.2 | 303A MIDI CH | 954 | 11 | 106 | [0, 16] | {} | CHANNEL · native candidate; focused acceptance required |
| part.channel.3 | 303B MIDI CH | 955 | 11 | 107 | [0, 16] | {} | CHANNEL · native candidate; focused acceptance required |
| part.channel.4 | BREAK MIDI CH | 956 | 11 | 108 | [0, 16] | {} | CHANNEL · native candidate; focused acceptance required |
| pref.OUT_P | OUTPUT | 960 | 11 | 112 | [0, 1] | {} | PREF · native candidate; focused acceptance required |
| pref.USBA_P | USB AUDIO | 961 | 11 | 113 | [0, 1] | {} | PREF · native candidate; focused acceptance required |
| pref.FREEPLAY_P | FREE PLAY | 962 | 11 | 114 | [0, 1] | {"bass":[0,1]} | PREF · native candidate; focused acceptance required |
| pref.SREP_P | REPEAT | 963 | 11 | 115 | [0, 7] | {"block":[0,191]} | PREF · native candidate; focused acceptance required |
| pref.STALLS_P | STALLS | 964 | 11 | 116 | [0, 1] | {} | PREF · native candidate; focused acceptance required |
| pref.BATT_P | BATTERY |  | unmapped | unmapped | [0, 5] | {} | PREF · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| song.0 | 909 | 972 | 12 | 4 | [0, 31] | {"block":[0,191]} | SONG · native candidate; focused acceptance required |
| song.1 | 808 | 973 | 12 | 5 | [0, 31] | {"block":[0,191]} | SONG · native candidate; focused acceptance required |
| song.2 | 303A | 974 | 12 | 6 | [0, 31] | {"block":[0,191]} | SONG · native candidate; focused acceptance required |
| song.3 | 303B | 975 | 12 | 7 | [0, 31] | {"block":[0,191]} | SONG · native candidate; focused acceptance required |
| song.4 | BREAK | 976 | 12 | 8 | [0, 31] | {"block":[0,191]} | SONG · native candidate; focused acceptance required |
| song.5 | LENGTH | 977 | 12 | 9 | [0, 192] | {} | SONG · native candidate; focused acceptance required |
| action.0 | No action |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.1 | SAVE PROJECT | 985 | 12 | 17 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.2 | CLEAR THIS PART | 986 | 12 | 18 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.3 | CLEAR PATTERN | 987 | 12 | 19 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.4 | COPY PATTERN | 988 | 12 | 20 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.5 | FACTORY RESET | 989 | 12 | 21 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.6 | ABOUT X0X | 990 | 12 | 22 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.7 | INSERT BAR | 991 | 12 | 23 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.8 | DELETE BAR | 992 | 12 | 24 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.9 | CLEAR SONG | 993 | 12 | 25 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.10 | PERFORMANCE | 994 | 12 | 26 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.11 | RUN PERF TEST | 995 | 12 | 27 | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action.12 | DUP AS VARIATION |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.13 | EDIT BLOCK NOTES |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.14 | QUEUE BLOCK |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.15 | DELETE BLOCK |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.16 | UNDO |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.17 | PASTE KEEP ACC/SW |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| action.18 | COPY/PASTE SEQUENCES |  | unmapped | unmapped | [0, 127] | {"pattern":[0,31],"part":[0,4],"block":[0,191]} · edge-trigger; explicit safety/preflight; no storage operation in audio callback | ACTION · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| system.one_core | Second core disabled at next power-on |  | unmapped | unmapped | [0, 1] | {} | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| selector.pattern | selector.pattern | 1010 | 12 | 74 | [0, 31] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.part | selector.part | 1011 | 12 | 75 | [0, 4] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.drum | selector.drum | 1012 | 12 | 76 | [0, 10] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.bass | selector.bass | 1013 | 12 | 77 | [0, 1] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.step | selector.step | 1014 | 12 | 78 | [0, 63] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.block_low | selector.block_low | 1015 | 12 | 79 | [0, 127] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.block_high | selector.block_high | 1016 | 12 | 80 | [0, 1] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.motion_low | selector.motion_low | 1017 | 12 | 81 | [0, 127] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.motion_high | selector.motion_high | 1018 | 12 | 82 | [0, 1] | {} | SELECTOR · native candidate; focused acceptance required |
| pref.break_slot_a | pref.break_slot_a | 1019 | 12 | 83 | [0, 127] | {"pattern":[0,31]} | PREF · native candidate; focused acceptance required |
| pref.break_slot_b | pref.break_slot_b | 1020 | 12 | 84 | [0, 127] | {"pattern":[0,31]} | PREF · native candidate; focused acceptance required |
| pref.song_edit_block | pref.song_edit_block | 1021 | 12 | 85 | [0, 191] | {} | PREF · native candidate; focused acceptance required |
| pref.tape_macro | pref.tape_macro | 1022 | 12 | 86 | [0, 127] | {} | PREF · native candidate; focused acceptance required |
| step.drum_hit | step.drum_hit | 1023 | 12 | 87 | [0, 1] | {"pattern":[0,31],"kit":[0,1],"drum":[0,10],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.drum_accent | step.drum_accent | 1024 | 12 | 88 | [0, 1] | {"pattern":[0,31],"kit":[0,1],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.bass_note | step.bass_note | 1025 | 12 | 89 | [0, 127] | {"pattern":[0,31],"bass":[0,1],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.bass_gate | step.bass_gate | 1026 | 12 | 90 | [0, 2] | {"pattern":[0,31],"bass":[0,1],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.bass_accent | step.bass_accent | 1027 | 12 | 91 | [0, 1] | {"pattern":[0,31],"bass":[0,1],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.bass_slide | step.bass_slide | 1028 | 12 | 92 | [0, 1] | {"pattern":[0,31],"bass":[0,1],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.break_allowed | step.break_allowed | 1029 | 12 | 93 | [0, 1] | {"pattern":[0,31],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| step.break_slice | step.break_slice | 1030 | 12 | 94 | [0, 8] | {"pattern":[0,31],"step":[0,63]} | STEP · native candidate; focused acceptance required |
| motion.value | motion.value | 1031 | 12 | 95 | [0, 255] | {"lane":[0,159],"step":[0,63]} | MOTION · native candidate; focused acceptance required |
| motion.target | motion.target | 1032 | 12 | 96 | [0, 7] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.voice | motion.voice | 1033 | 12 | 97 | [0, 127] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.param | motion.param | 1034 | 12 | 98 | [0, 15] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.pattern | motion.pattern | 1035 | 12 | 99 | [0, 31] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.part | motion.part | 1036 | 12 | 100 | [0, 4] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.record | motion.record | 1037 | 12 | 101 | [0, 1] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.hold | motion.hold | 1038 | 12 | 102 | [0, 1] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| motion.clear | motion.clear | 1039 | 12 | 103 | [0, 1] | {"lane":[0,159]} | MOTION · native candidate; focused acceptance required |
| probability.value | probability.value | 1040 | 12 | 104 | [0, 135] | {"pattern":[0,31],"part":[0,4],"drum":[0,10],"step":[0,63]} | PROB · native candidate; focused acceptance required |
| performance.mute | performance.mute | 1041 | 12 | 105 | [0, 1] | {"strip":[0,24]} | PERF · native candidate; focused acceptance required |
| performance.solo | performance.solo | 1042 | 12 | 106 | [0, 1] | {"strip":[0,24]} | PERF · native candidate; focused acceptance required |
| performance.pattern_cue | performance.pattern_cue | 1043 | 12 | 107 | [0, 31] | {"part":[0,4]} | PERF · native candidate; focused acceptance required |
| performance.chain_start | performance.chain_start | 1044 | 12 | 108 | [0, 31] | {} | PERF · native candidate; focused acceptance required |
| performance.chain_end | performance.chain_end | 1045 | 12 | 109 | [0, 31] | {} | PERF · native candidate; focused acceptance required |
| performance.song_enabled | performance.song_enabled | 1046 | 12 | 110 | [0, 1] | {} | PERF · native candidate; focused acceptance required |
| performance.song_record | performance.song_record | 1047 | 12 | 111 | [0, 1] | {} | PERF · native candidate; focused acceptance required |
| generator.new | generator.new | 1048 | 12 | 112 | [0, 1] | {"pattern":[0,31],"bass":[0,1]} | GENACTION · native candidate; focused acceptance required |
| generator.mutate | generator.mutate | 1049 | 12 | 113 | [0, 1] | {"pattern":[0,31],"bass":[0,1]} | GENACTION · native candidate; focused acceptance required |
| generator.seed_byte | generator.seed_byte | 1050 | 12 | 114 | [0, 255] | {"pattern":[0,31],"bass":[0,1],"seed_byte":[0,3]} | SEED · native candidate; focused acceptance required |
| arrangement.enabled | arrangement.enabled | 1051 | 12 | 115 | [0, 1] | {} | ARR · native candidate; focused acceptance required |
| arrangement.length | arrangement.length | 1052 | 12 | 116 | [0, 192] | {} | ARR · native candidate; focused acceptance required |
| arrangement.key | arrangement.key | 1053 | 12 | 117 | [0, 11] | {} | ARR · native candidate; focused acceptance required |
| arrangement.mode | arrangement.mode | 1054 | 12 | 118 | [0, 9] | {} | ARR · native candidate; focused acceptance required |
| arrangement.parent | arrangement.parent | 1055 | 12 | 119 | [0, 255] | {} | ARR · native candidate; focused acceptance required |
| arrangement.degree | arrangement.degree | 1056 | 13 | 0 | [0, 7] | {"block":[0,191]} | ARR · native candidate; focused acceptance required |
| arrangement.duration | arrangement.duration | 1057 | 13 | 1 | [1, 32] | {"block":[0,191]} | ARR · native candidate; focused acceptance required |
| arrangement.pattern | arrangement.pattern | 1058 | 13 | 2 | [0, 31] | {"block":[0,191],"part":[0,4]} | ARR · native candidate; focused acceptance required |
| arrangement.mute | arrangement.mute | 1059 | 13 | 3 | [0, 1] | {"block":[0,191],"part":[0,4]} | ARR · native candidate; focused acceptance required |
| arrangement.queue | arrangement.queue | 1060 | 13 | 4 | [0, 191] | {} | ARR · native candidate; focused acceptance required |
| follow.mode | follow.mode | 1061 | 13 | 5 | [0, 3] | {"bass":[0,1]} | FOLLOW · native candidate; focused acceptance required |
| follow.low | follow.low | 1062 | 13 | 6 | [0, 127] | {"bass":[0,1]} | FOLLOW · native candidate; focused acceptance required |
| follow.high | follow.high | 1063 | 13 | 7 | [0, 127] | {"bass":[0,1]} | FOLLOW · native candidate; focused acceptance required |
| pad.channel | pad.channel | 1064 | 13 | 8 | [0, 255] | {} | PAD · native candidate; focused acceptance required |
| pad.root | pad.root | 1065 | 13 | 9 | [0, 127] | {} | PAD · native candidate; focused acceptance required |
| pad.low | pad.low | 1066 | 13 | 10 | [0, 127] | {} | PAD · native candidate; focused acceptance required |
| pad.high | pad.high | 1067 | 13 | 11 | [0, 127] | {} | PAD · native candidate; focused acceptance required |
| selector.kit | selector.kit | 1068 | 13 | 12 | [0, 1] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.strip | selector.strip | 1069 | 13 | 13 | [0, 24] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.seed_byte | selector.seed_byte | 1070 | 13 | 14 | [0, 3] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.commit | selector.commit | 1071 | 13 | 15 | [0, 1] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.parameter_low | selector.parameter_low | 1072 | 13 | 16 | [0, 127] | {} | SELECTOR · native candidate; focused acceptance required |
| selector.parameter_high | selector.parameter_high | 1073 | 13 | 17 | [0, 15] | {} | SELECTOR · native candidate; focused acceptance required |
| value.exact_low7 | value.exact_low7 | 1074 | 13 | 18 | [0, 127] | {} | EXACT · native candidate; focused acceptance required |
| value.exact_mid7 | value.exact_mid7 | 1075 | 13 | 19 | [0, 127] | {} | EXACT · native candidate; focused acceptance required |
| value.exact_high2 | value.exact_high2 | 1076 | 13 | 20 | [0, 3] | {} | EXACT · native candidate; focused acceptance required |
| value.exact_commit | value.exact_commit | 1077 | 13 | 21 | [0, 1] | {} | EXACT · native candidate; focused acceptance required |
| lane.length | Individual sound-lane length | 1128 | 13 | 104 | [1, 64] | {"pattern":[0,31],"strip":[0,24]} | LANE · native candidate; focused acceptance required |
| lane.rate | Individual sound-lane rate | 1129 | 13 | 105 | [0, 7] | {"pattern":[0,31],"strip":[0,24]} | LANE · native candidate; focused acceptance required |
| value.exact_high7 | 21-bit exact value high chunk | 1130 | 13 | 106 | [0, 127] | {} | EXACT · native candidate; focused acceptance required |
| lane.mode | Prepared independent clock mode | 1131 | 13 | 107 | [0, 1] | {} | LANE · native candidate; focused acceptance required |
| selector.destination_pattern | Explicit pattern-copy destination | 1132 | 13 | 108 | [0, 31] | {} · source draft destination; selector.commit freezes it; never UI focus | SELECTOR · native candidate; focused acceptance required |
| action_control.arm | Arm destructive native action | 1133 | 13 | 109 | [0, 11] | {} · 0 cancels; action2/3/4/5/7/8/9 binds source/context/project revision | ACTIONCTL · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| action_control.cancel | Cancel source action arm | 1134 | 13 | 110 | [0, 1] | {} · 1 cancels source arm; 0 does nothing | ACTIONCTL · native candidate; focused acceptance required · isolated source-confirmed explicit-context stopped action candidate; storage atomicity and device acceptance open |
| rate.enable | Enable expanded rates (stopped) | 1135 | 13 | 111 | [0, 1] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| rate.exact | Expanded exact division | 1136 | 13 | 112 | [0, 20] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| rate.family | Rate sweep family | 1137 | 13 | 113 | [0, 3] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| rate.boundary | Rate latch boundary | 1138 | 13 | 114 | [0, 1] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| rate.hysteresis | Rate sweep hysteresis CC units | 1139 | 13 | 115 | [0, 2] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| rate.sweep_v1 | Family-safe rate sweep ABI1 | 1140 | 13 | 116 | [0, 127] | {"pattern":[0,31],"strip":[0,24]} | RATE · native candidate; focused acceptance required |
| output.master_knob | MASTER gain Q12 | 1100 | 13 | 76 | [0, 4096] | {} | MASTER · native candidate; focused acceptance required · runtime Q12 gain, source-owned ADC pickup; physical acceptance pending |
| slice.level_percent | slice.level_percent | 1104 | 13 | 80 | [0, 100] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.repeat | slice.repeat | 1105 | 13 | 81 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.layer | slice.layer | 1106 | 13 | 82 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.source_bpm_milli | slice.source_bpm_milli | 1107 | 13 | 83 | [20000, 300000] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.audition_index | slice.audition_index | 1108 | 13 | 84 | [0, 63] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.play | slice.play | 1109 | 13 | 85 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.stop | slice.stop | 1110 | 13 | 86 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.arm_sync | slice.arm_sync | 1111 | 13 | 87 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.import | slice.import | 1112 | 13 | 88 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.demo | slice.demo | 1113 | 13 | 89 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.save_import | slice.save_import | 1114 | 13 | 90 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.reopen_import | slice.reopen_import | 1115 | 13 | 91 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| slice.export | slice.export | 1116 | 13 | 92 | [0, 1] | {} | UNSUPPORTED · unimplemented · native slice dispatch remains unsupported; separate HOST browser overlay below |
| editor.undo | editor.undo | 1117 | 13 | 93 | [0, 1] | {} | HISTORY · native candidate; focused acceptance required · stopped shared project history; rejects owned/recording gestures; global proj.set settings remain outside journal |
| editor.redo | editor.redo | 1118 | 13 | 94 | [0, 1] | {} | HISTORY · native candidate; focused acceptance required · stopped shared project history; rejects owned/recording gestures; global proj.set settings remain outside journal |
| transport.play | transport.play | 1119 | 13 | 95 | [0, 1] | {} | TRANSPORT · native candidate; focused acceptance required |
| transport.stop | transport.stop | 1120 | 13 | 96 | [0, 1] | {} | TRANSPORT · native candidate; focused acceptance required |
| transport.record | transport.record | 1121 | 13 | 97 | [0, 1] | {} | TRANSPORT · native candidate; focused acceptance required |
| route.connect_permission | route.connect_permission |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| route.output_selection | route.output_selection |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| route.activate | route.activate |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| route.retry_cleanup | route.retry_cleanup |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| route.disconnect | route.disconnect |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| file.audio_selection | file.audio_selection |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| file.marker_selection | file.marker_selection |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| file.hardware_slot_upload | file.hardware_slot_upload |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| file.hardware_slot_clear | file.hardware_slot_clear |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| ui.navigation | ui.navigation |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| ui.keyboard_latch | ui.keyboard_latch |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| ui.factory_reset | ui.factory_reset |  | unmapped | unmapped | [0, 1] | {} · explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| ui.pattern_copy.source | Stopped copy source | 1241 | 15 | 9 | [0, 31] | {} · Private explicit copy recipe;guarded preview/arm and request execute;no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.pattern_copy.destination | Stopped copy destination | 1242 | 15 | 10 | [0, 31] | {} · Private explicit copy recipe;guarded preview/arm and request execute;no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.pattern_copy.scope | Stopped copy scope | 1243 | 15 | 11 | [0, 24] | {} · Private explicit copy recipe;guarded preview/arm and request execute;no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.constrained_generation.mode | Constrained generation mode | 1203 | 14 | 91 | [0, 4] | {} · Explicit source-owned stopped generation; guarded request/result, no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.constrained_generation.chord_block | Constrained generation chord_block | 1204 | 14 | 92 | [0, 191] | {} · Explicit source-owned stopped generation; guarded request/result, no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.constrained_generation.generate_confirmed | Constrained generation generate_confirmed | 1205 | 14 | 93 | [0, 1] | {} · Explicit source-owned stopped generation; guarded request/result, no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.drum_generation.density | Drum generation density | 1206 | 14 | 94 | [0, 100] | {} · Source-owned stopped drum generation; arm3, explicit context, shared length, request and result | REMOTE · native candidate; focused acceptance required |
| ui.drum_generation.positions | Drum generation positions | 1207 | 14 | 95 | [0, 3] | {} · Source-owned stopped drum generation; arm3, explicit context, shared length, request and result | REMOTE · native candidate; focused acceptance required |
| ui.drum_generation.anchors | Drum generation anchors | 1208 | 14 | 96 | [0, 5] | {} · Source-owned stopped drum generation; arm3, explicit context, shared length, request and result | REMOTE · native candidate; focused acceptance required |
| ui.drum_generation.seed_index | Drum generation seed_index | 1209 | 14 | 97 | [0, 255] | {} · Source-owned stopped drum generation; arm3, explicit context, shared length, request and result | REMOTE · native candidate; focused acceptance required |
| ui.drum_generation.replace_confirm | Drum generation replace_confirm | 1210 | 14 | 98 | [0, 1] | {} · Source-owned stopped drum generation; arm3, explicit context, shared length, request and result | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.every | Repeating variation every | 1211 | 14 | 99 | [0, 2] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.changes | Repeating variation changes | 1212 | 14 | 100 | [0, 8] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.anchor_lock | Repeating variation anchor_lock | 1213 | 14 | 101 | [0, 3] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.seed | Repeating variation seed | 1214 | 14 | 102 | [0, 255] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.mode | Repeating variation mode | 1215 | 14 | 103 | [0, 2] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.shape | Repeating variation shape | 1216 | 14 | 104 | [0, 1] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.range | Repeating variation range | 1217 | 14 | 105 | [0, 3] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.arm_confirm | Repeating variation arm_confirm | 1218 | 14 | 106 | [0, 1] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.repeating_variation.off_confirm | Repeating variation off_confirm | 1219 | 14 | 107 | [0, 1] | {} · Source-owned runtime variation;explicit context,arm4/5,guarded apply/OFF;no saved pattern mutation | REMOTE · native candidate; focused acceptance required |
| ui.live_overlay | Live drum display overlay (session only; default OFF) |  | unmapped | unmapped | [0, 1] | {} · Native drum VAR OPTIONS knob4 only; no project save, Undo, CC binding or timing-acceptance claim | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| ui.freeze_commit.destination | Freeze/Commit destination | 1220 | 14 | 108 | [0, 31] | {} · Explicit Freeze arm6/pending full-pass receipt;Commit arm7,held capture token,stopped destination transaction | REMOTE · native candidate; focused acceptance required |
| ui.freeze_commit.freeze_request | Freeze/Commit freeze_request | 1221 | 14 | 109 | [0, 1] | {} · Explicit Freeze arm6/pending full-pass receipt;Commit arm7,held capture token,stopped destination transaction | REMOTE · native candidate; focused acceptance required |
| ui.freeze_commit.commit_confirm | Freeze/Commit commit_confirm | 1222 | 14 | 110 | [0, 1] | {} · Explicit Freeze arm6/pending full-pass receipt;Commit arm7,held capture token,stopped destination transaction | REMOTE · native candidate; focused acceptance required |
| ui.random_options.length | Random options length | 1223 | 14 | 111 | [0, 64] | {} · Explicit source-owned stopped generation; guarded request/result, no UI focus | REMOTE · native candidate; focused acceptance required |
| ui.random_options.note_step | Random options note_step | 1224 | 14 | 112 | [0, 64] | {} · Explicit source-owned stopped generation; guarded request/result, no UI focus | REMOTE · native candidate; focused acceptance required |
| remote.generation.seed_byte0 | remote.generation.seed_byte0 | 1227 | 14 | 115 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.generation.seed_byte1 | remote.generation.seed_byte1 | 1228 | 14 | 116 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.generation.seed_byte2 | remote.generation.seed_byte2 | 1229 | 14 | 117 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.generation.seed_byte3 | remote.generation.seed_byte3 | 1230 | 14 | 118 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.request_low7 | remote.request_low7 | 1231 | 14 | 119 | [0, 127] | {} | REMOTE · native candidate; focused acceptance required |
| remote.request_high7 | remote.request_high7 | 1232 | 15 | 0 | [0, 127] | {} | REMOTE · native candidate; focused acceptance required |
| remote.action_arm | remote.action_arm | 1233 | 15 | 1 | [0, 7] | {} | REMOTE · native candidate; focused acceptance required |
| remote.cancel | remote.cancel | 1234 | 15 | 2 | [0, 1] | {} | REMOTE · native candidate; focused acceptance required |
| ui.cc_mode | GLO MIDI CC MODE |  | unmapped | unmapped | [0, 2] | {} · Session-only native GLO; invalidates pending CC contexts/arms; readback and non-CC MIDI remain available | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| hybrid808.session.0 | HYBRID | 1250 | 15 | 18 | [0, 1] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.1 | WAVE | 1251 | 15 | 19 | [0, 4] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.2 | WAVE MIX | 1252 | 15 | 20 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.3 | WAVE HZ | 1253 | 15 | 21 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.4 | W DECAY | 1254 | 15 | 22 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.5 | W SWEEP | 1255 | 15 | 23 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.6 | K FILTER | 1256 | 15 | 24 | [0, 1] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.7 | K CUTOFF | 1257 | 15 | 25 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.8 | V CUTOFF | 1258 | 15 | 26 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.9 | V PITCH | 1259 | 15 | 27 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.10 | V DECAY | 1260 | 15 | 28 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.11 | V BLEND | 1261 | 15 | 29 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.12 | LOW VEL | 1262 | 15 | 30 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.13 | MID VEL | 1263 | 15 | 31 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.14 | HIGH VEL | 1264 | 15 | 64 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.15 | J CHANCE | 1265 | 15 | 65 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.16 | J RANGE | 1266 | 15 | 66 | [0, 40] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.17 | VEL TIERS | 1267 | 15 | 67 | [0, 1] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.18 | S ATTACK | 1268 | 15 | 68 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.19 | S PUNCH | 1269 | 15 | 69 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.20 | S P TIME | 1270 | 15 | 70 | [0, 200] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.21 | S TAIL | 1271 | 15 | 71 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.22 | S LEVEL | 1272 | 15 | 72 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.23 | V PUNCH | 1273 | 15 | 73 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.24 | S ENV | 1274 | 15 | 74 | [0, 1] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.25 | S ATTACK | 1275 | 15 | 75 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.26 | S HOLD | 1276 | 15 | 76 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.27 | S DECAY | 1277 | 15 | 77 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.28 | S CURVE | 1278 | 15 | 78 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.29 | W ENV | 1279 | 15 | 79 | [0, 1] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.30 | W ATTACK | 1280 | 15 | 80 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.31 | W HOLD | 1281 | 15 | 81 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.32 | W DECAY | 1282 | 15 | 82 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.33 | W CURVE | 1283 | 15 | 83 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.34 | CLICK | 1284 | 15 | 84 | [0, 60] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.35 | C TONE | 1285 | 15 | 85 | [0, 127] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.36 | C LENGTH | 1286 | 15 | 86 | [0, 80] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.37 | CRUNCH | 1287 | 15 | 87 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| hybrid808.session.38 | SMOOTH | 1288 | 15 | 88 | [0, 100] | {} | KICK · native candidate; focused acceptance required |
| kick.preset.store_arm | Kick preset store_arm | 1290 | 15 | 90 | [0, 1] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.preset.store_execute | Kick preset store_execute | 1291 | 15 | 91 | [0, 1] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.preset.recall_arm | Kick preset recall_arm | 1292 | 15 | 92 | [0, 1] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.preset.recall_execute | Kick preset recall_execute | 1293 | 15 | 93 | [0, 1] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.preset.request_low7 | Kick preset request_low7 | 1294 | 15 | 94 | [0, 127] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.preset.request_high7 | Kick preset request_high7 | 1295 | 15 | 95 | [0, 127] | {} · Committed808BD context,destination0..15,request token,arm/guarded execute;readback16 | KICKPRESET · native candidate; focused acceptance required |
| kick.step_tier | 808 kick step velocity tier | 1296 | 15 | 96 | [0, 3] | {"pattern":[0,31],"step":[0,63]} · Committed source-owned pattern/step;no hit created | KICKSTEP · native candidate; focused acceptance required |
| phase303b.engine | 303B PHASE engine |  | unmapped | unmapped | [0, 1] | {} · Private hardware-first prototype; native knobs and variant command22; no direct MIDI CC binding | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| phase303b.wave | 303B PHASE wave |  | unmapped | unmapped | [0, 7] | {} · Private hardware-first prototype; native knobs and variant command22; no direct MIDI CC binding | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| phase303b.shape | 303B PHASE shape |  | unmapped | unmapped | [0, 127] | {} · Private hardware-first prototype; native knobs and variant command22; no direct MIDI CC binding | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| phase303b.envelope | 303B PHASE envelope |  | unmapped | unmapped | [0, 15] | {} · Private hardware-first prototype; native knobs and variant command22; no direct MIDI CC binding | UNSUPPORTED · explicitly unmapped/read-only/nonmusical · no writable direct CC address in reviewed registry |
| remote.variation.constraint_mode | Variation303 constraint mode | 1235 | 15 | 3 | [0, 3] | {} | REMOTE · native candidate; focused acceptance required |
| remote.variation.chord_block | Variation303 chord block | 1236 | 15 | 4 | [0, 191] | {} | REMOTE · native candidate; focused acceptance required |
| remote.capture.token_byte0 | Expected captured-pass token byte0 | 1237 | 15 | 5 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.capture.token_byte1 | Expected captured-pass token byte1 | 1238 | 15 | 6 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.capture.token_byte2 | Expected captured-pass token byte2 | 1239 | 15 | 7 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| remote.capture.token_byte3 | Expected captured-pass token byte3 | 1240 | 15 | 8 | [0, 255] | {} | REMOTE · native candidate; focused acceptance required |
| ui.pattern_copy.destination_scope | Scoped cross-part copy destination_scope | 1244 | 15 | 12 | [0, 24] | {} · Private explicit copy recipe;guarded preview/arm and request execute | REMOTE · native candidate; focused acceptance required |
| ui.pattern_copy.preview_arm | Scoped cross-part copy preview_arm | 1245 | 15 | 13 | [0, 1] | {} · Private explicit copy recipe;guarded preview/arm and request execute | REMOTE · native candidate; focused acceptance required |
| ui.pattern_copy.execute | Scoped cross-part copy execute | 1246 | 15 | 14 | [0, 1] | {} · Private explicit copy recipe;guarded preview/arm and request execute | REMOTE · native candidate; focused acceptance required |
