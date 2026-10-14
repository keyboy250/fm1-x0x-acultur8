# Individual lane clocks — local engineering candidate

The feature contains 25 existing lanes: 909 rows 0–10, 808 rows 11–21,
303 A/B 22–23, BREAK 24. Each of 32 patterns has length 1–64 and eight
rates. Selectable paired 808 timbres remain one lane per existing row.

Rate IDs retain 0–3 (16th, 16th triplet, 32nd, eighth triplet); IDs 4–7
are quarter, eighth, quarter triplet, 32nd triplet. Their 24-PPQN periods
are 6,4,3,8,24,12,16,2 ticks. Arrangement still uses 96 ticks per bar.
Timing edits apply at the next local step; the current interval/gate completes.
The five existing motion clocks follow 909 BD, 808 BD, A, B and BREAK.
This does not introduce 25 independent motion recording streams.

Storage keeps sound_t, pattern_t, project_t and undo shadow sizes unchanged.
Twenty-three non-bass timing pairs occupy 207 bits (26 bytes) per pattern.
Thirteen bytes reuse both drum headers (8), BREAK rsv (2), pattern rsv (3).
The other 13 timing bytes, four high BREAK gate bytes and sixteen additional
packed BREAK slice bytes require 33 bytes per pattern, 1056 total.
An explicit eight-byte LAN1/version/row-size/pattern-count/lane-count header
makes 1064 bytes. The descriptor audit finds **1070**, not 1071, inactive
sound cells: the additional T_FX[1][0] TAPE preference must be excluded.
The six remaining cells are unowned. Every owned offset is checked against
actual engine descriptors and the hidden TAPE preference in native tests.
Each slice is 0 (generator) or 1–8; all 64 gates and slices are independent.

Schema 5 retains the 412-byte variant and reserves flag bit7 for initialized
typed lane storage;
bit6 remains reserved for the separately required arrangement loop work.
Enabled saves use protected stream format5, CRC over the contiguous sound,
patterns, song, motion and variant. Old streams validate their original schema
masks before migration. Defaults remain disabled; no metadata writes occur to
legacy inactive bytes until explicit enable. Older firmware rejects format5.
Header byte4 has version1 in bits0–6 and desired independent-clock mode in
bit7. Off/On retains every prepared timing and all64 BREAK gates/slices.
Fresh uninitialized Off reads original raw headers and preserves inactive
bytes exactly. Initialized Off decodes the stored BD timing as each kit's
shared conductor, while A/B and the full BREAK keep their stored clocks.
Initialized Off is grouped-clock playback, not an old-format byte-equivalence
claim. Both initialized modes save protected format5. Older formats2/3/4
retain their exact validation masks before migration; no raw inactive byte
can activate metadata merely by resembling a header.

ABI1: command18 SET_LANE target=pattern0–31, voice=lane0–24, index0=length
1–64 or index1=rate0–7. Command19 SET_LANE_MODE target/voice/index=0,
value0/1. First initialization requires stopped/no pending transport and
uses the shared long-operation reservation with interrupts enabled for bulk
conversion. Subsequent Off/On commands are live: the audio owner applies the
latest requested mode at the next96-tick musical-bar boundary, before part
events. Enabling starts the additional20 drum clocks on step1 at that
boundary; the five conductor/303/BREAK clocks and held bass state continue.
Disabling groups each kit on its continuing BD clock. It does not clear
prepared rows, release held bass notes or reinitialize patterns.

Command18 index2 is BREAK only (voice24): value bits0–5 select step0–63,
bit6 is its gate, bits7–10 hold slice0–8; upper bits reject. Timing and BREAK
edits are available in initialized Off as well as On. Editing uses explicit
pattern/lane IDs and remains independent of playing selection.

Snapshot10 count9, target=pattern, voice=lane:

1. initialized
2. desired length
3. desired rate
4. playing pattern for this lane's part
5. effective position or255 while stopped
6. effective cycle count
7. latched rate for the current interval
8. effective independent-clock mode
9. audio-owned local timing pending (0/1)

Snapshot11 count7, address0/0: initialized, desired mode, effective mode,
mode pending,25 lanes,8 rates,64 steps. Snapshot12 count5, target=pattern,
voice=32-step page0/1: gate word and four words of eight4-bit slice values.
Capability1024; 512 remains reserved for arrangement loop work.
Commands must publish complete packed fields before their queue marker, with
no canonical/history mutation on invalid, protected, busy or full rejection.

Ordinary-RAM planning adds twenty8-byte clocks, a ten-byte latched-rate cache,
an audio-owned25-bit pending mask, an effective-mode byte and one metadata
pointer. Target32-bit growth measured180 bytes; final ordinary RAM is98048 bytes.
No pool allocation, project expansion or undo-shadow expansion is introduced.
The available pool reserve margin remains160 bytes before any other work.
Native64-bit measurements: sound1344, pattern528, project31356,
undo-shadow31328, sequencer440, extra-row clock8 bytes.

External timing uses the latched old rate through the current step. Clock
loss converts its remaining tick phase into an internal countdown, avoiding
an immediate duplicated onset. Continue retains positions/cycles and arms
the next step as the legacy transport does. Multiple queued clock ticks have
no sample timestamps: every logical event is delivered in clock order at
the current render boundary; overdue swing is collapsed rather than replayed
as fabricated historical audio. A fresh single tick retains swing.

Native checks pass exhaustive packing plus independently indexed seeded
alias tests, descriptor/TAPE separation, migration/protected rejections,
actual macro spans and whole-part copies, preserved modes/undo/reopen,
eight period oracles, local timing transitions, ties/Continue/fallback and
49 queued external ticks. Independent pinned pre-lane native PCM matches
262144 frames exactly (SHA256
fa0d83020bdc94d96b1d056595b09ea1a65666f114dd063e0a2f9a519e7942b7).
The parent ran the production-WASM runner successfully, including the
independent pinned pre-lane patterned Off PCM oracle. The pre-gesture-fix
candidate WASM is677254 bytes, SHA256
22ee6616b04f029e1e5b1a4fe7772cac96b32a8ab42a5fcacc3f2621917d1a78.
Mandatory suite9 also passed, artifact `site-2wrhduaj`; this is software
evidence for that checkpoint, before the later worklet gesture fix and ROM
outlining. Those changes require fresh acceptance.
The existing PERF benchmark uses a separate three-pattern legacy array and
is unavailable with initialized storage, in either clock mode. Its corrected
LCD message is `BENCHMARK NEEDS LEGACY PROJECT`.
The benchmark must not pretend its array contains the32-pattern slab.
Factory reset safely removes typed storage through the shared reservation.

The first official lane target build overflowed XIP by11208 bytes.
Behavior-preserving helper outlining and shared transport reduced this to5160,
3688 and3144 bytes; cold main-only minsize attributes subsequently reduced it
to2808,408,296 and40 bytes. The put_i outlining experiment saved nothing and
was reverted. No assets, DSP math, linker limits or original budgets changed.

The final official target (`build/target-lane-drain-outline.log`) fits:
image581236/581564 (328 bytes free), ordinary RAM98048/98304 (256 free),
pool335712/344064 (8352 free,160 above the unchanged8192-byte reserve).
All15 original static gates pass. This is compiled resource evidence,
not physical timing or worst-case stack acceptance.

The final fresh mandatory suite9, including production-WASM lane, gesture
and pinned pre-lane Off PCM checks, passes (`build/site-qdz1i1rd`).
WASM670784 bytes, SHA256
 ae7e3ca1afb2107e5ef5c83ee87bbaf0f3a2fed88c8c4a6f6fa2de5c8b1e1318;
source SHA256
 ea711902a294594453ddcee7d654930cfc4eb7298111496d78980f963204ca0b.
Disposable final native copies also pass lane, arrangement, bass-follow,
pad and independent262144-frame pinned Off PCM checks. Five scenarios/30
whole RGB screenshots match the pre-ROM-outline checkpoint. Both private
pixel fixtures fix host CPU-load telemetry to17; production host code is
unchanged, no pixels were masked, and this synthetic telemetry does not
qualify actual CPU-load display or physical performance. Evidence:
`build/lane-rom/final-cold-pixels/font-scenario-report.json`.

`tests/lane_feature_costs.py` regenerates the address-qualified additional
helper/caller/frame ledger in `tests/lane_feature_costs.json` from the exact
existing disassembly and unchanged original gate model. The final drain
helper runs once per outer engine_render before event/audio processing,
at its original queue/MIDI dispatch position. Baseline engine_render embeds
drain:953 instructions/frame128; final caller611/frame124 plus outlined
drain416/frame60. Lower caller metrics moved work into helpers; even adding
these prologue allocations is not a worst-stack calculation. Callback
alternatives and transitive callee chains are recorded conservatively.
See `docs/ACULTUR8_LANE_STATIC_COSTS.md` for regeneration and limits.

Live CC routing is a separate integration lane: these main-producer APIs
must not be called as a second producer directly from the audio MIDI IRQ.
All-parameter DAW CC acceptance is still pending that registry/ownership
integration. Static resource checks do not establish physical timing, worst
stack, receiver delivery or hardware acceptance. This candidate is private.
