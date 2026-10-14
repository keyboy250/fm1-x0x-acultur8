# Bass-follow target static evidence

This private candidate fits the exact optimized official target: image 581524/581564 bytes (40 spare), ordinary RAM 97864/98304 (440 spare), pool 335712/344064 (8352 free; 160 above the unchanged 8192 reserve). All 15 original `tests/target_budget.py` gates pass without model or limit changes. These are local target measurements, not physical audio deadline, cycle or stack-watermark acceptance.

Measured build source: `ad435ef34c60f5aa53d5ff07abfbadd71b10257ba8553a4d6d653ee34f7a63ab`. Production WASM: `b66e650260a587bb868d31b5464fbba3db70bffa555d28e60f5f7f3ea64cf540`. Target disassembly: `48f8d508d1ac83d5a657791a71c6711cb0324e15bc8ea817f74c41b0a51b7fb8`. Reports added after that build do not change its runtime objects.

`tests/bass_feature_costs.json` supplements the original gate report with 38 address-qualified rows and 122 shortest root/helper caller chains. It reuses the original model: backward-span nesting weight 4, divide extra weight 8, depth cap 4. The loop-only call metric excludes straight-line calls; `all_calls` preserves every emitted call, including indirect calls. A backward control-flow span can count as a loop in this approximation even when bounded branch behavior does not repeatedly execute it. The report does not sum static caller and callee scores into cycles.

| Newly outlined helper | Address | Instructions | Instruction bytes | Prologue frame | All call sites |
|---|---|---:|---:|---:|---:|
| bass_midi_shared | 0x2029860 | 17 | 52 | 12 | 0 |
| midi_try | 0x2029894 | 14 | 28 | 8 | 2 |
| bass_midi_off | 0x20298b0 | 34 | 94 | 28 | 2 |
| bass_release | 0x202990e | 22 | 54 | 24 | 2 |
| fire_bass | 0x202a65e | 212 | 610 | 96 | 9 |
| s_bon_checked | 0x202ec24 | 57 | 170 | 20 | 2 |

The earlier candidate overflowed XIP by 3240 bytes. Outlining the release/ownership/step bodies recovered 3024 bytes; sharing MIDI acceptance, shared-tone checks and the checked trigger then allowed the final fit. Lower caller instruction counts moved work into the helpers above. New callback and call overhead remains real work; the ledger exposes it rather than changing the original baseline.

Other relevant rows include bass_midi_on (166 instructions, 68-byte frame, 11 calls), bass_retune_held (93, 40, 5), seq_midi_prepare (62, 20, 3) and seq_midi_sync (38, 28, 3). The original gated seq_advance is 949 instructions with a 140-byte prologue allocation; seq_advance_arrangement is 853 with 124 bytes. Engine_render is 943 instructions, 128-byte prologue allocation and 81 total emitted calls (65 counted in the loop model). These individual frames are not a worst-case transitive stack total: callbacks, branch exclusions, tail transfers, IRQ frames and concurrent core execution require separate analysis and physical measurement.

The emitted plat_midi_out single-packet producer is 25 instructions with an 8-byte frame; plat_midi_pair is 28 with 12 bytes. Both have zero loops and zero calls. The production `midi_ring_try_put`/`midi_ring_try_pair` helpers are inlined into these functions; the queue-capacity checks, packet stores and publication are measured there. `cq_put` is also inlined, including in engine_arrangement_refresh (covered in the report). Several web-only entry points, including variant_command/snapshot and engine_arrangement_global, are not emitted in this firmware image and have no standalone target measurement here. Native/WASM validation does not substitute for an absent target cost row.

Direct calls and inter-function tail transfers resolve by exact address, preserving duplicate static names. Source callback alternatives explicitly record the immutable engine SINK and per-render sequencer dispatch: engine_render to either sequencer specialization; fire_bass to checked/compatibility trigger; retune to probability-free/compatibility trigger; MIDI paths to checked single/atomic-pair callbacks. These annotated alternatives are conservative connectivity, not a claim that every alternative executes. The chain report includes cleanup from reset/load/reinit as well as render/step paths, and transitive harmony, synth note-on and platform calls.

Reproduce against the existing artifacts (no target rebuild):

```sh
python3 tests/bass_feature_costs.py build/x0x.dis --report tests/bass_feature_costs.json --source-sha256 ad435ef34c60f5aa53d5ff07abfbadd71b10257ba8553a4d6d653ee34f7a63ab
python3 tests/test_bass_feature_costs.py
```

The report fails if its required helpers are absent, decoding contains unknown instructions, or the original passing gate report has a different disassembly/model. Its source ID is the recorded build identity supplied by the caller; the script hashes the actual disassembly, WASM and gate report. Publication remains held pending explicit user sign-off. Local queue acceptance does not establish delivery to an external receiver, particularly across disconnection.
