# X0X command49: read-only pattern pages, ABI 1

This candidate-only command reads the current project RAM, including unsaved
edits. It never uses the backup/export path, saves or changes project/UI state.
The currently installed predecessor does not support it.

## Request and reply

Request: `F0 7D 46 4C 31 01 token pattern section indexLo indexHi F7`.
The command is decimal49 / hex31. Pattern is zero-based0..31. Token is0..127.
Index is two little-endian7-bit digits; all requests must have exactly6 args.

Reply: `F0 7D 46 4C 31 01 token pattern section indexLo indexHi status ... crc[5] F7`.
Status0 is success,1 invalid arguments,2 busy/raced,3 unsupported request ABI.
Errors contain no page data. A success appends four unsigned32-bit numbers,
each in five little-endian7-bit digits: project edit revision, engine queue
write ticket, transport epoch, and project FNV-1a fingerprint. These are followed
by raw payload length (two7-bit digits) and pack7 payload. Pack7 has one high-bit
mask followed by up to seven low7-bit bytes per group. Unused high mask bits are
zero. Every reply ends with standard zlib CRC32 of the MIDI data bytes from7D
through the last payload byte (or status on error), excluding F0, CRC and F7.
The CRC is encoded as five little-endian7-bit digits, constrained to32 bits.
Maximum reply is205 bytes within the pre-existing288-byte reply buffer.
No pointers, target addresses, C struct padding or runtime DSP data are sent.

## Pages

| Section | Index | Raw payload |
| --- | --- | --- |
|0 summary|0|4 bytes: format0 legacy/1 initialized independent/2 expanded; swing raw0..100; dirty0/1; matching motion count0..160|
|1 lane|0..24|Lane fields below|
|2 motion|0..count-1|68 bytes: part,target,voice,parameter, then64 exact motion values including255|

Motion indexes enumerate used entries for the requested pattern in pool order,
not arbitrary memory addresses. Page indexes128..159 require the second digit.
Motion generation/padding bytes are not musical data and are not exported.

Lane0..10 is909,11..21 is808,22 is303A,23 is303B,24 isBREAK. Every lane begins
with length and rate. Drum pages are18 bytes: two timing bytes,8 hit-mask bytes
then8 shared kit accent-mask bytes, each low-order step first. Bass pages are144
bytes: length, rate, direction, transpose(raw,24=zero); generator density,
accent, slide, octave range, root, scale, base octave, mutate-bars; seed32 as
four little-endian bytes; then64(note,flags) byte pairs. Gates occupy flags0..1,
note accent bit2, slide bit3. BREAK pages are144 bytes: timing, slots A/B,
12 generator settings in existing BRK_* order, then64(gate,slice) pairs.
Legacy BREAK has implicit16-step1/16 timing,32 stored gates and16 stored slice
choices; unavailable tail fields normalize to zero. Independent formats use
the existing canonical packed lane/BREAK getters, including steps33..64.

## Coherence and nonmutation

Only main context serves pages. Queries refuse active/pending transport,
record/held input/performance/edit/save activity and undrained queues. They do
not reserve transport, consume queued commands, flush dirty state, commit Undo,
select a part/pattern/page, change sound, or hold/reset existing autosave.
Each page checks pre/post revision, ticket, epoch and project fingerprint;
a raced page is reduced to a BUSY error. Clients compare all four stamp values
across all pages, reread the summary after each pattern, and bracket the whole
multi-pattern scan. A mixed/partial scan must never be reported as accepted.
Each request is independent: there is no allocated or persisted snapshot session.
Existing autosave is independent; a query neither triggers nor postpones it.

`tools/fm1_pattern_read.py read --patterns 5 7 --output snapshot.json` converts
human pattern numbers1..32 and fetches every lane and matching motion record.
The tool sends only command49. It does not silently fall back to project export.
`compare before.json after.json` compares complete queried musical data, ignores
scan stamps/dirty state, and separately reports a whole-project fingerprint
change so equality is not overclaimed for patterns outside the requested set.
