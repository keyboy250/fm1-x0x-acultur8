/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
static unsigned writes,wire_n,done;
static uint8_t wire[288],incoming[80];static uint32_t incoming_n;
static int flash_ok=1;
static void fm1_irq_off(void) {} static void fm1_irq_on(void) {}
static void fm1_wdt_feed(void) {} static void audio_silence(void) {}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;writes++;}
static int fl_erase4k(uint32_t a,uint32_t *t){(void)a;*t=0;writes++;return 0;}
static int fl_write(uint32_t a,const void *p,uint32_t n){(void)a;(void)p;(void)n;writes++;return 0;}
static void ota_wire_send(const uint8_t *p,uint32_t n){assert(n<=sizeof wire);memcpy(wire,p,n);wire_n=n;}
static int ota_frame_get(const uint8_t **p,uint32_t *n){if(!incoming_n)return 0;*p=incoming;*n=incoming_n;return 1;}
static void ota_frame_done(void){incoming_n=0;done++;}
#include "../../firmware/src/app/editor.c"
static uint32_t val(unsigned p,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;i++)v|=(uint32_t)wire[p+i]<<(7*i);return v;}
static void send_query(unsigned version,unsigned token,unsigned strip,unsigned extra){
    uint8_t f[]={0x7D,0x46,0x4C,48,version,token,strip,0};
    incoming_n=7+extra;memcpy(incoming,f,incoming_n);wire_n=0;ed_service();
    assert(incoming_n==0&&wire_n>=10&&wire[0]==0xF0&&wire[wire_n-1]==0xF7);
    for(unsigned i=1;i<wire_n-1;i++)assert(wire[i]<128);
}
int main(void){
    for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    boot();run_ms(40);
    uint32_t stored=store_writes;unsigned total=0;
    FILE *packets=fopen("build/strip-query-replies.bin","wb");assert(packets);
    for(unsigned strip=0;strip<25;strip++)for(unsigned flags=0;flags<16;flags++){
        sound_strip_config_t c={(uint16_t)(strip?20000-strip:20),(uint16_t)(strip?20+strip:20000),24,48,250,(uint8_t)flags};
        proj.variant.strip[strip]=c;
        uint32_t fp=project_fingerprint(),rev=variant_current_revision(),w=engine_queue_written(),r=engine_queue_read();
        send_query(1,strip+flags,strip,0);
        assert(wire_n==31&&wire[5]==1&&wire[6]==strip+flags&&wire[7]==strip&&wire[8]==0);
        assert(val(9,5)==rev&&val(14,5)==w);
        assert(val(19,3)==c.highpass_hz&&val(22,3)==c.lowpass_hz);
        assert(wire[25]==24&&wire[26]==48&&val(27,2)==250&&wire[29]==flags);
        assert(fp==project_fingerprint()&&rev==variant_current_revision());
        assert(w==engine_queue_written()&&r==engine_queue_read()&&stored==store_writes&&!writes);
        assert(fwrite(wire,1,wire_n,packets)==wire_n);total++;
    }
    proj.variant.strip[23]=(sound_strip_config_t){146,10240,0,1,80,7};
    send_query(1,77,23,0);assert(fwrite(wire,1,wire_n,packets)==wire_n);fclose(packets);
    incoming_n=80;memset(incoming,0,sizeof incoming);memcpy(incoming,(uint8_t[]){0x7D,0x46,0x4C,48,1,4,23},7);wire_n=0;ed_service();assert(wire_n==10&&wire[8]==1);
    send_query(1,3,25,0);assert(wire_n==10&&wire[8]==1);
    send_query(2,3,23,0);assert(wire_n==10&&wire[8]==3);
    send_query(1,3,23,1);assert(wire_n==10&&wire[8]==1);
    for(unsigned n=4;n<7;n++){uint8_t f[]={0x7D,0x46,0x4C,48,1,3,23};memcpy(incoming,f,n);incoming_n=n;wire_n=0;ed_service();assert(wire_n==10&&wire[8]==1);}
    variant_command_t c={1,VC_SET_STRIP,1,0,0,VP_BOOST,0,5};variant_reply_t reply;
    assert(variant_command(&c,&reply)==VR_QUEUED);send_query(1,9,0,0);assert(wire_n==10&&wire[8]==2);
    run_ms(40);send_query(1,10,0,0);assert(wire[8]==0&&wire[25]==5);
    incoming_n=4;memcpy(incoming,(uint8_t[]){0x7D,0x46,0x4C,25},4);wire_n=0;ed_service();assert(wire_n==7&&wire[4]==25&&wire[5]==0);
    incoming_n=4;memcpy(incoming,(uint8_t[]){1,2,3,48},4);wire_n=0;ed_service();assert(!wire_n&&incoming_n==4);incoming_n=0;
    assert(!writes&&stored==store_writes);
    printf("PASS actual editor/service strip query: %u exact configurations, all25 strips/all16 flags, wire7-bit safety, invalid/truncated/version/busy requests, legacy PING, no project/revision/CQ/storage writes.\n",total);
    return 0;
}
