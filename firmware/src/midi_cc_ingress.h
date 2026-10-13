/* SPDX-License-Identifier: GPL-3.0-only */
/* Ordered IRQ -> main -> audio handoff using the existing packet storage.
 * mi_w: IRQ publishes; mi_ready: main publishes reviewed prefix; mi_r: audio.
 * No canonical/project/UI mutation occurs in an IRQ packet producer.
 * RING_PUBLISH must be a compiler memory barrier on the target.
 */
#pragma once
#include <stdint.h>
static volatile uint32_t mi_ready,mi_claimed=0xffffffffu;
static uint32_t cc_ingress_overflow,cc_ingress_coalesced;
static volatile uint8_t cc_ingress_panic;
static int cc_ingress_put(uint32_t *ring,volatile uint32_t *write,volatile uint32_t *read,
                          uint32_t capacity,uint32_t packet,uint8_t source)
{
    uint32_t w=*write,kind=(packet>>8)&240u,cc=kind==176u;
    int urgent=((packet>>8)&255u)>=248u || kind==128u || (kind==144u && !((packet>>24)&127u));
    packet=(packet&~240u)|((uint32_t)source<<4);
    /* Only the tail beyond main's claimed slot may be replaced. Main claims
     * before copying; aligned packet stores are atomic. Selector/action CCs
     * are NOT coalesced here: semantic boundaries require registry knowledge. */
    if(cc && w!=mi_ready && w-1!=mi_claimed && w!=*read && cc_coalescable(packet)){
        uint32_t old=ring[(w-1)%capacity];
        if((old&0xffffffu)==(packet&0xffffffu)){
            ring[(w-1)%capacity]=packet;RING_PUBLISH();cc_ingress_coalesced++;return 1;
        }
    }
    if(w-*read>=capacity-(cc?16u:urgent?0u:8u)){
        cc_ingress_overflow++;if(urgent)cc_ingress_panic=1;return 0;
    }
    ring[w%capacity]=packet;RING_PUBLISH();*write=w+1;return 1;
}
static void cc_ingress_service(uint32_t *ring,volatile uint32_t *write,volatile uint32_t *read,uint32_t capacity){
    unsigned budget=32,events=0;
    cc_stats.coalesced=cc_ingress_coalesced;cc_stats.overflow=cc_ingress_overflow;
    {uint32_t saved=plat_control_enter();
        if(cc_ingress_panic && cc_recover())cc_ingress_panic=0;
        plat_control_leave(saved);}
    /* Audio must finish every previously published non-control boundary before
     * main changes canonical state for a later CC. Audio drains CQ before MIDI.
     * On the single-core target main cannot resume inside the audio IRQ; host
     * and browser invoke service/render serially. Thus the consumed cursor is
     * also an applied boundary when observed here, despite read-before-dispatch.
     * Contiguous CCs may share one block; never cross a note/clock/transport. */
    if(mi_ready!=*read)return;
    while(budget-- && mi_ready!=*write){
        uint32_t packet,at,saved=plat_control_enter();
        at=mi_ready;mi_claimed=at;packet=ring[at%capacity];plat_control_leave(saved);
        if(((packet>>8)&240u)==176u && ((packet>>16)&127u)<120u){
            if(events){mi_claimed=0xffffffffu;return;}
            int result=cc_message(packet,(uint8_t)((packet>>4)&15u));
            if(result==CC_BUSY){mi_claimed=0xffffffffu;return;}
            /* An applied/rejected control is not an outbound message and cannot
             * become a note/control feedback echo in the audio dispatcher. */
            ring[at%capacity]=0;
        }
        saved=plat_control_enter();RING_PUBLISH();mi_ready=at+1;mi_claimed=0xffffffffu;plat_control_leave(saved);
        if(packet && !(((packet>>8)&240u)==176u && ((packet>>16)&127u)<120u))events=1;
    }
}
static void cc_ingress_clear_source(uint32_t *ring,volatile uint32_t *write,uint32_t capacity,uint8_t source){
    uint32_t saved=plat_control_enter(),at;
    for(at=mi_ready;at!=*write;at++){
        uint32_t p=ring[at%capacity];
        if(((p>>8)&240u)==176u && ((p>>4)&15u)==source && ((p>>16)&127u)<120u)ring[at%capacity]=0;
    }
    RING_PUBLISH();plat_control_leave(saved);
}
