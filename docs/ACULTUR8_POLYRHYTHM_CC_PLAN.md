# Critical independent lane timing and live MIDI CC plan

Status: requirements and source audit in progress; no implementation or acceptance claim. User requested2026-10-13. Both FM1-32 and FM1-33 are critical additions to the active variant objective. Preserve existing checkpoints and public publication hold.

## Current source evidence

`firmware/src/seq/pattern.h` stores each drum kit's shared length/rate in dpart_t and each bass part's own length/rate in bpart_t. `sequencer.c:seq_part_len/part_rate` returns a fixed16steps/16ths for BREAK. Individual drum hit rows do not have independent timing. `engine.c:midi_in` handles clock/Start/Continue/Stop and drum/bass/break notes; it contains no CC parameter handler. Existing browser commands are not proof of physical DAW CC support.

## Full required scope

Lane granularity is confirmed by the user: every individual sound lane, including each909/808drum row plus303A/B/BREAK. Cover all25existing sequence lanes; never accept a fivepart substitute. Audit paired808timbres so shared trigger semantics are stated explicitly. Define whether paired808 RS/Clave andCP/Maracas use the existing shared sequencer row or need independent trigger lanes. Keep marked-slice preview explicitly distinct from instrument BREAK.

Each lane needs independently persisted rate and step count, visible current position, and safe live adjustments. Define defaults that reproduce legacy audio exactly, supported count/rate ranges and per-pattern storage before coding. Use common musical time; a7/11/16step combination atone rate is polymeter, while different rate ratios support polyrhythms. Neither lane wrapping nor rate changes may redefine the arrangement's96tick musical bar or chord progression boundaries.

Define live-change phase policy: continuous sound controls at audio boundaries with appropriate smoothing; discrete rate/length updates at documented safe lane boundaries duringplayback. A running performance must not require Stop simply to automate these musical parameters. Verify shrink whilepastnewend, expand, rapid alternation, ties/slides, active-note cleanup, Start/Continue/Stop and externally clocked sessions. No duplicate/retriggered boundary hits or leaked notes.

## Complete CC registry

Inventory all musical controls: individual drum/synth parameters,25soundstrips (filters/boost/ceiling/stages/release), mix/pan/sends/masterFX, sequence timing/length/direction/transposition/chance/accent/slide/ties, generator/mutation controls, arrangement/chord settings, bass-follow ranges/modes and external pad settings. Include editable pattern values through documented addressing if they cannot fit direct flat CCs. Do not treat output meters, account/file/flash installation or destructive service actions as ordinary musical parameters.

Every entry needs stable parameter identity, channel+CC address, units, range,7bit scaling, optional paired14bit precision, application timing, acknowledgement/readback, persistence and ownership policy. Direct ordinary CC automation must work for all musical controls; NRPN may be optional but is not an NRPN-only replacement. Select a channel/bank design onlyaftercounting entries and auditing existing MIDI traffic. Protect bank/modulation/sustain and controller conventions intentionally. Standard CC120..127 are channelmode messages, not free parameter slots.

Official reference: https://midi.org/midi-1-0-control-change-messages . No CCnumbers are assigned or guaranteed by this draft.

## Shared authoritative path

USB/TRS hardware input and browser WebMIDI input must reach the same bounded engine setters. Retain canonical project intent, effective audio state, revision/dirty/readback and safe undo behavior. Existing stopped-only commands for some harmony/pad operations must be extended with live-safe ownership rules; keeping them stopped-only would not satisfy DAW performance control. Changes to note-producing channel/range/chord/mode must release actual old notes on their original channel and apply the newest valid intent. Avoid echoing inputCCs into feedback loops.

Bound automation work per audio block; coalesce superseded scalar values while preserving NoteOff/clock/transport order. Prove queue overflow/backpressure behavior and worst bursts. Never perform a flash write for each incomingCC. Define explicit Save afterperformance and power-loss expectations; hardware interrupted save remains separately tested.

## Resource and acceptance gates

Current measured firmware spare:2008 imagebytes,436ordinary RAMbytes,160poolbytes above unchanged8192reserve. Evaluate per-project versus per-pattern timing storage, expanded saved schemas/shadows, per-lane phase/event runtime and CC registry/ingress costs. Preserve original limits; a browser prototype is not physical firmware acceptance. If hardware integration requires memory redesign, pursue it explicitly rather than dropping individual lanes or CCcoverage.

Require all-lane oddlength/mixedrate tests, exact internal/external timing, independent arrangementbar clock, heldnote cleanup, concurrency/undo/protectedsave/migration, perentry CCcoverage, synthetic DAW automation duringplayback, actualWASM/browser acknowledgements and project reopen, independent baseline PCM/RNG/MIDI regressions and unchanged target build gates. Then perform realDAW/device delivery/timing/audio tests when available; leave physical acceptance pending until actually performed. Add FAQ diagrams and an exportable CCreference afterthemapisfrozen.
