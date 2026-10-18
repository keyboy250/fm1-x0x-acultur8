// SPDX-License-Identifier: GPL-3.0-only
// Single-threaded standalone sequencer fixture; no platform or IRQ exists.
#include <stdint.h>
uint32_t plat_control_enter(void){return 0;}
void plat_control_leave(uint32_t saved){(void)saved;}
