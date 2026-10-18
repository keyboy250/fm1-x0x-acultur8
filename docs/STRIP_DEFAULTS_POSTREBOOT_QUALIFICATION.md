# Per-sound defaults: post-reboot qualification, 2026-10-16

User resumed FM1 development. Supersedes the paused and pre-packing pending descriptions; prior failures are retained as evidence. FM1-45 remains doing until the complete acceptance boundary passes. No integration, public preview replacement, Pages publication or hardware flashing in this checkpoint.

## Observed passes

- Focused native defaults: all 25 sounds, selected-only recall, explicit Save/Apply, no-op flash writes, reopen, Undo/Redo and invalid/oversized/future/protected/I/O/transport/queue refusal. Actual UI read-Stop leaves project/history unchanged; a Stop after accepted preflight remains pending and recall remains undoable.
- Actual WASM/worklet: independent BD/CH/BREAK defaults, durable-bank replay, explicit recall, project save/reopen and busy/future-format rejection. Production UI harness: all25 targets, matching native/storage/audio-ticket gates, failure/retry and stale-event handling.
- Host whole-store compatibility: old seven-object snapshots load without defaults, including replacing a previously loaded eighth bank in the same process; new eight-object snapshots roundtrip byte-exactly; five truncated records reject. The pre-fix executable reproduces the legacy EOF failure.
- Target: 578000/581564 image bytes, ordinary RAM98300/98304, pool335772/344064. All15 existing static gates and supplemental defaults/clock direct-call helper ledger pass. Margins are3564 image bytes,4 ordinary RAM bytes and100 bytes above the required8192 pool reserve. These static limits do not prove device timing, IRQ or stack safety.139 frozen firmware/tool/public-asset inputs match the candidate.
- Real browser at isolated localhost8135: distinct BD73Hz/17000Hz/+9dB/-11dBFS/173ms and CH1200Hz/13000Hz/+3dB/-6dBFS/47ms defaults saved. Explicit recall, selected-only isolation, Undo/Redo, reload durability, no automatic project replacement, and playback disable/Stop reenable pass. Browser reload retained project CH501Hz/91ms until explicit recall. No private loops used.
- Pinned hi-hat and packed-ride PCM equality plus sanitizers pass. Independent source review finds no remaining blocking defaults defect.

## Corrections and preserved failures

Saved oversized-bank truncation and Stop/undo-ordering repairs are now exercised by permanent regressions. Host dump tests use the real tap SAVE command. The full suite first stopped on generated CC source-fingerprint drift; regeneration changes source hashes only, with all699 entries identical. A second run reached an obsolete native capability expectation; the test now preserves every legacy bit and separately requires the new defaults capability. Earlier logs are retained. The first new target attempt omitted public assets; the assets-complete run passes. None of these failed runs is counted as a complete-suite pass.

## Still pending

The final mandatory suite reached terminal PASS9/9 (2026-10-16T08:57:36Z), including31 host scenarios, host/WASM parity and41-file package checks. Tested artifact: build/site-wah286mb; final report preserved as build/strip-defaults-required-postreboot-final-report-20261016.json. Real-browser quota-exhaustion fault injection remains separate from the passing synthetic production-UI failure/retry fixture. Physical NOR/power-loss, native menu, USB/TRS/DAW, audio IRQ/stack/timing, listening and long soak remain pending. Preserve accepted expanded-rate preview build25641496, its origin and saves. Pages remains held.

## Evidence and resume order

See build/strip-defaults-focused-postreboot-20261016.log, build/host-store-compat-green-20261016.log, build/strip-defaults-browser-postreboot-20261016.json, build/strip-defaults-target-postreboot-assets-20261016.log and build/strip-defaults-target-postreboot-20261016/build/strip-defaults-helper-ledger.json. Passing complete-suite log: build/strip-defaults-required-postreboot-slots-20261016.log. Recovery patch/untracked copies: build/resume-defaults-20261016.

Complete-suite and exact package checks pass. Preserve a selective checkpoint and assess local integration next. Do not stage inherited dirty documents indiscriminately. Physical tests remain explicit pending work.

## BREAK navigation regression correction

The combined suite exposed two obsolete journeys: BREAK boot sound view already shows LOOPS, and two EDIT presses now reach STRIP LIMITER. Corrected actual-WASM slots and host upload journeys select loop A directly; added native coverage asserts LOOPS first followed by the three strip pages. Loaded/empty PCM signatures now differ, upload selects slot2, and all31 scenarios pass in the final suite. Runtime slot loading was not changed. Earlier failing logs remain intact.

## 2026-10-17 - Physical FM1 installation and303B default recall

2026-10-17 physical FM1: stock V15 backed up as two matching1MiB reads. Reviewed rates/defaults build and then boot-animation candidate installed (FM-1_900).303A/303B filters and limiter pages visible.303B saved HP146Hz/LP10240Hz with individual filters enabled; HP100->146Hz recall verified before and after normal power cycle. Only HP recall/persistence is accepted. Apply after restart showed a blank message despite successful recall (FM1-49). Other fields/sounds/isolation/audio/DAW/IRQ/stack/soak/power-loss remain pending. Pages held.

Requested default policy (FM1-48): normal HP150Hz; selected drums may use55..149Hz; no default below55Hz; LP at most11000Hz.146Hz/10240Hz are manual quantized test values;146Hz is below a strict150Hz standard. Choose exact supported values/drum exceptions and resolve default policy versus hard editing limits. No global preset change or hard clamp has been implemented.

Boot intro (FM1-47): image578896 (+896) bytes, ordinary RAM98300, pool335772; no persistent RAM increase. Original resource/helper and sanitizer timer-wrap/2second/watchdog/USB/button/OTA/ROM tests pass. Package SHA256 f9ebebb9cea5c164cef974e2932bc873bed8b9abd9dc04116a6ab5397e0d9799 installed. UI/powercycle used; animation-specific visual/skip acceptance still pending. No repair, repeat flash or publication in this documentation checkpoint. Pause at user request; resume with FM1-49.
