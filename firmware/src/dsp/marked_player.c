/* SPDX-License-Identifier: GPL-3.0-only */
#include "marked_player.h"
#include <string.h>
int marked_load(marked_player_t *p, const float *pcm, uint32_t frames, uint32_t rate, int channels,
                const marked_slice_t *slices, int count, uint32_t length)
{
    int i;
    if (!p || !pcm || !slices || !frames || frames > MARKED_MAX_FRAMES || rate < 8000 || rate > 48000 ||
        channels < 1 || channels > 2 || count < 1 || count > MARKED_MAX_SLICES || !length || length > 44100u * 60u)
        return -1;
    for (i = 0; i < count; i++)
        if (slices[i].start >= slices[i].end || slices[i].end > frames || slices[i].time >= length ||
            (i && (slices[i].start <= slices[i - 1].start || slices[i].time < slices[i - 1].time)))
            return -2;
    memset(p, 0, sizeof(*p));
    p->pcm = pcm; p->frames = frames; p->rate = rate; p->channels = (uint8_t)channels;
    p->count = (uint8_t)count; p->length = length; p->last_slice = 255;
    memcpy(p->slices, slices, count * sizeof(*slices));
    return 0;
}
void marked_stop(marked_player_t *p)
{
    int i;
    p->playing = 0;
    for (i = 0; i < MARKED_VOICES; i++) p->voice[i].on = 0;
}
void marked_play(marked_player_t *p, int repeat)
{
    marked_stop(p); p->cursor = p->next = 0; p->repeat = (uint8_t)!!repeat;
    p->triggers = 0; p->last_slice = 255; p->playing = !!p->count;
}
int marked_trigger(marked_player_t *p, int slice)
{
    int i;
    if (slice < 0 || slice >= p->count) return -1;
    for (i = 0; i < MARKED_VOICES; i++) if (!p->voice[i].on) {
        p->voice[i].pos = p->slices[slice].start; p->voice[i].end = p->slices[slice].end;
        p->voice[i].frac = 0; p->voice[i].on = 1;
        p->triggers++; p->last_slice = (uint8_t)slice; p->last_frame = p->cursor;
        return 0;
    }
    return -2; /* caller validates overlap capacity; never steal a sounding slice silently */
}
void marked_render(marked_player_t *p, float *l, float *r, uint32_t n)
{
    uint32_t i; int v;
    float inc = (float)p->rate / 44100.0f;
    for (i = 0; i < n; i++) {
        l[i] = r[i] = 0;
        if (p->playing) {
            while (p->next < p->count && p->slices[p->next].time <= p->cursor) {
                if (marked_trigger(p, p->next)) { marked_stop(p); break; }
                p->next++;
            }
        }
        for (v = 0; v < MARKED_VOICES; v++) {
            marked_voice_t *q = &p->voice[v];
            uint32_t a, b; float x, y;
            if (!q->on) continue;
            a = q->pos * p->channels;
            b = (q->pos + 1 < q->end ? q->pos + 1 : q->pos) * p->channels;
            x = p->pcm[a] + (p->pcm[b] - p->pcm[a]) * q->frac;
            y = p->channels == 2 ? p->pcm[a + 1] + (p->pcm[b + 1] - p->pcm[a + 1]) * q->frac : x;
            l[i] += x; r[i] += y;
            q->frac += inc;
            while (q->frac >= 1) { q->frac -= 1; if (++q->pos >= q->end) { q->on = 0; break; } }
        }
        if (p->playing && ++p->cursor >= p->length) {
            if (p->repeat) { p->cursor = 0; p->next = 0; }
            else p->playing = 0;
        }
    }
}
