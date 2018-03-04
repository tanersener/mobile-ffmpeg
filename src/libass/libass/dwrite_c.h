/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/**
 * Stripped version. Only definitions needed by libass. Contains fixes to
 * make it compile with C. Also needed on MSVC.
 */
#ifndef __INC_DWRITE__
#define __INC_DWRITE__

#define DWRITEAPI DECLSPEC_IMPORT

#include <unknwn.h>

typedef struct IDWriteFactory IDWriteFactory;
typedef struct IDWriteFont IDWriteFont;
typedef struct IDWriteFontCollection IDWriteFontCollection;
typedef struct IDWriteFontFace IDWriteFontFace;
typedef struct IDWriteFontFamily IDWriteFontFamily;
typedef struct IDWriteFontList IDWriteFontList;
typedef struct IDWriteFontFile IDWriteFontFile;
typedef struct IDWriteFontFileLoader IDWriteFontFileLoader;
typedef struct IDWriteFontFileStream IDWriteFontFileStream;
typedef struct IDWriteInlineObject IDWriteInlineObject;
typedef struct IDWriteLocalizedStrings IDWriteLocalizedStrings;
typedef struct IDWritePixelSnapping IDWritePixelSnapping;
typedef struct IDWriteTextFormat IDWriteTextFormat;
typedef struct IDWriteTextLayout IDWriteTextLayout;
typedef struct IDWriteTextRenderer IDWriteTextRenderer;

#include <dcommon.h>

typedef enum DWRITE_INFORMATIONAL_STRING_ID {
  DWRITE_INFORMATIONAL_STRING_NONE = 0,
  DWRITE_INFORMATIONAL_STRING_COPYRIGHT_NOTICE,
  DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS,
  DWRITE_INFORMATIONAL_STRING_TRADEMARK,
  DWRITE_INFORMATIONAL_STRING_MANUFACTURER,
  DWRITE_INFORMATIONAL_STRING_DESIGNER,
  DWRITE_INFORMATIONAL_STRING_DESIGNER_URL,
  DWRITE_INFORMATIONAL_STRING_DESCRIPTION,
  DWRITE_INFORMATIONAL_STRING_FONT_VENDOR_URL,
  DWRITE_INFORMATIONAL_STRING_LICENSE_DESCRIPTION,
  DWRITE_INFORMATIONAL_STRING_LICENSE_INFO_URL,
  DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES,
  DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES,
  DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES,
  DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES,
  DWRITE_INFORMATIONAL_STRING_SAMPLE_TEXT,
  DWRITE_INFORMATIONAL_STRING_FULL_NAME,
  DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,
  DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME
} DWRITE_INFORMATIONAL_STRING_ID;

typedef enum DWRITE_FACTORY_TYPE {
  DWRITE_FACTORY_TYPE_SHARED = 0,
  DWRITE_FACTORY_TYPE_ISOLATED 
} DWRITE_FACTORY_TYPE;

typedef enum DWRITE_FONT_FACE_TYPE {
  DWRITE_FONT_FACE_TYPE_CFF = 0,
  DWRITE_FONT_FACE_TYPE_TRUETYPE,
  DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION,
  DWRITE_FONT_FACE_TYPE_TYPE1,
  DWRITE_FONT_FACE_TYPE_VECTOR,
  DWRITE_FONT_FACE_TYPE_BITMAP,
  DWRITE_FONT_FACE_TYPE_UNKNOWN,
  DWRITE_FONT_FACE_TYPE_RAW_CFF
} DWRITE_FONT_FACE_TYPE;

typedef enum DWRITE_FONT_SIMULATIONS {
  DWRITE_FONT_SIMULATIONS_NONE      = 0x0000,
  DWRITE_FONT_SIMULATIONS_BOLD      = 0x0001,
  DWRITE_FONT_SIMULATIONS_OBLIQUE   = 0x0002 
} DWRITE_FONT_SIMULATIONS;

typedef enum DWRITE_FONT_STRETCH {
  DWRITE_FONT_STRETCH_UNDEFINED         = 0,
  DWRITE_FONT_STRETCH_ULTRA_CONDENSED   = 1,
  DWRITE_FONT_STRETCH_EXTRA_CONDENSED   = 2,
  DWRITE_FONT_STRETCH_CONDENSED         = 3,
  DWRITE_FONT_STRETCH_SEMI_CONDENSED    = 4,
  DWRITE_FONT_STRETCH_NORMAL            = 5,
  DWRITE_FONT_STRETCH_MEDIUM            = 5,
  DWRITE_FONT_STRETCH_SEMI_EXPANDED     = 6,
  DWRITE_FONT_STRETCH_EXPANDED          = 7,
  DWRITE_FONT_STRETCH_EXTRA_EXPANDED    = 8,
  DWRITE_FONT_STRETCH_ULTRA_EXPANDED    = 9 
} DWRITE_FONT_STRETCH;

typedef enum DWRITE_FONT_STYLE {
  DWRITE_FONT_STYLE_NORMAL = 0,
  DWRITE_FONT_STYLE_OBLIQUE,
  DWRITE_FONT_STYLE_ITALIC 
} DWRITE_FONT_STYLE;

typedef enum DWRITE_FONT_WEIGHT {
  DWRITE_FONT_WEIGHT_MEDIUM        = 500,
  /* rest dropped */
} DWRITE_FONT_WEIGHT;

typedef struct DWRITE_FONT_METRICS {
  UINT16 designUnitsPerEm;
  UINT16 ascent;
  UINT16 descent;
  INT16  lineGap;
  UINT16 capHeight;
  UINT16 xHeight;
  INT16  underlinePosition;
  UINT16 underlineThickness;
  INT16  strikethroughPosition;
  UINT16 strikethroughThickness;
} DWRITE_FONT_METRICS;

typedef struct DWRITE_GLYPH_OFFSET DWRITE_GLYPH_OFFSET;

typedef struct DWRITE_GLYPH_RUN {
  IDWriteFontFace           *fontFace;
  FLOAT                     fontEmSize;
  UINT32                    glyphCount;
  const UINT16              *glyphIndices;
  const FLOAT               *glyphAdvances;
  const DWRITE_GLYPH_OFFSET *glyphOffsets;
  BOOL                      isSideways;
  UINT32                    bidiLevel;
} DWRITE_GLYPH_RUN;

typedef struct DWRITE_GLYPH_RUN_DESCRIPTION DWRITE_GLYPH_RUN_DESCRIPTION;
typedef struct DWRITE_HIT_TEST_METRICS DWRITE_HIT_TEST_METRICS;
typedef struct DWRITE_LINE_METRICS DWRITE_LINE_METRICS;
typedef struct DWRITE_MATRIX DWRITE_MATRIX;
typedef struct DWRITE_STRIKETHROUGH DWRITE_STRIKETHROUGH;
typedef struct DWRITE_TEXT_METRICS DWRITE_TEXT_METRICS;

typedef struct DWRITE_TEXT_RANGE {
  UINT32 startPosition;
  UINT32 length;
} DWRITE_TEXT_RANGE;

typedef struct DWRITE_TRIMMING DWRITE_TRIMMING;
typedef struct DWRITE_UNDERLINE DWRITE_UNDERLINE;

#ifndef __MINGW_DEF_ARG_VAL
#ifdef __cplusplus
#define __MINGW_DEF_ARG_VAL(x) = x
#else
#define __MINGW_DEF_ARG_VAL(x)
#endif
#endif

#undef  INTERFACE
#define INTERFACE IDWriteFactory
DECLARE_INTERFACE_(IDWriteFactory,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFactory methods */
    STDMETHOD(GetSystemFontCollection)(THIS_
        IDWriteFontCollection **fontCollection,
        BOOL checkForUpdates __MINGW_DEF_ARG_VAL(FALSE)) PURE;

    STDMETHOD(dummy1)(THIS);
    STDMETHOD(dummy2)(THIS);
    STDMETHOD(dummy3)(THIS);
    STDMETHOD(dummy4)(THIS);
    STDMETHOD(dummy5)(THIS);
    STDMETHOD(dummy6)(THIS);
    STDMETHOD(dummy7)(THIS);
    STDMETHOD(dummy8)(THIS);
    STDMETHOD(dummy9)(THIS);
    STDMETHOD(dummy10)(THIS);
    STDMETHOD(dummy11)(THIS);

    STDMETHOD(CreateTextFormat)(THIS_
        WCHAR const *fontFamilyName,
        IDWriteFontCollection *fontCollection,
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_FONT_STYLE fontStyle,
        DWRITE_FONT_STRETCH fontStretch,
        FLOAT fontSize,
        WCHAR const *localeName,
        IDWriteTextFormat **textFormat) PURE;

    STDMETHOD(dummy12)(THIS);
    STDMETHOD(dummy13)(THIS);

    STDMETHOD(CreateTextLayout)(THIS_
        WCHAR const *string,
        UINT32 stringLength,
        IDWriteTextFormat *textFormat,
        FLOAT maxWidth,
        FLOAT maxHeight,
        IDWriteTextLayout **textLayout) PURE;

    /* remainder dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFactory_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFactory_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFactory_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFactory_GetSystemFontCollection(This,fontCollection,checkForUpdates) (This)->lpVtbl->GetSystemFontCollection(This,fontCollection,checkForUpdates)
#define IDWriteFactory_CreateTextFormat(This,fontFamilyName,fontCollection,fontWeight,fontStyle,fontStretch,fontSize,localeName,textFormat) (This)->lpVtbl->CreateTextFormat(This,fontFamilyName,fontCollection,fontWeight,fontStyle,fontStretch,fontSize,localeName,textFormat)
#define IDWriteFactory_CreateTextLayout(This,string,stringLength,textFormat,maxWidth,maxHeight,textLayout) (This)->lpVtbl->CreateTextLayout(This,string,stringLength,textFormat,maxWidth,maxHeight,textLayout)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFont
DECLARE_INTERFACE_(IDWriteFont,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFont methods */
    STDMETHOD(GetFontFamily)(THIS_
        IDWriteFontFamily **fontFamily) PURE;

    STDMETHOD_(DWRITE_FONT_WEIGHT, GetWeight)(THIS) PURE;
    STDMETHOD_(DWRITE_FONT_STRETCH, GetStretch)(THIS) PURE;
    STDMETHOD_(DWRITE_FONT_STYLE, GetStyle)(THIS) PURE;
    STDMETHOD_(BOOL, IsSymbolFont)(THIS) PURE;

    STDMETHOD(GetFaceNames)(THIS_
        IDWriteLocalizedStrings **names) PURE;

    STDMETHOD(GetInformationalStrings)(THIS_
        DWRITE_INFORMATIONAL_STRING_ID informationalStringID,
        IDWriteLocalizedStrings **informationalStrings,
        BOOL *exists) PURE;

    STDMETHOD_(DWRITE_FONT_SIMULATIONS, GetSimulations)(THIS) PURE;

    STDMETHOD_(void, GetMetrics)(THIS_
        DWRITE_FONT_METRICS *fontMetrics) PURE;

    STDMETHOD(HasCharacter)(THIS_
        UINT32 unicodeValue,
        BOOL *exists) PURE;

    STDMETHOD(CreateFontFace)(THIS_
        IDWriteFontFace **fontFace) PURE;

    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFont_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFont_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFont_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFont_CreateFontFace(This,fontFace) (This)->lpVtbl->CreateFontFace(This,fontFace)
#define IDWriteFont_GetFaceNames(This,names) (This)->lpVtbl->GetFaceNames(This,names)
#define IDWriteFont_GetFontFamily(This,fontFamily) (This)->lpVtbl->GetFontFamily(This,fontFamily)
#define IDWriteFont_GetInformationalStrings(This,informationalStringID,informationalStrings,exists) (This)->lpVtbl->GetInformationalStrings(This,informationalStringID,informationalStrings,exists)
#define IDWriteFont_GetMetrics(This,fontMetrics) (This)->lpVtbl->GetMetrics(This,fontMetrics)
#define IDWriteFont_GetSimulations(This) (This)->lpVtbl->GetSimulations(This)
#define IDWriteFont_GetStretch(This) (This)->lpVtbl->GetStretch(This)
#define IDWriteFont_GetStyle(This) (This)->lpVtbl->GetStyle(This)
#define IDWriteFont_GetWeight(This) (This)->lpVtbl->GetWeight(This)
#define IDWriteFont_HasCharacter(This,unicodeValue,exists) (This)->lpVtbl->HasCharacter(This,unicodeValue,exists)
#define IDWriteFont_IsSymbolFont(This) (This)->lpVtbl->IsSymbolFont(This)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontCollection
DECLARE_INTERFACE_(IDWriteFontCollection,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFontCollection methods */
    STDMETHOD_(UINT32, GetFontFamilyCount)(THIS) PURE;

    STDMETHOD(GetFontFamily)(THIS_
        UINT32 index,
        IDWriteFontFamily **fontFamily) PURE;

    STDMETHOD(FindFamilyName)(THIS_
        WCHAR const *familyName,
        UINT32 *index,
        BOOL *exists) PURE;

    STDMETHOD(GetFontFromFontFace)(THIS_
        IDWriteFontFace* fontFace,
        IDWriteFont **font) PURE;

    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontCollection_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFontCollection_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFontCollection_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontCollection_FindFamilyName(This,familyName,index,exists) (This)->lpVtbl->FindFamilyName(This,familyName,index,exists)
#define IDWriteFontCollection_GetFontFamily(This,index,fontFamily) (This)->lpVtbl->GetFontFamily(This,index,fontFamily)
#define IDWriteFontCollection_GetFontFamilyCount(This) (This)->lpVtbl->GetFontFamilyCount(This)
#define IDWriteFontCollection_GetFontFromFontFace(This,fontFace,font) (This)->lpVtbl->GetFontFromFontFace(This,fontFace,font)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontFace
DECLARE_INTERFACE_(IDWriteFontFace,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFontFace methods */
    STDMETHOD_(DWRITE_FONT_FACE_TYPE, GetType)(THIS) PURE;

    STDMETHOD(GetFiles)(THIS_
        UINT32 *numberOfFiles,
        IDWriteFontFile **fontFiles) PURE;

    STDMETHOD_(UINT32, GetIndex)(THIS) PURE;

    /* rest dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontFace_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontFace_GetType(This) (This)->lpVtbl->GetType(This)
#define IDWriteFontFace_GetFiles(This,fontFiles,b) (This)->lpVtbl->GetFiles(This,fontFiles,b)
#define IDWriteFontFace_GetIndex(This) (This)->lpVtbl->GetIndex(This)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontFamily
DECLARE_INTERFACE_(IDWriteFontFamily,IDWriteFontList)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDWriteFontList methods */
    STDMETHOD(GetFontCollection)(THIS_
        IDWriteFontCollection** fontCollection) PURE;

    STDMETHOD_(UINT32, GetFontCount)(THIS) PURE;

    STDMETHOD(GetFont)(THIS_
        UINT32 index,
        IDWriteFont **font) PURE;
#endif

    /* IDWriteFontFamily methods */
    STDMETHOD(GetFamilyNames)(THIS_
        IDWriteLocalizedStrings **names) PURE;

    /* rest dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontFamily_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFontFamily_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFontFamily_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontFamily_GetFont(This,index,font) (This)->lpVtbl->GetFont(This,index,font)
#define IDWriteFontFamily_GetFontCount(This) (This)->lpVtbl->GetFontCount(This)
#define IDWriteFontFamily_GetFamilyNames(This,names) (This)->lpVtbl->GetFamilyNames(This,names)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontFile
DECLARE_INTERFACE_(IDWriteFontFile,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFontFile methods */
    STDMETHOD(GetReferenceKey)(THIS_
        void const **fontFileReferenceKey,
        UINT32 *fontFileReferenceKeySize) PURE;

    STDMETHOD(GetLoader)(THIS_
        IDWriteFontFileLoader **fontFileLoader) PURE;

    /* rest dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontFile_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontFile_GetLoader(This,fontFileLoader) (This)->lpVtbl->GetLoader(This,fontFileLoader)
#define IDWriteFontFile_GetReferenceKey(This,fontFileReferenceKey,fontFileReferenceKeySize) (This)->lpVtbl->GetReferenceKey(This,fontFileReferenceKey,fontFileReferenceKeySize)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontFileLoader
DECLARE_INTERFACE_(IDWriteFontFileLoader,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFontFileLoader methods */
    STDMETHOD(CreateStreamFromKey)(THIS_
        void const *fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        IDWriteFontFileStream **fontFileStream) PURE;

    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontFileLoader_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFontFileLoader_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFontFileLoader_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontFileLoader_CreateStreamFromKey(This,fontFileReferenceKey,fontFileReferenceKeySize,fontFileStream) (This)->lpVtbl->CreateStreamFromKey(This,fontFileReferenceKey,fontFileReferenceKeySize,fontFileStream)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteFontFileStream
DECLARE_INTERFACE_(IDWriteFontFileStream,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteFontFileStream methods */
    STDMETHOD(ReadFileFragment)(THIS_
        void const **fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        void** fragmentContext) PURE;

    STDMETHOD_(void, ReleaseFileFragment)(THIS_
        void *fragmentContext) PURE;

    STDMETHOD(GetFileSize)(THIS_
        UINT64 *fileSize) PURE;

    STDMETHOD(GetLastWriteTime)(THIS_
        UINT64 *lastWriteTime) PURE;

    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteFontFileStream_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDWriteFontFileStream_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDWriteFontFileStream_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteFontFileStream_GetFileSize(This,fileSize) (This)->lpVtbl->GetFileSize(This,fileSize)
#define IDWriteFontFileStream_ReadFileFragment(This,fragmentStart,fileOffset,fragmentSize,fragmentContext) (This)->lpVtbl->ReadFileFragment(This,fragmentStart,fileOffset,fragmentSize,fragmentContext)
#define IDWriteFontFileStream_ReleaseFileFragment(This,fragmentContext) (This)->lpVtbl->ReleaseFileFragment(This,fragmentContext)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteLocalizedStrings
DECLARE_INTERFACE_(IDWriteLocalizedStrings,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteLocalizedStrings methods */
    STDMETHOD_(UINT32, GetCount)(THIS) PURE;

    STDMETHOD(dummy1)(THIS);
    STDMETHOD(dummy2)(THIS);
    STDMETHOD(dummy3)(THIS);
    STDMETHOD(dummy4)(THIS);

    STDMETHOD(GetString)(THIS_
        UINT32 index,
        WCHAR *stringBuffer,
        UINT32 size) PURE;

    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteLocalizedStrings_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteLocalizedStrings_GetCount(This) (This)->lpVtbl->GetCount(This)
#define IDWriteLocalizedStrings_GetString(This,index,stringBuffer,size) (This)->lpVtbl->GetString(This,index,stringBuffer,size)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteTextFormat
DECLARE_INTERFACE_(IDWriteTextFormat,IUnknown)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDWriteTextFormat methods */
    /* rest dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteTextFormat_Release(This) (This)->lpVtbl->Release(This)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteTextLayout
DECLARE_INTERFACE_(IDWriteTextLayout,IDWriteTextFormat)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDWriteTextFormat methods */
    STDMETHOD(dummy1)(THIS);
    STDMETHOD(dummy2)(THIS);
    STDMETHOD(dummy3)(THIS);
    STDMETHOD(dummy4)(THIS);
    STDMETHOD(dummy5)(THIS);
    STDMETHOD(dummy6)(THIS);
    STDMETHOD(dummy7)(THIS);
    STDMETHOD(dummy8)(THIS);
    STDMETHOD(dummy9)(THIS);
    STDMETHOD(dummy10)(THIS);
    STDMETHOD(dummy11)(THIS);
    STDMETHOD(dummy12)(THIS);
    STDMETHOD(dummy13)(THIS);
    STDMETHOD(dummy14)(THIS);
    STDMETHOD(dummy15)(THIS);
    STDMETHOD(dummy16)(THIS);
    STDMETHOD(dummy17)(THIS);
    STDMETHOD(dummy18)(THIS);
    STDMETHOD(dummy19)(THIS);
    STDMETHOD(dummy20)(THIS);
    STDMETHOD(dummy21)(THIS);
    STDMETHOD(dummy22)(THIS);
    STDMETHOD(dummy23)(THIS);
    STDMETHOD(dummy24)(THIS);
    STDMETHOD(dummy25)(THIS);
#endif

    /* IDWriteTextLayout methods */
    STDMETHOD(dummy26)(THIS);
    STDMETHOD(dummy27)(THIS);
    STDMETHOD(dummy28)(THIS);
    STDMETHOD(dummy29)(THIS);
    STDMETHOD(dummy30)(THIS);
    STDMETHOD(dummy31)(THIS);
    STDMETHOD(dummy32)(THIS);
    STDMETHOD(dummy33)(THIS);
    STDMETHOD(dummy34)(THIS);
    STDMETHOD(dummy35)(THIS);
    STDMETHOD(dummy36)(THIS);
    STDMETHOD(dummy37)(THIS);
    STDMETHOD(dummy38)(THIS);
    STDMETHOD(dummy39)(THIS);
    STDMETHOD(dummy40)(THIS);
    STDMETHOD(dummy41)(THIS);
    STDMETHOD(dummy42)(THIS);
    STDMETHOD(dummy43)(THIS);
    STDMETHOD(dummy44)(THIS);
    STDMETHOD(dummy45)(THIS);
    STDMETHOD(dummy46)(THIS);
    STDMETHOD(dummy47)(THIS);
    STDMETHOD(dummy48)(THIS);
    STDMETHOD(dummy49)(THIS);
    STDMETHOD(dummy50)(THIS);
    STDMETHOD(dummy51)(THIS);
    STDMETHOD(dummy52)(THIS);
    STDMETHOD(dummy53)(THIS);
    STDMETHOD(dummy54)(THIS);
    STDMETHOD(dummy55)(THIS);
    STDMETHOD(Draw)(THIS_
            void *clientDrawingContext,
            IDWriteTextRenderer *renderer,
            FLOAT originX,
            FLOAT originY) PURE;
    /* rest dropped */
    END_INTERFACE
};
#ifdef COBJMACROS
#define IDWriteTextLayout_Release(This) (This)->lpVtbl->Release(This)
#define IDWriteTextLayout_Draw(This,clientDrawingContext,renderer,originX,originY) (This)->lpVtbl->Draw(This,clientDrawingContext,renderer,originX,originY)
#endif /*COBJMACROS*/

#undef  INTERFACE
#define INTERFACE IDWriteTextRenderer
DECLARE_INTERFACE_(IDWriteTextRenderer,IDWritePixelSnapping)
{
    BEGIN_INTERFACE

#ifndef __cplusplus
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDWritePixelSnapping methods */
    STDMETHOD(IsPixelSnappingDisabled)(THIS_
            void *clientDrawingContext,
            BOOL *isDisabled) PURE;
    STDMETHOD(GetCurrentTransform)(THIS_
            void *clientDrawingContext,
            DWRITE_MATRIX *transform) PURE;
    STDMETHOD(GetPixelsPerDip)(THIS_
            void *clientDrawingContext,
            FLOAT *pixelsPerDip) PURE;
#endif

    /* IDWriteTextRenderer methods */
    STDMETHOD(DrawGlyphRun)(THIS_
            void *clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE measuringMode,
            DWRITE_GLYPH_RUN const *glyphRun,
            DWRITE_GLYPH_RUN_DESCRIPTION const *glyphRunDescription,
            IUnknown* clientDrawingEffect) PURE;
    STDMETHOD(DrawUnderline)(THIS_
            void *clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_UNDERLINE const *underline,
            IUnknown *clientDrawingEffect) PURE;
    STDMETHOD(DrawStrikethrough)(THIS_
            void *clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_STRIKETHROUGH const *strikethrough,
            IUnknown* clientDrawingEffect) PURE;
    STDMETHOD(DrawInlineObject)(THIS_
            void *clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject *inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown *clientDrawingEffect) PURE;

    END_INTERFACE
};

DEFINE_GUID(IID_IDWriteFactory, 0xb859ee5a,0xd838,0x4b5b,0xa2,0xe8,0x1a,0xdc,0x7d,0x93,0xdb,0x48);
DEFINE_GUID(IID_IDWritePixelSnapping, 0xeaf3a2da,0xecf4,0x4d24,0xb6,0x44,0xb3,0x4f,0x68,0x42,0x02,0x4b);
DEFINE_GUID(IID_IDWriteTextRenderer, 0xef8a8135,0x5cc6,0x45fe,0x88,0x25,0xc5,0xa0,0x72,0x4e,0xb8,0x19);

#endif /* __INC_DWRITE__ */
