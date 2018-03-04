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

#ifndef LIBASS_FONTSELECT_H
#define LIBASS_FONTSELECT_H

#include <stdbool.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct ass_shaper_font_data ASS_ShaperFontData;
typedef struct font_selector ASS_FontSelector;
typedef struct font_info ASS_FontInfo;

#include "ass_types.h"
#include "ass.h"
#include "ass_font.h"

typedef struct font_provider ASS_FontProvider;

/* Font Provider */
typedef struct ass_font_provider_meta_data ASS_FontProviderMetaData;

/**
 * Get font data. This is a stream interface which can be used as an
 * alternative to providing a font path (which may not be available).
 *
 * This is called by fontselect if a given font was added without a
 * font path (i.e. the path was set to NULL).
 *
 * \param font_priv font private data
 * \param output buffer; set to NULL to query stream size
 * \param offset stream offset
 * \param len bytes to read into output buffer from stream
 * \return actual number of bytes read, or stream size if data == NULL
 */
typedef size_t  (*GetDataFunc)(void *font_priv, unsigned char *data,
                               size_t offset, size_t len);

/**
 * Check whether the font contains PostScript outlines.
 *
 * \param font_priv font private data
 * \return true if the font contains PostScript outlines
 */
typedef bool    (*CheckPostscriptFunc)(void *font_priv);

/**
 * Check if a glyph is supported by a font.
 *
 * \param font_priv font private data
 * \param codepont Unicode codepoint (UTF-32)
 * \return true if codepoint is supported by the font
 */
typedef bool    (*CheckGlyphFunc)(void *font_priv, uint32_t codepoint);

/**
* Get index of a font in context of a font collection.
* This function is optional and may be needed to initialize the font index
* lazily.
*
* \param font_priv font private data
* \return font index inside the collection, or 0 in case of a single font
*/
typedef unsigned    (*GetFontIndex)(void *font_priv);

/**
 * Destroy a font's private data.
 *
 *  \param font_priv font private data
 */
typedef void    (*DestroyFontFunc)(void *font_priv);

/**
 * Destroy a font provider's private data.
 *
 * \param priv font provider private data
 */
typedef void    (*DestroyProviderFunc)(void *priv);

/**
 * Add fonts for a given font name; this should add all fonts matching the
 * given name to the fontselect database.
 *
 * This is called by fontselect whenever a new logical font is created. The
 * font provider set as default is used.
 *
 * \param lib ASS_Library instance
 * \param provider font provider instance
 * \param name font name (as specified in script)
 */
typedef void    (*MatchFontsFunc)(ASS_Library *lib,
                                  ASS_FontProvider *provider,
                                  char *name);

/**
 * Substitute font name by another. This implements generic font family
 * substitutions (e.g. sans-serif, serif, monospace) as well as font aliases.
 *
 * The generic families should map to sensible platform-specific font families.
 * Aliases are sometimes used to map from common fonts that don't exist on
 * a particular platform to similar alternatives. For example, a Linux
 * system with fontconfig may map "Arial" to "Liberation Sans" and Windows
 * maps "Helvetica" to "Arial".
 *
 * This is called by fontselect when a new logical font is created. The font
 * provider set as default is used.
 *
 * \param priv font provider private data
 * \param name input string for substitution, as specified in the script
 * \param meta metadata (fullnames and n_fullname) to be filled in
 */
typedef void    (*SubstituteFontFunc)(void *priv, const char *name,
                                      ASS_FontProviderMetaData *meta);

/**
 * Get an appropriate fallback font for a given codepoint.
 *
 * This is called by fontselect whenever a glyph is not found in the
 * physical font list of a logical font. fontselect will try to add the
 * font family with match_fonts if it does not exist in the font list
 * add match_fonts is not NULL. Note that the returned font family should
 * contain the requested codepoint.
 *
 * Note that fontselect uses the font provider set as default to determine
 * fallbacks.
 *
 * \param priv font provider private data
 * \param family original font family name (try matching a similar font) (never NULL)
 * \param codepoint Unicode codepoint (UTF-32)
 * \return output font family, allocated with malloc(), must be freed
 *         by caller.
 */
typedef char   *(*GetFallbackFunc)(void *priv,
                                   const char *family,
                                   uint32_t codepoint);

typedef struct font_provider_funcs {
    GetDataFunc         get_data;               /* optional/mandatory */
    CheckPostscriptFunc check_postscript;       /* mandatory */
    CheckGlyphFunc      check_glyph;            /* mandatory */
    DestroyFontFunc     destroy_font;           /* optional */
    DestroyProviderFunc destroy_provider;       /* optional */
    MatchFontsFunc      match_fonts;            /* optional */
    SubstituteFontFunc  get_substitutions;      /* optional */
    GetFallbackFunc     get_fallback;           /* optional */
    GetFontIndex        get_font_index;         /* optional */
} ASS_FontProviderFuncs;

/*
 * Basic font metadata. All strings must be encoded with UTF-8.
 * At minimum one family is required.
 */
struct ass_font_provider_meta_data {
    /**
     * List of localized font family names, e.g. "Arial".
     */
    char **families;

    /**
     * List of localized full names, e.g. "Arial Bold".
     * The English name should be listed first to speed up typical matching.
     */
    char **fullnames;

    /**
     * The PostScript name, e.g. "Arial-BoldMT".
     */
    char *postscript_name;

    int n_family;       // Number of localized family names
    int n_fullname;     // Number of localized full names

    int slant;          // Font slant value from FONT_SLANT_*
    int weight;         // Font weight in TrueType scale, 100-900
                        // See FONT_WEIGHT_*
    int width;          // Font weight in percent, normally 100
                        // See FONT_WIDTH_*
};

typedef struct ass_font_stream ASS_FontStream;

struct ass_font_stream {
    // GetDataFunc
    size_t  (*func)(void *font_priv, unsigned char *data,
                    size_t offset, size_t len);
    void *priv;
};


typedef struct ass_font_mapping ASS_FontMapping;

struct ass_font_mapping {
    const char *from;
    const char *to;
};

/**
 * Simple font substitution helper. This can be used to implement basic
 * mappings from one name to another. This is useful for supporting
 * generic font families in font providers.
 *
 * \param map list of mappings
 * \param len length of list of mappings
 * \param name font name to map from
 * \param meta metadata struct, mapped fonts will be stored into this
 */
void ass_map_font(const ASS_FontMapping *map, int len, const char *name,
                  ASS_FontProviderMetaData *meta);

ASS_FontSelector *
ass_fontselect_init(ASS_Library *library,
                    FT_Library ftlibrary, const char *family,
                    const char *path, const char *config,
                    ASS_DefaultFontProvider dfp);
char *ass_font_select(ASS_FontSelector *priv, ASS_Library *library,
                      ASS_Font *font, int *index, char **postscript_name,
                      int *uid, ASS_FontStream *data, uint32_t code);
void ass_fontselect_free(ASS_FontSelector *priv);

// Font provider functions
ASS_FontProvider *ass_font_provider_new(ASS_FontSelector *selector,
        ASS_FontProviderFuncs *funcs, void *data);

/**
 * \brief Create an empty font provider. A font provider can be used to
 * provide additional fonts to libass.
 * \param priv parent renderer
 * \param funcs callback functions
 * \param private data for provider callbacks
 *
 */
ASS_FontProvider *
ass_create_font_provider(ASS_Renderer *priv, ASS_FontProviderFuncs *funcs,
                         void *data);

/**
 * \brief Add a font to a font provider.
 * \param provider the font provider
 * \param meta font metadata. See struct definition for more information.
 * \param path absolute path to font, or NULL for memory-based fonts
 * \param index index inside a font collection file
 *              (-1 to look up by PostScript name)
 * \param data private data for font callbacks
 * \return success
 *
 */
bool
ass_font_provider_add_font(ASS_FontProvider *provider,
                           ASS_FontProviderMetaData *meta, const char *path,
                           int index, void *data);

/**
 * \brief Free font provider and associated fonts.
 * \param provider the font provider
 *
 */
void ass_font_provider_free(ASS_FontProvider *provider);

#endif                          /* LIBASS_FONTSELECT_H */
