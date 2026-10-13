/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile the production USB/TRS producer paths with CC enabled. Interrupt
 * interleavings are injected deterministically; main callback asserts context. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define X0X_CC 1
#include "../../firmware/src/app/cc.h"
cc_stats_t cc_stats;
static int in_irq,injected,busy,recoveries;
static uint32_t seen[80];static uint8_t sources[80];static unsigned nseen;
static void fm1_delay_ms(uint32_t ms){(void)ms;}
uint32_t plat_control_enter(void){assert(!in_irq);return 0;}
void plat_control_leave(uint32_t x){(void)x;}
int cc_coalescable(uint32_t packet){return ((packet>>16)&127u)==1u;}
int cc_recover(void){assert(!in_irq);recoveries++;return !busy;}
#include "usb.c"
#include "midi_uart.c"
int cc_message(uint32_t packet,uint8_t source){
    assert(!in_irq);seen[nseen]=packet;sources[nseen++]=source;
    if(!injected && ((packet>>24)&127u)==30u){
        injected=1;in_irq=1;
        assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,(packet&0xffffffu)|(40u<<24),source));in_irq=0;
    }
    return busy?CC_BUSY:CC_APPLIED;
}
static void feed(const uint8_t *bytes,unsigned n){
    unsigned at=(um.rd+um.pend)&(UM_RING-1);for(unsigned i=0;i<n;i++)um_ring[(at+i)&(UM_RING-1)]=bytes[i];
    in_irq=1;uart_midi_take(n);in_irq=0;
}
int main(void){
    const uint8_t bytes[]={0xb0,1,30,0xf8,2,50,0x91,60,90};feed(bytes,sizeof bytes);
    assert(nseen==0 && mi_w==4 && mi_ready==0);cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);
    assert(nseen==1 && mi_ready==2);cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(nseen==1);
    mi_r=mi_ready;cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(nseen==2 && mi_ready==4);
    mi_r=mi_ready;cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);
    assert(nseen==3 && sources[0]==1 && sources[1]==1 && sources[2]==1);
    assert(((seen[0]>>24)&127u)==30 && ((seen[2]>>24)&127u)==40);assert(mi_ready==5);
    assert(!midi_in_q[0] && ((midi_in_q[1]>>8)&255u)==0xf8 && !midi_in_q[2] && ((midi_in_q[3]>>8)&255u)==0x91);
    mi_r=mi_ready;unsigned prior=nseen;busy=1;
    const uint8_t more[]={0xb0,3,80,0xfc};feed(more,sizeof more);unsigned ready=mi_ready;
    cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(mi_ready==ready && nseen==prior+1);
    busy=0;cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(mi_ready==mi_w && ((midi_in_q[(ready+1)%MQ]>>8)&255u)==0xfc);
    mi_r=mi_w;for(unsigned i=0;i<MQ-16;i++){in_irq=1;assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,11u|(0xb0u<<8)|(3u<<16)|((i%128)<<24),0));in_irq=0;}
    in_irq=1;assert(!cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,11u|(0xb0u<<8)|(3u<<16),0));
    for(int i=0;i<16;i++)assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,i==0?(8u|(0x81u<<8)|(60u<<16)):15u|((i==1?0xf8u:0xfcu)<<8),0));
    assert(!cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));in_irq=0;assert(cc_ingress_panic && !recoveries);
    busy=1;cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(cc_ingress_panic && recoveries==1);
    busy=0;cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);assert(!cc_ingress_panic && recoveries==2);
    unsigned boundaries=0;
    for(unsigned i=0;i<20&&mi_r!=mi_w;i++){
        cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);
        while(mi_r!=mi_ready){uint32_t p=midi_in_q[mi_r++%MQ];if(p){
            unsigned st=(p>>8)&255u;
            assert(st==(boundaries==0?0x81u:boundaries==1?0xf8u:0xfcu));boundaries++;
        }}
    }
    assert(mi_r==mi_w && boundaries==16);
    puts("Production USB/TRS CC native: IRQ-only enqueue, main-only canonical dispatch, source tags, claimed-slot interleaving, ordered clock/note/Stop, busy retention and urgent-overflow recovery PASS");return 0;
}
