# Live CC native candidate

This is an isolated, unmerged implementation candidate based on `1e2d76f626d4384756775a1ecad07894a2690c10`. It is **not** complete live-CC acceptance, browser/DAW acceptance, device qualification or a firmware release.

## Ownership and ingress

The production USB and TRS packet producers run in TIMER5 interrupt context. They only publish the existing input ring. A new main-ready cursor publishes the prefix reviewed by main context; audio consumes that prefix only. Main validates and updates canonical intent, then publishes commands through the existing main-producer/audio-consumer CQ. No CQ opcode was added: all sixteen opcodes remain occupied, and CC refresh uses the existing arrangement marker with an explicit reconciliation flag.

Each main service reviews at most 32 packets: a contiguous CC prefix followed by a contiguous non-CC prefix. It stops before any following CC once a non-CC event has been published. It does not inspect later CC intent until audio has consumed that published prefix. Audio drains all CQ entries (existing CQ capacity 1024) before dispatching the MIDI prefix; main and audio run serially on the host/browser, and target main cannot resume inside the audio interrupt. Consequently the consumed cursor observed by main marks completed event application, including Note, Note-Off, clock and transport. This introduces a main-service/audio boundary at each non-CC-to-CC transition; up to 32 adjacent notes/clock/transport events share one published prefix. Mixed-event backlog latency and physical throughput remain unqualified. Urgent overflow recovery is an explicit fail-safe Stop/cleanup exception, not silent chronological replay.

A full CQ returns busy without consuming an exact transaction's three value chunks. The same queued commit retries after audio drains; any terminal accepted/rejected/unsupported result consumes the transaction. Exact framing and selector addresses cannot be exact-transaction targets: they are accepted only through their ordinary CC addresses, preventing recursive commit and self-modifying context. Native tests reject every framing/selector target without changing project bytes, then complete a valid musical transaction through the busy/retry path.

The same ring storage serves IRQ, main and audio; it is not replaced by another large mailbox. Adjacent sound/strip value packets from the same input source and address can coalesce only beyond the main-ready cursor and outside main's claimed slot. Selectors/actions are never coalesced. Native interrupt-interleaving tests verify that a packet copied by main cannot be overwritten by a producer.

CC traffic reserves sixteen ring slots; ordinary notes reserve eight; Note-Off, velocity-zero Note-On and realtime transport/clock can use the remaining capacity. Critical overflow raises a main-context recovery request, preserving Stop/cleanup work when the CQ is busy. These are bounded admission/recovery guarantees, **not a measured physical MIDI deadline**. Reviewed CC tombstones do not consume the audio dispatcher's MIDI event budget. CC never enters an outbound echo path.

USB, TRS and the future selected browser input have separate contexts. Repeated pattern/part/bass/step/block/motion/strip fields use staged MIDI selectors and an explicit commit. UI page selection is not a parameter target. Main source reset clears incomplete exact/context transactions and unreviewed queued old-source CCs, preserving Note-Off/Stop and other sources. The physical main loop resets USB control context on disconnect/suspend; TRS has no reliable cable-presence signal. Late browser input events still require client/worklet generation rejection before browser ingress is promoted.

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

- Individual 25-lane 1–64 lengths and eight rates are explicit `engine_cc_lane_set` callbacks, returning unsupported until the independently developed poly-lane branch is integrated and tested. They remain required.
- Optional CC14 companion decoding is not implemented; direct CC7 and the exact ordinary-CC transaction are implemented candidates.
- Browser ingress/port permission/selection, worklet source generations, actual compiled-WASM source-reset delivery and GUI readback are pending.
- Browser marked-WAV/preview callbacks, physical MASTER gain takeover/pickup, and nonmusical/file/storage action callbacks remain explicit unsupported rows.
- Settings excluded by the existing undo region need a reviewed policy if complete global-setting undo is required. No claim of expanded coverage is made.
- Full native regressions, actual WASM/production worklet, signed-in/real DAW browser acceptance, target ROM/RAM/pool/static cost, worst-case IRQ/stack/DMA timing and physical USB/TRS delivery are pending.

The full all-musical-parameters requirement remains intact. If the complete configuration does not fit, the implementation must be redesigned; unsupported rows and resource failures cannot be hidden or marked complete.
