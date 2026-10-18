# 303 split gate/pitch sequencing exploration

User-requested feasibility exploration, 2026-10-13. Proposed, not implemented. No physical CV output or hardware acceptance claimed; publication held.

## Concept

Use a per-303 mode selector: Classic / Split gate-pitch. Prototype on 303 A first; 303 B stays in Classic. Gate and pitch sequences have independent positions, rates and lengths but share the master musical clock. Gate events decide when to trigger/hold/release; pitch events update the currently selected note. On a trigger, sample that note, then apply existing transpose/range/chord-follow policy. This is internal CV/gate-style sequencing and optional MIDI note output, not analogue voltage output.

A useful example is a 16-step gate sequence against a 7-step pitch sequence, with selected rates: the pitch/rhythm pairing evolves over successive cycles. Length alone yields polymeter; differing rates add polyrhythmic relationships.

## First prototype policy

Use free-running pitch steps, not automatic pitch advance on each trigger. Coincident clock events update pitch before gate deterministically. Pitch changes during a sustained gate initially leave the sounding note unchanged; the next trigger samples the new pitch. Explicit slide/tie handling must be specified and tested before allowing live held-note retuning. A later pitch-follow-held-gate option is a distinct policy, not an implicit side effect.

Gate sequence owns rests/triggers/ties and accent/slide semantics; pitch sequence owns notes. Preserve current Classic semantics exactly. Gate duration must be defined relative to its own interval. A pitch step alone must never create a Note On. Rest, Stop, reset, rejected note and mode changes must clean up original accepted notes. Existing chord-follow cleanup/range behaviour applies to sampled pitches. External MIDI remains note events; no physical CV/gate connector implied.

## Mode and storage

Prefer one mode selector per 303, scoped to its pattern. Keep Classic data recoverable; do not rewrite stored notes while toggling. Prototype on stopped transport first. Live switching, if qualified, occurs at a documented shared bar boundary with cleanup and consistent phase initialization. Determine defaults/conversion and independent save/undo behaviour after auditing available storage. Current firmware image/RAM are tight; do not promise 64+64 steps or persistence without measured fit. Browser-only mockups must be labelled and cannot count as firmware acceptance.

## Controls and acceptance

Show side-by-side gate and pitch rows, separate length/rate/position and an audible-output note preview. Map mode and new musical controls into the complete CC inventory; scoped dice/locks must distinguish gate and pitch. FAQ diagram illustrates pitch updates versus gate triggers.

First acceptance gate: isolated native scheduling prototype, explicit tie/slide/held-pitch policy, resource estimate and persistence design. Then require pinned Classic PCM parity, coincident-event ordering, odd lengths/mixed rates, chord boundaries, Stop/Continue/internal/external clock, live CC, note cleanup, undo/save/reopen/migration and original target resource checks. Physical audio/timing and MIDI receiver tests remain pending. This exploration does not displace critical FM1-32/33.

## Guitar analogy in the FAQ

Picking/strumming represents gate timing; fretting represents pitch choice. The linked FAQ includes a two-hand diagram and an 8-step gate/3-step pitch example (24-tick combined cycle at equal rates). Pitch advances through rests, coincident updates precede triggers, and no new trigger does not imply a ringing tail stops. Hammer-ons/pull-offs and held-note retuning are explicit caveats, not implemented behaviours. See `faq#guide-cv-gate`; feature remains proposed.
