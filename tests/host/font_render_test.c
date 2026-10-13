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
#define X0X_HOST 1
#ifndef FONT_GFX_SOURCE
#define FONT_GFX_SOURCE "../../firmware/src/gfx.c"
#endif
#include FONT_GFX_SOURCE

static const felucca_font_t *fonts[] = {&FONT_T, &FONT_XS, &FONT_S, &FONT_B, &FONT_M, &FONT_L};

#if FONT_HUFFMAN
static uint32_t test_encode(const uint8_t *symbols, uint32_t count, uint8_t *encoded)
{
    uint32_t buffer = 0, bits = 0, bytes = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t index = 0, length = 1;
        while (FONT_HUFFMAN_SYMBOL[index] != symbols[i])
            index++;
        while (index >= FONT_HUFFMAN_FIRST_SYMBOL[length] + FONT_HUFFMAN_COUNT[length])
            length++;
        uint32_t code = FONT_HUFFMAN_FIRST_CODE[length] + index - FONT_HUFFMAN_FIRST_SYMBOL[length];
        buffer = (buffer << length) | code;
        bits += length;
        while (bits >= 8) {
            bits -= 8;
            encoded[bytes++] = (uint8_t)(buffer >> bits);
        }
    }
    if (bits)
        encoded[bytes++] = (uint8_t)(buffer << (8u - bits));
    return bytes;
}
#endif

static void malformed(void)
{
#if FONT_RLE && !FONT_HUFFMAN
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
#if FONT_HUFFMAN
    uint8_t encoded[16];
    /* Independent inverse of the generated canonical tables: all 256 symbols
     * exercise the real bit reader, including the longest 15-bit codes. */
    for (uint32_t symbol = 0; symbol < 256; symbol++) {
        uint32_t index = 0, length = 1;
        while (FONT_HUFFMAN_SYMBOL[index] != symbol)
            index++;
        while (index >= FONT_HUFFMAN_FIRST_SYMBOL[length] + FONT_HUFFMAN_COUNT[length])
            length++;
        uint32_t code = FONT_HUFFMAN_FIRST_CODE[length] + index - FONT_HUFFMAN_FIRST_SYMBOL[length];
        code <<= 16u - length;
        encoded[0] = (uint8_t)(code >> 8);
        encoded[1] = (uint8_t)code;
        font_reader_t r = {.p = encoded, .end = encoded + (length + 7u) / 8u};
        assert(font_symbol(&r) == (int32_t)symbol);
        r.bits = 0;                        /* omit padding, not another symbol */
        assert(font_symbol(&r) == -1);
        if (length > 8) {
            r = (font_reader_t){.p = encoded, .end = encoded + 1};
            assert(font_symbol(&r) == -1); /* incomplete long code */
        }
    }
    font_reader_t r = {.p = encoded, .end = encoded};
    assert(font_next(&r) == 0);
    r = (font_reader_t){.p = encoded, .end = encoded, .left = 128, .repeated = 1, .value = 0x42};
    for (int i = 0; i < 128; i++)
        assert(font_next(&r) == 0x42);
    assert(font_next(&r) == 0);
    r = (font_reader_t){.p = encoded, .end = encoded, .left = 4};
    for (int i = 0; i < 8; i++)
        assert(font_next(&r) == 0);         /* truncated literal */
    r = (font_reader_t){.p = encoded, .end = encoded, .bits = 1};
    assert(font_next(&r) == 0);             /* incomplete control code */
    const uint8_t packets[] = {0x81, 0x35, 0, 0xab, 255, 0x42};
    uint32_t bytes = test_encode(packets, sizeof packets, encoded);
    r = (font_reader_t){.p = encoded, .end = encoded + bytes};
    assert(font_next(&r) == 0x35);
    assert(font_next(&r) == 0x35);
    assert(font_next(&r) == 0xab);
    for (int i = 0; i < 128; i++)
        assert(font_next(&r) == 0x42);
    r.bits = 0;
    assert(font_next(&r) == 0);
    /* A byte-aligned repeat control with its value missing must fail closed. */
    uint8_t repeat = 0;
    for (uint32_t i = FONT_HUFFMAN_FIRST_SYMBOL[8];
         i < FONT_HUFFMAN_FIRST_SYMBOL[8] + FONT_HUFFMAN_COUNT[8]; i++)
        if (FONT_HUFFMAN_SYMBOL[i] >= 128)
            repeat = FONT_HUFFMAN_SYMBOL[i];
    assert(repeat >= 128);
    bytes = test_encode(&repeat, 1, encoded);
    assert(bytes == 1);
    r = (font_reader_t){.p = encoded, .end = encoded + bytes};
    assert(font_next(&r) == 0);
    /* Every byte truncation of every real glyph must remain inside its own
     * input slice even when the renderer asks for the full original bitmap. */
    for (uint32_t face = 0; face < sizeof fonts / sizeof fonts[0]; face++) {
        const felucca_font_t *f = fonts[face];
        for (uint32_t gi = 0; gi <= (uint32_t)(f->last - f->first); gi++) {
            const uint8_t *start = f->data + f->off[gi];
            uint32_t size = f->off[gi + 1u] - f->off[gi];
            uint32_t wanted = f->h * ((f->bw[gi] + 1u) / 2u);
            for (uint32_t cut = 0; cut <= size; cut++) {
                r = (font_reader_t){.p = start, .end = start + cut};
                for (uint32_t i = 0; i < wanted; i++)
                    (void)font_next(&r);
                assert(r.p <= r.end);
            }
        }
    }
#endif
}

int main(int argc, char **argv)
{
    const int benchmark = argc > 1 && argv[1][0] == 'b';
    int loops = benchmark ? 200 : 1;
    uint32_t frames = 0, checksum = 0;
    malformed();
    clock_t started = clock();
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
