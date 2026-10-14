# Acultur8 FM1 developer compatibility and audit guide

Contract version: 0.1 (development). Applies to human developers, Codex and other coding agents. This guide defines contribution requirements; it does not certify existing or future third-party modules. FM1-41 remains open until manifests, automated validation and a third-party integration fixture pass. Physical compatibility is a separate acceptance boundary.

## Scope and normative language

MUST means required for integration. SHOULD means the default, with a documented and reviewed exception. MAY means optional. Repository instructions and this guide are engineering constraints; they do not grant account access, publication, spending or flashing authority. Agents MUST follow the human owner's current authorization and report conflicts.

Current modules are source components compiled into a single firmware or WebAssembly engine. There is no supported runtime plugin loader. A compatible browser build does not establish compatible physical firmware, MIDI routing or device performance.

## Required contribution record

Each contributed or modified module MUST provide the following in its PR. A machine-readable manifest/schema is planned; until that validator exists, these fields are required documentation, not a claim of automated enforcement.

| Field | Required evidence |
| --- | --- |
| Identity | Stable module ID, version, author, source repository and exact commit |
| Rights | Licence, attribution and distribution restrictions for code and assets |
| Interfaces | Public functions, ABI/layout assumptions, dependencies and supported build flags |
| Parameters | Stable IDs, units, ranges, defaults, quantization and bypass behavior |
| Persistence | Project/preset schema versions, migration rules and older/newer-format behavior |
| MIDI | Protocol revision, channels/CCs, reserved addresses, note ownership and transport behavior |
| Capabilities | Supported firmware/browser/hardware combinations and explicit unavailable features |
| Resources | Linked flash, ordinary RAM, pool usage, stack analysis and measured CPU evidence |
| Recovery | Checkpoint, rollback instructions and non-destructive migration procedure |
| Validation | Exact commands, commit/toolchain identities, results, artifact hashes and open gates |

## Integration requirements

1. **Stable contracts.** Developers MUST preserve existing parameter meanings and identifiers. A changed API, ABI, MIDI meaning or save format MUST be versioned and accompanied by an explicit migration or rejection path. Reusing an address does not establish compatibility.
2. **Safe persistence.** Unsupported/corrupt formats MUST fail with an explanation before mutation. Imports MUST validate bounds, types and capabilities. Migrations MUST preserve the original file and provide a recoverable result; silent downgrade or destructive conversion is prohibited.
3. **Audio and state.** Contributions MUST preserve default/bypass behavior against the pinned reference where applicable. New processing MUST have finite-output, bounds and relevant discontinuity tests. Exact PCM parity is appropriate for claimed lossless changes; intentional sonic changes require documented expectations and listening acceptance.
4. **Ownership and concurrency.** Audio-thread commands MUST respect the existing queue/acknowledgment contracts. Main-thread edits MUST not race audio-owned state. Save/reopen, undo/redo and failure recovery MUST be checked for affected controls. No automatic route activation or unsolicited MIDI transmission.
5. **MIDI and performance.** Parameter mappings MUST be inventoried with gaps identified. Clock sources MUST have explicit ownership and avoid feedback. Stop, route changes, reconnect and project replacement MUST clean up owned sounding notes. Queue-full, partial/in-flight delivery and stale acknowledgments MUST be tested; local transport state is not receiver proof.
6. **Resource limits.** Contributions MUST pass the original firmware gates without weakening limits, suppressing checks or silently removing features. Report actual linked usage and reserve margins. Object-size savings, host timing or static instruction counts alone do not prove device fit or CPU/stack safety.
7. **Distribution.** PRs MUST exclude private loops, credentials and restricted SDK files. Distribution rights MUST be established before reuse. Browser/source publication and firmware flashing MUST follow the owner's separate authorization.
8. **Ensemble compatibility.** Multiple devices MUST declare roles, capabilities and supported command meanings. Generic MIDI clock does not transmit chords, presets or scene identities. Group updates MUST expose partial failures; acknowledgments MUST not be described as atomic multi-device updates. Unknown devices MUST not receive bulk configuration or flashing.

## Audit procedure and acceptance matrix

The author supplies evidence; an independent reviewer checks the changed contracts and tests. The reviewer MUST report findings against an exact commit. New commits require assessment of whether prior results still apply. A coding agent's assertion is not acceptance evidence.

| Gate | Evidence needed | Result vocabulary |
| --- | --- | --- |
| Rights and source | Licence/asset review, exact provenance, distribution exclusions | pass / fail / blocked |
| Contract and migration | ID/map diff, compatible/incompatible fixtures, save/reopen and rollback | pass / fail / not tested |
| Native engine | Relevant regressions, sanitizers and reference comparisons | pass / fail / not tested |
| Actual browser engine | Built WASM, affected controls, audio/state/reload evidence | pass / fail / not tested |
| Firmware build | Official pinned toolchain, original gates and linked budgets | pass / fail / blocked |
| External receiver | Real message delivery, note cleanup, clock and routing | pass / fail / not tested |
| Physical device | Authorized device tests, audio, timing, stack and recovery | pass / fail / not tested |
| Ensemble/long set | Actual devices, loss/reconnect behavior, audio mix and real-duration soak | pass / fail / not tested |

Every report MUST identify applicable gates and explain exclusions. An untested gate stays open. A simulated two-hour sequence is not a two-hour physical performance. Approval of a source checkpoint is not a hardware release certificate.

## Submission and reviewer checklist

- [ ] Record exact base/head commits, module versions and toolchains.
- [ ] Provide the contribution record and changed-contract summary.
- [ ] Identify affected consumers and supported version combinations.
- [ ] Preserve a recoverable checkpoint and original user files.
- [ ] Run applicable acceptance checks; retain failures and final artifacts.
- [ ] Report resource usage, mapping gaps, known defects and pending device gates.
- [ ] Obtain independent review and resolve material findings.
- [ ] Merge only after required checks pass; publish/flash only within authorization.

The release compatibility matrix MUST identify the tested producer/consumer versions, fixture, evidence and gate status. Do not label an entire module or device family compatible based on one build. A proposed dependency declaration or schema is not supported until its implementation and tests exist.

## Recommended handoff for coding agents

“Read the repository instructions and this guide. Work from the specified base commit. Preserve existing behavior, stable IDs, saved projects and user files. Supply the required contribution record, exact evidence and recovery checkpoint. Do not weaken acceptance gates or claim unperformed receiver/physical tests. Stop for unresolved licensing, account access, spending, flashing or a material scope change. Request independent review before integration.”

This prompt is a handoff template, not permission to execute its actions. The human owner's actual instructions control authority.
