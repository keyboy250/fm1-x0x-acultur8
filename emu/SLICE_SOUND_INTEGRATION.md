# Browser slice-sound integration · 20 October 2026

This website update applies Slice Profiles settings in the browser emulator.
The pre-existing WASM engine and physical 0.1.6 firmware are unchanged.

1. In Slice Profiles, apply the desired slices and sound settings. Export a review project JSON. Keep its original WAV.
2. In the emulator, choose that WAV and JSON under **WAV slices + sound settings**, then import and switch on the panel.
3. Select a slice and **Audition selected**. Inactive slices can still be auditioned.
4. Set Active, Length, Hold/Decay enabled, Hold, Decay and Level; **Apply slice sound** commits the selected slice and stops slice playback.
5. Play original timing, or enter the original loop BPM and **Play with FM1 · keep pitch**. Active hits play with the emulator instruments and follow the internal sequencer clock. Slower tempos expose gaps; faster tempos can overlap tails. No sustained-audio time stretching is performed.
6. **Save import in browser** keeps the original PCM, timing and all settings in IndexedDB. **Export review project** saves portable settings and a matching-audio fingerprint; keep the WAV with it. Marker export contains cuts/timing only.

## Implementation and limits

`slice-core.mjs` is copied unchanged from the released GPL-3.0-only Slice Profiles companion. `shaped-bank.mjs` uses its `renderSlice` routine once per edit/import to render separate immutable-source cuts. The resulting bank is passed to the existing WASM marked-player; scheduling and rate conversion remain in that engine. The AudioWorklet selects active metadata for group/sync playback or one original slice for audition. It maps playback highlights back to the original indices.

The renderer preserves the recorded signal during Hold, then linearly attenuates over Decay, applies Level and the review profile's click-reduction fade. Envelope off retains the selected Length; an enabled zero Hold + zero Decay is silent. Original WAV samples are unchanged. Legacy marker imports and older browser saves use neutral envelope/level with zero edge fade, preserving their original PCM.

Up to 64 slices, eight simultaneous one-shot voices, WAV mono/stereo 8–48 kHz and 30 seconds, and 1,440,000 packed source frames. Overlapping source cuts can require more packed memory than the WAV; oversized banks are refused without truncating. Shorten those Length values if needed. Sync retains the engine's overlap guard at changed tempos.

Review v1/v2 is supported; original sample rate, channels, frame count and decoded PCM SHA-256 must match. Invalid imports do not replace current settings. The exported review may also carry original onset times, loop duration and BPM for emulator round trips. The companion only restores its own source-boundary timing if such custom timing is reopened there.

This does not add physical FM1 sample export/upload, native Hold/Decay parameters, a new firmware engine, raw REX decoding, recording or velocity filtering. Existing firmware and release ZIPs remain frozen and describe the initial release state.

## Verification

Automated tests compared actual WASM-rendered stereo samples against the editor renderer, checked cutoff/silence, zero envelope, neutral legacy imports, inactive selection, review round trips, wrong-WAV refusal, unchanged original PCM and real sequencer sync at 90/120/180 BPM. Edge browser checks exercised the actual AudioWorklet, import/edit/audition, persistence across reload, portable JSON, layering/sync, errors and mobile controls. These checks do not establish user listening acceptance or physical-device support.

New source files retain GPL-3.0-only notices. Existing project and component notices remain in place. Repository branch `gh-pages/emu` contains the browser source; exact historical WASM build provenance remains in `manifest.json`.
