/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* ABI1 shared rate-family model: LAN2 projects, native CC and audio-owner latching. */
#define RF_VERSION 1
#define RF_COUNT 29
#define RF_INVALID 255u
enum { RF_STRAIGHT, RF_STRAIGHT_DOTTED, RF_STRAIGHT_TRIPLET, RF_ALL, RF_GRID, RF_FULL, RF_FAMILIES };
enum { RF_STEP, RF_BAR };
typedef struct { uint16_t pulses4; uint8_t kind; const char *label; } rf_division_t;
extern const rf_division_t rf_divisions[RF_COUNT];
unsigned rf_turn(unsigned family,unsigned rate,int direction);
int rf_allowed(unsigned family,unsigned rate);
unsigned rf_count(unsigned family);
unsigned rf_ordered(unsigned family,unsigned index);
unsigned rf_nearest(unsigned family,unsigned rate);
unsigned rf_bin(unsigned family,unsigned value);
/* Only prior bin is needed; first value is exact. 0..2 CC units hysteresis. */
unsigned rf_sweep(unsigned family,unsigned value,unsigned hysteresis,unsigned prior);
/* Bresenham Q8 intervals, with explicit remainder: four subunits per MIDI pulse.
 * Constant tick period gives <1 Q8 unit total error for any count. */
uint32_t rf_external_q8(unsigned rate,uint32_t tick_q8,uint8_t *remainder);
/* Internal timing uses exact integer BPM tenths, fixed rate/tempo denominator.
 * Caller resets remainder on tempo/rate change at a safe lane boundary. */
uint32_t rf_internal_q8(unsigned rate,unsigned fs,unsigned bpm_x10,uint32_t *remainder);
typedef struct { uint8_t selected,effective,pending,policy,bar_seen; } rf_latch_t;
int rf_select(rf_latch_t *state,unsigned rate);
/* Caller reports actual audio-owner step boundary; bar alone never truncates. */
int rf_boundary(rf_latch_t *state,int lane_step,int musical_bar);
