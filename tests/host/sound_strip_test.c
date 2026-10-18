/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/dsp/sound_strip.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FS 44100
#define PI 3.14159265358979323846
#define N 256
static float left[N], right[N];

static void fill(float l, float r)
{
    int i;
    for (i = 0; i < N; i++) { left[i] = l; right[i] = r; }
}

static sound_strip_config_t config(int flags)
{
    sound_strip_config_t c;
    sound_strip_config_default(&c);
    c.flags = (uint8_t)flags;
    return c;
}

static void settle(sound_strip_t *s)
{
    int i;
    for (i = 0; i < 20; i++) { fill(0.0f, 0.0f); sound_strip_process(s, left, right, N, 0); }
}

static void bypass_and_validation(void)
{
    sound_strip_t s, previous;
    sound_strip_config_t c = config(0);
    float original_l[N], original_r[N];
    int i;
    assert(sizeof(c) == 8 && sizeof(s) == 68);
    sound_strip_init(&s);
    for (i = 0; i < N; i++) { left[i] = (float)(i - 127) / 37.0f; right[i] = -left[i]; }
    left[0] = -0.0f; right[0] = 0.0f;
    memcpy(original_l, left, sizeof(left)); memcpy(original_r, right, sizeof(right));
    sound_strip_process(&s, left, right, N, 0);
    assert(!memcmp(left, original_l, sizeof(left)) && !memcmp(right, original_r, sizeof(right)));
    assert(sound_strip_bypassed(&s));
    previous = s;
    c.highpass_hz = 19; assert(sound_strip_set(&s, &c) == -1);
    assert(!memcmp(&previous, &s, sizeof(s)));
    c = config(0); c.flags = 16; assert(!sound_strip_config_valid(&c));
    c = config(0); c.boost_db = 25; assert(!sound_strip_config_valid(&c));
    c = config(0); c.ceiling_db = 49; assert(!sound_strip_config_valid(&c));
    c = config(0); c.release_ms = 9; assert(!sound_strip_config_valid(&c));
    assert(!sound_strip_config_valid(0));
    puts("strip: 8-byte config, 68-byte state, exact disabled parity and invalid-config atomicity passed");
}

static void silence_and_dc(void)
{
    sound_strip_t s;
    sound_strip_config_t c = config(SOUND_STRIP_FLAGS);
    int i, k;
    c.highpass_hz = 80; c.lowpass_hz = 8000; c.boost_db = 24;
    sound_strip_init(&s); assert(!sound_strip_set(&s, &c)); settle(&s);
    for (i = 0; i < N; i++) assert(left[i] == 0.0f && right[i] == 0.0f);
    for (k = 0; k < 200; k++) { fill(.2f, -.1f); sound_strip_process(&s, left, right, N, 0); }
    /* Float TPT state settles within approximately one 16-bit PCM unit at +24 dB. */
    for (i = 0; i < N; i++) assert(fabsf(left[i]) < .00004f && fabsf(right[i]) < .00004f);
    puts("strip: silence remains zero and DC is removed by the high-pass");
}

static double response(int highpass, int hz)
{
    sound_strip_t s;
    sound_strip_config_t c = config(SOUND_STRIP_ENABLED | (highpass ? SOUND_STRIP_HIGHPASS : SOUND_STRIP_LOWPASS));
    double in2 = 0.0, out2 = 0.0;
    int i;
    c.highpass_hz = c.lowpass_hz = 1000;
    sound_strip_init(&s); assert(!sound_strip_set(&s, &c));
    for (i = 0; i < FS; i++) {
        float x = (float)(.2 * sin(2.0 * PI * hz * i / FS)), y = x;
        float original = x;
        sound_strip_process(&s, &x, &y, 1, 0);
        assert(fabsf(x - y) < 1e-7f);
        if (i >= FS / 2) { in2 += (double)original * original; out2 += (double)x * x; }
    }
    return sqrt(out2 / in2);
}

static void frequency_response(void)
{
    const int frequencies[] = {50, 1000, 8000};
    int hp, i;
    for (hp = 0; hp <= 1; hp++) for (i = 0; i < 3; i++) {
        double w = tan(PI * frequencies[i] / FS) / tan(PI * 1000 / FS);
        double expected = (hp ? w : 1.0) / sqrt(1.0 + w * w);
        double measured = response(hp, frequencies[i]);
        assert(fabs(measured - expected) < .002);
        printf("strip: %s 1000 Hz, input %d Hz: measured %.6f, expected %.6f\n",
               hp ? "HPF" : "LPF", frequencies[i], measured, expected);
    }
}

static void boost_ceiling_and_stereo(void)
{
    sound_strip_t s;
    sound_strip_config_t c = config(SOUND_STRIP_ENABLED);
    sound_strip_meter_t m;
    int i, k;
    c.boost_db = 6;
    sound_strip_init(&s); assert(!sound_strip_set(&s, &c)); settle(&s);
    fill(.03f, -.015f); sound_strip_process(&s, left, right, N, &m);
    assert(fabsf(left[N-1] - .0598579f) < 1e-6f);
    c.flags |= SOUND_STRIP_LIMITER; c.boost_db = 24; c.ceiling_db = 12;
    assert(!sound_strip_set(&s, &c)); settle(&s);
    for (k = 0; k < 12; k++) {
        fill(0.0f, 0.0f);
        /* Several voices/tails of ONE sound have already been summed here. */
        for (i = 0; i < N; i += 11) { left[i] = .8f + .9f + .7f; right[i] = -left[i] * .25f; }
        sound_strip_process(&s, left, right, N, &m);
        for (i = 0; i < N; i++) {
            assert(fabsf(left[i]) <= .251189f && fabsf(right[i]) <= .251189f);
            assert(fabsf(right[i] + left[i] * .25f) < 1e-6f);
        }
        assert(m.input_peak > 2.39f && m.output_peak <= .251189f && m.gain_min < .02f);
    }
    /* The right channel can be the detector, without moving the stereo ratio. */
    fill(.125f, -2.0f); sound_strip_process(&s, left, right, N, 0);
    for (i = 0; i < N; i++) assert(fabsf(left[i] + right[i] * .0625f) < 1e-6f);
    /* New lower ceiling applies even on the very first sample, through a control ramp. */
    c.ceiling_db = 36; assert(!sound_strip_set(&s, &c));
    fill(2.0f, -1.0f); sound_strip_process(&s, left, right, N, 0);
    for (i = 0; i < N; i++) assert(fabsf(left[i]) <= .015849f && fabsf(right[i]) <= .015849f);
    puts("strip: boost, summed simultaneous peaks, stereo link and immediate lower ceiling passed");
}

static void release_and_toggles(void)
{
    sound_strip_t s;
    sound_strip_config_t c = config(SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER);
    float previous, x, y, gain0;
    int i;
    c.ceiling_db = 6; c.release_ms = 80;
    sound_strip_init(&s); assert(!sound_strip_set(&s, &c)); settle(&s);
    x = y = 4.0f; sound_strip_process(&s, &x, &y, 1, 0); gain0 = s.gain;
    previous = gain0;
    for (i = 0; i < 3528; i++) {
        x = y = .01f; sound_strip_process(&s, &x, &y, 1, 0);
        assert(s.gain >= previous && s.gain <= 1.0f);
        assert(s.gain - previous < .0003f);
        previous = s.gain;
    }
    assert(fabsf(s.gain - (1.0f - (1.0f - gain0) * expf(-1.0f))) < .001f);
    c = config(SOUND_STRIP_ENABLED | SOUND_STRIP_HIGHPASS); c.highpass_hz = 1000;
    assert(!sound_strip_set(&s, &c));
    for (i = 0; i < 3000; i++) { x = y = .25f; sound_strip_process(&s, &x, &y, 1, 0); }
    previous = x; c.flags = 0; assert(!sound_strip_set(&s, &c));
    for (i = 0; i < SOUND_STRIP_FADE_SAMPLES + 2; i++) {
        x = y = .25f; sound_strip_process(&s, &x, &y, 1, 0);
        assert(fabsf(x - previous) < .0013f); previous = x;
    }
    assert(sound_strip_bypassed(&s) && x == .25f);
    c.flags = SOUND_STRIP_ENABLED | SOUND_STRIP_LOWPASS; c.lowpass_hz = 100;
    assert(!sound_strip_set(&s, &c)); previous = .25f;
    for (i = 0; i < 1000; i++) {
        x = y = .25f; sound_strip_process(&s, &x, &y, 1, 0);
        assert(fabsf(x - previous) < .002f); previous = x;
    }
    /* Change active filter stages while a steady stream continues. */
    c.flags = SOUND_STRIP_ENABLED | SOUND_STRIP_HIGHPASS;
    assert(!sound_strip_set(&s, &c));
    for (i = 0; i < 1000; i++) {
        x = y = .25f; sound_strip_process(&s, &x, &y, 1, 0);
        assert(fabsf(x - previous) < .0014f); previous = x;
    }
    puts("strip: 80 ms monotonic release, smooth active bypass/enable and stage switching passed");
}

static void sweeping_and_mono(void)
{
    sound_strip_t s, mono;
    sound_strip_config_t c = config(SOUND_STRIP_FLAGS);
    unsigned state = 7;
    int k, i;
    sound_strip_init(&s);
    for (k = 0; k < 800; k++) {
        state = state * 1664525u + 1013904223u;
        c.highpass_hz = (uint16_t)(20 + state % 19981u);
        state = state * 1664525u + 1013904223u;
        c.lowpass_hz = (uint16_t)(20 + state % 19981u);
        c.boost_db = (uint8_t)(k % 25); c.ceiling_db = (uint8_t)(k % 49);
        c.release_ms = (uint8_t)(10 + k % 241);
        assert(!sound_strip_set(&s, &c));
        for (i = 0; i < N; i++) { left[i] = (float)sin((k * N + i) * .1); right[i] = -left[i] * .5f; }
        sound_strip_process(&s, left, right, N, 0);
        for (i = 0; i < N; i++) {
            assert(isfinite(left[i]) && isfinite(right[i]));
            assert(fabsf(left[i]) <= s.ceiling_target + 1e-7f);
            assert(fabsf(right[i] + left[i] * .5f) < 1e-6f);
        }
    }
    c = config(SOUND_STRIP_FLAGS); c.boost_db = 6;
    sound_strip_init(&s); sound_strip_init(&mono);
    assert(!sound_strip_set(&s, &c)); assert(!sound_strip_set(&mono, &c));
    for (k = 0; k < 20; k++) {
        float one[N];
        for (i = 0; i < N; i++) one[i] = left[i] = right[i] = (float)sin((k * N + i) * .08);
        sound_strip_process(&s, left, right, N, 0);
        sound_strip_process(&mono, one, one, N, 0);
        assert(!memcmp(left, one, sizeof(one)) && !memcmp(right, one, sizeof(one)));
    }
    puts("strip: 800 bounded cutoff/gain/release sweeps and mono/stereo equality passed");
}

static void interrupted_guard_release(void)
{
    const int enabled = SOUND_STRIP_ENABLED, limiter = SOUND_STRIP_LIMITER;
    const int depths[] = {1, 7, 110, 219, 220};
    int depth, mode, i;
    for (depth = 0; depth < 5; depth++) for (mode = 0; mode < 2; mode++) {
        sound_strip_t s;
        sound_strip_config_t c = config(enabled | limiter);
        float previous = 0.0f;
        sound_strip_init(&s); assert(!sound_strip_set(&s, &c));
        for (i = 0; i < depths[depth]; i++) {
            float x = 4.0f, y = -2.0f;
            sound_strip_process(&s, &x, &y, 1, 0);
            assert(x <= .891252f); previous = x;
        }
        c.flags = (uint8_t)(mode ? 0 : enabled);
        assert(!sound_strip_set(&s, &c));
        if (depths[depth] == 1 && mode) assert(!sound_strip_bypassed(&s));
        for (i = 0; i < 2 * SOUND_STRIP_FADE_SAMPLES + 1; i++) {
            float x = 4.0f, y = -2.0f;
            uint32_t remaining = s.flags >> 16;
            /* Repeated controls preserve the pending countdown instead of resetting/dropping it. */
            if (i < SOUND_STRIP_FADE_SAMPLES) {
                c.lowpass_hz = (uint16_t)(19000 + i);
                assert(!sound_strip_set(&s, &c));
                assert((s.flags >> 16) == remaining);
            }
            sound_strip_process(&s, &x, &y, 1, 0);
            assert(x >= previous - .02f && fabsf(x - previous) < .04f);
            assert(fabsf(y + x * .5f) < 1e-6f);
            assert((s.flags >> 16) <= SOUND_STRIP_FADE_SAMPLES);
            previous = x;
            if (depths[depth] == 1 && i == SOUND_STRIP_FADE_SAMPLES - 2)
                assert(s.guard_gain < 1.0f && (s.flags >> 16) == 1);
            if (depths[depth] == 1 && i == SOUND_STRIP_FADE_SAMPLES - 1)
                assert(s.guard_gain == 1.0f && !(s.flags >> 16));
        }
        assert(previous == 4.0f && s.guard_gain == 1.0f && !(s.flags >> 16));
        if (mode) assert(sound_strip_bypassed(&s));
    }
    /* Interrupt removal/re-enable repeatedly, then require a finite exact bypass.
     * Above-ceiling enable/tightening may attack immediately; each release must be smooth. */
    {
        sound_strip_t block, singles;
        sound_strip_config_t c = config(enabled | limiter);
        float previous = 0.0f;
        int k;
        sound_strip_init(&block); sound_strip_init(&singles);
        for (k = 0; k < 40; k++) {
            c.flags = (uint8_t)(k % 2 ? 0 : enabled | limiter);
            c.ceiling_db = (uint8_t)(k % 2 ? 1 : 1 + k % 12);
            assert(!sound_strip_set(&block, &c)); assert(!sound_strip_set(&singles, &c));
            fill(4.0f, -2.0f);
            sound_strip_process(&block, left, right, 7, 0);
            for (i = 0; i < 7; i++) {
                float x = 4.0f, y = -2.0f;
                sound_strip_process(&singles, &x, &y, 1, 0);
                assert(x == left[i] && y == right[i]);
                if (k % 2) assert(fabsf(x - previous) < .04f);
                else assert(x <= powf(10.0f, -(float)c.ceiling_db / 20.0f) + 1e-6f);
                previous = x;
            }
        }
        c.flags = 0; assert(!sound_strip_set(&block, &c));
        fill(4.0f, -2.0f); sound_strip_process(&block, left, right, N, 0);
        assert(sound_strip_bypassed(&block));
        left[0] = -0.0f; right[0] = 0.0f;
        sound_strip_process(&block, left, right, 1, 0);
        assert(signbit(left[0]) && !signbit(right[0]));
    }
    puts("strip: interrupted 1/7/110/219/220-sample enables, guard removal, rapid toggles, immediate ceiling and finite exact bypass passed");
}

static void smooth_gain_and_block_boundaries(void)
{
    sound_strip_t block, singles;
    sound_strip_config_t c = config(SOUND_STRIP_ENABLED);
    float previous = .01f;
    int i, k;
    sound_strip_init(&block); assert(!sound_strip_set(&block, &c)); settle(&block);
    c.boost_db = 24; assert(!sound_strip_set(&block, &c));
    for (i = 0; i < 3000; i++) {
        float x = .01f, y = .01f;
        sound_strip_process(&block, &x, &y, 1, 0);
        assert(x >= previous && x - previous < .0007f);
        previous = x;
    }
    assert(fabsf(previous - .1584893f) < .00001f);
    c = config(SOUND_STRIP_FLAGS); c.highpass_hz = 120; c.lowpass_hz = 4000; c.boost_db = 12;
    sound_strip_init(&block); sound_strip_init(&singles);
    assert(!sound_strip_set(&block, &c)); assert(!sound_strip_set(&singles, &c));
    for (k = 0; k < 10; k++) {
        float one_l[N], one_r[N];
        for (i = 0; i < N; i++) {
            one_l[i] = left[i] = (float)sin((k * N + i) * .1);
            one_r[i] = right[i] = -one_l[i] * .3f;
        }
        if (k == 4 || k == 6) {
            c.flags = (uint8_t)(k == 4 ? 0 : SOUND_STRIP_FLAGS);
            assert(!sound_strip_set(&block, &c)); assert(!sound_strip_set(&singles, &c));
        }
        sound_strip_process(&block, left, right, N, 0);
        for (i = 0; i < N; i++) sound_strip_process(&singles, one_l + i, one_r + i, 1, 0);
        assert(!memcmp(left, one_l, sizeof(left)) && !memcmp(right, one_r, sizeof(right)));
    }
    puts("strip: live +24 dB gain ramp and block-size-independent audio/toggles passed");
}

static void benchmark(void)
{
    sound_strip_t strips[25];
    sound_strip_config_t c = config(SOUND_STRIP_FLAGS);
    clock_t start;
    double seconds;
    volatile float checksum = 0.0f;
    int b, k, i;
    c.highpass_hz = 80; c.lowpass_hz = 12000; c.boost_db = 6; c.ceiling_db = 3;
    for (k = 0; k < 25; k++) { sound_strip_init(&strips[k]); assert(!sound_strip_set(&strips[k], &c)); }
    start = clock();
    for (b = 0; b < 1000; b++) for (k = 0; k < 25; k++) {
        for (i = 0; i < N; i++) { left[i] = (float)(i % 31) / 31.0f; right[i] = -left[i] * .7f; }
        sound_strip_process(&strips[k], left, right, N, 0);
        checksum += left[N-1];
    }
    seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("strip benchmark (HOST ONLY): 25 stereo strips, %d frames each, %.3f CPU s, %.2f%% of 44.1 kHz real time; state %u B + config %u B per strip; checksum %.3f\n",
           1000 * N, seconds, seconds * FS / (1000 * N) * 100.0,
           (unsigned)sizeof(sound_strip_t), (unsigned)sizeof(sound_strip_config_t), (double)checksum);
}

int main(void)
{
    bypass_and_validation(); silence_and_dc(); frequency_response();
    boost_ceiling_and_stereo(); release_and_toggles(); sweeping_and_mono();
    smooth_gain_and_block_boundaries(); interrupted_guard_release(); benchmark();
    puts("sound strip: all host checks passed; engine integration and physical device cost are pending");
    return 0;
}
