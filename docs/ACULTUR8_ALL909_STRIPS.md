# Eleven independent 909 sound strips

The eleven 909 voice IDs each own a 68-byte strip and a 12-byte continuous meter.
The first-kick struct fields and public wrappers remain available; ten extra pairs
add exactly 800 bytes, with no configuration duplication or audio scratch array.
The compact noise history remains 512 floats: 256 actual history plus one complete
256-frame block. These are prototype source/layout facts, not a combined device
resource or timing acceptance result.

`drum909_strip_set(d, voice, config)` runs only at an audio-owner boundary and
returns -1 for invalid voice/config without mutation. `drum909_strip_meter(d,
voice)` returns a const meter pointer, or NULL for an invalid voice. Call
`drum909_strip_meter_clear(d)` once before the outer engine block, then render all
event stretches without clearing. Input/output peaks accumulate by maximum and
individual limiter/guard gain by minimum. The BD wrapper maps to voice zero.
Settled bypass records unavailable peaks as zero and unity gain, without sample
meter work. Meters reset only on init or explicit clear, including silent voices.

Each voice's existing renderer produces its complete leveled mono sample before
the strip and before pan/reverb/delay placement. That includes BD body/click,
SD shells/noise, tom VCOs/stick, rim filter sum, clap bursts/tail and sampler output.
Original summing order, shared noise history, raw trigger/choke behavior, block
anchoring and warm-up logic are preserved. A strip is not reset on retrigger.
The final ceiling is per sound before effects/independent voices add together.

The original all-strips-off kit path remains separate from the additional active
callee. The latter must be measured explicitly: passing the original static
function gates alone is insufficient evidence for its runtime cost. When any
strip is enabled, bypassed voices may incur a bounded emitter pointer test while
their samples and sends remain unchanged. No caller uses a sample-ratio division.
Enabled samples incur one call to `sound_strip_process` from the additional
active dispatcher. Both the dispatcher and processor require explicit target
cost reporting. The all-off known-NULL emitter has no processor call.

Raw mute countdowns do not stop strip filter/control/guard tails. After raw source
completion, zero samples clock the strip without advancing the original voice's
envelopes, oscillator or warm-up filter. `drum909_active` includes common strip
activity. The shared pure activity policy stops inaudible filter history below
1e-8 and tracks limiter/guard/control release; enabled settled silence is idle.

The new runner `tests/host/run_all909_strip.sh` compiles both actual code and
pinned accepted 628a9afa5ccac09ffbd87e0d65763251f744b003 source/header with independent
storage and renamed symbols. It checks mixed default bypass bitwise across 1200
variable-size blocks, every voice's audible response and ceilings, lower ceilings,
live stage edits, retriggers, silent tails, pan/sends and independent meters against
the accepted raw output followed by the shared processor. A separate eight-chunk
test proves accumulated peaks/minimum gain survive later chunks until clear.
An all-enabled mixed-kit test checks independent ceilings/meters through
retriggering and staggered CH/OH choke transitions. Native and AddressSanitizer
focused tests pass; this does not establish target resource fit.
The runner prints three alternating-order host measurements of accepted,
all-off and all-enabled mixed 909 rendering. `ALL909_SKIP_BENCH=1` omits timing
only, retaining every parity/bounds/meter assertion. Host ratios do not establish
physical CPU/stack headroom.
The retained implementation uses the project's O2 compilation without size
attributes or fast-math changes. Active processing cost remains material and
requires device measurement; resource thresholds remain unchanged.
Existing kick/noise tests clear current meters explicitly at their render boundary;
pinned source/header and their audio parity assertions remain unchanged.

Combined target flash/RAM/pool/stack, original unchanged gates, additional active
callee metrics, full host/WASM suites and physical timing remain separate acceptance
steps owned by the integrated build. No SDK files or generated binaries belong
in this source change.
