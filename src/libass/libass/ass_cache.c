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

#include "config.h"
#include "ass_compat.h"

#include <inttypes.h>
#include <ft2build.h>
#include FT_OUTLINE_H
#include <assert.h>

#include "ass_utils.h"
#include "ass_font.h"
#include "ass_outline.h"
#include "ass_cache.h"

// type-specific functions
// create hash/compare functions for bitmap, outline and composite cache
#define CREATE_HASH_FUNCTIONS
#include "ass_cache_template.h"
#define CREATE_COMPARISON_FUNCTIONS
#include "ass_cache_template.h"

// font cache
static unsigned font_hash(void *buf, size_t len)
{
    ASS_FontDesc *desc = buf;
    unsigned hval;
    hval = fnv_32a_str(desc->family, FNV1_32A_INIT);
    hval = fnv_32a_buf(&desc->bold, sizeof(desc->bold), hval);
    hval = fnv_32a_buf(&desc->italic, sizeof(desc->italic), hval);
    hval = fnv_32a_buf(&desc->vertical, sizeof(desc->vertical), hval);
    return hval;
}

static unsigned font_compare(void *key1, void *key2, size_t key_size)
{
    ASS_FontDesc *a = key1;
    ASS_FontDesc *b = key2;
    if (strcmp(a->family, b->family) != 0)
        return 0;
    if (a->bold != b->bold)
        return 0;
    if (a->italic != b->italic)
        return 0;
    if (a->vertical != b->vertical)
        return 0;
    return 1;
}

static bool font_key_move(void *dst, void *src, size_t key_size)
{
    ASS_FontDesc *k = src;
    if (dst)
        memcpy(dst, src, key_size);
    else
        free(k->family);
    return true;
}

static void font_destruct(void *key, void *value)
{
    ass_font_clear(value);
}

const CacheDesc font_cache_desc = {
    .hash_func = font_hash,
    .compare_func = font_compare,
    .key_move_func = font_key_move,
    .destruct_func = font_destruct,
    .key_size = sizeof(ASS_FontDesc),
    .value_size = sizeof(ASS_Font)
};


// bitmap cache
static unsigned bitmap_hash(void *key, size_t key_size)
{
    BitmapHashKey *k = key;
    switch (k->type) {
        case BITMAP_OUTLINE: return outline_bitmap_hash(&k->u, key_size);
        case BITMAP_CLIP: return clip_bitmap_hash(&k->u, key_size);
        default: return 0;
    }
}

static unsigned bitmap_compare(void *a, void *b, size_t key_size)
{
    BitmapHashKey *ak = a;
    BitmapHashKey *bk = b;
    if (ak->type != bk->type) return 0;
    switch (ak->type) {
        case BITMAP_OUTLINE: return outline_bitmap_compare(&ak->u, &bk->u, key_size);
        case BITMAP_CLIP: return clip_bitmap_compare(&ak->u, &bk->u, key_size);
        default: return 0;
    }
}

static bool bitmap_key_move(void *dst, void *src, size_t key_size)
{
    BitmapHashKey *d = dst, *s = src;
    if (!dst) {
        if (s->type == BITMAP_OUTLINE)
            ass_cache_dec_ref(s->u.outline.outline);
        return true;
    }
    memcpy(dst, src, key_size);
    if (s->type != BITMAP_CLIP)
        return true;
    d->u.clip.text = strdup(s->u.clip.text);
    return d->u.clip.text;
}

static void bitmap_destruct(void *key, void *value)
{
    BitmapHashValue *v = value;
    BitmapHashKey *k = key;
    if (v->bm)
        ass_free_bitmap(v->bm);
    if (v->bm_o)
        ass_free_bitmap(v->bm_o);
    switch (k->type) {
        case BITMAP_OUTLINE: ass_cache_dec_ref(k->u.outline.outline); break;
        case BITMAP_CLIP: free(k->u.clip.text); break;
    }
}

const CacheDesc bitmap_cache_desc = {
    .hash_func = bitmap_hash,
    .compare_func = bitmap_compare,
    .key_move_func = bitmap_key_move,
    .destruct_func = bitmap_destruct,
    .key_size = sizeof(BitmapHashKey),
    .value_size = sizeof(BitmapHashValue)
};


// composite cache
static unsigned composite_hash(void *key, size_t key_size)
{
    CompositeHashKey *k = key;
    unsigned hval = filter_hash(&k->filter, key_size);
    for (size_t i = 0; i < k->bitmap_count; i++) {
        hval = fnv_32a_buf(&k->bitmaps[i].image, sizeof(k->bitmaps[i].image), hval);
        hval = fnv_32a_buf(&k->bitmaps[i].x, sizeof(k->bitmaps[i].x), hval);
        hval = fnv_32a_buf(&k->bitmaps[i].y, sizeof(k->bitmaps[i].y), hval);
    }
    return hval;
}

static unsigned composite_compare(void *a, void *b, size_t key_size)
{
    CompositeHashKey *ak = a;
    CompositeHashKey *bk = b;
    if (ak->bitmap_count != bk->bitmap_count)
        return 0;
    for (size_t i = 0; i < ak->bitmap_count; i++) {
        if (ak->bitmaps[i].image != bk->bitmaps[i].image ||
            ak->bitmaps[i].x != bk->bitmaps[i].x ||
            ak->bitmaps[i].y != bk->bitmaps[i].y)
            return 0;
    }
    return filter_compare(&ak->filter, &bk->filter, key_size);
}

static bool composite_key_move(void *dst, void *src, size_t key_size)
{
    if (dst) {
        memcpy(dst, src, key_size);
        return true;
    }
    CompositeHashKey *k = src;
    for (size_t i = 0; i < k->bitmap_count; i++)
        ass_cache_dec_ref(k->bitmaps[i].image);
    free(k->bitmaps);
    return true;
}

static void composite_destruct(void *key, void *value)
{
    CompositeHashValue *v = value;
    CompositeHashKey *k = key;
    if (v->bm)
        ass_free_bitmap(v->bm);
    if (v->bm_o)
        ass_free_bitmap(v->bm_o);
    if (v->bm_s)
        ass_free_bitmap(v->bm_s);
    for (size_t i = 0; i < k->bitmap_count; i++)
        ass_cache_dec_ref(k->bitmaps[i].image);
    free(k->bitmaps);
}

const CacheDesc composite_cache_desc = {
    .hash_func = composite_hash,
    .compare_func = composite_compare,
    .key_move_func = composite_key_move,
    .destruct_func = composite_destruct,
    .key_size = sizeof(CompositeHashKey),
    .value_size = sizeof(CompositeHashValue)
};


// outline cache
static unsigned outline_hash(void *key, size_t key_size)
{
    OutlineHashKey *k = key;
    switch (k->type) {
        case OUTLINE_GLYPH: return glyph_hash(&k->u, key_size);
        case OUTLINE_DRAWING: return drawing_hash(&k->u, key_size);
        default: return 0;
    }
}

static unsigned outline_compare(void *a, void *b, size_t key_size)
{
    OutlineHashKey *ak = a;
    OutlineHashKey *bk = b;
    if (ak->type != bk->type) return 0;
    switch (ak->type) {
        case OUTLINE_GLYPH: return glyph_compare(&ak->u, &bk->u, key_size);
        case OUTLINE_DRAWING: return drawing_compare(&ak->u, &bk->u, key_size);
        default: return 0;
    }
}

static bool outline_key_move(void *dst, void *src, size_t key_size)
{
    OutlineHashKey *d = dst, *s = src;
    if (!dst) {
        if (s->type == OUTLINE_GLYPH)
            ass_cache_dec_ref(s->u.glyph.font);
        return true;
    }
    memcpy(dst, src, key_size);
    if (s->type != OUTLINE_DRAWING)
        return true;
    d->u.drawing.text = strdup(s->u.drawing.text);
    return d->u.drawing.text;
}

static void outline_destruct(void *key, void *value)
{
    OutlineHashValue *v = value;
    OutlineHashKey *k = key;
    outline_free(&v->outline);
    outline_free(&v->border[0]);
    outline_free(&v->border[1]);
    switch (k->type) {
        case OUTLINE_GLYPH: ass_cache_dec_ref(k->u.glyph.font); break;
        case OUTLINE_DRAWING: free(k->u.drawing.text); break;
    }
}

const CacheDesc outline_cache_desc = {
    .hash_func = outline_hash,
    .compare_func = outline_compare,
    .key_move_func = outline_key_move,
    .destruct_func = outline_destruct,
    .key_size = sizeof(OutlineHashKey),
    .value_size = sizeof(OutlineHashValue)
};


// glyph metric cache
static bool glyph_metrics_key_move(void *dst, void *src, size_t key_size)
{
    if (!dst)
        return true;
    memcpy(dst, src, key_size);
    GlyphMetricsHashKey *k = src;
    ass_cache_inc_ref(k->font);
    return true;
}

static void glyph_metrics_destruct(void *key, void *value)
{
    GlyphMetricsHashKey *k = key;
    ass_cache_dec_ref(k->font);
}

const CacheDesc glyph_metrics_cache_desc = {
    .hash_func = glyph_metrics_hash,
    .compare_func = glyph_metrics_compare,
    .key_move_func = glyph_metrics_key_move,
    .destruct_func = glyph_metrics_destruct,
    .key_size = sizeof(GlyphMetricsHashKey),
    .value_size = sizeof(GlyphMetricsHashValue)
};



// Cache data
typedef struct cache_item {
    Cache *cache;
    const CacheDesc *desc;
    struct cache_item *next, **prev;
    struct cache_item *queue_next, **queue_prev;
    size_t size, ref_count;
} CacheItem;

struct cache {
    unsigned buckets;
    CacheItem **map;
    CacheItem *queue_first, **queue_last;

    const CacheDesc *desc;

    size_t cache_size;
    unsigned hits;
    unsigned misses;
    unsigned items;
};

#define CACHE_ALIGN 8
#define CACHE_ITEM_SIZE ((sizeof(CacheItem) + (CACHE_ALIGN - 1)) & ~(CACHE_ALIGN - 1))

static inline size_t align_cache(size_t size)
{
    return (size + (CACHE_ALIGN - 1)) & ~(CACHE_ALIGN - 1);
}

static inline CacheItem *value_to_item(void *value)
{
    return (CacheItem *) ((char *) value - CACHE_ITEM_SIZE);
}


// Create a cache with type-specific hash/compare/destruct/size functions
Cache *ass_cache_create(const CacheDesc *desc)
{
    Cache *cache = calloc(1, sizeof(*cache));
    if (!cache)
        return NULL;
    cache->buckets = 0xFFFF;
    cache->queue_last = &cache->queue_first;
    cache->desc = desc;
    cache->map = calloc(cache->buckets, sizeof(CacheItem *));
    if (!cache->map) {
        free(cache);
        return NULL;
    }

    return cache;
}

bool ass_cache_get(Cache *cache, void *key, void *value_ptr)
{
    char **value = (char **) value_ptr;
    const CacheDesc *desc = cache->desc;
    size_t key_offs = CACHE_ITEM_SIZE + align_cache(desc->value_size);
    unsigned bucket = desc->hash_func(key, desc->key_size) % cache->buckets;
    CacheItem *item = cache->map[bucket];
    while (item) {
        if (desc->compare_func(key, (char *) item + key_offs, desc->key_size)) {
            assert(item->size);
            if (!item->queue_prev || item->queue_next) {
                if (item->queue_prev) {
                    item->queue_next->queue_prev = item->queue_prev;
                    *item->queue_prev = item->queue_next;
                } else
                    item->ref_count++;
                *cache->queue_last = item;
                item->queue_prev = cache->queue_last;
                cache->queue_last = &item->queue_next;
                item->queue_next = NULL;
            }
            cache->hits++;
            desc->key_move_func(NULL, key, desc->key_size);
            *value = (char *) item + CACHE_ITEM_SIZE;
            item->ref_count++;
            return true;
        }
        item = item->next;
    }
    cache->misses++;

    item = malloc(key_offs + desc->key_size);
    if (!item) {
        desc->key_move_func(NULL, key, desc->key_size);
        *value = NULL;
        return false;
    }
    item->size = 0;
    item->cache = cache;
    item->desc = desc;
    if (!desc->key_move_func((char *) item + key_offs, key, desc->key_size)) {
        free(item);
        *value = NULL;
        return false;
    }
    *value = (char *) item + CACHE_ITEM_SIZE;

    CacheItem **bucketptr = &cache->map[bucket];
    if (*bucketptr)
        (*bucketptr)->prev = &item->next;
    item->prev = bucketptr;
    item->next = *bucketptr;
    *bucketptr = item;

    item->queue_prev = NULL;
    item->queue_next = NULL;
    item->ref_count = 1;
    return false;
}

void *ass_cache_key(void *value)
{
    CacheItem *item = value_to_item(value);
    return (char *) value + align_cache(item->desc->value_size);
}

void ass_cache_commit(void *value, size_t item_size)
{
    CacheItem *item = value_to_item(value);
    assert(!item->size && item_size);
    item->size = item_size;
    Cache *cache = item->cache;
    cache->cache_size += item_size;
    cache->items++;

    *cache->queue_last = item;
    item->queue_prev = cache->queue_last;
    cache->queue_last = &item->queue_next;
    item->ref_count++;
}

static inline void destroy_item(const CacheDesc *desc, CacheItem *item)
{
    assert(item->desc == desc);
    char *value = (char *) item + CACHE_ITEM_SIZE;
    desc->destruct_func(value + align_cache(desc->value_size), value);
    free(item);
}

void ass_cache_inc_ref(void *value)
{
    if (!value)
        return;
    CacheItem *item = value_to_item(value);
    assert(item->size && item->ref_count);
    item->ref_count++;
}

void ass_cache_dec_ref(void *value)
{
    if (!value)
        return;
    CacheItem *item = value_to_item(value);
    assert(item->size && item->ref_count);
    if (--item->ref_count)
        return;

    Cache *cache = item->cache;
    if (cache) {
        if (item->next)
            item->next->prev = item->prev;
        *item->prev = item->next;

        cache->items--;
        cache->cache_size -= item->size;
    }
    destroy_item(item->desc, item);
}

void ass_cache_cut(Cache *cache, size_t max_size)
{
    if (cache->cache_size <= max_size)
        return;

    do {
        CacheItem *item = cache->queue_first;
        if (!item)
            break;
        assert(item->size);

        cache->queue_first = item->queue_next;
        if (--item->ref_count) {
            item->queue_prev = NULL;
            continue;
        }

        if (item->next)
            item->next->prev = item->prev;
        *item->prev = item->next;

        cache->items--;
        cache->cache_size -= item->size;
        destroy_item(cache->desc, item);
    } while (cache->cache_size > max_size);
    if (cache->queue_first)
        cache->queue_first->queue_prev = &cache->queue_first;
    else
        cache->queue_last = &cache->queue_first;
}

void ass_cache_stats(Cache *cache, size_t *size, unsigned *hits,
                     unsigned *misses, unsigned *count)
{
    if (size)
        *size = cache->cache_size;
    if (hits)
        *hits = cache->hits;
    if (misses)
        *misses = cache->misses;
    if (count)
        *count = cache->items;
}

void ass_cache_empty(Cache *cache)
{
    for (int i = 0; i < cache->buckets; i++) {
        CacheItem *item = cache->map[i];
        while (item) {
            assert(item->size);
            CacheItem *next = item->next;
            if (item->queue_prev)
                item->ref_count--;
            if (item->ref_count)
                item->cache = NULL;
            else
                destroy_item(cache->desc, item);
            item = next;
        }
        cache->map[i] = NULL;
    }

    cache->queue_first = NULL;
    cache->queue_last = &cache->queue_first;
    cache->items = cache->hits = cache->misses = cache->cache_size = 0;
}

void ass_cache_done(Cache *cache)
{
    ass_cache_empty(cache);
    free(cache->map);
    free(cache);
}

// Type-specific creation function
Cache *ass_font_cache_create(void)
{
    return ass_cache_create(&font_cache_desc);
}

Cache *ass_outline_cache_create(void)
{
    return ass_cache_create(&outline_cache_desc);
}

Cache *ass_glyph_metrics_cache_create(void)
{
    return ass_cache_create(&glyph_metrics_cache_desc);
}

Cache *ass_bitmap_cache_create(void)
{
    return ass_cache_create(&bitmap_cache_desc);
}

Cache *ass_composite_cache_create(void)
{
    return ass_cache_create(&composite_cache_desc);
}
