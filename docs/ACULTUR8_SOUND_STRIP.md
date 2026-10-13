# Sound-strip DSP candidate

This is a host-tested audio building block with an opt-in 909 kick renderer API.
The engine command queue and browser controls do not enable that audio path yet. The separate
variant foundation can save its configuration. This does not complete FM1-27,
FM1-11 or FM1-12. Device placement and real-time measurements remain open.

## Signal path and ownership

At 44.1 kHz, each sound uses:

```text
all voices and ringing tails of ONE sound
                   |
                   v
      sum -> HPF -> LPF -> input boost -> linked peak limiter
                   |                              |
                   +------ wet/dry transition ----+
                                                  |
                                      final sample-peak guard
                                                  |
                                         pan and effect sends
```

The limiter receives the summed sound. Applying one limiter to each voice and then
adding the voices cannot enforce a per-sound ceiling. Independent sounds and effect
returns can still add together; the existing master protection remains necessary.

`sound_strip_set` and `sound_strip_process` share one owner. Set commands at an audio
block boundary, then process the block. Do not update the struct from another core
while rendering. Both stereo channels use the same settings and limiting gain.
The in-place API also supports mono by passing the same buffer as both channels.
Inputs must be finite with magnitude at most 64, comfortably beyond ordinary engine
levels. This is a DSP contract, not a file-parser validation function.

## Filters and controls

The high-pass and low-pass are nonresonant, first-order filters: 6 dB per octave.
Each uses a topology-preserving one-pole section with
`G = tan(pi * cutoff / 44100) / (1 + tan(pi * cutoff / 44100))`.
The high-pass is the complementary input-minus-low-pass output. No resonant
feedback or additional pole is introduced.

Coefficients, boost and an increasing ceiling glide toward new targets with a
roughly 5 ms one-pole time constant. Coefficient generation, dB conversion and
release calculation happen in `sound_strip_set`, outside the sample loop. The
sample loop has no allocation, transcendental functions or double arithmetic.
Filter states below `1e-20` are flushed. Finite precision can leave a tiny DC
residual; the +24 dB, 80 Hz high-pass test bounds it at `4e-5` full scale.

The variant save schema freezes an eight-byte little-endian configuration layout.
Compile-time size assertions and the schema's magic, version and length guard
the stored layout. The fields appear in this order:

| Field | Range | Default |
| --- | --- | --- |
| `highpass_hz` | 20–20000 Hz, unsigned 16-bit | 20 |
| `lowpass_hz` | 20–20000 Hz, unsigned 16-bit | 20000 |
| `boost_db` | 0–24 dB, unsigned 8-bit | 0 |
| `ceiling_db` | 0–48 dB attenuation, unsigned 8-bit | 1 = −1 dBFS |
| `release_ms` | 10–250 ms, unsigned 8-bit | 80; fixed in the first UI |
| `flags` | bit 0 enabled, 1 HPF, 2 LPF, 3 limiter | 0 = bypass |

The cutoffs may cross; that deliberately creates a strongly attenuated band.
Invalid values are rejected without mutating runtime state. BOOST is input gain
into the limiter, rather than an upward compressor or an independently defined
noise-floor control.

## Limiter choice

The detector reads the larger absolute sample of the stereo pair. Gain reduction
takes effect on the same sample; gain returns toward unity with the configured
release time constant. The final guard uses one additional linked gain when
necessary and a numerical clamp to contain rounding at the ceiling.

This enforces a **sample-peak ceiling**. There is no intersample true-peak guarantee
and no claim to reproduce IK Multimedia's algorithm. Instantaneous attenuation
can distort sharp or heavily boosted transients. It is the lower-memory design
chosen for this candidate, with listening acceptance still required.

A 64-sample stereo lookahead buffer alone would require 512 bytes per strip,
or 12,800 bytes for 25 strips, plus its envelope/state and any alignment handling.
This candidate has zero lookahead latency and no audio delay buffer. Its total
runtime is 68 bytes per strip, including transitions. This is four bytes more
than the initial 64-byte candidate: a retained final-guard gain is separate from
the wet limiter envelope. Its bounded 0–220 release countdown occupies the
previously unused upper 16 bits of the runtime flags word. Control and stage
updates preserve those bits; desired configuration flags remain in bits 8–15.
The saved eight-byte configuration and variant schema are unchanged. Optional per-block meters
are caller-owned: input peak, output peak and the minimum individual linked
limiter/guard stage gain. That gain is a stage indicator, not an exact end-to-end
wet/dry gain-reduction measurement; an editor must label it accordingly.

## Bypass, switching and ceiling changes

Disabled strips start in exact bypass. Once bypass is settled, processing returns
without writing either input buffer, preserving even signed zero. There is no
per-sample DSP work in this state unless the caller requests peak metering.

For an active stream, a bypass request fades wet to dry over 220 samples
(approximately 5 ms), then resets filter/limiter history. Enable fades from dry to
processed audio over the same interval. Changing filter/limiter stage flags first
fades to dry, changes/reset stages at zero wet mix, then fades in. Rapid changes
use the most recently requested configuration; they do not leave stale wet states.
The host test measures bounded sample differences for these active-stream cases.

An enabled limiter always guards the final mixed output. Tightening its ceiling,
or enabling it while dry audio already exceeds the ceiling, therefore attenuates
immediately. A smooth fade and an immediate hard ceiling cannot both preserve an
already-over-ceiling dry sample. This contract prioritizes the requested ceiling.
Disabling the strip/limiter releases the last final-guard gain to unity over
220 samples. This tail is independent of the wet mix: an enable immediately
interrupted after one sample cannot drop the guard when wet mix reaches zero.
Repeated controls preserve the countdown; re-enabling/tightening a limiter
may attack immediately, but any interrupted release continues smoothly when
attenuation is no longer necessary. Once removed, the guard does not keep
limiting the stream. Exact bypass requires both zero wet mix and a completed
guard release. Stereo uses one shared guard gain throughout this tail.

Integration must retain this processing call until `sound_strip_bypassed()` returns
true; skipping it immediately when the enable flag clears would skip the fade.
Do not make a second abrupt bypass switch around this module.

## Verification and resource gate

Run `sh tests/host/run_sound_strip.sh` in the existing build container or a host
with a C compiler. Production DSP compiles as strict C99 with warnings and double
promotion treated as errors. Tests cover:

- Exact disabled parity, fixed structure sizes and atomic rejection of bad values.
- Silence, DC removal and measured 50/1000/8000 Hz response against the bilinear
  one-pole formula for both filters.
- +6 dB boost, +24 dB limiting, already-summed coincident transients, both stereo
  detector directions and an immediate lower ceiling.
- Monotonic bounded 80 ms release, active bypass/enable and stage-switch ramps.
- 800 cutoff/boost/ceiling/release changes with finite, bounded, linked output;
  mono output equals an identical-channel stereo render.
- A bounded live +24 dB gain ramp and bit-identical rendering across single-sample
  and 256-sample blocks, including enable/bypass transitions.
- Interrupted enables at 1/7/110/219/220 samples, smooth final-guard release after
  strip/limiter removal, repeated controls preserving packed countdowns, rapid
  re-enable/tighter-ceiling toggles, stereo/block parity and finite signed-zero bypass.
- A 25-strip host-only render-cost measurement.

The first local container run measured 0.058 CPU seconds for 25 strips processing
256,000 stereo frames each, about 1.00% of real time on that host. This is a
repeatable host comparison, **not FM1 CPU headroom**. A later run can vary with
host load and compiler versions.

Before the interrupted-guard fix, the official pi32v2 compiler compiled the standalone object with the production
`-O2 -ffunction-sections -fno-builtin -mcpu=r3 -mfprev1 -ffp-contract=off` flags.
Its function text totals 2,280 bytes: process 978, set 1,132, init 54, validation
64, defaults 26 and bypass query 26. The object has no named undefined symbols,
unknown decoded instructions or software-double helpers. The process function has
297 decoded instructions, two floating-point divide sites and an 80-byte stack
frame; set has 282 instructions, seven divide sites and a 56-byte frame. These are
static measurements of the earlier 64-byte-state object, not measurements of the
current guard-tail fix or a complete linked-image/device deadline check. The current
68-byte-state object must be measured again with the integrated target build.

The guard-tail host run measured 0.059 CPU seconds for 25 strips processing
256,000 stereo frames each (1.02% of real time on that host). Its strict-C99 GCC
object has 4,037 bytes of text and no data/BSS, versus 3,687 bytes for the preserved
pre-fix source compiled with the same flags (+350 host text bytes). The kick suite retained bitwise
legacy bypass/other-voice parity and passed its filter, summed-tail, pan/send,
meter and silent-stage tests. Its host timing for 2,097,152 frames was 0.023011 s
legacy, 0.026822 s bypass and 0.076516 s enabled; `drum909_t` is 7,632 bytes
on that host, including the 68-byte strip and 12-byte meter. None of these host
numbers establishes device CPU headroom or target ABI placement.

Twenty-five runtimes require 1,700 bytes (100 bytes more than the initial candidate);
their compact configs add 200 bytes.
The measured baseline had only 1,548 bytes of pool space above its existing 8 KiB
reserve, before adding the variant foundation. Consequently the full allocation
does not yet fit that measured reserve. Keep this as a browser candidate until
additional RAM is recovered and placement, target image size, summed integration,
metering, smoothing, bypass, persistence and worst-case device timing are measured.
