# Seeded drum rhythm candidate 0.1.12-drumbeta

This separate local candidate extends the frozen 0.1.11-genbeta source. It is the first FM1-62 slice, not an installed release. The connected FM1 and public 0.1.9-query release are unchanged.

## Native controls

Select 909 or 808 and press ARP to open RHYTHM. A black key selects one of that kit's eleven instrument lanes; the selected row is highlighted. The shared AC row cannot be generated. PRESETS selects the target pattern under the existing part-selection rules.

| Knob | Control | Meaning |
| --- | --- | --- |
| 1 | DENS | 0..100 percent chance at each eligible, unlocked position |
| 2 | STEPS | ALL, EVEN (human steps2,4,...), ODD (1,3,...), OFFBT (3,7,11,15,... in sixteenth steps) |
| 3 | ANCHOR | NONE, BEAT, BACK, KEEP, B.LCK or S.LCK |
| 4 | SEED | Repeatable recipe index1..256 |

OCT+ proposes a fresh seed index different from the current index; OCT- repeats the current index. Both show a replacement question identifying kit, voice, pattern, seed and anchor. SEL confirms, HOME cancels. The exact target and recipe are captured in the question; changes to project/transport invalidate it. HOME+REC undoes and HOME+PLAY redoes under the existing stopped transport rules.

Recipe controls are transient and shared between drum lanes; they reset on boot to density50, ALL, NONE, seed1. Generated hits are ordinary pattern data and persist through the existing Save/recall flow. To regenerate a saved rhythm after boot, restore its recipe controls manually. This first slice does not add saved generator fields or MIDI bindings. A new seed can produce an identical rhythm, especially at density0/100 or with many anchors.

## Anchor policies

- NONE: randomize every eligible active position.
- BEAT: force hits at human steps1,5,9,13 and every subsequent four steps.
- BACK: force hits at human steps5,13 in each sixteen-step cycle.
- KEEP: preserve every currently active hit while allowing new hits elsewhere.
- B.LCK: preserve both hits and rests at BEAT positions; randomize other eligible positions.
- S.LCK: preserve both hits and rests at BACK positions; randomize other eligible positions.

Forced/retained hits outside STEPS are a conflict: refuse with the entire project unchanged. An anchor policy with no anchor position in the active length also refuses. Locked rests outside the allowed positions remain rests. Density0 still retains forced/locked hits; density100 fills only eligible unlocked positions. These policies operate in step positions, not musical-time conversion: rate remains the lane's existing rate.

## Preservation and transaction

Only the chosen808/909 hit row within its active length changes. The lane's length/rate, inactive tail, other drum/303/BREAK data, shared drum accents, swing, probability/condition and motion lanes, sound settings, strips, routing, song and all other project bytes remain intact. Per-step probability and motion remain attached to their original positions; generation does not copy/move them. No cross-lane hat-exclusion rule is implied.

Generation uses a local deterministic integer PRNG and a staged stopped transaction with queue, transport, recording/keys, write-protection and Undo-capacity checks. Cancel/refusal cannot partially edit the project. A changed row is one Undo/Redo entry. Repeating a seed with identical settings is stable, including KEEP. A byte-identical result reports UNCHANGED: SAME RHYTHM, without dirtying the project, resetting motion, or adding an Undo entry. Generation does not explicitly Save; existing independently configured auto-save still applies. Save and reload reproduce the stored hits, rather than running the generator again. No audio render or outgoing MIDI scheduling loop changed.

## Local verification and remaining work

See DRUM_GENERATION_STATUS.json for exact hashes and final evidence. Tests cover all22 drum lanes in legacy, independent and expanded formats, lengths1..64, positional/anchor conflicts, deterministic seeds, inactive tails and whole-project preservation; real command49 replies decoded by the production read-only client; native and real-WASM menu/Cancel/Undo/Redo/saved restart. All15 static resource gates remain unchanged. Ordinary RAM remains98292/98304bytes (12bytes free), with no additional static RAM consumed; the diagnostic application image is515368bytes.

This slice provides single-lane generation and rhythmic anchor locks. Arbitrary per-step locks, pre-commit musical audition, dedicated fill generation, simultaneous multi-lane policies, persisted recipes and controlled repeated playback variations remain separate work. Physical menu/listening/power-cycle acceptance remains pending. No device access, flashing or publication occurred.

## Reproduce offline

Use the preserved SDK/compiler and acultur8-fm1-emu-builder:local, network disabled. Import build/drum-generation/oracle-history.bundle refs/test-oracles/* into the extracted source repository before the required suite.

- tests/host/run_drum_generation_target.sh
- tests/host/run_drum_generation.sh
- tests/test_drum_generation_read.py
- X0X_VERSION=0.1.12-drumbeta lab/build_emulator.py --source-root /work --native
- node tests/host/drum_generation_wasm_test.mjs build/emu/x0x.wasm
- tools/acultur8_checks.sh
- Inherited run_generation.sh, test_generation_read.py, generation_wasm_test.mjs, run_cross_part_copy.sh, test_cross_part_read.py, cross_part_copy_wasm_test.mjs and pattern_copy_wasm_test.mjs (native fixtures before their clients).

The target package is diagnostic only, not approved for flashing or publication.
