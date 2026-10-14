/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
enum { CC_SOURCES=3, CC_APPLIED=0, CC_REJECTED=-1, CC_UNSUPPORTED=-2, CC_BUSY=-3 };
typedef struct { uint32_t accepted,rejected,unsupported,busy,coalesced,overflow; } cc_stats_t;
extern cc_stats_t cc_stats;
/* Main context only. IRQ producers may only publish input packets. */
int cc_message(uint32_t packet,uint8_t source);
int cc_parameter(uint16_t number,uint32_t value,uint8_t source);
void cc_reset_source(uint8_t source);
int cc_coalescable(uint32_t packet);
int cc_recover(void);
void plat_midi_service(void);
void plat_midi_clear_cc_source(uint8_t source);
uint32_t cc_source_field(uint8_t source,unsigned field);
