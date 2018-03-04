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

#ifndef LIBASS_RASTERIZER_H
#define LIBASS_RASTERIZER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ass_bitmap.h"


enum {
    SEGFLAG_DN           =  1,
    SEGFLAG_UL_DR        =  2,
    SEGFLAG_EXACT_LEFT   =  4,
    SEGFLAG_EXACT_RIGHT  =  8,
    SEGFLAG_EXACT_TOP    = 16,
    SEGFLAG_EXACT_BOTTOM = 32,
};

// Polyline segment struct
struct segment {
    int64_t c;
    int32_t a, b, scale, flags;
    int32_t x_min, x_max, y_min, y_max;
};

typedef struct {
    int outline_error;  // acceptable error (in 1/64 pixel units)

    // usable after rasterizer_set_outline
    ASS_Rect bbox;

    // internal buffers
    struct segment *linebuf[2];
    size_t size[2], capacity[2];
    size_t n_first;

    uint8_t *tile;
} RasterizerData;

bool rasterizer_init(RasterizerData *rst, int tile_order, int outline_error);
void rasterizer_done(RasterizerData *rst);

/**
 * \brief Convert outline to polyline and calculate exact bounds
 * \param path in: source outline
 * \param extra in: true if path is second border outline
 * \return false on error
 */
bool rasterizer_set_outline(RasterizerData *rst,
                            const ASS_Outline *path, bool extra);

/**
 * \brief Polyline rasterization function
 * \param x0, y0, width, height in: source window (full pixel units)
 * \param buf out: aligned output buffer (size = stride * height)
 * \param stride output buffer stride (aligned)
 * \return false on error
 * Deletes preprocessed polyline after work.
 */
bool rasterizer_fill(const BitmapEngine *engine, RasterizerData *rst,
                     uint8_t *buf, int x0, int y0,
                     int width, int height, ptrdiff_t stride);


#endif /* LIBASS_RASTERIZER_H */
