/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Single producer; same compiler publication barrier as the USB rings. Consumer
 * can only free capacity. Neither function spins, allocates, or partially publishes. */
static inline int midi_ring_try_put(uint32_t *q,volatile uint32_t *write,const volatile uint32_t *read,
                                   uint32_t capacity,uint32_t packet,int connected)
{
    uint32_t w=*write;
    if(!connected || w-*read>=capacity)return 0;
    q[w%capacity]=packet;__asm__ volatile("" ::: "memory");*write=w+1;return 1;
}
static inline int midi_ring_try_pair(uint32_t *q,volatile uint32_t *write,const volatile uint32_t *read,
                                    uint32_t capacity,uint32_t first,uint32_t second,int connected)
{
    uint32_t w=*write;
    if(!connected || capacity<2 || w-*read>capacity-2)return 0;
    q[w%capacity]=first;q[(w+1)%capacity]=second;
    __asm__ volatile("" ::: "memory");*write=w+2;return 1;
}
