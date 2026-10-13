/* SPDX-License-Identifier: GPL-3.0-only */
#include "sound_strip.h"
#include "fastmath.h"

#define STRIP_FS 44100.0f
#define STRIP_STAGES (SOUND_STRIP_HIGHPASS | SOUND_STRIP_LOWPASS | SOUND_STRIP_LIMITER)
#define STRIP_SMOOTH (1.0f / 220.0f)
#define STRIP_GUARD_MASK 0xffff0000u
#define STRIP_GUARD_REMAINING(s) ((s)->flags >> 16)

static void guard_count(sound_strip_t *s, uint32_t remaining)
{
    s->flags = (s->flags & ~STRIP_GUARD_MASK) | (remaining << 16);
}
typedef char strip_config_is_eight_bytes[(sizeof(sound_strip_config_t) == 8) ? 1 : -1];
typedef char strip_state_is_sixty_eight_bytes[(sizeof(sound_strip_t) == 68) ? 1 : -1];

void sound_strip_config_default(sound_strip_config_t *c)
{
    c->highpass_hz = 20; c->lowpass_hz = 20000;
    c->boost_db = 0; c->ceiling_db = 1; c->release_ms = 80; c->flags = 0;
}

int sound_strip_config_valid(const sound_strip_config_t *c)
{
    return c && c->highpass_hz >= 20 && c->highpass_hz <= 20000 &&
        c->lowpass_hz >= 20 && c->lowpass_hz <= 20000 && c->boost_db <= 24 &&
        c->ceiling_db <= 48 && c->release_ms >= 10 && c->release_ms <= 250 &&
        !(c->flags & ~SOUND_STRIP_FLAGS);
}

static void clear_audio(sound_strip_t *s)
{
    s->hp_z[0] = s->hp_z[1] = s->lp_z[0] = s->lp_z[1] = 0.0f;
    s->gain = 1.0f;
}

static float cutoff(uint16_t hz)
{
    float g = fm_tanf(FM_PI * (float)hz / STRIP_FS);
    return g / (1.0f + g); /* topology-preserving one-pole coefficient, strictly 0..1 */
}

void sound_strip_init(sound_strip_t *s)
{
    sound_strip_config_t c;
    s->mix = 0.0f;
    s->guard_gain = 1.0f;
    s->flags = 0;
    /* set() initializes all remaining fields while mix is zero. No libc needed. */
    sound_strip_config_default(&c);
    sound_strip_set(s, &c);
}

int sound_strip_set(sound_strip_t *s, const sound_strip_config_t *c)
{
    uint32_t guard_flags = SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER;
    if (!s || !sound_strip_config_valid(c)) return -1;
    /* A near-zero wet mix may still have a full-strength final ceiling guard.
     * Release its actual gain independently instead of dropping it at dry. */
    if (((s->flags >> 8) & guard_flags) == guard_flags &&
        (c->flags & guard_flags) != guard_flags && s->guard_gain < 1.0f)
        guard_count(s, SOUND_STRIP_FADE_SAMPLES);
    s->hp_target = cutoff(c->highpass_hz);
    s->lp_target = cutoff(c->lowpass_hz);
    s->boost_target = fm_db2lin((float)c->boost_db);
    s->ceiling_target = fm_db2lin(-(float)c->ceiling_db);
    s->release = 1.0f - fm_expf(-1000.0f / ((float)c->release_ms * STRIP_FS));
    s->flags = (s->flags & (STRIP_STAGES | STRIP_GUARD_MASK)) | ((uint32_t)c->flags << 8);
    if (s->mix == 0.0f) {
        s->hp = s->hp_target; s->lp = s->lp_target;
        s->boost = s->boost_target; s->ceiling = s->ceiling_target;
        s->flags = (s->flags & ~STRIP_STAGES) | (c->flags & STRIP_STAGES);
        clear_audio(s);
    }
    return 0;
}

int sound_strip_bypassed(const sound_strip_t *s)
{
    return !(s->flags & ((uint32_t)SOUND_STRIP_ENABLED << 8)) && s->mix == 0.0f &&
        STRIP_GUARD_REMAINING(s) == 0 && s->guard_gain == 1.0f;
}

int sound_strip_active(const sound_strip_t *s)
{
    uint32_t desired = (s->flags >> 8) & SOUND_STRIP_FLAGS;
    uint32_t stages = s->flags & STRIP_STAGES;
    if (sound_strip_bypassed(s)) return 0;
    if (s->mix != ((desired & SOUND_STRIP_ENABLED) ? 1.0f : 0.0f) ||
        stages != (desired & STRIP_STAGES) || STRIP_GUARD_REMAINING(s) || s->guard_gain != 1.0f) return 1;
    if ((stages & SOUND_STRIP_LIMITER) && s->gain < 1.0f - 1e-6f) return 1;
    if ((stages & SOUND_STRIP_HIGHPASS) &&
        (fm_fabsf(s->hp_z[0]) > 1e-8f || fm_fabsf(s->hp_z[1]) > 1e-8f)) return 1;
    return (stages & SOUND_STRIP_LOWPASS) &&
        (fm_fabsf(s->lp_z[0]) > 1e-8f || fm_fabsf(s->lp_z[1]) > 1e-8f);
}

static float glide(float current, float target)
{
    float delta = target - current;
    return fm_fabsf(delta) < 1e-7f * fm_maxf(target, 1.0f) ? target : current + STRIP_SMOOTH * delta;
}

static float lowpass(float input, float g, float *z)
{
    float v = (input - *z) * g;
    float y = v + *z;
    *z = fm_flush(y + v);
    return y;
}

/* Same scale factor on both sides; the clamp only catches single-precision rounding. */
static float guard(sound_strip_t *s, float *l, float *r, float ceiling, int desired)
{
    float peak = fm_maxf(fm_fabsf(*l), fm_fabsf(*r));
    float want = desired && peak > ceiling ? ceiling / peak : 1.0f;
    uint32_t remaining = STRIP_GUARD_REMAINING(s);
    if (desired && want < s->guard_gain) {
        s->guard_gain = want; /* requested tighter bounds always attack immediately */
    } else if (remaining) {
        s->guard_gain += (want - s->guard_gain) / (float)remaining;
        guard_count(s, --remaining);
        if (!remaining) s->guard_gain = want;
    } else s->guard_gain = want;
    *l *= s->guard_gain; *r *= s->guard_gain;
    if (desired) { /* clamp only catches single-precision rounding */
        *l = fm_clampf(*l, -ceiling, ceiling);
        *r = fm_clampf(*r, -ceiling, ceiling);
    }
    return s->guard_gain;
}

void sound_strip_process(sound_strip_t *s, float *l, float *r, uint32_t count,
                         sound_strip_meter_t *meter)
{
    uint32_t i;
    if (meter) { meter->input_peak = meter->output_peak = 0.0f; meter->gain_min = 1.0f; }
    if (sound_strip_bypassed(s)) {
        if (meter) for (i = 0; i < count; i++)
            meter->input_peak = fm_maxf(meter->input_peak, fm_maxf(fm_fabsf(l[i]), fm_fabsf(r[i])));
        if (meter) meter->output_peak = meter->input_peak;
        return; /* preserve every input bit, including signed zero */
    }
    for (i = 0; i < count; i++) {
        float dry_l = l[i], dry_r = r[i], x = dry_l, y = dry_r;
        uint32_t desired = (s->flags >> 8) & SOUND_STRIP_FLAGS;
        uint32_t active = s->flags & STRIP_STAGES;
        int stage_change = active != (desired & STRIP_STAGES);
        int want_wet = (desired & SOUND_STRIP_ENABLED) && !stage_change;
        float bound, used_gain = 1.0f;
        if (want_wet) s->mix = fm_minf(1.0f, s->mix + STRIP_SMOOTH);
        else s->mix = fm_maxf(0.0f, s->mix - STRIP_SMOOTH);
        if (s->mix < 1e-6f) s->mix = 0.0f;
        if (s->mix > 1.0f - 1e-6f) s->mix = 1.0f;
        if (s->mix == 0.0f) {
            clear_audio(s);
            if (stage_change) s->flags = (s->flags & ~STRIP_STAGES) | (desired & STRIP_STAGES);
        } else {
            s->hp = glide(s->hp, s->hp_target); s->lp = glide(s->lp, s->lp_target);
            s->boost = glide(s->boost, s->boost_target); s->ceiling = glide(s->ceiling, s->ceiling_target);
            if (active & SOUND_STRIP_HIGHPASS) {
                x -= lowpass(x, s->hp, &s->hp_z[0]); y -= lowpass(y, s->hp, &s->hp_z[1]);
            }
            if (active & SOUND_STRIP_LOWPASS) {
                x = lowpass(x, s->lp, &s->lp_z[0]); y = lowpass(y, s->lp, &s->lp_z[1]);
            }
            x *= s->boost; y *= s->boost;
            if (active & SOUND_STRIP_LIMITER) {
                float peak = fm_maxf(fm_fabsf(x), fm_fabsf(y));
                float want;
                bound = fm_minf(s->ceiling, s->ceiling_target);
                want = peak > bound ? bound / peak : 1.0f;
                if (want < s->gain) s->gain = want;
                else {
                    float next = s->gain + (want - s->gain) * s->release;
                    /* Single-precision release eventually rounds to its current value.
                     * Finish that converged release so silent-source activity is finite. */
                    s->gain = next == s->gain ? want : next;
                }
                x *= s->gain; y *= s->gain;
                used_gain = s->gain;
            }
            x = dry_l + (x - dry_l) * s->mix;
            y = dry_r + (y - dry_r) * s->mix;
        }
        /* Ceiling changes/enabling must bound the mixed output immediately. */
        if (STRIP_GUARD_REMAINING(s) || (desired & (SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER)) ==
                       (SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER)) {
            int guard_desired = (desired & (SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER)) ==
                       (SOUND_STRIP_ENABLED | SOUND_STRIP_LIMITER);
            float final_gain = guard(s, &x, &y, fm_minf(s->ceiling, s->ceiling_target), guard_desired);
            used_gain = fm_minf(used_gain, final_gain);
        }
        /* Once the fade reaches dry, do not rewrite either input sample. */
        if (s->mix != 0.0f || (desired & SOUND_STRIP_ENABLED) || s->guard_gain != 1.0f) { l[i] = x; r[i] = y; }
        if (meter) {
            meter->input_peak = fm_maxf(meter->input_peak, fm_maxf(fm_fabsf(dry_l), fm_fabsf(dry_r)));
            meter->output_peak = fm_maxf(meter->output_peak, fm_maxf(fm_fabsf(x), fm_fabsf(y)));
            meter->gain_min = fm_minf(meter->gain_min, used_gain);
        }
    }
}
