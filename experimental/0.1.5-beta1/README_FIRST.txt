X0X Extended Edition 0.1.5-beta1 — EXPERIMENTAL FM-1 firmware
USB identity: FM-1_9000105
Published package prepared 19 October 2026 UTC.

START HERE
1. Read KNOWN_ISSUES.txt and INSTALL_AND_ROLLBACK.txt.
2. Back up projects, private samples and reusable strip defaults separately.
3. Verify the firmware against SHA256SUMS.txt before installing.
4. Use the included fm1_install.py; installation requires Python 3, mido and
   python-rtmidi. This is a command-line installer, not a one-click application.

The .fwsc file is firmware, not an application to double-click. The complete
release ZIP includes that firmware, installer script, matching source ZIP,
offline searchable illustrated FAQ, warnings and comparison rollback files.
Extract x0x-0.1.5-beta1-source.zip to inspect/build the complete matching source.

This is an experimental prerelease for testing and feedback. Heavy combinations
of enabled strips/effects may glitch. Disable AUTOSAVE while stopped before
external MIDI performance, then save manually between takes. Unsaved edits can
be lost on power-off. MIDI timing variation remains under investigation.

The firmware is byte-identical to the 0.1.5-beta1 build installed and tested on
one FM-1. Testing is bounded and does not qualify every combination or session.
The isolated 909 block-processing optimization preserves the existing strip DSP.
No new routing switch, MIDI importer, note-remapping UI or on-device About FAQ
is included. See the FAQ for current features versus proposals.

Release: https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/v0.1.5-beta1
Website: https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.5-beta1/
Offline FAQ: docs/FAQ.html (open in a browser; no network needed for search).
The separately hosted browser alpha is an earlier build.

GPL-3.0-only and component notices are retained in the source archive.
No private projects, samples, recordings, calibration or credentials are included.
