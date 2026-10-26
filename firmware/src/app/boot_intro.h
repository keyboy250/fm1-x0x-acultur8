/* SPDX-License-Identifier: GPL-3.0-only */
/* Procedural intro: reuse gfx's existing strip canvas. No movie, heap, or
 * persistent state. Called only after USB/input IRQs start, outside safe mode. */
#ifdef X0X_VERSION
#define BOOT_VERSION_TEXT "v" X0X_VERSION
#else
#define BOOT_VERSION_TEXT "vDEV"
#endif
static void boot_intro_frame(uint32_t ms)
{
    static const uint8_t rows[2][5] = {{17,10,4,10,17},{14,17,17,17,14}};
    const uint16_t cyan = RGB(106,221,244), amber = RGB(255,188,94);
    uint32_t a = ms < 550u ? 0u : ms >= 1250u ? 256u : (ms - 550u) * 256u / 700u;
    int32_t last_x = 0, last_y = 0;
    uint32_t have_last = 0;
    a = a * a * (768u - 2u * a) / 65536u;
    cv_begin(240,144,C_BLACK);
    if (a < 220u) {
        for (int32_t k=0;k<240;k+=20) cv_line(k,0,k,143,RGB(16,36,48));
        for (int32_t k=0;k<144;k+=20) cv_line(0,k,239,k,RGB(16,36,48));
    }
    for (uint32_t g=0;g<3u;g++) for (uint32_t y=0;y<5u;y++) for (uint32_t x=0;x<5u;x++) {
        uint32_t i=g*25u+y*5u+x;
        int32_t sx=12+(int32_t)(i*216u/74u), q=(int32_t)((i*19u+ms/12u)&127u);
        int32_t sy=72+((q<64?q:127-q)-32)*(int32_t)(2u+i%3u)/2;
        int32_t tx=49+(int32_t)(g*52u+x*8u), ty=44+(int32_t)y*8;
        int32_t px=(sx*(int32_t)(256u-a)+tx*(int32_t)a)/256;
        int32_t py=(sy*(int32_t)(256u-a)+ty*(int32_t)a)/256;
        if (!(rows[g==1u][y] & (1u<<(4u-x)))) continue;
        if (!a && sx>(int32_t)(ms*240u/650u)) continue;
        if (have_last && a<220u) cv_line(last_x,last_y,px,py,RGB(42,113,145));
        cv_rect(px,py,6,6,i%5u?cyan:amber);
        last_x=px; last_y=py; have_last=1;
    }
    if (ms<800u) cv_rect((int32_t)(ms*239u/800u),0,1,144,cyan);
    if (ms>=1120u) {
        cv_text((240-text_w(&FONT_S,"EXTENDED EDITION"))/2,93,&FONT_S,"EXTENDED EDITION",amber);
        cv_text((240-text_w(&FONT_S,"RHYTHM / BASS / BREAKS"))/2,111,&FONT_S,"RHYTHM / BASS / BREAKS",C_GRAY);
    }
    cv_text((240-text_w(&FONT_T,BOOT_VERSION_TEXT))/2,128,&FONT_T,BOOT_VERSION_TEXT,C_GRAY);
    cv_rect(50,141,(int32_t)((ms<1300u?ms:1300u)*140u/1300u),2,amber);
    /* Match the preview: hold until 2.9s, then fade over the final 100ms. */
    if (ms>2900u) {
        uint32_t level=ms>=3000u?0u:(3000u-ms)*256u/100u;
        for (uint32_t i=0;i<240u*144u;i++) {
            uint32_t p=swap16(cv_px[i]);
            uint32_t r=((p>>11)*level)>>8;
            uint32_t g=(((p>>5)&63u)*level)>>8;
            uint32_t b=((p&31u)*level)>>8;
            cv_px[i]=swap16((r<<11)|(g<<5)|b);
        }
    }
    cv_blit(0,48);
}

static void boot_intro(void)
{
    uint32_t start=fm1_ticks(), last=0xFFFFFFFFu;
    for (;;) {
        uint32_t ms=(uint32_t)(fm1_ticks()-start)/(1000u*FM1_TICKS_PER_US);
        fm1_wdt_feed();
        usb_retry(fm1_ms);
        ota_service();                         /* identity requests remain responsive */
        if (ms>=3000u || fm1_in.buttons || usb.ota_req || usb.uboot_req) break;
        if (ms/40u!=last) {                     /* at most25 frames/s; no busy delay */
            last=ms/40u;
            boot_intro_frame(ms);
        }
    }
    lcd_sync();                                /* finish DMA before ui_init reuses canvas */
}
