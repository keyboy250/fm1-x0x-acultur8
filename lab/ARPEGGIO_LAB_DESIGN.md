# Acultur8 Chord & Pattern Lab

Status: source-audited interface proposal, 2026-10-13. No new runtime control or public publication claimed. The user's Arpeggio Lab screenshot is a visual reference, not a specification of existing FM1 functionality.

## What ARP currently means

In the reviewed local variant, `firmware/src/app/ui.c` handles `B_ARP` by opening the TB-3PO generator for 303 A or 303 B. It is a stored bass-pattern generator rather than a held-note arpeggiator. OCT+ generates a new line with a new seed; OCT- mutates the existing line.

The generator exposes density, accent and slide probabilities (0–100%), octave range (1–3), root (C–B), six generator scales, base octave (0–4), and automatic mutation interval (Off or 1–16 bars). Generator-setting edits prepare the next New/Mutate operation; they do not automatically replace the stored steps. The seed is stored with each bass pattern.

Sequence controls include 1–64 steps, rate, forward/reverse/ping-pong/random direction and transpose. The reviewed standalone local FM1-32 checkpoint supplies eight rates; combined CC/scene and physical acceptance remain pending. Do not describe the published baseline as having that full timing extension.

The variant's harmony system can constrain played bass notes to the current chord's root or chord tones, within a chosen range. This playback constraint is separate from the generator's scale. The interface must show both rather than imply that the generator's scale selector edits the progression.

## Proposed browser layout

1. **Transport and selection:** 303 A/B, selected pattern, playing pattern, clock source, tempo, start/stop, acknowledged/pending state.
2. **Harmony:** key and mode, degree I–VII progression blocks, block duration, current/queued chord, bass follow Off/Root/Chord and note range. Keep generator scale separately labelled.
3. **Pattern:** density, accent, slide, octave range, generator root/scale/base octave, seed, New line, Mutate and auto-mutation interval.
4. **Timing:** lane rate, independent step count, direction, transpose and the existing swing behavior. Label swing scope correctly; do not invent per-lane shuffle.
5. **Piano roll:** stored notes plus distinctly styled chord-constrained output, playhead, rests, ties, accents and slide links. Show selected versus playing patterns explicitly. Provide 16-step pages and an overview of all 64 steps.
6. **Performance mapping:** MIDI Learn/map display using the shared CC registry and authoritative engine path once live ingress is qualified. Do not claim DAW control before that acceptance passes.

Use the reference's clear grouping, readable values and immediate visual feedback with an original Acultur8 design. Keyboard-accessible sliders/numeric inputs accompany rotary visuals; controls must also work by touch without dragging. Each group links to a labelled FAQ diagram and a practical example.

## Extensions requiring separate engine work

- A true held-note arp with latch, retrigger and explicit chord-input ownership.
- Separate anchor and movement layers, pattern shape and octave/rhythm modifiers.
- Per-step gate length/velocity variation and humanize beyond existing accents/ties/slide semantics.
- Independent seed lock or preserve-anchor regeneration.

These are candidates for review, not hidden additions to the current firmware scope. Start with the existing generator and harmony controls; prioritize current independent-lane and live-CC acceptance first.

## Integration and acceptance

Reuse the existing WASM firmware, shared validated setters and acknowledged snapshot/ticket transport. Do not duplicate TB-3PO or harmony in JavaScript and present its prediction as actual engine output. A speculative preview must be labelled until the engine returns the resulting pattern.

Before connecting each control, verify its descriptor range, pattern ownership, undo transaction, dirty/readback state, running-performance safety and save/reopen behavior. New/Mutate must have explicit apply semantics and an undoable result. The piano roll must match actual engine notes, including transpose, chord boundaries, ties and slides. Test odd lane lengths, mixed rates, chord changes and MIDI automation together; preserve default audio and resource gates. Desktop/narrow layout and keyboard checks are required. Physical-device qualification remains pending.

No publication is authorized until the user's explicit sign-off under the current publication hold.

## Reference-to-control mapping

The reference image suggests a clear hierarchy, not identical engine features. Our first interface should use five visible groups: Harmony (key/mode, progression, follow), Pattern (density/accent/slide, generator scale/root/octaves, seed and explicit New/Mutate), Timing (length/rate/direction/transpose), Preview (stored versus played notes and playhead), and MIDI (mapping and acknowledged values). Keep current values readable beside knobs and give each group a diagram/help link. Colour supports, rather than replaces, labels and symbols. Animate actual snapshots; provide reduced-motion support.

Do not show Hold/Latch, anchor/movement or humanize as working controls until their engine implementation and acceptance exist. Generating in a scale and following the current progression chord are separate operations, and the preview must make that distinction visible. The source's auto-mutation counter advances at local pattern wraps, so odd lengths/rates make 'every N bars' misleading; label it in pattern cycles in the browser until a distinct musical-bar policy is implemented and tested.

## Scoped dice and locks

User-requested randomisation extends from whole composition to individual controls, with inherited locks and key/scale protection. See `RANDOMISATION_DESIGN.md` and FM1-34. This supersedes seed-lock-only exploration as a required planned feature; runtime implementation remains pending.
