/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "../dsp/sound_strip.h"

#define VARIANT_MAGIC 0x31564341u /* ACV1, little endian */
#define VARIANT_SCHEMA 1u
#define VARIANT_STRIPS 25u
#define VARIANT_ABI 1u
typedef struct {
    uint32_t magic;
    uint16_t schema, bytes;
    sound_strip_config_t strip[VARIANT_STRIPS];
} variant_settings_t;
typedef char variant_size_208[(sizeof(variant_settings_t) == 208u) ? 1 : -1];

enum { VP_HIGHPASS, VP_LOWPASS, VP_BOOST, VP_CEILING, VP_RELEASE, VP_FLAGS, VP_COUNT };
enum { VC_SET_SOUND = 1, VC_SET_STRIP, VC_BEGIN_EDIT, VC_END_EDIT, VC_UNDO, VC_REDO, VC_SAVE };
enum { VR_OK = 0, VR_QUEUED = 1, VR_INVALID = -1, VR_UNSUPPORTED = -2, VR_BUSY = -3,
       VR_PROTECTED = -4, VR_FULL = -5, VR_IO = -6 };
enum { VS_INFO, VS_SOUND, VS_STRIP, VS_METERS };
enum { VCAP_SOUND = 1u, VCAP_STRIP_CONFIG = 2u, VCAP_UNDO = 4u, VCAP_SAVE = 8u, VCAP_STRIP_AUDIO = 16u };
typedef struct {
    uint16_t abi, op;
    uint32_t request;
    uint8_t target, voice, index, reserved;
    int32_t value;
} variant_command_t;
typedef struct {
    uint32_t request, revision, ticket;
    int32_t result;
} variant_reply_t;
/* Values use uint32 for stable bounded transport encoding, never raw project pointers. */
typedef struct {
    uint32_t abi, section, revision, count;
    uint32_t value[16];
} variant_snapshot_t;
typedef char variant_command_size_16[(sizeof(variant_command_t) == 16u) ? 1 : -1];
typedef char variant_reply_size_16[(sizeof(variant_reply_t) == 16u) ? 1 : -1];
typedef char variant_snapshot_size_80[(sizeof(variant_snapshot_t) == 80u) ? 1 : -1];

void variant_defaults(variant_settings_t *v);
int variant_valid(const variant_settings_t *v);
int variant_is_default(const variant_settings_t *v);
int variant_field_get(const sound_strip_config_t *s, int index);
int variant_field_set(sound_strip_config_t *s, int index, int value);
void variant_apply(void);
void variant_changed(void);
int variant_command(const variant_command_t *command, variant_reply_t *reply);
int variant_snapshot(int section, int target, int voice, variant_snapshot_t *snapshot);
