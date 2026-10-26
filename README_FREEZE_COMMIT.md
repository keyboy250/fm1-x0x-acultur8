# Freeze and Commit candidate: 0.1.16-freezebeta

Private, separate child of the frozen 0.1.15 HOME candidate. Local qualification
is separate from device installation, listening, timing and publication.

## Capture the lucky pass

ARM a selected drum lane or 303 A/B variation as usual. On its new final ARP
page, **FREEZE / STORE**, press **OCT+** during playback. The status becomes
WAIT. At the end of that part's current complete pass it becomes **FROZEN**.
The recorded notes/hits repeat, including probability/condition rejections as
rests. Final 303 pitches, ties and slides are captured; the source pattern is
untouched. A capture takes the complete current pass, not just the note sounding
at the moment of the button press. A late press in the next pass captures that
next pass. This is not an audio-loop recorder.

The first candidate supports FWD 303 lines only. REV, PING and RND refuse
Freeze explicitly: a map of step positions cannot faithfully capture their
played order. A 303 line starting with TIE also refuses: that open boundary can
depend on a previous pass's held pitch. All 22 drum lanes and 303 A/B share the existing single selected
variation target. It does not freeze every part together.

A stopped Freeze is accepted only just after a whole forward pass has been
observed. A partial/stale pass refuses. Starting again while a live request is
pending waits for a complete new pass. Changing the variation recipe, OFF or
re-ARM clears the capture. The STORE TO selector keeps it intact. Power-on
and project load clear it. Freeze has no Undo entry and does not save its recipe.
While frozen, the selected part bypasses fresh probability/condition draws and
303 live pitch-follow projection. Existing mute and Stop cleanup still apply.
Sounds, shared drum accent, stored 303 accents and swing are not rewritten.
Captured acceptance means the internal synth/drum trigger; external receiver
delivery and clock jitter are separate from this musical-pattern capture.
Other parts keep their own probability processing; their future random draws
are not promised to match a run without freezing the selected part.

## Choose where to store it

Stop playback. On **FREEZE / STORE**, KNOB1 chooses **STORE TO P1..P32**.
Press **OCT-**, review the part and destination, then SEL to confirm or HOME to
cancel. The destination must differ from the source slot. It stores only the
captured selected part, preserving destination sounds, sound automation, all
other parts, destination accent flags/rows, swing and inactive tail steps.

303 length/rate, forward direction and captured final pitches are stored with
zero transpose. Destination generator settings remain, except auto-mutate is
turned off so it cannot immediately rewrite the committed line. Only the chosen
destination's probability/condition automation is removed: its outcomes are now
in the stored note/rest gates. Source automation remains untouched. A single
drum row copies length/rate in poly mode; legacy shared kit timing stays intact.

Commit is an atomic, stopped, write-protection-aware transaction with Undo and
Redo. It marks the destination dirty under the normal project save policy:
press SAVE explicitly to retain it, and automatic saving still applies when
enabled. The frozen audition remains on its original source until OFF/re-ARM.
When playing the stored destination normally, its usual live harmony-follow,
other automation, mutes and swing apply. Preserving destination accent/swing
means that audition can differ from the original frozen source's expression.

## Local acceptance gates

Actual accepted event capture (including rejected probability NOTE/TIE), stable
repeat, both 303s/all four note constraints and all 22 drum lanes across legacy,
LAN1 and LAN2; complete/partial pass boundaries, FWD refusal, source and whole
project invariance until commit, destination-only exact diff, sound/AC/swing,
stopped/protected/stale/cancel/invalid-slot refusals, Undo/Redo, Save/reboot,
production WASM UI/playback, inherited regression suites and unchanged target
resource/static-cost gates. No device operations are part of local testing.

Run the focused native suite with `sh tests/host/run_freeze_commit.sh`. Build the
production emulator with `python3 lab/build_emulator.py --native`, then run
`node tests/host/freeze_commit_wasm_test.mjs build/emu/x0x.wasm`. The existing
`sh tools/acultur8_checks.sh` remains the full required regression suite. Target
qualification uses `tests/host/run_freeze_commit_target.sh` in the offline SDK
container. Receipts retain all final hashes and the inventory-coverage failure:
the resumed required prefix is accepted only with identical runtime/WASM bytes.
