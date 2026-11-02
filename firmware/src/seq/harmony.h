/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Independent of TB3PO scale IDs and persisted legacy structs. */
typedef enum {
    HARMONY_IONIAN, HARMONY_DORIAN, HARMONY_PHRYGIAN, HARMONY_LYDIAN,
    HARMONY_MIXOLYDIAN, HARMONY_AEOLIAN, HARMONY_LOCRIAN,
    HARMONY_MAJOR_PENTATONIC, HARMONY_MINOR_PENTATONIC, HARMONY_CHROMATIC
} harmony_mode_t;
#define HARMONY_NO_PARENT 255
typedef struct {
    uint8_t key;          /* tonic pitch class, C=0 .. B=11 */
    uint8_t mode;         /* harmony_mode_t */
    uint8_t degree;       /* I=1 .. VII=7 */
    uint8_t parent_mode;  /* seven-note mode for pentatonic; otherwise ignored */
} harmony_spec_t;
typedef struct {
    uint8_t pitch_class[3]; /* root, diatonic third, diatonic fifth */
    uint8_t midi[3];        /* ascending, closed root-position voicing */
} harmony_triad_t;
typedef enum { HARMONY_ROOT_ONLY, HARMONY_CHORD_TONES } harmony_note_mode_t;

/* Transpose in semitones, then choose the nearest scale note within bounds.
 * Ties choose lower. A range containing no legal note fails without output. */
int harmony_scale_note(int key,int mode,int source,int transpose,int low,int high,uint8_t *out);

/* Return 1 on success, 0 on invalid input or no complete voicing in the inclusive
 * MIDI range. Output remains untouched on failure. Closest root to root_hint;
 * equal-distance ties choose the lower voicing. All note inputs must be 0..127. */
int harmony_resolve(const harmony_spec_t *spec, int root_hint,
                    int low, int high, harmony_triad_t *out);

/* Apply transpose (-127..127), octave-constrain the desired note into range,
 * then choose the nearest permitted chord tone (ties lower). Thus the FINAL
 * note is a root/chord tone even after transpose/range correction. If octave
 * equivalence cannot fit a narrow range, use its lower boundary before snap.
 * This transforms one pitch only; rhythm/accent/slide and legacy notes belong
 * to callers. Output remains untouched on failure. */
int harmony_resolve_note(const harmony_spec_t *spec, int source_note,
                         int transpose, int low, int high,
                         harmony_note_mode_t mode, uint8_t *out);
