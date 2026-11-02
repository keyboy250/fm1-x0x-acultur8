/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "../dsp/hybrid_kick.h"
#include "../dsp/sound_strip.h"
typedef struct { uint8_t active,cfg[HK_COUNT]; uint32_t sample_fingerprint; } kick_settings_t;
typedef char kick_settings_size_44[(sizeof(kick_settings_t)==44)?1:-1];
static inline void kick_settings_defaults(kick_settings_t *k){unsigned i;k->active=0;k->sample_fingerprint=0;for(i=0;i<HK_COUNT;i++)k->cfg[i]=(uint8_t)hybrid_kick_param(i)->def;}
static inline int kick_settings_valid(const kick_settings_t *k){unsigned i;if(k->active>1)return 0;if(!k->active){if(k->sample_fingerprint)return 0;for(i=0;i<HK_COUNT;i++)if(k->cfg[i]!=hybrid_kick_param(i)->def)return 0;}for(i=0;i<HK_COUNT;i++)if(k->cfg[i]>hybrid_kick_param(i)->max)return 0;return k->cfg[HK_SOURCE]==4 ? (!k->active || k->sample_fingerprint!=0) : !k->sample_fingerprint;}

#define KICK_PRESET_SLOTS 16
/* Only declared original BD parameters are copied;unused sound bytes belong to lane storage. */
typedef struct { kick_settings_t kick; uint8_t base[16]; sound_strip_config_t strip; } kick_preset_t;
typedef char kick_preset_size_68[(sizeof(kick_preset_t)==68)?1:-1];
int project_kick_preset_save(unsigned slot);
int project_kick_preset_get(unsigned slot,kick_preset_t *preset);
uint32_t engine_kick_sample_fingerprint(void);
