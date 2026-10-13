/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the production renderer against raw and packed generated fonts. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "../../firmware/src/gfx.c"

static const felucca_font_t *fonts[] = {&FONT_T, &FONT_XS, &FONT_S, &FONT_B, &FONT_M, &FONT_L};

static void malformed(void)
{
#if FONT_RLE
    const uint8_t raw[] = {0x81, 0x35, 0, 0xab};
    font_reader_t r = {raw, raw + sizeof raw, 0, 0, 0};
    assert(font_next(&r) == 0x35);
    assert(font_next(&r) == 0x35);
    assert(font_next(&r) == 0xab);
    assert(font_next(&r) == 0);
    r = (font_reader_t){raw, raw, 0, 0, 0};
    assert(font_next(&r) == 0);
    r = (font_reader_t){raw, raw + 1, 0, 0, 0};
    assert(font_next(&r) == 0);              /* repeat control without value */
    const uint8_t truncated[] = {3, 0xab};
    r = (font_reader_t){truncated, truncated + 2, 0, 0, 0};
    assert(font_next(&r) == 0xab);
    for (int i = 0; i < 8; i++)
        assert(font_next(&r) == 0);         /* short literal: bounded, transparent */
    const uint8_t longest[] = {255, 0x42};
    r = (font_reader_t){longest, longest + 2, 0, 0, 0};
    for (int i = 0; i < 128; i++)
        assert(font_next(&r) == 0x42);
    assert(font_next(&r) == 0);
#endif
}

int main(int argc, char **argv)
{
    const int benchmark = argc > 1 && argv[1][0] == 'b';
    int loops = benchmark ? 200 : 1;
    uint32_t frames = 0, checksum = 0;
    clock_t started = clock();
    malformed();
    palette_set(1);
    if (benchmark)
        cv_begin(73, 67, RGB(30, 60, 120));
    for (int repeat = 0; repeat < loops; repeat++)
        for (uint32_t face = 0; face < sizeof fonts / sizeof fonts[0]; face++) {
            const felucca_font_t *f = fonts[face];
            for (uint32_t ch = f->first; ch <= f->last; ch++)
                for (int clip = 0; clip < 4; clip++) {
                    char s[3] = {(char)ch, (char)ch, 0};
                    if (!benchmark) {
                        cv_begin(73, 67, RGB(30, 60, 120));
                        cv_rect(0, 19, 73, 13, RGB(150, 20, 70));
                        cv_line(0, 0, 72, 66, C_WHITE);
                    }
                    if (clip == 3)
                        cv_band(21, 19);
                    else if (benchmark)
                        cv_band(0, 67);
                    int32_t x = clip == 1 ? -9 : clip == 2 ? 64 : 12;
                    int32_t y = clip == 1 ? -13 : clip == 2 ? 61 : 8;
                    int32_t end = cv_text(x, y, f, s, RGB(210, 180, 60));
                    int32_t width = text_w(f, s);
                    assert(end - x == width);
                    checksum += cv_px[34] + cv_px[1500] + (uint32_t)end;
                    if (!benchmark) {
                        if (fwrite(&end, sizeof end, 1, stdout) != 1 ||
                            fwrite(&width, sizeof width, 1, stdout) != 1 ||
                            fwrite(cv_px, sizeof(uint16_t), cv_w * cv_h, stdout) != cv_w * cv_h)
                            return 2;
                    }
                    frames++;
                }
        }
    if (benchmark)
        printf("frames=%u seconds=%.6f checksum=%u\n", frames,
               (double)(clock() - started) / CLOCKS_PER_SEC, checksum);
    else
        fprintf(stderr, "frames=%u checksum=%u\n", frames, checksum);
    return 0;
}
