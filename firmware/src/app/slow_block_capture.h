/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "slow_block_diag.h"
/* Shared state machine: host adapters and the target platform inline exactly
 * these kernels. Platform owns boundary ordering and final counter sampling. */
#if X0X_DEVICE_TIMING
#include "../../hal/fm1_capture_barrier.h"
#define SBD_CAPTURE_BARRIER() fm1_capture_barrier()
#else
#define SBD_CAPTURE_BARRIER() __asm__ volatile("" ::: "memory")
#endif
static inline __attribute__((always_inline)) void slow_block_begin_inline(uint32_t start,uint32_t patterns,uint32_t epoch)
{
    /* Precondition: boot initialization or prior render_end left flags inactive. */
    slow_block_state.wait_sum=slow_block_state.wait_max=slow_block_state.nested=slow_block_state.nsub=0;
    slow_block_state.start=start; /* end is private scratch, set by render_end. */
    slow_block_state.patterns=patterns;slow_block_state.epoch=epoch;
    SBD_CAPTURE_BARRIER();slow_block_state.flags=SBD_ACTIVE;
}
static inline __attribute__((always_inline)) int slow_block_finish_inline(uint32_t half,uint32_t guard,uint32_t drops)
{
    uint32_t ticks=slow_block_state.end-slow_block_state.start,reason=0,sequence;
    uint32_t guard_before=slow_block_state.guard_before,drop_before=slow_block_state.drop_before;
    /* Single audio owner: carry final counters into the next half, even on early returns. */
    slow_block_state.guard_before=guard;slow_block_state.drop_before=drops;
    /* Stopped halves remain suppressed unless the DMA dropout counter changed.
     * Retain those actual events with the existing paired stage/worker/workload
     * record; patterns flags identify transport state at block start. */
    if(!(slow_block_state.patterns&(SBD_PLAYING|SBD_START_PENDING)) && drops==drop_before){slow_block_stage_advance(0);return 0;}
    if(guard!=guard_before)reason|=SBD_GUARD;
    if(drops!=drop_before)reason|=SBD_DROPOUT;
    /* Same integer load>92 boundary, folded for the fixed platform budget. */
    if(ticks>=SBD_SLOW_TICKS)reason|=SBD_SLOW;
    if(!reason){slow_block_stage_advance(0);return 0;}
    /* Retain a dropout-correlated half until another dropout or boot reset.
     * Rolling counter baselines above still advance on EVERY half. */
    if(!(reason&SBD_DROPOUT) && (slow_block_state.record[19]&SBD_DROPOUT)){slow_block_stage_advance(0);return 0;}
    /* Only retained events need publication metadata. No extra persistent state. */
    uint32_t cores=(uint32_t)plat_cpu2_on()|((uint32_t)plat_cpu2_gave_up()<<8);
    uint32_t rate=timing_rate_get();
    sequence=slow_block_state.sequence+1u;
    slow_block_state.sequence=sequence;SBD_CAPTURE_BARRIER();
    slow_block_state.record[0]=SBD_MAGIC;
    slow_block_state.record[1]=sequence+1u;
    /* [2] boot session is initialized before interrupts, never changed by capture. */
    slow_block_state.record[3]=half;
    slow_block_state.record[4]=slow_block_state.patterns;
    slow_block_state.record[5]=ticks;
    slow_block_state.record[6]=slow_block_state.wait_sum;
    slow_block_state.record[7]=slow_block_state.wait_max;
    slow_block_state.record[8]=slow_block_state.nested;
    slow_block_state.record[9]=slow_block_state.nsub;
    slow_block_state.record[10]=guard_before;
    slow_block_state.record[11]=guard;
    slow_block_state.record[12]=drop_before;
    slow_block_state.record[13]=drops;
    slow_block_state.record[14]=cores|0x80000000u;
    slow_block_state.record[15]=slow_block_state.start;
    slow_block_state.record[16]=slow_block_state.end;
    slow_block_state.record[17]=slow_block_state.epoch;
    slow_block_state.record[18]=rate;
    slow_block_state.record[19]=reason;
    /* Baselines advance on EVERY finish, outside measured render. Both records
     * publish under the same seqlock; separate reads must still cross-match. */
    slow_block_stage_advance(1);
    SBD_CAPTURE_BARRIER();slow_block_state.sequence=sequence+1u;SBD_CAPTURE_BARRIER();
    return 1;
}
