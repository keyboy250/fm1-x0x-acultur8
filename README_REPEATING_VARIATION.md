> Historical drum-slice guide inherited from 0.1.13. For this candidate's 303 extension and current resources, see README_303_VARIATION.md and VARIATION303_STATUS.json.

# Controlled repeating variations: local candidate0.1.13-varbeta

First FM1-63 slice, based on the frozen0.1.12-drumbeta source. This is a separate local candidate. No device installation or publication occurred.

## Behaviour

Arm one808/909 instrument lane on one explicit pattern. Play the unchanged base on ordinary passes, with a temporary variation on every2nd,4th or8th pass: e.g. A,A,A,A-prime. The pass is the selected lane's complete local cycle, not necessarily a four-beat bar. Independent lengths/rates retain their own cycles.

Every hit decision reads the base pattern directly. The previous variation is never fed back, so evolving variations cannot accumulate mutation drift. The visible step grid and command49 pattern queries continue to show the stored base. The overlay does not rewrite patterns or allocate a second pattern bank.

## Native controls

Select909 or808, then press ARP twice to open VARIATION (ARP's first page remains the existing RHYTHM generator). Select a drum lane with a black key.

| Page | Knob | Control |
| --- | --- | --- |
| VARIATION | 1 | EVERY: vary the2nd,4th or8th local pass |
| VARIATION | 2 | CHANGE: maximum0..8 candidate steps |
| VARIATION | 3 | LOCK: NONE, BEAT, BACK or BOTH |
| VARIATION | 4 | SEED:1..256 |
| VAR OPTIONS | 1 | MODE: TOGGL, ADD or DROP |
| VAR OPTIONS | 2 | SHAPE: SAME or EVOLV |
| VAR OPTIONS | 3 | RANGE: ALL, LAST4, LAST8 or LAST16 steps |

OCT+ asks to arm the selected lane/pattern with the captured recipe. SEL confirms; HOME cancels. OCT- asks to disable the overlay. The tag identifies the armed owner, e.g.909 SD P1 LIVE, or shows VARIATION OFF. Selecting another lane/pattern for inspection does not silently retarget an armed overlay. A new ARM replaces the single previous target.

Configure, arm and disable while stopped. Busy/recording/keys/transport/pending-control states refuse without a partial change. Recipe changes are transient and do not enter the project Undo journal. Off restores base-only playback; ordinary project Undo/Redo remains separate.

## Variation policy

- CHANGE limits the number of proposed step positions, capped by the available range. The seed rotates a contiguous region of candidate positions, forward or backward. This is a bounded local rhythm change, not unrestricted random rerolling.
- BEAT locks both hits and rests at human steps1,5,9,13 and subsequent groups of four. BACK locks5 and13 in each16-step cycle. BOTH combines them.
- Locked positions are skipped; the algorithm does not compensate by changing more positions. Consequently actual changes can be fewer than CHANGE, including zero.
- TOGGL flips eligible hits/rests. ADD only adds hits; DROP only removes hits. Existing hits in ADD and rests in DROP reduce the actual change count.
- SAME repeats the same variation on every scheduled pass. EVOLV deterministically changes its position selection for subsequent scheduled passes, always from the base.
- LAST4/8/16 restricts candidates to the final steps of the active lane. Short lanes use the entire available length. These are step regions, not time-stretched beat fills.
- Seed/recipe/base and local pass determine the result. The existing16-bit local cycle counter repeats after65536 cycles; no unbounded history is stored.

## Preservation and lifecycle

Only emitted drum-hit decisions differ. Stored base hits, inactive tails, all303/BREAK data, sounds/strips/routes, shared accent rows, swing, length/rate, song, automation and probability/condition lanes remain byte-identical. Added hits inherit the existing accent at their destination step. Existing probability/mute rules still apply; they can suppress proposed hits. Internal synthesis and outgoing sequence MIDI use the same accepted hit decision and existing note/channel/velocity encoding.

The target is fixed to one lane and pattern. Manual cues/arrangement selection retain priority; other patterns play normally. A full Start resets to the first base pass. Continue preserves local phase, subject to the ordinary next-step advance. Stop emits existing cleanup. Off does not erase the base.

The recipe is runtime-only and defaults OFF after boot. The RAM recipe remains a session control until Off or restart; a same-session project reload does not deserialize a recipe. Generated/stored base patterns save exactly as before. Save does not capture a live variant or persist this recipe; no project format or remote CC binding was added. Existing separately enabled303 auto-mutation, song recording or deliberate manual edits retain their existing behaviour and are outside this overlay's preservation action.

## Local evidence and resource limits

See VARIATION_STATUS.json and build/repeating-variation proof logs for exact final hashes/counts. New tests cover73728 recipe combinations, all22 lanes in three project formats, native internal/MIDI note-channel-velocity parity, seven-step/eighth-note polymeter, synthetic external-clock pulses, Start/Continue, cue isolation, P32 context, stopped guards, full-project/Undo/storage preservation and actual command49 replies decoded by the production reader. The real production WASM menu/PLAY/OFF paths match native8-cycle event fixtures for all22 lanes; ADD/EVOLVE/LAST8, Cancel, save bytes and reboot reset are exercised.

The original15 static target function gates retain their baseline. Target application517720bytes, ordinary staticRAM98300/98304bytes (8bytes more than parent;4bytes free), pool335708/344064 unchanged. The projection is95 target instructions with no loops/calls in its measured body. It adds bounded integer work in the drum-step path; the separate target/variation-step-review.json records this expansion, including the original caller. These static/host checks do not establish physical cycles, latency, listening quality, USB/TRS receiver delivery or device headroom.

## Remaining FM1-63 scope

This first slice covers one selected drum lane at a time. Melodic303 variation, arbitrary step locks, simultaneous multi-lane recipes, boundary-latched live recipe edits, keeping a variant into a separate saved slot and persisted recipe settings remain separate work. No whole-target or physical acceptance is implied.

## Reproduce offline

Use the preserved SDK/compiler and acultur8-fm1-emu-builder:local with network disabled. Import build/repeating-variation/oracle-history.bundle refs/test-oracles/* into a source extraction's local Git repository before the full suite.

- tests/host/run_repeating_variation_target.sh
- tests/host/run_repeating_variation.sh
- tests/test_variation_read.py
- X0X_VERSION=0.1.13-varbeta lab/build_emulator.py --source-root /work --native
- node tests/host/repeating_variation_wasm_test.mjs build/emu/x0x.wasm
- tools/acultur8_checks.sh
- Inherited drum-generation,303-generation and cross/same-part copy native/readback/WASM tests, generating native fixtures before their clients. The inherited drum test changes only its expected ARP page count from1 to3; its original RHYTHM page checks remain intact.

The target package is diagnostic only and is not approved for flashing or publication.
