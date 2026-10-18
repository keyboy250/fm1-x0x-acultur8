/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* LAN2: same64 slice choices0..8, lossless26-byte representation. */
int rf_slice_pack(uint8_t packed[26],const uint8_t slices[64]);
int rf_slice_get(const uint8_t packed[26],unsigned step);
int rf_slice_unpack(uint8_t slices[64],const uint8_t packed[26]);
