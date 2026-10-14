# Acultur8 development history

## 2026-10-13 â€” Authorized implementation begins

The public fork `keyboy250/fm1-x0x-acultur8` and integration branch `acultur8/main` were created. The preserved upstream commit is `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`; all 230 baseline files match the supplied source archive. PR #1 imports five modified files and eleven additions for browser slice audition and tempo sync.

Portable lab, consolidated regression checks and the public static package are in review. Original workstation source, dashboard and imports remain unchanged except for synchronized progress documentation. Only synthetic loop audio is included in the public package.

An official pinned target build succeeded, with 120 image bytes and 1,088 general RAM bytes spare. This is a resource constraint for upcoming features. Correct instruction decoding and cost-coverage gates are under review. Physical CPU timing, recovery, audio/MIDI and flashing acceptance remain pending.

Previous accepted scope: shared-engine browser emulator, exact WAV/marker import, original REX timing conversion in a separate private tool, pitch-preserving internal-clock slice sync, and illustrated FAQ. These existing results do not establish hosted or hardware acceptance.

## 2026-10-13 â€” Browser alpha.1 published; local candidates preserved

Reviewed PRs #1â€“#3 merged into `acultur8/main`. [Alpha.1](https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/acultur8-v0.1.0-alpha.1) identifies `0487778bf76915c2312644791d92d30378f4ecb5`. Mandatory [CI](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38361493563) passed. All 21 hosted files matched build hashes; hosted audio, synthetic six-slice import, 90/120/180 BPM sync, stop/restart and save/reload/reopen passed without console warnings/errors. Portable lab clean-clone launch without an SDK passed.

Unmerged candidates: lossless font compression (18,368 image bytes saved; 439 glyphs and 30 screenshots exact), bounded command/save/undo foundation (focused checks pass, including final long-gesture fixes), and standalone sound-strip DSP (host tests and official target object compilation pass). Final integrated checks, visible controls and kick integration remain open. Physical-device tests remain pending. Standing authorization is retained; the app goal is paused until user resume. Original files and private runtime/SDK material remain preserved.

## 2026-10-13 â€” About page published

[PR #4](https://github.com/keyboy250/fm1-x0x-acultur8/pull/4) merged as `84d924660522f275baff6c3cd1741479bbe898b1`. The [About page](https://keyboy250.github.io/fm1-x0x-acultur8/about/) links to Acultur8.com and the verified Acultur8 Traxsource label URL. Frozen source checks and independent review passed; merged CI and Pages deployment passed. Hosted page and links verified. About navigation is injected only into the public static emulator, preserving local lab routing. The main Acultur8 website launch package is separate and awaits hosting access.

## 2026-10-13 â€” Remaining-feature execution plan refreshed

The plan now starts from the published alpha and merged About page. Priorities: lossless fonts and combined resource sizing; command/save integration; one complete kick; all sounds; arrangement; progression; both basses; external MIDI; whole-scene browser acceptance. Proposed branch dependencies replace obsolete fixed PR numbers. Forecasts remain conditional on resource and acceptance results; hardware and decoder distribution stay pending. This is a planning update, with no additional target marked complete or app goal resumed.

## 2026-10-13 â€” Targets and FAQs synchronized

Open targets now follow the expedited ten-priority sequence. Next checks cover full resource sizing, acknowledged editing/persistence, the complete kick and whole-scene acceptance. Local/public FAQ source records published alpha/About work, parallel delivery, conditional estimates and the unchanged RAM constraint. All target statuses are preserved; no unfinished feature or physical test was marked accepted. A recovery archive precedes the documentation changes.


## 2026-10-13 â€” Active implementation: fonts and shared foundation merged

The user resumed the autonomous implementation goal. Reviewed [PR #6](https://github.com/keyboy250/fm1-x0x-acultur8/pull/6) merged lossless fonts as `8bd6ebd18ca9b5a59b8207efa93498dda9e89506`; all 439 glyphs and 1,756 rendering cases match. Reviewed [PR #7](https://github.com/keyboy250/fm1-x0x-acultur8/pull/7) merged command/save and standalone DSP foundation as `0f3d1d53c91a4f2344bf076f254cea1f801eb649`. Required [CI](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38370228694) and [Pages deployment](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38371087924) passed. This supersedes the earlier paused/unmerged snapshot.

The kick integration candidate passes required host/WASM/browser-package regressions and real worklet audible response, limiter/meter, save/reopen, undo/redo and unaffected-snare checks. An initial inlined active path failed the unchanged static cost gate; separating that measured active path restored all original 15 gates without modifying the baseline. Integrated target image is 570,020/581,564 bytes, general RAM 97,636/98,304, pool 334,612/344,064 (1,260 bytes above the 8,192 reserve). Additional active callee and per-sample costs are documented separately. Browser-control and hosted acceptance continue; no physical-device acceptance is claimed.


## 2026-10-13 â€” Kick browser checkpoint published and accepted

Reviewed [PR #8](https://github.com/keyboy250/fm1-x0x-acultur8/pull/8) merged as `9ab26cd78e3331ee3df1c115be8ba6ffdcd7d725`; final-head CI and merged [CI/Pages](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38373885337) passed. Hosted stage/filter/numeric editing, undo/redo, saved reload, busy-save guard, six-slice shared transport and 390px layout passed without console warnings/errors. All 24 hosted files match their manifest, and hosted WASM exactly matches local `fd8f1221180df959e9e979f0dd4533adadbf52f3d52de45fec468bebad15c6a1`. FM1-27 is accepted within its browser-prototype scope; physical device timing/audio remain pending.

The local dashboard on port 8098 edits the reviewed variant and preserves original dashboard/source files and private converter/runtime data. Its iframe editor edits and Save were verified; 12 disposable dashboard HTTP tests pass. A Windows/Linux source fingerprint difference was traced to path sorting and unchanged text line endings; exact canonical committed bytes match the hosted fingerprint. The follow-up enforces LF text checkout and explicit case-sensitive relative-path ordering, with a cross-platform regression. An isolated chord resolver candidate now passes native/sanitizer checks; progression/bass/MIDI integration remains open.


## 2026-10-13 â€” Shared harmony resolver dependency verified

A separate bounded C resolver provides Iâ€“VII triads in all seven church modes and all twelve keys, explicit pentatonic parent selection, ascending MIDI voicings and strict final root/chord-tone selection after transpose/range correction. Independent review and repeated strict native plus ASan/UBSan runs pass 18,299,366 assertions. The required host/WASM/browser suite remains green. Host object text is 1036 bytes with zero data/bss; this is not target/device cost evidence. No legacy sequencer/generator/save ABI changed. Progression sequencing, saved blocks, bass/pad events and browser controls remain pending; FM1-14 stays in progress.


The accepted first-kick browser prototype is tagged [acultur8-v0.1.0-alpha.kick.1](https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/acultur8-v0.1.0-alpha.kick.1) at `1394e5ff448106b358759ad46280d0113fa4ce87`. Its merged CI/Pages run `38375752238` passed; all 24 hosted files, exact WASM and the portable local/hosted source fingerprint match. This tag contains no hardware installer. Original alpha.1 remains unchanged.


## 2026-10-13: resource saving and basic arrangement integration

Reviewed PR #11 merged at `d7c63f5ecee490e275d377fd68005d3328ddcb24`; required CI and Pages passed and 24 hosted hashes match. Noise history remains 256 samples; exact pool saving 1,024 bytes and 33,600 bitwise comparison blocks pass.

Basic arrangement adds 192 paged rows, 1â€“8 musical-bar durations, next-bar queue and shared C triad metadata. Independent review closed the IRQ-start race through bounded locks and nested long-operation reservations. Combined official target: image 576180/581564, RAM 97860/98304, pool 333792/344064; 15 original static gates pass unchanged. New active callee costs are recorded in `lab/ARRANGEMENT.md`. Final integrated browser/hosted checks are pending before this candidate is promoted. Broader row-order/loop controls, all 25 processing paths, both bass followers, MIDI accompaniment and physical qualification remain pending.


## 2026-10-13 â€” All-25 sound-strip focused integration and lossless fonts

All 25 strips now cover the eleven sounds in each 909/808 engine, 303 A/B and the instrument mono BREAK path with HPF, LPF, boost and a sample-peak ceiling. Focused native and actual WASM checks plus independent source review pass. The ceiling has no lookahead or true-peak claim; the separate Marked-WAV preview lane retains its existing workflow. The complete fresh mandatory suite and local browser GUI acceptance pass; hosted publication acceptance remains pending, so FM1-11/FM1-12 are in progress.

The combined official target passes all 15 original gates unchanged: image 579636/581564 bytes, general RAM 97860/98304, pool 335712/344064. Pool free space is 8352 bytes against the unchanged 8192-byte reserve, leaving 160 bytes above reserve. New helper instruction/call/stack costs are recorded separately in `tests/all_sound_feature_costs.json` and caller costs in `tests/all_sound_target_costs.json`; static estimates do not establish cycles, stack watermark or physical deadline margin.

Lossless Huffman packing preserves all 439 glyphs and 1756 production render cases, including independent pinned-original pixel parity. Payload plus tables saves 6773 bytes before incremental decoder code. Native focused draw is approximately 49.6% slower than the prior packet format; physical draw and audio timing remain pending. See `docs/ACULTUR8_FONT_PACKING.md` and `tests/font_huffman_evidence.json`.

Public basic arrangement and harmony metadata at `e78ee13c60116e5b352d73b537cdf1f0980d9d8e` remain accepted. Broader insertion/order/loop controls and actual bass-follow, MIDI and pad behavior remain pending. No all-sound target or device acceptance is promoted by these focused results.

## 2026-10-13 â€” Complete all-sound local acceptance

Fresh mandatory host/WASM/font/parser/worklet/dashboard/site checks and all 31 host scenarios pass. Actual browser checks cover all 25 options, isolated 808/BREAK edits, draft selection lock, undo/redo, saved reopen, four-block arrangement with synchronized synthetic slices, live-save rejection and stopped save. Final status presentation clears after the selected C snapshot. At 390px the document is 375px and editor 343px wide. An isolated older accepted mask-1 WASM disables 24 options while retaining usable kick controls without console errors. Hosted publication remains pending. Static caller costs and additional address-qualified helpers/frames are recorded in `tests/all_sound_target_costs.json` and `tests/all_sound_feature_costs.json`; these do not establish full worst-case stack or physical cycles.

##2026-10-14: qualified combined development checkpoint and GitHub update authorisation

The user explicitly authorised keeping GitHub current. This source checkpoint includes reviewed25individual lanes, partial native/livebrowserCC, protectedhistory, canonical setters, MASTERownership/pickup/readback and losslessD8/D9/EXP metadata. The frozen runtime source7124f8a9f2e5060e3cb0c9ac44f831ce9e37ce5d87c3a47dedfadd6c31b548f4 built WASM60c3ab085454ba1d9f392d7ca478fb4787a9a08d1bc0486265c0e05a55a9d842 (677897bytes). Fresh9requiredchecks pass; original15static gates pass with image579172/581564, ordinaryRAM98184/98304,pool335712/344064. Checkpoints retain originalreferencePCM and all2432EXP floatbits. Sourcepassed independentlimitedintegrationreview.

Actualbrowser synthetic909/808/both303/BREAK+6slices ran125/90/180BPM, hat13steps1/32triplet, selectedbass57underAm and50underDm,469slicetriggers,sharedStop/save/reopen/master2800→2864→2800. This is partialscene evidence, nothumanlistening/clickdiagnosis/realreceiver/DAW/30minsoak orhardwarequalification. The earlier published all-sound acceptance receipt is preserved. Browser Pages jobs are explicitly held; newcode doesnotclaim a newhostedrelease.

CompleteCCcoverage stillhas24directgaps (11editingactions and13markedpreviewcontrols). Isolatedediting/undo optimisations andclockStopretry prototypes remain outside this checkpoint. Currentclocktestbench independently confirmed fulloutputqueue canomitStop whilelocalplaybackstops; twoactualWASMinstances ran7260simulatedseconds with348480clocks in87.24wallseconds, not a wall2hour/phasequalification. Performanceclock/setlists/modulecompatibility/multideviceensemble targets remain incomplete. Physicalflashing remains separatelyauthorised.


## Historical published acceptance retained during reconciliation

## 2026-10-13 — All-sound browser publication accepted

PR #13 passed independent review and CI run 38395393049; merged CI/Pages run 38395769684 succeeded. The 26 hosted files match source `10533c9fa817a6df5d64b09afcd47c8f75f14da165f9bf1c5c8df8a73e4b5503` and WASM `e765fd3c65b1ba22f311b425cd5bc964462ee0a783a5222069c2d50a3f78d8a0`. Published releases: alpha.strips.1 at `c081d63` (release 411644605) and alpha.arrangement.1 at `e78ee13` (release 411636789). Earlier pending entries remain historical checkpoints.

Actual hosted checks cover all 25 options and prior-project migration retaining boost 18 and arrangement length 4;808 boost 12/ceiling -18; BREAK 6/undo 0/redo 6 and saved reload; public six-slice sync with120BPM source and FM1 at 125 BPM; PLAY stop, live-save rejection and stopped-save acknowledgement. At 390px, document width 375px and panel 343px pass with zero console errors. Private publication verification preserves detailed proof without publishing runtime data.

FM1-12 is complete within browser functional acceptance after ceiling/tails/meter/save/bypass tests. FM1-11 remains in review for its explicit worst-case CPU/physical criterion; FM1-06 remains in review for physical qualification. The 15 unchanged static gates do not prove worst-case physical CPU. Next work remains actual bass/MIDI behavior, broader arrangement order/loop controls and the complete musical scene.
