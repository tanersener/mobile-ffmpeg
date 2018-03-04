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

#ifndef LIBASS_DRAWING_H
#define LIBASS_DRAWING_H

#include "ass.h"
#include "ass_outline.h"
#include "ass_bitmap.h"

typedef enum {
    TOKEN_MOVE,
    TOKEN_MOVE_NC,
    TOKEN_LINE,
    TOKEN_CUBIC_BEZIER,
    TOKEN_CONIC_BEZIER,
    TOKEN_B_SPLINE,
    TOKEN_EXTEND_SPLINE,
    TOKEN_CLOSE
} ASS_TokenType;

typedef struct ass_drawing_token {
    ASS_TokenType type;
    ASS_Vector point;
    struct ass_drawing_token *next;
    struct ass_drawing_token *prev;
} ASS_DrawingToken;

typedef struct {
    char *text; // drawing string
    int scale;  // scale (1-64) for subpixel accuracy
    double pbo; // drawing will be shifted in y direction by this amount
    double scale_x;      // FontScaleX
    double scale_y;      // FontScaleY
    int asc;             // ascender
    int desc;            // descender
    ASS_Outline outline; // target outline
    ASS_Vector advance;  // advance (from cbox)
    int hash;            // hash value (for caching)

    // private
    ASS_Library *library;
    ASS_DrawingToken *tokens;    // tokenized drawing
    double point_scale_x;
    double point_scale_y;
    ASS_Rect cbox;   // bounding box, or let's say... VSFilter's idea of it
} ASS_Drawing;

ASS_Drawing *ass_drawing_new(ASS_Library *lib);
void ass_drawing_free(ASS_Drawing *drawing);
void ass_drawing_set_text(ASS_Drawing *drawing, char *str, size_t n);
void ass_drawing_hash(ASS_Drawing *drawing);
ASS_Outline *ass_drawing_parse(ASS_Drawing *drawing, bool raw_mode);

#endif /* LIBASS_DRAWING_H */
