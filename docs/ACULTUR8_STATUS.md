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
