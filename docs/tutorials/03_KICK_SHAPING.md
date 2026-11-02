# 3. Shape the kick and check overlapping tails

[All tutorials](README.md) · [FAQ: stored values and milliseconds](../FAQ.md) · [Starter values](../STARTER_NATIVE_CONTROL_REFERENCE.txt)

Select the 808 kick. **KICK RAM** controls the hybrid layer, wave source/mix and step velocity selection; **WAVE SHAPE** and the later envelope pages are distinct groups. **SYNTH ENV** shapes the synth envelope; **WAVE ENV** shapes the added wave layer. Parameters are captured per hit, so a current tail and the next hit's intent need not be the same state.

## Raw values versus the native display

WAVE ENV's fourth knob is **W DECAY**. Stored raw 0–127 becomes approximately 5–3,000 ms; the display uses integer `5 + raw × 2995 / 127`. Raw 75 is displayed as **1,773 ms**. Raw 17 is **405 ms**, and the selected starter's raw 5 is **122 ms**. These values belong to different sounds/test fixtures; do not copy the test fixture into the starter by mistake. WAVE SHAPE also has a W DECAY control, with different meaning.

## A bounded overlap test

1. Capture or write down your current values and keep a compatible project/preset backup.
2. While stopped with MASTER down, use a test pattern with repeated kick hits and change only the intended WAVE ENV parameter. Leave W ENV enabled for this condition.
3. For context, quarter notes at 134 BPM are about 448 ms apart. A 1,773 ms wave envelope spans later hits; this was the scoped long-tail fixture.
4. Set a quiet level, play briefly, and listen for unexpected gaps, harsh glitches or abrupt tail cut-offs. A programmed click layer, velocity variation and the expected sound of overlapping envelopes are not automatically defects.
5. Stop, turn MASTER down, record DROPOUTS/GUARD, and restore the exact captured values. Do not SAVE a temporary fixture unintentionally.

## Filter and limiter control checks

On **STRIP FILTERS**, KNOB1 is HP and KNOB2 is LP; the other controls enable the corresponding filters. On **STRIP LIMITER**, KNOB1 is BOOST, KNOB2 is ceiling, KNOB3 is release, and KNOB4 is the limiter switch. These pages belong to the selected sound. Confirm selection and original values before a small sweep. Lowering BOOST for a check does not justify increasing MASTER. Return values exactly and verify before saving.
