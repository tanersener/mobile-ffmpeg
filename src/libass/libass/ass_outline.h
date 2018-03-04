/*
 * Copyright (C) 2016 Vabishchevich Nikolay <vabnick@gmail.com>
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

#ifndef LIBASS_OUTLINE_H
#define LIBASS_OUTLINE_H

#include <ft2build.h>
#include FT_OUTLINE_H
#include <stdbool.h>
#include <stdint.h>

#include "ass_utils.h"


typedef struct {
    int32_t x, y;
} ASS_Vector;

typedef struct {
    double x, y;
} ASS_DVector;

typedef struct {
    int32_t x_min, y_min, x_max, y_max;
} ASS_Rect;

typedef struct {
    double x_min, y_min, x_max, y_max;
} ASS_DRect;

static inline void rectangle_reset(ASS_Rect *rect)
{
    rect->x_min = rect->y_min = INT32_MAX;
    rect->x_max = rect->y_max = INT32_MIN;
}

static inline void rectangle_update(ASS_Rect *rect,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max)
{
    rect->x_min = FFMIN(rect->x_min, x_min);
    rect->y_min = FFMIN(rect->y_min, y_min);
    rect->x_max = FFMAX(rect->x_max, x_max);
    rect->y_max = FFMAX(rect->y_max, y_max);
}

/*
 * Outline represented with array of points and array of segments.
 * Segment here is spline of order 1 (line), 2 (quadratic) or 3 (cubic).
 * Each segment owns number of points equal to its order in point array
 * and uses first point owned by the next segment as last point.
 * Last segment in each contour instead of the next segment point uses
 * point owned by the first segment in that contour. Correspondingly
 * total number of points is equal to the sum of spline orders of all segments.
 */

enum {
    OUTLINE_LINE_SEGMENT     = 1,  // line segment
    OUTLINE_QUADRATIC_SPLINE = 2,  // quadratic spline
    OUTLINE_CUBIC_SPLINE     = 3,  // cubic spline
    OUTLINE_COUNT_MASK       = 3,  // spline order mask
    OUTLINE_CONTOUR_END      = 4   // last segment in contour flag
};

typedef struct {
    size_t n_points, max_points;
    size_t n_segments, max_segments;
    ASS_Vector *points;
    char *segments;
} ASS_Outline;

#define OUTLINE_MIN  (-((int32_t) 1 << 28))
#define OUTLINE_MAX  (((int32_t) 1 << 28) - 1)

bool outline_alloc(ASS_Outline *outline, size_t n_points, size_t n_segments);
bool outline_convert(ASS_Outline *outline, const FT_Outline *source);
bool outline_copy(ASS_Outline *outline, const ASS_Outline *source);
void outline_free(ASS_Outline *outline);

bool outline_add_point(ASS_Outline *outline, ASS_Vector pt, char segment);
bool outline_add_segment(ASS_Outline *outline, char segment);
bool outline_close_contour(ASS_Outline *outline);

void outline_translate(const ASS_Outline *outline, int32_t dx, int32_t dy);
void outline_adjust(const ASS_Outline *outline, double scale_x, int32_t dx, int32_t dy);
void outline_get_cbox(const ASS_Outline *outline, ASS_Rect *cbox);

bool outline_stroke(ASS_Outline *result, ASS_Outline *result1,
                    const ASS_Outline *path, int xbord, int ybord, int eps);


#endif /* LIBASS_OUTLINE_H */
