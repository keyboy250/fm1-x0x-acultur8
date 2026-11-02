/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Physical frames/second used for musical time, not the DSP design sample rate.
 * Unset builds preserve nominal timing. A non-default value is an explicit
 * device-specific build profile; it is never saved into project/Undo state.
 */
#ifndef X0X_TIMING_RATE_HZ
#define X0X_TIMING_RATE_HZ 44100u
#endif

typedef struct {
 uint32_t rate_hz, stored_hz, generation, unit, record_status;
 float ticks_per_frame;
} timing_rate_state_t;
extern volatile timing_rate_state_t timing_rate_state;
static inline uint32_t timing_rate_get(void) { return timing_rate_state.rate_hz; }
static inline void timing_rate_activate(uint32_t hz) {
 timing_rate_state.ticks_per_frame=24000000.0f/(float)hz;
 timing_rate_state.rate_hz=hz;
}
#if X0X_TIMING_RATE_HZ < 43000 || X0X_TIMING_RATE_HZ > 45000
#error "X0X_TIMING_RATE_HZ must be an integer physical audio rate from 43000 to 45000"
#endif

static inline uint32_t timing_audio_ticks(uint32_t frames)
{
    /* At most two 512-frame DMA halves; float rounding is below one TIMER4
     * tick here. Avoid a new 64-bit divide in the event publication path.
     * TIMER4's nominal 24MHz rate remains independent of the audio profile.
     */
    if(timing_rate_get()==44100u)return (frames*80000u)/147u;
    return (uint32_t)((float)frames * timing_rate_state.ticks_per_frame);
}
