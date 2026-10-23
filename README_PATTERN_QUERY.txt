READ-ONLY PATTERN QUERY / EXPERIMENTAL 0.1.9-query
This is the source corresponding to the released FM-1_9000109 firmware.
Read current RAM, including unsaved edits, with command 49. No save/export fallback.
Use tools/fm1_pattern_read.py read --patterns 5 7 --output before.json
Repeat after a panel edit into a different filename, then:
tools/fm1_pattern_read.py compare before.json after.json
Run these scripts with Python 3. Reading needs mido and python-rtmidi.
Stop playback, release keys/buttons and finish edits before reading.
CRC/coherence errors discard the scan; existing autosave remains independent.
See docs/PATTERN_QUERY_PROTOCOL.md for the wire schema and read-only contract.
Physical 303 A-only and 808-kick-only copy/Undo checks passed on one FM1.
Broader receiver timing, other individual scopes and heavy-load tests remain separate.
