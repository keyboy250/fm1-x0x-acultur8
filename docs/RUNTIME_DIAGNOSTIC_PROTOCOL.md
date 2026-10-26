# Read-only runtime diagnostics — private 0.1.18-diagbeta candidate

This separate child of frozen 0.1.17-randombeta investigates the P7 909-snare variation failure heard on the FM1. It does not claim to fix that failure. No installation or publication is part of local qualification.

The existing command49 query reads stored RAM notes, not the active recipe, counters or temporary variation. New command50 reports one atomic main-context snapshot of those runtime fields. It performs no save/export, command-queue drain, read-clear, selection, control input, note/CC transmission, reset, transport request or project/Undo edit. Normal existing autosave remains independent of a query.

## What the snapshot tells us

- Requested lane, actual playing slot, active recipe owner/slot, cadence, amount, locks, mode, seed, range and captured pitch constraints.
- Actual lane position/pass counter, shared-part counter, active clock length/rate, stored part length and the length supplied to the variation function. Shared clocks are reported when independent lane clocks are disabled.
- Applied arrangement flags, song/chain state, pending transport, queue progress, mute masks and the current probability/condition value.
- Stored base gate mask; a calculated predicted drum gate mask before mute/probability; and the existing Freeze recorder's accepted-event prefix, with count/length/owner validity.

**Predicted gates are calculated intent, not measured triggers.** Accepted prefix bits come from the existing recorder after the engine accepts the selected lane's hit. No extra audio hot-path writes, event logging, static target RAM or project schema were added. For303 lanes the masks represent NOTE gates, not a pitch trace. Accepted events do not prove that a MIDI receiver received a message, and they are not audio recordings.

The recorder is a current-pass prefix, not a ring buffer. Only positions1 through capture_count are meaningful. Tail flags from older passes are never emitted as accepted bits. A slot/owner mismatch invalidates the prefix. Polling can miss a complete pass, and a stopped prefix can describe the last observed partial pass. Frozen capture stays static by design. Use transport epoch, owner, counter and count together; no all-pass history or timestamp precision is implied.

## Client

Use Python3 with existing mido/python-rtmidi. No new library installation is required by this candidate.

```
python tools/fm1_runtime_read.py --lane 1 --samples 48 --interval 0.25 --output snare-runtime.json
```

Lane1 is909 snare (lane0 is909 kick;11..21 are808;22/23 are303A/B). The default is one snapshot. Polling is limited to at most4Hz and240 samples, with a1.5-second timeout per request. Choose exact ports if several matching devices exist. Older firmware will time out; the tool sends no fallback or upgrade command. It refuses to overwrite a result file and discards failed scans. Keep private device dumps outside public release assets.

Reading can be done while playing or stopped. The user controls playback; the client never starts it. The snapshot copies bounded existing state under the normal control interrupt guard, then computes/encodes the result outside that guard. Host PCM parity and target static gates do not establish zero physical timing cost; USB/IRQ timing and real FM1 observations remain pending.

## Protocol

Request SysEx data (without F0/F7): `7D 46 4C 32 01 token lane` (32 hex is command50 decimal). Lane0..23, token0..127; exact length7. ABI1 only.

Reply data: `7D 46 4C 32 01 token lane status [count words...] crc`. Success count52; each unsigned32-bit word uses five low-first7-bit bytes. Refusal has no count/words. Status0 success,1 invalid request,2 unavailable runtime,3 unsupported ABI. CRC32 covers every data byte before the five low-first7-bit CRC bytes. Success276 bytes including F0/F7; refusal15 bytes. No pointers or raw structure padding are exposed. The client checks CRC, length, version, field count, request correlation and bounded values.

| Word | Field |
|---|---|
| 0 | `revision` |
| 1 | `queue_written` |
| 2 | `queue_read` |
| 3 | `transport_epoch` |
| 4 | `playing` |
| 5 | `request_start` |
| 6 | `request_stop` |
| 7 | `request_continue` |
| 8 | `transport_hold` |
| 9 | `transport_stage` |
| 10 | `requested_lane` |
| 11 | `playing_pattern` |
| 12 | `lane_clocks_enabled` |
| 13 | `lane_storage` |
| 14 | `expanded_rates` |
| 15 | `arrangement_flags` |
| 16 | `song_on` |
| 17 | `song_position` |
| 18 | `chain_a` |
| 19 | `chain_b` |
| 20 | `recipe_lane` |
| 21 | `recipe_pattern` |
| 22 | `every_raw` |
| 23 | `change` |
| 24 | `seed` |
| 25 | `lock` |
| 26 | `mode` |
| 27 | `options` |
| 28 | `notes_mask` |
| 29 | `low_note` |
| 30 | `high_note` |
| 31 | `step_zero_based` |
| 32 | `cycle` |
| 33 | `active_length` |
| 34 | `active_rate` |
| 35 | `stored_part_length` |
| 36 | `part_cycle` |
| 37 | `freeze_state` |
| 38 | `capture_count` |
| 39 | `capture_length` |
| 40 | `capture_prefix_valid` |
| 41 | `capture_owner_matches` |
| 42 | `base_low` |
| 43 | `base_high` |
| 44 | `accepted_low` |
| 45 | `accepted_high` |
| 46 | `mute_mask` |
| 47 | `voice_mute_mask` |
| 48 | `probability` |
| 49 | `predicted_low` |
| 50 | `predicted_high` |
| 51 | `variation_length` |

Patterns in raw words are zero-based; the client adds one-based pattern numbers and hit-step lists. A recipe lane255 is OFF. step_zero_based255 means no current started position. freeze_state0 RECORD,1 REQUEST,2 HELD. every_raw0/1/2 means2/4/8 passes. Drum mode0/1/2 means TOGGLE/ADD/DROP; options bit0 selects EVOLVE rather than SAME and bits1..2 select ALL/LAST4/LAST8/LAST16. Probability1..100 is chance;101..135 is a condition code. Zero chance is possible. Muted bits use the lane index.

## Device test after a separately authorized install

1. Preserve a fresh project baseline and rollback, with playback stopped and MASTER down; install only after local qualification and explicit device authorization.
2. Recreate909 SD P7, EVERY2, CHANGE8, LOCK NONE, seed1, TOGGLE/SAME/ALL. Starting from the current P7 hits2,3,9,16, local emulation predicts alternating hits4,5,6,7,8,16.
3. Take stopped and user-started playback snapshots. Check owner/slot match, pending transport, counter progression, active versus variation length, expected gate mask, accepted prefix, mute and probability.
4. Stop and query stored P7 again to verify that observing runtime did not commit a variation. Diagnose from actual device data; do not infer a hardware fix from the emulator.
