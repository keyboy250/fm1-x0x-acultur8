# First integrated sound strip: 909 kick

The opt-in 909 BD path processes the complete mono kick sample after the existing
output level, then places that processed sample into its pan, reverb send and delay
send. Its body, beater click and retriggered ringing share one strip. The other ten
909 voices retain their legacy rendering. Initialization leaves the strip bypassed.

The renderer now requires `sound_strip.c` when linking. The shared engine queue now applies kick configuration at audio block boundaries;
versioned project saves retain it. The browser editor uses acknowledged C snapshots
and exposes opt-in stages, Hz/dB controls, meters, Undo/Redo and Save.

## Owner and interface

`drum909_bd_strip_set(d, config)` accepts a validated eight-byte strip configuration
and returns zero, or -1 without changing state. Call it on the audio owner between
render blocks. Do not mutate the embedded state concurrently with rendering.

`drum909_bd_strip_meter(d)` returns a pointer to the latest render-block meter:
input peak is the summed, leveled kick before the strip; output peak is before pan
and sends; minimum gain is the minimum individual limiter/guard stage gain. It is
not a wet/dry ratio or the total gain through the filters and boost. Bypass reports
zero input/output peaks (unavailable) and unity stage gain. A bypass label should
replace peak readouts. The original kick loop runs without per-sample meter work
when bypass is settled. An idle block reports zero peaks and unity stage gain.

The added persistent memory is 68 bytes for `sound_strip_t` and 12 bytes for the
meter: 80 bytes total. The config remains owned by the caller. There is no audio
scratch buffer, per-block allocation or duplicated voice state. A local mono sample
and 12-byte meter temporary are used for each enabled sample. Retriggering preserves
the strip state. Only the original drum's click/body trigger behavior resets.

Settled bypass chooses the unmodified original BD loop once per render block;
the enabled/transition loop is separate so bypass has no per-sample strip branch,
peak comparison or extra meter arithmetic. The final guard release remains part
of strip activity even after an interrupted wet fade reaches zero and BD is silent.

The strip receives zero input after the drum's mute countdown expires so its filter
tails and control fades complete. `drum909_active` includes those tails and fades.
Filter residuals below 1e-8 (-160 dBFS) are cleared once the drum voice is silent;
the strip then stops consuming samples until a new note or pending control fade.
The legacy click filter remains frozen during strip-only tail processing. Lowering
an enabled limiter ceiling bounds the first subsequent sample, including fades.

The finite input magnitude limit of 64 from the shared strip remains the DSP
contract. The configured ceiling applies to this kick before pan/sends. Independent
sounds and effects can still add together at the master bus.

## Verification

Run `sh tests/host/run_kick_strip.sh [preserved-pre-integration-drum909.c]`. Supplying
an independently preserved original renderer compiles it with renamed public
symbols and tables and provides the strongest legacy oracle. Without that argument
the runner clearly labels its bypass comparison as the current disabled renderer.

The focused suite compiles the actual renderer and actual shared strip with warnings
as errors. It checks mixed/retriggered stereo bypass bitwise, all other 909 voices
bitwise, loud summed body/click/retrigger output, lower live ceilings, changing filter
stages, idle and ringing transitions, proportional pan/sends, meter aggregation,
interrupted final-guard release, invalid configuration rejection and actual kick filter response. Its processing
placement oracle runs the legacy mono output through the standalone shared strip.

The runner prints three alternating-order measurements of host CPU time for
2,097,152 legacy, bypass and enabled frames and host object sizes. These are
container/host measurements; they are not
FM-1 timing or device acceptance. Initial enabled runs cost approximately three
times the standalone legacy kick renderer on this host, so the per-sample call is
a material integration cost. Existing target static instruction and new-call
budgets must remain enforced; this change does not approve a higher budget.

After removing settled-bypass meter work, three alternating-order host measurements
were legacy 0.022854/0.022996/0.026066 s, bypass 0.023122/0.022555/0.022588 s and
enabled 0.076781/0.076722/0.076946 s. Bypass/legacy ratios were 1.012/0.981/0.867;
the spread includes host scheduling noise. Enabled processing remains materially
more expensive. Host object text was 265282 bytes for the renderer (including sample
and lookup tables) and 4037 bytes for the shared strip. Target budgeting still needs
the actual target compiler and original unchanged thresholds.

Physical-device timing and device audio acceptance remain pending. The API and passing host audio behavior are a prototype,
not completion of the whole FM1 sound-processing feature.


## Integrated target checkpoint

The official target build fits: image 570020/581564 bytes, ordinary RAM
97636/98304, pool 334612/344064. The pool retains 9452 free bytes, 1260 above
the 8192 reserve. The kick state plus meter adds 80 pool bytes. All original
15 static gates pass with their baseline and thresholds unchanged.

The active kick function is intentionally separate so its additional work is
visible: `d9_render` 2909 instructions / 2038 loop instructions / 10 loop calls;
`d9_render_bd_strip` 499 / 302 / 3; `sound_strip_process` 349 / 315 / 0.
The active kick calls the shared processor once per sample, including tails/fades;
its other calls are conditional legacy envelope/distortion helpers. Static cost
is not CPU cycles. The new callee is outside the original baseline, so the old
budget pass does not qualify enabled processing for the physical device.

Real production worklet/WASM tests additionally pass audible filter response,
boost, limiter/meter bounds, saved audible reopen, audible undo/redo and bitwise
snare isolation. Local comparison against the pre-activation foundation WASM
also passes bitwise default/configured-bypass PCM. CI runs the shared-engine
behavior checks; the standalone kick runner always builds its independent oracle
from the preserved upstream Git commit.
