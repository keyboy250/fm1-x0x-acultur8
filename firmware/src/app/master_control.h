/* SPDX-License-Identifier: GPL-3.0-only */
/* Main producer owns the whole aligned word; IRQ reads only its target bits.
 * 0..12 target,13..25 ADC movement anchor,26..28 owner,29 ADC present,30 pickup.
 * Bit31 is reserved zero. Previous8191 means no ADC reading;0..4096 valid. */
#pragma once
#include <stdint.h>
enum { MC_ADC, MC_POINTER, MC_USB, MC_TRS, MC_WEB, MC_DETACHED };
enum { MC_HAS_ADC=1, MC_PICKUP=2, MC_TOLERANCE=8, MC_MOVEMENT=8, MC_NO_ADC=8191 };
#define MC_INITIAL(q,owner,flags) {(uint32_t)(q)|((uint32_t)MC_NO_ADC<<13)|((uint32_t)(owner)<<26)|((uint32_t)(flags)<<29)}
typedef struct { volatile uint32_t word; } master_control_t;
typedef char master_control_size_4[sizeof(master_control_t)==4?1:-1];
static inline uint32_t master_control_target(const master_control_t *m){return m->word&8191u;}
static inline unsigned master_control_owner(const master_control_t *m){return (m->word>>26)&7u;}
static inline unsigned master_control_flags(const master_control_t *m){return (m->word>>29)&3u;}
static inline uint32_t master_control_state(const master_control_t *m)
{uint32_t w=m->word;return (w&8191u)|(((w>>26)&7u)<<13)|((w>>14)&65536u);}
static inline int master_control_set(master_control_t *m,uint32_t value,unsigned owner)
{
    uint32_t w=m->word;
    if(value>4096u || owner<MC_POINTER || owner>MC_WEB)return -1;
    m->word=value|(w&(8191u<<13))|((uint32_t)owner<<26)|((w&(1u<<29))?3u<<29:0);return 0;
}
static inline void master_control_detach(master_control_t *m,unsigned source)
{
    uint32_t w=m->word;
    if(source<3u && ((w>>26)&7u)==MC_USB+source)m->word=(w&~(7u<<26))|((uint32_t)MC_DETACHED<<26);
}
static inline int master_control_adc(master_control_t *m,uint32_t value)
{
    uint32_t w=m->word,target=w&8191u,previous=(w>>13)&8191u;
    if(value>4096u || !(w&(1u<<29)))return -1;
    /* Anchor-relative travel of at least8Q12 is required before near/cross
     * pickup. Smaller samples retain the anchor, so slow movement accumulates
     * but stationary targets and +/-7 jitter cannot seize remote ownership.
     * First ADC sample only seeds the anchor. The target window also reaches
     * the physical squared10-bit maximum4088 when remote gain is4096. */
    if((w>>26)&7u){
        if(previous==MC_NO_ADC)goto anchor;
        if(value+MC_MOVEMENT>previous && value<previous+MC_MOVEMENT)return 0;
        if(!((value+MC_TOLERANCE>=target && value<=target+MC_TOLERANCE) ||
             (previous<target && value>=target) || (previous>target && value<=target)))goto anchor;
    }
    w=value|(1u<<29);
anchor:
    m->word=(w&~(8191u<<13))|(value<<13);return 0;
}
