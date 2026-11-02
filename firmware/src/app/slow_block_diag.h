/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "../worker_trace.h"
#include "../dsp/d9_workload_diag.h"
int worker_trace_snapshot(unsigned index,uint32_t *w);
/* Schema2: same20-word layout; prefer latest dropout over later guard/slow. */
#define SBD_MAGIC 0x53423103u
#define SBD_WORDS 20u
#define SBD_ACTIVE 1u
/* Must match the existing platform render budget; asserted in main_fm1.c. */
#define SBD_HALF_FRAMES 512u
#define SBD_SAMPLE_RATE 44100u
#define SBD_TICKS_PER_US 24u
#define SBD_BUDGET_US (SBD_HALF_FRAMES*1000000u/SBD_SAMPLE_RATE)
#define SBD_SLOW_TICKS (((SBD_BUDGET_US*93u+99u)/100u)*SBD_TICKS_PER_US)
#define SBD_PLAYING (1u<<31)
#define SBD_START_PENDING (1u<<30)
enum { SBD_SLOW=1, SBD_GUARD=2, SBD_DROPOUT=4 };
typedef struct {
    volatile uint32_t sequence,wait_sum,wait_max,nested,nsub,start,flags;
    volatile uint32_t guard_before,drop_before,patterns,epoch,end;
    volatile uint32_t record[SBD_WORDS];
} slow_block_state_t;
typedef char slow_block_state_is_128[(sizeof(slow_block_state_t)==128)?1:-1];
extern slow_block_state_t slow_block_state;
/* Paired page21: existing core0 profiler deltas, NOT exhaustive/exclusive work.
 * PR_808 means core0 wait when offloaded; nested IRQ time can overlap stages. */
#define SBD_STAGE_MAGIC 0x53545001u
#define SBD_STAGE_WORDS 16u
typedef struct {
    volatile uint32_t previous[8], previous_frames;
    volatile uint32_t record[SBD_STAGE_WORDS];
} slow_block_stage_state_t;
typedef char slow_block_stage_state_is_100[(sizeof(slow_block_stage_state_t)==100)?1:-1];
extern slow_block_stage_state_t slow_block_stage_state;
void slow_block_stage_advance(int capture);
int slow_block_stage_snapshot(uint32_t *words);
void slow_block_init(uint32_t session,uint32_t guard,uint32_t drops);
void slow_block_begin(uint32_t start,uint32_t patterns,uint32_t epoch);
void engine_slow_block_begin(uint32_t start);
int slow_block_wait(int dispatched);
uint32_t slow_block_render_end(uint32_t end); /* elapsed raw ticks */
void slow_block_nested(uint32_t ticks);
int slow_block_finish(uint32_t half,uint32_t guard,uint32_t drops);
int slow_block_snapshot(uint32_t *words); /* 20 words, or -2 busy; never mutates. */
