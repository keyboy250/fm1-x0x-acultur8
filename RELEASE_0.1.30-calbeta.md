Studio Precision X0X MIDI CC experimental beta (0.1.30-calbeta)

This release publishes the unchanged reviewed FM-1_9000130 firmware, corresponding source and a searchable MIDI implementation chart: 751 entries covering 714 firmware controls, 13 browser-only controls and 24 unmapped features. It includes the CC/readback work and accepted routing/inbox fixes, with the existing scheduled USB output and experimental per-unit calibration facility.

Local qualification records 134 required checks, 39 host scenarios, five MIDI feature suites, 15 target gates, updater simulation and a byte-identical frozen-source rebuild. Bounded physical USB CC filtering/readback and selected notes/transport checks passed. Native-panel Freeze/Commit/copy, Undo/Redo and save/reboot checks are separate evidence: advanced equivalent remote CC action hardware replay remains OPEN. This is not full physical MIDI qualification.

Known limitation: Reason external-clock clip/grid stability remains unresolved. The private 0.1.31 averaging and fine-tempo controls are deferred and absent here. No sync fix is claimed. Existing public emulator files remain unchanged. See KNOWN_ISSUES.txt and ACCEPTANCE_REVIEW.json.

- [Release and installation](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.30-calbeta/)
- [Searchable MIDI CC chart](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.30-calbeta/docs/midi/)
- [FAQ](https://keyboy250.github.io/fm1-x0x-acultur8/experimental/0.1.30-calbeta/docs/FAQ.html)
- [Complete experimental bundle](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.30-calbeta/x0x-0.1.30-calbeta-experimental.zip)
- [Exact firmware](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.30-calbeta/x0x-0.1.30-calbeta.fwsc)
- [Corresponding source](https://github.com/keyboy250/fm1-x0x-acultur8/releases/download/v0.1.30-calbeta/x0x-0.1.30-calbeta-source.zip)

Firmware SHA256: `e1221337c317efcf87cfb1a454350bc226dde0f1e5e0f978d9ce91a01da54529`
Source SHA256: `13eabcd7c0af7c9d1e7d637450551fb307d44486c93d7a2f335862bd46c5914c`

Back up before installation. Original X0X by Charles Vestal and contributors; foundation Felucca by Leo Kuroshita/Huegelton Instruments. Component credits and licenses are retained. This variant does not imply upstream endorsement.
