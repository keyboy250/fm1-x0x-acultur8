# Individual sound variants and serial bitcrusher

User-requested feature planning, 2026-10-13. Not implemented or accepted. Local-only publication hold retained.

## Reusable sound variants

Create named individual-sound presets with library slots, separate from pattern/project slots. Capture the selected voice's available synthesis/timbre controls and processing strip (HPF/LPF/boost/ceiling/release/bypass, future crusher). Make pan/sends inclusion explicit. Default load preserves sequence gates/notes, rate/length, progression, other sounds and imported audio. PRESETS currently selects patterns in X0X: do not relabel that existing encoder behaviour as a sound library.

Save selected sound to slot; browse/preview; load into the selected compatible voice; duplicate/rename/export/import; show destination and undoable result. A preset carries schema/version, engine/voice identity and parameter descriptors. 909/808/303 are different models: do not apply unrelated raw bytes across engines or quietly convert. Shared 808 RS/Clave and CP/Maracas lanes retain explicit timbre/strip limitations. Imported WAV/slice banks remain separate; a sound preset does not embed private loops.

Start with browser library/file storage after actual-engine capture/apply is defined. Browser-library slots are distinct from device flash slots. Hardware library capacity and menu need measured storage, migration and resource design before promises. Save library entries explicitly, not per-CC flash writes. Protect existing factory/original files; overwriting a library slot needs an explicit destination/replace action with recovery. Parameter snapshots must be authoritative; loading during playback needs bounded apply, note/tail safety, acknowledged readback and shared UI/CC ordering.

## Bitcrusher in series

Existing DSP has crush/quantise/decimate shaping in some voice and FX paths, but the current common sound strip only exposes HPF, LPF, boost and limiter. A uniform optional per-sound crusher is new integration, not already present for every selected strip.

Proposed fixed chain: sound/tails -> HPF -> optional crusher -> LPF -> boost -> ceiling -> pan/sends. LPF after crushing can soften new high-frequency content; it cannot undo aliasing. Limiter stays after the effect, including final wet/dry mixing, so enabled sample-peak bounds remain valid. Internal crusher wet/dry belongs before final protection. Prototype on one sound, then require all25 existing strips; no silent reduction to shared master crushing.

Candidate controls: enable/bypass, effective bit depth, decimation/sample-hold factor and wet/dry. Domains, gain handling and transitions must be measured before freezing UI ranges. No full-size audio buffers or per-sample coefficient construction; stereo state, block independence and quiet-tail activity must remain bounded. Reuse proven shaping primitives where their semantics fit, preserving exact bypass and existing sound behaviour. Versioned config/preset saves, undo, CC descriptors and locks/dice must include new controls after resource design. Do not silently widen the fixed eight-byte strip layout.

## Acceptance

Presets: exact selected-voice capture, compatible load, every non-target sound/sequence unchanged, named slot persistence/export/import, wrong-engine/corrupt/old-version rejection, overwrite recovery, undo/redo and save/reopen. Hardware slot/menu proof remains separate from browser storage.

Crusher: quantitative bit-depth and decimation fixtures, stereo/channel isolation, continuous state across arbitrary blocks, silence/tails, smooth controls, interrupted bypass, original PCM parity with effect off and enabled ceiling containment after mixed wet/dry. Qualify at least one actual-WASM GUI/audio path then all25, complete CC mapping, preset persistence and existing target memory/flash/cost limits. Physical CPU/draw/audio qualification remains pending. Current combined build has a measured flash overflow; a new effect is not accepted just because it works in the browser.

## Local prototype evidence

The isolated `build/sound-presets-prototype` library passes 1053 test cases across all25 voices with 219 synthesis descriptors and nine optional own-part routing descriptors. Its simulated adapter proves format/compatibility and library transaction behaviour, not actual audio-engine application. It uses memory storage only. The next integration requires an authoritative coherent snapshot and atomic bounded apply/readback; BEGIN/END with sequential existing setters does not supply rollback. Persistent browser storage, GUI and physical slots remain pending.
