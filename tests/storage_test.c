/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), HÃ¼gelton Instruments */
/* Host test of firmware/src/storage.c against a simulated NOR flash:
 * erase -> 0xFF, program can only clear bits, page writes must not wrap. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t nor[0x100000];
static int fail_after = -1;            /* torn-write injection: stop after N programs */

static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (fail_after == 0)
        return -9;
    if (fail_after > 0)
        fail_after--;
    if ((off & 0xFFu) + n > 256u) {
        printf("page wrap at %#x\n", off);
        exit(1);
    }
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/app/plat.h"     /* X0X: the object map */
#include "../firmware/src/storage.c"
#define OBJ_PROJECT0 OBJ_SOUND                 /* the test's three data objects: SOUND, PAT0, PAT1 */
#define OBJ_SETTINGS OBJ_SET

static int check(const char *what, int ok)
{
    printf("%-46s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

int main(void)
{
    char a[600], b[600], got[600];
    int bad = 0, n;
    memset(nor, 0xFF, sizeof nor);
    memset(a, 'A', sizeof a);
    memset(b, 'B', sizeof b);
    bad += check("empty flash loads nothing", st_load(OBJ_PROJECT0, got, sizeof got) < 0);
    bad += check("save A", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save B", st_save(OBJ_PROJECT0, b, sizeof b) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns B (newer seq)", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 1;                                  /* payload page 1 written, the rest torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("torn save keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 3;                                  /* all payload pages, header torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("header not written keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    bad += check("save A again", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    n = st_load_prev(OBJ_PROJECT0, got, sizeof got);
    bad += check("the copy before A is B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    bad += check("no copy before on an empty object", st_load_prev(OBJ_MOTION, got, sizeof got) < 0);
    nor[st_sector(OBJ_PROJECT0, 0) + ST_PAYLOAD_OFF + 10] ^= 0x01;   /* bit rot in one copy */
    nor[st_sector(OBJ_PROJECT0, 1) + ST_PAYLOAD_OFF + 10] ^= 0x01;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("both copies corrupt -> nothing", n < 0);
    bad += check("other objects untouched", st_load(OBJ_PROJECT0 + 1, got, sizeof got) < 0);
    bad += check("settings save/load",
                 st_save(OBJ_SETTINGS, "hello", 5) == 0 && st_load(OBJ_SETTINGS, got, 5) == 5 && !memcmp(got, "hello", 5));
    bad += check("CRC-32 is zlib's (check value 0xCBF43926)", st_crc32("123456789", 9) == 0xCBF43926u);
    memset(nor, 0xFF, sizeof nor);
    st_save(OBJ_PROJECT0 + 2, a, sizeof a);
    st_save(OBJ_PROJECT0 + 2, b, sizeof b);              /* B is newer, in the other copy */
    {
        st_hdr_t h;
        int cur = st_current(OBJ_PROJECT0 + 2, &h);
        nor[st_sector(OBJ_PROJECT0 + 2, (uint32_t)cur) + ST_PAYLOAD_OFF + 300] ^= 0x10;   /* rot in the newer copy */
    }
    n = st_load(OBJ_PROJECT0 + 2, got, sizeof got);
    bad += check("newer copy rotten -> the older one loads", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save over the rotten copy", st_save(OBJ_PROJECT0 + 2, b, sizeof b) == 0);
    n = st_load(OBJ_PROJECT0 + 2, got, sizeof got);
    bad += check("... loads the new data", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    nor[st_sector(OBJ_PROJECT0 + 2, 0) + 8] ^= 0x01;    /* both headers broken */
    nor[st_sector(OBJ_PROJECT0 + 2, 1) + 8] ^= 0x01;
    bad += check("both headers broken -> nothing", st_load(OBJ_PROJECT0 + 2, got, sizeof got) < 0);
    {   /* X0X map: every object in a data region, none in the sample slots (0xA0000..0xDBFFF), no overlaps */
        uint32_t o, p, ok = 1;
        for (o = 0; o < OBJ_STRIP_DEFAULTS; o++) {
            uint32_t a0 = st_sector(o, 0), a1 = st_sector(o, 1) + 4096;
            int in_data = (a0 >= 0x97000 && a1 <= 0xA0000) || (a0 >= 0xDC000 && a1 <= 0xE0000) ||
                          (a0 >= 0xFC000 && a1 <= 0xFF000);
            if (!in_data || a1 - a0 != 8192)
                ok = 0;
            for (p = 0; p < o; p++)
                if (a0 < st_sector(p, 1) + 4096 && st_sector(p, 0) < a1)
                    ok = 0;
        }
        bad += check("objects in data regions, clear of the sample slots", (int)ok);
    }
    /* Defaults pair uses the two free sectors, never samples/project/OTA. */
    bad += check("defaults A uses free gap",st_sector(OBJ_STRIP_DEFAULTS,0)==0x9F000u);
    bad += check("defaults B uses free global gap",st_sector(OBJ_STRIP_DEFAULTS,1)==0xFE000u);
    for(unsigned obj=0;obj<OBJ_STRIP_DEFAULTS;obj++)for(unsigned copy=0;copy<2;copy++)
        for(unsigned bc=0;bc<2;bc++)bad += check("defaults excludes existing object sector",st_sector(obj,copy)!=st_sector(OBJ_STRIP_DEFAULTS,bc));
    memset(nor+0x9F000u,0xFF,4096);memset(nor+0xFE000u,0xFF,4096);
    bad += check("empty defaults distinguished",st_load(OBJ_STRIP_DEFAULTS,got,sizeof got)==-1);
    bad += check("defaults first A save",st_save(OBJ_STRIP_DEFAULTS,a,212)==0);
    for(int phase=0;phase<2;phase++){
        fail_after=phase;bad += check("torn defaults returns failure",st_save(OBJ_STRIP_DEFAULTS,b,212)!=0);fail_after=-1;
        bad += check("torn defaults preserves committed A",st_load(OBJ_STRIP_DEFAULTS,got,sizeof got)==212&&!memcmp(got,a,212));
    }
    bad += check("defaults B commit/readback",st_save(OBJ_STRIP_DEFAULTS,b,212)==0&&st_load(OBJ_STRIP_DEFAULTS,got,sizeof got)==212&&!memcmp(got,b,212));
    nor[0xFE000u+ST_PAYLOAD_OFF]^=1;
    bad += check("damaged newest defaults falls back",st_load(OBJ_STRIP_DEFAULTS,got,sizeof got)==212&&!memcmp(got,a,212));
    nor[0x9F000u+ST_PAYLOAD_OFF]^=1;
    bad += check("damaged defaults distinguished from missing",st_load(OBJ_STRIP_DEFAULTS,got,sizeof got)==-2);
    /* A valid native NOR commit can carry an oversized SDF1-prefix object.
     * The generic storage API deliberately returns a bounded prefix; defaults
     * validation must use max213 to distinguish it from the exact212 format. */
    {
        uint8_t future[ST_PAYLOAD_MAX],prefix[213];
        memset(future,0xA5,sizeof future);memset(future,0,212);
        memcpy(future,"SDF1",4);future[4]=1;future[6]=212;future[8]=1;
        future[12]=20;future[14]=0x20;future[15]=0x4E;future[17]=1;future[18]=80;
        for(unsigned length=213;length<=ST_PAYLOAD_MAX;length=length==213?ST_PAYLOAD_MAX:ST_PAYLOAD_MAX+1){
            bad += check("oversized defaults native NOR commit",st_save(OBJ_STRIP_DEFAULTS,future,length)==0);
            bad += check("max212 truncates oversized native object",st_load(OBJ_STRIP_DEFAULTS,prefix,212)==212&&!memcmp(prefix,future,212));
            bad += check("max213 exposes oversized native object",st_load(OBJ_STRIP_DEFAULTS,prefix,213)==213&&!memcmp(prefix,future,213));
        }
    }
    printf("%s\n", bad ? "STORAGE TEST FAILED" : "storage test passed");
    return bad != 0;
}
