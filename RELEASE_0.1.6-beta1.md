# X0X Extended Edition 0.1.6-beta1 — Blocks and Slice Profiles

**Experimental prerelease for testing and feedback.** Build longer arrangements with independent block variations, deletion with Undo/Redo, visible SONG UNDO, **1–32-bar blocks**, and scale-safe 303 transposition.

## Downloads

- **Recommended:** [Complete experimental ZIP](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1-experimental.zip) — firmware, command-line installer, corresponding firmware source, searchable illustrated FAQ, browser companion, checksums and 0.1.5 rollback/source. Extract it and read README_FIRST.txt.
- [Firmware only](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1.fwsc) · [Exact firmware source](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1-source.zip) · [Checksums](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/SHA256SUMS.txt).
- [Browser slice-editor ZIP](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1-slice-editor.zip) · [Offline FAQ bundle](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1-FAQ.zip) · [Promo poster](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.6-beta1/x0x-0.1.6-beta1-poster.png).

Expected device identity: **FM-1_9000106**. Installation uses Python 3, mido and python-rtmidi. The .fwsc is firmware, not a double-click application.

## Browser Slice Profiles companion

Try the [slice editor online](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/browser-slice-editor/): frequency-guided detection, spectrum, band RMS/minimum dBFS, coloured attack matches and per-slice **Active, Length, Hold, Decay and Level**. Copy/paste/reset settings and save/reopen review projects. The original WAV stays unchanged.

**The companion is separate from the existing emulator and the native FM1 sampler.** Emulator integration is deferred for the stable-release work. No native sample upload, USB recording, raw REX decoding or velocity-shaped filter is included. Marker exports contain cuts/timing only; use review projects to preserve envelopes and levels.

## Tested scope and limits

The exact firmware was installed on one FM1. Bounded block copy/delete/Undo/Redo, independent kick editing, save/restart recall, sequential playback and a one-note scale-safe MIDI comparison passed. A 32-bar block measured approximately 64 seconds at 121 BPM and survived restart. Two arrangement cycles were reported clean. The browser snare-extraction/envelope workflow was accepted as a useful experimental starting point.

Heavy-load/effects, broad external MIDI timing and overnight qualification remain open. One pre-existing dropout had unknown cause/time; the sparse playback check added none. **The separate autosave fix is not included.** For external MIDI use the existing AUTOSAVE OFF/manual-save workaround described in the FAQ.

Back up first. Older firmware may reject 9–32-bar or Scale-mode project saves. Keep a pre-update project backup and sample WAVs separately; project backups omit samples, calibration and the separate reusable default bank. The included 0.1.5 rollback is not factory firmware, and downgrade/restore has not been physically exercised for this candidate.

Read the [searchable illustrated FAQ](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/docs/FAQ.html), [controls guide](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/docs/BLOCK_BRIDGE_CONTROLS_REVIEW.html), [installation guidance](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/INSTALL_AND_ROLLBACK.txt) and [beta test expectations](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/TEST_EXPECTATIONS.txt).

GPL-3.0-only and component notices retained. Based on Charles Vestal's X0X and Hügelton Instruments' Felucca. Font Awesome Free SVGs use CC BY 4.0. Only generated demo audio is bundled; no private projects, recordings, calibration or restricted SDK is distributed.
