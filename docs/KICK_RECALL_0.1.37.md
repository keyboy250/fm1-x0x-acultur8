# Private 0.1.37-kickrecall confirmation fix

Parent: installed 0.1.36-kickpresets / FM-1_9000136. Candidate identity: FM-1_9000137.

Physical reproduction: WAVE MIX was changed from 100 to 50, preset 1 selected, RECALL KICK confirmed with SELECT, and the dialog closed. Readback still showed 50. The other 38 hybrid settings, original BD controls, strip and stored preset were unchanged.

The panel input loop handles SELECT on release before replacing its previous button snapshot. The kick preset helper therefore saw SELECT as still held and refused the action. A previous synthetic test called button_tap directly with ui.btn clear and missed this sequence. The new regression drives the actual input loop with button-down and button-up states. It reproduces the old .36 failure.

The fix retires only the stale SELECT bit for native STORE/RECALL confirmation, only after all physical buttons and keys are released. All existing transport, owner, protected-project, queue and sample-identity guards remain. Programmatic/MIDI calls retain their strict owner checks. No DSP, sample, project format, preset-bank format or CC mapping changes.

CC map remains draft-2-kick-presets-1 (numeric 2, KP01). Project format 7 and independent preset/default bank schema 2 are unchanged from .36. The .36 rollback reads the same saved objects.

Positive regression covers actual press/release store and recall, cancellation, whole-project isolation, one-step HOME+REC Undo and HOME+PLAY Redo, held-button/key refusal, strict remote ownership and preset preservation. Local tests do not establish the final hardware fix until the installed device's confirmation is repeated.

Before a private update, preserve the current 50% trial project separately from favourite preset 1 (100%). Verify fresh project exports, all 25 strip settings, calibration, P8/P9 and the native preset record. After installation, verify those same values before another manual recall. No playback or volume messages are needed for this test. No publication is authorized.
