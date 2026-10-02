// alaw conversions

#include "l2.h"

static uint8_t alaw_of_u8[256];

static uint8_t linear_to_alaw(int pcm) {
    static const int seg_end[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
    int mask, seg;
    pcm >>= 3;
    if (pcm >= 0) {
        mask = 0xD5;
    } else {
        mask = 0x55;
        pcm = -pcm - 1;
    }
    for (seg = 0; seg < 8 && pcm > seg_end[seg]; seg++) {}
    if (seg >= 8)
        return (uint8_t)(0x7F ^ mask);
    int aval = seg << 4;
    aval |= seg < 2 ? (pcm >> 1) & 0xF : (pcm >> seg) & 0xF;
    return (uint8_t)(aval ^ mask);
}

uint32_t l2_alaw_length(uint32_t src_len, uint32_t src_rate) {
    return (uint32_t)((uint64_t)src_len * L2_VOICE_RATE / src_rate);
}

void l2_pcm8_to_alaw(const uint8_t *src, uint32_t src_len, uint32_t src_rate, uint8_t *dst, uint32_t dst_len) {
    if (!alaw_of_u8[0]) {
        for (int i = 0; i < 256; i++)
            alaw_of_u8[i] = linear_to_alaw((i - 128) * 256);
    }
    uint32_t step = (uint32_t)(((uint64_t)src_rate << 16) / L2_VOICE_RATE), pos = 0;
    for (uint32_t i = 0; i < dst_len; i++, pos += step) {
        uint32_t at = pos >> 16, frac = pos & 0xFFFF;
        int a = src[at], b = at + 1 < src_len ? src[at + 1] : a;
        dst[i] = alaw_of_u8[a + (((b - a) * (int)frac) >> 16)];
    }
}

void l2_pcm16_to_alaw(const int16_t *src, uint32_t n, uint8_t *dst) {
    for (uint32_t i = 0; i < n; i++)
        dst[i] = linear_to_alaw(src[i]);
}
