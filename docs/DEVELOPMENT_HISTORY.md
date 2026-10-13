# Acultur8 development history

## 2026-10-13 — Authorized implementation begins

The public fork `keyboy250/fm1-x0x-acultur8` and integration branch `acultur8/main` were created. The preserved upstream commit is `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`; all 230 baseline files match the supplied source archive. PR #1 imports five modified files and eleven additions for browser slice audition and tempo sync.

Portable lab, consolidated regression checks and the public static package are in review. Original workstation source, dashboard and imports remain unchanged except for synchronized progress documentation. Only synthetic loop audio is included in the public package.

An official pinned target build succeeded, with 120 image bytes and 1,088 general RAM bytes spare. This is a resource constraint for upcoming features. Correct instruction decoding and cost-coverage gates are under review. Physical CPU timing, recovery, audio/MIDI and flashing acceptance remain pending.

Previous accepted scope: shared-engine browser emulator, exact WAV/marker import, original REX timing conversion in a separate private tool, pitch-preserving internal-clock slice sync, and illustrated FAQ. These existing results do not establish hosted or hardware acceptance.

## 2026-10-13 — Browser alpha.1 published; local candidates preserved

Reviewed PRs #1–#3 merged into `acultur8/main`. [Alpha.1](https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/acultur8-v0.1.0-alpha.1) identifies `0487778bf76915c2312644791d92d30378f4ecb5`. Mandatory [CI](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38361493563) passed. All 21 hosted files matched build hashes; hosted audio, synthetic six-slice import, 90/120/180 BPM sync, stop/restart and save/reload/reopen passed without console warnings/errors. Portable lab clean-clone launch without an SDK passed.

Unmerged candidates: lossless font compression (18,368 image bytes saved; 439 glyphs and 30 screenshots exact), bounded command/save/undo foundation (focused checks pass, including final long-gesture fixes), and standalone sound-strip DSP (host tests and official target object compilation pass). Final integrated checks, visible controls and kick integration remain open. Physical-device tests remain pending. Standing authorization is retained; the app goal is paused until user resume. Original files and private runtime/SDK material remain preserved.

## 2026-10-13 — About page published

[PR #4](https://github.com/keyboy250/fm1-x0x-acultur8/pull/4) merged as `84d924660522f275baff6c3cd1741479bbe898b1`. The [About page](https://keyboy250.github.io/fm1-x0x-acultur8/about/) links to Acultur8.com and the verified Acultur8 Traxsource label URL. Frozen source checks and independent review passed; merged CI and Pages deployment passed. Hosted page and links verified. About navigation is injected only into the public static emulator, preserving local lab routing. The main Acultur8 website launch package is separate and awaits hosting access.

## 2026-10-13 — Remaining-feature execution plan refreshed

The plan now starts from the published alpha and merged About page. Priorities: lossless fonts and combined resource sizing; command/save integration; one complete kick; all sounds; arrangement; progression; both basses; external MIDI; whole-scene browser acceptance. Proposed branch dependencies replace obsolete fixed PR numbers. Forecasts remain conditional on resource and acceptance results; hardware and decoder distribution stay pending. This is a planning update, with no additional target marked complete or app goal resumed.

## 2026-10-13 — Targets and FAQs synchronized

Open targets now follow the expedited ten-priority sequence. Next checks cover full resource sizing, acknowledged editing/persistence, the complete kick and whole-scene acceptance. Local/public FAQ source records published alpha/About work, parallel delivery, conditional estimates and the unchanged RAM constraint. All target statuses are preserved; no unfinished feature or physical test was marked accepted. A recovery archive precedes the documentation changes.


## 2026-10-13 — Active implementation: fonts and shared foundation merged

The user resumed the autonomous implementation goal. Reviewed [PR #6](https://github.com/keyboy250/fm1-x0x-acultur8/pull/6) merged lossless fonts as `8bd6ebd18ca9b5a59b8207efa93498dda9e89506`; all 439 glyphs and 1,756 rendering cases match. Reviewed [PR #7](https://github.com/keyboy250/fm1-x0x-acultur8/pull/7) merged command/save and standalone DSP foundation as `0f3d1d53c91a4f2344bf076f254cea1f801eb649`. Required [CI](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38370228694) and [Pages deployment](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38371087924) passed. This supersedes the earlier paused/unmerged snapshot.

The kick integration candidate passes required host/WASM/browser-package regressions and real worklet audible response, limiter/meter, save/reopen, undo/redo and unaffected-snare checks. An initial inlined active path failed the unchanged static cost gate; separating that measured active path restored all original 15 gates without modifying the baseline. Integrated target image is 570,020/581,564 bytes, general RAM 97,636/98,304, pool 334,612/344,064 (1,260 bytes above the 8,192 reserve). Additional active callee and per-sample costs are documented separately. Browser-control and hosted acceptance continue; no physical-device acceptance is claimed.


## 2026-10-13 — Kick browser checkpoint published and accepted

Reviewed [PR #8](https://github.com/keyboy250/fm1-x0x-acultur8/pull/8) merged as `9ab26cd78e3331ee3df1c115be8ba6ffdcd7d725`; final-head CI and merged [CI/Pages](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38373885337) passed. Hosted stage/filter/numeric editing, undo/redo, saved reload, busy-save guard, six-slice shared transport and 390px layout passed without console warnings/errors. All 24 hosted files match their manifest, and hosted WASM exactly matches local `fd8f1221180df959e9e979f0dd4533adadbf52f3d52de45fec468bebad15c6a1`. FM1-27 is accepted within its browser-prototype scope; physical device timing/audio remain pending.

The local dashboard on port 8098 edits the reviewed variant and preserves original dashboard/source files and private converter/runtime data. Its iframe editor edits and Save were verified; 12 disposable dashboard HTTP tests pass. A Windows/Linux source fingerprint difference was traced to path sorting and unchanged text line endings; exact canonical committed bytes match the hosted fingerprint. The follow-up enforces LF text checkout and explicit case-sensitive relative-path ordering, with a cross-platform regression. An isolated chord resolver candidate now passes native/sanitizer checks; progression/bass/MIDI integration remains open.


## 2026-10-13 — Shared harmony resolver dependency verified

A separate bounded C resolver provides I–VII triads in all seven church modes and all twelve keys, explicit pentatonic parent selection, ascending MIDI voicings and strict final root/chord-tone selection after transpose/range correction. Independent review and repeated strict native plus ASan/UBSan runs pass 18,299,366 assertions. The required host/WASM/browser suite remains green. Host object text is 1036 bytes with zero data/bss; this is not target/device cost evidence. No legacy sequencer/generator/save ABI changed. Progression sequencing, saved blocks, bass/pad events and browser controls remain pending; FM1-14 stays in progress.
