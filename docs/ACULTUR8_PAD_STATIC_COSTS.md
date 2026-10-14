# External-pad static target evidence

Private local candidate only. The exact official probe fits image 579556/581564 bytes (2008 spare), ordinary RAM 97868/98304 (436 spare), and pool 335712/344064 (8352 free, 160 above the unchanged 8192 reserve). All 15 original target gates pass without changed thresholds or model. Net ordinary RAM grows four bytes from the accepted bass target; the five-byte pad ownership/status fields reuse alignment space. Pool allocation is unchanged. These figures do not establish physical CPU, DMA, stack or receiver timing acceptance.

The measured build source is `acabfca22ed9558d825d323bc555ea165eeee5da0d86d7fad7b06c2d091faa7c`. Production WASM is `0289ee8b2b139a72f2fb8ea92d5c52e625d3439192572f17ecd944a80e8d13b8` (663839 bytes). Target disassembly is `918289836a90666600994f40409d47d35c64590689f35acfede915217cda94a8`. The additional report is `tests/pad_feature_costs.json`; the accepted bass report is retained separately without rewriting its snapshot.

| Emitted helper | Address | Instructions | Instruction bytes | Prologue frame | All call sites |
|---|---|---:|---:|---:|---:|
| seq_pad_desired | 0x2028cf8 | 72 | 186 | 40 | 1 |
| pad_publish | 0x2028db2 | 78 | 188 | 52 | 1 |
| pad_sync | 0x202925e | 44 | 116 | 16 | 2 |
| s_midi_batch | 0x202e6b2 | 3 | 8 | 0 | 0 |
| plat_midi_batch | 0x2000f96 | 35 | 114 | 24 | 0 |
| seq_midi_prepare | 0x2028fc0 | 85 | 204 | 24 | 7 |
| seq_midi_sync | 0x20292d2 | 41 | 112 | 28 | 4 |
| engine_notes_clear | 0x202c954 | 16 | 50 | 8 | 2 |
| engine_midi_channel_allowed | 0x202c932 | 10 | 34 | 0 | 0 |
| engine_arrangement_refresh | 0x202c1a4 | 26 | 84 | 12 | 2 |
| project_apply_io | 0x200233a | 30 | 102 | 16 | 1 |

The producer accepts at most six packets, with capacity validation before any store and one publication. `midi_out_batch` and `midi_ring_try_batch` are inlined in `plat_midi_batch`: its five loop instructions cover the bounded packet stores, with no divide or call. The `s_midi_batch` callback uses a tail transfer to that producer rather than a call; the report records tail edges separately. `pad_release` is inlined in cleanup/sync users, and `do_stop` in the sequencer specializations; their work has not disappeared.

The report has 43 address-qualified rows and 58 shortest root/helper chains. Duplicate function names stay distinct by address. Direct call and inter-function tail targets resolve by emitted addresses. Conservative source alternatives connect the immutable production SINK and per-render legacy/arrangement selector, including pad_publish to s_midi_batch. These source annotations do not assert that every alternative executes. Physical cancellation callback is null; browser-only scan/cancellation state is excluded from this target.

Cleanup chains include Stop through sequencer preparation, reset/load through project_defaults and engine_notes_clear, and engine reinitialization. Chord publication follows final same-sample bass work through seq_midi_sync/pad_sync. Harmony resolution is measured transitively. Output intent bit5 is applied inside the inlined C_ARR drain in engine_render and through project_apply_io; pref_set, undo_arrangement_intent and ui_undo_preflight rows preserve legacy setter and prospective-undo costs. Browser command/snapshot/global setter entry points are absent from the linked firmware image and explicitly listed unmeasured, along with web_midi exports and tagged browser input APIs. Native/WASM correctness tests are separate evidence for those absent target entry points.

The original gated engine_render has 953 instructions, loop cost 1202, 65 loop-model calls and 81 total emitted calls; its prologue allocates 128 bytes. Legacy seq_advance has 961 instructions, cost 45334, 23 calls and a 140-byte prologue allocation. The arrangement specialization has 865 instructions, cost 35763, 26 calls and 124 bytes. Caller metrics exclude callee bodies. Earlier outlining moved work into shared helpers; lower caller counts must not be presented as lower total CPU work.

The unchanged model uses backward-span nesting weight4, divide extra weight8, maximum depth4. Backward spans are static control-flow approximations and may count bounded branch paths as loops. The loop call metric excludes straight-line calls; all_calls includes them and indirect calls. Prologue frames are individual emitted allocations only: they exclude transitive callbacks, path exclusivity, tail-call frame lifetime, interrupt nesting, DMA and concurrent-core behavior. Neither these scores nor summed frames establish cycles, deadline margin or worst-case stack watermark. MIDI queue acceptance also does not prove delivery after a receiver disconnects.

Reproduce from the existing exact artifacts, without target/emcc rebuild:

```sh
python3 tests/pad_feature_costs.py build/x0x.dis --report tests/pad_feature_costs.json --source-sha256 acabfca22ed9558d825d323bc555ea165eeee5da0d86d7fad7b06c2d091faa7c
python3 tests/test_bass_feature_costs.py
```

The pad script reuses the tested address-preserving parser/frame reader and original target cost function. It rejects unknown instructions, missing required emitted helpers, or mismatched/failed original gate evidence. Actual disassembly, WASM and original gate report are hashed; the supplied source identity is the recorded build identity. Test/report additions do not modify these runtime artifacts. Full local/browser/physical acceptance and publication sign-off remain separate boundaries.
