/*
 * Copyright (C) 2009 Grigori Goronzy <greg@geekmind.org>
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

#include <ft2build.h>
#include FT_OUTLINE_H
#include <math.h>
#include <stdbool.h>
#include <limits.h>

#include "ass_utils.h"
#include "ass_drawing.h"
#include "ass_font.h"

#define GLYPH_INITIAL_POINTS 100
#define GLYPH_INITIAL_SEGMENTS 100

/*
 * \brief Prepare drawing for parsing.  This just sets a few parameters.
 */
static void drawing_prepare(ASS_Drawing *drawing)
{
    // Scaling parameters
    drawing->point_scale_x = drawing->scale_x / (1 << (drawing->scale - 1));
    drawing->point_scale_y = drawing->scale_y / (1 << (drawing->scale - 1));
}

/*
 * \brief Finish a drawing.  This only sets the horizontal advance according
 * to the outline's bbox at the moment.
 */
static void drawing_finish(ASS_Drawing *drawing, bool raw_mode)
{
    ASS_Rect bbox = drawing->cbox;
    ASS_Outline *ol = &drawing->outline;

    if (drawing->library)
        ass_msg(drawing->library, MSGL_V,
                "Parsed drawing with %d points and %d segments",
                ol->n_points, ol->n_segments);

    if (raw_mode)
        return;

    drawing->advance.x = bbox.x_max - bbox.x_min;

    double pbo = drawing->pbo / (1 << (drawing->scale - 1));
    drawing->desc = double_to_d6(pbo * drawing->scale_y);
    drawing->asc = bbox.y_max - bbox.y_min - drawing->desc;

    // Place it onto the baseline
    for (size_t i = 0; i < ol->n_points; i++)
        ol->points[i].y -= drawing->asc;
}

/*
 * \brief Check whether a number of items on the list is available
 */
static int token_check_values(ASS_DrawingToken *token, int i, int type)
{
    for (int j = 0; j < i; j++) {
        if (!token || token->type != type) return 0;
        token = token->next;
    }

    return 1;
}

/*
 * \brief Tokenize a drawing string into a list of ASS_DrawingToken
 * This also expands points for closing b-splines
 */
static ASS_DrawingToken *drawing_tokenize(char *str)
{
    char *p = str;
    int type = -1, is_set = 0;
    double val;
    ASS_Vector point = {0, 0};

    ASS_DrawingToken *root = NULL, *tail = NULL, *spline_start = NULL;

    while (p && *p) {
        int got_coord = 0;
        if (*p == 'c' && spline_start) {
            // Close b-splines: add the first three points of the b-spline
            // back to the end
            if (token_check_values(spline_start->next, 2, TOKEN_B_SPLINE)) {
                for (int i = 0; i < 3; i++) {
                    tail->next = calloc(1, sizeof(ASS_DrawingToken));
                    tail->next->prev = tail;
                    tail = tail->next;
                    tail->type = TOKEN_B_SPLINE;
                    tail->point = spline_start->point;
                    spline_start = spline_start->next;
                }
                spline_start = NULL;
            }
        } else if (!is_set && mystrtod(&p, &val)) {
            point.x = double_to_d6(val);
            is_set = 1;
            got_coord = 1;
            p--;
        } else if (is_set == 1 && mystrtod(&p, &val)) {
            point.y = double_to_d6(val);
            is_set = 2;
            got_coord = 1;
            p--;
        } else if (*p == 'm')
            type = TOKEN_MOVE;
        else if (*p == 'n')
            type = TOKEN_MOVE_NC;
        else if (*p == 'l')
            type = TOKEN_LINE;
        else if (*p == 'b')
            type = TOKEN_CUBIC_BEZIER;
        else if (*p == 'q')
            type = TOKEN_CONIC_BEZIER;
        else if (*p == 's')
            type = TOKEN_B_SPLINE;
        // We're simply ignoring TOKEN_EXTEND_B_SPLINE here.
        // This is not harmful at all, since it can be ommitted with
        // similar result (the spline is extended anyway).

        // Ignore the odd extra value, it makes no sense.
        if (!got_coord)
            is_set = 0;

        if (type != -1 && is_set == 2) {
            if (root) {
                tail->next = calloc(1, sizeof(ASS_DrawingToken));
                tail->next->prev = tail;
                tail = tail->next;
            } else
                root = tail = calloc(1, sizeof(ASS_DrawingToken));
            tail->type = type;
            tail->point = point;
            is_set = 0;
            if (type == TOKEN_B_SPLINE && !spline_start)
                spline_start = tail->prev;
        }
        p++;
    }

    return root;
}

/*
 * \brief Free a list of tokens
 */
static void drawing_free_tokens(ASS_DrawingToken *token)
{
    while (token) {
        ASS_DrawingToken *at = token;
        token = token->next;
        free(at);
    }
}

/*
 * \brief Translate and scale a point coordinate according to baseline
 * offset and scale.
 */
static inline void translate_point(ASS_Drawing *drawing, ASS_Vector *point)
{
    point->x = lrint(drawing->point_scale_x * point->x);
    point->y = lrint(drawing->point_scale_y * point->y);

    rectangle_update(&drawing->cbox, point->x, point->y, point->x, point->y);
}

/*
 * \brief Add curve to drawing
 */
static bool drawing_add_curve(ASS_Drawing *drawing, ASS_DrawingToken *token,
                              bool spline, int started)
{
    ASS_Vector p[4];
    for (int i = 0; i < 4; ++i) {
        p[i] = token->point;
        translate_point(drawing, &p[i]);
        token = token->next;
    }

    if (spline) {
        int x01 = (p[1].x - p[0].x) / 3;
        int y01 = (p[1].y - p[0].y) / 3;
        int x12 = (p[2].x - p[1].x) / 3;
        int y12 = (p[2].y - p[1].y) / 3;
        int x23 = (p[3].x - p[2].x) / 3;
        int y23 = (p[3].y - p[2].y) / 3;

        p[0].x = p[1].x + ((x12 - x01) >> 1);
        p[0].y = p[1].y + ((y12 - y01) >> 1);
        p[3].x = p[2].x + ((x23 - x12) >> 1);
        p[3].y = p[2].y + ((y23 - y12) >> 1);
        p[1].x += x12;
        p[1].y += y12;
        p[2].x -= x12;
        p[2].y -= y12;
    }

    return (started ||
        outline_add_point(&drawing->outline, p[0], 0)) &&
        outline_add_point(&drawing->outline, p[1], 0) &&
        outline_add_point(&drawing->outline, p[2], 0) &&
        outline_add_point(&drawing->outline, p[3], OUTLINE_CUBIC_SPLINE);
}

/*
 * \brief Create and initialize a new drawing and return it
 */
ASS_Drawing *ass_drawing_new(ASS_Library *lib)
{
    ASS_Drawing *drawing = calloc(1, sizeof(*drawing));
    if (!drawing)
        return NULL;
    rectangle_reset(&drawing->cbox);
    drawing->library = lib;
    drawing->scale_x = 1.;
    drawing->scale_y = 1.;

    if (!outline_alloc(&drawing->outline, GLYPH_INITIAL_POINTS, GLYPH_INITIAL_SEGMENTS)) {
        free(drawing);
        return NULL;
    }
    return drawing;
}

/*
 * \brief Free a drawing
 */
void ass_drawing_free(ASS_Drawing *drawing)
{
    if (drawing) {
        free(drawing->text);
        outline_free(&drawing->outline);
    }
    free(drawing);
}

/*
 * \brief Copy an ASCII string to the drawing text buffer
 */
void ass_drawing_set_text(ASS_Drawing *drawing, char *str, size_t len)
{
    free(drawing->text);
    drawing->text = strndup(str, len);
}

/*
 * \brief Create a hashcode for the drawing
 * XXX: To avoid collisions a better hash algorithm might be useful.
 */
void ass_drawing_hash(ASS_Drawing *drawing)
{
    if (!drawing->text)
        return;
    drawing->hash = fnv_32a_str(drawing->text, FNV1_32A_INIT);
}

/*
 * \brief Convert token list to outline.  Calls the line and curve evaluators.
 */
ASS_Outline *ass_drawing_parse(ASS_Drawing *drawing, bool raw_mode)
{
    bool started = false;
    ASS_DrawingToken *token;
    ASS_Vector pen = {0, 0};

    drawing->tokens = drawing_tokenize(drawing->text);
    drawing_prepare(drawing);

    token = drawing->tokens;
    while (token) {
        // Draw something according to current command
        switch (token->type) {
        case TOKEN_MOVE_NC:
            pen = token->point;
            translate_point(drawing, &pen);
            token = token->next;
            break;
        case TOKEN_MOVE:
            pen = token->point;
            translate_point(drawing, &pen);
            if (started) {
                if (!outline_add_segment(&drawing->outline, OUTLINE_LINE_SEGMENT))
                    goto error;
                if (!outline_close_contour(&drawing->outline))
                    goto error;
                started = false;
            }
            token = token->next;
            break;
        case TOKEN_LINE: {
            ASS_Vector to = token->point;
            translate_point(drawing, &to);
            if (!started && !outline_add_point(&drawing->outline, pen, 0))
                goto error;
            if (!outline_add_point(&drawing->outline, to, OUTLINE_LINE_SEGMENT))
                goto error;
            started = true;
            token = token->next;
            break;
        }
        case TOKEN_CUBIC_BEZIER:
            if (token_check_values(token, 3, TOKEN_CUBIC_BEZIER) &&
                token->prev) {
                if (!drawing_add_curve(drawing, token->prev, false, started))
                    goto error;
                token = token->next;
                token = token->next;
                token = token->next;
                started = true;
            } else
                token = token->next;
            break;
        case TOKEN_B_SPLINE:
            if (token_check_values(token, 3, TOKEN_B_SPLINE) &&
                token->prev) {
                if (!drawing_add_curve(drawing, token->prev, true, started))
                    goto error;
                token = token->next;
                started = true;
            } else
                token = token->next;
            break;
        default:
            token = token->next;
            break;
        }
    }

    // Close the last contour
    if (started) {
        if (!outline_add_segment(&drawing->outline, OUTLINE_LINE_SEGMENT))
            goto error;
        if (!outline_close_contour(&drawing->outline))
            goto error;
    }

    drawing_finish(drawing, raw_mode);
    drawing_free_tokens(drawing->tokens);
    return &drawing->outline;

error:
    drawing_free_tokens(drawing->tokens);
    return NULL;
}
