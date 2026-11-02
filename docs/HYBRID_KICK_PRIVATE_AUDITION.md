# Private0.1.35-kickshape — native sound audition

First hardware scope:808 kick only. No Riser/Air port or publication.

## Before installation

The existing0.1.34 private kick build is the expected parent. Keep its exact rollback package and verify current device identity before writing. Fresh readiness, repeated byte-identical project exports,25 strip settings, P8/P9, calibration and native transport checks are required. Do not start playback or raise MASTER automatically. Installation is gated by full local qualification and frozen-source byte parity. The parent PHASE pitch/save/performance acceptance remains separate and open.

## Explicit limitations

- The39 kick controls are **session-only**. They reset on reboot/project reload and are not stored in the song, per-sound defaults or Undo history. Change them only with playback stopped and the kick released. This is a sound audition, not a completed production integration.
- **Existing MIDI CC bindings remain unchanged.** CC map identity remains `draft-1-actions-prototype-1`. The new kick controls are explicitly UNMAPPED in the generated registry. There is no new automation address assignment or claim of finished CC-map migration. The release requirement for versioned maps and pre-upgrade breaking-change warnings remains in force.
- Original808 output is preserved when HYBRID isOFF. When ON, the native synth runs live, blended with a small interpolated wavetable or compressed one-shot. The browser's127-velocity rendered bank is not on the FM1. Native oscillator/envelope approximations are not promised sample-identical to the browser preview.
- USER1 reads **zone0 of existing user sample slot1** in place as a one-shot. This build does not create a new flash region or write/upload/replace any sample. Empty USER1 selection is refused. Any later favourite-kick upload requires explicit slot choice, fresh backup/readback of existing samples and CRC verification before replacing data. The original sample uploader/slot format remains in use; no sample upload is part of this install.
- Jitter and optional velocity tiers affect the internal kick sound; outgoing sequencer MIDI note velocity retains the existing behaviour in this audition. No DAW/MIDI/audio alignment claim.
- Save/power-cycle persistence, new CC mapping, sample-loader UX, exhaustive performance and listening acceptance remain release work.

## Native navigation

Select808 -> EDIT -> BD. Eight kick pages follow the original808 sound page: KICK RAM,WAVE SHAPE,VELOCITY MOD,SYNTH ENV,WAVE ENV,ENV CURVES,CLICK,CLICK FX. Refer to the extension section below for their four-knob layouts. The SELECT list also exposes Low/Mid/High velocities,jitter chance/range,body-filter enable andvelocity-to-punch. Original KIT and all four strip pages remain available. Controls refuse edits until playback is stopped and the previous kick has finished. HYBRID defaults OFF; the two envelopes default OFF;click level/crunch/smooth default0.

## Audition sequence — guided, one observed action at a time

First verify original kick with HYBRID OFF. Then, stopped/released, enable Hybrid with WARM and35%wave. Compare0/50/100 blend, four waves, velocity modulation, tier/jitter choices and optional filter. Test custom playback only after its actual slot contents are known. Keep a quiet output level. Measure physical CPU/IRQ stack/overruns with all existing parts before accepting headroom. Stop on an unexpected result; retained0.1.33 rollback is available. No publication implied by any local pass.


## Envelope/click extension �0.1.35 private audition

New controls start bypassed: S ENV OFF,W ENV OFF,CLICK0,CRUNCH0,SMOOTH0. Native sound when all additions are bypassed is tested against the frozen0.1.34 DSP. No saved project bytes or CC dispatch changes. The new39-row registry remains explicitly unmapped; map identity is unchanged. A redundant16-sound drive cache was removed to recover64 pool bytes; the canonical drive pot value is read directly with the original arithmetic. This requires original full-kit PCM parity and existing drive/strip/range checks.

Eight kick pages preserve the existing14-page native limit and the original KIT/filter/limiter/sync/default pages: KICK RAM,WAVE SHAPE,VELOCITY MOD,SYNTH ENV (enable,attack,hold,decay),WAVE ENV,ENV CURVES (synth curve,wave curve,sampleattack,samplepunch),CLICK (level,tone,length,crunch),CLICK FX (smooth,samplepunch-time,sampletail,samplelevel). The SELECT list exposes LOW/MID/HIGH velocities,jitterchance/range,bodyfilter enable andvelocity-to-punch along with existing Drive/Dist. All39 controls must be reachable through pages or that list. This is a private session-only UI reorganization; existing CC addresses remain unchanged. A/H/D read in ms; curves use signedpercent,negative holding more body andpositive dropping faster. Hold maps0..500ms,decay5..3000ms,attack0..100ms. Native7-bit duration encodings are coarser than the browser sliders.

S ENV shapes the live808 circuit output; it does not replace or extend its natural resonance tail. W ENV replaces the wavetable amplitude envelope,or multiplies the existing custom sample after sample-specific attack/punch/tail processing. Velocity-to-Decay scales W ENV. Native source remains a compact live engine rather than the browser's rendered synth bank; identical browser/FM1 PCM is not claimed.

Click is a deterministic,velocity-sensitive high-passed noise transient added after the hybrid body filter but before existing808 drive/strip processing. Tone maps800..12000Hz;length1..80ms,level0..60. Crunch and Smooth0 bypass their stages;Crunch quantizes, Smooth applies a two-stage low-pass toward800Hz. They process before the click envelope so the tail ends within its selected length. Click randomness does not advance kick velocity jitter. All setters remain stopped/released-only; HYBRID must be ON to audition the new layers.

Hardware listening,performance/IRQ-stack headroom and actual knob acceptance remain pending until separately observed. Do not infer them from local sanitizer/static-budget checks. Retain exact0.1.34 rollback and fresh project/strip/calibration backups before device installation. Riser/Air and custom-sample upload are outside this install.
