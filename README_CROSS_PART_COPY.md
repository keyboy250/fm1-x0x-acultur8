# Cross-part copy candidate: 0.1.10-crosscopy

Separate local candidate based on the exact published/installed 0.1.9-query source. It is not approved for device installation or publication. Public 0.1.9-query and the installed FM1 are unchanged.

## Operation

GLOBAL > COPY/PASTE SEQUENCES now offers COPY FROM, PASTE INTO, SOURCE PART, DEST PART, then PASTE KEEP ACC/SW. SELECT moves; ALGORITHM changes a value; SEL opens the confirmation and SEL confirms; HOME cancels or backs out. Source selection initially sets the destination to the same part. Destination choices are compatible with the source: all parts to all parts, any of 22 drum lanes to any drum lane, or either 303 to either 303. Same-pattern cross-part copying is supported. Identical pattern and part is a no-op.

## Ownership rules

- Cross-drum copies replace only destination hits and, in independent/expanded projects, its lane length/rate. Legacy shared kit length/rate remains destination-owned.
- Cross-303 copies replace notes, gates, slides/ties, length/rate, direction, transpose and generator configuration, except destination per-step accents and generator accent probability remain unchanged.
- Destination sound/preset bases, sound strips, MIDI routing, shared drum accent rows and pattern swing remain unchanged. Unrelated lanes and source data remain unchanged.
- Cross-part copying retains destination tone/FX automation. Source step probability and condition lanes replace destination probability lanes, with their part/voice remapped. An absent source probability lane clears the selected destination probability lane.
- Existing same-part/all-part copies retain their established behaviour, including copied 303 note articulation and scoped tone automation. This candidate does not silently redefine those operations.
- Drum-to-303 rhythm conversion is not implemented: it needs a separate pitch/gate policy.

## Transaction

The copy is stopped-only, staged from an immutable source, and published atomically. Playback/recording/edit/save/protection gates and full motion/Undo capacity checks remain in force. Confirmation is invalidated if project or transport changes. One Undo restores the transaction; Redo reapplies it. The copy operation does not call Save; configured autosave remains independent. Read-only command49 remains unchanged.

## Local verification

See CROSS_PART_COPY_STATUS.json and build/cross-copy/*.log for final results. Tests cover all 462 cross-drum routes and both cross-303 routes, three project formats, same/different patterns, preservation, refusal paths, Undo/Redo, actual command49 replies, production Python decoding, canonical emulator menu/buttons, saved object parity and simulated restart. Original same-part and required regression suites also run. Target gates retain the existing budgets; no limits were relaxed.

Hardware artifacts in build/cross-copy/target are diagnostic only. Physical menu, listening, device copy/Undo and power-cycle acceptance remain pending. The candidate adds no audio-render or MIDI hot-path work.

## Reproduce offline

Use the existing acultur8-fm1-emu-builder:local image and the preserved SDK/compiler mounts. Run tests/host/run_cross_part_copy.sh; tests/test_cross_part_read.py; tests/host/run_cross_part_target.sh; lab/build_emulator.py --source-root /work --native with X0X_VERSION=0.1.10-crosscopy; tests/host/run_pattern_copy.sh; both pattern_copy_wasm_test.mjs and cross_part_copy_wasm_test.mjs with build/emu/x0x.wasm; then tools/acultur8_checks.sh. Cross-part native fixtures must be generated before their Python/WASM checks.

The source ZIP intentionally excludes Git history and generated build files. For offline oracle reproduction, initialise Git in the extracted candidate and fetch refs/test-oracles/* from the accompanying build/cross-copy/oracle-history.bundle. That bundle preserves the eight exact accepted reference commits; none of the comparison checks are replaced by the candidate itself.
