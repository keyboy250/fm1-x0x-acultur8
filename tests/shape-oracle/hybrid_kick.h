/* SPDX-License-Identifier: GPL-3.0-only */
/* Private session-only kick audition. Does not occupy saved project bytes. */
#pragma once
#include <stdint.h>
#include "x0x_param.h"
#ifndef X0X_POOL
#define X0X_POOL
#endif
enum {HK_ENABLE,HK_SOURCE,HK_BLEND,HK_PITCH,HK_DECAY,HK_SWEEP,HK_FILTER,HK_CUTOFF,
 HK_VCUT,HK_VPITCH,HK_VDECAY,HK_VBLEND,HK_LOW,HK_MID,HK_HIGH,HK_CHANCE,HK_RANGE,HK_TIERS,
 HK_ATTACK,HK_PUNCH,HK_PTIME,HK_TAIL,HK_LEVEL,HK_VPUNCH,HK_COUNT};
typedef struct {
 uint8_t cfg[HK_COUNT];
 const uint8_t *sample;
 uint32_t sample_n,sample_i;
 float sample_fraction,sample_step;
 int32_t pred,idx;
 float phase,hz,sweep,env,decay,mix,vol,cutoff,z1,z2,punch,punch_decay,tail,tail_decay;
 uint32_t elapsed,left,rng;
 int16_t previous;
 uint8_t active;
} hybrid_kick_t;
const x0x_param_t *hybrid_kick_param(int field);
