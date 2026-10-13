/* SPDX-License-Identifier: GPL-3.0-only */
/* PCM slice audition engine. Browser uses PCM in RAM; physical FM1 storage integration is separate. */
#pragma once
#include <stdint.h>
#define MARKED_MAX_SLICES 64
#define MARKED_VOICES 8
#define MARKED_MAX_FRAMES 1440000u
typedef struct { uint32_t start, end, time; } marked_slice_t;
typedef struct { uint32_t pos, end; float frac; uint8_t on; } marked_voice_t;
typedef struct {
    const float *pcm;
    uint32_t frames, rate, length, cursor, triggers, last_frame;
    uint8_t channels, count, next, playing, repeat, last_slice;
    marked_slice_t slices[MARKED_MAX_SLICES];
    marked_voice_t voice[MARKED_VOICES];
} marked_player_t;
int marked_load(marked_player_t *p, const float *pcm, uint32_t frames, uint32_t rate, int channels,
                const marked_slice_t *slices, int count, uint32_t length);
void marked_stop(marked_player_t *p);
void marked_play(marked_player_t *p, int repeat);
int marked_trigger(marked_player_t *p, int slice);
void marked_render(marked_player_t *p, float *l, float *r, uint32_t n);
