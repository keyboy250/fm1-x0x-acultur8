/* SPDX-License-Identifier: GPL-3.0-only */
/* Execute the actual intro and gfx rasterizer with a simulated wraparound clock
 * and USB/button arrivals. No target hardware or MIDI access. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define X0X_HOST 1
#define FM1_TICKS_PER_US 24u
static uint16_t screen[240*240];
static uint32_t elapsed, base, fm1_ms, feeds, services, frames, trigger;
static struct { uint32_t buttons; } fm1_in;
static struct { uint32_t ota_req, uboot_req; } usb;
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p)
{
    assert(x+w<=240 && y+h<=240);
    for (uint32_t r=0;r<h;r++) memcpy(screen+(y+r)*240+x,p+r*w,w*2);
    frames++;
}
#include "../firmware/src/gfx.c"
static uint32_t fm1_ticks(void) { fm1_ms=elapsed++; return base+fm1_ms*24000u; }
static void fm1_wdt_feed(void) { feeds++; }
static void usb_retry(uint32_t now)
{
    services++;
    if (now>=120) {
        if (trigger==1) usb.ota_req=1;
        if (trigger==2) usb.uboot_req=1;
        if (trigger==3) fm1_in.buttons=1;
    }
}
static void ota_service(void) {}
#include "../firmware/src/app/boot_intro.h"
static void reset(uint32_t mode)
{
    elapsed=fm1_ms=feeds=services=frames=0;
    base=0xFFFF0000u; /* deliberately crosses the real32-bit tick boundary */
    trigger=mode; fm1_in.buttons=usb.ota_req=usb.uboot_req=0;
    memset(screen,0,sizeof screen);
}
static void ppm(const char *path)
{
    FILE *f=fopen(path,"wb"); assert(f); fprintf(f,"P6\n240 240\n255\n");
    for (uint32_t i=0;i<240*240;i++) {
        uint16_t p=swap16(screen[i]);
        unsigned char rgb[3]={(unsigned char)(((p>>11)&31)*255/31),
          (unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};
        assert(fwrite(rgb,1,3,f)==3);
    }
    fclose(f);
}
int main(void)
{
    assert(text_w(&FONT_S,"EXTENDED EDITION")<=240);
    assert(text_w(&FONT_S,"RHYTHM / BASS / BREAKS")<=240);
    palette_set(1); reset(0); boot_intro();
    assert(fm1_ms>=3000 && fm1_ms<=3002);
    assert(frames==75 && feeds>2900 && services==feeds);
    for (uint32_t mode=1;mode<=3;mode++) {
        reset(mode); boot_intro();
        assert(fm1_ms>=120 && fm1_ms<=122 && frames<=4);
        assert(services==feeds);
    }
    reset(0); fm1_in.buttons=1; boot_intro(); assert(!frames && fm1_ms<=2);
    for (uint32_t ms=0;ms<=3000;ms++) boot_intro_frame(ms);
    boot_intro_frame(1600);
    static uint16_t held[240*240]; memcpy(held,screen,sizeof held);
    boot_intro_frame(2900); assert(!memcmp(held,screen,sizeof held));
    ppm("build/boot-intro-logo.ppm");
    boot_intro_frame(3000);
    for (uint32_t i=48*240;i<192*240;i++) assert(screen[i]==0);
    boot_intro_frame(400); ppm("build/boot-intro-grid.ppm");
    boot_intro_frame(850); ppm("build/boot-intro-morph.ppm");
    puts("PASS: 3s deadline across timer wrap, 25fps bound, watchdog/USB service, OTA/ROM/button exit, held-button skip, every-ms sanitizer sweep, stable logo hold.");
    return 0;
}
