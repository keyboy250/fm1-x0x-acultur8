/* SPDX-License-Identifier: GPL-3.0-only
 * Function bodies are inserted from the actual candidate by the runner.
 * Only mailbox, editor reply and updater identity hardware boundaries are mocked.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define POLY_COLD
#define ED_HDR0 0x7D
#define ED_HDR1 0x46
#define ED_HDR2 0x4C
#define ED_SMP_BEGIN 11
#define ED_SMP_READ 16
static uint8_t mailbox[640], ota_dec[640];
static uint32_t mailbox_n;
static int ready, handled, inject_during_reply, identity_replies, inject_after_empty;
static uint8_t query[] = {0x7D,0x46,0x4C,49,1,77,7,0,0,0};
static int ota_frame_get(const uint8_t **p,uint32_t *n) {
    if(!ready) {
        /* USB ISR publishes after the empty flag observation, before return. */
        if(inject_after_empty) {
            inject_after_empty=0;memcpy(mailbox,query,sizeof query);
            mailbox_n=sizeof query;ready=1;
        }
        return 0;
    }
    *p=mailbox;*n=mailbox_n;return 1;
}
static void ota_frame_done(void) { ready=0; }
static void inject(const uint8_t *p,uint32_t n) { assert(!ready && n<=sizeof mailbox);memcpy(mailbox,p,n);mailbox_n=n;ready=1; }
static void ed_handle(const uint8_t *p,uint32_t n) {
    assert(n==sizeof query && p[3]==query[3]);handled++;
    if(inject_during_reply) { inject_during_reply=0;inject(query,sizeof query); }
}
static void ota_reply_identity(void) { identity_replies++; }
/* ACTUAL_SOURCE_FUNCTIONS */
#if EXPECT_FIXED
#define editor_updater_service ota_editor_service
#else
#define editor_updater_service ota_service
#endif
int main(void) {
    const unsigned commands[]={1,25,48,49,50,51};
    unsigned scans=0;
    for(unsigned cycle=0;cycle<1000;cycle++)for(unsigned c=0;c<6;c++) {
        handled=0;query[3]=(uint8_t)commands[c];
        inject(query,sizeof query);inject_during_reply=1;
        ed_service();assert(handled==1 && ready);
        assert(editor_updater_service()==0);
#if EXPECT_FIXED
        assert(ready);ed_service();assert(handled==2 && !ready);
#else
        assert(!ready);ed_service();assert(handled==1);
#endif
        scans++;
    }
    unsigned gap_cases=0;
    for(unsigned cycle=0;cycle<1000;cycle++)for(unsigned c=0;c<6;c++) {
        assert(!ready);handled=0;query[3]=(uint8_t)commands[c];inject_after_empty=1;
        assert(editor_updater_service()==0 && !inject_after_empty);
#if EXPECT_GAP_FIXED
        assert(ready);ed_service();assert(handled==1 && !ready);
#else
        assert(!ready);ed_service();assert(handled==0);
#endif
        gap_cases++;
    }
    /* An ISR publishing after the wrapper returned empty is handled next pass. */
    for(unsigned c=0;c<6;c++) {
        handled=0;query[3]=(uint8_t)commands[c];assert(editor_updater_service()==0 && !ready);
        inject(query,sizeof query);ed_service();assert(handled==1 && !ready);
    }
    uint8_t raw[9]={0,0x59,0x11,0,0,0,0xff},wire[640];
    uint32_t n=ota_pack7(raw,7,wire);
    inject(wire,n);ed_service();assert(ready);
    assert(ota_service()==7 && identity_replies==1 && !ready);
    for(unsigned cmd=0x41;cmd<=0x42;cmd++) {
        raw[2]=(uint8_t)cmd;raw[3]=2;raw[6]=3;raw[7]=4;raw[8]=(uint8_t)~7u;
        n=ota_pack7(raw,9,wire);inject(wire,n);ed_service();assert(ready);
        assert(ota_service()==9 && !ready && ota_dec[2]==cmd && ota_dec[6]==3 && ota_dec[7]==4);
        assert(identity_replies==1);
        wire[n-1]^=1;inject(wire,n);assert(ota_service()==0 && !ready);
    }
    /* Short editor prefixes must drain rather than stick in the mailbox. */
    for(unsigned length=0;length<4;length++) {
        inject(query,length);ed_service();assert(ready);
        assert(editor_updater_service()==0 && !ready);
    }
    /* All complete editor command headers and maximum mailbox size retain ownership. */
    for(unsigned cmd=0;cmd<128;cmd++) {
        query[3]=(uint8_t)cmd;inject(query,4);assert(editor_updater_service()==0);
#if EXPECT_FIXED
        assert(ready);
#else
        assert(!ready);
#endif
        ota_frame_done();
    }
    memset(wire,0,sizeof wire);memcpy(wire,query,4);
    inject(wire,sizeof wire);assert(editor_updater_service()==0);
#if EXPECT_FIXED
    assert(ready);
#else
    assert(!ready);
#endif
    ota_frame_done();
    /* Active updater sessions use ota_take directly and must drain foreign
     * editor frames, since ed_service is not running inside the session. */
    inject(query,sizeof query);assert(ota_take()==0 && !ready);
    /* Recovery boot also runs without ed_service and must retain its drain. */
    inject(query,sizeof query);assert(ota_service()==0 && !ready);
    for(unsigned byte=0;byte<3;byte++) {
        memcpy(wire,query,sizeof query);wire[byte]^=1;inject(wire,sizeof query);
        assert(ota_service()==0 && !ready);
    }
    assert(ota_service()==0 && !ready);
    printf("%s: %u reply/next-query cases; %u empty-check/arrival/take cases (%s); after-return arrivals, short headers, ownership, identity, backup, recovery and active updater PASS\n",
           EXPECT_FIXED?"EDITOR OWNERSHIP RETAINED":"ORIGINAL LOSS REPRODUCED",scans,gap_cases,
           EXPECT_GAP_FIXED?"PRESERVED":"LOSS REPRODUCED");
    return 0;
}
