# Individual-lane additional static evidence

Regenerate from the existing exact target artifacts (no target build):

```powershell
python tests/lane_feature_costs.py build/x0x.dis --report tests/lane_feature_costs.json --source-sha256 ea711902a294594453ddcee7d654930cfc4eb7298111496d78980f963204ca0b --baseline-dis ../bass-integration-20261013/build/x0x.dis
python tests/test_lane_feature_costs.py
```

The original target_budget model and all15 limits remain unchanged. The
script rejects mismatched gate/disassembly identity, non-pass gates, missing
required functions and unknown instructions. It retains duplicate labels by
address, direct and tail edges, immutable-source callback alternatives and
one shortest root/helper chain per reachable pair. The report contains120
rows and108 chains, including model/row/transport producers and their callees.
Web-only command/snapshot exports absent from this physical target are listed
as unmeasured. Main command/snapshot bodies compiled out of this target are
also explicitly absent, rather than assigned fabricated zero costs.

Exact disassembly SHA256:
`d6cfdd362aee1a994bb6640bea03b545faa9b5598f115be01ee03b2f274cf63f`.
Baseline disassembly SHA256:
`918289836a90666600994f40409d47d35c64590689f35acfede915217cda94a8`.
The baseline is the existing pre-lane target artifact; its drain is inlined.
No separate baseline drain frame is inferred.

| Body | Instructions | Loop estimate | Divisions | Loop calls | Model cost | Prologue bytes |
|---|---:|---:|---:|---:|---:|---:|
| Baseline engine_render |953|775|2|65|1202|128|
| Current engine_render |611|447|0|44|732|124|
| Current drain |416|388|2|24|530|60|
| poly_musical_tick |90|62|0|3|62|52|
| row_fire |144|0|0|0|0|80|
| seq_advance_poly tail wrapper |1|0|0|0|0|0|

Drain remains once per outer render, before the event/audio loop, preserving
queue and MIDI dispatch order. Caller scores exclude outlined callee work.
The loop model assigns row_fire zero loop cost despite its144 instructions;
that is a static-model property, not free work. Frames are observed prologue
allocations only. They exclude dynamic callback choice, nesting, spills beyond
the recognized prologue and interrupt/core overlap. A caller plus callee frame
sum does not establish worst stack. Shortest chains are navigational evidence,
not exhaustive path, branch-frequency or cycle calculations.

No physical CPU headroom, deadline margin, receiver delivery or hardware
acceptance is claimed. Full all-parameter CC integration remains separate.
This evidence and candidate remain private.
