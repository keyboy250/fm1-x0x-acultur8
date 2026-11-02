HISTORICAL PARENT NOTES:0.1.29only. Currentcandidate authority:README_CALIBRATION.md.

# Private 0.1.29-timefix candidate

Authorized scope: isolated source correction and local tests. No device access, installation, push, or publication.

Parent: sealed 0.1.28-clockfix source, SHA256 `1735599232349edc69e532c4737e9c69b78d50ca5665f62c84b879f75873a4c6`.

## Correction

The physical audio rate used for musical time is separate from the nominal DSP design rate. Build with `X0X_TIMING_RATE_HZ=44119` to select the experimental profile for this test unit. The profile is derived from its analog recording and a long-run engine simulation. It is not a universal FM1 calibration or physical acceptance result.

The sequencer uses the profile for legacy and expanded lane timing, external tempo estimation, and external-clock timeout durations. USB MIDI deadlines use the same rate to convert audio sample positions into nominal 24 MHz TIMER4 ticks. Default builds retain 44,100 Hz and the parent deadline rounding. Browser/WASM timing stays nominal because its audio context supplies the nominal sample rate.

DSP sample-rate constants, project format, MIDI CC addresses, stored tempo settings, and USB audio format are unchanged. No persistent calibration setting or public calibration control is added.

Four clock-role opcode shifts are explicitly unsigned. The frozen parent reproduces the signed-shift UBSAN finding; the emitted command bits are unchanged. Calibrated clock-role fixtures now pass strict ASAN/UBSAN.

## Reproduce

With the existing compiler and SDK configured:

```
X0X_TIMING_RATE_HZ=44119 python3 tools/build.py --release 0.1.29-timefix --sdk <existing-sdk>
```

External dependencies are documented in BUILDING.md. `TIMING_RATE_PROFILE.json` records the measurement, selected rate, and model limits.

Local recipes:

- `tools/timing_rate_focused_check.sh`: nominal and explicit profiles, real-engine clocks, deadline scheduling, and clock roles.
- `tools/timing_rate_lanes_check.sh`: all 21 divisions and 25 lanes; unchanged parent negative control.
- `tools/timing_rate_full_check.sh`: fresh nominal host/WASM and complete inherited MIDI/application regressions.
- `tools/timing_rate_target_check.sh`: 44,119 Hz target build and existing static budgets.
- `tools/timing_rate_package_check.sh`: updater simulation and invalid-profile guards.

## Limits

The development kit's 44,118 Hz reference and upstream's independent I2S clock comments support separating physical and nominal rates, but do not calibrate every unit. The inferred hardware rate is relative to the Focusrite recording clock. TIMER4's absolute frequency has not been independently measured.

The model predicts much lower drift for this unit. Physical USB timing, analog recording drift, listening, persistence, receiver behavior, other units, and temperature/long-run behavior remain untested for this candidate. Do not globally replace DSP sample-rate constants or adjust fixed latency to conceal a rate error.

Any installation requires separate authorization, fresh byte-checked backups, and the preserved 0.1.28-clockfix rollback. Existing source, the installed unit, original recordings, and release-review packet remain preserved. Parent-version status/manifest files describe history; the new candidate receipt is the authority for current local qualification.
