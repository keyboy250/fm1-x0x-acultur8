# 5. Build a complex pattern and measure it honestly

[All tutorials](README.md) · [Full FAQ](../FAQ.md) · [Comparison and limits](../FEATURE_COMPARISON.md)

Use your own backed-up project. A complex pattern with both 303 generators mutating can reveal workload behavior that a fixed pattern does not. Keep the starting settings and mutated result separate; do not restore old notes over newer user work silently.

1. Select the intended pattern while stopped with MASTER down. Open PERFORMANCE and record the exact cumulative DROPOUTS and GUARD counts before playback.
2. Use your own timer. Run a bounded window without MIDI readback/query traffic if the purpose is to separate playback workload from query effects.
3. If a counter rises, stop early and retain power. Otherwise note the counts while still playing, press STOP once, then note them again. This distinguishes the observed playback interval from a later STOP transition.
4. Keep MASTER down before diagnostic reads. Do not SAVE or reboot before capturing a retained event if diagnosis is the goal.
5. Record IRQ high-water, pattern, duration, core mode and whether guards/stalls were enabled. A stopped LOAD/PEAK display is not a retained measurement of the earlier playing workload.

Guard engagements are not the same counter as dropouts. Zero dropouts with new guards is a different result from zero of both. Report cumulative counts and deltas clearly; “0,0” must not hide an existing cumulative count. The scoped .60 P11 result was 25 user-timed minutes with 0 dropouts and 0 guards on one unit. Empty retained records do not provide per-block timing or explain earlier unrecorded events.
