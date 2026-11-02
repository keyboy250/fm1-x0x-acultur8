# Constrained 303 generation: first local candidate

Private label: 0.1.11-genbeta / FM-1_9000111. Parent: the frozen, locally qualified 0.1.10-crosscopy source. Neither parent nor this new candidate has been installed or published by this task. The connected FM1 remains on0.1.9-query.

This is the first slice of FM1-61. Random drum generation (FM1-62), controlled repeating variations (FM1-63), locked-step generation and a pre-commit musical preview remain separate work. Do not call the whole generative roadmap complete.

## Controls

Select303 A or303 B and its destination pattern. ARP opens GENERATE; press ARP again for SCALE and again for the new CONSTRAINED page. Existing generator pages and their live/manual/periodic mutation behaviour remain unchanged.

On CONSTRAINED, KNOB1 chooses NOTES; KNOB2 selects the chord-source BLOCK; KNOB3/4 retain the existing OCTAVE/OCTS controls. Configure density, accent/slide chance and generator root/scale on the existing pages. OCT+ asks to replace this part's notes using a fresh seed. OCT- asks to regenerate using the current saved seed. SEL confirms; HOME cancels. Confirmations name the destination303/pattern and, for chord modes, the source block. No note preview is generated before confirmation in this first slice.

| NOTES | Stored pitch constraint |
| --- | --- |
| GEN | Existing generator ROOT and six-scale selection |
| KEY | Project arrangement key and mode, including pentatonic/chromatic modes |
| CHORD | Triad of the selected existing arrangement block using project key/mode/parent |
| BOTH | Intersection of generator ROOT/SCALE with the selected block chord |
| FREE | Chromatic pitches within the chosen register, with no scale filter |

The chord is captured at generation time; this is not a continuously chord-following generator. A missing block, degree0/no chord, unresolved chord or empty intersection refuses without changing the project. CHROMATIC without an appropriate diatonic chord definition does not invent a chord. A selected pentatonic mode needs its explicit parent for chord resolution.

Register bounds use existing generator settings: inclusive MIDI low=12*(base_octave+1), high=low+12*octave_range-1. OCTS spans1..3 octaves. Every stored pitch, including inactive steps, stays in the allowed set/range. Density0 creates rests;100 creates notes at every active step. Slides into rests are removed. Generator accent/slide controls explicitly govern new articulation. Final playback transpose, harmony-follow settings, motion/probability lanes and output routing remain independent and unchanged; stored-note constraints are not a claim about pitches after an intentional playback transform.

For this snapshot workflow, set the existing periodic MUTATE control OFF first. Otherwise generation refuses, so an old automatic mutation setting cannot immediately overwrite the new constraints. Turning periodic mutation on later explicitly re-enables the existing legacy generator behaviour.

## Preservation and recall

Only the selected303's note/flags array and seed change. Its length/rate/direction/transpose, generator controls other than seed, sound/strip/routing, other parts/patterns, shared accents, swing, song and all motion remain intact. This operation can replace the selected303's note accent/slide flags according to its existing generator controls; it is distinct from cross-part copy's accent-preservation policy.

A stopped staged transaction validates transport, recording/keys, pending writes/queues, write protection and Undo capacity. Refusal and Cancel make no partial edit. A changed generation is one Undo/Redo transaction. Saved notes and the full32-bit seed survive recall; NOTE mode and block focus are transient UI choices, not a new saved-project schema. Repeat is reproducible with the same seed, density/articulation settings, range and captured harmonic context. Auto-save continues under its existing independent configuration; generation does not call Save.

No DSP/MIDI rendering loop was changed. Two transient mode bytes fit existing UI padding; no added static RAM or project fields. Existing ordinary-RAM headroom remains12bytes, so further features still need resource review. Diagnostic target packages are NOT_FOR_FLASHING.

## Evidence and remaining gates

Final status and hashes are recorded in GENERATION_STATUS.json. Local tests exercise all4095 nonempty pitch sets, all128 single-note ranges, all12 keys/10 project modes,588 diatonic chord contexts, both303s/five modes/three project formats with twelve seeds each; P32/B192 context packing; refusals/races, Undo/Redo, actual command49 replies through the production Python reader, native saved recall and real production-WASM controls. The original full regression and cross-copy tests remain separate checks.

Physical menu, listening, receiver delivery and saved power-cycle acceptance remain pending. No device access or publication in this implementation task.

Reproduce with the preserved offline toolchain and SDK: tests/host/run_generation.sh; tests/host/run_generation_target.sh; cc_inventory.py --write and gen_cc_bindings.py; X0X_VERSION=0.1.11-genbeta lab/build_emulator.py --source-root /work --native; tests/test_generation_read.py; generation_wasm_test.mjs with build/emu/x0x.wasm; tools/acultur8_checks.sh. Run cross_part_copy tests and their query/WASM clients after generating native fixtures. Exact pinned oracle history remains in build/generation/oracle-history.bundle; import refs/test-oracles/* into an extracted source-only tree before running the original regression suite.
