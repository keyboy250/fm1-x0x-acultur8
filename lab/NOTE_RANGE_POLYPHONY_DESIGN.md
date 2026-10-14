# Note-range folding and polyphony constraints

Requested: 2026-10-13. Planned; no runtime acceptance or new internal voices claimed. Browser Pages publication remains held; reviewed GitHub source updates are authorised.

## Musical operation
Offer per applicable melodic lane/module Range enabled, lowest note, highest note, maximum simultaneous notes and deterministic voicing policy. Display MIDI note numbers alongside octave labels to remove octave-naming ambiguity. Example inclusive range C2 to C4: D5 folds down by octaves to D4; B1 folds up to B2. Preserve pitch class by adding or subtracting multiples of twelve, never clamp to a different note name.

Choose the nearest octave-equivalent pitch inside the range. A narrower-than-octave range may contain no equivalent note: report this in preview and skip that unlocked generated note. Reject inverted bounds; do not silently widen bounds or change locked source notes. De-duplicate chord tones that fold onto the same MIDI note before applying the voice cap.

Range folding is distinct from chord inversion. A chord inversion changes which chord tone is lowest; optional deterministic voicing can choose octave placements for smooth movement, while preserving chord pitch classes and obeying the range. Label the basic control Octave fold / Note range, with inversion as a separate musical policy.

## Polyphony and randomness
Apply a maximum-note cap only where simultaneous notes exist. The two 303 engines remain monophonic; this control does not turn them into polyphonic synths. External MIDI chord/pad output can use a capped voicing, with actual receiver acceptance separate. Additional internal pad voices require separate memory and CPU feasibility.

Randomisation uses the existing seeded scope and lock system. Generate scale/chord-valid candidates, choose octave placements, fold into range, remove duplicates and select within the cap using an explicit stable policy (for example root priority, then closest movement). Random chord inversion or octave choice may vary within these constraints. Range and polyphony settings can themselves be locked; a broader dice roll cannot override locks. Report conflicting constraints instead of changing locked notes.

For sustained MIDI output, retain the actual emitted pitches and route for Note Off; later range/chord/cap changes must not recompute the old Note Off pitch. Define live changes at a musical boundary and clean up released/removed voices. Importing or changing constraints should not silently rewrite stored patterns; preview and one undoable Apply are separate from optional playback-only folding.

## Acceptance
Exhaustively test MIDI pitches zero through one hundred twenty-seven and valid bounds; preserve pitch class, enforce inclusive bounds and deterministic missing-equivalent handling. Cover duplicate collapse, stable cap selection, root-priority conflicts, seeded reproduction and inherited locks. Test simultaneous gate/pitch/chord changes, sustained notes, route disconnect, Stop and stuck-note cleanup; verify CC mapping, actual engine readback, undo/redo, save/reopen and migration. Preserve legacy output with constraints disabled. Measure original firmware flash, RAM, pool and audio timing limits before integration. FAQ diagrams should show octave-equivalent notes folding into a band and chord voices being reduced to a cap.
