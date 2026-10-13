/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared 44.1 kHz sound strip. Browser candidate; device cost/placement not accepted. */
#pragma once
#include <stdint.h>

enum {
    SOUND_STRIP_ENABLED = 1,
    SOUND_STRIP_HIGHPASS = 2,
    SOUND_STRIP_LOWPASS = 4,
    SOUND_STRIP_LIMITER = 8,
    SOUND_STRIP_FLAGS = 15,
    SOUND_STRIP_FADE_SAMPLES = 220
};

/* Fixed eight-byte little-endian layout in the versioned variant save format. */
typedef struct {
    uint16_t highpass_hz;    /* 20..20000 Hz */
    uint16_t lowpass_hz;     /* 20..20000 Hz; crossover is allowed */
    uint8_t boost_db;       /* 0..24 dB */
    uint8_t ceiling_db;     /* 0..48 dB attenuation: 1 means -1 dBFS */
    uint8_t release_ms;     /* 10..250 ms; initial UI holds this at 80 ms */
    uint8_t flags;
} sound_strip_config_t;

/* 68 B on the supported 32-bit float ABI. No per-strip lookahead audio buffers. */
typedef struct {
    float hp_z[2], lp_z[2];
    float hp, lp, boost, ceiling;
    float hp_target, lp_target, boost_target, ceiling_target;
    float gain, release, mix;
    float guard_gain;       /* final linked guard survives interrupted wet fades */
    uint32_t flags;         /* low byte: active stages; next byte: desired config flags;
                            * upper 16 bits: bounded final-guard release countdown (0..220) */
} sound_strip_t;

/* Optional per-call output, owned by caller. gain_min is the minimum individual
 * linked limiter/guard stage gain, 0..1; it is not end-to-end wet/dry gain. */
typedef struct { float input_peak, output_peak, gain_min; } sound_strip_meter_t;

void sound_strip_config_default(sound_strip_config_t *config);
int sound_strip_config_valid(const sound_strip_config_t *config);
void sound_strip_init(sound_strip_t *strip);
/* Control/block boundary only, with single audio-thread ownership; invalid configs leave state intact. */
int sound_strip_set(sound_strip_t *strip, const sound_strip_config_t *config);
/* In-place stereo. l == r supports mono. Settled bypass never writes the buffers.
 * Enable/bypass/stage changes ramp for 220 samples per leg. Final guard removal
 * releases over 220 samples, including interrupted ramps; bypass includes this tail.
 * An enabled limiter
 * bounds the final mixed output at the requested sample ceiling during a ramp.
 * Input must be finite, with |sample| <= 64 (well beyond the normal engine range).
 * Apply once AFTER summing all voices/tails belonging to a sound, before its pan/sends.
 */
void sound_strip_process(sound_strip_t *strip, float *l, float *r, uint32_t count,
                         sound_strip_meter_t *meter);
int sound_strip_bypassed(const sound_strip_t *strip);
