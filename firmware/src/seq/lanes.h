/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "pattern.h"
/* Main-context transactions: size sharing, never an audio/DSP fast-math change. */
#ifdef __clang__
#define POLY_COLD __attribute__((noinline,minsize))
#else
#define POLY_COLD __attribute__((noinline))
#endif
#define POLY_LANES 25
#define POLY_ROWS 23
#define POLY_PAYLOAD 1056u
#define POLY_BYTES 1070u
#define POLY_MAGIC 0x314e414cu /* LAN1 */
#define POLY_FLAG 128u
/* These cells are owned only with POLY_FLAG. No descriptor or TAPE macro cell
 * belongs to the slab. The pointer is the existing sound_t byte matrix. */
unsigned poly_sound_offset(unsigned cell);
int poly_cell_owned(unsigned sound_offset);
int poly_valid(const pattern_t *p,const uint8_t *sound);
void poly_enable(pattern_t *p,uint8_t *sound);
int poly_mode(const uint8_t *sound);
void poly_mode_set(uint8_t *sound,int enabled);
int poly_length(const pattern_t *p,const uint8_t *sound,int pattern,int lane);
int poly_rate(const pattern_t *p,const uint8_t *sound,int pattern,int lane);
void poly_set(pattern_t *p,uint8_t *sound,int pattern,int lane,int length,int rate);
int poly_break_gate(const pattern_t *p,const uint8_t *sound,int pattern,int step);
int poly_break_slice(const pattern_t *p,const uint8_t *sound,int pattern,int step);
void poly_break_set(pattern_t *p,uint8_t *sound,int pattern,int step,int gate,int slice);
int poly_part(int lane);
int poly_expanded(const uint8_t *sound);
int poly_expand(pattern_t *patterns,uint8_t *sound);
/* LAN2 must be enabled while transport is reserved/stopped. Main writes are guarded.
 * Per-sound policy shared across patterns: family, boundary, hysteresis. */
int poly_rate_config(const pattern_t *patterns,const uint8_t *sound,int lane,int field);
void poly_rate_config_set(pattern_t *patterns,uint8_t *sound,int lane,int field,int value);
