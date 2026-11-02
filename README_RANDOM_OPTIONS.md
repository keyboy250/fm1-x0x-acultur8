# Random length and selected note: 0.1.17-randombeta candidate

Separate private child of frozen 0.1.16. No FM1 installation or publication.

On either 303, the CONSTRAINED ARP page now has **NOTES**, **BLOCK**, **STEPS**,
and **STEP**. Existing octave and octave-range knobs remain on SCALE and GENERATE.

* **STEPS = KEEP or 1–64:** with STEP ALL, generate a complete line at that
  length. Examples: 8, 16, 32. KEEP preserves the existing generator behavior.
* **STEP = ALL or 1–64:** choose one existing NOTE step to randomize its pitch
  only, within generator scale, project key/scale, selected block chord, their
  intersection, or FREE mode. It picks a different legal pitch when possible.
  Existing transpose is accounted for. Timing, length, seed, all flags
  (including accent/slide) and every other step stay byte-identical.

For a single note, STEPS does not resize the line: the chosen step must already
be inside its active length and contain NOTE. REST/TIE steps refuse instead of
silently changing their gate. If no alternative pitch exists, it is a true no-op.
Normal playback harmony-follow may still project stored notes afterwards.

These are deliberate stored-note edits, separate from the existing temporary
VARIATION recipe. They require stopped playback, OCT+ fresh seed or OCT- repeat
seed, then SEL confirmation/HOME Cancel. Whole-line requests show their length;
single-note requests show their step number. Undo/Redo and normal save/autosave
apply. Select another pattern first when preserving an original base is desired.
Seed repeat is deterministic for the same input pattern; a single-note action
excludes the current pitch, so repeating it on the changed note can choose again.

Explicit whole-line length requests preserve existing accent flags and inactive
tail steps, destination sounds and sound motion, swing, other parts/patterns,
rate/direction/transpose. Only selected notes/gates, seed and selected length
change. Existing KEEP/ALL generation retains its prior density/articulation
behavior. Selectors are shared transient UI state, reset to KEEP/ALL at boot.

For drums, the extra **RANDOM SIZE** ARP page supplies STEPS for the selected
track. Existing drum position restriction is still on the first GENERATE page.
Length changes affect only that lane in LAN1/LAN2. A legacy shared-kit length
mismatch refuses with USE POLY FOR TRACK LENGTH, protecting other drum rhythms.
Single-step chord/scale pitch randomization is a 303 feature.

No new MIDI binding or project schema. Frozen capture, live variation, copy and
HOME startup remain inherited. Local tests are distinct from physical device,
audio/listening, timing and external MIDI receiver acceptance.
