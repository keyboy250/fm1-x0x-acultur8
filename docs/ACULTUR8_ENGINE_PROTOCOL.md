# Shared control and project contract (ABI 1)

This foundation connects the host, browser and future device editor to one C command handler. It does not enable the sound-strip DSP or claim device qualification. Existing engine target IDs, sound storage and project-prefix offsets remain stable.

## Commands and acknowledgements

`variant_command()` accepts a fixed-width ABI/version, operation, request ID, target/voice/index and integer value. Replies echo the request ID, return an explicit result and expose the engine queue ticket. Sound and strip edits return `QUEUED` only after the bounded engine queue accepts them; a full queue returns `BUSY` without changing the project mirror, revision or undo state. Invalid versions, addresses and values return errors before mutation. Controls use integer pot values for existing sound parameters and physical Hz/dB/ms for strip settings.

The main loop is the only producer; the audio ISR consumes commands. Browser calls execute in the AudioWorklet's serialized event/render context. Queue tickets use unsigned 32-bit counters; compare progress by signed subtraction within half the counter range. An accepted ticket means queued; the applied counter identifies commands consumed at an audio boundary. DSP smoothing and gain-reduction measurements are distinct from desired settings.

Bounded snapshots report capabilities, revision, dirty/save-protection state, queue progress and one sound voice or strip. They do not expose project pointers or allocate a second project copy. ABI 1 advertises strip configuration separately from audible strip processing. Browser and device transports must check capabilities and must not label configured processing as active before DSP support lands.

The binary command is 16 bytes (`u16 abi/op`, `u32 request`, four `u8 target/voice/index/reserved`, `i32 value`); the 16-byte reply is `u32 request/revision/ticket`, `i32 result`. A snapshot is 80 bytes: four `u32 abi/section/revision/count` followed by at most sixteen `u32` values. Supported platforms use little-endian integers. Revision counts accepted project edits; it is not a storage generation or DSP meter counter.

| Snapshot section | Values |
| --- | --- |
| `VS_INFO = 0` | capabilities, dirty, writeProtected, queueWritten, queueRead, extensionSchema, stripCount |
| `VS_SOUND = 1` | One existing target/voice's pot values; count is its parameter count |
| `VS_STRIP = 2` | high-pass Hz, low-pass Hz, boost dB, ceiling attenuation dB, release ms, flags |

Capability bits are sound controls `1`, strip configuration `2`, undo `4`, save `8`. Results are OK `0`, QUEUED `1`, INVALID `-1`, UNSUPPORTED `-2`, BUSY `-3`, PROTECTED `-4`, FULL `-5`, IO `-6`. A protected project refuses editor commands until a supported load or explicit factory reset; the original stored objects remain available for recovery. Operation values are SET_SOUND `1`, SET_STRIP `2`, BEGIN_EDIT `3`, END_EDIT `4`, UNDO `5`, REDO `6`, SAVE `7`.

Explicit edit begin/end groups one gesture into one existing undo entry, even across idle pauses. Repeated begin is idempotent. The transport must send end on pointer cancellation, blur or disconnection; undo, save and reset also end the gesture. Autosave waits while an explicit gesture is open. Undo/redo and save use the same application path as physical controls. Saving while playback or its stop request is active returns `BUSY`; callers retry after stop. A rejected project load protects its stored bytes from later autosave until an explicit factory reset or successful supported load.

## Browser transport

Send `{type:'variant-command', abi:1, op, request, target, voice, index, value}` to the existing worklet port. Every field is required: ABI/operation are unsigned 16-bit integers, request is unsigned 32-bit, target/voice/index are unsigned 8-bit, and value is signed 32-bit. JavaScript validates these ranges before calling WASM; the C export separately rejects fields outside their pre-cast ranges. Unsupported in-range ABI versions return `UNSUPPORTED`. Replies are `{type:'variant-reply', request, result, revision, ticket}`. A transport validation rejection has result `INVALID` and zero progress metadata; an engine or direct C rejection reports its current revision/ticket. Only accepted commands change desired settings. Do not treat a rejected request's ticket as a new accepted edit.

`{type:'variant-select', strip:0..24}` selects the strip included with the next ordinary frame. Invalid selections retain the previous selection. Frames include `variant:{info,strip,settings}`; each snapshot is `{abi,section,revision,values}`. `info.values` follows the `VS_INFO` row above and `settings.values` follows `VS_STRIP`. These are copied bounded arrays; subsequent snapshots cannot mutate a previous message. Compare `info.values[4]` (queueRead) with an accepted reply ticket before showing the edit as applied. Capability `2` means persisted strip configuration, not audible processing. The direct `web_variant_snapshot(section,target,voice)` export also supports sound snapshots; invalid addresses or unsupported sections return a null pointer.

Run `node tests/host/variant_wasm_test.mjs path/to/x0x.wasm` against a fresh emulator build. This uses the production worklet and real shared C handler, fills the 1,024-entry queue without rendering, verifies accepted/applied progress and rejected no-ops, groups a gesture across idle pauses, saves/reopens variant settings, checks playback/stop backpressure and corrupted-store protection. The existing marked-player, sync/mixing and sample-import tests remain separate regression gates. Capacity refusal and interrupted device writes require the native/storage and physical acceptance checks below; the browser worklet harness does not establish hardware flash behavior.

## Variant settings and storage

The extension is appended after the existing arrangement in `project_t`: an 8-byte header (magic, schema, length) and 25 eight-byte sound-strip configurations, 208 bytes total. Strip order is 909 voices 0–10, 808 voices 0–10, 303 A, 303 B, break. Each configuration stores high-pass Hz, low-pass Hz, boost dB, ceiling attenuation dB, release milliseconds and flags. Defaults are 20 Hz / 20000 Hz / 0 dB / 1 dB / 80 ms / flags 0 (exact bypass). DSP state and meters are never saved.

Existing packed format 2 keeps its exact legacy raw length when variant settings are at canonical defaults. Format 3 adds the extension to that same legacy prefix. Both formats carry bounded raw/packed lengths and a CRC. The reader validates decompression, complete input consumption, CRC, schema, exact extension length and value ranges before copying a candidate into the live project. Existing format-1 migration remains supported. Future extension schema versions require explicit migration functions; they must not reinterpret schema-1 bytes or silently discard unknown sections. A later schema may add typed, length-delimited arrangement/harmony sections while migrating these 25 strip records unchanged.

The existing 69,120-byte graphics canvas supplies bounded temporary load staging while stopped, avoiding another ~30 KiB RAM allocation. Load and drawing execute synchronously on the main loop; neither audio interrupts nor the second core accesses this canvas. Before borrowing it, `lcd_sync()` completes its outstanding pixel DMA. A recorded hardware LCD timeout refuses the borrow because DMA source release is uncertain. Boot loading occurs after LCD initialization and before UI initialization. Every subsequent drawing pass begins with a fresh canvas, and finishing a candidate invalidates all display hashes and outlines so its bytes cannot be sent as pixels. Failed validation leaves the live project, pending edit, undo shadow and complete undo journal unchanged, including redo entries and oversized pending edits. Successful load resets undo to the loaded state. These source lifetime guards do not establish physical DMA timing acceptance. Stream capacity is measured before any flash-object write, so capacity refusal leaves all stored objects untouched.

New firmware imports supported legacy projects; older upstream firmware cannot interpret nondefault format-3 variant saves. Keep a legacy backup for downgrade. Defaults retain the original format-2 save representation and audio path.

## Required checks

Legacy format-1/2 import; default format-2 output; variant format-3 round trip; unknown schema/format, malformed lengths, truncated input and bad CRC rejection without live-project mutation; no writes on capacity refusal; extension undo/redo; invalid command no-ops; queue saturation with explicit backpressure and eventual drain; unchanged legacy audio/reference tests. Target image/RAM/pool gates and physical playback acceptance remain separate. The extension costs 208 bytes in the project and another 208 in its undo shadow; runtime queue/snapshot/DSP state must be counted separately.

## Integrated target checkpoint — 2026-10-13

The configuration-only foundation with font packing builds using the pinned official compiler/SDK: image565,204/581,564 bytes, ordinary RAM97,636/98,304, pool334,532/344,064. This leaves16,360 image bytes,668 ordinary RAM bytes and1,340 pool bytes beyond the required8KiB reserve. All15 existing static target budgets pass. The standalone strip is not yet called by the engine. Its updated object has2,564 bytes of function text, a68-byte state and an84-byte process frame. See [recorded evidence](../tests/variant_foundation_evidence.json). Physical timing/flash acceptance and future combined feature sizing remain pending.

## Read-only USB-MIDI strip query (X0X command48)

The physical device implements a separate versioned query. It uses the existing
editor SysEx framing and reply buffer; it adds no parameter-write, save, apply,
watch, streaming or update operation. It is available after normal startup.

Request: `F0 7D 46 4C 30 01 token strip F7` (hexadecimal command30 = decimal48).
`token` is0..127; `strip` is0..24. Send exactly these three arguments after the
command. Allow only one outstanding query; clients correlate token and strip.
Strip indices are909 voices0..10,808 voices11..21,303A22,303B23,BREAK24.

Every matching query replies with `F0 7D 46 4C 30 01 token strip status ... F7`.
Status0 is success,1 invalid arguments,2 engine edit queue busy,3 unsupported
request version. Error replies have no data (10 bytes including F0/F7).

Success replies are31 bytes including F0/F7. After status they contain:

| Field | Seven-bit bytes, least significant first |
| --- | --- |
| Project revision |5; highest byte0..15 |
| Consumed engine queue ticket |5; highest byte0..15 |
| High-pass Hz |3;20..20000 |
| Low-pass Hz |3;20..20000 |
| Boost dB |1;0..24 |
| Ceiling attenuation dB |1;0..48, where1 means minus1dBFS |
| Release ms |2;10..250 |
| Flags |1; bit0 overall strip, bit1 HP, bit2 LP, bit3 limiter |

All payload bytes are seven-bit safe. Frequencies use three bytes because the
supported range exceeds MIDI's two-byte16383 limit. Values come from the main
thread's current project configuration, after verifying that the audio command
queue is drained. They are not a read of saved-default storage or of mutable
DSP state, meters, smoothing progress, measured cutoff response or audible output.
A busy reply contains no settings; retry it with a fresh token. Queries do not
stop playback, select a sound, write the queue, mark dirty, create undo entries,
apply defaults or save anything. Normal unrelated application activity, including
a previously pending autosave, remains independent of the query.

`python tools/fm1_strip_read.py --strip 23` prints exact303B values as JSON.
`--all` reads all25 sounds sequentially and refuses a mixed-revision/ticket scan.
`--port` and optional `--output-port` disambiguate multiple devices; Windows
numeric input/output suffixes may differ. The tool sends command48 only. It does
not fall back to CC, sample, debug, save or firmware-update commands. Earlier
firmware does not answer this query; a timeout must not be reported as a zero
value or a passed readback check.


## Candidate command49: unsaved pattern readback

The separate query candidate adds bounded, CRC-protected RAM pages for all32
patterns, all25 lanes, shared accents/swing and pattern motion/probability.
It never calls the project backup/export flush or a save/setter/transport action.
See [the complete ABI and nonmutation contract](PATTERN_QUERY_PROTOCOL.md) and
`tools/fm1_pattern_read.py`. The current physical 0.1.8 predecessor does not
support command49; local query tests are not device acceptance.
