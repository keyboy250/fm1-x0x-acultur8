# CC Undo/Redo static helper costs

This ledger describes the existing official history checkpoint objects, not a new build. The parent measured XIP overflow1,592bytes (+112history); target acceptance is pending. Original15resource/static gates and their limits are unchanged. `tests/cc_history_feature_costs.py` reuses `target_budget.cost` with the original model (nest4,divide8,maxdepth4).

Regenerate offline object disassembly using `build/history-cost-experiment/extract.sh`, then `python tests/cc_history_feature_costs.py --report tests/cc_history_feature_costs.json`. The exact official unity object is the saved `build/cc-history-checkpoint/target-x0x.o`; existing engine object is copied from `build/o2/app_engine.o`. Both byte hashes and disassembly hashes are recorded. Source identity is the frozen history manifest (CCee30ba…/UI438561…). No source, compiler, link or baseline update is performed by the reporter.

ELF relocations qualify direct calls and tail transfers. IDs retain object, section, function and relative address, including duplicate names. The46reported rows include every emitted transitive callee available in these two objects; unresolvedexternal/indirect targets remain explicit. Source alternatives preserve `do_undo`→both `undo_undo`/`undo_redo`, and `ud_swap`→`undo_changed` callback. External relocations cannot create artificial intra-function loops: their printed targets are suppressed before the unchanged original cost model examines backward spans. Shared backward exit branches can still contribute structural loop estimates; these are not literal iteration counts.

|Helper|Instructions|All callsites|Original model cost|Observed frame allocation bytes|
|---|---:|---:|---:|---:|
|ui_cc_undo|31|4|25|12|
|do_undo|93|17|16|52|
|ui_undo_preflight|27|2|0|12|
|undo_arrangement_intent|97|4|27|40|
|engine_transport_reserve|29|3|20|4|
|engine_transport_release|11|1|0|4|

The main path is `ui_cc_undo`→reserve→`do_undo`→preflight/intent→journal commit and Undo/Redo→`ud_swap`→changed callback, then existing motion/arrangement/BREAK refresh, page rebuild and message. The reporter includes these callees rather than treating the small wrapper as the whole operation. Nested reservation calls and journal scanning/swapping remain real work.

Frames list register/return-address pushes and local allocations, including a scalar `rets` push. Allocation sums are conservative observations within a function, not transitive worst stack; mutually exclusive allocation sites, tail transfers, external helpers, indirect callbacks and interrupt nesting require actual path/hardware analysis. Static instruction/call/model scores are not physical cycles, deadlines or CPU headroom. Local queue acceptance does not prove receiver delivery. Physical MIDI, worst-case performance/stack and linked resource acceptance remain pending.

Parser checks pass: normal regeneration; missing engine/root helpers reject; malformed ELF rejects; exact original model and sourced callback alternatives verified. No automatic fallback, omission or baseline relaxation.
