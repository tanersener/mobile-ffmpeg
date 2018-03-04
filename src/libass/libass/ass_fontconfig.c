/*
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "ass_fontconfig.h"
#include "ass_fontselect.h"
#include "ass_utils.h"

#define MAX_NAME 100

typedef struct fc_private {
    FcConfig *config;
    FcFontSet *fallbacks;
    FcCharSet *fallback_chars;
} ProviderPrivate;

static bool check_postscript(void *priv)
{
    FcPattern *pat = (FcPattern *)priv;
    char *format;

    FcResult result =
        FcPatternGetString(pat, FC_FONTFORMAT, 0, (FcChar8 **)&format);
    if (result != FcResultMatch)
        return false;

    return !strcmp(format, "Type 1") || !strcmp(format, "Type 42") ||
           !strcmp(format, "CID Type 1") || !strcmp(format, "CFF");
}

static bool check_glyph(void *priv, uint32_t code)
{
    FcPattern *pat = (FcPattern *)priv;
    FcCharSet *charset;

    if (!pat)
        return true;

    if (code == 0)
        return true;

    FcResult result = FcPatternGetCharSet(pat, FC_CHARSET, 0, &charset);
    if (result != FcResultMatch)
        return false;
    if (FcCharSetHasChar(charset, code) == FcTrue)
        return true;
    return false;
}

static void destroy(void *priv)
{
    ProviderPrivate *fc = (ProviderPrivate *)priv;

    if (fc->fallback_chars)
        FcCharSetDestroy(fc->fallback_chars);
    if (fc->fallbacks)
        FcFontSetDestroy(fc->fallbacks);
    FcConfigDestroy(fc->config);
    free(fc);
}

static void scan_fonts(FcConfig *config, ASS_FontProvider *provider)
{
    int i;
    FcFontSet *fonts;
    ASS_FontProviderMetaData meta;

    // get list of fonts
    fonts = FcConfigGetFonts(config, FcSetSystem);

    // fill font_info list
    for (i = 0; i < fonts->nfont; i++) {
        FcPattern *pat = fonts->fonts[i];
        FcBool outline;
        int index, weight;
        char *path;
        char *fullnames[MAX_NAME];
        char *families[MAX_NAME];

        // skip non-outline fonts
        FcResult result = FcPatternGetBool(pat, FC_OUTLINE, 0, &outline);
        if (result != FcResultMatch || outline != FcTrue)
            continue;

        // simple types
        result  = FcPatternGetInteger(pat, FC_SLANT, 0, &meta.slant);
        result |= FcPatternGetInteger(pat, FC_WIDTH, 0, &meta.width);
        result |= FcPatternGetInteger(pat, FC_WEIGHT, 0, &weight);
        result |= FcPatternGetInteger(pat, FC_INDEX, 0, &index);
        if (result != FcResultMatch)
            continue;

        // fontconfig uses its own weight scale, apparently derived
        // from typographical weight. we're using truetype weights, so
        // convert appropriately
        if (weight <= FC_WEIGHT_LIGHT)
            meta.weight = FONT_WEIGHT_LIGHT;
        else if (weight <= FC_WEIGHT_MEDIUM)
            meta.weight = FONT_WEIGHT_MEDIUM;
        else
            meta.weight = FONT_WEIGHT_BOLD;

        // path
        result = FcPatternGetString(pat, FC_FILE, 0, (FcChar8 **)&path);
        if (result != FcResultMatch)
            continue;

        // read family names
        meta.n_family = 0;
        while (FcPatternGetString(pat, FC_FAMILY, meta.n_family,
                    (FcChar8 **)&families[meta.n_family]) == FcResultMatch
                    && meta.n_family < MAX_NAME)
            meta.n_family++;
        meta.families = families;

        // read fullnames
        meta.n_fullname = 0;
        while (FcPatternGetString(pat, FC_FULLNAME, meta.n_fullname,
                    (FcChar8 **)&fullnames[meta.n_fullname]) == FcResultMatch
                    && meta.n_fullname < MAX_NAME)
            meta.n_fullname++;
        meta.fullnames = fullnames;

        // read PostScript name
        result = FcPatternGetString(pat, FC_POSTSCRIPT_NAME, 0,
                (FcChar8 **)&meta.postscript_name);
        if (result != FcResultMatch)
            meta.postscript_name = NULL;

        ass_font_provider_add_font(provider, &meta, path, index, (void *)pat);
    }
}

static void cache_fallbacks(ProviderPrivate *fc)
{
    FcResult result;

    if (fc->fallbacks)
        return;

    // Create a suitable pattern
    FcPattern *pat = FcPatternCreate();
    FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"sans-serif");
    FcPatternAddBool(pat, FC_OUTLINE, FcTrue);
    FcConfigSubstitute (fc->config, pat, FcMatchPattern);
    FcDefaultSubstitute (pat);

    // FC_LANG is automatically set according to locale, but this results
    // in strange sorting sometimes, so remove the attribute completely.
    FcPatternDel(pat, FC_LANG);

    // Sort installed fonts and eliminate duplicates; this can be very
    // expensive.
    fc->fallbacks = FcFontSort(fc->config, pat, FcTrue, &fc->fallback_chars,
            &result);

    // If this fails, just add an empty set
    if (result != FcResultMatch)
        fc->fallbacks = FcFontSetCreate();

    FcPatternDestroy(pat);
}

static char *get_fallback(void *priv, const char *family, uint32_t codepoint)
{
    ProviderPrivate *fc = (ProviderPrivate *)priv;
    FcResult result;

    cache_fallbacks(fc);

    if (!fc->fallbacks || fc->fallbacks->nfont == 0)
        return NULL;

    if (codepoint == 0) {
        char *family = NULL;
        result = FcPatternGetString(fc->fallbacks->fonts[0], FC_FAMILY, 0,
                (FcChar8 **)&family);
        if (result == FcResultMatch) {
            return strdup(family);
        } else {
            return NULL;
        }
    }

    // fallback_chars is the union of all available charsets, so
    // if we can't find the glyph in there, the system does not
    // have any font to render this glyph.
    if (FcCharSetHasChar(fc->fallback_chars, codepoint) == FcFalse)
        return NULL;

    for (int j = 0; j < fc->fallbacks->nfont; j++) {
        FcPattern *pattern = fc->fallbacks->fonts[j];

        FcCharSet *charset;
        result = FcPatternGetCharSet(pattern, FC_CHARSET, 0, &charset);

        if (result == FcResultMatch && FcCharSetHasChar(charset,
                    codepoint)) {
            char *family = NULL;
            result = FcPatternGetString(pattern, FC_FAMILY, 0,
                    (FcChar8 **)&family);
            if (result == FcResultMatch) {
                return strdup(family);
            } else {
                return NULL;
            }
        }
    }

    // we shouldn't get here
    return NULL;
}

static void get_substitutions(void *priv, const char *name,
                              ASS_FontProviderMetaData *meta)
{
    ProviderPrivate *fc = (ProviderPrivate *)priv;

    FcPattern *pat = FcPatternCreate();
    if (!pat)
        return;

    FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)name);
    FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"__libass_delimiter");
    FcPatternAddBool(pat, FC_OUTLINE, FcTrue);
    if (!FcConfigSubstitute(fc->config, pat, FcMatchPattern))
        goto cleanup;

    // read and strdup fullnames
    meta->n_fullname = 0;
    meta->fullnames = calloc(MAX_NAME, sizeof(char *));
    if (!meta->fullnames)
        goto cleanup;

    char *alias = NULL;
    while (FcPatternGetString(pat, FC_FAMILY, meta->n_fullname,
                (FcChar8 **)&alias) == FcResultMatch
                && meta->n_fullname < MAX_NAME
                && strcmp(alias, "__libass_delimiter") != 0) {
        alias = strdup(alias);
        if (!alias)
            goto cleanup;
        meta->fullnames[meta->n_fullname] = alias;
        meta->n_fullname++;
    }

cleanup:
    FcPatternDestroy(pat);
}

static ASS_FontProviderFuncs fontconfig_callbacks = {
    .check_postscript   = check_postscript,
    .check_glyph        = check_glyph,
    .destroy_provider   = destroy,
    .get_substitutions  = get_substitutions,
    .get_fallback       = get_fallback,
};

ASS_FontProvider *
ass_fontconfig_add_provider(ASS_Library *lib, ASS_FontSelector *selector,
                            const char *config)
{
    int rc;
    ProviderPrivate *fc = NULL;
    ASS_FontProvider *provider = NULL;

    fc = calloc(1, sizeof(ProviderPrivate));
    if (fc == NULL)
        return NULL;

    // build and load fontconfig configuration
    fc->config = FcConfigCreate();
    rc = FcConfigParseAndLoad(fc->config, (unsigned char *) config, FcTrue);
    if (!rc) {
        ass_msg(lib, MSGL_WARN, "No usable fontconfig configuration "
                "file found, using fallback.");
        FcConfigDestroy(fc->config);
        fc->config = FcInitLoadConfig();
    }
    if (fc->config)
        rc = FcConfigBuildFonts(fc->config);

    if (!rc || !fc->config) {
        ass_msg(lib, MSGL_FATAL,
                "No valid fontconfig configuration found!");
        FcConfigDestroy(fc->config);
        free(fc);
        return NULL;
    }

    // create font provider
    provider = ass_font_provider_new(selector, &fontconfig_callbacks, fc);

    // build database from system fonts
    scan_fonts(fc->config, provider);

    return provider;
}
