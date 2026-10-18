# Official upstream integration - local qualification

User approved this integration in chat on 2026-10-17. Upstream: Charles Vestal's fm1-x0x main at 2e31c517ded3732c58b845eb0c800fe018004b8d (33 commits after our common baseline47f8e4c). Pre-integration Extended Edition source is preserved as local commit169a753, in addition to the untouched publication-candidate directory. This branch is local; publication and device installation remain held.

## Included

- Previous A/B stream-copy recovery after an interrupted save, adapted to the existing atomic staging loader and recognized variant formats. Lane streams must also match an intact current or previous sound object. Unknown/future formats and unrecoverable corruption remain protected, rather than silently replaced or resaved.
- Safe-mode OCT-/OCT+ update entry, one-core recovery/setting, retained crash diagnostics, and upstream stock-level supply initialization. These hardware changes are source/target-build qualified only, not observed on this unit.
- Upstream G.711 mu-law 909 cymbals. This replaces our existing lossless packing and deliberately changes cymbal samples. Independent original-PCM error measurements exceed35dB SNR for all three assets; codec-matched frozen DSP oracles remain separate. Old-format differential fixtures retain all voices with explicitly codec-matched input assets; pinned source code stays untouched. Actual production host/WASM parity and independent original-PCM codec/accessor checks cover the new samples. Listening is still required.
- USB project backup/restore, including the current variant data in its project stream. The separate reusable strip-default bank is retained on the device, like panel calibration; it is not included in a project backup. User loops also remain outside these files. Restore retains the physical confirmation, guards pending transport, validates loading, and reports rejected loads instead of claiming success. Settings-only core recovery also preserves rejected/future projects. All project-save paths honor a streaming backup's save hold.
- Screen-reader announcements, keyboard/site accessibility and narrow-screen controls. The portable emulator embeds its words dictionary and can fall back to the browser's voice without the optional upstream reader voice assets. Physical screen-reader listening/browser speech acceptance remains pending.
- ENV+knob factory/default shortcut, explicitly excluded from Save/Apply action controls. Our saved-default semantics, command48 read-only query, bounded native messages, defined fastmath and Extended Edition intro are retained.

## Deliberately deferred

Upstream commit0417760's one-knob BREAK filter and its sound-revision5 migration. It allocates sound byte578 (T_BRK/voice0/parameter2), which is already part of our frozen LAN1/LAN2 timing slab. Porting it unchanged corrupts existing independent-lane projects. Our existing BREAK strip HP/LP controls remain available. The upstream scenario is retained as a deferred reference, not counted as a passing feature test. A future port needs explicit storage/schema migration and independent compatibility fixtures.

## Resource tradeoff

The expanded SysEx output queue uses pool RAM. To preserve the unchanged8192-byte pool reserve, undo payload capacity is18KiB rather than20KiB; the journal still drops oldest history first. This does not reduce pattern, lane, arrangement or saved-default extents. No original resource limits or target-budget baselines were relaxed.

Local qualification version:0.1.3-beta1 / FM-1_9000103. Target image491264/581564 bytes; ordinary RAM98276/98304 bytes; pool335708/344064 bytes (8356 free,164 above the8192 reserve). Original15 static processing gates and matching27-function caller/helper gates pass. This is88112 bytes below the installed579376-byte image. Static measurements do not establish audio deadlines, stack watermark or thermal/rail acceptance.

## Physical acceptance still open

No upstream build has been installed. The unit remains on0.1.2-beta1, pinned package529fb5c3... . The existing303B LP recall is unconfirmed (10240 ->9813, reported Apply still read9813); no default resave was instructed. New cymbal listening, core recovery, rails, update/backup behavior and normal device/DAW/soak checks remain separate from local automated tests.

## Evidence

Mandatory full suite: build/acultur8-checks.json and final qualification log. New native integrated tests cover torn saves, variant restore, independent saved-bank retention, guarded ENV actions and read-only announcements under ASan/UBSan; upstream backup tests cut every read and each of54 flash writes in a simulated NOR. Actual worklet/WASM tests exercise screen-reader HELLO/WHERE/SAY/BYE without storage changes. Target evidence is built independently in the sibling upstream-target-20261017 directory. Interim failing logs remain historical, not acceptance evidence.
