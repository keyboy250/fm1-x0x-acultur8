# Live CC native candidate

This is an isolated, unmerged implementation candidate combining reviewed standalone lanes `d659184` and native CC `c764899` in the isolated lane-cc workspace. It is **not** complete live-CC acceptance, browser/DAW acceptance, device qualification or a firmware release.

## Ownership and ingress

The production USB and TRS packet producers run in TIMER5 interrupt context. They only publish the existing input ring. A new main-ready cursor publishes the prefix reviewed by main context; audio consumes that prefix only. Main validates and updates canonical intent, then publishes commands through the existing main-producer/audio-consumer CQ. No CQ opcode was added: all sixteen opcodes remain occupied, and CC refresh uses the existing arrangement marker with an explicit reconciliation flag.

Each main service reviews at most 32 packets: a contiguous CC prefix followed by a contiguous non-CC prefix. It stops before any following CC once a non-CC event has been published. It does not inspect later CC intent until audio has consumed that published prefix. Audio drains all CQ entries (existing CQ capacity 1024) before dispatching the MIDI prefix; main and audio run serially on the host/browser, and target main cannot resume inside the audio interrupt. Consequently the consumed cursor observed by main marks completed event application, including Note, Note-Off, clock and transport. This introduces a main-service/audio boundary at each non-CC-to-CC transition; up to 32 adjacent notes/clock/transport events share one published prefix. Mixed-event backlog latency and physical throughput remain unqualified. Urgent overflow recovery is an explicit fail-safe Stop/cleanup exception, not silent chronological replay.

A full CQ returns busy without consuming an exact transaction's three value chunks. The same queued commit retries after audio drains; any terminal accepted/rejected/unsupported result consumes the transaction. Exact framing and selector addresses cannot be exact-transaction targets: they are accepted only through their ordinary CC addresses, preventing recursive commit and self-modifying context. Native tests reject every framing/selector target without changing project bytes, then complete a valid musical transaction through the busy/retry path.

The same ring storage serves IRQ, main and audio; it is not replaced by another large mailbox. Adjacent sound/strip value packets from the same input source and address can coalesce only beyond the main-ready cursor and outside main's claimed slot. Selectors/actions are never coalesced. Native interrupt-interleaving tests verify that a packet copied by main cannot be overwritten by a producer.

CC traffic reserves sixteen ring slots; ordinary notes reserve eight; Note-Off, velocity-zero Note-On and realtime transport/clock can use the remaining capacity. Critical overflow raises a main-context recovery request, preserving Stop/cleanup work when the CQ is busy. These are bounded admission/recovery guarantees, **not a measured physical MIDI deadline**. Reviewed CC tombstones do not consume the audio dispatcher's MIDI event budget. CC never enters an outbound echo path.

USB, TRS and the selected browser input have separate contexts. Repeated pattern/part/bass/step/block/motion/strip fields use staged MIDI selectors and an explicit commit. UI page selection is not a parameter target. Main source reset clears incomplete exact/context transactions and unreviewed queued old-source CCs, preserving Note-Off/Stop and other sources. The physical main loop resets USB control context on disconnect/suspend; TRS has no reliable cable-presence signal. Late browser input events still require client/worklet generation rejection before browser ingress is promoted.

## Native proof recorded

Run `sh tests/host/run_live_cc.sh` using a native compiler. This does not invoke Emscripten or a target compiler.

- Every one of the 273 current engine pot descriptors reaches canonical sound storage and the existing audio queue while transport runs, including both kits, both basses, BREAK, FX, all mix parts and master.
- All 25 processing strips accept cutoff/boost/stage changes while transport runs.
- Explicit pattern/bass contexts remain isolated between input sources and from the open panel. Current part length/rate and generator metadata/new-line changes execute while running.
- All 35 probability conditions and `MOT_NONE` preserve their exact canonical values. Motion domains are validated against each current engine descriptor.
- An ordinary-CC three-chunk exact transaction carries 21 bits, including 300000. The browser source-BPM setter is still unsupported; representing its value is not application acceptance. Legacy high2 remains compatible; new high7 occupies previously unused parameter number 1130.
- Running follow changes reproject from the accepted original NOTE; follow Off restores its absolute source. Invalid narrow bounds release the held note without a TIE or later CC reviving it.
- The actual native browser C FIFO retains old pad-channel ownership on saturated cleanup, then accepts old-channel Offs before the replacement triad. Channel conflicts leave canonical intent unchanged.
- CC sound changes retain stopped undo/redo and save/reopen behavior. No CC performs a flash save. Existing settings outside the legacy undo region retain that limitation; this proof does not claim new undo coverage for every global setting.
- Protected storage rejects musical edits but cannot suppress the safety Stop CC.
- Actual engine tests cover both note/channel directions, Stop then CC Play versus CC Play then Stop, both external-clock/follow directions, CQ-full exact commit retry, and full-ring reserved Note-Off/clock/Stop recovery.
- Production USB/TRS paths compile with CC enabled, and deterministic IRQ/main interleavings prove main-only canonical mutation, source tags, busy-head retention and urgent-overflow recovery.
- Existing native variant/storage/undo, 81648-case bass-follow/generator/USB ownership, and browser C FIFO prefix/Stop/reset regressions pass on this candidate. These focused suites do not establish full mandatory or WASM acceptance.

## C ABI prepared for browser coordination

`web_midi_ingest(packet, source)` returns bounded input-ring acceptance; the selected browser input uses source2. `web_cc_reset_source(source)` invalidates incomplete control context and unreviewed CCs; callers must reject stale port events by their own generation. `web_cc_source(source, field)` exposes applied selectors0–11, ready12, parameter13, exact parts14, exact value15, and draft selectors16–27. `web_cc_stat(index)` exposes accepted/rejected/unsupported/busy/coalesced/overflow at indices0–5. `web_cc_pending()` reports input-ring backlog. `web_cc_version()` identifies ABI1/map1 and does **not** assert complete parameter coverage. These are native syntax/tested C exports, not compiled-WASM/browser acceptance.

## Complete-scope gaps

The exhaustive row ledger is `ACULTUR8_CC_RUNTIME_COVERAGE.json`. A candidate binding is not a completed acceptance result.

- Individual25-lane lengths/rates and prepared mode control now have focused native integration proof below. Actual compiled-WASM/browser and target acceptance remain pending.
- Optional CC14 companion decoding is not implemented; direct CC7 and the exact ordinary-CC transaction are implemented candidates.
- Browser ingress/port permission/selection, worklet source generations, actual compiled-WASM source-reset delivery and GUI readback are pending.
- Browser marked-WAV/preview callbacks, physical MASTER gain takeover/pickup, and nonmusical/file/storage action callbacks remain explicit unsupported rows.
- Settings excluded by the existing undo region need a reviewed policy if complete global-setting undo is required. No claim of expanded coverage is made.
- Full native regressions, actual WASM/production worklet, signed-in/real DAW browser acceptance, target ROM/RAM/pool/static cost, worst-case IRQ/stack/DMA timing and physical USB/TRS delivery are pending.

The full all-musical-parameters requirement remains intact. If the complete configuration does not fit, the implementation must be redesigned; unsupported rows and resource failures cannot be hidden or marked complete.

## Individual-lane integration checkpoint

Stable parameter numbers1128 length,1129 rate,1130 exact-high7 remain unchanged;
1131 adds independent-clock mode (channel13 CC107). Length uses1–64 and all
eight existing rate IDs. Explicit committed selectors choose pattern0–31 and
strip0–24; CC never borrows panel selection. Existing part length/rate addresses
route through the same packed engine setters when metadata is initialized;
legacy descriptor values retain their old range when uninitialized. BREAK
gate1029/slice1030 now cover selected step0–63 after initialization, retaining
their original addresses. Legacy uninitialized projects reject steps16–63.

Initial conversion remains stopped/no pending transport, rechecked by the
shared reservation under IRQ exclusion. Ineligible initialization is terminal
REJECTED, allowing a later Stop to proceed. Only genuine CQ exhaustion yields
retryable BUSY. Exhausted motion allocation likewise rejects terminally rather
than blocking following transport indefinitely. Subsequent prepared Off/On is
live at the next96-tick common bar; timing retains the current interval and
latches at the next local step. Prepared Off retains all25 settings/BREAK64.

`run_live_cc.sh` now includes `lane_cc_test.c`: all32 patterns x25 lanes x8
rates x64 lengths are dispatched/read back while playing; a nonuniform matrix
and200 seeded mutations verify distant independence. All32x64 BREAK gates and
slices, format5 save/reopen, full-CQ retry before Stop, semantic and IRQ-start
rejections, protected storage, all25 mixed-rate/odd-length engine positions,
common bar clock and live old-rate phase/Off-On boundaries pass. The original
lane model/sequencer suite also passes. Independent pinned pre-lane native Off
PCM matches262144 frames exactly:
`fa0d83020bdc94d96b1d056595b09ea1a65666f114dd063e0a2f9a519e7942b7`.

Physical UI and CC are ordered main-context producers of the same canonical
bytes and one undo journal. The last accepted write wins. Incoming automation
inside a panel gesture joins that gesture's journal group; no independent
source undo or CC ownership lock is claimed. The actual native test interleaves
UI and two source contexts, then verifies grouped Undo/Redo and isolated lane
readback. IRQ producers publish packets only; packed writes use the shared
bounded engine critical section. The1056-byte payload/8-byte header, excluded
TAPE cell, project/shadow sizes and default uninitialized PCM remain unchanged.

This checkpoint does not inherit standalone lane target acceptance. Combined
ROM/RAM/callee/stack costs require a fresh target probe; no budgets are relaxed.
All690 inventory rows remain accounted for with explicit implementation gaps:
MASTER takeover/pickup; browser marked-player/import/action callbacks; optional
paired14-bit decoding; destructive/storage callbacks; global settings outside
the existing undo region; actual DAW/USB/TRS/browser delivery and performance.
The complete all-musical-controls requirement remains pending those checks and
implementations. All work remains private.

The generated690-row coverage ledger currently marks649 native candidates,
27 unimplemented and14 explicitly unmapped/read-only/nonmusical. The27
unimplemented direct bindings are action.1–11, output.master_knob,
slice.level_percent/repeat/layer/source_bpm_milli/audition_index/play/stop/
arm_sync/import/demo/save_import/reopen_import/export and editor.undo/redo.
Native-candidate status means dispatch exists, not exhaustive per-row acceptance.
Optional14-bit decoding and global-undo limitations are additional gaps rather
than falsely promoted rows. The676 lookup entries declare4312 bytes of constant
number/word/cutoff tables before alignment; linked retention and code/ordinary
RAM must be measured on the combined target. No pool or persisted expansion
was introduced by the lane CC integration.

## Decoder and production USB review correction

The first combined target failed at XIP overflow11272 bytes
(`build/target-combined-first.log`). The first full emulator suite passed,
but it did not qualify the subsequently identified browser ingress return-value
bug or production USB urgent-fullness bypass. Those outcomes are separate.

The reviewed production USB fix routes all valid MIDI packets through the CC
admission helper even when the input ring is full; only the non-CC legacy
branch retains its old outer capacity behavior. A native HAL fixture invokes
actual `ep1_rx` and production SIE helpers at full capacity: nonurgent CC/NoteOn
drops increment accounting; Stop/NoteOff/velocity-zero NoteOn schedule main-only
panic cleanup, retained while cleanup is busy. This corrects the earlier test's
insufficient direct-helper-only USB overflow coverage.

Generated CC lookup now retains60 eight-byte runs plus arithmetic decoding of
the regular25-strip grid. Production table declarations shrink from4312 to736
bytes (including unchanged256-byte cutoff table), saving3576 data bytes before
linked code/alignment effects. No existing wire address, allocated unsupported
row or validation is removed. Test-only complete tables are conditional and
not retained by physical production. The generator checks all65536 numbers and
holes against its full inventory before emitting output; an actual C native
comparison independently checks the same domain against the frozen676-entry
pre-decoder map in `tests/host/cc_mapping_v1_reference.h`. Native CC4 fixtures
and inventory9 checks pass after both changes. Combined linked resources and
physical callback/IRQ costs still require a fresh measured probe.

## Prospective validation and bounded publication checkpoint

The decoder target probe measured3424 linked bytes saved, leaving7848 bytes
XIP overflow (`build/target-combined-decoder.log`). Four main-only POLY_COLD
attributes then saved96 more, leaving7752 (`build/target-combined-cold.log`).
Neither failed link establishes final ordinary RAM, static gates or timing.

ARR/FOLLOW/PAD now share an outlined prospective helper. The204-byte main-owned
candidate copy and192-degree/harmony validation run with IRQs enabled; publication
rechecks CQ capacity and current pad channel availability under the lock, then
writes only the changed desired byte and existing marker. Song length/queue
retain their previous validation scope. Legacy recorded song bytes can change
in audio context, so duration/mute read the current byte under lock and preserve
its other bits; pattern changes publish only their selected byte. No stale
whole204-byte candidate replaces canonical storage.

Native tests wrap the actual validator and assert IRQ exclusion is inactive.
Invalid theory/range/collision and publication-capacity failures preserve project,
journal, shadow, revision and storage. Controlled lock-entry capacity/channel
injections prove defensive rechecks; these do not add a production producer.
A real sequencer Start/legacy-song-record callback between prospective validation
and publication verifies all latest unselected song bytes and mute bits survive.
The complete native CC4 fixture runner passes after refactoring. Target/compiler
size, frame/callee costs and full production-WASM acceptance need fresh builds.
Other CC paths are not silently claimed to have new bounded-work qualification.
