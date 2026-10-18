/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual production USB consumer; only MMIO/IRQ transport is stubbed. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_CC 1
#define FELUCCA_OTA 1
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "../../firmware/src/app/cc.h"
cc_stats_t cc_stats;
int cc_coalescable(uint32_t p){(void)p;return 0;}
int cc_recover(void){return 1;}
static unsigned reviewed_cc;
int cc_message(uint32_t p,uint8_t s){(void)p;(void)s;reviewed_cc++;return 0;}
uint32_t plat_control_enter(void){return 0;}
void plat_control_leave(uint32_t f){(void)f;}
static void fm1_delay_ms(uint32_t n){(void)n;}
#include "../../firmware/hal/fm1_usb.h"
static uint8_t regs[32];static uint32_t reg_read;
static uint32_t wire[512],nw;
static int irq_enabled=1,lock_depth,before_stop,after_stop,pending_irq;
static void (*deferred_irq)(void);
static uint32_t enter(void){uint32_t saved=(uint32_t)irq_enabled;irq_enabled=0;lock_depth++;return saved;}
static void leave(uint32_t saved);
static void before_commit(void);
static uint32_t sie_on(void){return 1;}
static uint32_t sie_done(void){return 1;}
static void sie_write(uint32_t r,uint32_t v){assert(r<32);regs[r]=(uint8_t)v;}
static void sie_read(uint32_t r){assert(r<32);reg_read=r;}
static uint32_t sie_data(void){return regs[reg_read];}
static void ep_send(uint32_t ep,void *p,uint32_t n){
 assert(ep==1&&n<=64&&n%4==0&&lock_depth==1&&!irq_enabled);
 uint8_t *b=p;for(uint32_t i=0;i<n;i+=4)wire[nw++]=(uint32_t)b[i]|((uint32_t)b[i+1]<<8)|((uint32_t)b[i+2]<<16)|((uint32_t)b[i+3]<<24);
 if(after_stop){after_stop=0;pending_irq=1;}
}
#define fm1_usb_sie_on sie_on
#define fm1_usb_sie_done sie_done
#define fm1_usb_sie_wr_start sie_write
#define fm1_usb_sie_rd_start sie_read
#define fm1_usb_sie_data sie_data
#define fm1_usb_ep_send ep_send
#define MIDI_TX_ENTER() enter()
#define MIDI_TX_LEAVE(saved) leave(saved)
#define MIDI_TX_BEFORE_COMMIT() before_commit()
#include "usb.c"
static uint32_t pk(unsigned s,unsigned n,unsigned v){return(s>=248?15u:s>>4)|(s<<8)|(n<<16)|(v<<24);}
static unsigned status(uint32_t p){return(p>>8)&255u;}
static void leave(uint32_t saved){assert(lock_depth>=1);lock_depth--;irq_enabled=(int)saved;if(irq_enabled&&pending_irq){pending_irq=0;midi_out_cancel(1);assert(midi_out_event(pk(252,0,0)));}if(irq_enabled&&deferred_irq){void(*fn)(void)=deferred_irq;deferred_irq=0;fn();}}
static void before_commit(void){assert(!lock_depth);if(before_stop){before_stop=0;midi_out_cancel(1);}}
static void reset(void){memset(&usb,0,sizeof usb);usb.config=usb.up=1;usb_midi_generation=usb_midi_observed_generation=0;mo_w=mo_r=0;mo_cancel_attacks=mo_cancel_transport=0;mi_w=mi_r=mi_ready=0;for(int i=0;i<3;i++)mi_retired_clock[i]=0;so_w=so_r=0;sx_busy=0;nw=0;memset(regs,0,sizeof regs);irq_enabled=1;before_stop=after_stop=pending_irq=lock_depth=0;deferred_irq=0;}
static void next(void){regs[S_TXCSR1]=0;ep1_tx();assert(!lock_depth);}
#ifndef CLOCK_USB_NO_MAIN
int main(void){
 reset();for(unsigned i=0;i<64;i++)assert(midi_out_event(i==3?pk(128,60,0):i==20?pk(128,64,0):i==40?pk(144,67,0):i%2?pk(144,62,90):pk(248,0,0)));
 uint32_t slots[64];memcpy(slots,midi_out_q,sizeof slots);midi_out_cancel(1);assert(!memcmp(slots,midi_out_q,sizeof slots));next();assert(mo_r==16&&mo_cancel_transport==48&&nw==1&&status(wire[0])==128);uint32_t r=mo_r;ep1_tx();assert(mo_r==r);assert(midi_out_event(pk(252,0,0)));for(int i=0;i<4;i++)next();assert(mo_r==mo_w&&nw==4&&status(wire[3])==252);assert(!mo_cancel_attacks&&!mo_cancel_transport);
 /* Stop preempts uncommitted endpoint staging: producer cannot rewrite slots,
  * all old realtime/On are filtered; owned cleanup survives. */
 reset();assert(midi_out_event(pk(248,0,0)));assert(midi_out_event(pk(144,60,90)));assert(midi_out_event(pk(128,60,0)));before_stop=1;next();assert(mo_r==3&&nw==1&&wire[0]==pk(128,60,0));assert(midi_out_event(pk(252,0,0)));next();assert(nw==2&&status(wire[1])==252);
 /* Stop arriving after DMA handoff is deferred while IRQ masked. One already
  * handed prefix is allowed; current FC follows, no old future prefix remains. */
 reset();for(int i=0;i<20;i++)assert(midi_out_event(pk(248,0,0)));after_stop=1;next();assert(nw==16&&mo_r==16&&mo_cancel_transport==4);next();assert(nw==17&&status(wire[16])==252&&mo_r==mo_w);
 /* Wrap, sparse cancelled prefix, retained Off and fresh Stop. */
 reset();mo_w=mo_r=0xfffffffdu;assert(midi_out_event(pk(248,0,0)));assert(midi_out_event(pk(250,0,0)));assert(midi_out_event(pk(128,70,0)));assert(midi_out_event(pk(144,70,90)));midi_out_cancel(1);next();assert(mo_r==1&&nw==1&&wire[0]==pk(128,70,0));assert(midi_out_event(pk(252,0,0)));next();assert(nw==2&&status(wire[1])==252);
 /* Note reconciliation cancels attacks only, preserving current transport. */
 reset();unsigned rt[]={248,250,251,252};for(int i=0;i<4;i++)assert(midi_out_event(pk(rt[i],0,0)));assert(midi_out_event(pk(144,60,90)));assert(midi_out_event(pk(144,60,0)));assert(midi_out_event(pk(128,60,0)));midi_out_cancel(0);next();assert(nw==6);for(int i=0;i<4;i++)assert(status(wire[i])==rt[i]);
 /* SysEx endpoint prefix is preserved during musical cancellation. */
 reset();sx_out_q[0]=0x22112204u;so_w=1;assert(midi_out_event(pk(248,0,0)));assert(midi_out_event(pk(128,60,0)));midi_out_cancel(1);next();assert(so_r==1&&mo_r==2&&nw==2&&wire[0]==0x22112204u&&status(wire[1])==128);
 /* Restore prior disabled state rather than enabling nesting unconditionally. */
 reset();irq_enabled=0;assert(midi_out_event(pk(252,0,0)));next();assert(!irq_enabled&&nw==1);
 printf("PASS actual USB consumer: full/wrap/prefix/backpressure, no ring slot rewrite, queued stale realtime/On filtering, currentFC/ownedOff, precommit Stop race, posthandoff16-word allowance, SysEx prefix and exact prior IRQ state; cancellation metadata=%zu bytes; physical delivery not tested\n",sizeof mo_cancel_attacks+sizeof mo_cancel_transport);return 0;}
#endif
