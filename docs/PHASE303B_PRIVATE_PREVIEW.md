# 0.1.33 PHASE hardware preview

Private prototype for musical evaluation on the physical FM1 first. It is not a
public release. The public 0.1.32 firmware and emulator remain separate.

Select 303B, open EDIT, and use the first page, **303B ENGINE**. With playback
stopped and 303B notes released, K1 chooses the original **303** or **PHASE**.
K2 selects SAW, SQUARE, PULSE, DBL SINE, SAW PLS, RES SAW, RES TRI or RES TRAP.
K3 sets phase shape (0–127). K4 sets the colour-envelope depth (0–15).

This is monophonic phase distortion using the MIT-licensed eight-wave oscillator
core from Felucca-Engines. It does not contain that firmware's complete voice,
modulation or folder model. A short attack, release, colour envelope and slide
are provided by the FM1 adapter. PHASE has no separate MIDI CC mapping yet.
Original 303 CC values remain in its original parameter bank. Common channel
strip filters, limiter, pan, sends and sequencer routing still apply.

303A stays on the original engine. 303B defaults to its original engine and
retains its twelve original sound parameters when PHASE is selected. Returning
to 303 reconstructs the original DSP from those parameters. Existing note
patterns, global Straight mode and per-drum triplet overrides are retained.

The two previously reserved song-header bytes store PHASE controls under project
schema6. Untouched older projects remain schema5. Editing a PHASE control opts
that project into schema6, even if the engine is later switched back to 303.
The ordinary Save/Undo/Redo mechanisms include these settings. Older 0.1.32
firmware refuses a schema6 save and protects it from overwrite; reverting the
firmware therefore requires the matching pre-install project backup. Merely
selecting 303 does not downgrade the project.

Before installing: complete local qualification, freeze the package and source,
verify a 0.1.32 rollback package, obtain fresh stopped/Master-down/Reason-closed
readiness, and take repeated matching device backups. Hardware testing proceeds
one observed action at a time. Host timing and automated builds cannot establish
device audio deadlines, listening quality, physical MIDI or power-cycle retention.

Attribution: Leo Kuroshita (@kurogedelic), Hügelton Instruments, MIT.
See LICENSES/phase-engine-MIT.txt and the source header for the pinned blob.
