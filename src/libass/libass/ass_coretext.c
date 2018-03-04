/*
 * Copyright (C) 2013 Stefano Pigozzi <stefano.pigozzi@gmail.com>
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

#include <CoreFoundation/CoreFoundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <CoreText/CoreText.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "ass_coretext.h"

#define SAFE_CFRelease(x) do { if (x) CFRelease(x); } while(0)

static const ASS_FontMapping font_substitutions[] = {
    {"sans-serif", "Helvetica"},
    {"serif", "Times"},
    {"monospace", "Courier"}
};

static char *cfstr2buf(CFStringRef string)
{
    if (!string)
        return NULL;
    const int encoding = kCFStringEncodingUTF8;
    const char *buf_ptr = CFStringGetCStringPtr(string, encoding);
    if (buf_ptr) {
        return strdup(buf_ptr);
    } else {
        size_t len = CFStringGetLength(string);
        CFIndex buf_len = CFStringGetMaximumSizeForEncoding(len, encoding);
        char *buf = malloc(buf_len);
        CFStringGetCString(string, buf, buf_len, encoding);
        return buf;
    }
}

static void destroy_font(void *priv)
{
    CTFontDescriptorRef fontd = priv;
    SAFE_CFRelease(fontd);
}

static bool check_postscript(void *priv)
{
    CTFontDescriptorRef fontd = priv;
    CFNumberRef cfformat =
        CTFontDescriptorCopyAttribute(fontd, kCTFontFormatAttribute);
    int format;

    if (!CFNumberGetValue(cfformat, kCFNumberIntType, &format))
        return false;

    SAFE_CFRelease(cfformat);

    return format == kCTFontFormatOpenTypePostScript ||
           format == kCTFontFormatPostScript;
}

static bool check_glyph(void *priv, uint32_t code)
{
    if (code == 0)
        return true;

    CTFontDescriptorRef fontd = priv;
    CFCharacterSetRef set =
        CTFontDescriptorCopyAttribute(fontd, kCTFontCharacterSetAttribute);

    if (!set)
        return true;

    bool result = CFCharacterSetIsLongCharacterMember(set, code);
    SAFE_CFRelease(set);
    return result;
}

static char *get_font_file(CTFontDescriptorRef fontd)
{
    CFURLRef url = CTFontDescriptorCopyAttribute(fontd, kCTFontURLAttribute);
    CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    char *buffer = cfstr2buf(path);
    SAFE_CFRelease(path);
    SAFE_CFRelease(url);
    return buffer;
}

static void get_name(CTFontDescriptorRef fontd, CFStringRef attr,
                     char **array, int *idx)
{

    CFStringRef name = CTFontDescriptorCopyAttribute(fontd, attr);
    if (name) {
        array[*idx] = cfstr2buf(name);
        SAFE_CFRelease(name);
        *idx += 1;
    }
}

static void get_trait(CFDictionaryRef traits, CFStringRef attribute,
                      float *trait)
{
    CFNumberRef cftrait = CFDictionaryGetValue(traits, attribute);
    if (!CFNumberGetValue(cftrait, kCFNumberFloatType, trait))
        *trait = 0.0;
}

static void get_font_traits(CTFontDescriptorRef fontd,
                            ASS_FontProviderMetaData *meta)
{
    float weight, slant, width;

    CFDictionaryRef traits =
        CTFontDescriptorCopyAttribute(fontd, kCTFontTraitsAttribute);

    get_trait(traits, kCTFontWeightTrait, &weight);
    get_trait(traits, kCTFontSlantTrait,  &slant);
    get_trait(traits, kCTFontWidthTrait,  &width);

    SAFE_CFRelease(traits);

    // Printed all of my system fonts (see if'deffed code below). Here is how
    // CoreText 'normalized' weights maps to CSS/libass:

    // opentype:   0   100   200   300   400   500   600   700   800   900
    // css:                 LIGHT        REG   MED  SBOLD BOLD  BLACK  EXTRABL
    // libass:                   LIGHT  MEDIUM            BOLD
    // coretext:            -0.4         0.0   0.23  0.3   0.4   0.62

    if (weight >= 0.62)
        meta->weight = 800;
    else if (weight >= 0.4)
        meta->weight = 700;
    else if (weight >= 0.3)
        meta->weight = 600;
    else if (weight >= 0.23)
        meta->weight = 500;
    else if (weight >= -0.4)
        meta->weight = 400;
    else
        meta->weight = 200;

    if (slant > 0.03)
        meta->slant  = FONT_SLANT_ITALIC;
    else
        meta->slant  = FONT_SLANT_NONE;

    if (width <= -0.2)
        meta->width = FONT_WIDTH_CONDENSED;
    else if (width >= 0.2)
        meta->width = FONT_WIDTH_EXPANDED;
    else
        meta->width  = FONT_WIDTH_NORMAL;

#if 0
    char *name[1];
    int idx = 0;
    get_name(fontd, kCTFontDisplayNameAttribute, name, &idx);
    char *file = get_font_file(fontd);
    printf(
       "Font traits for: %-40s [%-50s] "
       "<slant: %f, %03d>, <weight: (%f, %03d)>, <width: %f, %03d>\n",
       name[0], file,
       slant, meta->slant, weight, meta->weight, width, meta->width);
    free(name[0]);
    free(file);
#endif
}

static void process_descriptors(ASS_FontProvider *provider, CFArrayRef fontsd)
{
    ASS_FontProviderMetaData meta;
    char *families[1];
    char *identifiers[1];
    char *fullnames[1];

    if (!fontsd)
        return;

    for (int i = 0; i < CFArrayGetCount(fontsd); i++) {
        CTFontDescriptorRef fontd = CFArrayGetValueAtIndex(fontsd, i);
        int index = -1;

        char *path = get_font_file(fontd);
        if (strcmp("", path) == 0) {
            // skip the font if the URL field in the font descriptor is empty
            free(path);
            continue;
        }

        memset(&meta, 0, sizeof(meta));
        get_font_traits(fontd, &meta);

        get_name(fontd, kCTFontFamilyNameAttribute, families, &meta.n_family);
        meta.families = families;

        get_name(fontd, kCTFontDisplayNameAttribute, fullnames, &meta.n_fullname);
        meta.fullnames = fullnames;

        int zero = 0;
        get_name(fontd, kCTFontNameAttribute, identifiers, &zero);
        meta.postscript_name = identifiers[0];

        CFRetain(fontd);
        ass_font_provider_add_font(provider, &meta, path, index, (void*)fontd);

        for (int j = 0; j < meta.n_family; j++)
            free(meta.families[j]);

        for (int j = 0; j < meta.n_fullname; j++)
            free(meta.fullnames[j]);

        free(meta.postscript_name);

        free(path);
    }
}

static void match_fonts(ASS_Library *lib, ASS_FontProvider *provider,
                        char *name)
{
    enum { attributes_n = 3 };
    CTFontDescriptorRef ctdescrs[attributes_n];
    CFMutableDictionaryRef cfattrs[attributes_n];
    CFStringRef attributes[attributes_n] = {
        kCTFontFamilyNameAttribute,
        kCTFontDisplayNameAttribute,
        kCTFontNameAttribute,
    };

    CFStringRef cfname =
        CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);

    for (int i = 0; i < attributes_n; i++) {
        cfattrs[i] = CFDictionaryCreateMutable(NULL, 0, 0, 0);
        CFDictionaryAddValue(cfattrs[i], attributes[i], cfname);
        ctdescrs[i] = CTFontDescriptorCreateWithAttributes(cfattrs[i]);
    }

    CFArrayRef descriptors =
        CFArrayCreate(NULL, (const void **)&ctdescrs, attributes_n, NULL);

    CTFontCollectionRef ctcoll =
        CTFontCollectionCreateWithFontDescriptors(descriptors, 0);

    CFArrayRef fontsd =
        CTFontCollectionCreateMatchingFontDescriptors(ctcoll);

    process_descriptors(provider, fontsd);

    SAFE_CFRelease(fontsd);
    SAFE_CFRelease(ctcoll);

    for (int i = 0; i < attributes_n; i++) {
        SAFE_CFRelease(cfattrs[i]);
        SAFE_CFRelease(ctdescrs[i]);
    }

    SAFE_CFRelease(descriptors);
    SAFE_CFRelease(cfname);
}

static char *get_fallback(void *priv, const char *family, uint32_t codepoint)
{
    CFStringRef name = CFStringCreateWithBytes(
        0, (UInt8 *)family, strlen(family), kCFStringEncodingUTF8, false);
    CTFontRef font = CTFontCreateWithName(name, 0, NULL);
    uint32_t codepointle = OSSwapHostToLittleInt32(codepoint);
    CFStringRef r = CFStringCreateWithBytes(
        0, (UInt8*)&codepointle, sizeof(codepointle),
        kCFStringEncodingUTF32LE, false);
    CTFontRef fb = CTFontCreateForString(font, r, CFRangeMake(0, 1));
    CFStringRef cffamily = CTFontCopyFamilyName(fb);
    char *res_family = cfstr2buf(cffamily);

    SAFE_CFRelease(name);
    SAFE_CFRelease(font);
    SAFE_CFRelease(r);
    SAFE_CFRelease(fb);
    SAFE_CFRelease(cffamily);

    return res_family;
}

static void get_substitutions(void *priv, const char *name,
                              ASS_FontProviderMetaData *meta)
{
    const int n = sizeof(font_substitutions) / sizeof(font_substitutions[0]);
    ass_map_font(font_substitutions, n, name, meta);
}

static ASS_FontProviderFuncs coretext_callbacks = {
    .check_postscript   = check_postscript,
    .check_glyph        = check_glyph,
    .destroy_font       = destroy_font,
    .match_fonts        = match_fonts,
    .get_substitutions  = get_substitutions,
    .get_fallback       = get_fallback,
};

ASS_FontProvider *
ass_coretext_add_provider(ASS_Library *lib, ASS_FontSelector *selector,
                          const char *config)
{
    return ass_font_provider_new(selector, &coretext_callbacks, NULL);
}
