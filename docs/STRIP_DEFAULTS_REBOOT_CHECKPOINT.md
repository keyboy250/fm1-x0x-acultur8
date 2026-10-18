# Current resumed defaults qualification - 2026-10-16

2026-10-16 resumed qualification: focused native all25 defaults, Stop/Undo atomicity, actual WASM/worklet, UI failure/retry and legacy host-store compatibility pass. Isolated browser8135 confirms BD/CH independent defaults, Undo/Redo, reload durability and no automatic project replacement. Frozen target passes unchanged15 gates and supplemental helper ledger: image578000, RAM98300, pool335772. Pinned HH/ride PCM and sanitizer checks pass. Complete mandatory suite terminal PASS9/9, including31 host scenarios and packaged parser/WASM checks; no integration or accepted-preview promotion. Physical NOR/power-loss/menu/IRQ/stack/listening/DAW/soak pending. Pages held.

Read docs/STRIP_DEFAULTS_POSTREBOOT_QUALIFICATION.md in the isolated candidate. Older pause and pre-packing entries below are preserved history, superseded by this section.

# Reboot pause - 2026-10-16

Paused at the user's request. Work is saved locally; no new defaults qualification or publication is claimed. Candidate: build-dependencies/strip-defaults-integration-20261014, branch feat/sound-strip-user-defaults, base e227ad728f40a2128ef892f6d0d171aff30e3413. Recoverable tracked diff and untracked source copies: build/resume-defaults-20261016/reboot-final.patch and untracked-final/.

Per-sound defaults remain isolated and incomplete. Oversized-bank rejection and Stop/undo atomicity repairs have focused native regression evidence in build/defaults-bank-review-20261016. Full suite was deliberately interrupted; do not promote its running/interrupted report to PASS. Legacy seven-record host-store compatibility repair and regression are saved but not yet qualified. The attempted compatibility RED command failed at shell redirection before Docker ran; rerun with candidate cwd.

Resume order: run legacy host-store compatibility RED against prior executable; rebuild and qualify current host/WASM, focused defaults and full mandatory suite; build a fresh immutable target and check unchanged original resource gates plus helper ledger; perform isolated browser durability/Undo/isolation acceptance. Update target status only after acceptance. Preserve accepted rate preview build25641496 and browser saves. Pages held; physical testing/flashing pending.

Next two working days: first close saved-default persistence, transaction and compatibility checks; then target/resource and isolated browser acceptance, independent review and selective integration if every required gate passes. Preserve failures and original files; no timing promise or automatic publication.

FAQ clarification: Save as sound default and Apply saved default are explicit stopped operations per sound. They must not silently replace current project settings. New defaults are still under qualification and have not replaced the accepted preview. Local servers stop at reboot and need restarting from the preserved handoff.

# Sound-strip defaults: shutdown checkpoint

Status: paused at the user's request to shut down the computer. Do not build, integrate, push, merge, deploy Pages or flash until the user resumes. Pages remains held. The new defaults candidate is NOT qualified or integrated.

## Accepted baseline

Canonical workspace: `m-vave mods/build-dependencies/lane-cc-integration-20261013`.
Expanded rates are reviewed and locally integrated from `e227ad728f40a2128ef892f6d0d171aff30e3413`, with mandatory9/9, original15 target gates and browser acceptance PASS. The frozen preview is `build/expanded-rates-reviewed-integration-20261014/site`, served at `http://127.0.0.1:8133/emu/`. Badge: `v0.1.0-alpha build25641496`; source256414965c6330524392c504498583d5040a2539525b5a807e778fd148e55eb5; WASM240984c163aee9003f90be449cbb5bbec9c074ee01656fe4880b7a45ab6f0173. The preview is switched off. Existing browser project/import storage remains intact. Defaults changes have not been copied into this accepted runtime or preview.

## User decisions and isolated candidate

User explicitly requested editable, saved defaults for high-pass, low-pass and brick-wall limiter settings. Clarifications: separate defaults for EACH SOUND, and persistence in BOTH browser and FM1 hardware.

Candidate: `m-vave mods/build-dependencies/strip-defaults-integration-20261014`, branch `feat/sound-strip-user-defaults`, committed base `e227ad728f40a2128ef892f6d0d171aff30e3413`, current work uncommitted. `build/canonical-baseline.json` records pre-change canonical hashes; inherited user document edits are preserved. Do not stage/integrate every dirty file blindly.

Implemented draft:
- Shared firmware defaults bank: new object7/OBJ_STRIP_DEFAULTS,212bytes, magicSDF1/schema1,25 independent configs and presence mask. Existing project format/schema stay unchanged. No resident bank; stopped main operations reuse existing project I/O buffer. Project hash caches remain sized to original seven project objects, avoiding new target RAM there.
- Native A/B bank sectors: A0x9F000 and B0xFE000. Sample slots span0xA0000..0xDC000, so0xDA000 is NOT free. Existing calibration0xDC000/0xDD000, motion0xDE000/0xDF000, settings0xFC000/0xFD000 and OTA regions are preserved. Missing-bank versus damaged header distinction is bank-specific.
- Commands20 Save default and21 Apply default; capability2048. Save uses current canonical strip, validates full bank and readback, and avoids identical flash rewrites. Recall is explicit, stopped, reserves transport and publishes all six scalar fields atomically under the existing exact ICFG guard. One undo gesture; other sounds unchanged. No automatic replacement of loaded projects.
- Native EDIT pages appended for FILTERS, LIMITER and DEFAULTS; selected individual voice. Frequency pots use12 subdivisions per octave, exact stored Hz remains visible; release10..250ms.
- Browser editable release and removal of forced80ms on unrelated gestures. Save as sound default / Apply saved default. Durable browser storage event distinguishes localStorage success/failure. Successful native no-op default saves force bank replay for browser retry. Existing MIDI map stays699rows.

## Evidence and outstanding qualification

Pre-ride focused native/WASM/UI suite PASS in `build/strip-defaults-focused-third.log`:25 independent defaults, reboot and project reopen, selected-only recall, undo/redo, no-op flash saves, I/O/future-format/protected/transport/open-gesture/full-queue refusals and native page exposure. The UI test currently covers release bounds and preserving release, not yet all defaults-button/durability flows.

First frozen target `build/strip-defaults-target1` FAILED flash fit by1908bytes. Log: `build/strip-defaults-target-first.log`. Preserve this evidence; no limit/baseline relaxation.

To recover space, latest draft extends exact PCM packing to the first32768 ride frames, keeping the remaining tail raw. Prefix theoretical saving5619bytes. Shared inline decoder handles HH and ride; raw-oracle macro X0X_RAW_HH remains supported and also exposes raw ride. Updated `tests/pcm_blocks_test.py` adds actual ride accessor versus pinned original sample with ASan/UBSan. These NEW ride checks and the rebuilt target have NOT run yet. No sound-parity or target-fit claim for this latest draft.

Latest host/WASM compile PASS in `build/strip-defaults-packed-ride-build.log`, manifest `build/emu/manifest.json`: sourcef7df2dda940ba2f7f89c16bcfb2e91cb5df74a8afefe651c488379d579006c9a; WASM520a9394f1bed8cc55152f9ce75b1f0dc7d33adcf258be3bfdf0610277732a02;682978bytes. This is compiled, NOT qualified. First build had the native setter accidentally inserted into pref_get; corrected before the passing focused suite. Shell runner CRLF and missing POSIX compile flag were corrected; failures retained.

Resume sequence:
1. Verify shutdown snapshot hashes, canonical baseline and latest manifest. Keep the accepted8133 preview on build25641496 during qualification.
2. Run focused defaults suite against latest packed-ride build; run exact PCM sanitizer/reference comparisons and native NOR-bank empty/damaged/torn-write/non-overlap checks. Extend frontend defaults tests for save/apply, selection isolation, protected/busy refusal, storage failure and identical-save retry. Review native action and browser durable-ACK paths.
3. Freeze latest runtime to a NEW target directory; build with pinned offline toolchain/SDK. Pass image581564, ordinaryRAM98304, pool reserve8192 and ALL original15 target gates, unchanged clock family and direct-call/tail-call closure. Do not reuse target1 as evidence for the modified ride source.
4. Run complete `python tools/acultur8_check.py` with the new mandatory strip_defaults group. Fix failures, keeping pinned raw PCM comparisons intact. Physical hardware/MIDI/stack/CPU/listening/soak remain separate.
5. Actual browser QA on a separate origin8134: edit/save distinct BD and CH defaults, alter current values, apply/reopen, release persistence, project save/undo and failed-storage reporting; capture proof. Do not reset user8133 saves or request MIDI permissions for simulated acceptance.
6. Only after all checks and review pass: recoverable local commit, selective integration with canonical-baseline preflight, append history/targets/FAQs/checkpoint, verify artifact hashes, then update same-origin8133 preview while off. Keep Pages held and preserve older artifacts.

## Restore accepted preview after reboot

Servers will stop when the computer shuts down. Reopen the existing browser origin to preserve browser saves. In PowerShell, run the following only after user resume, and only if port8133 is free:

```powershell
# Run from your repository checkout.
Start-Process -FilePath 'python' -ArgumentList @('tools/serve_alpha.py','--site','build/expanded-rates-reviewed-integration-20261014/site','--port','8133') -WorkingDirectory $PWD -WindowStyle Hidden
```

Verify the HTTP release manifest and badgebuild25641496. Do not serve the unqualified defaults candidate as the user preview.
