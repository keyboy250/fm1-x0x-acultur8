/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#define RUNTIME_DIAGNOSTIC_WORDS 52u
/* Main context only. One bounded interrupt-guarded read; never advances audio,
 * drains commands, saves, clears counters or selects a part. */
int engine_runtime_snapshot(unsigned lane,uint32_t out[RUNTIME_DIAGNOSTIC_WORDS]); /* replaced below */
