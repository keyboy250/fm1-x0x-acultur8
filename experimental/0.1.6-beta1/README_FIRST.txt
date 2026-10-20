ACULTUR8 X0X EXTENDED EDITION 0.1.6-beta1 - EXPERIMENTAL PRERELEASE
Prepared 20 October 2026. Expected device identity FM-1_9000106.
Release: https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/v0.1.6-beta1
Website: https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.6-beta1/

FIRMWARE: independent block variations, block deletion/Undo/Redo, visible SONG
UNDO, 1-32-bar arrangement blocks and scale-safe 303 transposition. The firmware
is byte-identical to the installed, tested package; no rebuild or new flash.

BROWSER COMPANION: Slice Profiles editor with frequency-guided detection, spectrum,
RMS/dBFS gate, per-slice Active/Length/Hold/Decay/Level, copy/paste/reset and review
project saving. Accepted by the user for a representative snare-extraction workflow.
It is not integrated into the FM1's native sampler or the existing emulator.

Read docs/FAQ.html, KNOWN_ISSUES.txt, INSTALL_AND_ROLLBACK.txt and TEST_EXPECTATIONS.txt.
The FAQ retains the illustrated guide and adds current controls and sampler help.
The source ZIP matches this firmware, including reviewed local changes to SONG UNDO.
Rollback firmware and corresponding source are in rollback/.

No private projects, recordings, flash dumps, device calibration or credentials
are included. The only bundled WAV is a deterministic generated synthetic demo.
Separate autosave candidate, 128-step lanes, new synth engines, slow LFOs,
probability arrangements and velocity-shaped sampler filters are not included.

Run the included browser tool from the extracted folder with:
  python -m http.server 8144 --bind 127.0.0.1
Then open http://127.0.0.1:8144/ for the release overview and browser companion.
