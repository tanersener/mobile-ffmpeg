/*
 * Copyright (C) 2015 Vabishchevich Nikolay <vabnick@gmail.com>
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

#include <math.h>
#include <stdbool.h>

#include "ass_utils.h"
#include "ass_bitmap.h"


/*
 * Cascade Blur Algorithm
 *
 * The main idea is simple: to approximate gaussian blur with large radius
 * you can downscale, then apply filter with small pattern, then upscale back.
 *
 * To achieve desired precision down/upscaling should be done with sufficiently smooth kernel.
 * Experiment shows that downscaling of factor 2 with kernel [1, 5, 10, 10, 5, 1] and
 * corresponding upscaling are enough for 8-bit precision.
 *
 * For central filter here is used generic 9-tap filter with one of 3 different patterns
 * combined with one of optional prefilters with fixed kernels. Kernel coefficients
 * of the main filter are obtained from solution of least squares problem
 * for Fourier transform of resulting kernel.
 */


#define STRIPE_WIDTH  (1 << (C_ALIGN_ORDER - 1))
#define STRIPE_MASK   (STRIPE_WIDTH - 1)
static int16_t zero_line[STRIPE_WIDTH];
static int16_t dither_line[2 * STRIPE_WIDTH] = {
#if STRIPE_WIDTH > 8
     8, 40,  8, 40,  8, 40,  8, 40,  8, 40,  8, 40,  8, 40,  8, 40,
    56, 24, 56, 24, 56, 24, 56, 24, 56, 24, 56, 24, 56, 24, 56, 24,
#else
     8, 40,  8, 40,  8, 40,  8, 40,
    56, 24, 56, 24, 56, 24, 56, 24,
#endif
};

inline static const int16_t *get_line(const int16_t *ptr, uintptr_t offs, uintptr_t size)
{
    return offs < size ? ptr + offs : zero_line;
}

inline static void copy_line(int16_t *buf, const int16_t *ptr, uintptr_t offs, uintptr_t size)
{
    ptr = get_line(ptr, offs, size);
    for (int k = 0; k < STRIPE_WIDTH; ++k)
        buf[k] = ptr[k];
}

/*
 * Unpack/Pack Functions
 *
 * Convert between regular 8-bit bitmap and internal format.
 * Internal image is stored as set of vertical stripes of size [STRIPE_WIDTH x height].
 * Each pixel is represented as 16-bit integer in range of [0-0x4000].
 */

void ass_stripe_unpack_c(int16_t *dst, const uint8_t *src, ptrdiff_t src_stride,
                         uintptr_t width, uintptr_t height)
{
    for (uintptr_t y = 0; y < height; ++y) {
        int16_t *ptr = dst;
        for (uintptr_t x = 0; x < width; x += STRIPE_WIDTH) {
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                ptr[k] = (uint16_t) (((src[x + k] << 7) | (src[x + k] >> 1)) + 1) >> 1;
                //ptr[k] = (0x4000 * src[x + k] + 127) / 255;
            ptr += STRIPE_WIDTH * height;
        }
        dst += STRIPE_WIDTH;
        src += src_stride;
    }
}

void ass_stripe_pack_c(uint8_t *dst, ptrdiff_t dst_stride, const int16_t *src,
                       uintptr_t width, uintptr_t height)
{
    for (uintptr_t x = 0; x < width; x += STRIPE_WIDTH) {
        uint8_t *ptr = dst;
        for (uintptr_t y = 0; y < height; ++y) {
            const int16_t *dither = dither_line + (y & 1) * STRIPE_WIDTH;
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                ptr[k] = (uint16_t) (src[k] - (src[k] >> 8) + dither[k]) >> 6;
                //ptr[k] = (255 * src[k] + 0x1FFF) / 0x4000;
            ptr += dst_stride;
            src += STRIPE_WIDTH;
        }
        dst += STRIPE_WIDTH;
    }
    uintptr_t left = dst_stride - ((width + STRIPE_MASK) & ~STRIPE_MASK);
    for (uintptr_t y = 0; y < height; ++y) {
        for (uintptr_t x = 0; x < left; ++x)
            dst[x] = 0;
        dst += dst_stride;
    }
}

/*
 * Contract Filters
 *
 * Contract image by factor 2 with kernel [1, 5, 10, 10, 5, 1].
 */

static inline int16_t shrink_func(int16_t p1p, int16_t p1n,
                                  int16_t z0p, int16_t z0n,
                                  int16_t n1p, int16_t n1n)
{
    /*
    return (1 * p1p + 5 * p1n + 10 * z0p + 10 * z0n + 5 * n1p + 1 * n1n + 16) >> 5;
    */
    int32_t r = (p1p + p1n + n1p + n1n) >> 1;
    r = (r + z0p + z0n) >> 1;
    r = (r + p1n + n1p) >> 1;
    return (r + z0p + z0n + 2) >> 2;
}

void ass_shrink_horz_c(int16_t *dst, const int16_t *src,
                       uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_width = (src_width + 5) >> 1;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[3 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr + 0 * STRIPE_WIDTH, src, offs + 0 * step, size);
            copy_line(ptr + 1 * STRIPE_WIDTH, src, offs + 1 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = shrink_func(ptr[2 * k - 4], ptr[2 * k - 3],
                                     ptr[2 * k - 2], ptr[2 * k - 1],
                                     ptr[2 * k + 0], ptr[2 * k + 1]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        offs += step;
    }
}

void ass_shrink_vert_c(int16_t *dst, const int16_t *src,
                       uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_height = (src_height + 5) >> 1;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p1p = get_line(src, offs - 4 * STRIPE_WIDTH, step);
            const int16_t *p1n = get_line(src, offs - 3 * STRIPE_WIDTH, step);
            const int16_t *z0p = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *z0n = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n1p = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            const int16_t *n1n = get_line(src, offs + 1 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = shrink_func(p1p[k], p1n[k], z0p[k], z0n[k], n1p[k], n1n[k]);
            dst += STRIPE_WIDTH;
            offs += 2 * STRIPE_WIDTH;
        }
        src += step;
    }
}

/*
 * Expand Filters
 *
 * Expand image by factor 2 with kernel [5, 10, 1], [1, 10, 5].
 */

static inline void expand_func(int16_t *rp, int16_t *rn,
                               int16_t p1, int16_t z0, int16_t n1)
{
    /*
    *rp = (5 * p1 + 10 * z0 + 1 * n1 + 8) >> 4;
    *rn = (1 * p1 + 10 * z0 + 5 * n1 + 8) >> 4;
    */
    uint16_t r = (uint16_t) (((uint16_t) (p1 + n1) >> 1) + z0) >> 1;
    *rp = (uint16_t) (((uint16_t) (r + p1) >> 1) + z0 + 1) >> 1;
    *rn = (uint16_t) (((uint16_t) (r + n1) >> 1) + z0 + 1) >> 1;
}

void ass_expand_horz_c(int16_t *dst, const int16_t *src,
                       uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_width = 2 * src_width + 4;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = STRIPE_WIDTH; x < dst_width; x += 2 * STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH / 2; ++k)
                expand_func(&dst[2 * k], &dst[2 * k + 1],
                            ptr[k - 2], ptr[k - 1], ptr[k]);
            int16_t *next = dst + step - STRIPE_WIDTH;
            for (int k = STRIPE_WIDTH / 2; k < STRIPE_WIDTH; ++k)
                expand_func(&next[2 * k], &next[2 * k + 1],
                            ptr[k - 2], ptr[k - 1], ptr[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        dst += step;
    }
    if ((dst_width - 1) & STRIPE_WIDTH)
        return;

    for (uintptr_t y = 0; y < src_height; ++y) {
        copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
        copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
        for (int k = 0; k < STRIPE_WIDTH / 2; ++k)
            expand_func(&dst[2 * k], &dst[2 * k + 1],
                        ptr[k - 2], ptr[k - 1], ptr[k]);
        dst += STRIPE_WIDTH;
        offs += STRIPE_WIDTH;
    }
}

void ass_expand_vert_c(int16_t *dst, const int16_t *src,
                       uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_height = 2 * src_height + 4;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; y += 2) {
            const int16_t *p1 = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                expand_func(&dst[k], &dst[k + STRIPE_WIDTH],
                            p1[k], z0[k], n1[k]);
            dst += 2 * STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

/*
 * First Supplementary Filters
 *
 * Perform 1D convolution with kernel [1, 2, 1].
 */

static inline int16_t pre_blur1_func(int16_t p1, int16_t z0, int16_t n1)
{
    /*
    return (1 * p1 + 2 * z0 + 1 * n1 + 2) >> 2;
    */
    return (uint16_t) (((uint16_t) (p1 + n1) >> 1) + z0 + 1) >> 1;
}

void ass_pre_blur1_horz_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_width = src_width + 2;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur1_func(ptr[k - 2], ptr[k - 1], ptr[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_pre_blur1_vert_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_height = src_height + 2;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p1 = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur1_func(p1[k], z0[k], n1[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

/*
 * Second Supplementary Filters
 *
 * Perform 1D convolution with kernel [1, 4, 6, 4, 1].
 */

static inline int16_t pre_blur2_func(int16_t p2, int16_t p1, int16_t z0,
                                     int16_t n1, int16_t n2)
{
    /*
    return (1 * p2 + 4 * p1 + 6 * z0 + 4 * n1 + 1 * n2 + 8) >> 4;
    */
    uint16_t r1 = ((uint16_t) (((uint16_t) (p2 + n2) >> 1) + z0) >> 1) + z0;
    uint16_t r2 = p1 + n1;
    uint16_t r = ((uint16_t) (r1 + r2) >> 1) | (0x8000 & r1 & r2);
    return (uint16_t) (r + 1) >> 1;
}

void ass_pre_blur2_horz_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_width = src_width + 4;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur2_func(ptr[k - 4], ptr[k - 3], ptr[k - 2], ptr[k - 1], ptr[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_pre_blur2_vert_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_height = src_height + 4;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p2 = get_line(src, offs - 4 * STRIPE_WIDTH, step);
            const int16_t *p1 = get_line(src, offs - 3 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n2 = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur2_func(p2[k], p1[k], z0[k], n1[k], n2[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

/*
 * Third Supplementary Filters
 *
 * Perform 1D convolution with kernel [1, 6, 15, 20, 15, 6, 1].
 */

static inline int16_t pre_blur3_func(int16_t p3, int16_t p2, int16_t p1, int16_t z0,
                                     int16_t n1, int16_t n2, int16_t n3)
{
    /*
    return (1 * p3 + 6 * p2 + 15 * p1 + 20 * z0 + 15 * n1 + 6 * n2 + 1 * n3 + 32) >> 6;
    */
    return (20 * (uint16_t) z0 +
            15 * (uint16_t) (p1 + n1) +
             6 * (uint16_t) (p2 + n2) +
             1 * (uint16_t) (p3 + n3) + 32) >> 6;
}

void ass_pre_blur3_horz_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_width = src_width + 6;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur3_func(ptr[k - 6], ptr[k - 5], ptr[k - 4], ptr[k - 3],
                                        ptr[k - 2], ptr[k - 1], ptr[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_pre_blur3_vert_c(int16_t *dst, const int16_t *src,
                          uintptr_t src_width, uintptr_t src_height)
{
    uintptr_t dst_height = src_height + 6;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p3 = get_line(src, offs - 6 * STRIPE_WIDTH, step);
            const int16_t *p2 = get_line(src, offs - 5 * STRIPE_WIDTH, step);
            const int16_t *p1 = get_line(src, offs - 4 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs - 3 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *n2 = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n3 = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = pre_blur3_func(p3[k], p2[k], p1[k], z0[k], n1[k], n2[k], n3[k]);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

/*
 * Main 9-tap Parametric Filters
 *
 * Perform 1D convolution with kernel
 *         [c3, c2, c1, c0, d, c0, c1, c2, c3] or
 *     [c3,  0, c2, c1, c0, d, c0, c1, c2,  0, c3] or
 * [c3,  0, c2,  0, c1, c0, d, c0, c1,  0, c2,  0, c3] accordingly.
 *
 * cN = param[N], d = 1 - 2 * (c0 + c1 + c2 + c3).
 */

static inline int16_t blur_func(int16_t p4, int16_t p3, int16_t p2, int16_t p1, int16_t z0,
                                int16_t n1, int16_t n2, int16_t n3, int16_t n4, const int16_t c[])
{
    p1 -= z0;
    p2 -= z0;
    p3 -= z0;
    p4 -= z0;
    n1 -= z0;
    n2 -= z0;
    n3 -= z0;
    n4 -= z0;
    return (((p1 + n1) * c[0] +
             (p2 + n2) * c[1] +
             (p3 + n3) * c[2] +
             (p4 + n4) * c[3] +
             0x8000) >> 16) + z0;
}

void ass_blur1234_horz_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_width = src_width + 8;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(ptr[k - 8], ptr[k - 7], ptr[k - 6], ptr[k - 5], ptr[k - 4],
                                   ptr[k - 3], ptr[k - 2], ptr[k - 1], ptr[k - 0], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_blur1234_vert_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_height = src_height + 8;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p4 = get_line(src, offs - 8 * STRIPE_WIDTH, step);
            const int16_t *p3 = get_line(src, offs - 7 * STRIPE_WIDTH, step);
            const int16_t *p2 = get_line(src, offs - 6 * STRIPE_WIDTH, step);
            const int16_t *p1 = get_line(src, offs - 5 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs - 4 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs - 3 * STRIPE_WIDTH, step);
            const int16_t *n2 = get_line(src, offs - 2 * STRIPE_WIDTH, step);
            const int16_t *n3 = get_line(src, offs - 1 * STRIPE_WIDTH, step);
            const int16_t *n4 = get_line(src, offs - 0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(p4[k], p3[k], p2[k], p1[k], z0[k],
                                   n1[k], n2[k], n3[k], n4[k], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

void ass_blur1235_horz_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_width = src_width + 10;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
#if STRIPE_WIDTH < 10
    int16_t buf[3 * STRIPE_WIDTH];
    int16_t *ptr = buf + 2 * STRIPE_WIDTH;
#else
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
#endif
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
#if STRIPE_WIDTH < 10
            copy_line(ptr - 2 * STRIPE_WIDTH, src, offs - 2 * step, size);
#endif
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(ptr[k - 10], ptr[k - 8], ptr[k - 7], ptr[k - 6], ptr[k - 5],
                                   ptr[k -  4], ptr[k - 3], ptr[k - 2], ptr[k - 0], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_blur1235_vert_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_height = src_height + 10;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p4 = get_line(src, offs - 10 * STRIPE_WIDTH, step);
            const int16_t *p3 = get_line(src, offs -  8 * STRIPE_WIDTH, step);
            const int16_t *p2 = get_line(src, offs -  7 * STRIPE_WIDTH, step);
            const int16_t *p1 = get_line(src, offs -  6 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs -  5 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs -  4 * STRIPE_WIDTH, step);
            const int16_t *n2 = get_line(src, offs -  3 * STRIPE_WIDTH, step);
            const int16_t *n3 = get_line(src, offs -  2 * STRIPE_WIDTH, step);
            const int16_t *n4 = get_line(src, offs -  0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(p4[k], p3[k], p2[k], p1[k], z0[k],
                                   n1[k], n2[k], n3[k], n4[k], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}

void ass_blur1246_horz_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_width = src_width + 12;
    uintptr_t size = ((src_width + STRIPE_MASK) & ~STRIPE_MASK) * src_height;
    uintptr_t step = STRIPE_WIDTH * src_height;

    uintptr_t offs = 0;
#if STRIPE_WIDTH < 12
    int16_t buf[3 * STRIPE_WIDTH];
    int16_t *ptr = buf + 2 * STRIPE_WIDTH;
#else
    int16_t buf[2 * STRIPE_WIDTH];
    int16_t *ptr = buf + STRIPE_WIDTH;
#endif
    for (uintptr_t x = 0; x < dst_width; x += STRIPE_WIDTH) {
        for (uintptr_t y = 0; y < src_height; ++y) {
#if STRIPE_WIDTH < 12
            copy_line(ptr - 2 * STRIPE_WIDTH, src, offs - 2 * step, size);
#endif
            copy_line(ptr - 1 * STRIPE_WIDTH, src, offs - 1 * step, size);
            copy_line(ptr - 0 * STRIPE_WIDTH, src, offs - 0 * step, size);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(ptr[k - 12], ptr[k - 10], ptr[k - 8], ptr[k - 7], ptr[k - 6],
                                   ptr[k -  5], ptr[k -  4], ptr[k - 2], ptr[k - 0], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
    }
}

void ass_blur1246_vert_c(int16_t *dst, const int16_t *src,
                         uintptr_t src_width, uintptr_t src_height,
                         const int16_t *param)
{
    uintptr_t dst_height = src_height + 12;
    uintptr_t step = STRIPE_WIDTH * src_height;

    for (uintptr_t x = 0; x < src_width; x += STRIPE_WIDTH) {
        uintptr_t offs = 0;
        for (uintptr_t y = 0; y < dst_height; ++y) {
            const int16_t *p4 = get_line(src, offs - 12 * STRIPE_WIDTH, step);
            const int16_t *p3 = get_line(src, offs - 10 * STRIPE_WIDTH, step);
            const int16_t *p2 = get_line(src, offs -  8 * STRIPE_WIDTH, step);
            const int16_t *p1 = get_line(src, offs -  7 * STRIPE_WIDTH, step);
            const int16_t *z0 = get_line(src, offs -  6 * STRIPE_WIDTH, step);
            const int16_t *n1 = get_line(src, offs -  5 * STRIPE_WIDTH, step);
            const int16_t *n2 = get_line(src, offs -  4 * STRIPE_WIDTH, step);
            const int16_t *n3 = get_line(src, offs -  2 * STRIPE_WIDTH, step);
            const int16_t *n4 = get_line(src, offs -  0 * STRIPE_WIDTH, step);
            for (int k = 0; k < STRIPE_WIDTH; ++k)
                dst[k] = blur_func(p4[k], p3[k], p2[k], p1[k], z0[k],
                                   n1[k], n2[k], n3[k], n4[k], param);
            dst += STRIPE_WIDTH;
            offs += STRIPE_WIDTH;
        }
        src += step;
    }
}



static void calc_gauss(double *res, int n, double r2)
{
    double alpha = 0.5 / r2;
    double mul = exp(-alpha), mul2 = mul * mul;
    double cur = sqrt(alpha / M_PI);

    res[0] = cur;
    cur *= mul;
    res[1] = cur;
    for (int i = 2; i <= n; ++i) {
        mul *= mul2;
        cur *= mul;
        res[i] = cur;
    }
}

static void coeff_blur121(double *coeff, int n)
{
    double prev = coeff[1];
    for (int i = 0; i <= n; ++i) {
        double res = (prev + 2 * coeff[i] + coeff[i + 1]) / 4;
        prev = coeff[i];
        coeff[i] = res;
    }
}

static void coeff_filter(double *coeff, int n, const double kernel[4])
{
    double prev1 = coeff[1], prev2 = coeff[2], prev3 = coeff[3];
    for (int i = 0; i <= n; ++i) {
        double res = coeff[i + 0]  * kernel[0] +
            (prev1 + coeff[i + 1]) * kernel[1] +
            (prev2 + coeff[i + 2]) * kernel[2] +
            (prev3 + coeff[i + 3]) * kernel[3];
        prev3 = prev2;
        prev2 = prev1;
        prev1 = coeff[i];
        coeff[i] = res;
    }
}

static void calc_matrix(double mat[4][4], const double *mat_freq, const int *index)
{
    for (int i = 0; i < 4; ++i) {
        mat[i][i] = mat_freq[2 * index[i]] + 3 * mat_freq[0] - 4 * mat_freq[index[i]];
        for (int j = i + 1; j < 4; ++j)
            mat[i][j] = mat[j][i] =
                mat_freq[index[i] + index[j]] + mat_freq[index[j] - index[i]] +
                2 * (mat_freq[0] - mat_freq[index[i]] - mat_freq[index[j]]);
    }

    // invert transpose
    for (int k = 0; k < 4; ++k) {
        int ip = k, jp = k;  // pivot
        double z = 1 / mat[ip][jp];
        mat[ip][jp] = 1;
        for (int i = 0; i < 4; ++i) {
            if (i == ip)
                continue;

            double mul = mat[i][jp] * z;
            mat[i][jp] = 0;
            for (int j = 0; j < 4; ++j)
                mat[i][j] -= mat[ip][j] * mul;
        }
        for (int j = 0; j < 4; ++j)
            mat[ip][j] *= z;
    }
}

/**
 * \brief Solve least squares problem for kernel of the main filter
 * \param mu out: output coefficients
 * \param index in: filter tap positions
 * \param prefilter in: supplementary filter type
 * \param r2 in: desired standard deviation squared
 * \param mul in: scale multiplier
 */
static void calc_coeff(double mu[4], const int index[4], int prefilter, double r2, double mul)
{
    double mul2 = mul * mul, mul3 = mul2 * mul;
    double kernel[] = {
        (5204 + 2520 * mul + 1092 * mul2 + 3280 * mul3) / 12096,
        (2943 -  210 * mul -  273 * mul2 - 2460 * mul3) / 12096,
        ( 486 -  924 * mul -  546 * mul2 +  984 * mul3) / 12096,
        (  17 -  126 * mul +  273 * mul2 -  164 * mul3) / 12096,
    };

    double mat_freq[14];
    memcpy(mat_freq, kernel, sizeof(kernel));
    memset(mat_freq + 4, 0, sizeof(mat_freq) - sizeof(kernel));
    int n = 6;
    coeff_filter(mat_freq, n, kernel);
    for (int k = 0; k < 2 * prefilter; ++k)
        coeff_blur121(mat_freq, ++n);

    double vec_freq[13];
    n = index[3] + prefilter + 3;
    calc_gauss(vec_freq, n, r2);
    memset(vec_freq + n + 1, 0, sizeof(vec_freq) - (n + 1) * sizeof(vec_freq[0]));
    n -= 3;
    coeff_filter(vec_freq, n, kernel);
    for (int k = 0; k < prefilter; ++k)
        coeff_blur121(vec_freq, --n);

    double mat[4][4];
    calc_matrix(mat, mat_freq, index);

    double vec[4];
    for (int i = 0; i < 4; ++i)
        vec[i] = mat_freq[0] - mat_freq[index[i]] - vec_freq[0] + vec_freq[index[i]];

    for (int i = 0; i < 4; ++i) {
        double res = 0;
        for (int j = 0; j < 4; ++j)
            res += mat[i][j] * vec[j];
        mu[i] = FFMAX(0, res);
    }
}

typedef struct {
    int level, prefilter, filter;
    int16_t coeff[4];
} BlurMethod;

static void find_best_method(BlurMethod *blur, double r2)
{
    static const int index[][4] = {
        { 1, 2, 3, 4 },
        { 1, 2, 3, 5 },
        { 1, 2, 4, 6 },
    };

    double mu[5];
    if (r2 < 1.9) {
        blur->level = blur->prefilter = blur->filter = 0;

        if (r2 < 0.5) {
            mu[2] = 0.085 * r2 * r2 * r2;
            mu[1] = 0.5 * r2 - 4 * mu[2];
            mu[3] = mu[4] = 0;
        } else {
            calc_gauss(mu, 4, r2);
        }
    } else {
        double mul = 1;
        if (r2 < 6.693) {
            blur->level = 0;

            if (r2 < 2.8)
                blur->prefilter = 1;
            else if (r2 < 4.4)
                blur->prefilter = 2;
            else
                blur->prefilter = 3;

            blur->filter = blur->prefilter - 1;
        } else {
            frexp((r2 + 0.7) / 26.5, &blur->level);
            blur->level = (blur->level + 3) >> 1;
            mul = pow(0.25, blur->level);
            r2 *= mul;

            if (r2 < 3.15 - 1.5 * mul)
                blur->prefilter = 0;
            else if (r2 < 5.3 - 5.2 * mul)
                blur->prefilter = 1;
            else
                blur->prefilter = 2;

            blur->filter = blur->prefilter;
        }
        calc_coeff(mu + 1, index[blur->filter], blur->prefilter, r2, mul);
    }

    for (int i = 1; i <= 4; ++i)
        blur->coeff[i - 1] = (int) (0x10000 * mu[i] + 0.5);
}

/**
 * \brief Perform approximate gaussian blur
 * \param r2 in: desired standard deviation squared
 */
bool ass_gaussian_blur(const BitmapEngine *engine, Bitmap *bm, double r2)
{
    BlurMethod blur;
    find_best_method(&blur, r2);

    int w = bm->w, h = bm->h;
    int offset = ((2 * (blur.prefilter + blur.filter) + 17) << blur.level) - 5;
    int end_w = ((w + offset) & ~((1 << blur.level) - 1)) - 4;
    int end_h = ((h + offset) & ~((1 << blur.level) - 1)) - 4;

    const int stripe_width = 1 << (engine->align_order - 1);
    int size = end_h * ((end_w + stripe_width - 1) & ~(stripe_width - 1));
    int16_t *tmp = ass_aligned_alloc(2 * stripe_width, 4 * size, false);
    if (!tmp)
        return false;

    engine->stripe_unpack(tmp, bm->buffer, bm->stride, w, h);
    int16_t *buf[2] = {tmp, tmp + size};
    int index = 0;

    for (int i = 0; i < blur.level; ++i) {
        engine->shrink_vert(buf[index ^ 1], buf[index], w, h);
        h = (h + 5) >> 1;
        index ^= 1;
    }
    for (int i = 0; i < blur.level; ++i) {
        engine->shrink_horz(buf[index ^ 1], buf[index], w, h);
        w = (w + 5) >> 1;
        index ^= 1;
    }
    if (blur.prefilter) {
        engine->pre_blur_horz[blur.prefilter - 1](buf[index ^ 1], buf[index], w, h);
        w += 2 * blur.prefilter;
        index ^= 1;
    }
    engine->main_blur_horz[blur.filter](buf[index ^ 1], buf[index], w, h, blur.coeff);
    w += 2 * blur.filter + 8;
    index ^= 1;
    for (int i = 0; i < blur.level; ++i) {
        engine->expand_horz(buf[index ^ 1], buf[index], w, h);
        w = 2 * w + 4;
        index ^= 1;
    }
    if (blur.prefilter) {
        engine->pre_blur_vert[blur.prefilter - 1](buf[index ^ 1], buf[index], w, h);
        h += 2 * blur.prefilter;
        index ^= 1;
    }
    engine->main_blur_vert[blur.filter](buf[index ^ 1], buf[index], w, h, blur.coeff);
    h += 2 * blur.filter + 8;
    index ^= 1;
    for (int i = 0; i < blur.level; ++i) {
        engine->expand_vert(buf[index ^ 1], buf[index], w, h);
        h = 2 * h + 4;
        index ^= 1;
    }
    assert(w == end_w && h == end_h);

    if (!realloc_bitmap(engine, bm, w, h)) {
        ass_aligned_free(tmp);
        return false;
    }
    offset = ((blur.prefilter + blur.filter + 8) << blur.level) - 4;
    bm->left -= offset;
    bm->top  -= offset;

    engine->stripe_pack(bm->buffer, bm->stride, buf[index], w, h);
    ass_aligned_free(tmp);
    return true;
}

