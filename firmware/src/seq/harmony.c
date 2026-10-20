/* SPDX-License-Identifier: GPL-3.0-only */
#include "harmony.h"

static const uint8_t modes[7][7] = {
    {0,2,4,5,7,9,11}, {0,2,3,5,7,9,10}, {0,1,3,5,7,8,10},
    {0,2,4,6,7,9,11}, {0,2,4,5,7,9,10}, {0,2,3,5,7,8,10},
    {0,1,3,5,6,8,10}
};

static int distances(const harmony_spec_t *spec, uint8_t pc[3], int span[3])
{
    int mode, degree, i, root;
    if (!spec || spec->key > 11 || spec->degree < 1 || spec->degree > 7)
        return 0;
    mode = spec->mode;
    if (mode == HARMONY_MAJOR_PENTATONIC || mode == HARMONY_MINOR_PENTATONIC)
        mode = spec->parent_mode;
    if (mode < HARMONY_IONIAN || mode > HARMONY_LOCRIAN)
        return 0;
    degree = spec->degree - 1;
    root = modes[mode][degree];
    for (i = 0; i < 3; ++i) {
        int d = degree + 2 * i;
        int offset = modes[mode][d % 7] + 12 * (d / 7);
        pc[i] = (uint8_t)((spec->key + offset) % 12);
        span[i] = offset - root;
    }
    return 1;
}

static int valid_range(int low, int high)
{
    return low >= 0 && high <= 127 && low <= high;
}

static int abs_distance(int a, int b)
{
    return a > b ? a - b : b - a;
}

int harmony_scale_note(int key,int mode,int source,int transpose,int low,int high,uint8_t *out)
{
    unsigned mask=0;int i,n,desired,best=-1,distance=256;
    if(!out || key<0 || key>11 || mode<0 || mode>HARMONY_CHROMATIC ||
       source<0 || source>127 || transpose < -127 || transpose>127 || !valid_range(low,high))return 0;
    if(mode<HARMONY_MAJOR_PENTATONIC)for(i=0;i<7;i++)mask|=1u<<modes[mode][i];
    else mask=mode==HARMONY_MAJOR_PENTATONIC?0x295u:mode==HARMONY_MINOR_PENTATONIC?0x4a9u:0xfffu;
    desired=source+transpose;
    for(n=low;n<=high;n++)if(mask&(1u<<((n+12-key)%12))){
        int d=abs_distance(n,desired);
        if(d<distance){best=n;distance=d;}
    }
    if(best<0)return 0;
    *out=(uint8_t)best;return 1;
}

int harmony_resolve(const harmony_spec_t *spec, int root_hint,
                    int low, int high, harmony_triad_t *out)
{
    harmony_triad_t result;
    int span[3], n, best = -1, distance = 256, i;
    if (!out || !valid_range(low, high) || root_hint < 0 || root_hint > 127 ||
        !distances(spec, result.pitch_class, span))
        return 0;
    for (n = result.pitch_class[0]; n <= 127; n += 12) {
        int d = abs_distance(n, root_hint);
        if (n >= low && n + span[2] <= high && d < distance) {
            best = n;
            distance = d;
        }
    }
    if (best < 0)
        return 0;
    for (i = 0; i < 3; ++i)
        result.midi[i] = (uint8_t)(best + span[i]);
    *out = result;
    return 1;
}

int harmony_resolve_note(const harmony_spec_t *spec, int source_note,
                         int transpose, int low, int high,
                         harmony_note_mode_t mode, uint8_t *out)
{
    uint8_t pc[3];
    int span[3], desired, i, n, best = -1, distance = 256;
    if (!out || !valid_range(low, high) || source_note < 0 || source_note > 127 ||
        transpose < -127 || transpose > 127 ||
        (mode != HARMONY_ROOT_ONLY && mode != HARMONY_CHORD_TONES) ||
        !distances(spec, pc, span))
        return 0;
    desired = source_note + transpose;
    while (desired < low) desired += 12;
    while (desired > high) desired -= 12;
    if (desired < low) desired = low;
    for (i = 0; i < (mode == HARMONY_ROOT_ONLY ? 1 : 3); ++i) {
        for (n = pc[i]; n <= 127; n += 12) {
            int d = abs_distance(n, desired);
            if (n >= low && n <= high &&
                (d < distance || (d == distance && n < best))) {
                best = n;
                distance = d;
            }
        }
    }
    if (best < 0)
        return 0;
    *out = (uint8_t)best;
    return 1;
}
