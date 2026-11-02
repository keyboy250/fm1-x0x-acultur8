/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "fm1_cc.h"
/* Preserve the diagnostic publication/activation device barrier. */
FM1_INLINE void fm1_capture_barrier(void)
{
    __asm__ volatile("csync" ::: "memory");
}
