/* SPDX-License-Identifier: GPL-3.0-only */
/* Included once by engine.c. Only core0 and its nested TIMER5 write this state. */
#include "slow_block_diag.h"
#include <string.h>
slow_block_state_t slow_block_state __attribute__((section(".noinit"),aligned(4)));
#if X0X_DEVICE_TIMING
#define SBD_BARRIER() __asm__ volatile("csync" ::: "memory")
#else
#define SBD_BARRIER() __asm__ volatile("" ::: "memory")
#endif
worker_trace_state_t worker_trace_state __attribute__((section(".noinit"),aligned(4)));
slow_block_stage_state_t slow_block_stage_state __attribute__((section(".noinit"),aligned(4)));
/* eng_prof and eng_prof_frames are declared by engine.c before this include.
 * Only core0 writes them, including synchronous core2 fallback. */
void slow_block_stage_advance(int capture)
{
    if(capture){
        slow_block_stage_state.record[0]=SBD_STAGE_MAGIC;
        slow_block_stage_state.record[1]=slow_block_state.record[1];
        slow_block_stage_state.record[2]=slow_block_state.record[2];
        slow_block_stage_state.record[3]=slow_block_state.record[3];
        slow_block_stage_state.record[4]=slow_block_state.record[4];
        slow_block_stage_state.record[5]=slow_block_state.record[17];
        /* Zero means raw CPU counts with no captured rate; do not guess 24MHz. */
        slow_block_stage_state.record[6]=plat_cycles_cpu()?0u:plat_cycles_hz();
    }
    for(unsigned i=0;i<8;i++){
        uint32_t now=eng_prof[i];
        if(capture)slow_block_stage_state.record[8+i]=now-slow_block_stage_state.previous[i];
        slow_block_stage_state.previous[i]=now;
    }
    uint32_t frames=eng_prof_frames;
    if(capture)slow_block_stage_state.record[7]=frames-slow_block_stage_state.previous_frames;
    slow_block_stage_state.previous_frames=frames;
    worker_trace_finish(capture,slow_block_state.record);
    d9_workload_finish(capture,slow_block_state.record,slow_block_stage_state.record[7]);
}
#include "slow_block_capture.h"
#ifndef SBD_TEST_COPY_HOOK
#define SBD_TEST_COPY_HOOK() ((void)0)
#endif
void slow_block_init(uint32_t session,uint32_t guard,uint32_t drops)
{
    memset((void *)&slow_block_state,0,sizeof slow_block_state);
    slow_block_state.guard_before=guard;slow_block_state.drop_before=drops;
    slow_block_state.record[0]=SBD_MAGIC;
    slow_block_state.record[2]=session;
    memset((void *)&slow_block_stage_state,0,sizeof slow_block_stage_state);
    slow_block_stage_state.record[0]=SBD_STAGE_MAGIC;
    slow_block_stage_state.record[2]=session;
    memset((void *)&worker_trace_state,0,sizeof worker_trace_state);
    worker_trace_state.header[0]=WTR_MAGIC;worker_trace_state.header[2]=session;
    worker_trace_state.header[6]=24000000u;
    memset((void *)&d9_workload_state,0,sizeof d9_workload_state);
    d9_workload_state.header[0]=D9W_MAGIC;d9_workload_state.header[2]=session;
    d9_workload_state.header[10]=D9W_VOICES;d9_workload_state.header[11]=D9W_COLUMNS;
    slow_block_stage_advance(0);
    SBD_BARRIER();
}
void slow_block_begin(uint32_t start,uint32_t patterns,uint32_t epoch)
{
    slow_block_begin_inline(start,patterns,epoch);
}
int slow_block_wait(int dispatched)
{
    uint32_t t,dt;int failed;
    if(!(slow_block_state.flags&SBD_ACTIVE))return !dispatched || plat_cpu2_wait();
    slow_block_state.nsub++;
    if(!dispatched)return 1;
    t=plat_ticks24();failed=plat_cpu2_wait();dt=plat_ticks24()-t;
    slow_block_state.wait_sum+=dt;
    if(dt>slow_block_state.wait_max)slow_block_state.wait_max=dt;
    return failed;
}
uint32_t slow_block_render_end(uint32_t end)
{
    slow_block_state.flags=0;SBD_BARRIER();slow_block_state.end=end;
    return end-slow_block_state.start;
}
void slow_block_nested(uint32_t ticks)
{
    if(slow_block_state.flags&SBD_ACTIVE)slow_block_state.nested+=ticks;
}
int slow_block_finish(uint32_t half,uint32_t guard,uint32_t drops)
{
    return slow_block_finish_inline(half,guard,drops);
}
int slow_block_snapshot(uint32_t *w)
{
    uint32_t sequence=slow_block_state.sequence;
    if(sequence&1u)return -2;
    SBD_BARRIER();
    for(unsigned i=0;i<SBD_WORDS;i++)w[i]=slow_block_state.record[i];
    SBD_TEST_COPY_HOOK();SBD_BARRIER();
    if(sequence!=slow_block_state.sequence)return -2;
    return SBD_WORDS;
}

int slow_block_stage_snapshot(uint32_t *w)
{
    uint32_t sequence=slow_block_state.sequence;
    if(sequence&1u)return -2;
    SBD_BARRIER();
    for(unsigned i=0;i<SBD_STAGE_WORDS;i++)w[i]=slow_block_stage_state.record[i];
    SBD_TEST_COPY_HOOK();SBD_BARRIER();
    if(sequence!=slow_block_state.sequence)return -2;
    return SBD_STAGE_WORDS;
}

int worker_trace_snapshot(unsigned index,uint32_t *w)
{
    uint32_t sequence=slow_block_state.sequence;
    if(index>WTR_CAPACITY)return -1;
    if(sequence&1u)return -2;
    SBD_BARRIER();
    if(index==0){for(unsigned i=0;i<12;i++)w[i]=worker_trace_state.header[i];}
    else{
        if(index>worker_trace_state.header[7])return -1;
        w[0]=worker_trace_state.header[1];w[1]=worker_trace_state.header[2];
        w[2]=worker_trace_state.header[3];w[3]=index;
        for(unsigned i=0;i<8;i++)w[4+i]=worker_trace_state.retained[index-1][i];
    }
    SBD_TEST_COPY_HOOK();SBD_BARRIER();
    if(sequence!=slow_block_state.sequence)return -2;
    return 12;
}

int d9_workload_snapshot(unsigned index,uint32_t *w)
{
 uint32_t sequence=slow_block_state.sequence;
 if(index>D9W_VOICES)return -1;
 if(sequence&1u)return -2;
 SBD_BARRIER();
 if(index==0){for(unsigned i=0;i<12;i++)w[i]=d9_workload_state.header[i];}
 else{
  if(!d9_workload_state.header[6])return -1;
  w[0]=d9_workload_state.header[1];w[1]=d9_workload_state.header[2];w[2]=d9_workload_state.header[3];w[3]=index;
  for(unsigned i=0;i<D9W_COLUMNS;i++)w[4+i]=d9_workload_state.retained[index-1][i];
 }
 SBD_TEST_COPY_HOOK();SBD_BARRIER();if(sequence!=slow_block_state.sequence)return -2;return 12;
}
