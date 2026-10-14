# Scoped randomisation and locks

Requested 2026-10-13. Planned feature; no runtime or hardware acceptance claimed. Browser Pages publication remains held; reviewed GitHub source updates are authorised.

## Dice hierarchy

Composition > selected section/block > selected pattern > individual sound lane > module/feature > parameter. Scope labels and counts accompany each dice graphic; never silently include unselected patterns. Whole-composition rolls affect musical content/settings across the project, respecting the same locks used by narrow rolls. Use original SVG dice artwork with accessible names, keyboard/touch activation and text labels.

## Locks

Provide lock controls at every level plus individual parameter/step locks. Effective lock is the union of ancestor and local locks: a child cannot override a locked parent. Rolling never changes lock state or lock policy. Lock key and scale by default once chosen; offer separate locks for progression, tempo, lane rate/length, sound/FX, notes/gates/accents/slides and individual steps. Locking a progression preserves degree order and durations. Key/scale locks constrain newly generated pitches and chords as well as preserving the key/mode values. Chord-following bass generation uses the applicable progression chord; don't assume every generator scale is the harmony scale. If constraints conflict with a locked note, keep the note and report the conflict; never rewrite it silently. Locked length also excludes hidden/out-of-range steps from accidental modification. Protect selected seed as an independent option; retain actual transaction seed in the result for reproducibility.

## Musical behaviour and transactions

Use a descriptor-driven eligibility registry: ranges, musical domains, scope, lock identity, live-safe setter, persistence and CC mapping. Include a bounded variation amount and musical profiles, not uniform random values for every DSP control. Exclude file operations, flashing, sample import/removal, device routing, permissions and transport actions from musical dice. Preserve imported WAV/REX data and original slice markers. Default to preserving output level/limiter ceiling and MIDI routing; offer intentional level variation within reviewed bounds.

First build deterministic candidate generation against a snapshot, followed by preview and one undoable Apply. Locked values must be bit-exact. Report eligible/changed/skipped counts; all-locked scope is a no-op with explanation. Same source state, seed, lock mask and algorithm version reproduce the same candidate. Apply through authoritative validated engine transactions, with bounded queue capacity and documented ordering against CC/UI edits. Reject/re-preview if relevant source state or locks changed; never partly apply a composition because a queue filled. Keep live pattern/chord changes at documented musical boundaries with note cleanup. Locks govern randomisation, not ordinary manual or DAW editing; explicit MIDI controls for roll/variation/locks belong to the complete mapping inventory. Trigger rolls on a defined rising edge to avoid rerolling every repeated CC value.

## Persistence, display and FAQ

Store lock masks/settings with an explicitly versioned project design after measuring firmware memory, migration and undo costs. No claim that existing packed lane storage has spare capacity for locks. Start with a browser preview prototype, clearly labelled if metadata is browser-only; it cannot qualify firmware persistence. Show locked controls visually and with accessible state. FAQ diagrams: scope tree, inherited lock example and seeded preview > apply > undo. Animation follows actual state and has reduced-motion support.

## Acceptance

Prove ancestor/local lock preservation at every scope, key/scale/chord constraints, exact same-seed reproduction, all-locked no-op, descriptor range coverage, bounded variation, hidden-step policy, stale-preview handling, atomic apply/undo/redo, save/reopen and old-project migration. Exercise mixed 25-lane lengths/rates, chord boundaries, sustained notes and simultaneous live CC. Re-run default PCM and original target resource gates; desktop/narrow/keyboard and physical draw/audio timing remain separate checks. No full-composition randomisation acceptance from a generator-only button.

## Note range and voice constraints

See [note-range and polyphony design](NOTE_RANGE_POLYPHONY_DESIGN.md). Random candidates must respect locked inclusive pitch ranges and voice caps; octave folding preserves pitch class. Constraints and optional inversion policy apply to compatible melodic generators/output, without promising extra internal synth voices. Conflicts require visible reporting and defined note cleanup.
