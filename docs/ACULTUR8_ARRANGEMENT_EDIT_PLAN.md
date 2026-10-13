# Complete arrangement editing increment — proposed local design

Status: read-only engineering proposal, not implementation or acceptance. Derived from existing FM1-13/14 requirements; does not replace the current MIDI/browser checks. Public publication remains held. Physical flashing is separate.

## Required final behavior

Support insert, duplicate, delete and explicit move/order across all 192 saved rows, plus persisted inclusive loop bounds. Preserve prepared inactive rows, separate editing/playing/queued/start selection, musical-bar durations, shared chord/bass/MIDI behavior, undo/redo and old-project loading. Live existing row edits and next-boundary queueing remain available; structural edits require stopped transport.

## Proposed representation

Keep the existing 204-byte arrangement and 412-byte variant extension. Encode inclusive loop start/end as index+1 in the two existing song reserved bytes, enabled by schema4 flags bit6 and advertised by capability512. With the flag off, retain old reserved bytes exactly; do not interpret them as loop bounds. Existing schema4 firmware rejects the new flag instead of silently ignoring an active loop. Validate both endpoints against active length and order before any mutation. This uses no new saved bytes or project-shadow/pool allocation; actual source and target measurements must establish that claim.

Propose command16 for structural actions, command17 for loop configuration and snapshot9 for loop bounds. Freeze exact ABI layouts and result codes before code. Capability negotiation must disable unsupported controls; do not equate this browser ABI with physical USB SysEx support.

## Structural semantics and ownership

- Insert/duplicate shift the full saved tail, not merely active rows. Reject FULL when active length is192 or the final saved row contains prepared data; never silently discard it.
- Delete shifts the saved tail left, clears row191 and decrements active length only for an active-row deletion. Define deletion of the last remaining active row without producing an invalid length.
- Move permutes saved rows without changing length or losing inactive preparation. Define whether the supplied destination is the final index after removal; use one consistent rule in C, UI and tests.
- Explicitly remap editing, start and queued selections by row identity; define their fallback if a row is deleted. Reject or remap loop bounds before publishing new metadata, without silent widening.
- Require older queued tickets applied and an empty command queue before structural mutation. Use the existing stopped transport reservation; perform bounded row shifts with IRQs enabled, then publish bounded metadata and one audio marker.
- The audio owner relatches stopped state at that marker so deferred Start/Continue cannot use stale chord/pattern state. Preserve existing IRQ-before/during-reservation behavior.

## Undo and storage proposal

Each structural action becomes one immediate undo entry. An unused existing u16 journal-header padding field may mark structural entries, allowing only those entries to require stopped undo/redo while preserving ordinary legacy live undo. Audit every journal reader/writer, allocation/CRC boundary and wrap/eviction path before using it. Reject an oversized previous pending gesture before any gesture commit, source mutation or history change.

A worst structural edit can touch up to1349 canonical bytes (1156 song +192 degrees +1 flags), with an estimated own entry no larger than1370 bytes in the existing20480-byte journal pool. Temporary row scratch is7 bytes. These are design bounds, not measured target frame/flash costs. There is currently only2008 image bytes and436 ordinary RAM bytes spare, plus160 pool bytes above reserve; retain all original thresholds and measure the integrated build.

## Loop and transition contract

Use the existing96-tick musical-bar clock and each block duration. An explicit queued block overrides the next boundary. If a queued block lies outside the loop, finish its duration then return to loop start. Verify Start, Continue, Stop, repeated chords, odd pattern lengths and external-clock fallback. Define stopped start selection outside loop and disabled-arrangement behavior explicitly before implementation.

## Acceptance gates

1. Exhaust all192 source/destination positions and prepared tails; prove exact preservation for insert/duplicate/delete/move and every rejection.
2. Exact undo/redo, single-entry history, journal wrap/eviction and oversized pending-gesture rejection; failed operations change neither source nor history.
3. IRQ before/during stopped reservation, prior queued ticket exclusion, deferred Start/Continue and audio-owned stopped relatch.
4. Loop start/end/duration/queued override/repeat boundaries, odd patterns and external96-tick fallback, with no timing drift.
5. Schema2/3 migration, schema4 loop-disabled reserved-byte preservation, flagged bounds validation, corrupt/future save protection and old-firmware rejection.
6. Actual production WASM/browser commands, capability fallback, acknowledgement/readback, live edit/queue independence, structural stopped locks, save/reopen and narrow layout.
7. Independent baseline PCM/RNG/MIDI regressions, full required suite and unchanged target image/RAM/pool/static gates. Do not infer physical timing or receiver delivery from those checks.

Implementation begins after the current MIDI input/restart acceptance checkpoint is recoverable. This document records the full remaining scope and explicit design questions; no feature is marked complete.
