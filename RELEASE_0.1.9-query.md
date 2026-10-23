# X0X Extended Edition 0.1.9-query — Copy sequences and inspect without saving

Experimental prerelease of the exact FM-1_9000109 build tested on one FM1.

- Nested COPY/PASTE SEQUENCES menu above CLOCK OUT: ALL PARTS, every 909/808 drum row,303 A and 303 B separately—25 choices.
- Preserve destination shared kit accents and swing. Copy bass notes, articulation, timing, generator settings and motion; single-lane edits leave other lanes unchanged.
- Undo/Redo each paste as one transaction.
- New command 49 RAM query and Python snapshot/compare tool inspect unsaved patterns without export/save, with CRC and coherent-scan validation.

[Feature tour and comparison](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.9-query/promo/) · [Searchable FAQ](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.9-query/docs/FAQ.html)

## Downloads

[Complete bundle](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.9-query/x0x-0.1.9-query-experimental.zip) · [Firmware](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.9-query/x0x-0.1.9-query.fwsc) · [Corresponding source](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.9-query/x0x-0.1.9-query-source.zip) · [Offline FAQ](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.9-query/x0x-0.1.9-query-FAQ.zip) · [Checksums](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.9-query/SHA256SUMS.txt)

Expected identity: FM-1_9000109. Firmware SHA256: `cb3efbfe17de3ae9baa8d89afeb42d3cdccabef5d089c5c4ed515dc52ee87489`.
Extract the bundle and read README_FIRST.txt and INSTALL_AND_ROLLBACK.txt. Python 3, mido and python-rtmidi are needed for the command-line tools. Back up first; sample WAVs require separate backup.

## Evidence and limits

Local: all 25 copy scopes, native strict sanitizer tests, production WASM menu/history/storage, 134 required checks / 39 host scenarios,2,557,290 strict query checkpoints,9 Python tests and 15 original target resource gates. Exact OTA/loader/installer simulations passed.

Physical subset:909 full/kick/snare visual copy/Undo;909 full-copy save/restart with distinct AC step 2/swing 60;303 A-only and 808 kick-only exact readback, source/other-lane isolation and Undo baseline/fingerprint restoration. All 25 strip configurations and current project were preserved on install. Not all 25 hardware scope acceptance, broad receiver timing, heavy-load or power-loss qualification.

The query does not trigger save or postpone independent autosave. The separate autosave fix is not included. Internal synthesis bypass, TD-3 routing acceptance, Pi router, named profiles and weighted/evolving blocks remain planned. USB MIDI output is retained; TRS is input. Existing browser emulator and Slice Profiles assets are unchanged.

The bundle includes previous public 0.1.6-beta1 rollback/source; downgrade/restore has not been physically exercised. GPL/component notices and upstream credits retained. No private device data is distributed.
