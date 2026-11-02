0.1.26-copymidi / FM-1_9000126 - isolated local candidate
No device access,installation or publication. Physical acceptance remains open.

SCOPED / CROSS-PART COPY
Six additive CC addresses preserve all721 parent assignments. Human CH15:
CC9 source pattern0..31;CC10 destination0..31;CC11 source scope0..24;
CC12 destination scope0..24;CC13 preview/arm0..1;CC14 execute0..1.
All integers are reachable with CC7 round(raw*maximum/127). Existing exact
framing also works after a committed legacy selector context. Pure copy CC7
plans require no legacy selector or panel focus. Copy staging never edits the
native GLO copy configuration. Shared request low7/high7 and Cancel unchanged.
Shared arm still has max7:arm0 disarms,2..7 retain all previous meanings.

Scopes0 ALL;1..11 909 BD SD LT MT HT RS CP CH OH CR RD;
12..22 808 BD SD LT MT HT RS CP CB CY OH CH;23 303 A;24 303 B.
ALL pairs only with ALL,drums with drums,303s with303s:489 compatible pairs.
Same-pattern cross-part copy supported;identical complete tuple is NO_CHANGE.

Stage all four targets,both14-bit monotonic request bytes,then preview/arm1.
Preview checks motion/Undo capacity without changing project/history/storage.
Require command51 page10 preview_valid1 and page4 ACCEPTED for that request
before execute0 then1. Recipe/project/transport epoch/owner/queue/protection
changes refuse execution. Another source may stage independently;its execution
invalidates the first source's project guard. Repeated applied requests dedup.
Cancel,preview/arm0,shared arm0 or source reset disarm. New recipe requires a
fresh request ID. Per-source recipe16B/state72B unchanged;no new persistent data.

Same-scope copies include scoped musical motion. Cross-part copies preserve
destination tone automation,timbre and303 accents/generator accent while moving
notes and matching probability/condition lane. Destination swing/shared drum
accents stay. Legacy single-row copies preserve shared drum length/rate;
initialized independent lane timing follows the copied lane. Source,unselected
parts,song references and unrelated sound bytes remain. Undo/Redo is one native
transaction. No implicit MIDI SAVE or start is sent by the plan.

READBACK:command51 ABI1 page10 index0,following five common stamps:
source_pattern,destination_pattern,source_scope,destination_scope,
recipe_valid_mask,preview_valid,current_fingerprint,referenced_bars,
source_motion_count,destination_motion_count,same_target.
Valid mask15 means all four explicit targets staged. References count distinct
song bars using destination scope. Same-target compares tuple only. Motion
counts describe lanes selected for transfer/replacement,not a full diff.
Page4 is correlated result receipt;reason15=copy motion or Undo capacity.
Page7 recipe_kind4=copy. Capability bits63 include bit5 copy. Identity9000126.
Command49 independently reads actual notes,timing and motion on both slots.
Page10 is available only while the source owns a copy recipe. Changing CC MODE
clears source recipes,so page10 then replies unavailable until restaged in FULL.
Read-only capability/result/diagnostic queries still respond in SIMPLE/OFF.
SIMPLE and OFF block all copy controls. CC MODE is session-only,default FULL.

LOCAL CHECKS
tools/midi_copy_check.sh runs full inherited MIDI/release checks,new copy matrix,
wire decoder,existing native UI copy suites,489 production worklet route plans.
Also run tests/host/run_midi_copy_guards.sh and native menu WASM regressions
pattern_copy_wasm_test.mjs/cross_part_copy_wasm_test.mjs. ASAN/UBSAN native matrix
covers489 pairs x3 storage formats x3 ingress sources x2 pattern relations.
Existing UI copy suites independently verify destination preservation rules.
tests/host/run_midi_copy_target.sh builds only a local firmware package and
checks15 unchanged target gates. Existing8192B pool reserve must remain.
Frozen receipt and source/fixture hashes are authoritative for completed runs.
No local test establishes physical USB/TRS delivery,DAW,audio or device timing.
