# Acultur8 feature status

Browser alpha **acultur8-v0.1.0-alpha.1** is published from `0487778bf76915c2312644791d92d30378f4ecb5`.

| Capability | Source/local status | Release or device acceptance |
| --- | --- | --- |
| Existing X0X engine and host simulator | Mandatory host/WASM checks and 31 scenarios pass | Browser alpha published; device qualification pending |
| WAV/exact markers and individual slice audition | Implemented | Hosted synthetic six-slice workflow passes |
| Pitch-preserving internal-clock slice sync | Implemented | Hosted 90/120/180 BPM and shared transport pass; external clock separate |
| Browser saved imports | Implemented | Hosted save/reload/reopen passes |
| Portable local dashboard | Merged; disposable clean-clone checks pass | Launch without optional SDK passes |
| Raw REX conversion | Optional separate native tool | Decoder/SDK not distributed |
| Lossless font compression | Unmerged; 18,368 image bytes saved, exact glyph/display/audio checks pass | Integrated review/CI and device timing pending |
| Shared commands, extension saves, undo | Unmerged; focused tests pass | Final integrated tests and browser controls pending |
| Filters and boost/ceiling | Standalone candidate host tests and target object compilation pass | Kick integration, controls/meters, persistence and full resources pending |
| Arrangement/chords/chord-following bass | Planned | Implementation and whole-scene acceptance pending |
| External MIDI pad | Planned | Browser events and real receiver checks separate |
| Internal pad and hardware slice upload | Later experiments | Resource and device acceptance required |

[Browser alpha](https://keyboy250.github.io/fm1-x0x-acultur8/) · [Release](https://github.com/keyboy250/fm1-x0x-acultur8/releases/tag/acultur8-v0.1.0-alpha.1) · [CI](https://github.com/keyboy250/fm1-x0x-acultur8/actions/runs/38361493563). All 21 deployed files matched build hashes. The alpha includes no promoted hardware installer. Physical CPU, audio/MIDI, storage and recovery checks remain pending.


## 2026-10-13: resource saving and basic arrangement integration

Reviewed PR #11 merged at `d7c63f5ecee490e275d377fd68005d3328ddcb24`; required CI and Pages passed and 24 hosted hashes match. Noise history remains 256 samples; exact pool saving 1,024 bytes and 33,600 bitwise comparison blocks pass.

Basic arrangement adds 192 paged rows, 1–8 musical-bar durations, next-bar queue and shared C triad metadata. Independent review closed the IRQ-start race through bounded locks and nested long-operation reservations. Combined official target: image 576180/581564, RAM 97860/98304, pool 333792/344064; 15 original static gates pass unchanged. New active callee costs are recorded in `lab/ARRANGEMENT.md`. Final integrated browser/hosted checks are pending before this candidate is promoted. Broader row-order/loop controls, all 25 processing paths, both bass followers, MIDI accompaniment and physical qualification remain pending.
