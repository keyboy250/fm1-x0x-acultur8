# Arrangement candidate

This candidate adds arrangement playback and chord **metadata** to the shared
C engine. Bass following and accompaniment are separate, pending features.
It has not passed publication acceptance yet. Physical FM1 testing remains
pending.

## Editing and playback

The editor has twelve pages of sixteen selectors: 192 existing song rows. Each
row assigns a pattern to 909, 808, 303 A, 303 B and BREAK, plus five mute flags,
a duration of 1–8 four-quarter-note bars and an optional degree (None or I–VII).

- **Editing** selects the row shown in the controls without moving playback.
- **Playing** identifies the row whose intent the audio engine has latched.
- **Queue** selects the next musical-bar transition while playing, or the start
  row while stopped. Rows outside the active length cannot be queued.

Editing a playing row takes effect on its next occurrence. Changed pattern
assignments restart at the boundary; unchanged parts can continue their own
odd-length cycles. Arrangement duration is independent of the 909 pattern
length. Disabled arrangement retains legacy song repeat behavior.

```mermaid
flowchart LR
    E[Choose an editing page] --> R[Edit one row]
    R --> A[C engine acknowledges and saves intent]
    Q[Queue a block] --> B[Next musical bar]
    B --> L[Latch patterns, mutes and chord]
    L --> P[Play the block for its saved duration]
    A --> L
```

The key and seven-note mode define diatonic triads. Pentatonic modes require an
explicit seven-note parent for degree chords. Choose that parent before changing
mode if existing rows have degrees. Chromatic mode cannot represent I–VII;
clear those degrees before switching. The displayed pitch classes and MIDI
voicing come from the same C resolver used by future musical features.

## Transport and saving

Stop before changing length, enabling/disabling arrangement, saving or restoring
arrangement history. Pending Start/Continue also count as a transport transition.
A busy result preserves the project and history; it does not silently stop
playback. Legacy SONG editing and recording direct users to the lab while
arrangement is enabled.

Project format 4 carries schema 2. Old format 1/2 projects and the schema-1
format-3 strip extension migrate with arrangement disabled and degrees set to
None. Default saves retain the original format-2 layout. Unsupported or corrupt
extensions protect the existing project and undo history. Keep an old-format
backup when moving between upstream and variant firmware.

## Bridge contract

ABI 1 retains 16-byte commands/replies and 80-byte snapshots. New capabilities
are arrangement (32) and harmony metadata (64); neither advertises bass/pad
audio behavior. Editors disable unsupported controls on older engines.

| Operation | Fields |
| --- | --- |
| 8: set block | target row 0–191; voice packs duration-minus-one and degree; index five mute bits; value five 5-bit pattern IDs |
| 9: set global | index 0 flags, 1 key, 2 mode, 3 pentatonic parent; target/voice zero |
| 10: set length | value 0–192; other fields zero; stopped only |
| 11: edit block | target row 0–191; other fields zero |
| 12: queue block | target below active length; other fields zero; arrangement enabled |

Snapshots 4/5/6 contain the selected block, arrangement transport/intent and
playing chord respectively. The browser displays acknowledged C snapshots,
including separate desired and applied states. It does not maintain another
sequencer or harmony implementation.

## Acceptance checkpoint

The complete local browser-alpha suite passes. Focused checks cover 288 timing
combinations, 192-row persistence, independent legacy migration fixtures,
CRC-valid invalid metadata rejection, protected/history preservation, queue
pressure, same-sample boundaries and external-clock fallback. Actual browser
checks cover pages, edits during playback, chord display, busy saves, stopped
undo/redo, saved reload, a 390-pixel layout and older-engine capability rejection.

The initial firmware target fits image/RAM/pool limits, but fails several
existing hot-path cost gates. Independent review also found a stopped-check/
audio-IRQ transport race. Both need correction, fresh tests and review before
publication. New conditional feature paths must have explicit compiled-cost
measurements; passing legacy static checks does not establish device headroom.

Insert/duplicate/delete, loop-range controls, both chord-following basses,
external accompaniment and whole-scene soak remain broader development targets.
