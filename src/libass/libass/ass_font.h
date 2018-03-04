/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
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

#ifndef LIBASS_FONT_H
#define LIBASS_FONT_H

#include <stdint.h>
#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H

typedef struct ass_font ASS_Font;
typedef struct ass_font_desc ASS_FontDesc;

#include "ass.h"
#include "ass_types.h"
#include "ass_fontselect.h"
#include "ass_cache.h"

#define VERTICAL_LOWER_BOUND 0x02f1

#define ASS_FONT_MAX_FACES 10
#define DECO_UNDERLINE 1
#define DECO_STRIKETHROUGH 2

struct ass_font_desc {
    char *family;
    unsigned bold;
    unsigned italic;
    int vertical;               // @font vertical layout
};

struct ass_font {
    ASS_FontDesc desc;
    ASS_Library *library;
    FT_Library ftlibrary;
    int faces_uid[ASS_FONT_MAX_FACES];
    FT_Face faces[ASS_FONT_MAX_FACES];
    ASS_ShaperFontData *shaper_priv;
    int n_faces;
    double scale_x, scale_y;    // current transform
    FT_Vector v;                // current shift
    double size;
};

void charmap_magic(ASS_Library *library, FT_Face face);
ASS_Font *ass_font_new(Cache *font_cache, ASS_Library *library,
                       FT_Library ftlibrary, ASS_FontSelector *fontsel,
                       ASS_FontDesc *desc);
void ass_font_set_transform(ASS_Font *font, double scale_x,
                            double scale_y, FT_Vector *v);
void ass_face_set_size(FT_Face face, double size);
void ass_font_set_size(ASS_Font *font, double size);
void ass_font_get_asc_desc(ASS_Font *font, uint32_t ch, int *asc,
                           int *desc);
int ass_font_get_index(ASS_FontSelector *fontsel, ASS_Font *font,
                       uint32_t symbol, int *face_index, int *glyph_index);
uint32_t ass_font_index_magic(FT_Face face, uint32_t symbol);
FT_Glyph ass_font_get_glyph(ASS_Font *font,
                            uint32_t ch, int face_index, int index,
                            ASS_Hinting hinting, int deco);
void ass_font_clear(ASS_Font *font);

#endif                          /* LIBASS_FONT_H */
