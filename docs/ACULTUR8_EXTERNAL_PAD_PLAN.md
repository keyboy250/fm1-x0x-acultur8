# External MIDI chord accompaniment plan

Status: planned and unimplemented. This is a private local design contract, not
an accepted runtime feature or permission to publish. Public pushes, pull
requests, Pages deployments and releases remain held for explicit user sign-off.

The complete target is a sustained external-MIDI triad following the playing
arrangement block alongside the existing sequencer. The external instrument
supplies its pad sound. No internal pad oscillator, audio voices or effects are
part of this target; an internal synth would require a separate audio/resource
design and acceptance.

## Configuration and migration

Reuse the existing four arrangement fields without increasing the 204-byte
arrangement or 412-byte variant extension:

| Field | Meaning |
|---|---|
| `pad_channel` | 255 disables accompaniment; 0 through 15 selects its dedicated MIDI channel |
| `pad_root` | MIDI root-position voicing hint, 0 through 127 |
| `pad_low`, `pad_high` | Inclusive MIDI bounds, 0 <= low <= high <= 127 |

Initial velocity is fixed at 90. Root-position triads use the accepted
`harmony_resolve` root/third/fifth contract, closest eligible root to the hint,
lower voicing on ties. Do not substitute seventh chords, inversions, partial
chords or silently widened ranges.

Use schema 4 with the same format-4 stream and extension size. Validate schema 2
with its original enable-only flags contract and schema 3 with its original
bass-mode flags contract before migration. Schema-2/3 migration sets
`pad_channel=255`: previously reserved nondefault pad-channel bytes must not
unexpectedly begin emitting MIDI. Retain the validated root hint and ranges.
Legacy/schema-1 loads use disabled defaults. CRC-valid malformed fields,
incorrect schema/length, torn streams and bad CRC must preserve live project,
undo journal and stored bytes, lock writes, and retain existing owed cleanup.

Add runtime ownership for three actually queued notes, their original channel
and a status byte: approximately five bytes, potentially eight after alignment.
Use note/channel 255 as inactive; MIDI note zero remains valid. Do not allocate
pool memory. Desired notes are recomputed from the latched chord instead of
adding another saved song table or persistent desired-voicing copy.

## Musical behavior

An eligible accompaniment requires running playback, enabled nonempty
arrangement, enabled MIDI note output, a configured pad channel, a positive
latched degree, complete voicing within bounds, a collision-free channel and
ready transport. Start/Continue establishes the currently playing triad.
One unchanged triad sustains across repetitions and adjacent blocks.

Changed chords release the old triad then establish the new triad at the
arrangement boundary. Current key/mode/parent/degree come from the same musical
latch as bass following, never from the editing page or unlatched desired data.
Retry attacks run after same-sample bass NOTE/TIE/retune handling so only the
current final chord is queued. Drop obsolete desired attacks when another
boundary, stop or error occurs; retain cleanup owed for actually queued notes.

Degree zero means no pad chord and releases accompaniment. Disabled/empty
arrangement, disabled note output, collision, impossible complete voicing and
transport loss also release or retain pending release. Do not play a legacy
root fallback or partial triad in these cases. Resolution recovery starts the
current eligible chord at an agreed musical boundary, not an obsolete one.

No changes to pattern timing, bass envelope/gate/accent/slide/TIE handling,
automatic mutation, random streams, drums, breaks or ordinary MIDI clock are
permitted merely to add accompaniment.

## Atomic producer and cleanup ownership

Extend the existing feedback producer to a bounded transaction of at most six
USB-MIDI packets: three old Note Offs followed by three new Note Ons. Cleanup
alone requires at most three packets. The transaction checks complete capacity
and publishes all packets together, or rejects everything. Keep the existing
64-packet queue, publication barriers and single-producer/control ownership.
An identical old/new triad emits nothing.

Only accepted enqueue changes ownership. Full queues, disconnected transport or
rejected transactions retain original notes/channel and retry without spinning,
allocating or replacing the ownership with desired state. Stop, output disable,
channel changes, mute/collision, reset/load and engine reinitialization preserve
owed cleanup. A rejected cleanup never allows a future stopped attack.
Firmware enqueue acceptance remains distinct from delivery to a receiver.

The pad channel is dedicated and must differ from all five configured sequencer
channels. Reject conflicting pad commands. Prevent ordinary channel setters
from introducing a collision while pads are enabled, and retain a runtime
collision check for loaded/external changes. Collision releases pads and reports
its reason without rewriting the saved choice or sounding a partial chord.
The existing bass same-channel/same-pitch coalescing remains unchanged: it has
one receiver tone and attack; independent articulation requires separate channels.

## Bounded command and snapshot contract

Append `VC_SET_PAD=14`, with target=voice=0 and indices 0=channel, 1=root hint,
2=low, 3=high. Append `VC_SET_MIDI_OUTPUT=15`, target=voice=index=0 and value
0/1, applying the existing saved `notes_out` field and effective output intent.
Both use stopped-only transport reservation, validation before narrowing,
atomic canonical publication, queue tickets, dirty state and undo gestures.
Invalid, full, protected or busy requests change nothing.

Append capability bit 256 and `VS_PAD=8`, target=voice=0, with exactly 16 values:

| Index | Value |
|---|---|
| 0-3 | Configured channel, root hint, low, high |
| 4 | MIDI note-output enabled |
| 5 | Effective eligibility |
| 6 | Blocked reason: none/no chord/range/collision/disconnected/output disabled |
| 7 | Original owned channel or 255 |
| 8-10 | Three owned notes or 255 |
| 11-13 | Current desired triad or 255 |
| 14 | Pending cleanup or chord update |
| 15 | Engine transport readiness |

Queue pressure must be distinguishable from musical resolution failure. The
snapshot and browser rendering must not describe transport readiness or owned
notes as proof of physical receiver delivery.

## Actual browser MIDI routing

The current browser synthetic ownership snapshots are insufficient for this
target. Add actual WebMIDI output selection and a bounded acknowledged bridge
from the production worklet/WASM producer to the selected output.

The router obtains MIDI access without SysEx, handles unavailable API, permission
denial and no outputs, and requires explicit output selection. Do not persist
device identifiers in the firmware project or automatically activate a route
after reopen. Permission/access and device state are defined by the
[W3C Web MIDI specification](https://www.w3.org/TR/webmidi/).

Use a browser-only bounded WASM output ring and one bounded in-flight worklet
batch, each no larger than 64 packets. Number batches and acknowledge the
accepted prefix. On a partial `MIDIOutput.send` error, retain the unsent suffix
instead of blindly replaying already accepted attacks. Preserve pending cleanup
against the original output ID/route generation. Output switching requires
stopped playback and accepted cleanup before activating the new route.

Disconnect disables readiness and future attacks; retain owed cleanup for that
output's reconnection. Closing/error/page-hide cleanup is attempted and reported,
but cannot prove delivery after a device disappears. Route state includes pending
cleanup and the selected original port, not just a global ready boolean.

Expose output selection, note-output enable, pad channel/root/range, current
chord, owned notes and pending cleanup. Measure browser scheduling/arrival
jitter and audio/MIDI alignment explicitly; MessagePort timing or successful
API calls do not establish sample-accurate physical timing. No MIDI receiver
or internal-pad audio acceptance is inferred from synthetic browser tests.

## Required acceptance

| Layer | Evidence |
|---|---|
| Native harmony | C-Am-F-G; all supported keys/modes/degrees; diminished triads; pentatonic parent rules; deterministic hint/ties; range edges; complete-voicing failure; degree zero |
| Native timing | Internal/external clocks, clock output off, queued/loop/repeat boundaries, same-sample bass events, unchanged-chord sustain; unchanged sequencer/pattern/bass/RNG traces |
| Native producer | Atomic 3/6-packet free-capacity and wrap readback; rejected On/Off ownership; newest-chord retries; stop/reset/load/reinit/disconnect cleanup; note zero; collision handling |
| Persistence | Schema-2/3 migration disabled, schema-4 same-size roundtrip, undo/redo, invalid and torn save protection without collateral writes |
| Actual WASM | Production worklet commands/snapshots/persistence/chord packets/cleanup; disabled-pad PCM parity with independently built pinned accepted bass firmware |
| Browser router | Fake-output acknowledgment/readback, full queue, partial send error, rejected cleanup, original-output routing, disconnect/reconnect/switch, unavailable API and permission denial |
| Physical | Real receiver chord/velocity/channel readback, stuck-note checks, musical timing, FM1 CPU/stack/DMA/save/reboot acceptance |

All local tests and routing UI must retain the explicit physical-receiver pending
boundary when no real MIDI instrument/device is connected.

## Resource gate and read-only recovery shortlist

The measured bass target is image 581524/581564 (40 flash bytes spare), ordinary
RAM 97864/98304 (440 spare), and pool 335712/344064 (160 above the unchanged
8192-byte reserve). These values come from the current local artifact ledger.
Forty bytes cannot accommodate the full accompaniment implementation. Flash
recovery must precede integration; retain every existing limit and original gate.
Do not shrink queues, history, samples, delay, fonts or reserves to make it fit.

Read-only artifact identity: disassembly SHA256
`48f8d508d1ac83d5a657791a71c6711cb0324e15bc8ea817f74c41b0a51b7fb8`,
recorded build source `ad435ef34c60f5aa53d5ff07abfbadd71b10257ba8553a4d6d653ee34f7a63ab`.
The following are experiments to measure, not proven savings or approved changes:

1. Share the cold `ui.c:put_s` body with targeted outlining. The source has 169
   call expressions after excluding its definition; no standalone `put_s` label
   is emitted. `draw_main` alone contains 14166 instruction bytes. Start with
   this narrow helper instead of rewriting page drawing. Compare exact official
   image/disassembly, unchanged gates, saved/UI behavior and framebuffer pixels.
   Calls can cost more than compiler-expanded short constant strings, so retain
   the change only if the official image actually shrinks.
2. Outline `sequencer.c:fire_step` as a single shared body across the two existing
   constant-specialized advance paths. No standalone body is currently emitted;
   legacy/arrangement callers contain 2880/2532 instruction bytes. Preserve the
   arrangement argument, callback order, step carry, mutation and clock logic.
   Measure the additional branch/call/frame/helper cost alongside unchanged
   gates and native/WASM traces. `do_stop`/`restart` are secondary individually
   measured candidates, not grounds to merge both clock paths wholesale.
3. If those fail to create sufficient room, examine duplicated 909 sample-voice
   bodies while retaining separate bypass/processed entry policies. Current
   `d9_render`/`d9_render_strips` contain 9384/9880 instruction bytes and force-inline
   the voice helpers. Test narrowly outlining the repeated sample voice, not
   per-sample `d9_emit` or a wholesale mixed renderer rewrite. This is the highest
   timing risk: its forced inlining currently removes known-null strip work.
   Exact independent PCM, noise/filter warm-up, tails, pan/send, all-strip and
   additional helper-cost evidence are mandatory; reject any failing gate.

Instruction-byte figures exclude jump-table data, are not savings predictions, and are not cycle totals.
No optimization, build, runtime or test change was made for this planning pass.
Browser-only routing buffers must remain excluded from the physical target.
