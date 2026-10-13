# FM1 instrument mod specification

User-requested direction recorded 2026-10-13. These are feature requirements and proposed implementation choices, not implemented firmware or hardware acceptance. The original archive and firmware working copy are unchanged by this specification.

## Sound shaping, independently per sound

Every 808/909 drum sound, 303 A, 303 B and the break part should have independent high-pass and low-pass filtering and a boost/ceiling limiter. This must operate before individual drums sum into their kit: a kit-wide processor alone cannot separate a booming kick from toms or other bass sounds.

Proposed chain: synthesis and existing sound distortion -> HPF -> LPF -> boost/limiter -> pan and dry/send routing -> shared FX -> existing master protection. Decide exact placement relative to existing voice LEVEL controls during the first prototype. Shared reverb/delay returns can add energy beyond an individual sound's ceiling; retain master protection. Filters help allocate low-frequency space, but do not eliminate summing overload or replace gain staging.

Proposed first controls: HPF cutoff, LPF cutoff, BOOST, CEILING, and bypass. Begin with non-resonant filters and a fixed limiter release, then expose release if useful. Compare inexpensive first-order and 12 dB/octave designs before choosing; do not introduce expensive resonant filters or true-peak oversampling without measurements. Smooth parameter changes and protect silence/denormals. No processing by default for existing projects.

The current master limiter in firmware/src/dsp/master.c has lookahead and a soft-ceiling stage. It is a reference for a smaller independent processor, not proof of a strict adjustable sample ceiling. Acceptance must measure the claimed ceiling, overshoot, distortion, silence, bypass equivalence and stereo linking. Lookahead adds delay; align participating paths, effect sends and bypass to avoid shifts in groove and wet/dry phase. Account for idle voices with delayed tails rather than skipping them early.

Insertion points: drum909.c d9_emit receives an individual voice sample before pan/sends; drum808.c lane_mix has both a fast bypass branch and a shaped branch. Both 808 paths must receive consistent processing. Bass and break processing belongs before their engine.c mix additions. Limiter/filter state must follow render ownership, including the second-core 808/break path; controls must use the existing queue.

## Sequencer pages versus arrangement blocks

Source evidence: pattern.h defines 32 patterns and 64 steps. ui.c already pages steps in groups of 16 and patterns in two banks. sequencer.h defines 192 song entries with a pattern per part and 1-8 repetitions. These existing capabilities are not new work to rebuild.

The requested extension is an explicit paged arrangement: a visible block number, per-part pattern assignment, block duration/repeat count, optional chord, and a next-block transition. Changing the page used for editing must not inadvertently change the block playing. Proposed default transitions occur at the next musical boundary, with a clear queued state, rather than immediately interrupting playback.

Important source constraint: the current 909 pattern length defines the song's bar/transition boundary. A proposed four-bar block must specify musical duration independently of changing a kit's step length, or clearly expose the existing semantics. Test unequal part lengths, triplet rates, swing, external clock, fills, progression wrap and edits during playback. Preserve existing song behavior when new arrangement mode is off.

## Chord progression as the shared harmonic timeline

Choose key/root and a seven-note scale. Each progression entry stores degree 1-7 and duration, with a displayed chord name and quality derived from the scale. Initial scope: diatonic triads; seventh chords and inversions are optional extensions, not required for the first pass.

Examples:

| Degree | C major | A natural minor |
| --- | --- | --- |
| 1 | C major: C E G | A minor: A C E |
| 2 | D minor: D F A | B diminished: B D F |
| 3 | E minor: E G B | C major: C E G |
| 4 | F major: F A C | D minor: D F A |
| 5 | G major: G B D | E minor: E G B |
| 6 | A minor: A C E | F major: F A C |
| 7 | B diminished: B D F | G major: G B D |

Natural minor's degree 5 is minor; harmonic minor can provide the major dominant, with other chord qualities derived accordingly. Do not use the major triad-quality table for every scale. TB-3PO currently includes Minor, Phrygian, Harmonic Minor, Minor Pentatonic, Dorian and Major. Minor Pentatonic has five pitches: disable seven-degree diatonic progression selection there or require an explicit seven-note parent scale. No silent remapping.

The lane supplies the chord active at a musical position to both 303s and any pad output. Progression changes, note triggers, MIDI output and the visible active chord must agree on event order. The initial implementation may attach chord/duration to arrangement blocks, with a standalone progression loop later, but its timeline must have a defined relationship to the arrangement.

## Chord-following bass

User requirement: generated bass notes can be restricted to the current chord, not merely the key's full scale. Strict chord mode selects root/third/fifth only, across the chosen octave range; make root-only a useful optional mode and retain existing scale mode. Passing tones are an optional later setting and must never leak into strict mode.

Apply the restriction to both generation and mutation. A reused rhythmic pattern must follow different chords on different blocks without overwriting the original pattern just because playback advances. Proposed model: preserve rhythm/accents/slides and a deterministic harmonic intent, resolving pitch against the active chord at playback. Decide the saved representation and compatibility before implementation; replacing absolute MIDI-note patterns blindly would break existing projects.

Define chord-boundary held-note behavior explicitly. A safe first strict mode releases/retargets notes at the boundary; an optional hold policy can be added later. Verify ties, slides, transpose, octave clamping, random playback direction, repeated chords, user-entered notes and outgoing MIDI. Turning chord-following off preserves existing absolute-pitch behavior.

## Pad accompaniment

Yes, the progression can trigger a pad. Proposed first implementation sends chord note-on/note-off messages to an external MIDI synth or computer instrument on a dedicated configurable channel. This adds little FM1 audio DSP work because the recipient produces the sound. It still needs event/state handling and real MIDI acceptance. Existing bass MIDI echo is not evidence of polyphonic pad support. DIN/TRS routing must be checked against the actual device; USB output needs a compatible host/recipient and cannot be assumed to connect directly to another USB device.

Keep the actual previous note list for note-offs; avoid premature retriggers on a repeated chord and release notes on stop/reset/channel change. Send cleanup only on the configured pad channel. A receiving pad may have a long release, so old and new chords can still overlap after note-off; choose receiver settings/voicing deliberately. First version can use sustained triads; root position, bass register separation and inversions are later musical choices.

Internal pad is a separate exploratory target. Both existing 303 parts are monophonic. A new three-voice oscillator/envelope pad is plausible but has unmeasured cost; start with simple sine/triangle tones, fixed voice cap, conservative level, no dedicated heavy effects and bounded release tails. Rich unison, resonant per-voice filters and long overlapping chord tails multiply workload. Default to off and do not count a host render as hardware CPU proof.

## Implementation and acceptance order

1. Simulator/toolchain baseline: representative factory and dense patterns, CPU/RAM and audio records.
2. One drum's HPF/LPF and limiter module; compare bypass and stressed audio, then expand per sound after measured headroom.
3. Explicit arrangement block navigation on top of existing paging/song structures.
4. Chord tables/timeline and strict chord-following bass, with host pitch/event tests.
5. External MIDI pad chord output and stuck-note/transport tests.
6. Optional minimal internal pad only after cumulative workload is measured.

All new saved parameters/timelines need a versioned migration, load of old projects, save/reopen/undo proof, and storage budget checks. Device installation and recovery remain separate release gates. None of the new features is currently implemented or compiled.
