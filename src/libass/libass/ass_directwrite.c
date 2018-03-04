/*
 * Copyright (C) 2015 Stephan Vedder <stephan.vedder@gmail.com>
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
#define COBJMACROS

#include "config.h"
#include "ass_compat.h"

#include <initguid.h>
#include <ole2.h>
#include <shobjidl.h>

#include "dwrite_c.h"

#include "ass_directwrite.h"
#include "ass_utils.h"

#define NAME_MAX_LENGTH 256
#define FALLBACK_DEFAULT_FONT L"Arial"

static const ASS_FontMapping font_substitutions[] = {
    {"sans-serif", "Arial"},
    {"serif", "Times New Roman"},
    {"monospace", "Courier New"}
};

/*
 * The private data stored for every font, detected by this backend.
 */
typedef struct {
    IDWriteFont *font;
    IDWriteFontFace *face;
    IDWriteFontFileStream *stream;
} FontPrivate;

typedef struct {
    HMODULE directwrite_lib;
    IDWriteFactory *factory;
} ProviderPrivate;

/**
 * Custom text renderer class for logging the fonts used. It does not
 * actually render anything or do anything apart from that.
 */

typedef struct FallbackLogTextRenderer {
    IDWriteTextRenderer iface;
    IDWriteTextRendererVtbl vtbl;
    IDWriteFactory *dw_factory;
    LONG ref_count;
} FallbackLogTextRenderer;

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_IsPixelSnappingDisabled(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    BOOL* isDisabled
    )
{
    *isDisabled = true;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_GetCurrentTransform(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    DWRITE_MATRIX* transform
    )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_GetPixelsPerDip(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    FLOAT* pixelsPerDip
    )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_DrawGlyphRun(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
    )
{
    FallbackLogTextRenderer *this = (FallbackLogTextRenderer *)This;
    HRESULT hr;
    IDWriteFontCollection *font_coll = NULL;
    IDWriteFont **font = (IDWriteFont **)clientDrawingContext;

    hr = IDWriteFactory_GetSystemFontCollection(this->dw_factory, &font_coll, FALSE);
    if (FAILED(hr))
        return E_FAIL;

    hr = IDWriteFontCollection_GetFontFromFontFace(font_coll, glyphRun->fontFace,
                                                   font);
    if (FAILED(hr))
        return E_FAIL;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_DrawUnderline(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_UNDERLINE const* underline,
    IUnknown* clientDrawingEffect
    )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_DrawStrikethrough(
    IDWriteTextRenderer *This,
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown* clientDrawingEffect
    )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_DrawInlineObject(
    IDWriteTextRenderer *This,
    void *clientDrawingContext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject *inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    IUnknown *clientDrawingEffect
    )
{
    return S_OK;
}

// IUnknown methods

static ULONG STDMETHODCALLTYPE FallbackLogTextRenderer_AddRef(
    IDWriteTextRenderer *This
    )
{
    FallbackLogTextRenderer *this = (FallbackLogTextRenderer *)This;
    return InterlockedIncrement(&this->ref_count);
}

static ULONG STDMETHODCALLTYPE FallbackLogTextRenderer_Release(
    IDWriteTextRenderer *This
    )
{
    FallbackLogTextRenderer *this = (FallbackLogTextRenderer *)This;
    unsigned long new_count = InterlockedDecrement(&this->ref_count);
    if (new_count == 0) {
        free(this);
        return 0;
    }

    return new_count;
}

static HRESULT STDMETHODCALLTYPE FallbackLogTextRenderer_QueryInterface(
    IDWriteTextRenderer *This,
    REFIID riid,
    void **ppvObject
    )
{
    if (IsEqualGUID(riid, &IID_IDWriteTextRenderer)
        || IsEqualGUID(riid, &IID_IDWritePixelSnapping)
        || IsEqualGUID(riid, &IID_IUnknown)) {
        *ppvObject = This;
    } else {
        *ppvObject = NULL;
        return E_FAIL;
    }

    This->lpVtbl->AddRef(This);
    return S_OK;
}

static void init_FallbackLogTextRenderer(FallbackLogTextRenderer *r,
                                         IDWriteFactory *factory)
{
    *r = (FallbackLogTextRenderer){
        .iface = {
            .lpVtbl = &r->vtbl,
        },
        .vtbl = {
            FallbackLogTextRenderer_QueryInterface,
            FallbackLogTextRenderer_AddRef,
            FallbackLogTextRenderer_Release,
            FallbackLogTextRenderer_IsPixelSnappingDisabled,
            FallbackLogTextRenderer_GetCurrentTransform,
            FallbackLogTextRenderer_GetPixelsPerDip,
            FallbackLogTextRenderer_DrawGlyphRun,
            FallbackLogTextRenderer_DrawUnderline,
            FallbackLogTextRenderer_DrawStrikethrough,
            FallbackLogTextRenderer_DrawInlineObject,
        },
        .dw_factory = factory,
    };
}

/*
 * This function is called whenever a font is accessed for the
 * first time. It will create a FontFace for metadata access and
 * memory reading, which will be stored within the private data.
 */
static bool init_font_private_face(FontPrivate *priv)
{
    HRESULT hr;
    IDWriteFontFace *face;

    if (priv->face != NULL)
        return true;

    hr = IDWriteFont_CreateFontFace(priv->font, &face);
    if (FAILED(hr) || !face)
        return false;

    priv->face = face;
    return true;
}

/*
 * This function is called whenever a font is used for the first
 * time. It will create a FontStream for memory reading, which
 * will be stored within the private data.
 */
static bool init_font_private_stream(FontPrivate *priv)
{
    HRESULT hr = S_OK;
    IDWriteFontFile *file = NULL;
    IDWriteFontFileStream *stream = NULL;
    IDWriteFontFileLoader *loader = NULL;
    UINT32 n_files = 1;
    const void *refKey = NULL;
    UINT32 keySize = 0;

    if (priv->stream != NULL)
        return true;

    if (!init_font_private_face(priv))
        return false;

    /* DirectWrite only supports one file per face */
    hr = IDWriteFontFace_GetFiles(priv->face, &n_files, &file);
    if (FAILED(hr) || !file)
        return false;

    hr = IDWriteFontFile_GetReferenceKey(file, &refKey, &keySize);
    if (FAILED(hr)) {
        IDWriteFontFile_Release(file);
        return false;
    }

    hr = IDWriteFontFile_GetLoader(file, &loader);
    if (FAILED(hr) || !loader) {
        IDWriteFontFile_Release(file);
        return false;
    }

    hr = IDWriteFontFileLoader_CreateStreamFromKey(loader, refKey, keySize, &stream);
    if (FAILED(hr) || !stream) {
        IDWriteFontFile_Release(file);
        return false;
    }

    priv->stream = stream;
    IDWriteFontFile_Release(file);

    return true;
}

/*
 * Read a specified part of a fontfile into memory.
 * If the font wasn't used before first creates a
 * FontStream and save it into the private data for later usage.
 * If the parameter "buf" is NULL libass wants to know the
 * size of the Fontfile
 */
static size_t get_data(void *data, unsigned char *buf, size_t offset,
                       size_t length)
{
    HRESULT hr = S_OK;
    FontPrivate *priv = (FontPrivate *) data;
    const void *fileBuf = NULL;
    void *fragContext = NULL;

    if (!init_font_private_stream(priv))
        return 0;

    if (buf == NULL) {
        UINT64 fileSize;
        hr = IDWriteFontFileStream_GetFileSize(priv->stream, &fileSize);
        if (FAILED(hr))
            return 0;

        return fileSize;
    }

    hr = IDWriteFontFileStream_ReadFileFragment(priv->stream, &fileBuf, offset,
                                                length, &fragContext);

    if (FAILED(hr) || !fileBuf)
        return 0;

    memcpy(buf, fileBuf, length);

    IDWriteFontFileStream_ReleaseFileFragment(priv->stream, fragContext);

    return length;
}

/*
 * Check whether the font contains PostScript outlines.
 */
static bool check_postscript(void *data)
{
    FontPrivate *priv = (FontPrivate *) data;

    if (!init_font_private_face(priv))
        return false;

    DWRITE_FONT_FACE_TYPE type = IDWriteFontFace_GetType(priv->face);
    return type == DWRITE_FONT_FACE_TYPE_CFF ||
           type == DWRITE_FONT_FACE_TYPE_RAW_CFF ||
           type == DWRITE_FONT_FACE_TYPE_TYPE1;
}

/*
 * Lazily return index of font. It requires the FontFace to be present, which is expensive to initialize.
 */
static unsigned get_font_index(void *data)
{
    FontPrivate *priv = (FontPrivate *)data;

    if (!init_font_private_face(priv))
        return 0;

    return IDWriteFontFace_GetIndex(priv->face);
}

/*
 * Check if the passed font has a specific unicode character.
 */
static bool check_glyph(void *data, uint32_t code)
{
    HRESULT hr = S_OK;
    FontPrivate *priv = (FontPrivate *) data;
    BOOL exists = FALSE;

    if (code == 0)
        return true;

    hr = IDWriteFont_HasCharacter(priv->font, code, &exists);
    if (FAILED(hr))
        return false;

    return exists;
}

/*
 * This will release the directwrite backend
 */
static void destroy_provider(void *priv)
{
    ProviderPrivate *provider_priv = (ProviderPrivate *)priv;
    provider_priv->factory->lpVtbl->Release(provider_priv->factory);
    FreeLibrary(provider_priv->directwrite_lib);
    free(provider_priv);
}

/*
 * This will destroy a specific font and it's
 * Fontstream (in case it does exist)
 */

static void destroy_font(void *data)
{
    FontPrivate *priv = (FontPrivate *) data;

    IDWriteFont_Release(priv->font);
    if (priv->face != NULL)
        IDWriteFontFace_Release(priv->face);
    if (priv->stream != NULL)
        IDWriteFontFileStream_Release(priv->stream);

    free(priv);
}

static int encode_utf16(wchar_t *chars, uint32_t codepoint)
{
    if (codepoint < 0x10000) {
        chars[0] = codepoint;
        return 1;
    } else {
        chars[0] = (codepoint >> 10) + 0xD7C0;
        chars[1] = (codepoint & 0x3FF) + 0xDC00;
        return 2;
    }
}

static char *get_fallback(void *priv, const char *base, uint32_t codepoint)
{
    HRESULT hr;
    ProviderPrivate *provider_priv = (ProviderPrivate *)priv;
    IDWriteFactory *dw_factory = provider_priv->factory;
    IDWriteTextFormat *text_format = NULL;
    IDWriteTextLayout *text_layout = NULL;
    FallbackLogTextRenderer renderer;

    init_FallbackLogTextRenderer(&renderer, dw_factory);

    hr = IDWriteFactory_CreateTextFormat(dw_factory, FALLBACK_DEFAULT_FONT, NULL,
            DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 1.0f, L"", &text_format);
    if (FAILED(hr)) {
        return NULL;
    }

    // Encode codepoint as UTF-16
    wchar_t char_string[2];
    int char_len = encode_utf16(char_string, codepoint);

    // Create a text_layout, a high-level text rendering facility, using
    // the given codepoint and dummy format.
    hr = IDWriteFactory_CreateTextLayout(dw_factory, char_string, char_len, text_format,
        0.0f, 0.0f, &text_layout);
    if (FAILED(hr)) {
        IDWriteTextFormat_Release(text_format);
        return NULL;
    }

    // Draw the layout with a dummy renderer, which logs the
    // font used and stores it.
    IDWriteFont *font = NULL;
    hr = IDWriteTextLayout_Draw(text_layout, &font, &renderer.iface, 0.0f, 0.0f);
    if (FAILED(hr) || font == NULL) {
        IDWriteTextLayout_Release(text_layout);
        IDWriteTextFormat_Release(text_format);
        return NULL;
    }

    // We're done with these now
    IDWriteTextLayout_Release(text_layout);
    IDWriteTextFormat_Release(text_format);

    // Now, just extract the first family name
    BOOL exists = FALSE;
    IDWriteLocalizedStrings *familyNames = NULL;
    hr = IDWriteFont_GetInformationalStrings(font,
            DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES,
            &familyNames, &exists);
    if (FAILED(hr) || !exists) {
        IDWriteFont_Release(font);
        return NULL;
    }

    wchar_t temp_name[NAME_MAX_LENGTH];
    hr = IDWriteLocalizedStrings_GetString(familyNames, 0, temp_name, NAME_MAX_LENGTH);
    if (FAILED(hr)) {
        IDWriteLocalizedStrings_Release(familyNames);
        IDWriteFont_Release(font);
        return NULL;
    }
    temp_name[NAME_MAX_LENGTH-1] = 0;

    // DirectWrite may not have found a valid fallback, so check that
    // the selected font actually has the requested glyph.
    if (codepoint > 0) {
        hr = IDWriteFont_HasCharacter(font, codepoint, &exists);
        if (FAILED(hr) || !exists) {
            IDWriteLocalizedStrings_Release(familyNames);
            IDWriteFont_Release(font);
            return NULL;
        }
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, NULL, 0,NULL, NULL);
    char *family = (char *) malloc(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, family, size_needed, NULL, NULL);

    IDWriteLocalizedStrings_Release(familyNames);
    IDWriteFont_Release(font);
    return family;
}

static int map_width(enum DWRITE_FONT_STRETCH stretch)
{
    switch (stretch) {
    case DWRITE_FONT_STRETCH_ULTRA_CONDENSED: return 50;
    case DWRITE_FONT_STRETCH_EXTRA_CONDENSED: return 63;
    case DWRITE_FONT_STRETCH_CONDENSED:       return FONT_WIDTH_CONDENSED;
    case DWRITE_FONT_STRETCH_SEMI_CONDENSED:  return 88;
    case DWRITE_FONT_STRETCH_MEDIUM:          return FONT_WIDTH_NORMAL;
    case DWRITE_FONT_STRETCH_SEMI_EXPANDED:   return 113;
    case DWRITE_FONT_STRETCH_EXPANDED:        return FONT_WIDTH_EXPANDED;
    case DWRITE_FONT_STRETCH_EXTRA_EXPANDED:  return 150;
    case DWRITE_FONT_STRETCH_ULTRA_EXPANDED:  return 200;
    default:
        return FONT_WIDTH_NORMAL;
    }
}

static void add_font(IDWriteFont *font, IDWriteFontFamily *fontFamily,
                     ASS_FontProvider *provider)
{
    HRESULT hr;
    BOOL exists;
    wchar_t temp_name[NAME_MAX_LENGTH];
    int size_needed;
    ASS_FontProviderMetaData meta = {0};

    meta.weight = IDWriteFont_GetWeight(font);
    meta.width = map_width(IDWriteFont_GetStretch(font));

    DWRITE_FONT_STYLE style = IDWriteFont_GetStyle(font);
    meta.slant = (style == DWRITE_FONT_STYLE_NORMAL) ? FONT_SLANT_NONE :
                 (style == DWRITE_FONT_STYLE_OBLIQUE)? FONT_SLANT_OBLIQUE :
                 (style == DWRITE_FONT_STYLE_ITALIC) ? FONT_SLANT_ITALIC : FONT_SLANT_NONE;

    IDWriteLocalizedStrings *psNames;
    hr = IDWriteFont_GetInformationalStrings(font,
            DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &psNames, &exists);
    if (FAILED(hr))
        goto cleanup;

    if (exists) {
        hr = IDWriteLocalizedStrings_GetString(psNames, 0, temp_name, NAME_MAX_LENGTH);
        if (FAILED(hr)) {
            IDWriteLocalizedStrings_Release(psNames);
            goto cleanup;
        }

        temp_name[NAME_MAX_LENGTH-1] = 0;
        size_needed = WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, NULL, 0, NULL, NULL);
        char *mbName = (char *) malloc(size_needed);
        if (!mbName) {
            IDWriteLocalizedStrings_Release(psNames);
            goto cleanup;
        }
        WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, mbName, size_needed, NULL, NULL);
        meta.postscript_name = mbName;

        IDWriteLocalizedStrings_Release(psNames);
    }

    IDWriteLocalizedStrings *fontNames;
    hr = IDWriteFont_GetInformationalStrings(font,
            DWRITE_INFORMATIONAL_STRING_FULL_NAME, &fontNames, &exists);
    if (FAILED(hr))
        goto cleanup;

    if (exists) {
        meta.n_fullname = IDWriteLocalizedStrings_GetCount(fontNames);
        meta.fullnames = (char **) calloc(meta.n_fullname, sizeof(char *));
        if (!meta.fullnames) {
            IDWriteLocalizedStrings_Release(fontNames);
            goto cleanup;
        }
        for (int k = 0; k < meta.n_fullname; k++) {
            hr = IDWriteLocalizedStrings_GetString(fontNames, k,
                                                   temp_name,
                                                   NAME_MAX_LENGTH);
            if (FAILED(hr)) {
                IDWriteLocalizedStrings_Release(fontNames);
                goto cleanup;
            }

            temp_name[NAME_MAX_LENGTH-1] = 0;
            size_needed = WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, NULL, 0, NULL, NULL);
            char *mbName = (char *) malloc(size_needed);
            if (!mbName) {
                IDWriteLocalizedStrings_Release(fontNames);
                goto cleanup;
            }
            WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, mbName, size_needed, NULL, NULL);
            meta.fullnames[k] = mbName;
        }
        IDWriteLocalizedStrings_Release(fontNames);
    }

    IDWriteLocalizedStrings *familyNames;
    hr = IDWriteFont_GetInformationalStrings(font,
            DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &familyNames, &exists);
    if (FAILED(hr) || !exists)
        hr = IDWriteFontFamily_GetFamilyNames(fontFamily, &familyNames);
    if (FAILED(hr))
        goto cleanup;

    meta.n_family = IDWriteLocalizedStrings_GetCount(familyNames);
    meta.families = (char **) calloc(meta.n_family, sizeof(char *));
    if (!meta.families) {
        IDWriteLocalizedStrings_Release(familyNames);
        goto cleanup;
    }
    for (int k = 0; k < meta.n_family; k++) {
        hr = IDWriteLocalizedStrings_GetString(familyNames, k,
                                               temp_name,
                                               NAME_MAX_LENGTH);
        if (FAILED(hr)) {
            IDWriteLocalizedStrings_Release(familyNames);
            goto cleanup;
        }

        temp_name[NAME_MAX_LENGTH-1] = 0;
        size_needed = WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, NULL, 0, NULL, NULL);
        char *mbName = (char *) malloc(size_needed);
        if (!mbName) {
            IDWriteLocalizedStrings_Release(familyNames);
            goto cleanup;
        }
        WideCharToMultiByte(CP_UTF8, 0, temp_name, -1, mbName, size_needed, NULL, NULL);
        meta.families[k] = mbName;
    }
    IDWriteLocalizedStrings_Release(familyNames);

    FontPrivate *font_priv = (FontPrivate *) calloc(1, sizeof(*font_priv));
    if (!font_priv)
        goto cleanup;
    font_priv->font = font;
    font = NULL;

    ass_font_provider_add_font(provider, &meta, NULL, 0, font_priv);

cleanup:
    if (meta.families) {
        for (int k = 0; k < meta.n_family; k++)
            free(meta.families[k]);
        free(meta.families);
    }

    if (meta.fullnames) {
        for (int k = 0; k < meta.n_fullname; k++)
            free(meta.fullnames[k]);
        free(meta.fullnames);
    }

    free(meta.postscript_name);

    if (font)
        IDWriteFont_Release(font);
}

/*
 * Scan every system font on the current machine and add it
 * to the libass lookup. Stores the FontPrivate as private data
 * for later memory reading
 */
static void scan_fonts(IDWriteFactory *factory,
                       ASS_FontProvider *provider)
{
    HRESULT hr = S_OK;
    IDWriteFontCollection *fontCollection = NULL;
    IDWriteFont *font = NULL;
    hr = IDWriteFactory_GetSystemFontCollection(factory, &fontCollection, FALSE);

    if (FAILED(hr) || !fontCollection)
        return;

    UINT32 familyCount = IDWriteFontCollection_GetFontFamilyCount(fontCollection);

    for (UINT32 i = 0; i < familyCount; ++i) {
        IDWriteFontFamily *fontFamily = NULL;

        hr = IDWriteFontCollection_GetFontFamily(fontCollection, i, &fontFamily);
        if (FAILED(hr))
            continue;

        UINT32 fontCount = IDWriteFontFamily_GetFontCount(fontFamily);
        for (UINT32 j = 0; j < fontCount; ++j) {
            hr = IDWriteFontFamily_GetFont(fontFamily, j, &font);
            if (FAILED(hr))
                continue;

            // Simulations for bold or oblique are sometimes synthesized by
            // DirectWrite. We are only interested in physical fonts.
            if (IDWriteFont_GetSimulations(font) != 0) {
                IDWriteFont_Release(font);
                continue;
            }

            add_font(font, fontFamily, provider);
        }

        IDWriteFontFamily_Release(fontFamily);
    }

    IDWriteFontCollection_Release(fontCollection);
}

static void get_substitutions(void *priv, const char *name,
                              ASS_FontProviderMetaData *meta)
{
    const int n = sizeof(font_substitutions) / sizeof(font_substitutions[0]);
    ass_map_font(font_substitutions, n, name, meta);
}

/*
 * Called by libass when the provider should perform the
 * specified task
 */
static ASS_FontProviderFuncs directwrite_callbacks = {
    .get_data           = get_data,
    .check_postscript   = check_postscript,
    .check_glyph        = check_glyph,
    .destroy_font       = destroy_font,
    .destroy_provider   = destroy_provider,
    .get_substitutions  = get_substitutions,
    .get_fallback       = get_fallback,
    .get_font_index     = get_font_index,
};

typedef HRESULT (WINAPI *DWriteCreateFactoryFn)(
    DWRITE_FACTORY_TYPE factoryType,
    REFIID              iid,
    IUnknown            **factory
);

/*
 * Register the directwrite provider. Upon registering
 * scans all system fonts. The private data for this
 * provider is IDWriteFactory
 * On failure returns NULL
 */
ASS_FontProvider *ass_directwrite_add_provider(ASS_Library *lib,
                                               ASS_FontSelector *selector,
                                               const char *config)
{
    HRESULT hr = S_OK;
    IDWriteFactory *dwFactory = NULL;
    ASS_FontProvider *provider = NULL;
    DWriteCreateFactoryFn DWriteCreateFactoryPtr = NULL;
    ProviderPrivate *priv = NULL;

    HMODULE directwrite_lib = LoadLibraryW(L"Dwrite.dll");
    if (!directwrite_lib)
        goto cleanup;

    DWriteCreateFactoryPtr = (DWriteCreateFactoryFn)GetProcAddress(directwrite_lib,
                                                                   "DWriteCreateFactory");
    if (!DWriteCreateFactoryPtr)
        goto cleanup;

    hr = DWriteCreateFactoryPtr(DWRITE_FACTORY_TYPE_SHARED,
                                &IID_IDWriteFactory,
                                (IUnknown **) (&dwFactory));
    if (FAILED(hr) || !dwFactory) {
        ass_msg(lib, MSGL_WARN, "Failed to initialize directwrite.");
        dwFactory = NULL;
        goto cleanup;
    }

    priv = (ProviderPrivate *)calloc(sizeof(*priv), 1);
    if (!priv)
        goto cleanup;

    priv->directwrite_lib = directwrite_lib;
    priv->factory = dwFactory;
    provider = ass_font_provider_new(selector, &directwrite_callbacks, priv);
    if (!provider)
        goto cleanup;

    scan_fonts(dwFactory, provider);
    return provider;

cleanup:

    free(priv);
    if (dwFactory)
        dwFactory->lpVtbl->Release(dwFactory);
    if (directwrite_lib)
        FreeLibrary(directwrite_lib);

    return NULL;
}
