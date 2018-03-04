/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 * Copyright (C) 2011 Grigori Goronzy <greg@chown.ath.cx>
 * Copyright (c) 2011-2014, Yu Zhuohuang <yuzhuohuang@qq.com>
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "ass_utils.h"
#include "ass_outline.h"
#include "ass_bitmap.h"
#include "ass_render.h"


#define ALIGN           C_ALIGN_ORDER
#define DECORATE(func)  ass_##func##_c
#include "ass_func_template.h"
#undef ALIGN
#undef DECORATE

#if (defined(__i386__) || defined(__x86_64__)) && CONFIG_ASM

#define ALIGN           4
#define DECORATE(func)  ass_##func##_sse2
#include "ass_func_template.h"
#undef ALIGN
#undef DECORATE

#define ALIGN           5
#define DECORATE(func)  ass_##func##_avx2
#include "ass_func_template.h"
#undef ALIGN
#undef DECORATE

#endif


void ass_synth_blur(const BitmapEngine *engine, int opaque_box, int be,
                    double blur_radius, Bitmap *bm_g, Bitmap *bm_o)
{
    bool blur_g = !bm_o || opaque_box;
    if (blur_g && !bm_g)
        return;

    // Apply gaussian blur
    double r2 = blur_radius * blur_radius / log(256);
    if (r2 > 0.001) {
        if (bm_o)
            ass_gaussian_blur(engine, bm_o, r2);
        if (blur_g)
            ass_gaussian_blur(engine, bm_g, r2);
    }

    // Apply box blur (multiple passes, if requested)
    if (be) {
        size_t size_o = 0, size_g = 0;
        if (bm_o)
            size_o = sizeof(uint16_t) * bm_o->stride * 2;
        if (blur_g)
            size_g = sizeof(uint16_t) * bm_g->stride * 2;
        size_t size = FFMAX(size_o, size_g);
        uint16_t *tmp = size ? ass_aligned_alloc(32, size, false) : NULL;
        if (!tmp)
            return;
        if (bm_o) {
            unsigned passes = be;
            unsigned w = bm_o->w;
            unsigned h = bm_o->h;
            unsigned stride = bm_o->stride;
            unsigned char *buf = bm_o->buffer;
            if(w && h){
                if(passes > 1){
                    be_blur_pre(buf, w, h, stride);
                    while(--passes){
                        memset(tmp, 0, stride * 2);
                        engine->be_blur(buf, w, h, stride, tmp);
                    }
                    be_blur_post(buf, w, h, stride);
                }
                memset(tmp, 0, stride * 2);
                engine->be_blur(buf, w, h, stride, tmp);
            }
        }
        if (blur_g) {
            unsigned passes = be;
            unsigned w = bm_g->w;
            unsigned h = bm_g->h;
            unsigned stride = bm_g->stride;
            unsigned char *buf = bm_g->buffer;
            if(w && h){
                if(passes > 1){
                    be_blur_pre(buf, w, h, stride);
                    while(--passes){
                        memset(tmp, 0, stride * 2);
                        engine->be_blur(buf, w, h, stride, tmp);
                    }
                    be_blur_post(buf, w, h, stride);
                }
                memset(tmp, 0, stride * 2);
                engine->be_blur(buf, w, h, stride, tmp);
            }
        }
        ass_aligned_free(tmp);
    }
}

static bool alloc_bitmap_buffer(const BitmapEngine *engine, Bitmap *bm, int w, int h,
                                bool zero)
{
    unsigned align = 1 << engine->align_order;
    size_t s = ass_align(align, w);
    // Too often we use ints as offset for bitmaps => use INT_MAX.
    if (s > (INT_MAX - 32) / FFMAX(h, 1))
        return false;
    uint8_t *buf = ass_aligned_alloc(align, s * h + 32, zero);
    if (!buf)
        return false;
    bm->w = w;
    bm->h = h;
    bm->stride = s;
    bm->buffer = buf;
    return true;
}

Bitmap *alloc_bitmap(const BitmapEngine *engine, int w, int h, bool zero)
{
    Bitmap *bm = malloc(sizeof(Bitmap));
    if (!bm)
        return NULL;
    if (!alloc_bitmap_buffer(engine, bm, w, h, zero)) {
        free(bm);
        return NULL;
    }
    bm->left = bm->top = 0;
    return bm;
}

bool realloc_bitmap(const BitmapEngine *engine, Bitmap *bm, int w, int h)
{
    uint8_t *old = bm->buffer;
    if (!alloc_bitmap_buffer(engine, bm, w, h, false))
        return false;
    ass_aligned_free(old);
    return true;
}

void ass_free_bitmap(Bitmap *bm)
{
    if (bm)
        ass_aligned_free(bm->buffer);
    free(bm);
}

Bitmap *copy_bitmap(const BitmapEngine *engine, const Bitmap *src)
{
    Bitmap *dst = alloc_bitmap(engine, src->w, src->h, false);
    if (!dst)
        return NULL;
    dst->left = src->left;
    dst->top = src->top;
    memcpy(dst->buffer, src->buffer, src->stride * src->h);
    return dst;
}

Bitmap *outline_to_bitmap(ASS_Renderer *render_priv,
                          ASS_Outline *outline1, ASS_Outline *outline2,
                          int bord)
{
    RasterizerData *rst = &render_priv->rasterizer;
    if (outline1 && !rasterizer_set_outline(rst, outline1, false)) {
        ass_msg(render_priv->library, MSGL_WARN, "Failed to process glyph outline!\n");
        return NULL;
    }
    if (outline2 && !rasterizer_set_outline(rst, outline2, !!outline1)) {
        ass_msg(render_priv->library, MSGL_WARN, "Failed to process glyph outline!\n");
        return NULL;
    }

    if (bord < 0 || bord > INT_MAX / 2)
        return NULL;
    if (rst->bbox.x_max > INT_MAX - 63 || rst->bbox.y_max > INT_MAX - 63)
        return NULL;

    int x_min = rst->bbox.x_min >> 6;
    int y_min = rst->bbox.y_min >> 6;
    int x_max = (rst->bbox.x_max + 63) >> 6;
    int y_max = (rst->bbox.y_max + 63) >> 6;
    int w = x_max - x_min;
    int h = y_max - y_min;

    int mask = (1 << render_priv->engine->tile_order) - 1;

    if (w < 0 || h < 0 ||
        w > INT_MAX - (2 * bord + mask) || h > INT_MAX - (2 * bord + mask)) {
        ass_msg(render_priv->library, MSGL_WARN, "Glyph bounding box too large: %dx%dpx",
                w, h);
        return NULL;
    }

    int tile_w = (w + 2 * bord + mask) & ~mask;
    int tile_h = (h + 2 * bord + mask) & ~mask;
    Bitmap *bm = alloc_bitmap(render_priv->engine, tile_w, tile_h, false);
    if (!bm)
        return NULL;
    bm->left = x_min - bord;
    bm->top =  y_min - bord;

    if (!rasterizer_fill(render_priv->engine, rst, bm->buffer,
                         x_min - bord, y_min - bord,
                         bm->stride, tile_h, bm->stride)) {
        ass_msg(render_priv->library, MSGL_WARN, "Failed to rasterize glyph!\n");
        ass_free_bitmap(bm);
        return NULL;
    }

    return bm;
}

/**
 * \brief fix outline bitmap
 *
 * The glyph bitmap is subtracted from outline bitmap. This way looks much
 * better in some cases.
 */
void fix_outline(Bitmap *bm_g, Bitmap *bm_o)
{
    int x, y;
    const int l = bm_o->left > bm_g->left ? bm_o->left : bm_g->left;
    const int t = bm_o->top > bm_g->top ? bm_o->top : bm_g->top;
    const int r =
        bm_o->left + bm_o->stride <
        bm_g->left + bm_g->stride ? bm_o->left + bm_o->stride : bm_g->left + bm_g->stride;
    const int b =
        bm_o->top + bm_o->h <
        bm_g->top + bm_g->h ? bm_o->top + bm_o->h : bm_g->top + bm_g->h;

    unsigned char *g =
        bm_g->buffer + (t - bm_g->top) * bm_g->stride + (l - bm_g->left);
    unsigned char *o =
        bm_o->buffer + (t - bm_o->top) * bm_o->stride + (l - bm_o->left);

    for (y = 0; y < b - t; ++y) {
        for (x = 0; x < r - l; ++x) {
            unsigned char c_g, c_o;
            c_g = g[x];
            c_o = o[x];
            o[x] = (c_o > c_g) ? c_o - (c_g / 2) : 0;
        }
        g += bm_g->stride;
        o += bm_o->stride;
    }
}

/**
 * \brief Shift a bitmap by the fraction of a pixel in x and y direction
 * expressed in 26.6 fixed point
 */
void shift_bitmap(Bitmap *bm, int shift_x, int shift_y)
{
    int x, y, b;
    int w = bm->w;
    int h = bm->h;
    int s = bm->stride;
    unsigned char *buf = bm->buffer;

    assert((shift_x & ~63) == 0 && (shift_y & ~63) == 0);

    // Shift in x direction
    for (y = 0; y < h; y++) {
        for (x = w - 1; x > 0; x--) {
            b = (buf[x + y * s - 1] * shift_x) >> 6;
            buf[x + y * s - 1] -= b;
            buf[x + y * s] += b;
        }
    }

    // Shift in y direction
    for (x = 0; x < w; x++) {
        for (y = h - 1; y > 0; y--) {
            b = (buf[x + (y - 1) * s] * shift_y) >> 6;
            buf[x + (y - 1) * s] -= b;
            buf[x + y * s] += b;
        }
    }
}

/**
 * \brief Blur with [[1,2,1], [2,4,2], [1,2,1]] kernel
 * This blur is the same as the one employed by vsfilter.
 * Pure C implementation.
 */
void ass_be_blur_c(uint8_t *buf, intptr_t w, intptr_t h,
                   intptr_t stride, uint16_t *tmp)
{
    uint16_t *col_pix_buf = tmp;
    uint16_t *col_sum_buf = tmp + w;
    unsigned x, y, old_pix, old_sum, temp1, temp2;
    uint8_t *src, *dst;
    memset(tmp, 0, sizeof(uint16_t) * w * 2);
    y = 0;

    {
        src=buf+y*stride;

        x = 1;
        old_pix = src[x-1];
        old_sum = old_pix;
        for ( ; x < w; x++) {
            temp1 = src[x];
            temp2 = old_pix + temp1;
            old_pix = temp1;
            temp1 = old_sum + temp2;
            old_sum = temp2;
            col_pix_buf[x-1] = temp1;
            col_sum_buf[x-1] = temp1;
        }
        temp1 = old_sum + old_pix;
        col_pix_buf[x-1] = temp1;
        col_sum_buf[x-1] = temp1;
    }

    for (y++; y < h; y++) {
        src=buf+y*stride;
        dst=buf+(y-1)*stride;

        x = 1;
        old_pix = src[x-1];
        old_sum = old_pix;
        for ( ; x < w; x++) {
            temp1 = src[x];
            temp2 = old_pix + temp1;
            old_pix = temp1;
            temp1 = old_sum + temp2;
            old_sum = temp2;

            temp2 = col_pix_buf[x-1] + temp1;
            col_pix_buf[x-1] = temp1;
            dst[x-1] = (col_sum_buf[x-1] + temp2) >> 4;
            col_sum_buf[x-1] = temp2;
        }
        temp1 = old_sum + old_pix;
        temp2 = col_pix_buf[x-1] + temp1;
        col_pix_buf[x-1] = temp1;
        dst[x-1] = (col_sum_buf[x-1] + temp2) >> 4;
        col_sum_buf[x-1] = temp2;
    }

    {
        dst=buf+(y-1)*stride;
        for (x = 0; x < w; x++)
            dst[x] = (col_sum_buf[x] + col_pix_buf[x]) >> 4;
    }
}

void be_blur_pre(uint8_t *buf, intptr_t w, intptr_t h, intptr_t stride)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            // This is equivalent to (value * 64 + 127) / 255 for all
            // values from 0 to 256 inclusive. Assist vectorizing
            // compilers by noting that all temporaries fit in 8 bits.
            buf[y * stride + x] =
                (uint8_t) ((buf[y * stride + x] >> 1) + 1) >> 1;
        }
    }
}

void be_blur_post(uint8_t *buf, intptr_t w, intptr_t h, intptr_t stride)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            // This is equivalent to (value * 255 + 32) / 64 for all values
            // from 0 to 96 inclusive, and we only care about 0 to 64.
            uint8_t value = buf[y * stride + x];
            buf[y * stride + x] = (value << 2) - (value > 32);
        }
    }
}

/*
 * To find these values, simulate blur on the border between two
 * half-planes, one zero-filled (background) and the other filled
 * with the maximum supported value (foreground). Keep incrementing
 * the \be argument. The necessary padding is the distance by which
 * the blurred foreground image extends beyond the original border
 * and into the background. Initially it increases along with \be,
 * but very soon it grinds to a halt. At some point, the blurred
 * image actually reaches a stationary point and stays unchanged
 * forever after, simply _shifting_ by one pixel for each \be
 * step--moving in the direction of the non-zero half-plane and
 * thus decreasing the necessary padding (although the large
 * padding is still needed for intermediate results). In practice,
 * images are finite rather than infinite like half-planes, but
 * this can only decrease the required padding. Half-planes filled
 * with extreme values are the theoretical limit of the worst case.
 * Make sure to use the right pixel value range in the simulation!
 */
int be_padding(int be)
{
    if (be <= 3)
        return be;
    if (be <= 7)
        return 4;
    if (be <= 123)
        return 5;
    return FFMAX(128 - be, 0);
}

bool outline_to_bitmap2(ASS_Renderer *render_priv, ASS_Outline *outline,
                        ASS_Outline *border1, ASS_Outline *border2,
                        Bitmap **bm_g, Bitmap **bm_o)
{
    assert(bm_g && bm_o);
    *bm_g = *bm_o = NULL;

    if (outline && !outline->n_points)
        outline = NULL;
    if (border1 && !border1->n_points)
        border1 = NULL;
    if (border2 && !border2->n_points)
        border2 = NULL;

    if (outline) {
        *bm_g = outline_to_bitmap(render_priv, outline, NULL, 1);
        if (!*bm_g)
            return false;
    }

    if (border1 || border2) {
        *bm_o = outline_to_bitmap(render_priv, border1, border2, 1);
        if (!*bm_o) {
            return false;
        }
    }

    return true;
}

/**
 * \brief Add two bitmaps together at a given position
 * Uses additive blending, clipped to [0,255]. Pure C implementation.
 */
void ass_add_bitmaps_c(uint8_t *dst, intptr_t dst_stride,
                       uint8_t *src, intptr_t src_stride,
                       intptr_t height, intptr_t width)
{
    unsigned out;
    uint8_t* end = dst + dst_stride * height;
    while (dst < end) {
        for (unsigned j = 0; j < width; ++j) {
            out = dst[j] + src[j];
            dst[j] = FFMIN(out, 255);
        }
        dst += dst_stride;
        src += src_stride;
    }
}

void ass_sub_bitmaps_c(uint8_t *dst, intptr_t dst_stride,
                       uint8_t *src, intptr_t src_stride,
                       intptr_t height, intptr_t width)
{
    short out;
    uint8_t* end = dst + dst_stride * height;
    while (dst < end) {
        for (unsigned j = 0; j < width; ++j) {
            out = dst[j] - src[j];
            dst[j] = FFMAX(out, 0);
        }
        dst += dst_stride;
        src += src_stride;
    }
}

void ass_mul_bitmaps_c(uint8_t *dst, intptr_t dst_stride,
                       uint8_t *src1, intptr_t src1_stride,
                       uint8_t *src2, intptr_t src2_stride,
                       intptr_t w, intptr_t h)
{
    uint8_t* end = src1 + src1_stride * h;
    while (src1 < end) {
        for (unsigned x = 0; x < w; ++x) {
            dst[x] = (src1[x] * src2[x] + 255) >> 8;
        }
        dst  += dst_stride;
        src1 += src1_stride;
        src2 += src2_stride;
    }
}
