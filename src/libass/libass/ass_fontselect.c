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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <limits.h>
#include <ft2build.h>
#include <sys/types.h>
#include <dirent.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TYPE1_TABLES_H

#include "ass_utils.h"
#include "ass.h"
#include "ass_library.h"
#include "ass_fontselect.h"
#include "ass_fontconfig.h"
#include "ass_coretext.h"
#include "ass_directwrite.h"
#include "ass_font.h"
#include "ass_string.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX_FULLNAME 100

// internal font database element
// all strings are utf-8
struct font_info {
    int uid;            // unique font face id

    char **families;    // family name
    char **fullnames;   // list of localized fullnames (e.g. Arial Bold Italic)
    int n_family;
    int n_fullname;

    int slant;
    int weight;         // TrueType scale, 100-900
    int width;

    // how to access this face
    char *path;            // absolute path
    int index;             // font index inside font collections

    char *postscript_name; // can be used as an alternative to index to
                           // identify a font inside a collection

    // font source
    ASS_FontProvider *provider;

    // private data for callbacks
    void *priv;
};

struct font_selector {
    // uid counter
    int uid;

    // fallbacks
    char *family_default;
    char *path_default;
    int index_default;

    // font database
    int n_font;
    int alloc_font;
    ASS_FontInfo *font_infos;

    ASS_FontProvider *default_provider;
    ASS_FontProvider *embedded_provider;
};

struct font_provider {
    ASS_FontSelector *parent;
    ASS_FontProviderFuncs funcs;
    void *priv;
};

typedef struct font_data_ft FontDataFT;
struct font_data_ft {
    ASS_Library *lib;
    FT_Face face;
    int idx;
};

static bool check_postscript_ft(void *data)
{
    FontDataFT *fd = (FontDataFT *)data;
    PS_FontInfoRec postscript_info;
    return !FT_Get_PS_Font_Info(fd->face, &postscript_info);
}

static bool check_glyph_ft(void *data, uint32_t codepoint)
{
    FontDataFT *fd = (FontDataFT *)data;

    if (!codepoint)
        return true;

    return !!FT_Get_Char_Index(fd->face, codepoint);
}

static void destroy_font_ft(void *data)
{
    FontDataFT *fd = (FontDataFT *)data;

    FT_Done_Face(fd->face);
    free(fd);
}

static size_t
get_data_embedded(void *data, unsigned char *buf, size_t offset, size_t len)
{
    FontDataFT *ft = (FontDataFT *)data;
    ASS_Fontdata *fd = ft->lib->fontdata;
    int i = ft->idx;

    if (buf == NULL)
        return fd[i].size;

    if (offset >= fd[i].size)
        return 0;

    if (len > fd[i].size - offset)
        len = fd[i].size - offset;

    memcpy(buf, fd[i].data + offset, len);
    return len;
}

static ASS_FontProviderFuncs ft_funcs = {
    .get_data          = get_data_embedded,
    .check_postscript  = check_postscript_ft,
    .check_glyph       = check_glyph_ft,
    .destroy_font      = destroy_font_ft,
};

static void load_fonts_from_dir(ASS_Library *library, const char *dir)
{
    DIR *d = opendir(dir);
    if (!d)
        return;
    while (1) {
        struct dirent *entry = readdir(d);
        if (!entry)
            break;
        if (entry->d_name[0] == '.')
            continue;
        char fullname[4096];
        snprintf(fullname, sizeof(fullname), "%s/%s", dir, entry->d_name);
        size_t bufsize = 0;
        ass_msg(library, MSGL_INFO, "Loading font file '%s'", fullname);
        void *data = read_file(library, fullname, &bufsize);
        if (data) {
            ass_add_font(library, entry->d_name, data, bufsize);
            free(data);
        }
    }
    closedir(d);
}

/**
 * \brief Create a bare font provider.
 * \param selector parent selector. The provider will be attached to it.
 * \param funcs callback/destroy functions
 * \param data private data of the provider
 * \return the font provider
 */
ASS_FontProvider *
ass_font_provider_new(ASS_FontSelector *selector, ASS_FontProviderFuncs *funcs,
                      void *data)
{
    ASS_FontProvider *provider = calloc(1, sizeof(ASS_FontProvider));
    if (provider == NULL)
        return NULL;

    provider->parent   = selector;
    provider->funcs    = *funcs;
    provider->priv     = data;

    return provider;
}

/**
 * Free all data associated with a FontInfo struct. Handles FontInfo structs
 * with incomplete allocations well.
 *
 * \param info FontInfo struct to free associated data from
 */
static void ass_font_provider_free_fontinfo(ASS_FontInfo *info)
{
    int j;

    if (info->fullnames) {
        for (j = 0; j < info->n_fullname; j++)
            free(info->fullnames[j]);
        free(info->fullnames);
    }

    if (info->families) {
        for (j = 0; j < info->n_family; j++)
            free(info->families[j]);
        free(info->families);
    }

    if (info->path)
        free(info->path);

    if (info->postscript_name)
        free(info->postscript_name);

}

/**
 * \brief Add a font to a font provider.
 * \param provider the font provider
 * \param meta basic metadata of the font
 * \param path path to the font file, or NULL
 * \param index face index inside the file (-1 to look up by PostScript name)
 * \param data private data for the font
 * \return success
 */
bool
ass_font_provider_add_font(ASS_FontProvider *provider,
                           ASS_FontProviderMetaData *meta, const char *path,
                           int index, void *data)
{
    int i;
    int weight, slant, width;
    ASS_FontSelector *selector = provider->parent;
    ASS_FontInfo *info;

#if 0
    int j;
    printf("new font:\n");
    printf("  families: ");
    for (j = 0; j < meta->n_family; j++)
        printf("'%s' ", meta->families[j]);
    printf("\n");
    printf("  fullnames: ");
    for (j = 0; j < meta->n_fullname; j++)
        printf("'%s' ", meta->fullnames[j]);
    printf("\n");
    printf("  slant: %d\n", meta->slant);
    printf("  weight: %d\n", meta->weight);
    printf("  width: %d\n", meta->width);
    printf("  path: %s\n", path);
    printf("  index: %d\n", index);
#endif

    weight = meta->weight;
    slant  = meta->slant;
    width  = meta->width;

    // check slant/weight for validity, use defaults if they're invalid
    if (weight < 100 || weight > 900)
        weight = 400;
    if (slant < 0 || slant > 110)
        slant = 0;
    if (width < 50 || width > 200)
        width = 100;

    // check size
    if (selector->n_font >= selector->alloc_font) {
        selector->alloc_font = FFMAX(1, 2 * selector->alloc_font);
        selector->font_infos = realloc(selector->font_infos,
                selector->alloc_font * sizeof(ASS_FontInfo));
    }

    // copy over metadata
    info = selector->font_infos + selector->n_font;
    memset(info, 0, sizeof(ASS_FontInfo));

    // set uid
    info->uid = selector->uid++;

    info->slant         = slant;
    info->weight        = weight;
    info->width         = width;
    info->n_fullname    = meta->n_fullname;
    info->n_family      = meta->n_family;

    info->families = calloc(meta->n_family, sizeof(char *));
    if (info->families == NULL)
        goto error;

    if (meta->n_fullname) {
        info->fullnames = calloc(meta->n_fullname, sizeof(char *));
        if (info->fullnames == NULL)
            goto error;
    }

    for (i = 0; i < info->n_family; i++) {
        info->families[i] = strdup(meta->families[i]);
        if (info->families[i] == NULL)
            goto error;
    }

    for (i = 0; i < info->n_fullname; i++) {
        info->fullnames[i] = strdup(meta->fullnames[i]);
        if (info->fullnames[i] == NULL)
            goto error;
    }

    if (meta->postscript_name) {
        info->postscript_name = strdup(meta->postscript_name);
        if (info->postscript_name == NULL)
            goto error;
    }

    if (path) {
        info->path = strdup(path);
        if (info->path == NULL)
            goto error;
    }

    info->index = index;
    info->priv  = data;
    info->provider = provider;

    selector->n_font++;
    return true;

error:
    ass_font_provider_free_fontinfo(info);

    if (provider->funcs.destroy_font)
        provider->funcs.destroy_font(data);

    return false;
}

/**
 * \brief Clean up font database. Deletes all fonts that have an invalid
 * font provider (NULL).
 * \param selector the font selector
 */
static void ass_fontselect_cleanup(ASS_FontSelector *selector)
{
    int i, w;

    for (i = 0, w = 0; i < selector->n_font; i++) {
        ASS_FontInfo *info = selector->font_infos + i;

        // update write pointer
        if (info->provider != NULL) {
            // rewrite, if needed
            if (w != i)
                memcpy(selector->font_infos + w, selector->font_infos + i,
                        sizeof(ASS_FontInfo));
            w++;
        }

    }

    selector->n_font = w;
}

void ass_font_provider_free(ASS_FontProvider *provider)
{
    int i;
    ASS_FontSelector *selector = provider->parent;

    // free all fonts and mark their entries
    for (i = 0; i < selector->n_font; i++) {
        ASS_FontInfo *info = selector->font_infos + i;

        if (info->provider == provider) {
            ass_font_provider_free_fontinfo(info);

            if (info->provider->funcs.destroy_font)
                info->provider->funcs.destroy_font(info->priv);

            info->provider = NULL;
        }

    }

    // delete marked entries
    ass_fontselect_cleanup(selector);

    // free private data of the provider
    if (provider->funcs.destroy_provider)
        provider->funcs.destroy_provider(provider->priv);

    free(provider);
}

static bool check_postscript(ASS_FontInfo *fi)
{
    ASS_FontProvider *provider = fi->provider;
    assert(provider && provider->funcs.check_postscript);

    return provider->funcs.check_postscript(fi->priv);
}

/**
 * \brief Return whether the given font is in the given family.
 */
static bool matches_family_name(ASS_FontInfo *f, const char *family)
{
    for (int i = 0; i < f->n_family; i++) {
        if (ass_strcasecmp(f->families[i], family) == 0)
            return true;
    }
    return false;
}

/**
 * \brief Return whether the given font has the given fullname or
 * PostScript name depending on whether it has PostScript outlines.
 */
static bool matches_full_or_postscript_name(ASS_FontInfo *f,
                                            const char *fullname)
{
    bool matches_fullname = false;
    bool matches_postscript_name = false;

    for (int i = 0; i < f->n_fullname; i++) {
        if (ass_strcasecmp(f->fullnames[i], fullname) == 0) {
            matches_fullname = true;
            break;
        }
    }

    if (f->postscript_name != NULL &&
        ass_strcasecmp(f->postscript_name, fullname) == 0)
        matches_postscript_name = true;

    if (matches_fullname == matches_postscript_name)
        return matches_fullname;

    if (check_postscript(f))
        return matches_postscript_name;
    else
        return matches_fullname;
}

/**
 * \brief Compare attributes of font (a) against a font request (req). Returns
 * a matching score - the lower the better.
 * Ignores font names/families!
 * \param a font
 * \param b font request
 * \return matching score
 */
static unsigned font_attributes_similarity(ASS_FontInfo *a, ASS_FontInfo *req)
{
    unsigned similarity = 0;
    similarity += ABS(a->weight - req->weight);
    similarity += ABS(a->slant - req->slant);
    similarity += ABS(a->width - req->width);

    return similarity;
}

#if 0
// dump font information
static void font_info_dump(ASS_FontInfo *font_infos, size_t len)
{
    int i, j;

    // dump font infos
    for (i = 0; i < len; i++) {
        printf("font %d\n", i);
        printf("  families: ");
        for (j = 0; j < font_infos[i].n_family; j++)
            printf("'%s' ", font_infos[i].families[j]);
        printf("  fullnames: ");
        for (j = 0; j < font_infos[i].n_fullname; j++)
            printf("'%s' ", font_infos[i].fullnames[j]);
        printf("\n");
        printf("  slant: %d\n", font_infos[i].slant);
        printf("  weight: %d\n", font_infos[i].weight);
        printf("  width: %d\n", font_infos[i].width);
        printf("  path: %s\n", font_infos[i].path);
        printf("  index: %d\n", font_infos[i].index);
        printf("  score: %d\n", font_infos[i].score);

    }
}
#endif

static bool check_glyph(ASS_FontInfo *fi, uint32_t code)
{
    ASS_FontProvider *provider = fi->provider;
    assert(provider && provider->funcs.check_glyph);

    return provider->funcs.check_glyph(fi->priv, code);
}

static char *
find_font(ASS_FontSelector *priv, ASS_Library *library,
          ASS_FontProviderMetaData meta, unsigned bold, unsigned italic,
          int *index, char **postscript_name, int *uid, ASS_FontStream *stream,
          uint32_t code, bool *name_match)
{
    ASS_FontInfo req = {0};
    ASS_FontInfo *selected = NULL;

    // do we actually have any fonts?
    if (!priv->n_font)
        return NULL;

    // fill font request
    req.slant   = italic;
    req.weight  = bold;
    req.width   = 100;

    // Match font family name against font list
    unsigned score_min = UINT_MAX;
    for (int i = 0; i < meta.n_fullname; i++) {
        const char *fullname = meta.fullnames[i];

        for (int x = 0; x < priv->n_font; x++) {
            ASS_FontInfo *font = &priv->font_infos[x];
            unsigned score = UINT_MAX;

            if (matches_family_name(font, fullname)) {
                // If there's a family match, compare font attributes
                // to determine best match in that particular family
                score = font_attributes_similarity(font, &req);
                *name_match = true;
            } else if (matches_full_or_postscript_name(font, fullname)) {
                // If we don't have any match, compare fullnames against request
                // if there is a match now, assign lowest score possible. This means
                // the font should be chosen instantly, without further search.
                score = 0;
                *name_match = true;
            }

            // Consider updating idx if score is better than current minimum
            if (score < score_min) {
                // Check if the font has the requested glyph.
                // We are doing this here, for every font face, because
                // coverage might differ between the variants of a font
                // family. In practice, it is common that the regular
                // style has the best coverage while bold/italic/etc
                // variants cover less (e.g. FreeSans family).
                // We want to be able to match even if the closest variant
                // does not have the requested glyph, but another member
                // of the family has the glyph.
                if (!check_glyph(font, code))
                    continue;

                score_min = score;
                selected = font;
            }

            // Lowest possible score instantly matches; this is typical
            // for fullname matches, but can also occur with family matches.
            if (score == 0)
                break;
        }

        // The list of names is sorted by priority. If we matched anything,
        // we can and should stop.
        if (selected != NULL)
            break;
    }

    // found anything?
    char *result = NULL;
    if (selected) {
        ASS_FontProvider *provider = selected->provider;

        // successfully matched, set up return values
        *postscript_name = selected->postscript_name;
        *uid   = selected->uid;

        // use lazy evaluation for index if applicable
        if (provider->funcs.get_font_index) {
            *index = provider->funcs.get_font_index(selected->priv);
        } else
            *index = selected->index;

        // set up memory stream if there is no path
        if (selected->path == NULL) {
            stream->func = provider->funcs.get_data;
            stream->priv = selected->priv;
            // Prefer PostScript name because it is unique. This is only
            // used for display purposes so it doesn't matter that much,
            // though.
            if (selected->postscript_name)
                result = selected->postscript_name;
            else
                result = selected->families[0];
        } else
            result = selected->path;

    }

    return result;
}

static char *select_font(ASS_FontSelector *priv, ASS_Library *library,
                         const char *family, unsigned bold, unsigned italic,
                         int *index, char **postscript_name, int *uid,
                         ASS_FontStream *stream, uint32_t code)
{
    ASS_FontProvider *default_provider = priv->default_provider;
    ASS_FontProviderMetaData meta = {0};
    char *result = NULL;
    bool name_match = false;

    if (family == NULL)
        return NULL;

    ASS_FontProviderMetaData default_meta = {
        .n_fullname = 1,
        .fullnames  = (char **)&family,
    };

    // Get a list of substitutes if applicable, and use it for matching.
    if (default_provider && default_provider->funcs.get_substitutions) {
        default_provider->funcs.get_substitutions(default_provider->priv,
                                                  family, &meta);
    }

    if (!meta.n_fullname) {
        meta = default_meta;
    }

    result = find_font(priv, library, meta, bold, italic, index,
                       postscript_name, uid, stream, code, &name_match);

    // If no matching font was found, it might not exist in the font list
    // yet. Call the match_fonts callback to fill in the missing fonts
    // on demand, and retry the search for a match.
    if (result == NULL && name_match == false && default_provider &&
            default_provider->funcs.match_fonts) {
        // TODO: consider changing the API to make more efficient
        // implementations possible.
        for (int i = 0; i < meta.n_fullname; i++) {
            default_provider->funcs.match_fonts(library, default_provider,
                                                meta.fullnames[i]);
        }
        result = find_font(priv, library, meta, bold, italic, index,
                           postscript_name, uid, stream, code, &name_match);
    }

    // cleanup
    if (meta.fullnames != default_meta.fullnames) {
        for (int i = 0; i < meta.n_fullname; i++)
            free(meta.fullnames[i]);
        free(meta.fullnames);
    }

    return result;
}


/**
 * \brief Find a font. Use default family or path if necessary.
 * \param library ASS library handle
 * \param family font family
 * \param treat_family_as_pattern treat family as fontconfig pattern
 * \param bold font weight value
 * \param italic font slant value
 * \param index out: font index inside a file
 * \param code: the character that should be present in the font, can be 0
 * \return font file path
*/
char *ass_font_select(ASS_FontSelector *priv, ASS_Library *library,
                      ASS_Font *font, int *index, char **postscript_name,
                      int *uid, ASS_FontStream *data, uint32_t code)
{
    char *res = 0;
    const char *family = font->desc.family;
    unsigned bold = font->desc.bold;
    unsigned italic = font->desc.italic;
    ASS_FontProvider *default_provider = priv->default_provider;

    if (family && *family)
        res = select_font(priv, library, family, bold, italic, index,
                postscript_name, uid, data, code);

    if (!res && priv->family_default) {
        res = select_font(priv, library, priv->family_default, bold,
                italic, index, postscript_name, uid, data, code);
        if (res)
            ass_msg(library, MSGL_WARN, "fontselect: Using default "
                    "font family: (%s, %d, %d) -> %s, %d, %s",
                    family, bold, italic, res, *index,
                    *postscript_name ? *postscript_name : "(none)");
    }

    if (!res && default_provider && default_provider->funcs.get_fallback) {
        const char *search_family = family;
        if (!search_family || !*search_family)
            search_family = "Arial";
        char *fallback_family = default_provider->funcs.get_fallback(
                default_provider->priv, search_family, code);

        if (fallback_family) {
            res = select_font(priv, library, fallback_family, bold, italic,
                    index, postscript_name, uid, data, code);
            free(fallback_family);
        }
    }

    if (!res && priv->path_default) {
        res = priv->path_default;
        *index = priv->index_default;
        ass_msg(library, MSGL_WARN, "fontselect: Using default font: "
                "(%s, %d, %d) -> %s, %d, %s", family, bold, italic,
                priv->path_default, *index,
                *postscript_name ? *postscript_name : "(none)");
    }

    if (res)
        ass_msg(library, MSGL_INFO,
                "fontselect: (%s, %d, %d) -> %s, %d, %s", family, bold,
                italic, res, *index, *postscript_name ? *postscript_name : "(none)");

    return res;
}


/**
 * \brief Read basic metadata (names, weight, slant) from a FreeType face,
 * as required for the FontSelector for matching and sorting.
 * \param lib FreeType library
 * \param face FreeType face
 * \param info metadata, returned here
 * \return success
 */
static bool
get_font_info(FT_Library lib, FT_Face face, ASS_FontProviderMetaData *info)
{
    int i;
    int num_fullname = 0;
    int num_family   = 0;
    int num_names = FT_Get_Sfnt_Name_Count(face);
    int slant, weight;
    char *fullnames[MAX_FULLNAME];
    char *families[MAX_FULLNAME];

    // we're only interested in outlines
    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE))
        return false;

    for (i = 0; i < num_names; i++) {
        FT_SfntName name;

        if (FT_Get_Sfnt_Name(face, i, &name))
            continue;

        if (name.platform_id == TT_PLATFORM_MICROSOFT &&
                (name.name_id == TT_NAME_ID_FULL_NAME ||
                 name.name_id == TT_NAME_ID_FONT_FAMILY)) {
            char buf[1024];
            ass_utf16be_to_utf8(buf, sizeof(buf), (uint8_t *)name.string,
                                name.string_len);

            if (name.name_id == TT_NAME_ID_FULL_NAME) {
                fullnames[num_fullname] = strdup(buf);
                if (fullnames[num_fullname] == NULL)
                    goto error;
                num_fullname++;
            }

            if (name.name_id == TT_NAME_ID_FONT_FAMILY) {
                families[num_family] = strdup(buf);
                if (families[num_family] == NULL)
                    goto error;
                num_family++;
            }
        }

    }

    // check if we got a valid family - if not use whatever FreeType gives us
    if (num_family == 0 && face->family_name) {
        families[0] = strdup(face->family_name);
        if (families[0] == NULL)
            goto error;
        num_family++;
    }

    // we absolutely need a name
    if (num_family == 0)
        goto error;

    // calculate sensible slant and weight from style attributes
    slant  = 110 * !!(face->style_flags & FT_STYLE_FLAG_ITALIC);
    weight = 300 * !!(face->style_flags & FT_STYLE_FLAG_BOLD) + 400;

    // fill our struct
    info->slant  = slant;
    info->weight = weight;
    info->width  = 100;     // FIXME, should probably query the OS/2 table

    info->postscript_name = (char *)FT_Get_Postscript_Name(face);

    info->families = calloc(sizeof(char *), num_family);
    if (info->families == NULL)
        goto error;
    memcpy(info->families, &families, sizeof(char *) * num_family);
    info->n_family = num_family;

    if (num_fullname) {
        info->fullnames = calloc(sizeof(char *), num_fullname);
        if (info->fullnames == NULL)
            goto error;
        memcpy(info->fullnames, &fullnames, sizeof(char *) * num_fullname);
        info->n_fullname = num_fullname;
    }

    return true;

error:
    for (i = 0; i < num_family; i++)
        free(families[i]);

    for (i = 0; i < num_fullname; i++)
        free(fullnames[i]);

    free(info->families);
    free(info->fullnames);

    return false;
}

/**
 * \brief Free the dynamically allocated fields of metadata
 * created by get_font_info.
 * \param meta metadata created by get_font_info
 */
static void free_font_info(ASS_FontProviderMetaData *meta)
{
    int i;

    for (i = 0; i < meta->n_family; i++)
        free(meta->families[i]);

    for (i = 0; i < meta->n_fullname; i++)
        free(meta->fullnames[i]);

    free(meta->families);
    free(meta->fullnames);
}

/**
 * \brief Process memory font.
 * \param priv private data
 * \param library library object
 * \param ftlibrary freetype library object
 * \param idx index of the processed font in library->fontdata
 *
 * Builds a FontInfo with FreeType and some table reading.
*/
static void process_fontdata(ASS_FontProvider *priv, ASS_Library *library,
                             FT_Library ftlibrary, int idx)
{
    int rc;
    const char *name = library->fontdata[idx].name;
    const char *data = library->fontdata[idx].data;
    int data_size = library->fontdata[idx].size;

    FT_Face face;
    int face_index, num_faces = 1;

    for (face_index = 0; face_index < num_faces; ++face_index) {
        ASS_FontProviderMetaData info;
        FontDataFT *ft;

        rc = FT_New_Memory_Face(ftlibrary, (unsigned char *) data,
                                data_size, face_index, &face);
        if (rc) {
            ass_msg(library, MSGL_WARN, "Error opening memory font '%s'",
                   name);
            continue;
        }

        num_faces = face->num_faces;

        charmap_magic(library, face);

        memset(&info, 0, sizeof(ASS_FontProviderMetaData));
        if (!get_font_info(ftlibrary, face, &info)) {
            ass_msg(library, MSGL_WARN,
                    "Error getting metadata for embedded font '%s'", name);
            FT_Done_Face(face);
            continue;
        }

        ft = calloc(1, sizeof(FontDataFT));

        if (ft == NULL) {
            free_font_info(&info);
            FT_Done_Face(face);
            continue;
        }

        ft->lib  = library;
        ft->face = face;
        ft->idx  = idx;

        if (!ass_font_provider_add_font(priv, &info, NULL, face_index, ft)) {
            ass_msg(library, MSGL_WARN, "Failed to add embedded font '%s'",
                    name);
        }

        free_font_info(&info);
    }
}

/**
 * \brief Create font provider for embedded fonts. This parses the fonts known
 * to the current ASS_Library and adds them to the selector.
 * \param lib library
 * \param selector font selector
 * \param ftlib FreeType library - used for querying fonts
 * \return font provider
 */
static ASS_FontProvider *
ass_embedded_fonts_add_provider(ASS_Library *lib, ASS_FontSelector *selector,
                                FT_Library ftlib)
{
    int i;
    ASS_FontProvider *priv = ass_font_provider_new(selector, &ft_funcs, NULL);
    if (priv == NULL)
        return NULL;

    if (lib->fonts_dir && lib->fonts_dir[0]) {
        load_fonts_from_dir(lib, lib->fonts_dir);
    }

    for (i = 0; i < lib->num_fontdata; ++i)
        process_fontdata(priv, lib, ftlib, i);

    return priv;
}

struct font_constructors {
    ASS_DefaultFontProvider id;
    ASS_FontProvider *(*constructor)(ASS_Library *, ASS_FontSelector *,
                                     const char *);
    const char *name;
};

struct font_constructors font_constructors[] = {
#ifdef CONFIG_CORETEXT
    { ASS_FONTPROVIDER_CORETEXT,        &ass_coretext_add_provider,     "coretext"},
#endif
#ifdef CONFIG_DIRECTWRITE
    { ASS_FONTPROVIDER_DIRECTWRITE,     &ass_directwrite_add_provider,  "directwrite"},
#endif
#ifdef CONFIG_FONTCONFIG
    { ASS_FONTPROVIDER_FONTCONFIG,      &ass_fontconfig_add_provider,   "fontconfig"},
#endif
    { ASS_FONTPROVIDER_NONE, NULL, NULL },
};

/**
 * \brief Init font selector.
 * \param library libass library object
 * \param ftlibrary freetype library object
 * \param family default font family
 * \param path default font path
 * \return newly created font selector
 */
ASS_FontSelector *
ass_fontselect_init(ASS_Library *library,
                    FT_Library ftlibrary, const char *family,
                    const char *path, const char *config,
                    ASS_DefaultFontProvider dfp)
{
    ASS_FontSelector *priv = calloc(1, sizeof(ASS_FontSelector));
    if (priv == NULL)
        return NULL;

    priv->uid = 1;
    priv->family_default = family ? strdup(family) : NULL;
    priv->path_default = path ? strdup(path) : NULL;
    priv->index_default = 0;

    priv->embedded_provider = ass_embedded_fonts_add_provider(library, priv,
            ftlibrary);

    if (priv->embedded_provider == NULL) {
        ass_msg(library, MSGL_WARN, "failed to create embedded font provider");
    }

    if (dfp >= ASS_FONTPROVIDER_AUTODETECT) {
        for (int i = 0; font_constructors[i].constructor; i++ )
            if (dfp == font_constructors[i].id ||
                dfp == ASS_FONTPROVIDER_AUTODETECT) {
                priv->default_provider =
                    font_constructors[i].constructor(library, priv, config);
                if (priv->default_provider) {
                    ass_msg(library, MSGL_INFO, "Using font provider %s",
                            font_constructors[i].name);
                    break;
                }
            }

        if (!priv->default_provider)
            ass_msg(library, MSGL_WARN, "can't find selected font provider");

    }

    return priv;
}

void ass_get_available_font_providers(ASS_Library *priv,
                                      ASS_DefaultFontProvider **providers,
                                      size_t *size)
{
    size_t offset = 2;

    *size = offset;
    for (int i = 0; font_constructors[i].constructor; i++)
        (*size)++;

    *providers = calloc(*size, sizeof(ASS_DefaultFontProvider));

    if (*providers == NULL) {
        *size = (size_t)-1;
        return;
    }

    (*providers)[0] = ASS_FONTPROVIDER_NONE;
    (*providers)[1] = ASS_FONTPROVIDER_AUTODETECT;

    for (int i = offset; i < *size; i++)
        (*providers)[i] = font_constructors[i-offset].id;
}

/**
 * \brief Free font selector and release associated data
 * \param the font selector
 */
void ass_fontselect_free(ASS_FontSelector *priv)
{
    if (priv->default_provider)
        ass_font_provider_free(priv->default_provider);
    if (priv->embedded_provider)
        ass_font_provider_free(priv->embedded_provider);

    free(priv->font_infos);
    free(priv->path_default);
    free(priv->family_default);

    free(priv);
}

void ass_map_font(const ASS_FontMapping *map, int len, const char *name,
                  ASS_FontProviderMetaData *meta)
{
    for (int i = 0; i < len; i++) {
        if (ass_strcasecmp(map[i].from, name) == 0) {
            meta->fullnames = calloc(1, sizeof(char *));
            if (meta->fullnames) {
                meta->fullnames[0] = strdup(map[i].to);
                if (meta->fullnames[0])
                    meta->n_fullname = 1;
            }
            return;
        }
    }
}
