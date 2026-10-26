/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "timing_rate.h"

/* TIMER4 runs at nominal 24MHz; audio uses the explicit physical-rate profile.
 * A DMA free half is rendered one half ahead of playback. Stamp every event
 * before publishing its ring slot, keeping Start/notes/clock in sample order.
 * Zero is an immediate deadline (main-context output / retired transport).
 * Outstanding audio deadlines are at most two halves ahead (<24ms).
 */
typedef struct {
    uint32_t due[64];
    uint32_t base, frame, chunk, active;
} midi_deadline_t;

static inline void midi_deadline_begin(midi_deadline_t *s,uint32_t now)
{
    s->base=now+timing_audio_ticks(512u);
    s->frame=s->chunk=0;
    s->active=1;
}
static inline uint32_t midi_deadline_value(const midi_deadline_t *s)
{
    uint32_t due;
    if(!s->active)return 0;
    due=s->base+timing_audio_ticks(s->frame+s->chunk);
    return due?due:1u; /* reserve zero for immediate output even at TIMER4 wrap */
}
static inline int midi_deadline_ready(uint32_t due,uint32_t now)
{
    /* No valid future deadline is more than two DMA halves away. Treat an
     * older wrapped deadline as overdue after arbitrarily long backpressure. */
    return !due || (int32_t)(now-due)>=0 || due-now>600000u;
}
