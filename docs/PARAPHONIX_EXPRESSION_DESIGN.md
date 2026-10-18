# FM1-43: Paraphonix distributed chord expression

Status: distant-future exploration. Requested by the user; not implemented or accepted. Depends on compatible device contracts, performance clock, ensemble orchestration, chord/range control and reliable MIDI note ownership. This work does not displace the current regression and performance priorities.

## Musical intent

One progression conductor supplies the current chord, key and scale. A voice allocator distributes a bounded voicing across compatible devices. Devices play complementary notes and musical roles rather than each independently reproducing the full chord. Example: three compatible monophonic devices play D, F and A for a D-minor triad; a two-voice budget selects a documented two-note voicing rather than silently changing the key. Allocation is capability-dependent and not proof that current FM1 parts accept arbitrary external note control.

“Paraphonix” is a proposed product name for coordinated expression. Distributed instruments do not automatically share a physical filter, envelope or audio path. A shared expression gesture means validated coordinated control; a shared audio effect would need a separately designed mixing/processing path.

## Proposed controls and contracts

- Global sounding-note budget and per-device/part caps; reserve capacity for bass or melodic roles. Define which parts participate. Notes in release tails, sustained notes and incoming notes need explicit accounting; uncountable voices are labelled unverified.
- Chord voicing, inversion, octave range and minimum spacing, with smooth voice leading. Octave folding preserves pitch class but may collapse notes; deduplicate and apply the documented chord-tone priority. Narrow ranges and missing devices can make a voicing impossible; report the reduction rather than inventing notes.
- Shared expression controls such as velocity, gate duration, dynamics and supported timbre parameters. Channel pressure, pitch bend, controller resolution and per-note expression must be advertised and tested per receiver; no assumption of MPE support or universal parameter meanings.
- Locks for progression, key/scale, range, voice budget, device roles and selected notes. Seeded random voicing and movement obey those locks and budgets. Manual/external MIDI participation has an explicit policy; the engine cannot enforce a global cap on uncontrolled devices.
- Stable voice identities recording device, route, channel, actual emitted pitch, allocation epoch and note state. Repeated chord tones can sustain; changed tones are released or stolen by a declared policy. Receiver sustain behavior and release time affect actual overlapping voices.
- A display of requested, allocated and verified sounding voices, pending changes and reduced-capability devices. Route reconnection must not silently resend an old chord or override local playing.

## Performance behavior

Changes are prepared against the current capabilities and applied at a chosen musical boundary. Device acknowledgments are separate; group delivery is not assumed atomic. Define policies for late, disconnected or failed receivers, bounded queues, partial delivery and stale acknowledgments. Stop, Panic, source change and project replacement clean up only owned notes, including the actual transposed pitches. Expression updates must not revive released voices.

A voice cap reduces note density, not guaranteed acoustic loudness. Different patches, bass overlap, release tails and effects can still overload the mix. Keep audio metering, headroom and limiting separate from voice allocation; evaluate the combined audio on real equipment.

## Acceptance before promotion

1. Inventory actual per-device note/expression capabilities and agree on the versioned conductor/allocator contract.
2. Test deterministic chord membership, range, deduplication, locks, voice priority and caps, including impossible voicings and rapid progression changes.
3. Test actual engine/receiver allocation, repeated tones, stealing, sustain, disconnect, Stop/Panic, queue saturation and stale/in-flight events. Test caps over release behavior rather than counting only NoteOn commands.
4. Verify saved configuration, undo/reopen, protocol mismatch rejection, independent review and original firmware resource gates for any firmware changes.
5. Qualify a physical multi-device chord/expression scene: measured timing, audible transitions, mixed-output headroom and a real-duration performance soak. Simulations remain separate evidence.

First next check is a capability inventory and a small host-side allocator model after FM1-39/41/42 are ready. No extra onboard polyphony, DSP engines, hardware purchase or flashing is implied by this proposal.
