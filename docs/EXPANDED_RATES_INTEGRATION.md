# Expanded rates: local integration checkpoint

Qualification status: independent review, actual browser acceptance and complete mandatory suite9/9 PASS. Qualified source is integrated locally; FM1-44 remains open for physical acceptance. Pages, public deployment, releases, flashing and physical qualification remain held.

The candidate extends reviewed clock commit `12d32d695387ef83601ec3c4cc9cddce08f3a0cd` in `expanded-rates-integration-20261014`. It offers 21 divisions: straight, dotted and triplet for 1/1 through 1/64. Original rate IDs 0..7 and parameter1129/channel13 CC105 remain unchanged.

Stopped browser setup: Switch on, scroll to Sound lanes, initialize independent lanes if needed, then Enable expanded rates. Select a sound, choose its length and rate family, then its Step rate. Policy is shared by that sound across all 32 patterns; lengths and exact rates remain per pattern. Families are Straight, Straight+dotted, Straight+triplets and All. Changing family preserves allowed rates and remaps excluded rates to the nearest duration, with slower ties. Existing triplets are preserved on first migration.

New channel13 MIDI bindings:

| CC | Parameter | Function |
|---|---|---|
|111|1135|Enable expanded rates, stopped only|
|112|1136|Exact division ID 0..20|
|113|1137|Sweep family 0..3|
|114|1138|Next step or next sound step after bar|
|115|1139|Hysteresis 0..2 CC units|
|116|1140|Family-safe sweep, slow to fast across 0..127|

Use the existing context selectors to choose the pattern and sound. Enum CC values use the documented normal CC scaling; exact transaction commands accept enum IDs. Sweeps are monotonic in both directions within the selected family, with exact endpoints. Repeated bins are no-ops even under full queue pressure. The legacy exact-rate CC is preserved separately.

Runtime uses rational sample timing, including fractional MIDI pulses. Changes preserve the interval in flight and adopt at the selected sound boundary. Each interval retains its denominator across tempo changes. Ambiguous queued ticks or an out-of-domain external period freeze expanded transport before unsafe arithmetic; clock alone cannot restart it. Legal 20..300 BPM pulse quantisation endpoints and valid recovery are covered.

LAN2/project format6 losslessly preserves all 32 patterns, 25 lanes, 64 lengths/steps, BREAK gates/slices and unowned cells. Older firmware explicitly refuses format6. Undo/redo, protected refusals and save/reopen are tested. Hi-hat PCM is packed losslessly to regain target space; every decoded sample and pinned legacy rendering/parameter/tail comparisons must remain exact.

Evidence identities: runtime source `256414965c6330524392c504498583d5040a2539525b5a807e778fd148e55eb5`; WASM `240984c163aee9003f90be449cbb5bbec9c074ee01656fe4880b7a45ab6f0173` (685261 bytes); target disassembly `8ee11f39d6ae5697222d19c91255f0b1d3dc711aba5a7f615e0ee50b7ff051d3`. All112 frozen firmware/host/assets and build-generator inputs match.

Original 15 static target gates and unchanged clock-family reference PASS. Additional direct-call/tail-call closure covers79 bodies. Target image579364/581564, ordinaryRAM98300/98304, pool335772/344064. Margins are2200 image bytes,4 ordinaryRAM bytes and100 bytes above the8192 pool reserve. Static fit does not establish physical stack, CPU, jitter or soak acceptance.

Actual browser QA used separate origin8134: BD length8, 1/8 dotted, Straight+dotted, bar policy and hysteresis2; CH length3, 1/16 triplet and Straight+triplets. Both survive save/reopen. All21 options and family exclusions are read back; default viewport has no horizontal overflow. No MIDI permissions were requested and the user's8133 stored projects were untouched.

Remaining acceptance: real MIDI input/DAW/receiver, native device menu interaction, listening, IRQ/stack/timing and performance soak. Composition-wide apply and randomisation extensions are separate work. Local implementation does not mark FM1-44 physically complete.

Evidence: [integration receipt](../build/expanded-rates-reviewed-integration-20261014/RATE_INTEGRATION_RECEIPT.json), [nine-check report](../build/expanded-rates-reviewed-integration-20261014/acultur8-checks.json), [original15 gates](../build/expanded-rates-reviewed-integration-20261014/expanded-target-original15.json), [caller/helper ledger](../build/expanded-rates-reviewed-integration-20261014/expanded-target-clock-ledger.json), [browser receipt](../build/expanded-rates-reviewed-integration-20261014/expanded-rate-browser-receipt.json). The recoverable pre-integration checkpoint is `build/pre-expanded-rates-integration-20261014`. No push, PR merge or Pages deployment is part of this integration.
