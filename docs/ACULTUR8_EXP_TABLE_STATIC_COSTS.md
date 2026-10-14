# Exact EXP table encoding: integrated resource and call costs

Local checkpoint 2026-10-13. No physical timing or full MIDI completion claim.

Reviewed patch 57c04c3fe8877ffd5efbbc867ff1a4aa4d61a3f0d7d5e1b3371a800fea3ca1d3 preserves every one of 2432 IEEE32 values. Mandatory production/raw decoder output must match original complete-word SHA c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf under ASan/UBSan. Pinned lane PCM fixtures regenerate raw tables using their own original archived generator.

## Linked official target
Before 580884 bytes; after 579172: 1712 linked ROM bytes recovered (object-only estimate1720 differs by8). Limit581564 =>2392spare. General RAM98184/98304 unchanged; pool335712/344064 unchanged with8192reserve, leaving160above reserve. All15 original static gates pass, with original models and limits. Evidence build/target-combined-exp-authorized.log.

## Conditional setter work
The decoder has20 static instructions, zero calls and12 observed prologue bytes. D9setter820→814 staticinstructions, zero→one calls, observed52byteframe unchanged. FXsetter417→415, zero→one calls, observed52byteframe unchanged. D9initialization963 instructions/11calls/104byteframe unchanged. Reduced caller counts transfer work to the decoder; they do not establish CPU savings.

The setter path in engine.c reaches drum909_set and fxbus_set through engine parameter application. Queued controls are applied at audio render boundaries, as well as initialization/main contexts. Thus the extra conditional call and12byte nested decoder allocation must be included in that path's analysis. The decoder has no per-sample render invocation, but is not solely main-thread work. Rendering bodies remain unchanged in the reviewed object comparison. Whole-chain stack, IRQ nesting, physical cycles and worst-case control bursts remain unmeasured.

Isolated before/after ELF, assembly and address-qualified call/frame metrics remain in build/exp-pot-experiment/metrics.json. This ledger supplements unchanged static render gates; it does not replace device qualification.
