# Shared harmony resolver (first isolated slice)

`firmware/src/seq/harmony.c` is a pure shared C implementation for future
arrangement, bass and external MIDI pad integration. No engine, project schema,
TB3PO or dashboard behavior changes in this slice. The required build runner
now enforces the resolver test.
Physical firmware and external receiver acceptance remain pending.

## Contract

`harmony_spec_t` carries tonic pitch class (C=0 through B=11), an explicit mode,
degree I=1 through VII=7, and an optional seven-note parent mode. Mode IDs are
independent of the legacy TB3PO IDs. Supported seven-note modes are Ionian,
Dorian, Phrygian, Lydian, Mixolydian, Aeolian and Locrian. Root, third and fifth
come from alternate degrees of that mode, including scale and octave wrap.
Minor and diminished triads are derived rather than replaced with major chords.

Major/minor pentatonic selection must supply a seven-note parent. The entire
I–VII chord lane uses that parent, not a fabricated seven-degree pentatonic
scale. Missing/unsupported parents and chromatic mode reject. The parent field
is ignored for a seven-note mode. Callers must translate their own mode IDs
explicitly and retain legacy absolute-note behavior in their existing path.

`harmony_resolve(spec, root_hint, low, high, out)` returns three pitch classes
and an ascending closed root-position MIDI triad within inclusive bounds.
It selects the eligible root closest to the hint, with lower roots winning ties.
It fails if the range cannot accommodate the whole chord. No inversion, octave
spreading, randomization or silent range widening is performed.

`harmony_resolve_note(spec, source_note, transpose, low, high, follow, out)`
applies transpose, octave-constrains that target into the range, then selects
the nearest root or chord tone within that range. Equal-distance ties choose
the lower note. If no octave-equivalent target fits a narrow range, the lower
boundary is the target before chord snapping. The final MIDI note must still
belong to the selected chord; semitone boundary clipping never substitutes a
non-chord tone. Transpose changes the preferred note, not the chord key.

For example, C Ionian I–VI–IV–V produces C major, A minor, F major and G major.
C Ionian VII is B diminished. An input note 59 with transpose +1 in range 48–59
resolves to 48 in root-only mode, preserving the C root through the upper edge.

Both functions return 1 on success and 0 for invalid input or impossible range.
They reject null pointers, keys outside 0–11, degrees outside 1–7, unsupported
modes, invalid/inverted MIDI bounds, hints/source outside 0–127, transpose
outside -127–127 and unsupported follow modes. Failure leaves output untouched.
The caller owns rhythm, accents, slides, ties, held-note cleanup, transition
timing and persistence. Calling these functions alone establishes none of that
integration acceptance.

## Resources and validation

The module has one 49-byte read-only mode table, no writable static state,
heap allocation, floating point, external engine dependencies or random state.
Intent is four bytes; a resolved triad is six bytes on the tested host ABI.
Each triad lookup examines at most eleven root octaves. Each note lookup
examines at most 33 tone/octave candidates; range correction is also bounded
by the validated MIDI/transpose inputs. Neither function can loop on untrusted
unbounded values. Functions use local temporaries and commit output only after
successful resolution, allowing shared usage without module synchronization.
Target instruction cost, actual stack usage, flash/RAM linker placement and
audio deadlines must be measured after integration with the physical toolchain.

The standalone host test uses a separate major-scale rotation oracle. It checks
all 12 keys, seven modes and seven degrees; all hint notes; major/minor/diminished
results; C–Am–F–G; deterministic voicing/ties; complete range fit; both pentatonic
parent paths; invalid/output-preserving paths; and every input note against five
transposes, six ranges and both strict follow modes. The note oracle brute-force
checks the final eligible MIDI notes, including the transpose/range edges.

From PowerShell at the repository root, use the existing official local builder:

```powershell
docker run --rm --network none -v "${PWD}:/src" -w /src acultur8-fm1-emu-builder:local sh -c 'cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic tests/host/harmony_test.c firmware/src/seq/harmony.c -o /tmp/harmony_test && /tmp/harmony_test'
```

The final native compile/test passed 18,299,366 assertions; the same exhaustive
test passed with AddressSanitizer and UndefinedBehaviorSanitizer. At `-Os`, its host
object measured 1,036 text bytes with zero data/bss; its sole undefined reference
was the compiler stack-protector hook. These are host-object observations, not
target budget acceptance.
No SDK files or generated binaries are added to the repository.
