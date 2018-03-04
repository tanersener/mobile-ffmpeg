/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 * Copyright (C) 2011 Grigori Goronzy <greg@chown.ath.cx>
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

#ifndef LIBASS_CACHE_H
#define LIBASS_CACHE_H

#include "ass.h"
#include "ass_font.h"
#include "ass_outline.h"
#include "ass_bitmap.h"

typedef struct cache Cache;

// cache values

typedef struct {
    bool valid;
    Bitmap *bm;               // the actual bitmaps
    Bitmap *bm_o;
} BitmapHashValue;

typedef struct {
    Bitmap *bm;
    Bitmap *bm_o;
    Bitmap *bm_s;
} CompositeHashValue;

typedef struct {
    bool valid;
    ASS_Outline outline;
    ASS_Outline border[2];
    ASS_Rect bbox_scaled;       // bbox after scaling, but before rotation
    ASS_Vector advance;         // 26.6, advance distance to the next outline in line
    int asc, desc;              // ascender/descender
} OutlineHashValue;

typedef struct {
    FT_Glyph_Metrics metrics;
} GlyphMetricsHashValue;

// Create definitions for bitmap, outline and composite hash keys
#define CREATE_STRUCT_DEFINITIONS
#include "ass_cache_template.h"

// Type-specific function pointers
typedef unsigned(*HashFunction)(void *key, size_t key_size);
typedef unsigned(*HashCompare)(void *a, void *b, size_t key_size);
typedef bool(*CacheKeyMove)(void *dst, void *src, size_t key_size);
typedef void(*CacheItemDestructor)(void *key, void *value);

// cache hash keys

typedef struct outline_hash_key {
    enum {
        OUTLINE_GLYPH,
        OUTLINE_DRAWING,
    } type;
    union {
        GlyphHashKey glyph;
        DrawingHashKey drawing;
    } u;
} OutlineHashKey;

typedef struct bitmap_hash_key {
    enum {
        BITMAP_OUTLINE,
        BITMAP_CLIP,
    } type;
    union {
        OutlineBitmapHashKey outline;
        ClipMaskHashKey clip;
    } u;
} BitmapHashKey;

typedef struct {
    BitmapHashValue *image;
    int x, y;
} BitmapRef;

enum {
    FILTER_BORDER_STYLE_3 = 1,
    FILTER_NONZERO_BORDER = 2,
    FILTER_NONZERO_SHADOW = 4,
    FILTER_DRAW_SHADOW    = 8,  // VSFilter compatibility
};

typedef struct {
    FilterDesc filter;
    size_t bitmap_count;
    BitmapRef *bitmaps;
} CompositeHashKey;

typedef struct
{
    HashFunction hash_func;
    HashCompare compare_func;
    CacheKeyMove key_move_func;
    CacheItemDestructor destruct_func;
    size_t key_size;
    size_t value_size;
} CacheDesc;

Cache *ass_cache_create(const CacheDesc *desc);
bool ass_cache_get(Cache *cache, void *key, void *value_ptr);
void *ass_cache_key(void *value);
void ass_cache_commit(void *value, size_t item_size);
void ass_cache_inc_ref(void *value);
void ass_cache_dec_ref(void *value);
void ass_cache_cut(Cache *cache, size_t max_size);
void ass_cache_stats(Cache *cache, size_t *size, unsigned *hits,
                     unsigned *misses, unsigned *count);
void ass_cache_empty(Cache *cache);
void ass_cache_done(Cache *cache);
Cache *ass_font_cache_create(void);
Cache *ass_outline_cache_create(void);
Cache *ass_glyph_metrics_cache_create(void);
Cache *ass_bitmap_cache_create(void);
Cache *ass_composite_cache_create(void);

#endif                          /* LIBASS_CACHE_H */
