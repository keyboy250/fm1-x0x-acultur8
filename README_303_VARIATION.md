# Controlled 303 variations — local candidate 0.1.14-303varbeta

Separate candidate based on the frozen 0.1.13-varbeta source. This extends the existing temporary drum variation controls to either 303 A or 303 B. One lane/pattern recipe can be armed at a time across drums and basses. Arming another owner replaces the previous owner. No FM1 access, flashing or publication is part of this qualification.

## Musical behaviour

The ordinary passes play the base. Every second, fourth or eighth local pass can vary up to eight candidate step positions. Only the pitch of an existing NOTE can change. REST, TIE, slide, note accent, shared drum accent, probability, swing, lengths/rates, automation, sounds and routing retain their existing behaviour. TIE continues the accepted sounding note; a rejected NOTE is not revived by a TIE. No rhythm generation, gate insertion or deletion occurs in this 303 overlay.

Pitch choices are NEAR (nearest legal neighbour, seeded preference when both directions fit), UP or DOWN. The search is bounded to one octave either way, within the captured generator octave range. NEAR tries both directions; UP/DOWN use only their chosen direction. The final candidate must fit the captured pitch-class mask AND the existing live harmony-follow resolver, after transpose. If no alternative fits, the exact base event remains. This does not quantize or rewrite an out-of-scale base pattern.

## Scale/chord constraints

Choose NOTES on the existing CONSTRAINED page or the new VAR OPTIONS page:

| NOTES | Snapshot captured when ARM is confirmed |
| --- | --- |
| GEN | The selected 303 generator root and scale |
| KEY | Project key and scale/mode |
| CHORD | Triad from the selected existing song BLOCK |
| BOTH | Generator scale intersected with that block's triad |

FREE remains an option for the earlier stored generator, but is refused when arming this constrained variation. Missing chord, empty intersection or invalid constraints refuse without replacing the active recipe. Set the BLOCK, generator root/scale and generator octave range before arming. The confirmation identifies the mode and block and says SNAP / PITCH. The snapshot does not silently follow edits to those recipe inputs; re-arm to capture different constraints. Separately enabled live scale/chord following remains active at playback and can further restrict a proposed pitch. No live-follow or song setting is enabled by arming.

## Native controls

With the chosen 303 and pattern selected, ARP now cycles through GENERATE, SCALE, CONSTRAINED, VARIATION and VAR OPTIONS. The first three pages retain their previous stored-generation actions.

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| VARIATION | EVERY: 2 / 4 / 8 | CHANGE: 0..8 | LOCK: NONE / BEAT / BACK / BOTH | SEED: 1..256 |
| VAR OPTIONS | PITCH: NEAR / UP / DOWN | SHAPE: SAME / EVOLV | RANGE: ALL / LAST4 / LAST8 / LAST16 | NOTES constraint |

On these two pages OCT+ opens ARM confirmation; SEL confirms and HOME cancels. OCT- opens OFF confirmation. Configure, ARM and OFF while stopped. The LIVE tag identifies the armed owner, for example `303A P5 LIVE`, even when inspecting a different part. All recipe controls are shared session controls; the target only changes on confirmed ARM.

BEAT locks step positions 1,5,9,13…; BACK locks 5 and 13 in each group of sixteen; BOTH combines them. These are position locks, including existing rests. Locked positions are skipped without compensating elsewhere. CHANGE is an upper bound on candidate positions, not a promise that every candidate changes. RANGE limits the positions considered; short patterns use the available length. SAME repeats the seeded position/pitch choice; EVOLV changes its seeded choices on later variation passes without feeding the previous result back into the base.

For 303 variation, a local pass is a group of the active lane's length in played steps, regardless of forward, reverse, ping-pong or random direction. Random direction can revisit a candidate position. This definition avoids changing legacy wrap counters used by automatic mutation and probability. Phase resets on Start or a pattern restart/cue and is retained by Continue, including its normal next-step advance. The compact 303 phase counter wraps after 256 local passes. Drum cadence retains its existing counter/policy.

## Preservation, Save and Undo

The stored grid and read-only command 49 query show the base pattern. The overlay does not modify project data, sound settings, note flags, generator seed, Undo history or saved objects. Save stores the base, not the temporary variant. OFF restores base-only playback; ordinary Undo remains available for deliberate project edits. Boot defaults variation OFF. As with the parent candidate, the runtime recipe remains a session control during a same-session project load; it is not deserialized from saved data.

Auto-mutation must be OFF on the selected 303 before ARM. While that lane/pattern is armed, automatic mutation is also suppressed defensively if its legacy setting is subsequently enabled. The setting itself is unchanged; its normal behaviour resumes after OFF. Unrelated explicitly enabled recording, manual edits or another part's legacy auto-mutation are independent actions.

## Local qualification

The machine-readable receipt is `VARIATION303_STATUS.json`. Proofs live in `build/303-variation/`. Tests exercise both 303s, all three project formats, all four constraint modes, all three pitch choices, normal and articulated patterns, all playback directions, live root/chord/scale following, transpose, probability rejection/TIE handling, singleton/no-alternative ranges, invalid/busy/stale confirmations, seven-step/eighth-note passes, evolving last-four fills, Start/Continue/cues, actual accepted synthesis/MIDI events and note ownership, Save/boot and read-only queries. Production WASM runs actual menu/knob/ARM/PLAY/OFF actions against 24 native eight-cycle MIDI pitch/timing/accent fixtures.

The original required regression suite and inherited drum variation, drum generation, 303 generation and cross/same-part copy tests must pass before freezing. The inherited 303 generator test changes only its page-count expectation from three to five; checks of the original three pages remain.

Target limits are unchanged. The recipe grows from 8 to 12 bytes, using the remaining four ordinary static RAM bytes. Two per-track phase bytes occupy previously unused padding with an explicit unchanged 24-byte track-size assertion. Ordinary static RAM is 98,304/98,304 bytes and the pool remains 335,708/344,064 bytes. Added note-step work is measured separately; neither static budgets nor host/WASM tests establish physical timing, headroom, listening quality or receiver delivery. The firmware package is a local diagnostic candidate, not approved for installation or publication.

## Reproduce offline

Use the existing network-disabled local builder with the pinned SDK/compiler. Import the parent's `oracle-history.bundle` refs before the required suite.

1. `sh tests/host/run_303_variation.sh`
2. `python3 tests/test_303_variation_read.py`
3. `X0X_VERSION=0.1.14-303varbeta python3 lab/build_emulator.py --source-root /work --native`
4. `node tests/host/variation303_wasm_test.mjs build/emu/x0x.wasm`
5. `sh tests/host/run_303_variation_regressions.sh`

Remaining work includes simultaneous multi-part recipes, arbitrary note locks, live boundary-latched control changes, keeping a variation into a saved slot, persisted recipes and physical FM1 acceptance.
