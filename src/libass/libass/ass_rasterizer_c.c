/*
 * Copyright (C) 2014 Vabishchevich Nikolay <vabnick@gmail.com>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"
#include "ass_compat.h"

#include "ass_utils.h"
#include "ass_rasterizer.h"
#include <assert.h>



void ass_fill_solid_tile16_c(uint8_t *buf, ptrdiff_t stride, int set)
{
    uint8_t value = set ? 255 : 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++)
            buf[x] = value;
        buf += stride;
    }
}

void ass_fill_solid_tile32_c(uint8_t *buf, ptrdiff_t stride, int set)
{
    uint8_t value = set ? 255 : 0;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++)
            buf[x] = value;
        buf += stride;
    }
}


/*
 * Halfplane Filling Functions
 *
 * Fill pixels with antialiasing corresponding to equation
 * A * x + B * y < C, where
 * x, y - offset of pixel center from bottom-left,
 * A = a * scale, B = b * scale, C = c * scale / 64.
 *
 * Normalization of coefficients prior call:
 * max(abs(a), abs(b)) * scale = 1 << 61
 *
 * Used Algorithm
 * Let
 * max_ab = max(abs(A), abs(B)),
 * min_ab = min(abs(A), abs(B)),
 * CC = C - A * x - B * y, then
 * result = (clamp((CC - min_ab / 4) / max_ab) +
 *           clamp((CC + min_ab / 4) / max_ab) +
 *           1) / 2,
 * where clamp(Z) = max(-0.5, min(0.5, Z)).
 */

void ass_fill_halfplane_tile16_c(uint8_t *buf, ptrdiff_t stride,
                                 int32_t a, int32_t b, int64_t c, int32_t scale)
{
    int16_t aa = (a * (int64_t) scale + ((int64_t) 1 << 49)) >> 50;
    int16_t bb = (b * (int64_t) scale + ((int64_t) 1 << 49)) >> 50;
    int16_t cc = ((int32_t) (c >> 11) * (int64_t) scale + ((int64_t) 1 << 44)) >> 45;
    cc += (1 << 9) - ((aa + bb) >> 1);

    int16_t abs_a = aa < 0 ? -aa : aa;
    int16_t abs_b = bb < 0 ? -bb : bb;
    int16_t delta = (FFMIN(abs_a, abs_b) + 2) >> 2;

    int16_t va1[16], va2[16];
    for (int x = 0; x < 16; x++) {
        va1[x] = aa * x - delta;
        va2[x] = aa * x + delta;
    }

    static const int16_t full = (1 << 10) - 1;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int16_t c1 = cc - va1[x];
            int16_t c2 = cc - va2[x];
            c1 = FFMINMAX(c1, 0, full);
            c2 = FFMINMAX(c2, 0, full);
            buf[x] = (c1 + c2) >> 3;
        }
        buf += stride;
        cc -= bb;
    }
}

void ass_fill_halfplane_tile32_c(uint8_t *buf, ptrdiff_t stride,
                                 int32_t a, int32_t b, int64_t c, int32_t scale)
{
    int16_t aa = (a * (int64_t) scale + ((int64_t) 1 << 50)) >> 51;
    int16_t bb = (b * (int64_t) scale + ((int64_t) 1 << 50)) >> 51;
    int16_t cc = ((int32_t) (c >> 12) * (int64_t) scale + ((int64_t) 1 << 44)) >> 45;
    cc += (1 << 8) - ((aa + bb) >> 1);

    int16_t abs_a = aa < 0 ? -aa : aa;
    int16_t abs_b = bb < 0 ? -bb : bb;
    int16_t delta = (FFMIN(abs_a, abs_b) + 2) >> 2;

    int16_t va1[32], va2[32];
    for (int x = 0; x < 32; x++) {
        va1[x] = aa * x - delta;
        va2[x] = aa * x + delta;
    }

    static const int16_t full = (1 << 9) - 1;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int16_t c1 = cc - va1[x];
            int16_t c2 = cc - va2[x];
            c1 = FFMINMAX(c1, 0, full);
            c2 = FFMINMAX(c2, 0, full);
            buf[x] = (c1 + c2) >> 2;
        }
        buf += stride;
        cc -= bb;
    }
}


/*
 * Generic Filling Functions
 *
 * Used Algorithm
 * Construct trapeziod from each polyline segment and its projection into left side of tile.
 * Render that trapeziod into internal buffer with additive blending and correct sign.
 * Store clamped absolute value from internal buffer into result buffer.
 */

// Render top/bottom line of the trapeziod with antialiasing
static inline void update_border_line16(int16_t res[16],
                                        int16_t abs_a, const int16_t va[16],
                                        int16_t b, int16_t abs_b,
                                        int16_t c, int up, int dn)
{
    int16_t size = dn - up;
    int16_t w = (1 << 10) + (size << 4) - abs_a;
    w = FFMIN(w, 1 << 10) << 3;

    int16_t dc_b = abs_b * (int32_t) size >> 6;
    int16_t dc = (FFMIN(abs_a, dc_b) + 2) >> 2;

    int16_t base = (int32_t) b * (int16_t) (up + dn) >> 7;
    int16_t offs1 = size - ((base + dc) * (int32_t) w >> 16);
    int16_t offs2 = size - ((base - dc) * (int32_t) w >> 16);

    size <<= 1;
    for (int x = 0; x < 16; x++) {
        int16_t cw = (c - va[x]) * (int32_t) w >> 16;
        int16_t c1 = cw + offs1;
        int16_t c2 = cw + offs2;
        c1 = FFMINMAX(c1, 0, size);
        c2 = FFMINMAX(c2, 0, size);
        res[x] += c1 + c2;
    }
}

void ass_fill_generic_tile16_c(uint8_t *buf, ptrdiff_t stride,
                               const struct segment *line, size_t n_lines,
                               int winding)
{
    int16_t res[16][16], delta[18];
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            res[y][x] = 0;
    for (int y = 0; y < 18; y++)
        delta[y] = 0;

    static const int16_t full = 1 << 10;
    const struct segment *end = line + n_lines;
    for (; line != end; ++line) {
        assert(line->y_min >= 0 && line->y_min < 1 << 10);
        assert(line->y_max > 0 && line->y_max <= 1 << 10);
        assert(line->y_min <= line->y_max);

        int16_t up_delta = line->flags & SEGFLAG_DN ? 4 : 0;
        int16_t dn_delta = up_delta;
        if (!line->x_min && (line->flags & SEGFLAG_EXACT_LEFT)) dn_delta ^= 4;
        if (line->flags & SEGFLAG_UL_DR) {
            int16_t tmp = up_delta;
            up_delta = dn_delta;
            dn_delta = tmp;
        }

        int up = line->y_min >> 6, dn = line->y_max >> 6;
        int16_t up_pos = line->y_min & 63;
        int16_t up_delta1 = up_delta * up_pos;
        int16_t dn_pos = line->y_max & 63;
        int16_t dn_delta1 = dn_delta * dn_pos;
        delta[up + 1] -= up_delta1;
        delta[up] -= (up_delta << 6) - up_delta1;
        delta[dn + 1] += dn_delta1;
        delta[dn] += (dn_delta << 6) - dn_delta1;
        if (line->y_min == line->y_max)
            continue;

        int16_t a = (line->a * (int64_t) line->scale + ((int64_t) 1 << 49)) >> 50;
        int16_t b = (line->b * (int64_t) line->scale + ((int64_t) 1 << 49)) >> 50;
        int16_t c = ((int32_t) (line->c >> 11) * (int64_t) line->scale + ((int64_t) 1 << 44)) >> 45;
        c -= (a >> 1) + b * up;

        int16_t va[16];
        for (int x = 0; x < 16; x++)
            va[x] = a * x;
        int16_t abs_a = a < 0 ? -a : a;
        int16_t abs_b = b < 0 ? -b : b;
        int16_t dc = (FFMIN(abs_a, abs_b) + 2) >> 2;
        int16_t base = (1 << 9) - (b >> 1);
        int16_t dc1 = base + dc;
        int16_t dc2 = base - dc;

        if (up_pos) {
            if (dn == up) {
                update_border_line16(res[up], abs_a, va, b, abs_b, c, up_pos, dn_pos);
                continue;
            }
            update_border_line16(res[up], abs_a, va, b, abs_b, c, up_pos, 64);
            up++;
            c -= b;
        }
        for (int y = up; y < dn; y++) {
            for (int x = 0; x < 16; x++) {
                int16_t c1 = c - va[x] + dc1;
                int16_t c2 = c - va[x] + dc2;
                c1 = FFMINMAX(c1, 0, full);
                c2 = FFMINMAX(c2, 0, full);
                res[y][x] += (c1 + c2) >> 3;
            }
            c -= b;
        }
        if (dn_pos)
            update_border_line16(res[dn], abs_a, va, b, abs_b, c, 0, dn_pos);
    }

    int16_t cur = 256 * winding;
    for (int y = 0; y < 16; y++) {
        cur += delta[y];
        for (int x = 0; x < 16; x++) {
            int16_t val = res[y][x] + cur, neg_val = -val;
            val = (val > neg_val ? val : neg_val);
            buf[x] = FFMIN(val, 255);
        }
        buf += stride;
    }
}

// Render top/bottom line of the trapeziod with antialiasing
static inline void update_border_line32(int16_t res[32],
                                        int16_t abs_a, const int16_t va[32],
                                        int16_t b, int16_t abs_b,
                                        int16_t c, int up, int dn)
{
    int16_t size = dn - up;
    int16_t w = (1 << 9) + (size << 3) - abs_a;
    w = FFMIN(w, 1 << 9) << 5;

    int16_t dc_b = abs_b * (int32_t) size >> 6;
    int16_t dc = (FFMIN(abs_a, dc_b) + 2) >> 2;

    int16_t base = (int32_t) b * (int16_t) (up + dn) >> 7;
    int16_t offs1 = size - ((base + dc) * (int32_t) w >> 16);
    int16_t offs2 = size - ((base - dc) * (int32_t) w >> 16);

    size <<= 1;
    for (int x = 0; x < 32; x++) {
        int16_t cw = (c - va[x]) * (int32_t) w >> 16;
        int16_t c1 = cw + offs1;
        int16_t c2 = cw + offs2;
        c1 = FFMINMAX(c1, 0, size);
        c2 = FFMINMAX(c2, 0, size);
        res[x] += c1 + c2;
    }
}

void ass_fill_generic_tile32_c(uint8_t *buf, ptrdiff_t stride,
                               const struct segment *line, size_t n_lines,
                               int winding)
{
    int16_t res[32][32], delta[34];
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            res[y][x] = 0;
    for (int y = 0; y < 34; y++)
        delta[y] = 0;

    static const int16_t full = 1 << 9;
    const struct segment *end = line + n_lines;
    for (; line != end; ++line) {
        assert(line->y_min >= 0 && line->y_min < 1 << 11);
        assert(line->y_max > 0 && line->y_max <= 1 << 11);
        assert(line->y_min <= line->y_max);

        int16_t up_delta = line->flags & SEGFLAG_DN ? 4 : 0;
        int16_t dn_delta = up_delta;
        if (!line->x_min && (line->flags & SEGFLAG_EXACT_LEFT)) dn_delta ^= 4;
        if (line->flags & SEGFLAG_UL_DR) {
            int16_t tmp = up_delta;
            up_delta = dn_delta;
            dn_delta = tmp;
        }

        int up = line->y_min >> 6, dn = line->y_max >> 6;
        int16_t up_pos = line->y_min & 63;
        int16_t up_delta1 = up_delta * up_pos;
        int16_t dn_pos = line->y_max & 63;
        int16_t dn_delta1 = dn_delta * dn_pos;
        delta[up + 1] -= up_delta1;
        delta[up] -= (up_delta << 6) - up_delta1;
        delta[dn + 1] += dn_delta1;
        delta[dn] += (dn_delta << 6) - dn_delta1;
        if (line->y_min == line->y_max)
            continue;

        int16_t a = (line->a * (int64_t) line->scale + ((int64_t) 1 << 50)) >> 51;
        int16_t b = (line->b * (int64_t) line->scale + ((int64_t) 1 << 50)) >> 51;
        int16_t c = ((int32_t) (line->c >> 12) * (int64_t) line->scale + ((int64_t) 1 << 44)) >> 45;
        c -= (a >> 1) + b * up;

        int16_t va[32];
        for (int x = 0; x < 32; x++)
            va[x] = a * x;
        int16_t abs_a = a < 0 ? -a : a;
        int16_t abs_b = b < 0 ? -b : b;
        int16_t dc = (FFMIN(abs_a, abs_b) + 2) >> 2;
        int16_t base = (1 << 8) - (b >> 1);
        int16_t dc1 = base + dc;
        int16_t dc2 = base - dc;

        if (up_pos) {
            if (dn == up) {
                update_border_line32(res[up], abs_a, va, b, abs_b, c, up_pos, dn_pos);
                continue;
            }
            update_border_line32(res[up], abs_a, va, b, abs_b, c, up_pos, 64);
            up++;
            c -= b;
        }
        for (int y = up; y < dn; y++) {
            for (int x = 0; x < 32; x++) {
                int16_t c1 = c - va[x] + dc1;
                int16_t c2 = c - va[x] + dc2;
                c1 = FFMINMAX(c1, 0, full);
                c2 = FFMINMAX(c2, 0, full);
                res[y][x] += (c1 + c2) >> 2;
            }
            c -= b;
        }
        if (dn_pos)
            update_border_line32(res[dn], abs_a, va, b, abs_b, c, 0, dn_pos);
    }

    int16_t cur = 256 * winding;
    for (int y = 0; y < 32; y++) {
        cur += delta[y];
        for (int x = 0; x < 32; x++) {
            int16_t val = res[y][x] + cur, neg_val = -val;
            val = (val > neg_val ? val : neg_val);
            buf[x] = FFMIN(val, 255);
        }
        buf += stride;
    }
}
