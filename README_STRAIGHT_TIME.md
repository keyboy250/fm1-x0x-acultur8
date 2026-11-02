# Global straight time and sound overrides

Private next-release candidate: **0.1.32-straightbeta**, based on the frozen public 0.1.30-calbeta source. Private 0.1.31 averaging/fine-BPM experiments are excluded. No hardware installation or publication has been performed.

## Use it

Stop playback. Open **GLO → SYNC RATE MODE** and choose **STRAIGHT** or **ALL** with ALGORITHM.

In STRAIGHT, turning a sequencer RATE knob moves through exactly:

`1/16 → 1/8 → 1/4 → 1/2 → 1/1 → 2/1 → 3/1 → 4/1 → 6/1 → 8/1 → 12/1 → 16/1 → 32/1`

Select an individual drum sound or pitched part, press **EDIT**, and use SELECT to find **STRIP SYNC → RATE MODE**:

- **GLOBAL** follows the GLO setting.
- **STRAIGHT** uses the 13-value list regardless of GLO.
- **STR+TRIPLET** permits the existing straight and triplet divisions for that sound, without dotted entries.
- **ALL** permits the complete list for that sound.

For a triplet snare in an otherwise straight composition, set GLO to STRAIGHT, select the snare, set its strip RATE MODE to STR+TRIPLET, and then choose its RATE in SEQ. Other sounds stay on GLOBAL. The SEQ RATE POLICY page also exposes the same local override.

Policy changes require stopped playback and do not rewrite any pattern's existing rate. Existing dotted/triplet rates remain audible and correctly labelled until edited. Turning the knob chooses the next permitted value; changing mode alone does not quantize a composition. Policy and per-sound overrides are saved with the project and covered by stopped Undo/Redo. New projects and older projects retain their existing behavior until this feature is explicitly enabled; GLOBAL/ALL are the default new-policy settings.

The MIDI rate sweep follows the same effective policy. Explicit exact-ID commands retain their existing meaning and ranges; this candidate does not silently remap direct MIDI automation or legacy CC values. New long rates can be chosen on the panel and through the existing native exact-rate API after LAN3 activation. Optional LAN3 readback extends VS_LANE from 13 to 16 values: field12=2, field13=global mode, field14=local override, field15=effective family. LAN1/LAN2 readback remains unchanged.

## Other synced controls

The shared delay-time knob skips dotted/triplet entries in global STRAIGHT mode, using the delay engine's supported **1/16, 1/8, 1/4, 1/2** stops. Its fixed audio buffer and existing tempo-dependent delay folding remain unchanged; this does not provide a 32-bar delay. Per-sound overrides govern that sound's sequencing, not the shared send effect.

This Extended Edition source has no separate tempo-synced LFO engine/rate selector. The physical LFO button is the mixer/tie control. No oscillator or unrelated plugin has been invented or modified. Free-running parameters are unchanged.

## Compatibility and implementation

- Persistent LAN3 marker occupies the existing owned lane slab. Existing rate IDs 0..20 retain their meanings; IDs21..28 add the eight requested multi-bar durations. Length/rate packing remains five rate bits and six length bits, with no project-size increase.
- Global policy and follow flags use exactly the unused LAN2 configuration/header bits. All lengths, slices, gates and sound controls are preserved. Older LAN2-capable firmware refuses a LAN3 project and preserves its stored bytes; retain a pre-feature project for rollback.
- Internal timing reduces the existing rational numerator/denominator by a common factor of six. Three unused bits in each existing rate-unit cell retain long countdown carry; another spare bit distinguishes idle from a legitimate UINT32_MAX countdown. Runtime structures and allocated memory do not grow.
- Audio-owner step/bar latching remains in place; mode changes never truncate an in-flight step. The supported physical timing profile remains 43900..44300 Hz and 20..300 BPM.

## Validation

Run `sh tools/straight_time_check.sh` in the existing compiler image for sanitized native control, timer carry, Undo/Redo, save/reboot, long timing and legacy rate regressions. Run `tests/host/straight_time_wasm_test.mjs` against the actual built worklet/WASM for physical-input UI, saved reload and LAN2-firmware refusal. The existing rate WASM oracle suite also runs unchanged, with its pinned prior oracle.

The actual pi32v2 firmware build and unchanged static budget checks pass. These checks do not measure physical audio deadlines, listening quality or actual knob feel. Hardware acceptance and next-release publication remain open.
