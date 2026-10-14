# FM1-44: Expanded lane divisions and safe rate sweeps

Status: requested critical extension; design only, not implemented or accepted. Extends FM1-32/33 for every individual sound lane. Preserve the qualified engine and existing projects while testing a separate candidate.

## Confirmed current limitation

The local engine has eight stable rate IDs: 0=1/16, 1=1/16T, 2=1/32, 3=1/8T, 4=1/4, 5=1/8, 6=1/4T, 7=1/32T. Thus 1/8 exists locally, but 1/2 and dotted rates are missing. This is local source evidence, not proof of which build a preview is running. Drum/BREAK persisted rates and runtime latches use three bits. Changing a label or list cannot add engine timing safely.

## Requested musical divisions

Provide straight, dotted and triplet choices for 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 and 1/64. D or a dot means dotted (duration multiplied by 3/2); T means triplet (duration multiplied by 2/3). With a quarter-note beat, 1/8 is half a beat, 1/8D is three quarters of a beat, 1/8T is one third of a beat, and 1/2 is two beats. Show both compact and plain-language labels.

Offer per-lane rate families: Straight only; Straight + dotted (no triplets); Straight + triplets; All. Default new sweep configuration to Straight only. A composition-wide apply action may set participating lanes, without replacing their independent rates or lengths. Locks and randomisation obey these families.

## Safe automation contract

Keep existing raw lane.rate parameter 1129 and channel13 CC105 meanings unchanged. Do not reorder its eight IDs or silently reinterpret recorded automation. Add an explicitly versioned family-aware sweep parameter and policy controls, with collision-free addresses assigned from the authoritative inventory. Require native and browser parity; a browser-only filter would not protect direct device MIDI.

The sweep maps all 128 input values monotonically from slow to fast over the selected family's valid durations, with documented stable bins and endpoints. No triplet can be emitted in a non-triplet family. Repeated values within a bin are no-ops. Define modest boundary hysteresis for noisy hardware controllers, deterministic first-value and direction-change behavior, and exact-parameter selection separately. Do not change tempo, step count, notes or other lanes. Family changes keep an allowed current rate; otherwise show and apply the documented nearest-duration choice at a safe boundary, using a fixed tie-break rule. No silent remapping of stored projects or old CC recordings.

During playback, latch the new rate at the next lane step boundary, preserving the interval already in flight, the existing step position, pending note releases, ties/slides and other lanes. Offer an explicit bar-quantized policy for performers; do not claim zero phase change from any arbitrary sweep. Show selected versus effective rate and pending state. Rate filtering avoids unwanted triplet jumps, but cannot guarantee every rhythmic change will sound pleasing.

## Storage, timing and compatibility

Expand beyond three bits with a versioned, lossless project migration that preserves all old rate IDs, all 32 patterns and 25 lanes, lengths, BREAK data and unrelated sound cells. Older firmware must reject unsupported formats explicitly, rather than misread them. Persist family, sweep policy and hysteresis settings; include undo/redo, project reopen, CC readback, capability contracts, device menu and browser controls.

Use rational musical durations. At 24 MIDI pulses per quarter, some fast dotted/straight divisions fall between pulses; define tested sample-time interpolation and late/lost-clock policy. Never approximate them by rounding each step to whole MIDI pulses. Require bounded scheduling, no cumulative drift, no zero-length interval, correct swing eligibility and safe full-range tempo arithmetic. Long divisions need wider counters if required, without wrapping existing uint8 state.

## Acceptance before promotion

1. Exhaustive division ratios, all rate families and 128-value CC sweeps in both directions; endpoints, hysteresis, locks and no-triplet invariants.
2. Exhaustive legacy migration and new save/reopen across all patterns/lanes; old audio parity and invalid-format rejection.
3. Actual native and WASM internal/external-clock positions at varied tempos, block sizes and fractional-pulse rates; long-run drift and queue-pressure checks.
4. Live rate/family changes, safe interval latching, tie/slide/release ownership, Stop, disconnect, undo/redo and simultaneous automation.
5. Actual browser selection/readback, keyboard/narrow layout and illustrated FAQ; device menu and real DAW/receiver checks separately.
6. Independent review and unchanged image/RAM/pool/static gates. Physical CPU, jitter, listening and performance soak remain pending until performed.

Next: inventory storage and counter costs in an isolated candidate, then qualify migration and rational timing before connecting new controls. Do not mark the target complete from a dropdown or browser-only sweep filter.
