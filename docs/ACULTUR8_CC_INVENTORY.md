# Acultur8 complete CC design inventory

**Inventory and draft mapping only. Native candidate support is recorded in ACULTUR8_CC_RUNTIME_COVERAGE.json; this inventory does not certify browser/DAW/device acceptance.**

Generated with `python tools/cc_inventory.py --write`; validate with `--check`. No firmware compilation is performed.

753 inventory rows: 727 addressed field templates/descriptor rows and 26 explicitly unmapped/read-only/nonmusical rows. Repeated contexts enumerate the full pattern, step, block and lane extents in JSON. Categories: {'musical': 667, 'nonmusical': 7, 'diagnostic': 1, 'read_only': 1, 'action': 26, 'browser_musical': 5, 'browser_action': 8, 'nonmusical_action': 38}.

## Source-derived engine descriptor counts

| Family | Per-voice counts | Total |
| --- | --- | --- |
| 909 | [11, 10, 9, 9, 9, 7, 8, 8, 8, 8, 8, 2] | 97 |
| 808 | [10, 10, 9, 9, 9, 9, 10, 8, 8, 8, 8, 2] | 100 |
| 303 | [12, 12] | 24 |
| BREAK | [2] | 2 |
| FX | [16] | 16 |
| MIX | [4, 4, 4, 4, 4] | 20 |
| MASTER | [14] | 14 |

## Mapping contract

Fixed channel/CC addresses are independent of configurable note channels. CC120–127 are reserved for channel-mode messages. CC32–63 are reserved fine companions; optional 14-bit precision supplements ordinary 7-bit CC and is not NRPN-only.

Structured fields use explicit MIDI-owned selectors, never the open UI page or editing focus. Pattern 0–31, part 0–4, step 0–63, block 0–191 and motion lane 0–159 are explicit contexts. Selectors must be scoped per input source and reset on detach. Split-address transactions need an explicit commit barrier; its final wire protocol is not accepted yet.

Ranges describe stored encodings. Length pots add one, tempo adds 20 BPM, transpose subtracts 24. Enum holes and 255 sentinels require semantic validation, not numeric clamping. Frequency scaling is a draft logarithmic proposal; exact cutoff resolution and 14-bit pairing need acceptance. Direct CC7 quantizes wide ranges. The additional ordinary-CC explicit parameter/context/value transaction has three 7-bit chunks (21 bits), sufficient for browser source-BPM milli (19 bits); the browser setter remains unimplemented. Old high2 retains the original 16-bit compatibility path. No false claim of exact 7-bit representation.

Sparse parent/pad-channel domains use allowed-value ordinal scaling for both CC7 and optional CC14, so intermediate values remain valid. Motion values require an explicit parameter target: each sound descriptor has its own 0..max domain plus MOT_NONE=255; T_PROB supports probability 0..100, all 35 condition codes 101..135, and MOT_NONE. The original panel edits its 5%-step probability subset, while the evaluator accepts the wider domain. Exact-value transactions test target membership and preserve every condition code.

`lane.length` and `lane.rate` address all25 lanes in each of32 patterns; eight rates preserve IDs0..3. `lane.mode` appends address1131. First initialization requires stopped transport; subsequent mode changes are live at the next96-tick musical boundary. Legacy part descriptor addresses remain stable and dispatch through packed lane setters when initialized. BREAK step contexts now cover all64 steps; uninitialized legacy projects reject steps16..63 until initialized.

## Runtime design and acceptance still required

The original audio `midi_in` handler ignores CC. The isolated native candidate now intercepts it in an ordered main-context handoff; consult the runtime ledger and native checkpoint. Browser input listeners, compiled worklet/WASM acceptance and complete live coverage remain open. Original stopped-only follow/pad/output/length commands alone cannot establish live DAW automation.

Do not call UI/canonical/undo functions from the MIDI audio callback. Validate and publish canonical intent with audio-owner application, dirty/readback acknowledgement and bounded undo gestures. Reconcile motion hold/record. Live mode/channel/range changes retain original-channel Off debt before replacement attacks. Define musical boundaries for length/rate/block changes.

Context proof: `usb_poll` is TIMER5 ISR (`usb.c:4,960`, called by `main_fm1.c:146`); `uart_midi_poll` is also TIMER5 ISR (`midi_uart.c:87`, `main_fm1.c:148`). “Outside the render” does not mean main context. Reject direct canonical UI setters in either packet producer. Ordered main-context ring dispatch or a bounded CC mailbox needs further ownership/resource/order proof.

No incoming CC echo. Bound/coalesce automation without reordering selectors/actions; protect Note-Off/Stop/clock and expose rejection/overflow counters. Device IDs and input routes are not project data. Storage/destructive actions remain explicitly inventoried with preflight rather than silently omitted.

Acceptance: exhaustive registry coverage and endpoint/enum scaling; collision/reservation checks; every editable field while playing; canonical readback/motion/undo/save-reopen; saturated ingress and output cleanup; source-replacement races; real DAW/browser GUI; unchanged target resource gates and physical timing.

## Complete logical field registry

| ID | Name | Range | Channel / CC | Context | Category |
| --- | --- | --- | --- | --- | --- |
| sound.909.BD.0 | Tune | [0, 127] | 1 / 0 | {} | musical |
| sound.909.BD.1 | Attack | [0, 127] | 1 / 1 | {} | musical |
| sound.909.BD.2 | Decay | [0, 127] | 1 / 2 | {} | musical |
| sound.909.BD.3 | Level | [0, 127] | 1 / 3 | {} | musical |
| sound.909.BD.4 | P.Dpth | [0, 127] | 1 / 4 | {} | musical |
| sound.909.BD.5 | Pitch | [0, 127] | 1 / 5 | {} | musical |
| sound.909.BD.6 | Drive | [0, 127] | 1 / 6 | {} | musical |
| sound.909.BD.7 | Dist | [0, 6] | 1 / 7 | {} | musical |
| sound.909.BD.8 | Rev | [0, 127] | 1 / 8 | {} | musical |
| sound.909.BD.9 | Dly | [0, 127] | 1 / 9 | {} | musical |
| sound.909.BD.10 | Pan | [0, 127] | 1 / 10 | {} | musical |
| sound.909.SD.0 | Tune | [0, 127] | 1 / 16 | {} | musical |
| sound.909.SD.1 | Tone | [0, 127] | 1 / 17 | {} | musical |
| sound.909.SD.2 | Snappy | [0, 127] | 1 / 18 | {} | musical |
| sound.909.SD.3 | Decay | [0, 127] | 1 / 19 | {} | musical |
| sound.909.SD.4 | Level | [0, 127] | 1 / 20 | {} | musical |
| sound.909.SD.5 | Drive | [0, 127] | 1 / 21 | {} | musical |
| sound.909.SD.6 | Dist | [0, 6] | 1 / 22 | {} | musical |
| sound.909.SD.7 | Rev | [0, 127] | 1 / 23 | {} | musical |
| sound.909.SD.8 | Dly | [0, 127] | 1 / 24 | {} | musical |
| sound.909.SD.9 | Pan | [0, 127] | 1 / 25 | {} | musical |
| sound.909.LT.0 | Tune | [0, 127] | 1 / 64 | {} | musical |
| sound.909.LT.1 | Decay | [0, 127] | 1 / 65 | {} | musical |
| sound.909.LT.2 | Level | [0, 127] | 1 / 66 | {} | musical |
| sound.909.LT.3 | Attack | [0, 127] | 1 / 67 | {} | musical |
| sound.909.LT.4 | Drive | [0, 127] | 1 / 68 | {} | musical |
| sound.909.LT.5 | Dist | [0, 6] | 1 / 69 | {} | musical |
| sound.909.LT.6 | Rev | [0, 127] | 1 / 70 | {} | musical |
| sound.909.LT.7 | Dly | [0, 127] | 1 / 71 | {} | musical |
| sound.909.LT.8 | Pan | [0, 127] | 1 / 72 | {} | musical |
| sound.909.MT.0 | Tune | [0, 127] | 1 / 80 | {} | musical |
| sound.909.MT.1 | Decay | [0, 127] | 1 / 81 | {} | musical |
| sound.909.MT.2 | Level | [0, 127] | 1 / 82 | {} | musical |
| sound.909.MT.3 | Attack | [0, 127] | 1 / 83 | {} | musical |
| sound.909.MT.4 | Drive | [0, 127] | 1 / 84 | {} | musical |
| sound.909.MT.5 | Dist | [0, 6] | 1 / 85 | {} | musical |
| sound.909.MT.6 | Rev | [0, 127] | 1 / 86 | {} | musical |
| sound.909.MT.7 | Dly | [0, 127] | 1 / 87 | {} | musical |
| sound.909.MT.8 | Pan | [0, 127] | 1 / 88 | {} | musical |
| sound.909.HT.0 | Tune | [0, 127] | 1 / 96 | {} | musical |
| sound.909.HT.1 | Decay | [0, 127] | 1 / 97 | {} | musical |
| sound.909.HT.2 | Level | [0, 127] | 1 / 98 | {} | musical |
| sound.909.HT.3 | Attack | [0, 127] | 1 / 99 | {} | musical |
| sound.909.HT.4 | Drive | [0, 127] | 1 / 100 | {} | musical |
| sound.909.HT.5 | Dist | [0, 6] | 1 / 101 | {} | musical |
| sound.909.HT.6 | Rev | [0, 127] | 1 / 102 | {} | musical |
| sound.909.HT.7 | Dly | [0, 127] | 1 / 103 | {} | musical |
| sound.909.HT.8 | Pan | [0, 127] | 1 / 104 | {} | musical |
| sound.909.RS.0 | Level | [0, 127] | 1 / 112 | {} | musical |
| sound.909.RS.1 | Tune | [0, 127] | 1 / 113 | {} | musical |
| sound.909.RS.2 | Drive | [0, 127] | 1 / 114 | {} | musical |
| sound.909.RS.3 | Dist | [0, 6] | 1 / 115 | {} | musical |
| sound.909.RS.4 | Rev | [0, 127] | 1 / 116 | {} | musical |
| sound.909.RS.5 | Dly | [0, 127] | 1 / 117 | {} | musical |
| sound.909.RS.6 | Pan | [0, 127] | 1 / 118 | {} | musical |
| sound.909.CP.0 | Level | [0, 127] | 2 / 8 | {} | musical |
| sound.909.CP.1 | Tune | [0, 127] | 2 / 9 | {} | musical |
| sound.909.CP.2 | Tail | [0, 127] | 2 / 10 | {} | musical |
| sound.909.CP.3 | Drive | [0, 127] | 2 / 11 | {} | musical |
| sound.909.CP.4 | Dist | [0, 6] | 2 / 12 | {} | musical |
| sound.909.CP.5 | Rev | [0, 127] | 2 / 13 | {} | musical |
| sound.909.CP.6 | Dly | [0, 127] | 2 / 14 | {} | musical |
| sound.909.CP.7 | Pan | [0, 127] | 2 / 15 | {} | musical |
| sound.909.CH.0 | Decay | [0, 127] | 2 / 24 | {} | musical |
| sound.909.CH.1 | Level | [0, 127] | 2 / 25 | {} | musical |
| sound.909.CH.2 | Tune | [0, 127] | 2 / 26 | {} | musical |
| sound.909.CH.3 | Drive | [0, 127] | 2 / 27 | {} | musical |
| sound.909.CH.4 | Dist | [0, 6] | 2 / 28 | {} | musical |
| sound.909.CH.5 | Rev | [0, 127] | 2 / 29 | {} | musical |
| sound.909.CH.6 | Dly | [0, 127] | 2 / 30 | {} | musical |
| sound.909.CH.7 | Pan | [0, 127] | 2 / 31 | {} | musical |
| sound.909.OH.0 | Decay | [0, 127] | 2 / 72 | {} | musical |
| sound.909.OH.1 | Level | [0, 127] | 2 / 73 | {} | musical |
| sound.909.OH.2 | Tune | [0, 127] | 2 / 74 | {} | musical |
| sound.909.OH.3 | Drive | [0, 127] | 2 / 75 | {} | musical |
| sound.909.OH.4 | Dist | [0, 6] | 2 / 76 | {} | musical |
| sound.909.OH.5 | Rev | [0, 127] | 2 / 77 | {} | musical |
| sound.909.OH.6 | Dly | [0, 127] | 2 / 78 | {} | musical |
| sound.909.OH.7 | Pan | [0, 127] | 2 / 79 | {} | musical |
| sound.909.CR.0 | Tune | [0, 127] | 2 / 88 | {} | musical |
| sound.909.CR.1 | Level | [0, 127] | 2 / 89 | {} | musical |
| sound.909.CR.2 | Decay | [0, 127] | 2 / 90 | {} | musical |
| sound.909.CR.3 | Drive | [0, 127] | 2 / 91 | {} | musical |
| sound.909.CR.4 | Dist | [0, 6] | 2 / 92 | {} | musical |
| sound.909.CR.5 | Rev | [0, 127] | 2 / 93 | {} | musical |
| sound.909.CR.6 | Dly | [0, 127] | 2 / 94 | {} | musical |
| sound.909.CR.7 | Pan | [0, 127] | 2 / 95 | {} | musical |
| sound.909.RD.0 | Tune | [0, 127] | 2 / 104 | {} | musical |
| sound.909.RD.1 | Level | [0, 127] | 2 / 105 | {} | musical |
| sound.909.RD.2 | Decay | [0, 127] | 2 / 106 | {} | musical |
| sound.909.RD.3 | Drive | [0, 127] | 2 / 107 | {} | musical |
| sound.909.RD.4 | Dist | [0, 6] | 2 / 108 | {} | musical |
| sound.909.RD.5 | Rev | [0, 127] | 2 / 109 | {} | musical |
| sound.909.RD.6 | Dly | [0, 127] | 2 / 110 | {} | musical |
| sound.909.RD.7 | Pan | [0, 127] | 2 / 111 | {} | musical |
| sound.909.KIT.0 | Accent | [0, 127] | 3 / 0 | {} | musical |
| sound.909.KIT.1 | Veloc | [0, 127] | 3 / 1 | {} | musical |
| sound.808.BD.0 | Level | [0, 127] | 3 / 16 | {} | musical |
| sound.808.BD.1 | Tone | [0, 127] | 3 / 17 | {} | musical |
| sound.808.BD.2 | Decay | [0, 127] | 3 / 18 | {} | musical |
| sound.808.BD.3 | Tune | [0, 127] | 3 / 19 | {} | musical |
| sound.808.BD.4 | Attack | [0, 127] | 3 / 20 | {} | musical |
| sound.808.BD.5 | Drive | [0, 127] | 3 / 21 | {} | musical |
| sound.808.BD.6 | Dist | [0, 6] | 3 / 22 | {} | musical |
| sound.808.BD.7 | Rev | [0, 127] | 3 / 23 | {} | musical |
| sound.808.BD.8 | Dly | [0, 127] | 3 / 24 | {} | musical |
| sound.808.BD.9 | Pan | [0, 127] | 3 / 25 | {} | musical |
| sound.808.SD.0 | Level | [0, 127] | 3 / 64 | {} | musical |
| sound.808.SD.1 | Tone | [0, 127] | 3 / 65 | {} | musical |
| sound.808.SD.2 | Snappy | [0, 127] | 3 / 66 | {} | musical |
| sound.808.SD.3 | Tune | [0, 127] | 3 / 67 | {} | musical |
| sound.808.SD.4 | Decay | [0, 127] | 3 / 68 | {} | musical |
| sound.808.SD.5 | Drive | [0, 127] | 3 / 69 | {} | musical |
| sound.808.SD.6 | Dist | [0, 6] | 3 / 70 | {} | musical |
| sound.808.SD.7 | Rev | [0, 127] | 3 / 71 | {} | musical |
| sound.808.SD.8 | Dly | [0, 127] | 3 / 72 | {} | musical |
| sound.808.SD.9 | Pan | [0, 127] | 3 / 73 | {} | musical |
| sound.808.LT.0 | Level | [0, 127] | 3 / 80 | {} | musical |
| sound.808.LT.1 | Tune | [0, 127] | 3 / 81 | {} | musical |
| sound.808.LT.2 | Decay | [0, 127] | 3 / 82 | {} | musical |
| sound.808.LT.3 | Sound | [0, 1] | 3 / 83 | {} | musical |
| sound.808.LT.4 | Drive | [0, 127] | 3 / 84 | {} | musical |
| sound.808.LT.5 | Dist | [0, 6] | 3 / 85 | {} | musical |
| sound.808.LT.6 | Rev | [0, 127] | 3 / 86 | {} | musical |
| sound.808.LT.7 | Dly | [0, 127] | 3 / 87 | {} | musical |
| sound.808.LT.8 | Pan | [0, 127] | 3 / 88 | {} | musical |
| sound.808.MT.0 | Level | [0, 127] | 3 / 96 | {} | musical |
| sound.808.MT.1 | Tune | [0, 127] | 3 / 97 | {} | musical |
| sound.808.MT.2 | Decay | [0, 127] | 3 / 98 | {} | musical |
| sound.808.MT.3 | Sound | [0, 1] | 3 / 99 | {} | musical |
| sound.808.MT.4 | Drive | [0, 127] | 3 / 100 | {} | musical |
| sound.808.MT.5 | Dist | [0, 6] | 3 / 101 | {} | musical |
| sound.808.MT.6 | Rev | [0, 127] | 3 / 102 | {} | musical |
| sound.808.MT.7 | Dly | [0, 127] | 3 / 103 | {} | musical |
| sound.808.MT.8 | Pan | [0, 127] | 3 / 104 | {} | musical |
| sound.808.HT.0 | Level | [0, 127] | 3 / 112 | {} | musical |
| sound.808.HT.1 | Tune | [0, 127] | 3 / 113 | {} | musical |
| sound.808.HT.2 | Decay | [0, 127] | 3 / 114 | {} | musical |
| sound.808.HT.3 | Sound | [0, 1] | 3 / 115 | {} | musical |
| sound.808.HT.4 | Drive | [0, 127] | 3 / 116 | {} | musical |
| sound.808.HT.5 | Dist | [0, 6] | 3 / 117 | {} | musical |
| sound.808.HT.6 | Rev | [0, 127] | 3 / 118 | {} | musical |
| sound.808.HT.7 | Dly | [0, 127] | 3 / 119 | {} | musical |
| sound.808.HT.8 | Pan | [0, 127] | 4 / 0 | {} | musical |
| sound.808.RS.0 | Level | [0, 127] | 4 / 8 | {} | musical |
| sound.808.RS.1 | Tune | [0, 127] | 4 / 9 | {} | musical |
| sound.808.RS.2 | Decay | [0, 127] | 4 / 10 | {} | musical |
| sound.808.RS.3 | Sound | [0, 1] | 4 / 11 | {} | musical |
| sound.808.RS.4 | Drive | [0, 127] | 4 / 12 | {} | musical |
| sound.808.RS.5 | Dist | [0, 6] | 4 / 13 | {} | musical |
| sound.808.RS.6 | Rev | [0, 127] | 4 / 14 | {} | musical |
| sound.808.RS.7 | Dly | [0, 127] | 4 / 15 | {} | musical |
| sound.808.RS.8 | Pan | [0, 127] | 4 / 16 | {} | musical |
| sound.808.CP.0 | Level | [0, 127] | 4 / 24 | {} | musical |
| sound.808.CP.1 | Tune | [0, 127] | 4 / 25 | {} | musical |
| sound.808.CP.2 | Decay | [0, 127] | 4 / 26 | {} | musical |
| sound.808.CP.3 | Attack | [0, 127] | 4 / 27 | {} | musical |
| sound.808.CP.4 | Sound | [0, 1] | 4 / 28 | {} | musical |
| sound.808.CP.5 | Drive | [0, 127] | 4 / 29 | {} | musical |
| sound.808.CP.6 | Dist | [0, 6] | 4 / 30 | {} | musical |
| sound.808.CP.7 | Rev | [0, 127] | 4 / 31 | {} | musical |
| sound.808.CP.8 | Dly | [0, 127] | 4 / 64 | {} | musical |
| sound.808.CP.9 | Pan | [0, 127] | 4 / 65 | {} | musical |
| sound.808.CB.0 | Level | [0, 127] | 4 / 72 | {} | musical |
| sound.808.CB.1 | Tune | [0, 127] | 4 / 73 | {} | musical |
| sound.808.CB.2 | Decay | [0, 127] | 4 / 74 | {} | musical |
| sound.808.CB.3 | Drive | [0, 127] | 4 / 75 | {} | musical |
| sound.808.CB.4 | Dist | [0, 6] | 4 / 76 | {} | musical |
| sound.808.CB.5 | Rev | [0, 127] | 4 / 77 | {} | musical |
| sound.808.CB.6 | Dly | [0, 127] | 4 / 78 | {} | musical |
| sound.808.CB.7 | Pan | [0, 127] | 4 / 79 | {} | musical |
| sound.808.CY.0 | Level | [0, 127] | 4 / 88 | {} | musical |
| sound.808.CY.1 | Decay | [0, 127] | 4 / 89 | {} | musical |
| sound.808.CY.2 | Tune | [0, 127] | 4 / 90 | {} | musical |
| sound.808.CY.3 | Drive | [0, 127] | 4 / 91 | {} | musical |
| sound.808.CY.4 | Dist | [0, 6] | 4 / 92 | {} | musical |
| sound.808.CY.5 | Rev | [0, 127] | 4 / 93 | {} | musical |
| sound.808.CY.6 | Dly | [0, 127] | 4 / 94 | {} | musical |
| sound.808.CY.7 | Pan | [0, 127] | 4 / 95 | {} | musical |
| sound.808.OH.0 | Level | [0, 127] | 4 / 104 | {} | musical |
| sound.808.OH.1 | Decay | [0, 127] | 4 / 105 | {} | musical |
| sound.808.OH.2 | Tune | [0, 127] | 4 / 106 | {} | musical |
| sound.808.OH.3 | Drive | [0, 127] | 4 / 107 | {} | musical |
| sound.808.OH.4 | Dist | [0, 6] | 4 / 108 | {} | musical |
| sound.808.OH.5 | Rev | [0, 127] | 4 / 109 | {} | musical |
| sound.808.OH.6 | Dly | [0, 127] | 4 / 110 | {} | musical |
| sound.808.OH.7 | Pan | [0, 127] | 4 / 111 | {} | musical |
| sound.808.CH.0 | Level | [0, 127] | 5 / 0 | {} | musical |
| sound.808.CH.1 | Tune | [0, 127] | 5 / 1 | {} | musical |
| sound.808.CH.2 | Decay | [0, 127] | 5 / 2 | {} | musical |
| sound.808.CH.3 | Drive | [0, 127] | 5 / 3 | {} | musical |
| sound.808.CH.4 | Dist | [0, 6] | 5 / 4 | {} | musical |
| sound.808.CH.5 | Rev | [0, 127] | 5 / 5 | {} | musical |
| sound.808.CH.6 | Dly | [0, 127] | 5 / 6 | {} | musical |
| sound.808.CH.7 | Pan | [0, 127] | 5 / 7 | {} | musical |
| sound.808.KIT.0 | Level | [0, 127] | 5 / 16 | {} | musical |
| sound.808.KIT.1 | Accent | [0, 127] | 5 / 17 | {} | musical |
| sound.303.A.0 | Cutoff | [0, 127] | 5 / 64 | {} | musical |
| sound.303.A.1 | Reso | [0, 127] | 5 / 65 | {} | musical |
| sound.303.A.2 | EnvMod | [0, 127] | 5 / 66 | {} | musical |
| sound.303.A.3 | Decay | [0, 127] | 5 / 67 | {} | musical |
| sound.303.A.4 | Accent | [0, 127] | 5 / 68 | {} | musical |
| sound.303.A.5 | Wave | [0, 1] | 5 / 69 | {} | musical |
| sound.303.A.6 | Tune | [0, 127] | 5 / 70 | {} | musical |
| sound.303.A.7 | Volume | [0, 127] | 5 / 71 | {} | musical |
| sound.303.A.8 | Drive | [0, 127] | 5 / 72 | {} | musical |
| sound.303.A.9 | DrvTyp | [0, 2] | 5 / 73 | {} | musical |
| sound.303.A.10 | Slide | [0, 127] | 5 / 74 | {} | musical |
| sound.303.A.11 | AccDec | [0, 127] | 5 / 75 | {} | musical |
| sound.303.B.0 | Cutoff | [0, 127] | 5 / 80 | {} | musical |
| sound.303.B.1 | Reso | [0, 127] | 5 / 81 | {} | musical |
| sound.303.B.2 | EnvMod | [0, 127] | 5 / 82 | {} | musical |
| sound.303.B.3 | Decay | [0, 127] | 5 / 83 | {} | musical |
| sound.303.B.4 | Accent | [0, 127] | 5 / 84 | {} | musical |
| sound.303.B.5 | Wave | [0, 1] | 5 / 85 | {} | musical |
| sound.303.B.6 | Tune | [0, 127] | 5 / 86 | {} | musical |
| sound.303.B.7 | Volume | [0, 127] | 5 / 87 | {} | musical |
| sound.303.B.8 | Drive | [0, 127] | 5 / 88 | {} | musical |
| sound.303.B.9 | DrvTyp | [0, 2] | 5 / 89 | {} | musical |
| sound.303.B.10 | Slide | [0, 127] | 5 / 90 | {} | musical |
| sound.303.B.11 | AccDec | [0, 127] | 5 / 91 | {} | musical |
| sound.BREAK.GLOBAL.0 | Level | [0, 127] | 5 / 96 | {} | musical |
| sound.BREAK.GLOBAL.1 | Pitch | [0, 24] | 5 / 97 | {} | musical |
| sound.FX.GLOBAL.0 | Volume | [0, 127] | 5 / 112 | {} | musical |
| sound.FX.GLOBAL.1 | Dist | [0, 7] | 5 / 113 | {} | musical |
| sound.FX.GLOBAL.2 | Drive | [0, 127] | 5 / 114 | {} | musical |
| sound.FX.GLOBAL.3 | Comp | [0, 127] | 5 / 115 | {} | musical |
| sound.FX.GLOBAL.4 | RvDec | [0, 127] | 5 / 116 | {} | musical |
| sound.FX.GLOBAL.5 | RvTone | [0, 127] | 5 / 117 | {} | musical |
| sound.FX.GLOBAL.6 | RvHPF | [0, 127] | 5 / 118 | {} | musical |
| sound.FX.GLOBAL.7 | RvLvl | [0, 127] | 5 / 119 | {} | musical |
| sound.FX.GLOBAL.8 | DlTime | [0, 12] | 6 / 0 | {} | musical |
| sound.FX.GLOBAL.9 | DlFdbk | [0, 127] | 6 / 1 | {} | musical |
| sound.FX.GLOBAL.10 | DlTone | [0, 127] | 6 / 2 | {} | musical |
| sound.FX.GLOBAL.11 | DlLvl | [0, 127] | 6 / 3 | {} | musical |
| sound.FX.GLOBAL.12 | DlHPF | [0, 127] | 6 / 4 | {} | musical |
| sound.FX.GLOBAL.13 | DlType | [0, 1] | 6 / 5 | {} | musical |
| sound.FX.GLOBAL.14 | Wear | [0, 127] | 6 / 6 | {} | musical |
| sound.FX.GLOBAL.15 | Ping | [0, 1] | 6 / 7 | {} | musical |
| sound.MIX.909.0 | LEVEL | [0, 127] | 6 / 24 | {} | musical |
| sound.MIX.909.1 | REV | [0, 127] | 6 / 25 | {} | musical |
| sound.MIX.909.2 | DLY | [0, 127] | 6 / 26 | {} | musical |
| sound.MIX.909.3 | PAN | [0, 127] | 6 / 27 | {} | musical |
| sound.MIX.808.0 | LEVEL | [0, 127] | 6 / 72 | {} | musical |
| sound.MIX.808.1 | REV | [0, 127] | 6 / 73 | {} | musical |
| sound.MIX.808.2 | DLY | [0, 127] | 6 / 74 | {} | musical |
| sound.MIX.808.3 | PAN | [0, 127] | 6 / 75 | {} | musical |
| sound.MIX.303A.0 | LEVEL | [0, 127] | 6 / 88 | {} | musical |
| sound.MIX.303A.1 | REV | [0, 127] | 6 / 89 | {} | musical |
| sound.MIX.303A.2 | DLY | [0, 127] | 6 / 90 | {} | musical |
| sound.MIX.303A.3 | PAN | [0, 127] | 6 / 91 | {} | musical |
| sound.MIX.303B.0 | LEVEL | [0, 127] | 6 / 104 | {} | musical |
| sound.MIX.303B.1 | REV | [0, 127] | 6 / 105 | {} | musical |
| sound.MIX.303B.2 | DLY | [0, 127] | 6 / 106 | {} | musical |
| sound.MIX.303B.3 | PAN | [0, 127] | 6 / 107 | {} | musical |
| sound.MIX.BREAK.0 | LEVEL | [0, 127] | 7 / 0 | {} | musical |
| sound.MIX.BREAK.1 | REV | [0, 127] | 7 / 1 | {} | musical |
| sound.MIX.BREAK.2 | DLY | [0, 127] | 7 / 2 | {} | musical |
| sound.MIX.BREAK.3 | PAN | [0, 127] | 7 / 3 | {} | musical |
| sound.MASTER.GLOBAL.0 | THRESH | [0, 127] | 7 / 16 | {} | musical |
| sound.MASTER.GLOBAL.1 | RATIO | [0, 9] | 7 / 17 | {} | musical |
| sound.MASTER.GLOBAL.2 | ATTACK | [0, 127] | 7 / 18 | {} | musical |
| sound.MASTER.GLOBAL.3 | RELEAS | [0, 127] | 7 / 19 | {} | musical |
| sound.MASTER.GLOBAL.4 | MAKEUP | [0, 127] | 7 / 20 | {} | musical |
| sound.MASTER.GLOBAL.5 | MIX | [0, 127] | 7 / 21 | {} | musical |
| sound.MASTER.GLOBAL.6 | PUMP | [0, 127] | 7 / 22 | {} | musical |
| sound.MASTER.GLOBAL.7 | PUMP BY | [0, 2] | 7 / 23 | {} | musical |
| sound.MASTER.GLOBAL.8 | FILTER | [0, 3] | 7 / 24 | {} | musical |
| sound.MASTER.GLOBAL.9 | CUTOFF | [0, 127] | 7 / 25 | {} | musical |
| sound.MASTER.GLOBAL.10 | RESO | [0, 127] | 7 / 26 | {} | musical |
| sound.MASTER.GLOBAL.11 | LIMIT | [0, 1] | 7 / 27 | {} | musical |
| sound.MASTER.GLOBAL.12 | COMP | [0, 127] | 7 / 28 | {} | musical |
| sound.MASTER.GLOBAL.13 | FILTER | [0, 127] | 7 / 29 | {} | musical |
| strip.0.highpass | highpass | [20, 20000] | 7 / 80 | {} | musical |
| strip.0.lowpass | lowpass | [20, 20000] | 7 / 81 | {} | musical |
| strip.0.boost | boost | [0, 24] | 7 / 82 | {} | musical |
| strip.0.ceiling | ceiling | [0, 48] | 7 / 83 | {} | musical |
| strip.0.release | release | [10, 250] | 7 / 84 | {} | musical |
| strip.0.flags | flags | [0, 15] | 7 / 85 | {} | musical |
| strip.0.enabled | enabled | [0, 1] | 7 / 86 | {} | musical |
| strip.0.highpass_enabled | highpass_enabled | [0, 1] | 7 / 87 | {} | musical |
| strip.0.lowpass_enabled | lowpass_enabled | [0, 1] | 7 / 88 | {} | musical |
| strip.0.limiter_enabled | limiter_enabled | [0, 1] | 7 / 89 | {} | musical |
| strip.1.highpass | highpass | [20, 20000] | 7 / 92 | {} | musical |
| strip.1.lowpass | lowpass | [20, 20000] | 7 / 93 | {} | musical |
| strip.1.boost | boost | [0, 24] | 7 / 94 | {} | musical |
| strip.1.ceiling | ceiling | [0, 48] | 7 / 95 | {} | musical |
| strip.1.release | release | [10, 250] | 7 / 96 | {} | musical |
| strip.1.flags | flags | [0, 15] | 7 / 97 | {} | musical |
| strip.1.enabled | enabled | [0, 1] | 7 / 98 | {} | musical |
| strip.1.highpass_enabled | highpass_enabled | [0, 1] | 7 / 99 | {} | musical |
| strip.1.lowpass_enabled | lowpass_enabled | [0, 1] | 7 / 100 | {} | musical |
| strip.1.limiter_enabled | limiter_enabled | [0, 1] | 7 / 101 | {} | musical |
| strip.2.highpass | highpass | [20, 20000] | 7 / 104 | {} | musical |
| strip.2.lowpass | lowpass | [20, 20000] | 7 / 105 | {} | musical |
| strip.2.boost | boost | [0, 24] | 7 / 106 | {} | musical |
| strip.2.ceiling | ceiling | [0, 48] | 7 / 107 | {} | musical |
| strip.2.release | release | [10, 250] | 7 / 108 | {} | musical |
| strip.2.flags | flags | [0, 15] | 7 / 109 | {} | musical |
| strip.2.enabled | enabled | [0, 1] | 7 / 110 | {} | musical |
| strip.2.highpass_enabled | highpass_enabled | [0, 1] | 7 / 111 | {} | musical |
| strip.2.lowpass_enabled | lowpass_enabled | [0, 1] | 7 / 112 | {} | musical |
| strip.2.limiter_enabled | limiter_enabled | [0, 1] | 7 / 113 | {} | musical |
| strip.3.highpass | highpass | [20, 20000] | 7 / 116 | {} | musical |
| strip.3.lowpass | lowpass | [20, 20000] | 7 / 117 | {} | musical |
| strip.3.boost | boost | [0, 24] | 7 / 118 | {} | musical |
| strip.3.ceiling | ceiling | [0, 48] | 7 / 119 | {} | musical |
| strip.3.release | release | [10, 250] | 8 / 0 | {} | musical |
| strip.3.flags | flags | [0, 15] | 8 / 1 | {} | musical |
| strip.3.enabled | enabled | [0, 1] | 8 / 2 | {} | musical |
| strip.3.highpass_enabled | highpass_enabled | [0, 1] | 8 / 3 | {} | musical |
| strip.3.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 4 | {} | musical |
| strip.3.limiter_enabled | limiter_enabled | [0, 1] | 8 / 5 | {} | musical |
| strip.4.highpass | highpass | [20, 20000] | 8 / 8 | {} | musical |
| strip.4.lowpass | lowpass | [20, 20000] | 8 / 9 | {} | musical |
| strip.4.boost | boost | [0, 24] | 8 / 10 | {} | musical |
| strip.4.ceiling | ceiling | [0, 48] | 8 / 11 | {} | musical |
| strip.4.release | release | [10, 250] | 8 / 12 | {} | musical |
| strip.4.flags | flags | [0, 15] | 8 / 13 | {} | musical |
| strip.4.enabled | enabled | [0, 1] | 8 / 14 | {} | musical |
| strip.4.highpass_enabled | highpass_enabled | [0, 1] | 8 / 15 | {} | musical |
| strip.4.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 16 | {} | musical |
| strip.4.limiter_enabled | limiter_enabled | [0, 1] | 8 / 17 | {} | musical |
| strip.5.highpass | highpass | [20, 20000] | 8 / 20 | {} | musical |
| strip.5.lowpass | lowpass | [20, 20000] | 8 / 21 | {} | musical |
| strip.5.boost | boost | [0, 24] | 8 / 22 | {} | musical |
| strip.5.ceiling | ceiling | [0, 48] | 8 / 23 | {} | musical |
| strip.5.release | release | [10, 250] | 8 / 24 | {} | musical |
| strip.5.flags | flags | [0, 15] | 8 / 25 | {} | musical |
| strip.5.enabled | enabled | [0, 1] | 8 / 26 | {} | musical |
| strip.5.highpass_enabled | highpass_enabled | [0, 1] | 8 / 27 | {} | musical |
| strip.5.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 28 | {} | musical |
| strip.5.limiter_enabled | limiter_enabled | [0, 1] | 8 / 29 | {} | musical |
| strip.6.highpass | highpass | [20, 20000] | 8 / 64 | {} | musical |
| strip.6.lowpass | lowpass | [20, 20000] | 8 / 65 | {} | musical |
| strip.6.boost | boost | [0, 24] | 8 / 66 | {} | musical |
| strip.6.ceiling | ceiling | [0, 48] | 8 / 67 | {} | musical |
| strip.6.release | release | [10, 250] | 8 / 68 | {} | musical |
| strip.6.flags | flags | [0, 15] | 8 / 69 | {} | musical |
| strip.6.enabled | enabled | [0, 1] | 8 / 70 | {} | musical |
| strip.6.highpass_enabled | highpass_enabled | [0, 1] | 8 / 71 | {} | musical |
| strip.6.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 72 | {} | musical |
| strip.6.limiter_enabled | limiter_enabled | [0, 1] | 8 / 73 | {} | musical |
| strip.7.highpass | highpass | [20, 20000] | 8 / 76 | {} | musical |
| strip.7.lowpass | lowpass | [20, 20000] | 8 / 77 | {} | musical |
| strip.7.boost | boost | [0, 24] | 8 / 78 | {} | musical |
| strip.7.ceiling | ceiling | [0, 48] | 8 / 79 | {} | musical |
| strip.7.release | release | [10, 250] | 8 / 80 | {} | musical |
| strip.7.flags | flags | [0, 15] | 8 / 81 | {} | musical |
| strip.7.enabled | enabled | [0, 1] | 8 / 82 | {} | musical |
| strip.7.highpass_enabled | highpass_enabled | [0, 1] | 8 / 83 | {} | musical |
| strip.7.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 84 | {} | musical |
| strip.7.limiter_enabled | limiter_enabled | [0, 1] | 8 / 85 | {} | musical |
| strip.8.highpass | highpass | [20, 20000] | 8 / 88 | {} | musical |
| strip.8.lowpass | lowpass | [20, 20000] | 8 / 89 | {} | musical |
| strip.8.boost | boost | [0, 24] | 8 / 90 | {} | musical |
| strip.8.ceiling | ceiling | [0, 48] | 8 / 91 | {} | musical |
| strip.8.release | release | [10, 250] | 8 / 92 | {} | musical |
| strip.8.flags | flags | [0, 15] | 8 / 93 | {} | musical |
| strip.8.enabled | enabled | [0, 1] | 8 / 94 | {} | musical |
| strip.8.highpass_enabled | highpass_enabled | [0, 1] | 8 / 95 | {} | musical |
| strip.8.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 96 | {} | musical |
| strip.8.limiter_enabled | limiter_enabled | [0, 1] | 8 / 97 | {} | musical |
| strip.9.highpass | highpass | [20, 20000] | 8 / 100 | {} | musical |
| strip.9.lowpass | lowpass | [20, 20000] | 8 / 101 | {} | musical |
| strip.9.boost | boost | [0, 24] | 8 / 102 | {} | musical |
| strip.9.ceiling | ceiling | [0, 48] | 8 / 103 | {} | musical |
| strip.9.release | release | [10, 250] | 8 / 104 | {} | musical |
| strip.9.flags | flags | [0, 15] | 8 / 105 | {} | musical |
| strip.9.enabled | enabled | [0, 1] | 8 / 106 | {} | musical |
| strip.9.highpass_enabled | highpass_enabled | [0, 1] | 8 / 107 | {} | musical |
| strip.9.lowpass_enabled | lowpass_enabled | [0, 1] | 8 / 108 | {} | musical |
| strip.9.limiter_enabled | limiter_enabled | [0, 1] | 8 / 109 | {} | musical |
| strip.10.highpass | highpass | [20, 20000] | 8 / 112 | {} | musical |
| strip.10.lowpass | lowpass | [20, 20000] | 8 / 113 | {} | musical |
| strip.10.boost | boost | [0, 24] | 8 / 114 | {} | musical |
| strip.10.ceiling | ceiling | [0, 48] | 8 / 115 | {} | musical |
| strip.10.release | release | [10, 250] | 8 / 116 | {} | musical |
| strip.10.flags | flags | [0, 15] | 8 / 117 | {} | musical |
| strip.10.enabled | enabled | [0, 1] | 8 / 118 | {} | musical |
| strip.10.highpass_enabled | highpass_enabled | [0, 1] | 8 / 119 | {} | musical |
| strip.10.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 0 | {} | musical |
| strip.10.limiter_enabled | limiter_enabled | [0, 1] | 9 / 1 | {} | musical |
| strip.11.highpass | highpass | [20, 20000] | 9 / 4 | {} | musical |
| strip.11.lowpass | lowpass | [20, 20000] | 9 / 5 | {} | musical |
| strip.11.boost | boost | [0, 24] | 9 / 6 | {} | musical |
| strip.11.ceiling | ceiling | [0, 48] | 9 / 7 | {} | musical |
| strip.11.release | release | [10, 250] | 9 / 8 | {} | musical |
| strip.11.flags | flags | [0, 15] | 9 / 9 | {} | musical |
| strip.11.enabled | enabled | [0, 1] | 9 / 10 | {} | musical |
| strip.11.highpass_enabled | highpass_enabled | [0, 1] | 9 / 11 | {} | musical |
| strip.11.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 12 | {} | musical |
| strip.11.limiter_enabled | limiter_enabled | [0, 1] | 9 / 13 | {} | musical |
| strip.12.highpass | highpass | [20, 20000] | 9 / 16 | {} | musical |
| strip.12.lowpass | lowpass | [20, 20000] | 9 / 17 | {} | musical |
| strip.12.boost | boost | [0, 24] | 9 / 18 | {} | musical |
| strip.12.ceiling | ceiling | [0, 48] | 9 / 19 | {} | musical |
| strip.12.release | release | [10, 250] | 9 / 20 | {} | musical |
| strip.12.flags | flags | [0, 15] | 9 / 21 | {} | musical |
| strip.12.enabled | enabled | [0, 1] | 9 / 22 | {} | musical |
| strip.12.highpass_enabled | highpass_enabled | [0, 1] | 9 / 23 | {} | musical |
| strip.12.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 24 | {} | musical |
| strip.12.limiter_enabled | limiter_enabled | [0, 1] | 9 / 25 | {} | musical |
| strip.13.highpass | highpass | [20, 20000] | 9 / 28 | {} | musical |
| strip.13.lowpass | lowpass | [20, 20000] | 9 / 29 | {} | musical |
| strip.13.boost | boost | [0, 24] | 9 / 30 | {} | musical |
| strip.13.ceiling | ceiling | [0, 48] | 9 / 31 | {} | musical |
| strip.13.release | release | [10, 250] | 9 / 64 | {} | musical |
| strip.13.flags | flags | [0, 15] | 9 / 65 | {} | musical |
| strip.13.enabled | enabled | [0, 1] | 9 / 66 | {} | musical |
| strip.13.highpass_enabled | highpass_enabled | [0, 1] | 9 / 67 | {} | musical |
| strip.13.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 68 | {} | musical |
| strip.13.limiter_enabled | limiter_enabled | [0, 1] | 9 / 69 | {} | musical |
| strip.14.highpass | highpass | [20, 20000] | 9 / 72 | {} | musical |
| strip.14.lowpass | lowpass | [20, 20000] | 9 / 73 | {} | musical |
| strip.14.boost | boost | [0, 24] | 9 / 74 | {} | musical |
| strip.14.ceiling | ceiling | [0, 48] | 9 / 75 | {} | musical |
| strip.14.release | release | [10, 250] | 9 / 76 | {} | musical |
| strip.14.flags | flags | [0, 15] | 9 / 77 | {} | musical |
| strip.14.enabled | enabled | [0, 1] | 9 / 78 | {} | musical |
| strip.14.highpass_enabled | highpass_enabled | [0, 1] | 9 / 79 | {} | musical |
| strip.14.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 80 | {} | musical |
| strip.14.limiter_enabled | limiter_enabled | [0, 1] | 9 / 81 | {} | musical |
| strip.15.highpass | highpass | [20, 20000] | 9 / 84 | {} | musical |
| strip.15.lowpass | lowpass | [20, 20000] | 9 / 85 | {} | musical |
| strip.15.boost | boost | [0, 24] | 9 / 86 | {} | musical |
| strip.15.ceiling | ceiling | [0, 48] | 9 / 87 | {} | musical |
| strip.15.release | release | [10, 250] | 9 / 88 | {} | musical |
| strip.15.flags | flags | [0, 15] | 9 / 89 | {} | musical |
| strip.15.enabled | enabled | [0, 1] | 9 / 90 | {} | musical |
| strip.15.highpass_enabled | highpass_enabled | [0, 1] | 9 / 91 | {} | musical |
| strip.15.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 92 | {} | musical |
| strip.15.limiter_enabled | limiter_enabled | [0, 1] | 9 / 93 | {} | musical |
| strip.16.highpass | highpass | [20, 20000] | 9 / 96 | {} | musical |
| strip.16.lowpass | lowpass | [20, 20000] | 9 / 97 | {} | musical |
| strip.16.boost | boost | [0, 24] | 9 / 98 | {} | musical |
| strip.16.ceiling | ceiling | [0, 48] | 9 / 99 | {} | musical |
| strip.16.release | release | [10, 250] | 9 / 100 | {} | musical |
| strip.16.flags | flags | [0, 15] | 9 / 101 | {} | musical |
| strip.16.enabled | enabled | [0, 1] | 9 / 102 | {} | musical |
| strip.16.highpass_enabled | highpass_enabled | [0, 1] | 9 / 103 | {} | musical |
| strip.16.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 104 | {} | musical |
| strip.16.limiter_enabled | limiter_enabled | [0, 1] | 9 / 105 | {} | musical |
| strip.17.highpass | highpass | [20, 20000] | 9 / 108 | {} | musical |
| strip.17.lowpass | lowpass | [20, 20000] | 9 / 109 | {} | musical |
| strip.17.boost | boost | [0, 24] | 9 / 110 | {} | musical |
| strip.17.ceiling | ceiling | [0, 48] | 9 / 111 | {} | musical |
| strip.17.release | release | [10, 250] | 9 / 112 | {} | musical |
| strip.17.flags | flags | [0, 15] | 9 / 113 | {} | musical |
| strip.17.enabled | enabled | [0, 1] | 9 / 114 | {} | musical |
| strip.17.highpass_enabled | highpass_enabled | [0, 1] | 9 / 115 | {} | musical |
| strip.17.lowpass_enabled | lowpass_enabled | [0, 1] | 9 / 116 | {} | musical |
| strip.17.limiter_enabled | limiter_enabled | [0, 1] | 9 / 117 | {} | musical |
| strip.18.highpass | highpass | [20, 20000] | 10 / 0 | {} | musical |
| strip.18.lowpass | lowpass | [20, 20000] | 10 / 1 | {} | musical |
| strip.18.boost | boost | [0, 24] | 10 / 2 | {} | musical |
| strip.18.ceiling | ceiling | [0, 48] | 10 / 3 | {} | musical |
| strip.18.release | release | [10, 250] | 10 / 4 | {} | musical |
| strip.18.flags | flags | [0, 15] | 10 / 5 | {} | musical |
| strip.18.enabled | enabled | [0, 1] | 10 / 6 | {} | musical |
| strip.18.highpass_enabled | highpass_enabled | [0, 1] | 10 / 7 | {} | musical |
| strip.18.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 8 | {} | musical |
| strip.18.limiter_enabled | limiter_enabled | [0, 1] | 10 / 9 | {} | musical |
| strip.19.highpass | highpass | [20, 20000] | 10 / 12 | {} | musical |
| strip.19.lowpass | lowpass | [20, 20000] | 10 / 13 | {} | musical |
| strip.19.boost | boost | [0, 24] | 10 / 14 | {} | musical |
| strip.19.ceiling | ceiling | [0, 48] | 10 / 15 | {} | musical |
| strip.19.release | release | [10, 250] | 10 / 16 | {} | musical |
| strip.19.flags | flags | [0, 15] | 10 / 17 | {} | musical |
| strip.19.enabled | enabled | [0, 1] | 10 / 18 | {} | musical |
| strip.19.highpass_enabled | highpass_enabled | [0, 1] | 10 / 19 | {} | musical |
| strip.19.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 20 | {} | musical |
| strip.19.limiter_enabled | limiter_enabled | [0, 1] | 10 / 21 | {} | musical |
| strip.20.highpass | highpass | [20, 20000] | 10 / 24 | {} | musical |
| strip.20.lowpass | lowpass | [20, 20000] | 10 / 25 | {} | musical |
| strip.20.boost | boost | [0, 24] | 10 / 26 | {} | musical |
| strip.20.ceiling | ceiling | [0, 48] | 10 / 27 | {} | musical |
| strip.20.release | release | [10, 250] | 10 / 28 | {} | musical |
| strip.20.flags | flags | [0, 15] | 10 / 29 | {} | musical |
| strip.20.enabled | enabled | [0, 1] | 10 / 30 | {} | musical |
| strip.20.highpass_enabled | highpass_enabled | [0, 1] | 10 / 31 | {} | musical |
| strip.20.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 64 | {} | musical |
| strip.20.limiter_enabled | limiter_enabled | [0, 1] | 10 / 65 | {} | musical |
| strip.21.highpass | highpass | [20, 20000] | 10 / 68 | {} | musical |
| strip.21.lowpass | lowpass | [20, 20000] | 10 / 69 | {} | musical |
| strip.21.boost | boost | [0, 24] | 10 / 70 | {} | musical |
| strip.21.ceiling | ceiling | [0, 48] | 10 / 71 | {} | musical |
| strip.21.release | release | [10, 250] | 10 / 72 | {} | musical |
| strip.21.flags | flags | [0, 15] | 10 / 73 | {} | musical |
| strip.21.enabled | enabled | [0, 1] | 10 / 74 | {} | musical |
| strip.21.highpass_enabled | highpass_enabled | [0, 1] | 10 / 75 | {} | musical |
| strip.21.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 76 | {} | musical |
| strip.21.limiter_enabled | limiter_enabled | [0, 1] | 10 / 77 | {} | musical |
| strip.22.highpass | highpass | [20, 20000] | 10 / 80 | {} | musical |
| strip.22.lowpass | lowpass | [20, 20000] | 10 / 81 | {} | musical |
| strip.22.boost | boost | [0, 24] | 10 / 82 | {} | musical |
| strip.22.ceiling | ceiling | [0, 48] | 10 / 83 | {} | musical |
| strip.22.release | release | [10, 250] | 10 / 84 | {} | musical |
| strip.22.flags | flags | [0, 15] | 10 / 85 | {} | musical |
| strip.22.enabled | enabled | [0, 1] | 10 / 86 | {} | musical |
| strip.22.highpass_enabled | highpass_enabled | [0, 1] | 10 / 87 | {} | musical |
| strip.22.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 88 | {} | musical |
| strip.22.limiter_enabled | limiter_enabled | [0, 1] | 10 / 89 | {} | musical |
| strip.23.highpass | highpass | [20, 20000] | 10 / 92 | {} | musical |
| strip.23.lowpass | lowpass | [20, 20000] | 10 / 93 | {} | musical |
| strip.23.boost | boost | [0, 24] | 10 / 94 | {} | musical |
| strip.23.ceiling | ceiling | [0, 48] | 10 / 95 | {} | musical |
| strip.23.release | release | [10, 250] | 10 / 96 | {} | musical |
| strip.23.flags | flags | [0, 15] | 10 / 97 | {} | musical |
| strip.23.enabled | enabled | [0, 1] | 10 / 98 | {} | musical |
| strip.23.highpass_enabled | highpass_enabled | [0, 1] | 10 / 99 | {} | musical |
| strip.23.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 100 | {} | musical |
| strip.23.limiter_enabled | limiter_enabled | [0, 1] | 10 / 101 | {} | musical |
| strip.24.highpass | highpass | [20, 20000] | 10 / 104 | {} | musical |
| strip.24.lowpass | lowpass | [20, 20000] | 10 / 105 | {} | musical |
| strip.24.boost | boost | [0, 24] | 10 / 106 | {} | musical |
| strip.24.ceiling | ceiling | [0, 48] | 10 / 107 | {} | musical |
| strip.24.release | release | [10, 250] | 10 / 108 | {} | musical |
| strip.24.flags | flags | [0, 15] | 10 / 109 | {} | musical |
| strip.24.enabled | enabled | [0, 1] | 10 / 110 | {} | musical |
| strip.24.highpass_enabled | highpass_enabled | [0, 1] | 10 / 111 | {} | musical |
| strip.24.lowpass_enabled | lowpass_enabled | [0, 1] | 10 / 112 | {} | musical |
| strip.24.limiter_enabled | limiter_enabled | [0, 1] | 10 / 113 | {} | musical |
| sync.global_mode | SYNC RATE MODE | [0, 1] | none | {} | nonmusical |
| sync.strip_override | RATE MODE | [0, 3] | none | {"strip": [0, 24]} | nonmusical |
| pref.SQ_SWING | SWING | [0, 100] | 11 / 20 | {"pattern": [0, 31]} | musical |
| pref.SQ_DLEN | LENGTH | [0, 63] | 11 / 21 | {"kit": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_DRATE | RATE | [0, 3] | 11 / 22 | {"kit": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_BLEN | LENGTH | [0, 63] | 11 / 23 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_BRATE | RATE | [0, 3] | 11 / 24 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_BDIR | DIR | [0, 3] | 11 / 25 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_BTRANS | TRANSP | [0, 48] | 11 / 26 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| pref.SQ_TEMPO | TEMPO | [0, 255] | 11 / 27 | {} | musical |
| pref.SQ_ACCENT | ACCENT | [0, 127] | 11 / 28 | {} | musical |
| pref.SQ_CLK | CLOCK OUT | [0, 1] | 11 / 29 | {} | musical |
| pref.SQ_NOTES | NOTES OUT | [0, 1] | 11 / 30 | {} | musical |
| pref.SQ_THEME | THEME | [0, 4] | 11 / 31 | {} | nonmusical |
| pref.SQ_KEYLED | LIGHTS | [0, 2] | 11 / 64 | {} | nonmusical |
| pref.SQ_KEYSOUND | KEY SOUND | [0, 1] | 11 / 65 | {} | musical |
| pref.SQ_AUTOSAVE | AUTOSAVE | [0, 1] | 11 / 66 | {} | nonmusical |
| generator.0 | DENS | [0, 100] | 11 / 72 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.1 | ACCENT | [0, 100] | 11 / 73 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.2 | SLIDE | [0, 100] | 11 / 74 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.3 | OCTS | [0, 2] | 11 / 75 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.4 | ROOT | [0, 11] | 11 / 76 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.5 | SCALE | [0, 5] | 11 / 77 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.6 | OCTAVE | [0, 4] | 11 / 78 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.7 | MUTATE | [0, 16] | 11 / 79 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| break.generator.0 | Cmplx | [0, 100] | 11 / 88 | {"pattern": [0, 31]} | musical |
| break.generator.1 | Anchor | [0, 100] | 11 / 89 | {"pattern": [0, 31]} | musical |
| break.generator.2 | Roll | [0, 100] | 11 / 90 | {"pattern": [0, 31]} | musical |
| break.generator.3 | Fill | [0, 100] | 11 / 91 | {"pattern": [0, 31]} | musical |
| break.generator.4 | Rtg2x | [0, 100] | 11 / 92 | {"pattern": [0, 31]} | musical |
| break.generator.5 | Rtg3x | [0, 100] | 11 / 93 | {"pattern": [0, 31]} | musical |
| break.generator.6 | Rtg4x | [0, 100] | 11 / 94 | {"pattern": [0, 31]} | musical |
| break.generator.7 | Rtg8x | [0, 100] | 11 / 95 | {"pattern": [0, 31]} | musical |
| break.generator.8 | Phrase | [0, 4] | 11 / 96 | {"pattern": [0, 31]} | musical |
| break.generator.9 | BChnc | [0, 100] | 11 / 97 | {"pattern": [0, 31]} | musical |
| break.generator.10 | A Len | [0, 5] | 11 / 98 | {"pattern": [0, 31]} | musical |
| break.generator.11 | B Len | [0, 5] | 11 / 99 | {"pattern": [0, 31]} | musical |
| part.channel.0 | 909 MIDI CH | [0, 16] | 11 / 104 | {} | musical |
| part.channel.1 | 808 MIDI CH | [0, 16] | 11 / 105 | {} | musical |
| part.channel.2 | 303A MIDI CH | [0, 16] | 11 / 106 | {} | musical |
| part.channel.3 | 303B MIDI CH | [0, 16] | 11 / 107 | {} | musical |
| part.channel.4 | BREAK MIDI CH | [0, 16] | 11 / 108 | {} | musical |
| pref.OUT_P | OUTPUT | [0, 1] | 11 / 112 | {} | musical |
| pref.USBA_P | USB AUDIO | [0, 1] | 11 / 113 | {} | musical |
| pref.FREEPLAY_P | FREE PLAY | [0, 1] | 11 / 114 | {"bass": [0, 1]} | musical |
| pref.SREP_P | REPEAT | [0, 7] | 11 / 115 | {"block": [0, 191]} | musical |
| pref.STALLS_P | STALLS | [0, 1] | 11 / 116 | {} | diagnostic |
| pref.BATT_P | BATTERY | [0, 5] | none | {} | read_only |
| song.0 | 909 | [0, 31] | 12 / 4 | {"block": [0, 191]} | musical |
| song.1 | 808 | [0, 31] | 12 / 5 | {"block": [0, 191]} | musical |
| song.2 | 303A | [0, 31] | 12 / 6 | {"block": [0, 191]} | musical |
| song.3 | 303B | [0, 31] | 12 / 7 | {"block": [0, 191]} | musical |
| song.4 | BREAK | [0, 31] | 12 / 8 | {"block": [0, 191]} | musical |
| song.5 | LENGTH | [0, 192] | 12 / 9 | {} | musical |
| action.0 | No action | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.1 | SAVE PROJECT | [0, 127] | 12 / 17 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.2 | CLEAR THIS PART | [0, 127] | 12 / 18 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.3 | CLEAR PATTERN | [0, 127] | 12 / 19 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.4 | COPY PATTERN | [0, 127] | 12 / 20 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.5 | FACTORY RESET | [0, 127] | 12 / 21 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.6 | ABOUT X0X | [0, 127] | 12 / 22 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.7 | INSERT BAR | [0, 127] | 12 / 23 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.8 | DELETE BAR | [0, 127] | 12 / 24 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.9 | CLEAR SONG | [0, 127] | 12 / 25 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.10 | PERFORMANCE | [0, 127] | 12 / 26 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.11 | RUN PERF TEST | [0, 127] | 12 / 27 | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.12 | DUP AS VARIATION | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.13 | EDIT BLOCK NOTES | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.14 | QUEUE BLOCK | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.15 | DELETE BLOCK | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.16 | UNDO | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.17 | PASTE KEEP ACC/SW | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| action.18 | COPY/PASTE SEQUENCES | [0, 127] | none | {"block": [0, 191], "part": [0, 4], "pattern": [0, 31]} | action |
| system.one_core | Second core disabled at next power-on | [0, 1] | none | {} | nonmusical |
| selector.pattern | selector.pattern | [0, 31] | 12 / 74 | {} | musical |
| selector.part | selector.part | [0, 4] | 12 / 75 | {} | musical |
| selector.drum | selector.drum | [0, 10] | 12 / 76 | {} | musical |
| selector.bass | selector.bass | [0, 1] | 12 / 77 | {} | musical |
| selector.step | selector.step | [0, 63] | 12 / 78 | {} | musical |
| selector.block_low | selector.block_low | [0, 127] | 12 / 79 | {} | musical |
| selector.block_high | selector.block_high | [0, 1] | 12 / 80 | {} | musical |
| selector.motion_low | selector.motion_low | [0, 127] | 12 / 81 | {} | musical |
| selector.motion_high | selector.motion_high | [0, 1] | 12 / 82 | {} | musical |
| pref.break_slot_a | pref.break_slot_a | [0, 127] | 12 / 83 | {"pattern": [0, 31]} | musical |
| pref.break_slot_b | pref.break_slot_b | [0, 127] | 12 / 84 | {"pattern": [0, 31]} | musical |
| pref.song_edit_block | pref.song_edit_block | [0, 191] | 12 / 85 | {} | musical |
| pref.tape_macro | pref.tape_macro | [0, 127] | 12 / 86 | {} | musical |
| step.drum_hit | step.drum_hit | [0, 1] | 12 / 87 | {"drum": [0, 10], "kit": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.drum_accent | step.drum_accent | [0, 1] | 12 / 88 | {"kit": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.bass_note | step.bass_note | [0, 127] | 12 / 89 | {"bass": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.bass_gate | step.bass_gate | [0, 2] | 12 / 90 | {"bass": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.bass_accent | step.bass_accent | [0, 1] | 12 / 91 | {"bass": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.bass_slide | step.bass_slide | [0, 1] | 12 / 92 | {"bass": [0, 1], "pattern": [0, 31], "step": [0, 63]} | musical |
| step.break_allowed | step.break_allowed | [0, 1] | 12 / 93 | {"pattern": [0, 31], "step": [0, 63]} | musical |
| step.break_slice | step.break_slice | [0, 8] | 12 / 94 | {"pattern": [0, 31], "step": [0, 63]} | musical |
| motion.value | motion.value | [0, 255] | 12 / 95 | {"lane": [0, 159], "step": [0, 63]} | musical |
| motion.target | motion.target | [0, 7] | 12 / 96 | {"lane": [0, 159]} | musical |
| motion.voice | motion.voice | [0, 127] | 12 / 97 | {"lane": [0, 159]} | musical |
| motion.param | motion.param | [0, 15] | 12 / 98 | {"lane": [0, 159]} | musical |
| motion.pattern | motion.pattern | [0, 31] | 12 / 99 | {"lane": [0, 159]} | musical |
| motion.part | motion.part | [0, 4] | 12 / 100 | {"lane": [0, 159]} | musical |
| motion.record | motion.record | [0, 1] | 12 / 101 | {"lane": [0, 159]} | musical |
| motion.hold | motion.hold | [0, 1] | 12 / 102 | {"lane": [0, 159]} | musical |
| motion.clear | motion.clear | [0, 1] | 12 / 103 | {"lane": [0, 159]} | musical |
| probability.value | probability.value | [0, 135] | 12 / 104 | {"drum": [0, 10], "part": [0, 4], "pattern": [0, 31], "step": [0, 63]} | musical |
| performance.mute | performance.mute | [0, 1] | 12 / 105 | {"strip": [0, 24]} | musical |
| performance.solo | performance.solo | [0, 1] | 12 / 106 | {"strip": [0, 24]} | musical |
| performance.pattern_cue | performance.pattern_cue | [0, 31] | 12 / 107 | {"part": [0, 4]} | musical |
| performance.chain_start | performance.chain_start | [0, 31] | 12 / 108 | {} | musical |
| performance.chain_end | performance.chain_end | [0, 31] | 12 / 109 | {} | musical |
| performance.song_enabled | performance.song_enabled | [0, 1] | 12 / 110 | {} | musical |
| performance.song_record | performance.song_record | [0, 1] | 12 / 111 | {} | musical |
| generator.new | generator.new | [0, 1] | 12 / 112 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.mutate | generator.mutate | [0, 1] | 12 / 113 | {"bass": [0, 1], "pattern": [0, 31]} | musical |
| generator.seed_byte | generator.seed_byte | [0, 255] | 12 / 114 | {"bass": [0, 1], "pattern": [0, 31], "seed_byte": [0, 3]} | musical |
| arrangement.enabled | arrangement.enabled | [0, 1] | 12 / 115 | {} | musical |
| arrangement.length | arrangement.length | [0, 192] | 12 / 116 | {} | musical |
| arrangement.key | arrangement.key | [0, 11] | 12 / 117 | {} | musical |
| arrangement.mode | arrangement.mode | [0, 9] | 12 / 118 | {} | musical |
| arrangement.parent | arrangement.parent | [0, 255] | 12 / 119 | {} | musical |
| arrangement.degree | arrangement.degree | [0, 7] | 13 / 0 | {"block": [0, 191]} | musical |
| arrangement.duration | arrangement.duration | [1, 32] | 13 / 1 | {"block": [0, 191]} | musical |
| arrangement.pattern | arrangement.pattern | [0, 31] | 13 / 2 | {"block": [0, 191], "part": [0, 4]} | musical |
| arrangement.mute | arrangement.mute | [0, 1] | 13 / 3 | {"block": [0, 191], "part": [0, 4]} | musical |
| arrangement.queue | arrangement.queue | [0, 191] | 13 / 4 | {} | musical |
| follow.mode | follow.mode | [0, 3] | 13 / 5 | {"bass": [0, 1]} | musical |
| follow.low | follow.low | [0, 127] | 13 / 6 | {"bass": [0, 1]} | musical |
| follow.high | follow.high | [0, 127] | 13 / 7 | {"bass": [0, 1]} | musical |
| pad.channel | pad.channel | [0, 255] | 13 / 8 | {} | musical |
| pad.root | pad.root | [0, 127] | 13 / 9 | {} | musical |
| pad.low | pad.low | [0, 127] | 13 / 10 | {} | musical |
| pad.high | pad.high | [0, 127] | 13 / 11 | {} | musical |
| selector.kit | selector.kit | [0, 1] | 13 / 12 | {} | musical |
| selector.strip | selector.strip | [0, 24] | 13 / 13 | {} | musical |
| selector.seed_byte | selector.seed_byte | [0, 3] | 13 / 14 | {} | musical |
| selector.commit | selector.commit | [0, 1] | 13 / 15 | {} | musical |
| selector.parameter_low | selector.parameter_low | [0, 127] | 13 / 16 | {} | musical |
| selector.parameter_high | selector.parameter_high | [0, 15] | 13 / 17 | {} | musical |
| value.exact_low7 | value.exact_low7 | [0, 127] | 13 / 18 | {} | musical |
| value.exact_mid7 | value.exact_mid7 | [0, 127] | 13 / 19 | {} | musical |
| value.exact_high2 | value.exact_high2 | [0, 3] | 13 / 20 | {} | musical |
| value.exact_commit | value.exact_commit | [0, 1] | 13 / 21 | {} | musical |
| lane.length | Individual sound-lane length | [1, 64] | 13 / 104 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| lane.rate | Individual sound-lane rate | [0, 7] | 13 / 105 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| value.exact_high7 | 21-bit exact value high chunk | [0, 127] | 13 / 106 | {} | musical |
| lane.mode | Prepared independent clock mode | [0, 1] | 13 / 107 | {} | musical |
| selector.destination_pattern | Explicit pattern-copy destination | [0, 31] | 13 / 108 | {} | musical |
| action_control.arm | Arm destructive native action | [0, 11] | 13 / 109 | {} | action |
| action_control.cancel | Cancel source action arm | [0, 1] | 13 / 110 | {} | action |
| rate.enable | Enable expanded rates (stopped) | [0, 1] | 13 / 111 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| rate.exact | Expanded exact division | [0, 20] | 13 / 112 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| rate.family | Rate sweep family | [0, 3] | 13 / 113 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| rate.boundary | Rate latch boundary | [0, 1] | 13 / 114 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| rate.hysteresis | Rate sweep hysteresis CC units | [0, 2] | 13 / 115 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| rate.sweep_v1 | Family-safe rate sweep ABI1 | [0, 127] | 13 / 116 | {"pattern": [0, 31], "strip": [0, 24]} | musical |
| output.master_knob | MASTER gain Q12 | [0, 4096] | 13 / 76 | {} | musical |
| slice.level_percent | slice.level_percent | [0, 100] | 13 / 80 | {} | browser_musical |
| slice.repeat | slice.repeat | [0, 1] | 13 / 81 | {} | browser_musical |
| slice.layer | slice.layer | [0, 1] | 13 / 82 | {} | browser_musical |
| slice.source_bpm_milli | slice.source_bpm_milli | [20000, 300000] | 13 / 83 | {} | browser_musical |
| slice.audition_index | slice.audition_index | [0, 63] | 13 / 84 | {} | browser_musical |
| slice.play | slice.play | [0, 1] | 13 / 85 | {} | browser_action |
| slice.stop | slice.stop | [0, 1] | 13 / 86 | {} | browser_action |
| slice.arm_sync | slice.arm_sync | [0, 1] | 13 / 87 | {} | browser_action |
| slice.import | slice.import | [0, 1] | 13 / 88 | {} | browser_action |
| slice.demo | slice.demo | [0, 1] | 13 / 89 | {} | browser_action |
| slice.save_import | slice.save_import | [0, 1] | 13 / 90 | {} | browser_action |
| slice.reopen_import | slice.reopen_import | [0, 1] | 13 / 91 | {} | browser_action |
| slice.export | slice.export | [0, 1] | 13 / 92 | {} | browser_action |
| editor.undo | editor.undo | [0, 1] | 13 / 93 | {} | action |
| editor.redo | editor.redo | [0, 1] | 13 / 94 | {} | action |
| transport.play | transport.play | [0, 1] | 13 / 95 | {} | action |
| transport.stop | transport.stop | [0, 1] | 13 / 96 | {} | action |
| transport.record | transport.record | [0, 1] | 13 / 97 | {} | action |
| route.connect_permission | route.connect_permission | [0, 1] | none | {} | nonmusical_action |
| route.output_selection | route.output_selection | [0, 1] | none | {} | nonmusical_action |
| route.activate | route.activate | [0, 1] | none | {} | nonmusical_action |
| route.retry_cleanup | route.retry_cleanup | [0, 1] | none | {} | nonmusical_action |
| route.disconnect | route.disconnect | [0, 1] | none | {} | nonmusical_action |
| file.audio_selection | file.audio_selection | [0, 1] | none | {} | nonmusical_action |
| file.marker_selection | file.marker_selection | [0, 1] | none | {} | nonmusical_action |
| file.hardware_slot_upload | file.hardware_slot_upload | [0, 1] | none | {} | nonmusical_action |
| file.hardware_slot_clear | file.hardware_slot_clear | [0, 1] | none | {} | nonmusical_action |
| ui.navigation | ui.navigation | [0, 1] | none | {} | nonmusical_action |
| ui.keyboard_latch | ui.keyboard_latch | [0, 1] | none | {} | nonmusical_action |
| ui.factory_reset | ui.factory_reset | [0, 1] | none | {} | nonmusical_action |
| ui.pattern_copy.source | Stopped copy source | [0, 31] | 15 / 9 | {} | nonmusical_action |
| ui.pattern_copy.destination | Stopped copy destination | [0, 31] | 15 / 10 | {} | nonmusical_action |
| ui.pattern_copy.scope | Stopped copy scope | [0, 24] | 15 / 11 | {} | nonmusical_action |
| ui.constrained_generation.mode | Constrained generation mode | [0, 4] | 14 / 91 | {} | nonmusical_action |
| ui.constrained_generation.chord_block | Constrained generation chord_block | [0, 191] | 14 / 92 | {} | nonmusical_action |
| ui.constrained_generation.generate_confirmed | Constrained generation generate_confirmed | [0, 1] | 14 / 93 | {} | nonmusical_action |
| ui.drum_generation.density | Drum generation density | [0, 100] | 14 / 94 | {} | nonmusical_action |
| ui.drum_generation.positions | Drum generation positions | [0, 3] | 14 / 95 | {} | nonmusical_action |
| ui.drum_generation.anchors | Drum generation anchors | [0, 5] | 14 / 96 | {} | nonmusical_action |
| ui.drum_generation.seed_index | Drum generation seed_index | [0, 255] | 14 / 97 | {} | nonmusical_action |
| ui.drum_generation.replace_confirm | Drum generation replace_confirm | [0, 1] | 14 / 98 | {} | nonmusical_action |
| ui.repeating_variation.every | Repeating variation every | [0, 2] | 14 / 99 | {} | nonmusical_action |
| ui.repeating_variation.changes | Repeating variation changes | [0, 8] | 14 / 100 | {} | nonmusical_action |
| ui.repeating_variation.anchor_lock | Repeating variation anchor_lock | [0, 3] | 14 / 101 | {} | nonmusical_action |
| ui.repeating_variation.seed | Repeating variation seed | [0, 255] | 14 / 102 | {} | nonmusical_action |
| ui.repeating_variation.mode | Repeating variation mode | [0, 2] | 14 / 103 | {} | nonmusical_action |
| ui.repeating_variation.shape | Repeating variation shape | [0, 1] | 14 / 104 | {} | nonmusical_action |
| ui.repeating_variation.range | Repeating variation range | [0, 3] | 14 / 105 | {} | nonmusical_action |
| ui.repeating_variation.arm_confirm | Repeating variation arm_confirm | [0, 1] | 14 / 106 | {} | nonmusical_action |
| ui.repeating_variation.off_confirm | Repeating variation off_confirm | [0, 1] | 14 / 107 | {} | nonmusical_action |
| ui.live_overlay | Live drum display overlay (session only; default OFF) | [0, 1] | none | {} | nonmusical_action |
| ui.freeze_commit.destination | Freeze/Commit destination | [0, 31] | 14 / 108 | {} | nonmusical_action |
| ui.freeze_commit.freeze_request | Freeze/Commit freeze_request | [0, 1] | 14 / 109 | {} | nonmusical_action |
| ui.freeze_commit.commit_confirm | Freeze/Commit commit_confirm | [0, 1] | 14 / 110 | {} | nonmusical_action |
| ui.random_options.length | Random options length | [0, 64] | 14 / 111 | {} | nonmusical_action |
| ui.random_options.note_step | Random options note_step | [0, 64] | 14 / 112 | {} | nonmusical_action |
| remote.generation.seed_byte0 | remote.generation.seed_byte0 | [0, 255] | 14 / 115 | {} | musical |
| remote.generation.seed_byte1 | remote.generation.seed_byte1 | [0, 255] | 14 / 116 | {} | musical |
| remote.generation.seed_byte2 | remote.generation.seed_byte2 | [0, 255] | 14 / 117 | {} | musical |
| remote.generation.seed_byte3 | remote.generation.seed_byte3 | [0, 255] | 14 / 118 | {} | musical |
| remote.request_low7 | remote.request_low7 | [0, 127] | 14 / 119 | {} | musical |
| remote.request_high7 | remote.request_high7 | [0, 127] | 15 / 0 | {} | musical |
| remote.action_arm | remote.action_arm | [0, 7] | 15 / 1 | {} | musical |
| remote.cancel | remote.cancel | [0, 1] | 15 / 2 | {} | musical |
| ui.cc_mode | GLO MIDI CC MODE | [0, 2] | none | {} | nonmusical |
| remote.variation.constraint_mode | Variation303 constraint mode | [0, 3] | 15 / 3 | {} | musical |
| remote.variation.chord_block | Variation303 chord block | [0, 191] | 15 / 4 | {} | musical |
| remote.capture.token_byte0 | Expected captured-pass token byte0 | [0, 255] | 15 / 5 | {} | musical |
| remote.capture.token_byte1 | Expected captured-pass token byte1 | [0, 255] | 15 / 6 | {} | musical |
| remote.capture.token_byte2 | Expected captured-pass token byte2 | [0, 255] | 15 / 7 | {} | musical |
| remote.capture.token_byte3 | Expected captured-pass token byte3 | [0, 255] | 15 / 8 | {} | musical |
| ui.pattern_copy.destination_scope | Scoped cross-part copy destination_scope | [0, 24] | 15 / 12 | {} | musical |
| ui.pattern_copy.preview_arm | Scoped cross-part copy preview_arm | [0, 1] | 15 / 13 | {} | musical |
| ui.pattern_copy.execute | Scoped cross-part copy execute | [0, 1] | 15 / 14 | {} | musical |
