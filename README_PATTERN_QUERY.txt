FM1 TRUE READ-ONLY PATTERN QUERY - SEPARATE LOCAL CANDIDATE
Parent: ../all-lane-copy-candidate-20261023 (private installed 0.1.8).
This candidate adds command49 and tools/fm1_pattern_read.py. It has not been
installed or published. The current FM1 and its unsaved P5/P7 test are untouched.

PURPOSE
Read the current project RAM, including unsaved notes, without triggering the
stopped backup/export path, which calls ui_flush() and can save dirty edits.
No generic memory peek, setter, CC, note, transport, restore or save command.

USAGE AFTER SEPARATE DEVICE QUALIFICATION/INSTALLATION
  python tools/fm1_pattern_read.py read --patterns 5 7 --output before.json
  [user makes a copy or performs Undo on the physical panel]
  python tools/fm1_pattern_read.py read --patterns 5 7 --output after.json
  python tools/fm1_pattern_read.py compare before.json after.json
Reading requires the existing mido/python-rtmidi runtime; file comparison uses
the Python standard library only. Snapshots are only written after a complete valid scan; existing files are not
overwritten. Compare reads local files and needs no MIDI connection. It compares
queried musical data, excluding dirty state/stamps, and separately reports if
the whole project fingerprint changed. Equality is not a claim about unqueried
patterns or full device state.

READBACK
All32 stored pattern slots; all11 drum rows on each kit; both303 sequences;
BREAK; pattern swing and both shared kit accent rows; per-lane length/rate;
303 notes/gates/ties/slides/note accents, direction/transpose/generator seed;
all pattern-owned motion and probability/condition lanes, including unset255.
Legacy, initialized independent-lane and expanded-rate formats are decoded.
The protocol is documented in docs/PATTERN_QUERY_PROTOCOL.md.

READ-ONLY CONTRACT
The command reads RAM in the ordinary main context. It never calls ui_flush,
project_save, transport reservation, any setter, dirty/history commit, page or
pattern selection, nor saves-hold controls. It reuses the existing reply buffer
and allocates no extra project or static buffer. Each page is CRC32 protected.
Reads return BUSY during playback, pending transport, recording, held keys or
buttons, performance testing, external edit, pending explicit save, save hold,
or an undrained engine queue. The query neither drains nor rearranges a queue.
A race while making a page discards its data and returns BUSY.

Every page has revision, queue ticket, transport epoch and project fingerprint.
The reader brackets every pattern and the whole multi-pattern scan, accepting
only same-stamp pages. Mixed scans are discarded; unsupported firmware or a
timeout is an error, never an empty pattern or a backup/export fallback.
Existing autosave operates independently: this command neither starts nor
postpones it, and does not change its settings/timestamps. Query activity is
limited to its incoming/outgoing protocol buffers; musical and UI state stays
unchanged. These are intent/implementation rules; completed local evidence is
recorded in PATTERN_QUERY_STATUS.json, not inferred as physical acceptance.

LOCAL TESTS
The actual C editor service is exercised with strict halt-on-error ASan/UBSan.
Tests compare full project, UI, sequence, Undo/shadow, revision, queue counters,
stored hashes and store-write counts around each request. They check all32
slots/25 lanes/3 formats, all25 copy and Undo snapshots, motion ordinal159,
invalid/truncated/version/busy requests and interleaved edit/transport races.
Python decodes actual C wire replies and drives the production client against
a fake MIDI peer that refuses any command except49. Mixed stamps, corruption,
partial files, timeouts and busy retries fail closed. Existing required engine,
strip query, production WASM copy and unchanged target-budget checks remain
separate proof layers. No listening, receiver or hardware timing claim.
