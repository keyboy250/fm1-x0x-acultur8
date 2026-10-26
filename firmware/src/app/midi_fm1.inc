/* SPDX-License-Identifier: GPL-3.0-only */
/* MIDI: Felucca's rings carry 4-byte USB-MIDI event packets, byte 0 = cable / CIN */
#ifndef MIDI_SERVICE_OBSERVE_HOOK
#define MIDI_SERVICE_OBSERVE_HOOK() ((void)0)
#endif
int plat_midi_connected(void) { return midi_usb_ready(); }
int plat_midi_in(uint32_t *pkt)
{
    while(mi_r!=mi_ready){
        uint32_t p=midi_in_q[mi_r++%MQ],source=(p>>4)&15u,status=(p>>8)&255u,i;
        int retired=source<3&&mi_retired_clock[source];
        for(i=0;i<3;i++)if(mi_retired_clock[i])mi_retired_clock[i]--;
        if((status==248u||status==250u||status==251u||status==252u)&&
           (retired||(source==0&&(usb_midi_generation!=usb_midi_observed_generation||usb_midi_generation==UINT32_MAX))))continue;
        *pkt=p;if(p)return 1;
    }
    return 0;
}
void plat_midi_clear_cc_source(uint8_t source){cc_ingress_clear_source(midi_in_q,&mi_w,MQ,source);}
void plat_midi_service(void){
    static uint8_t previous_usb;
    uint32_t saved=MIDI_TX_ENTER();
    uint8_t connected=(uint8_t)(usb.config&&!usb.suspended);
    uint32_t generation=usb_midi_generation;
    int retire=(previous_usb&&!connected)||usb_midi_observed_generation!=generation||generation==UINT32_MAX;
    if(retire){
        seq_transport_retire(&seq);seq.ext_pending=0;midi_out_cancel(1);midi_in_retire_clock(1);
    }
    MIDI_SERVICE_OBSERVE_HOOK();
    usb_midi_observed_generation=generation;previous_usb=connected;
    MIDI_TX_LEAVE(saved);
    if(retire)cc_reset_source(0);
    cc_generation_service();
    cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);
}
int plat_midi_out(uint32_t pkt) { return midi_out_event(pkt); }
int plat_midi_pair(uint32_t first,uint32_t second) { return midi_out_pair(first,second); }
int plat_midi_batch(const uint32_t *packets,uint32_t count) { return midi_out_batch(packets,count); }
void plat_midi_cancel(void) {midi_out_cancel(0);}
void plat_midi_transport_cancel(void) {midi_out_cancel(1);}


void plat_midi_retire_clock(uint8_t sources){uint32_t saved=MIDI_TX_ENTER();midi_in_retire_clock(sources);MIDI_TX_LEAVE(saved);}
