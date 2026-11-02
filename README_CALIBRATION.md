# Private calibration utility and firmware candidate (0.1.30-calbeta)

Authorized: a separate candidate and local tests only. No device commands, installation, repository push, or publication are part of this work. This source derives from the frozen 0.1.29-timefix archive; that candidate and the installed 0.1.28-clockfix are preserved.

## Setting and storage

This candidate defaults to nominal 44,100 Hz. The runtime physical timing rate is bounded to 43,900–44,300 Hz and used together by the sequencer and USB MIDI deadlines. DSP design rates remain unchanged; tempo-synced FX alignment is a separate unqualified check.

Calibration belongs to the unit, not a song: its versioned CRC record extends the existing A/B panel object, leaving the first 32 bytes of panel mapping intact. There is no new flash allocation. Project load, Undo/Redo, project backup/restore, factory project reset, and strip defaults do not carry this setting. Re-teaching panel controls preserves a valid timing extension. Older firmware can read the mapping prefix but ignores timing calibration; re-teaching the panel on older firmware may discard the extension.

Apply/Reset require idle transport, no staged/queued transition, an empty TX queue, and a matching prior rate/generation/unit tag. A transport hold prevents mixed-rate playback during storage; a new user PLAY request arriving during that hold is retained until the transaction ends. Successful writes are read back before the active rate changes. Busy, stale, malformed, out-of-bounds, damaged, and future records are refused. No write is automatically retried. A failed/uncertain save requires inspection and may need a manual stopped reboot to reconcile A/B storage; this utility never reboots the unit.

The unit tag is a random local calibration identifier created on first Apply, not a hardware serial number or MIDI channel. Before that first binding, the operator must identify the measured unit and select its exact ports. Reset returns to 44,100 Hz while retaining the unit tag and advancing the record generation. Repeated unchanged operations avoid another flash write. Unknown/corrupt records are not erased by Reset; preserve them for review.

## Utility

`tools/fm1_calibrate.py` is a Python command-line utility. Offline analysis needs NumPy and SciPy. Live commands additionally need mido and python-rtmidi. Dependency names are listed in `tools/calibration-requirements.txt`; none were installed by this work. Use the existing configured environment or a separate environment of your choosing. `--help`, analysis, and verification do not open MIDI ports.

The following is a future operator workflow, **not authorization to install or connect now**:

First, `python tools/fm1_calibrate.py ports` lists MIDI input/output names without opening ports. Select the measured FM1, not the audio-interface MIDI or a virtual loopback.

1. Once this firmware is separately installed and qualified for device access, stop transport and read a snapshot:

```
python tools/fm1_calibrate.py status --input-port "YOUR FM1 INPUT" --output-port "YOUR FM1 OUTPUT" --out before.json
```

2. Record at least two minutes (preferably five) of isolated, evenly spaced quarter-note kicks at a fixed BPM. Use no swing, variation, other voices, delay or modulation. Keep recording levels below clipping. Record through a stable interface clock, export PCM WAV without stretching, and retain the original. The utility does not create a calibration pattern or start playback. Stop both transports afterward.

3. Analyze the WAV, supplying the `active_hz` from the snapshot:

```
python tools/fm1_calibrate.py analyze control.wav --bpm 134 --current-rate 44100 --out proposal.json
```

The utility validates duration, repeatability, interval continuity, and fit residuals. It reports measured BPM and ppm error, proposes the nearest bounded integer-Hz rate, and hashes the proposal. This is calibration relative to the recording interface, not an absolute frequency certification. Already-passing measurements do not need Apply.

4. Review the proposal. Explicitly Apply while stopped, using the same exact ports and snapshot:

```
python tools/fm1_calibrate.py apply --input-port "YOUR FM1 INPUT" --output-port "YOUR FM1 OUTPUT" --snapshot before.json --proposal proposal.json --receipt apply.json
```

The utility rechecks the snapshot, reserves a new receipt file, sends one guarded mutation request, validates its acknowledgement, and queries the setting again. A lost acknowledgement is marked uncertain and is never retried automatically. Receipts record attempted requests, prior state, acknowledgement, and verification. Existing output/receipt files are never overwritten, except that the utility completes the new receipt it created for its own operation.

5. Repeat the independent recording and verify the result. Supply the new active rate:

```
python tools/fm1_calibrate.py verify corrected.wav --bpm 134 --current-rate 44120 --out verification.json
```

The default verification limit is 25 ppm, explicitly shown in the report. The command returns failure outside that tolerance, without changing the device. A setting readback match is not timing verification or a physical power-cycle test.

6. To Reset, first save a fresh status snapshot, then explicitly request Reset:

```
python tools/fm1_calibrate.py reset --input-port "YOUR FM1 INPUT" --output-port "YOUR FM1 OUTPUT" --snapshot latest.json --receipt reset.json
```

These live commands send only command 52. They never send CC, notes, transport, update, sample, project-save, factory-reset, or debug-register commands. Older firmware without this ABI will time out; no fallback write is attempted.

## Qualification

`tools/calibration_core_check.sh` exercises the real runtime setting and protocol with synthetic storage, strict ASAN/UBSAN, actual A/B flash logic, power cuts, and the runtime timing matrix. `tests/test_calibration_utility.py` checks the utility against the compiled C handler over a simulated pipe. It never opens MIDI ports. `tools/calibration_full_check.sh` runs fresh nominal host/WASM and inherited MIDI/application regressions; `tools/calibration_target_check.sh` builds the generic target and checks the existing budgets. Compiler, SDK, and oracle-history dependencies remain as documented for the parent.

Physical Apply/Reset, analog drift, USB output timing, reboot persistence on actual NOR, panel interaction, audio, tempo-synced FX, different units, and temperature/long-run behavior remain open. This candidate is not release approval.
