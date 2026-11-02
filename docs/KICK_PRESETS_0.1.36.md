# Hybrid kick projects, presets and MIDI — 0.1.36-kickpresets

Private FM1 candidate, identity FM-1_9000136. Map draft-2-kick-presets-1 (numeric map 2, kick identity KP01). Physical installation and DAW acceptance are recorded separately in the hardware packet.

## Keep your sounds

The project now saves all 39 hybrid controls: synth/wave blend, source, pitch/sweep, filter, velocity destinations, tiers/jitter, sample shaping, separate synth/wave envelopes and curves, and click tone/length/crunch/smooth. Ordinary project Save and existing autosave retain these settings. Loading a project restores them. Recall/Undo/Redo update the audio owner through its command queue.

The 808 BD KIT page adds KICK SLOT (1–16), STORE KICK and RECALL KICK. Select a slot, turn the action knob, then SELECT confirms and HOME cancels. Store/recall require stopped playback, released keys and a finished kick tail. Incoming kick triggers are ignored during the bounded preset transaction so they cannot interrupt the stored/recall state. Existing accents remain on knob 1. Overwriting a slot asks for confirmation on the device.

Each slot contains the 39 hybrid settings, the original ten 808 BD sound parameters and the kick strip (HP, LP, boost, limiter ceiling/release/flags). Slots are independent of project load and Undo. Recalling is one undoable project gesture; save the project to retain that recall. Shared kit/global FX remain part of the project. Native slots are numbered; portable JSON exports can have names. Stored jitter settings recall with seed 808; ongoing random history is not saved.

USER1 references the existing custom sample slot 1, zone 1. Its fingerprint is saved with the kick; it does not duplicate sample audio. Missing/changed audio refuses a preset recall before changing the project. Project load retains its saved reference and disables a mismatching custom layer with a warning. Built-in SINE/WARM/HOLLOW/EDGE remain available. Sample upload is a separate existing workflow.

## Velocity

LOW/MID/HIGH defaults are 65/90/110. With VEL TIERS enabled, ordinary input velocity chooses a tier. J CHANCE is the percentage of hits varied; J RANGE is the maximum plus/minus MIDI velocity values. For example 10% chance and range 10 occasionally changes a 110 hit by up to ±10. The final value is clamped to 1–127.

KICK RAM knob 4 is STEP VEL: hold a white step and choose AUTO, LOW, MID or HIGH. AUTO follows the existing step accent/input velocity and the VEL TIERS switch. An explicit tier chooses its named base even when automatic tiers are off. VEL TIERS remains in the SELECT parameter list. These choices save with the pattern and use the existing motion pool; a full pool refuses an additional row. Same-part kick/full-pattern copies carry the tier choices.

V CUTOFF, V PITCH, V DECAY and V BLEND are bipolar velocity modulation depths (raw 64 is zero). They use the resolved hit velocity. Tier/jitter is calculated once per sequenced hit and that same final velocity drives both the internal kick and outgoing MIDI. Other drums retain their previous behavior.

## MIDI CC and readback

All existing addressed controls retain their channel, CC, range and context. This map adds 46 addresses. The offline searchable KICK_CC_MAP.html and KICK_CC_MAP.csv list all 803 inventory rows; KICK_CC_COMPATIBILITY.json records the additive comparison against map draft-1-actions-prototype-1. Inform users of this map version when upgrading and keep the previous chart with older projects/controllers.

The 39 kick fields are IDs 1250–1288: channel 15 CC 18–31 and 64–88. STEP VEL is ID 1296, channel 15 CC 96, using the committed source-owned pattern/step context. Raw CC scales the parameter domain; the existing exact-value transaction addresses every stored value precisely. Context is MIDI-owned, independent of the open panel page.

Live control changes affect the next kick hit. The current tail retains its snapshot. Preset store/recall remain stopped-only. IDs 1290–1295 (channel 15 CC 90–95) provide store arm/execute, recall arm/execute and request low/high. Commit 808 kick context (part 1, kit 1, drum 0), destination slot 0–15 and both request chunks; arm, then an execute rising edge. Arms are source-owned and guarded by revision, project fingerprint and transport epoch. Cancel with zero on the arm control; use a new increasing request token for a new transaction. Repeated successful requests are idempotent. Detach clears source context and arms.

Existing read-only SysEx command 51 ABI 1 adds pages 11–19: capabilities; desired fields; audio-owned next-hit fields; step tiers; slot summaries; preset result; original BD/strip; raw slot record; current-hit fields. Indexed replies carry nonce, CRC and common revision/queue/transport/source stamps. Page 13 acknowledges next-hit intent; page 19 is the current/last hit snapshot. Neither implies a listening test or DAW automation acceptance.

`tools/fm1_kick_presets.py` reads the current sound or exports a named slot in format acultur8-kick-preset-1. It sends only queries, never notes, clock, save or CC. JSON contains the original ten sound parameters, six strip values, 39 hybrid values and a custom sample fingerprint reference. It does not contain sample audio.

## Compatibility and recovery

Untouched old projects retain their original format. Once a kick control or step tier is edited, project format 7 stores the extension. The first reusable preset upgrades the independent strip-default bank to schema 2 while retaining all 25 strip defaults. Earlier 0.1.35 firmware safely refuses these newer objects and must not be used to edit them. Keep the original .35 project backup and the .35 firmware package; rollback requires restoring the corresponding old project/default bank rather than resaving newer objects through older firmware.

The private hardware packet preserves a fresh byte-checked old project, strip and calibration readback, P8/P9 snapshots, exact source/package hashes and rollback. The manually captured sine kick also has an independent named JSON backup. Local tests cover save/reopen, slot independence, Undo/Redo, cancellation, interrupted writes, custom sample mismatch, MIDI ownership/queue refusal and identical captured audio against .35. Physical save/restart, incoming DAW automation, listening and device timing remain separate acceptance evidence.
