/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#define WTR_MAGIC 0x57545201u
#define WTR_CAPACITY 16u
enum { WTR_OK=1u,WTR_UNAVAILABLE=2u,WTR_BAD_JOB=4u,WTR_TIMEOUT=8u,WTR_MISMATCH=16u,WTR_HOST=32u };
typedef struct {
    volatile uint32_t count,flags,frames;
    volatile uint32_t live[WTR_CAPACITY][8];
    volatile uint32_t header[12],retained[WTR_CAPACITY][8];
} worker_trace_state_t;
extern worker_trace_state_t worker_trace_state;
/* Core0 alone writes this state, after matching mailbox completion barriers.
 * Core1 timestamps live only in its existing mailbox cache line, before done.
 * Invalid/unavailable worker stamps are never interpreted as execution time. */
static inline void worker_trace_add(uint32_t job,uint32_t arg,uint32_t dispatch,
    uint32_t start,uint32_t end,uint32_t enter,uint32_t seen,uint32_t flags)
{
    uint32_t index=worker_trace_state.count;
    worker_trace_state.count=index+1u;
    worker_trace_state.flags|=flags;
    worker_trace_state.frames+=(arg&255u)?arg&255u:256u;
    if(index>=WTR_CAPACITY)return;
    worker_trace_state.live[index][0]=job;worker_trace_state.live[index][1]=arg;
    worker_trace_state.live[index][2]=dispatch;worker_trace_state.live[index][3]=start;
    worker_trace_state.live[index][4]=end;worker_trace_state.live[index][5]=enter;
    worker_trace_state.live[index][6]=seen;worker_trace_state.live[index][7]=flags;
}
/* Called inside the existing odd/even publication transaction, after render
 * and DMA checks. Every finish resets the live window, even suppressed events. */
static inline void worker_trace_finish(int capture,const volatile uint32_t *base)
{
    uint32_t total=worker_trace_state.count,stored=total<WTR_CAPACITY?total:WTR_CAPACITY;
    if(capture){
        worker_trace_state.header[0]=WTR_MAGIC;worker_trace_state.header[1]=base[1];
        worker_trace_state.header[2]=base[2];worker_trace_state.header[3]=base[3];
        worker_trace_state.header[4]=base[4];worker_trace_state.header[5]=base[17];
        worker_trace_state.header[6]=24000000u;worker_trace_state.header[7]=stored;
        worker_trace_state.header[8]=total;worker_trace_state.header[9]=total-stored;
        worker_trace_state.header[10]=worker_trace_state.flags;
        worker_trace_state.header[11]=worker_trace_state.frames;
        for(unsigned j=0;j<stored;j++)for(unsigned i=0;i<8;i++)
            worker_trace_state.retained[j][i]=worker_trace_state.live[j][i];
    }
    worker_trace_state.count=worker_trace_state.flags=worker_trace_state.frames=0;
}
