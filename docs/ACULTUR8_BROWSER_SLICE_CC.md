# Browser slice CC — local acceptance boundary

The13 marked-audio controls have a HOST/browser implementation through an explicitly enabled selected-input route (source2). Local synthetic acceptance passed with production WASM/worklet/control code and genuine disposable IndexedDB. This does not establish physical USB/TRS slice dispatch, a real DAW/controller, real AudioContext output, receiver delivery, file-download delivery, target resources or hardware deadlines. Shared C native capability masks and persisted schemas are unchanged.

## Stable mapping and receipts

All existing registry addresses remain unchanged:

| Parameter | Channel / CC | Canonical value | Application |
|---|---|---|---|
|1104 level|13 /80|0–100 percent|browser mix gain|
|1105 repeat|13 /81|0/1|original-audition next loop boundary|
|1106 layer|13 /82|0/1|slice replacement/additive browser mix|
|1107 source BPM|13 /83|20000–300000 milli-BPM|canonical source tempo|
|1108 audition|13 /84|0–63|existing slice index; unavailable index rejects|
|1109 play|13 /85|0/1|original trigger timing|
|1110 stop|13 /86|0/1|slice/sync Stop; native transport remains independent|
|1111 sync|13 /87|0/1|next native four-quarter bar|
|1112 import|13 /88|0/1|already selected matching WAV and markers|
|1113 demo|13 /89|0/1|synthetic unequal-onset demonstration|
|1114 save import|13 /90|0/1|IndexedDB transaction completion|
|1115 reopen|13 /91|0/1|stored bytes revalidation then accepted C load|
|1116 export|13 /92|0/1|prepare marker JSON; CC requests no download|

Ordinary CC rounds linearly to the canonical registry range. For actions1109–1116, raw0–63 rearms and64–127 fires once until rearmed; semantic rejection leaves the action retryable. Common native exact framing carries all21bits. Browser consumes only a matching source2 exact assembly; native selectors, staged/applied contexts, arms, destination and edges remain intact. Exact values must match operation and range. Imported finer BPM metadata is retained separately; scheduling retains the existing C float BPM precision. Source BPM changes during sync rearm at the next four-quarter bar and stop current marked tails according to the existing C behavior, rather than promising seamless tempo morphing.

Native prefix receipts acknowledge ingress, not application. One route queue is bounded64 and native prefixes16. A fresh authoritative frame after a native receipt must report pending0 and no transport/reservation/CQ work before a subsequent browser operation. One browser operation runs at a time. The worklet rechecks route, owner, pending transport, native job and reservation at application. Destructive load additionally requires stopped native and slice playback. Unsupported ABI, invalid values/files, ownership, queue/transport activity and storage failure produce explicit rejection reasons. Timeout warns that a result may already have applied; stale-route receipts do not count as new-route acceptance. Source2 route changes never enable themselves or echo to output.

## Host contract and recovery

Required HOST ABI `web_slice_cc_version()==0x00010001` is separate from native CC version/capabilities. `web_cc_take_exact(parameter,value)` uses valid21bit values and returns0 consumed, -1 invalid, -2 unmatched. It consumes only exact-valid bits. `web_marked_repeat(0/1)` returns0 accepted, -1 invalid, -2 unloaded, preserving current cursor/voices. `web_slice_control_state()` flags1 playing,2 pending transport,4 reservation/staging,8 owner/UI input,16 native job,32 CQ/MIDI backlog.

Correlated `slice-command` ABI1 uses a nonzero uint32 request, operation/value and optional route/exact envelope. `slice-reply` reports applied/rejected reason and current slice state. Frame `ccInput.source` carries the actual32-word source2 state; `ccInput.slice` carries version/revision/loaded/count/repeat/sourceBpm/bpmMilli/gain/layer/gate/owned. Missing/old exports fail closed for these new controls.

Import uses selected files only: no chooser, permission request or private asset is opened by MIDI. WAV/marker limits, finite samples, matching metadata/optional checksum, ordered bounds and cyclic eight-voice overlap are checked before replacement. The current import changes only after accepted C load. Unexpected C rejection attempts confirmed prior-audio restoration. IndexedDB writes use an immutable snapshot and transaction completion; failed save preserves the prior committed record. Reopen revalidates before replacement. Engine replacement retains the prior model, restoring once only from a supported fresh idle frame; failed replacement can use its retained engine. Offline preparation and loaded/busy/owned readiness are reflected in actual audition buttons.

Export makes JSON visible. Manual export can request browser download, without asserting delivery; CC never synthesizes a user gesture to download. There is no new user storage schema migration; isolated tests use unique databases deleted afterward. Main integration must preserve existing storage namespaces for real saves, using the packaging namespace substitution in `slice-control.mjs`.

## Reproducible checks

Mandatory engine runner adds native `run_slice_bridge.sh`, Node `slice_control_test`, `slice_cc_router_test`, `slice_ui_test`, actual `slice_cc_wasm_test`, actual dashboard HTTP `slice_http_test.py`, and `slice_cc_coverage_test.py`. Missing new HOST exports fail the actual-WASM check. Controlled DOM/C/storage tests remain labeled separately from actual C/WASM and browser evidence.

Fresh isolated WASM684733bytes SHA `b90be94362a3985eeb7857d35e7e5963a3b63e5308aeeb27bd15480923df76c9` passed actual gain PCM, original repeat boundary, audition, sync musical bar, exact21bit/native-context retention and pending/playing/owner guard checks. Existing native CC75-case actual-WASM regression also passed. Production-C HOST bridge, service/router tests, production-module DOM lifecycle and dashboard Handler HTTP module/MIME/private-path rejection passed. Windows sandbox loopback initially returned Win10013; offline-container loopback passed without a runtime change.

Root ran `tests/host/slice_browser_test.html` on port8123: all13 paths, actual producer/renderer PCM/sync, genuine unique IndexedDB commit/reopen/aborted transaction/prior-commit preservation, missing-file retention, ownership and explicit disconnect passed; console warnings/errors were empty. Evidence: main workspace `dashboard/data/slice-cc-browser-idb-20261014.png` and `.txt`. The harness manually advances a production worklet with a synthetic explicit input port; it uses no real AudioContext or device permission.

To repeat independently, serve the isolated candidate with `python tests/slice_browser_server.py 8123` and open `/tests/host/slice_browser_test.html`. Its allowlist exposes only the harness, required production modules and WASM. A subsequent packaged-UI check should prepare a synthetic demo while powered off, power on and confirm audition buttons become ready; confirm owner/job disabling and retained-model readiness after failed reload; audition/Play/Stop; save/reopen; inspect exported JSON and treat download request separately from file delivery.

Before main integration: apply only reviewed narrow hunks preserving newer generator/EXP/controller changes; verify generated native header/wire mappings unchanged; regenerate coverage/source fingerprints; package both modules, namespace storage and retain converter handoff; serve through dashboard allowlist; rerun mandatory checks on main's fresh WASM; repeat browser storage/UI acceptance. Original target gates remain unchanged, and full hardware/DAW/all-parameter acceptance stays open.

## Later real-audio and packaged-UI evidence — 2026-10-14

The generated browser overlay records the initial synthetic candidate checkpoint: its `real_audio_context:false` is not the latest observed proof. The later frozen production graph test at source43a7c0c76d6343ceba769d4ebbe66d92cd14f3d02c0fdd355fb30103da55499e / WASM e8d5dc0adef7c2f28eca7b7ee5f457ae0d02dee54f1504dfc8c12f2d760c6854 passed all13controls with an actual44100Hz AudioContext and AudioWorkletNode. Four settled analyser windows each measured RMS0.25/0.125/0 at100/50/0percent gain; ratio0.5. Output was muted. Genuine disposable IndexedDB, explicit Stop/context/database cleanup and route/gesture guards passed without console warnings/errors. This supplements, rather than replaces, the mandatory synthetic regression evidence. Human hearing/click diagnosis, real DAW/controller, physical USB/TRS/receiver and download delivery remain unaccepted.

Actual packaged UI also passed public-demo load, zero-level original Play/Stop and saved import reopen. File-chooser import, sync with instrument GUI and actual download delivery remain separate checks. Per-drum lane editing separately retained909CH7/808BD13 through save/reload. A generic Python server on Windows served MJS as plain text and failed worklet startup; the reviewed `tools/serve_alpha.py` now verifies/snapshots the release manifest and exposes only allowlisted bytes with explicit module MIME, loopback/Host and path guards. Six real HTTP regressions pass on Windows and in the pinned offline container. A fresh origin8132 starts the actual packaged engine correctly; earlier failure evidence is preserved.

Raw REX conversion remains a separate local Windows tool. The preview now explains the matching WAV/JSON handoff and links to an illustrated-workflow FAQ section, using project-subpath-safe links. The public package still excludes decoder files, raw/private loops and converter APIs. No new Pages deployment is accepted.
