# Timing calibration protocol — command 52, ABI 1

SysEx data (without F0/F7) begins `7D 46 4C 34` (52 decimal).

Query payload: `[1, token, 0]`.

Apply/Reset payload: `[1, token, op, rate:u35, expected_active_hz:u35, expected_generation:u35, expected_unit:u35, new_unit:u35, crc:u35]`. Each u35 is five low-first 7-bit bytes, restricted to unsigned 32-bit values (top byte <=15). Token is 0..127. Op1 applies a rate43900..44300; op2 resets with rate0. Query has3payload bytes; mutations have33payload bytes. Mutation CRC32 covers the four-byte header plus the first28payload bytes, excluding CRC. The header and exact length are checked before an operation.

Reply data: header, `[1, token, op, status, 10]`, ten u35 words, CRC u35. Exactly64data bytes /66bytes with F0/F7. CRC32 covers every data byte before the CRC.

Words: active_hz, stored_hz (0=nominal reset), generation, unit_tag, record_status, min_hz43900, max_hz44300, default_hz44100, busy, capabilities1.

Status:0success,1invalid/bounds,2busy/TX pending,3stale,4storage failure/uncertain,6damaged/future/exhausted record,7unsupported ABI. Every reply includes a current in-RAM snapshot; failed storage can mark record_status damaged until a stopped reboot reloads actual A/B storage. Do not automatically repeat a mutation after any failure or timeout.

Record status:0valid extension,1blank/missing,2legacy panel-only,3damaged/uncertain,4future/unsupported. Query never writes. New unit_tag is nonzero; after first binding it must match the stored tag. Reset retains the tag. Generation must match; successful changed writes increment it. Idempotent unchanged operations return success without a write. A unit tag is local metadata, not an authenticated hardware identity.

Persistent layout: existing OBJ_PANEL prefix32bytes; then six little-endian words: magic0x314C4354,version1,stored_rate,generation,unit_tag,CRC32. The CRC covers the first52bytes, including panel mapping. Total56bytes. Existing A/B object commit and payload CRC remain in force. Unknown length/version/magic is not rewritten. No project schema or CC mapping is changed.
