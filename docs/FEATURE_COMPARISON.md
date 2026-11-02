# Comparative features — public .32 and the .60 kick prerelease

[Documentation](README.md) · [Full FAQ](FAQ.md) · [Full CC/MIDI map](CC_MIDI_MAP.md) · [Tutorials](tutorials/README.md)

This comparison is pinned to [Acultur8 v0.1.32-straightbeta](https://github.com/keyboy250/fm1-x0x-acultur8/tree/v0.1.32-straightbeta) and the reviewed `0.1.60-stoppedtrace` source. It does **not** claim that the latest upstream X0X lacks any feature. X0X by Charles Vestal and contributors, built on Felucca by Leo Kuroshita / Hugelton Instruments, remains the credited foundation.

| Area | Public .32 baseline | .60 kick prerelease | Evidence / limits |
| --- | --- | --- | --- |
| 909, 808, two 303 parts, BREAK and song workflow | Existing X0X/Acultur8 foundation | Retained | Inherited engine, not a new .60 invention |
| All 25 sound strips | Already documented | Retained, with exact-output strip/workload changes in later development | Full software checks; physical sweeps covered representative kick controls, not every combination |
| Per-sound HP/LP, boost and limiter | Already documented | Retained; shared mono processing work reduced | Exact PCM/state comparisons and representative quiet tests |
| Independent lanes and expanded rate families | Present in earlier Acultur8 | Retained | Do not present these as new kick-only features |
| Arrangement/chords/bass following | Already documented | Retained | Host/scenario evidence; no complete DAW/live-set certification |
| Hybrid 808 wave layer and 39 controls | Not in pinned .32 drum808 source | Active saved hybrid kick with built-in sources and optional USER1 reference | Current source, exact-value codec and scoped listening |
| Separate synth/wave envelopes and click shaping | No hybrid layer in that pinned source | Current hybrid controls | Screen milliseconds differ from stored raw values |
| Native kick preset bank | No 16-slot hybrid kick bank in pinned .32 | 16 native kick slots; confirmed STORE/RECALL | Exact record readback, unchanged other slots and normal restart passed |
| Starter sound | No Favorite Slot 1 package in pinned release | One selected preset, not an untuned draft bank | Export bytes, codec round-trip and separate quiet audition passed |
| Kick step velocity tiers and jitter | Not part of the pinned hybrid extension | LOW/MID/HIGH tiers, per-step selection, jitter and modulation | Retained ABI; no new independent resolved-velocity telemetry claim |
| Long-tail hybrid retrigger handling | Not applicable to the absent hybrid layer | Existing residual-ramp handling retained | Current-sound and overlapping 1,773 ms quiet checks; not exhaustive phase/velocity coverage |
| PHASE 303B option | Not part of that release's stated extension | Native option retained from later development | Four native/unmapped inventory rows; do not imply direct CC binding |
| Complete inventory | 753 rows, draft-1-actions-prototype-1 | 803 rows, draft-2-kick-presets-1 | 50 added inventory rows: 46 addressed kick controls/actions/tier plus 4 native PHASE entries |
| Preset action guards | No later kick-preset ABI | Explicit context, request token, arm/execute and stopped-only transaction | Source-owned MIDI contexts are independent of panel focus |
| Timing/calibration and clock routing | Earlier timing work already present | Retained | Calibration preserved across normal restart; no new universal MIDI-jitter claim |
| P2/P11 workload reduction work | Earlier engine checkpoint | Later hat/table RAM placement and strip work reductions retained | Host improvements and bounded device observations, not every workload |
| Stopped dropout capture | Earlier diagnostic behavior differs | .60 can retain a dropout block even when stopped | A diagnostic capability, not attribution of a fixed root cause |
| Test scope | Earlier release's own evidence | 135 checks, 39 host scenarios, six kick groups; current bounded device checks | Host identity 9000138 and target identity 9000160 are distinct |
| Sustained physical observation | Keep earlier results in their original scope | User-timed complex P11 25 minutes, 0 dropouts / 0 guards | One unit and workload; earlier .59 events remain unexplained |
| Physical core1 / full stack and deadlines | Not established by this comparison | Still open | IRQ 1,092 / 7,936 B is not a whole-system or core1 bound |
| Automatic preset importer / mod-pack loader | Not promised here | Deferred; JSON exporter is read-only | Use the supplied native-control reference and an empty destination slot |
| Native Tightness Factor | Not included by this comparison | Deferred | Do not promise it in promo or tutorials |
| Browser, Pages and physical acceptance | Separate evidence boundaries | Remain separate | This prerelease does not deploy Pages or certify every browser/DAW setup |

## Provenance

The .32 README and original drum808 source were read from the pinned public tag. Its inventory contains 753 rows; the current complete inventory contains 803. Full prior FAQ topics are retained in [FAQ.md](FAQ.md), with historical labels separated from current release scope. Numeric map assignments were not changed by producing these documents.
