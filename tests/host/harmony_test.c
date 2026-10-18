/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/harmony.h"

static int failures, checks;
#define CHECK(c) do { ++checks; if (!(c)) { \
    if (failures++ < 20) printf("FAIL line %d: %s\n", __LINE__, #c); \
} } while (0)

/* Independent oracle: rotate the major scale, normalize its new tonic, then
 * stack alternating degrees. This catches wrong mode rows and wrap arithmetic. */
static int expected(int key, int mode, int degree, int tone)
{
    static const int major[] = {0,2,4,5,7,9,11};
    int d = mode + degree - 1 + tone * 2;
    return (key + major[d % 7] + 12 * (d / 7) - major[mode]) % 12;
}

static void progression(void)
{
    const int degrees[] = {1,6,4,5};
    const uint8_t pcs[][3] = {{0,4,7},{9,0,4},{5,9,0},{7,11,2}};
    const uint8_t notes[][3] = {{60,64,67},{57,60,64},{65,69,72},{67,71,74}};
    const int hints[] = {60,57,65,67};
    harmony_spec_t spec = {0,HARMONY_IONIAN,1,HARMONY_NO_PARENT};
    harmony_triad_t triad;
    for (int i = 0; i < 4; ++i) {
        spec.degree = (uint8_t)degrees[i];
        CHECK(harmony_resolve(&spec, hints[i], 48, 84, &triad));
        CHECK(memcmp(triad.pitch_class, pcs[i], 3) == 0);
        CHECK(memcmp(triad.midi, notes[i], 3) == 0);
    }
    spec.degree = 7;
    CHECK(harmony_resolve(&spec, 59, 48, 84, &triad));
    CHECK(triad.midi[1] - triad.midi[0] == 3);
    CHECK(triad.midi[2] - triad.midi[0] == 6); /* B diminished */
    spec.mode = HARMONY_AEOLIAN;
    spec.key = 9;
    spec.degree = 1;
    CHECK(harmony_resolve(&spec, 57, 48, 84, &triad));
    CHECK(memcmp(triad.midi, notes[1], 3) == 0); /* A minor tonic */
}

static void all_modes(void)
{
    harmony_triad_t triad, repeated;
    for (int key = 0; key < 12; ++key) {
        for (int mode = 0; mode < 7; ++mode) {
            for (int degree = 1; degree <= 7; ++degree) {
                harmony_spec_t spec = {(uint8_t)key,(uint8_t)mode,
                                       (uint8_t)degree,HARMONY_NO_PARENT};
                for (int hint = 0; hint <= 127; ++hint) {
                    CHECK(harmony_resolve(&spec, hint, 0, 127, &triad));
                    CHECK(harmony_resolve(&spec, hint, 0, 127, &repeated));
                    CHECK(memcmp(&triad, &repeated, sizeof triad) == 0);
                    for (int tone = 0; tone < 3; ++tone) {
                        CHECK(triad.pitch_class[tone] == expected(key,mode,degree,tone));
                        CHECK(triad.midi[tone] % 12 == triad.pitch_class[tone]);
                        CHECK(triad.midi[tone] <= 127);
                        if (tone) CHECK(triad.midi[tone] > triad.midi[tone-1]);
                    }
                    CHECK(triad.midi[1] - triad.midi[0] == 3 ||
                          triad.midi[1] - triad.midi[0] == 4);
                    CHECK(triad.midi[2] - triad.midi[0] == 6 ||
                          triad.midi[2] - triad.midi[0] == 7);
                    /* No eligible root is closer; lower wins ties. */
                    for (int root = triad.pitch_class[0]; root < 128; root += 12) {
                        int span = triad.midi[2] - triad.midi[0];
                        int d = root > hint ? root-hint : hint-root;
                        int best = triad.midi[0] > hint ? triad.midi[0]-hint : hint-triad.midi[0];
                        if (root + span <= 127) {
                            CHECK(d >= best);
                            if (d == best) CHECK(root >= triad.midi[0]);
                        }
                    }
                }
                const int bounds[][2] = {{0,6},{0,7},{120,127},{61,72},{36,48}};
                for (int r = 0; r < 5; ++r) {
                    int low = bounds[r][0], high = bounds[r][1], best = -1;
                    int span = (expected(key,mode,degree,2)-expected(key,mode,degree,0)+12)%12;
                    int distance = 256;
                    memset(&triad,0xa5,sizeof triad);
                    for (int root = low; root+span <= high; ++root) {
                        if (root%12 == expected(key,mode,degree,0)) {
                            int d = root > 60 ? root-60 : 60-root;
                            if (d < distance) { distance = d; best = root; }
                        }
                    }
                    CHECK(harmony_resolve(&spec,60,low,high,&triad) == (best >= 0));
                    if (best >= 0) {
                        CHECK(triad.midi[0] == best);
                        CHECK(triad.midi[2] <= high);
                        CHECK(triad.midi[2] == best+span);
                    } else {
                        memset(&repeated,0xa5,sizeof repeated);
                        CHECK(memcmp(&triad,&repeated,sizeof triad) == 0);
                    }
                }
                /* Explicit pentatonic parent must reuse ALL seven degrees. */
                CHECK(harmony_resolve(&spec,60,36,84,&triad));
                spec.parent_mode = spec.mode;
                for (int pent = HARMONY_MAJOR_PENTATONIC;
                     pent <= HARMONY_MINOR_PENTATONIC; ++pent) {
                    spec.mode = (uint8_t)pent;
                    CHECK(harmony_resolve(&spec,60,36,84,&repeated));
                    CHECK(memcmp(&triad,&repeated,sizeof triad) == 0);
                }
            }
        }
    }
}

static void strict_notes(void)
{
    const int transposes[] = {-127,-13,0,13,127};
    const int ranges[][2] = {{0,127},{24,48},{48,60},{120,127},{0,6},{61,61}};
    for (int key = 0; key < 12; ++key)
    for (int mode = 0; mode < 7; ++mode)
    for (int degree = 1; degree <= 7; ++degree) {
        harmony_spec_t spec = {(uint8_t)key,(uint8_t)mode,(uint8_t)degree,255};
        for (int r = 0; r < 6; ++r)
        for (int source = 0; source <= 127; ++source)
        for (int t = 0; t < 5; ++t)
        for (int follow = HARMONY_ROOT_ONLY; follow <= HARMONY_CHORD_TONES; ++follow) {
            int low = ranges[r][0], high = ranges[r][1], available = 0;
            int desired = source + transposes[t], best = -1, best_d = 1000;
            uint8_t note = 255, again = 255;
            /* Independent octave constraint and nearest-note brute force. */
            if (desired < low) desired += ((low-desired+11)/12)*12;
            if (desired > high) desired -= ((desired-high+11)/12)*12;
            if (desired < low) desired = low;
            for (int n = low; n <= high; ++n) {
                int allowed = n%12 == expected(key,mode,degree,0);
                if (follow == HARMONY_CHORD_TONES)
                    allowed |= n%12 == expected(key,mode,degree,1) ||
                               n%12 == expected(key,mode,degree,2);
                if (allowed) {
                    int d = n > desired ? n-desired : desired-n;
                    available = 1;
                    if (d < best_d) { best = n; best_d = d; }
                }
            }
            CHECK(harmony_resolve_note(&spec,source,transposes[t],low,high,
                  (harmony_note_mode_t)follow,&note) == available);
            if (available) {
                CHECK(note == best);
                CHECK(harmony_resolve_note(&spec,source,transposes[t],low,high,
                      (harmony_note_mode_t)follow,&again));
                CHECK(note == again);
            } else CHECK(note == 255);
        }
    }
    harmony_spec_t c = {0,HARMONY_IONIAN,1,255};
    uint8_t note;
    CHECK(harmony_resolve_note(&c,127,127,36,60,HARMONY_CHORD_TONES,&note));
    CHECK(note == 48); /* octave constrained to 50, tie snaps lower */
    CHECK(harmony_resolve_note(&c,0,-127,36,60,HARMONY_CHORD_TONES,&note));
    CHECK(note == 40);
    CHECK(harmony_resolve_note(&c,59,1,48,59,HARMONY_ROOT_ONLY,&note));
    CHECK(note == 48); /* adding transpose must not produce clamped B */
}

static void rejection(void)
{
    const harmony_spec_t invalid[] = {
        {12,0,1,255},{0,0,0,255},{0,0,8,255},{0,255,1,255},
        {0,HARMONY_CHROMATIC,1,HARMONY_IONIAN},
        {0,HARMONY_MAJOR_PENTATONIC,1,255},
        {0,HARMONY_MINOR_PENTATONIC,7,HARMONY_CHROMATIC}
    };
    harmony_triad_t triad, untouched;
    memset(&untouched,0xa5,sizeof untouched);
    for (unsigned i = 0; i < sizeof invalid/sizeof invalid[0]; ++i) {
        uint8_t note = 222;
        triad = untouched;
        CHECK(!harmony_resolve(&invalid[i],60,0,127,&triad));
        CHECK(memcmp(&triad,&untouched,sizeof triad) == 0);
        CHECK(!harmony_resolve_note(&invalid[i],60,0,0,127,HARMONY_ROOT_ONLY,&note));
        CHECK(note == 222);
    }
    harmony_spec_t c = {0,0,1,255};
    const int bad[][3] = {{-1,0,127},{128,0,127},{60,-1,127},
                         {60,0,128},{60,61,60},{60,61,62}};
    for (unsigned i = 0; i < sizeof bad/sizeof bad[0]; ++i) {
        triad = untouched;
        CHECK(!harmony_resolve(&c,bad[i][0],bad[i][1],bad[i][2],&triad));
        CHECK(memcmp(&triad,&untouched,sizeof triad) == 0);
    }
    CHECK(!harmony_resolve(NULL,60,0,127,&triad));
    CHECK(!harmony_resolve(&c,60,0,127,NULL));
    uint8_t note = 222;
    CHECK(!harmony_resolve_note(&c,-1,0,0,127,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,128,0,0,127,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,-128,0,127,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,128,0,127,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,0,-1,127,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,0,0,128,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,0,70,60,HARMONY_ROOT_ONLY,&note));
    CHECK(!harmony_resolve_note(&c,60,0,0,127,(harmony_note_mode_t)2,&note));
    CHECK(!harmony_resolve_note(NULL,60,0,0,127,HARMONY_ROOT_ONLY,&note));
    CHECK(note == 222);
    CHECK(!harmony_resolve_note(&c,60,0,0,127,HARMONY_ROOT_ONLY,NULL));
    triad = untouched;
    CHECK(!harmony_resolve(&c,60,0,6,&triad)); /* incomplete triad */
    CHECK(memcmp(&triad,&untouched,sizeof triad) == 0);
}

int main(void)
{
    progression(); all_modes(); strict_notes(); rejection();
    printf("harmony: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
