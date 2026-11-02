0.1.24-varmidi / FM-1_9000124 - local candidate, not installed or published

Repeating playback variation over MIDI1, with explicit context and readback.
The base pattern, project, sounds and Undo journal remain unchanged. This is
temporary playback variation; Freeze/Commit remote control is a later slice.
One global live variation can target one of22 drum lanes or303A/B. Each of the
three MIDI sources has one private staging recipe, shared with303/drum generation.
Switching recipe family invalidates its arm and seed validity, not request history.

NEW ADDRESSES (human channels; existing703 addresses retained)
CH14 CC99  ui.repeating_variation.every       0/1/2 = every2/4/8 passes
CH14 CC100 ui.repeating_variation.changes     0..8 maximum changes
CH14 CC101 ui.repeating_variation.anchor_lock 0..3 bitmask: BEAT1 / BACK2 locks
CH14 CC102 ui.repeating_variation.seed        0..255 deterministic raw seed
CH14 CC103 ui.repeating_variation.mode        drums0 TOGGL /1 ADD /2 DROP;3030 NEAR /1 UP /2 DOWN
CH14 CC104 ui.repeating_variation.shape       0 SAME /1 EVOLV
CH14 CC105 ui.repeating_variation.range       0 ALL /1 LAST4 /2 LAST8 /3 LAST16
CH14 CC106 ui.repeating_variation.arm_confirm low0/high1 execute ON
CH14 CC107 ui.repeating_variation.off_confirm low0/high1 execute OFF
CH15 CC3   remote.variation.constraint_mode   0 GEN /1 KEY /2 CHORD /3 BOTH
CH15 CC4   remote.variation.chord_block       0..191 zero-based chord block
Use exact framing for all wide integer values, including seed and chord block.
CC7 uses firmware rounding and cannot represent every wide value.
Anchor and mode details are shared with the existing native variation algorithm;
303 playback changes NOTE pitches only and preserves rhythm/articulation.

TRANSACTION
1. Require capability bit8 in command51 page0; CC MODE FULL; stopped/drained
   transport, no record/preview/staging owner and a writable project.
2. Commit source-owned selectors: pattern0..31 plus drum part/kit0..1,
   drum0..10 and matching strip0..21; or303 part2/3,bass0/1,strip22/23.
3. Stage variation fields with an explicit seed (zero is valid). For303s stage
   constraint and explicit chord block; GEN uses this303's generator scale/range,
   KEY uses project key/scale, CHORD uses selected block, BOTH intersects GEN/chord.
   FREE, absent/empty chord intersection and auto-mutate refuse. No live chord capture.
4. Set BOTH low/high bytes of a fresh monotonic14-bit request. Shared arm control
   remote.arm accepts4 ON /5 OFF (existing2=303 generation,3=drum generation).
5. Execute matching ON/OFF low0 then high1. Read page4 matching request result,
   then page9 live recipe. No automatic playback or flash write occurs.
   OFF requires matching live pattern/lane if a variation is active; OFF with no
   variation is a true no-op. OFF does not require a staged seed.
6. Cancel invalidates a pending arm; repeated execute of an unchanged request is
   deduplicated. Stale project,transport epoch,runtime recipe,context or staged recipe
   is refused. Another source can replace the global live recipe; existing arms
   then become stale. Read current page9 before acting.
Arm/OFF require stopped transport. They do not mark project dirty, add Undo entries
or persist the variation across project load/reboot. APPLIED/NO_CHANGE compares the
live runtime recipe; project revision may stay unchanged. Do not infer success from
project revision alone or treat staging readback as playback/delivery evidence.

READBACK - existing command51 ABI1; CRC/token/source/page/index validation retained
Every page begins revision,queue_written,queue_read,transport_epoch,generation.
Page0: map1,ABI1,feature_bits15 (3031,drum2,CCdiagnostics4,variation8),sources3,
       identity9000124.
Page7: CC mode,recipe_kind(0=303,1=drum,2=variation),aggregate counters.
Page8: staged every,amount,anchor_lock,seed,variation_mode,shape,range,
       constraint_mode,chord_block,seed_valid,off_request; variation family only.
Page9: global live lane(255=OFF),pattern,every,amount,seed,anchor_lock,
       variation_mode,shape,range,notes_mask,low_note,high_note,context_matches.
Page1: explicit context; page4: correlated action receipt. Pages2/3 retain their
303/drum-only meaning and are unavailable for a staged variation recipe.
Pages5/6 remain unsupported. Command49 reads unchanged stored musical data;
command50 offers bounded runtime diagnostics, not complete event or MIDI history.
tools/fm1_variation_plan.py prints public wire plans and never opens MIDI ports.
tools/fm1_generation_read.py decodes/validates actual query replies.

CC MODE: FULL/SIMPLE/OFF remains session-only, native GLO only. SIMPLE permits
direct SOUND/STRIP controls; selectors,generation,variation,transport/storage CCs
are blocked. OFF blocks custom CC. Notes/clock/Start/Stop and readback remain.
Init/project-load/reboot return FULL. A mode change invalidates pending contexts.

LOCAL EVIDENCE
tests/host/run_midi_variation.sh:24 lanes,three formats,three sources,303 constraints,
runtime/project/Undo preservation,request/cancel/stale/source/owner/mode guards.
tests/test_variation_readback.py:real native command51 CRC/token/domain fixtures.
tests/host/midi_variation_wasm_test.mjs:actual production worklet public wire plans,
all24 lanes,save parity,Arm/OFF/cancel and independent native MIDI attack traces
for eight passes on three representative drum lanes and both303s. MIDI trace
comparison includes legato/retrigger behaviour; it is not physical port acceptance.
tools/midi_variation_check.sh:full134 required checks/39 host scenarios plus inherited
generation,variation,Freeze/Commit,overlay,diagnostic and boot regressions.
tests/host/run_midi_variation_target.sh:original15 resource gates, unchanged limits.
Hardware USB/TRS,DAW,display/listening,jitter/latency and long-run gates remain open.
