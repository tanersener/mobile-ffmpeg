/*
 * fontconfig/src/fccfg.c
 *
 * Copyright © 2000 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the author(s) not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Objects MT-safe for readonly access. */

#include "fcint.h"
#include <dirent.h>
#include <sys/types.h>

#if defined (_WIN32) && !defined (R_OK)
#define R_OK 4
#endif

static FcConfig    *_fcConfig; /* MT-safe */

static FcConfig *
FcConfigEnsure (void)
{
    FcConfig	*config;
retry:
    config = fc_atomic_ptr_get (&_fcConfig);
    if (!config)
    {
	config = FcInitLoadConfigAndFonts ();

	if (!fc_atomic_ptr_cmpexch (&_fcConfig, NULL, config)) {
	    FcConfigDestroy (config);
	    goto retry;
	}
    }
    return config;
}

static void
FcDestroyAsRule (void *data)
{
    FcRuleDestroy (data);
}

static void
FcDestroyAsRuleSet (void *data)
{
    FcRuleSetDestroy (data);
}

FcBool
FcConfigInit (void)
{
  return FcConfigEnsure () ? FcTrue : FcFalse;
}

void
FcConfigFini (void)
{
    FcConfig *cfg = fc_atomic_ptr_get (&_fcConfig);
    if (cfg && fc_atomic_ptr_cmpexch (&_fcConfig, cfg, NULL))
	FcConfigDestroy (cfg);
}

static FcChar8 *
FcConfigRealPath(const FcChar8 *path)
{
    char	resolved_name[PATH_MAX+1];
    char	*resolved_ret;

    if (!path)
	return NULL;

#ifndef _WIN32
    resolved_ret = realpath((const char *) path, resolved_name);
#else
    if (GetFullPathNameA ((LPCSTR) path, PATH_MAX, resolved_name, NULL) == 0)
    {
        fprintf (stderr, "Fontconfig warning: GetFullPathNameA failed.\n");
        return NULL;
    }
    resolved_ret = resolved_name;
#endif
    if (resolved_ret)
	path = (FcChar8 *) resolved_ret;
    return FcStrCopyFilename(path);
}

FcConfig *
FcConfigCreate (void)
{
    FcSetName	set;
    FcConfig	*config;
    FcMatchKind	k;
    FcBool	err = FcFalse;

    config = malloc (sizeof (FcConfig));
    if (!config)
	goto bail0;

    config->configDirs = FcStrSetCreate ();
    if (!config->configDirs)
	goto bail1;

    config->configMapDirs = FcStrSetCreate();
    if (!config->configMapDirs)
	goto bail1_5;

    config->configFiles = FcStrSetCreate ();
    if (!config->configFiles)
	goto bail2;

    config->fontDirs = FcStrSetCreate ();
    if (!config->fontDirs)
	goto bail3;

    config->acceptGlobs = FcStrSetCreate ();
    if (!config->acceptGlobs)
	goto bail4;

    config->rejectGlobs = FcStrSetCreate ();
    if (!config->rejectGlobs)
	goto bail5;

    config->acceptPatterns = FcFontSetCreate ();
    if (!config->acceptPatterns)
	goto bail6;

    config->rejectPatterns = FcFontSetCreate ();
    if (!config->rejectPatterns)
	goto bail7;

    config->cacheDirs = FcStrSetCreate ();
    if (!config->cacheDirs)
	goto bail8;

    for (k = FcMatchKindBegin; k < FcMatchKindEnd; k++)
    {
	config->subst[k] = FcPtrListCreate (FcDestroyAsRuleSet);
	if (!config->subst[k])
	    err = FcTrue;
    }
    if (err)
	goto bail9;

    config->maxObjects = 0;
    for (set = FcSetSystem; set <= FcSetApplication; set++)
	config->fonts[set] = 0;

    config->rescanTime = time(0);
    config->rescanInterval = 30;

    config->expr_pool = NULL;

    config->sysRoot = FcConfigRealPath((const FcChar8 *) getenv("FONTCONFIG_SYSROOT"));

    config->rulesetList = FcPtrListCreate (FcDestroyAsRuleSet);
    if (!config->rulesetList)
	goto bail9;
    config->availConfigFiles = FcStrSetCreate ();
    if (!config->availConfigFiles)
	goto bail10;

    FcRefInit (&config->ref, 1);

    return config;

bail10:
    FcPtrListDestroy (config->rulesetList);
bail9:
    for (k = FcMatchKindBegin; k < FcMatchKindEnd; k++)
	if (config->subst[k])
	    FcPtrListDestroy (config->subst[k]);
    FcStrSetDestroy (config->cacheDirs);
bail8:
    FcFontSetDestroy (config->rejectPatterns);
bail7:
    FcFontSetDestroy (config->acceptPatterns);
bail6:
    FcStrSetDestroy (config->rejectGlobs);
bail5:
    FcStrSetDestroy (config->acceptGlobs);
bail4:
    FcStrSetDestroy (config->fontDirs);
bail3:
    FcStrSetDestroy (config->configFiles);
bail2:
    FcStrSetDestroy (config->configMapDirs);
bail1_5:
    FcStrSetDestroy (config->configDirs);
bail1:
    free (config);
bail0:
    return 0;
}

static FcFileTime
FcConfigNewestFile (FcStrSet *files)
{
    FcStrList	    *list = FcStrListCreate (files);
    FcFileTime	    newest = { 0, FcFalse };
    FcChar8	    *file;
    struct  stat    statb;

    if (list)
    {
	while ((file = FcStrListNext (list)))
	    if (FcStat (file, &statb) == 0)
		if (!newest.set || statb.st_mtime - newest.time > 0)
		{
		    newest.set = FcTrue;
		    newest.time = statb.st_mtime;
		}
	FcStrListDone (list);
    }
    return newest;
}

FcBool
FcConfigUptoDate (FcConfig *config)
{
    FcFileTime	config_time, config_dir_time, font_time;
    time_t	now = time(0);
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }
    config_time = FcConfigNewestFile (config->configFiles);
    config_dir_time = FcConfigNewestFile (config->configDirs);
    font_time = FcConfigNewestFile (config->fontDirs);
    if ((config_time.set && config_time.time - config->rescanTime > 0) ||
	(config_dir_time.set && (config_dir_time.time - config->rescanTime) > 0) ||
	(font_time.set && (font_time.time - config->rescanTime) > 0))
    {
	/* We need to check for potential clock problems here (OLPC ticket #6046) */
	if ((config_time.set && (config_time.time - now) > 0) ||
    	(config_dir_time.set && (config_dir_time.time - now) > 0) ||
        (font_time.set && (font_time.time - now) > 0))
	{
	    fprintf (stderr,
                    "Fontconfig warning: Directory/file mtime in the future. New fonts may not be detected.\n");
	    config->rescanTime = now;
	    return FcTrue;
	}
	else
	    return FcFalse;
    }
    config->rescanTime = now;
    return FcTrue;
}

FcExpr *
FcConfigAllocExpr (FcConfig *config)
{
    if (!config->expr_pool || config->expr_pool->next == config->expr_pool->end)
    {
	FcExprPage *new_page;

	new_page = malloc (sizeof (FcExprPage));
	if (!new_page)
	    return 0;

	new_page->next_page = config->expr_pool;
	new_page->next = new_page->exprs;
	config->expr_pool = new_page;
    }

    return config->expr_pool->next++;
}

FcConfig *
FcConfigReference (FcConfig *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }

    FcRefInc (&config->ref);

    return config;
}

void
FcConfigDestroy (FcConfig *config)
{
    FcSetName	set;
    FcExprPage	*page;
    FcMatchKind	k;

    if (FcRefDec (&config->ref) != 1)
	return;

    (void) fc_atomic_ptr_cmpexch (&_fcConfig, config, NULL);

    FcStrSetDestroy (config->configDirs);
    FcStrSetDestroy (config->configMapDirs);
    FcStrSetDestroy (config->fontDirs);
    FcStrSetDestroy (config->cacheDirs);
    FcStrSetDestroy (config->configFiles);
    FcStrSetDestroy (config->acceptGlobs);
    FcStrSetDestroy (config->rejectGlobs);
    FcFontSetDestroy (config->acceptPatterns);
    FcFontSetDestroy (config->rejectPatterns);

    for (k = FcMatchKindBegin; k < FcMatchKindEnd; k++)
	FcPtrListDestroy (config->subst[k]);
    FcPtrListDestroy (config->rulesetList);
    FcStrSetDestroy (config->availConfigFiles);
    for (set = FcSetSystem; set <= FcSetApplication; set++)
	if (config->fonts[set])
	    FcFontSetDestroy (config->fonts[set]);

    page = config->expr_pool;
    while (page)
    {
      FcExprPage *next = page->next_page;
      free (page);
      page = next;
    }
    if (config->sysRoot)
	FcStrFree (config->sysRoot);

    free (config);
}

/*
 * Add cache to configuration, adding fonts and directories
 */

FcBool
FcConfigAddCache (FcConfig *config, FcCache *cache,
		  FcSetName set, FcStrSet *dirSet, FcChar8 *forDir)
{
    FcFontSet	*fs;
    intptr_t	*dirs;
    int		i;
    FcBool      relocated = FcFalse;

    if (strcmp ((char *)FcCacheDir(cache), (char *)forDir) != 0)
      relocated = FcTrue;

    /*
     * Add fonts
     */
    fs = FcCacheSet (cache);
    if (fs)
    {
	int	nref = 0;
	
	for (i = 0; i < fs->nfont; i++)
	{
	    FcPattern	*font = FcFontSetFont (fs, i);
	    FcChar8	*font_file;
	    FcChar8	*relocated_font_file = NULL;

	    if (FcPatternObjectGetString (font, FC_FILE_OBJECT,
					  0, &font_file) == FcResultMatch)
	    {
		if (relocated)
		  {
		    FcChar8 *slash = FcStrLastSlash (font_file);
		    relocated_font_file = FcStrBuildFilename (forDir, slash + 1, NULL);
		    font_file = relocated_font_file;
		  }

		/*
		 * Check to see if font is banned by filename
		 */
		if (!FcConfigAcceptFilename (config, font_file))
		{
		    free (relocated_font_file);
		    continue;
		}
	    }

	    /*
	     * Check to see if font is banned by pattern
	     */
	    if (!FcConfigAcceptFont (config, font))
	    {
		free (relocated_font_file);
		continue;
	    }

	    if (relocated_font_file)
	    {
	      font = FcPatternCacheRewriteFile (font, cache, relocated_font_file);
	      free (relocated_font_file);
	    }

	    if (FcFontSetAdd (config->fonts[set], font))
		nref++;
	}
	FcDirCacheReference (cache, nref);
    }

    /*
     * Add directories
     */
    dirs = FcCacheDirs (cache);
    if (dirs)
    {
	for (i = 0; i < cache->dirs_count; i++)
	{
	    const FcChar8 *dir = FcCacheSubdir (cache, i);
	    FcChar8 *s = NULL;

	    if (relocated)
	    {
		FcChar8 *base = FcStrBasename (dir);
		dir = s = FcStrBuildFilename (forDir, base, NULL);
		FcStrFree (base);
	    }
	    if (FcConfigAcceptFilename (config, dir))
		FcStrSetAddFilename (dirSet, dir);
	    if (s)
		FcStrFree (s);
	}
    }
    return FcTrue;
}

static FcBool
FcConfigAddDirList (FcConfig *config, FcSetName set, FcStrSet *dirSet)
{
    FcStrList	    *dirlist;
    FcChar8	    *dir;
    FcCache	    *cache;

    dirlist = FcStrListCreate (dirSet);
    if (!dirlist)
        return FcFalse;
	
    while ((dir = FcStrListNext (dirlist)))
    {
	if (FcDebug () & FC_DBG_FONTSET)
	    printf ("adding fonts from %s\n", dir);
	cache = FcDirCacheRead (dir, FcFalse, config);
	if (!cache)
	    continue;
	FcConfigAddCache (config, cache, set, dirSet, dir);
	FcDirCacheUnload (cache);
    }
    FcStrListDone (dirlist);
    return FcTrue;
}

/*
 * Scan the current list of directories in the configuration
 * and build the set of available fonts.
 */

FcBool
FcConfigBuildFonts (FcConfig *config)
{
    FcFontSet	    *fonts;

    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }
	
    fonts = FcFontSetCreate ();
    if (!fonts)
	return FcFalse;

    FcConfigSetFonts (config, fonts, FcSetSystem);

    if (!FcConfigAddDirList (config, FcSetSystem, config->fontDirs))
	return FcFalse;
    if (FcDebug () & FC_DBG_FONTSET)
	FcFontSetPrint (fonts);
    return FcTrue;
}

FcBool
FcConfigSetCurrent (FcConfig *config)
{
    FcConfig *cfg;

retry:
    cfg = fc_atomic_ptr_get (&_fcConfig);

    if (config == cfg)
	return FcTrue;

    if (config && !config->fonts[FcSetSystem])
	if (!FcConfigBuildFonts (config))
	    return FcFalse;

    if (!fc_atomic_ptr_cmpexch (&_fcConfig, cfg, config))
	goto retry;

    FcConfigReference (config);
    if (cfg)
	FcConfigDestroy (cfg);

    return FcTrue;
}

FcConfig *
FcConfigGetCurrent (void)
{
    return FcConfigEnsure ();
}

FcBool
FcConfigAddConfigDir (FcConfig	    *config,
		      const FcChar8 *d)
{
    return FcStrSetAddFilename (config->configDirs, d);
}

FcStrList *
FcConfigGetConfigDirs (FcConfig   *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return FcStrListCreate (config->configDirs);
}

FcBool
FcConfigAddFontDir (FcConfig	    *config,
		    const FcChar8   *d,
		    const FcChar8   *m,
		    const FcChar8   *salt)
{
    if (FcDebug() & FC_DBG_CACHE)
    {
	if (m)
	{
	    printf ("%s -> %s%s%s%s\n", d, m, salt ? " (salt: " : "", salt ? (const char *)salt : "", salt ? ")" : "");
	}
	else if (salt)
	{
	    printf ("%s%s%s%s\n", d, salt ? " (salt: " : "", salt ? (const char *)salt : "", salt ? ")" : "");
	}
    }
    return FcStrSetAddFilenamePairWithSalt (config->fontDirs, d, m, salt);
}

FcBool
FcConfigResetFontDirs (FcConfig *config)
{
    if (FcDebug() & FC_DBG_CACHE)
    {
	printf ("Reset font directories!\n");
    }
    return FcStrSetDeleteAll (config->fontDirs);
}

FcStrList *
FcConfigGetFontDirs (FcConfig	*config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return FcStrListCreate (config->fontDirs);
}

static FcBool
FcConfigPathStartsWith(const FcChar8	*path,
		       const FcChar8	*start)
{
    int len = strlen((char *) start);

    if (strncmp((char *) path, (char *) start, len) != 0)
	return FcFalse;

    switch (path[len]) {
    case '\0':
    case FC_DIR_SEPARATOR:
	return FcTrue;
    default:
	return FcFalse;
    }
}

FcChar8 *
FcConfigMapFontPath(FcConfig		*config,
		    const FcChar8	*path)
{
    FcStrList	*list;
    FcChar8	*dir;
    const FcChar8 *map, *rpath;
    FcChar8     *retval;

    list = FcConfigGetFontDirs(config);
    if (!list)
	return 0;
    while ((dir = FcStrListNext(list)))
	if (FcConfigPathStartsWith(path, dir))
	    break;
    FcStrListDone(list);
    if (!dir)
	return 0;
    map = FcStrTripleSecond(dir);
    if (!map)
	return 0;
    rpath = path + strlen ((char *) dir);
    while (*rpath == '/')
	rpath++;
    retval = FcStrBuildFilename(map, rpath, NULL);
    if (retval)
    {
	size_t len = strlen ((const char *) retval);
	while (len > 0 && retval[len-1] == '/')
	    len--;
	/* trim the last slash */
	retval[len] = 0;
    }
    return retval;
}

const FcChar8 *
FcConfigMapSalt (FcConfig      *config,
		 const FcChar8 *path)
{
    FcStrList *list;
    FcChar8 *dir;

    list = FcConfigGetFontDirs (config);
    if (!list)
	return NULL;
    while ((dir = FcStrListNext (list)))
	if (FcConfigPathStartsWith (path, dir))
	    break;
    FcStrListDone (list);
    if (!dir)
	return NULL;

    return FcStrTripleThird (dir);
}

FcBool
FcConfigAddCacheDir (FcConfig	    *config,
		     const FcChar8  *d)
{
    return FcStrSetAddFilename (config->cacheDirs, d);
}

FcStrList *
FcConfigGetCacheDirs (const FcConfig *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return FcStrListCreate (config->cacheDirs);
}

FcBool
FcConfigAddConfigFile (FcConfig	    *config,
		       const FcChar8   *f)
{
    FcBool	ret;
    FcChar8	*file = FcConfigFilename (f);

    if (!file)
	return FcFalse;

    ret = FcStrSetAdd (config->configFiles, file);
    FcStrFree (file);
    return ret;
}

FcStrList *
FcConfigGetConfigFiles (FcConfig    *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return FcStrListCreate (config->configFiles);
}

FcChar8 *
FcConfigGetCache (FcConfig  *config FC_UNUSED)
{
    return NULL;
}

FcFontSet *
FcConfigGetFonts (FcConfig	*config,
		  FcSetName	set)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return config->fonts[set];
}

void
FcConfigSetFonts (FcConfig	*config,
		  FcFontSet	*fonts,
		  FcSetName	set)
{
    if (config->fonts[set])
	FcFontSetDestroy (config->fonts[set]);
    config->fonts[set] = fonts;
}


FcBlanks *
FcBlanksCreate (void)
{
    /* Deprecated. */
    return NULL;
}

void
FcBlanksDestroy (FcBlanks *b FC_UNUSED)
{
    /* Deprecated. */
}

FcBool
FcBlanksAdd (FcBlanks *b FC_UNUSED, FcChar32 ucs4 FC_UNUSED)
{
    /* Deprecated. */
    return FcFalse;
}

FcBool
FcBlanksIsMember (FcBlanks *b FC_UNUSED, FcChar32 ucs4 FC_UNUSED)
{
    /* Deprecated. */
    return FcFalse;
}

FcBlanks *
FcConfigGetBlanks (FcConfig	*config FC_UNUSED)
{
    /* Deprecated. */
    return NULL;
}

FcBool
FcConfigAddBlank (FcConfig	*config FC_UNUSED,
		  FcChar32    	blank FC_UNUSED)
{
    /* Deprecated. */
    return FcFalse;
}


int
FcConfigGetRescanInterval (FcConfig *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return 0;
    }
    return config->rescanInterval;
}

FcBool
FcConfigSetRescanInterval (FcConfig *config, int rescanInterval)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }
    config->rescanInterval = rescanInterval;
    return FcTrue;
}

/*
 * A couple of typos escaped into the library
 */
int
FcConfigGetRescanInverval (FcConfig *config)
{
    return FcConfigGetRescanInterval (config);
}

FcBool
FcConfigSetRescanInverval (FcConfig *config, int rescanInterval)
{
    return FcConfigSetRescanInterval (config, rescanInterval);
}

FcBool
FcConfigAddRule (FcConfig	*config,
		 FcRule		*rule,
		 FcMatchKind	kind)
{
    /* deprecated */
    return FcFalse;
}

static FcValue
FcConfigPromote (FcValue v, FcValue u, FcValuePromotionBuffer *buf)
{
    if (v.type == FcTypeInteger)
    {
	v.type = FcTypeDouble;
	v.u.d = (double) v.u.i;
    }
    else if (v.type == FcTypeVoid && u.type == FcTypeMatrix)
    {
	v.u.m = &FcIdentityMatrix;
	v.type = FcTypeMatrix;
    }
    else if (buf && v.type == FcTypeString && u.type == FcTypeLangSet)
    {
	v.u.l = FcLangSetPromote (v.u.s, buf);
	v.type = FcTypeLangSet;
    }
    else if (buf && v.type == FcTypeVoid && u.type == FcTypeLangSet)
    {
	v.u.l = FcLangSetPromote (NULL, buf);
	v.type = FcTypeLangSet;
    }
    else if (buf && v.type == FcTypeVoid && u.type == FcTypeCharSet)
    {
	v.u.c = FcCharSetPromote (buf);
	v.type = FcTypeCharSet;
    }
    if (buf && v.type == FcTypeDouble && u.type == FcTypeRange)
    {
	v.u.r = FcRangePromote (v.u.d, buf);
	v.type = FcTypeRange;
    }
    return v;
}

FcBool
FcConfigCompareValue (const FcValue	*left_o,
		      unsigned int      op_,
		      const FcValue	*right_o)
{
    FcValue	left = FcValueCanonicalize(left_o);
    FcValue	right = FcValueCanonicalize(right_o);
    FcBool	ret = FcFalse;
    FcOp	op = FC_OP_GET_OP (op_);
    int		flags = FC_OP_GET_FLAGS (op_);
    FcValuePromotionBuffer buf1, buf2;

    left = FcConfigPromote (left, right, &buf1);
    right = FcConfigPromote (right, left, &buf2);
    if (left.type == right.type)
    {
	switch (left.type) {
	case FcTypeUnknown:
	    break;	/* No way to guess how to compare for this object */
	case FcTypeInteger:
	    break;	/* FcConfigPromote prevents this from happening */
	case FcTypeDouble:
	    switch ((int) op) {
	    case FcOpEqual:
	    case FcOpContains:
	    case FcOpListing:
		ret = left.u.d == right.u.d;
		break;
	    case FcOpNotEqual:
	    case FcOpNotContains:
		ret = left.u.d != right.u.d;
		break;
	    case FcOpLess:
		ret = left.u.d < right.u.d;
		break;
	    case FcOpLessEqual:
		ret = left.u.d <= right.u.d;
		break;
	    case FcOpMore:
		ret = left.u.d > right.u.d;
		break;
	    case FcOpMoreEqual:
		ret = left.u.d >= right.u.d;
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeBool:
	    switch ((int) op) {
	    case FcOpEqual:
		ret = left.u.b == right.u.b;
		break;
	    case FcOpContains:
	    case FcOpListing:
		ret = left.u.b == right.u.b || left.u.b >= FcDontCare;
		break;
	    case FcOpNotEqual:
		ret = left.u.b != right.u.b;
		break;
	    case FcOpNotContains:
		ret = !(left.u.b == right.u.b || left.u.b >= FcDontCare);
		break;
	    case FcOpLess:
		ret = left.u.b != right.u.b && right.u.b >= FcDontCare;
		break;
	    case FcOpLessEqual:
		ret = left.u.b == right.u.b || right.u.b >= FcDontCare;
		break;
	    case FcOpMore:
		ret = left.u.b != right.u.b && left.u.b >= FcDontCare;
		break;
	    case FcOpMoreEqual:
		ret = left.u.b == right.u.b || left.u.b >= FcDontCare;
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeString:
	    switch ((int) op) {
	    case FcOpEqual:
	    case FcOpListing:
		if (flags & FcOpFlagIgnoreBlanks)
		    ret = FcStrCmpIgnoreBlanksAndCase (left.u.s, right.u.s) == 0;
		else
		    ret = FcStrCmpIgnoreCase (left.u.s, right.u.s) == 0;
		break;
	    case FcOpContains:
		ret = FcStrStrIgnoreCase (left.u.s, right.u.s) != 0;
		break;
	    case FcOpNotEqual:
		if (flags & FcOpFlagIgnoreBlanks)
		    ret = FcStrCmpIgnoreBlanksAndCase (left.u.s, right.u.s) != 0;
		else
		    ret = FcStrCmpIgnoreCase (left.u.s, right.u.s) != 0;
		break;
	    case FcOpNotContains:
		ret = FcStrStrIgnoreCase (left.u.s, right.u.s) == 0;
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeMatrix:
	    switch ((int) op) {
	    case FcOpEqual:
	    case FcOpContains:
	    case FcOpListing:
		ret = FcMatrixEqual (left.u.m, right.u.m);
		break;
	    case FcOpNotEqual:
	    case FcOpNotContains:
		ret = !FcMatrixEqual (left.u.m, right.u.m);
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeCharSet:
	    switch ((int) op) {
	    case FcOpContains:
	    case FcOpListing:
		/* left contains right if right is a subset of left */
		ret = FcCharSetIsSubset (right.u.c, left.u.c);
		break;
	    case FcOpNotContains:
		/* left contains right if right is a subset of left */
		ret = !FcCharSetIsSubset (right.u.c, left.u.c);
		break;
	    case FcOpEqual:
		ret = FcCharSetEqual (left.u.c, right.u.c);
		break;
	    case FcOpNotEqual:
		ret = !FcCharSetEqual (left.u.c, right.u.c);
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeLangSet:
	    switch ((int) op) {
	    case FcOpContains:
	    case FcOpListing:
		ret = FcLangSetContains (left.u.l, right.u.l);
		break;
	    case FcOpNotContains:
		ret = !FcLangSetContains (left.u.l, right.u.l);
		break;
	    case FcOpEqual:
		ret = FcLangSetEqual (left.u.l, right.u.l);
		break;
	    case FcOpNotEqual:
		ret = !FcLangSetEqual (left.u.l, right.u.l);
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeVoid:
	    switch ((int) op) {
	    case FcOpEqual:
	    case FcOpContains:
	    case FcOpListing:
		ret = FcTrue;
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeFTFace:
	    switch ((int) op) {
	    case FcOpEqual:
	    case FcOpContains:
	    case FcOpListing:
		ret = left.u.f == right.u.f;
		break;
	    case FcOpNotEqual:
	    case FcOpNotContains:
		ret = left.u.f != right.u.f;
		break;
	    default:
		break;
	    }
	    break;
	case FcTypeRange:
	    ret = FcRangeCompare (op, left.u.r, right.u.r);
	    break;
	}
    }
    else
    {
	if (op == FcOpNotEqual || op == FcOpNotContains)
	    ret = FcTrue;
    }
    return ret;
}


#define _FcDoubleFloor(d)	((int) (d))
#define _FcDoubleCeil(d)	((double) (int) (d) == (d) ? (int) (d) : (int) ((d) + 1))
#define FcDoubleFloor(d)	((d) >= 0 ? _FcDoubleFloor(d) : -_FcDoubleCeil(-(d)))
#define FcDoubleCeil(d)		((d) >= 0 ? _FcDoubleCeil(d) : -_FcDoubleFloor(-(d)))
#define FcDoubleRound(d)	FcDoubleFloor ((d) + 0.5)
#define FcDoubleTrunc(d)	((d) >= 0 ? _FcDoubleFloor (d) : -_FcDoubleFloor (-(d)))

static FcValue
FcConfigEvaluate (FcPattern *p, FcPattern *p_pat, FcMatchKind kind, FcExpr *e)
{
    FcValue	v, vl, vr, vle, vre;
    FcMatrix	*m;
    FcChar8     *str;
    FcOp	op = FC_OP_GET_OP (e->op);
    FcValuePromotionBuffer buf1, buf2;

    switch ((int) op) {
    case FcOpInteger:
	v.type = FcTypeInteger;
	v.u.i = e->u.ival;
	break;
    case FcOpDouble:
	v.type = FcTypeDouble;
	v.u.d = e->u.dval;
	break;
    case FcOpString:
	v.type = FcTypeString;
	v.u.s = e->u.sval;
	v = FcValueSave (v);
	break;
    case FcOpMatrix:
	{
	  FcMatrix m;
	  FcValue xx, xy, yx, yy;
	  v.type = FcTypeMatrix;
	  xx = FcConfigPromote (FcConfigEvaluate (p, p_pat, kind, e->u.mexpr->xx), v, NULL);
	  xy = FcConfigPromote (FcConfigEvaluate (p, p_pat, kind, e->u.mexpr->xy), v, NULL);
	  yx = FcConfigPromote (FcConfigEvaluate (p, p_pat, kind, e->u.mexpr->yx), v, NULL);
	  yy = FcConfigPromote (FcConfigEvaluate (p, p_pat, kind, e->u.mexpr->yy), v, NULL);
	  if (xx.type == FcTypeDouble && xy.type == FcTypeDouble &&
	      yx.type == FcTypeDouble && yy.type == FcTypeDouble)
	  {
	    m.xx = xx.u.d;
	    m.xy = xy.u.d;
	    m.yx = yx.u.d;
	    m.yy = yy.u.d;
	    v.u.m = &m;
	  }
	  else
	    v.type = FcTypeVoid;
	  v = FcValueSave (v);
	}
	break;
    case FcOpCharSet:
	v.type = FcTypeCharSet;
	v.u.c = e->u.cval;
	v = FcValueSave (v);
	break;
    case FcOpLangSet:
	v.type = FcTypeLangSet;
	v.u.l = e->u.lval;
	v = FcValueSave (v);
	break;
    case FcOpRange:
	v.type = FcTypeRange;
	v.u.r = e->u.rval;
	v = FcValueSave (v);
	break;
    case FcOpBool:
	v.type = FcTypeBool;
	v.u.b = e->u.bval;
	break;
    case FcOpField:
	if (kind == FcMatchFont && e->u.name.kind == FcMatchPattern)
	{
	    if (FcResultMatch != FcPatternObjectGet (p_pat, e->u.name.object, 0, &v))
		v.type = FcTypeVoid;
	}
	else if (kind == FcMatchPattern && e->u.name.kind == FcMatchFont)
	{
	    fprintf (stderr,
                    "Fontconfig warning: <name> tag has target=\"font\" in a <match target=\"pattern\">.\n");
	    v.type = FcTypeVoid;
	}
	else
	{
	    if (FcResultMatch != FcPatternObjectGet (p, e->u.name.object, 0, &v))
		v.type = FcTypeVoid;
	}
	v = FcValueSave (v);
	break;
    case FcOpConst:
	if (FcNameConstant (e->u.constant, &v.u.i))
	    v.type = FcTypeInteger;
	else
	    v.type = FcTypeVoid;
	break;
    case FcOpQuest:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	if (vl.type == FcTypeBool)
	{
	    if (vl.u.b)
		v = FcConfigEvaluate (p, p_pat, kind, e->u.tree.right->u.tree.left);
	    else
		v = FcConfigEvaluate (p, p_pat, kind, e->u.tree.right->u.tree.right);
	}
	else
	    v.type = FcTypeVoid;
	FcValueDestroy (vl);
	break;
    case FcOpEqual:
    case FcOpNotEqual:
    case FcOpLess:
    case FcOpLessEqual:
    case FcOpMore:
    case FcOpMoreEqual:
    case FcOpContains:
    case FcOpNotContains:
    case FcOpListing:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	vr = FcConfigEvaluate (p, p_pat, kind, e->u.tree.right);
	v.type = FcTypeBool;
	v.u.b = FcConfigCompareValue (&vl, e->op, &vr);
	FcValueDestroy (vl);
	FcValueDestroy (vr);
	break;	
    case FcOpOr:
    case FcOpAnd:
    case FcOpPlus:
    case FcOpMinus:
    case FcOpTimes:
    case FcOpDivide:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	vr = FcConfigEvaluate (p, p_pat, kind, e->u.tree.right);
	vle = FcConfigPromote (vl, vr, &buf1);
	vre = FcConfigPromote (vr, vle, &buf2);
	if (vle.type == vre.type)
	{
	    switch ((int) vle.type) {
	    case FcTypeDouble:
		switch ((int) op) {
		case FcOpPlus:	
		    v.type = FcTypeDouble;
		    v.u.d = vle.u.d + vre.u.d;
		    break;
		case FcOpMinus:
		    v.type = FcTypeDouble;
		    v.u.d = vle.u.d - vre.u.d;
		    break;
		case FcOpTimes:
		    v.type = FcTypeDouble;
		    v.u.d = vle.u.d * vre.u.d;
		    break;
		case FcOpDivide:
		    v.type = FcTypeDouble;
		    v.u.d = vle.u.d / vre.u.d;
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		if (v.type == FcTypeDouble &&
		    v.u.d == (double) (int) v.u.d)
		{
		    v.type = FcTypeInteger;
		    v.u.i = (int) v.u.d;
		}
		break;
	    case FcTypeBool:
		switch ((int) op) {
		case FcOpOr:
		    v.type = FcTypeBool;
		    v.u.b = vle.u.b || vre.u.b;
		    break;
		case FcOpAnd:
		    v.type = FcTypeBool;
		    v.u.b = vle.u.b && vre.u.b;
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		break;
	    case FcTypeString:
		switch ((int) op) {
		case FcOpPlus:
		    v.type = FcTypeString;
		    str = FcStrPlus (vle.u.s, vre.u.s);
		    v.u.s = FcStrdup (str);
		    FcStrFree (str);
			
		    if (!v.u.s)
			v.type = FcTypeVoid;
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		break;
	    case FcTypeMatrix:
		switch ((int) op) {
		case FcOpTimes:
		    v.type = FcTypeMatrix;
		    m = malloc (sizeof (FcMatrix));
		    if (m)
		    {
			FcMatrixMultiply (m, vle.u.m, vre.u.m);
			v.u.m = m;
		    }
		    else
		    {
			v.type = FcTypeVoid;
		    }
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		break;
	    case FcTypeCharSet:
		switch ((int) op) {
		case FcOpPlus:
		    v.type = FcTypeCharSet;
		    v.u.c = FcCharSetUnion (vle.u.c, vre.u.c);
		    if (!v.u.c)
			v.type = FcTypeVoid;
		    break;
		case FcOpMinus:
		    v.type = FcTypeCharSet;
		    v.u.c = FcCharSetSubtract (vle.u.c, vre.u.c);
		    if (!v.u.c)
			v.type = FcTypeVoid;
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		break;
	    case FcTypeLangSet:
		switch ((int) op) {
		case FcOpPlus:
		    v.type = FcTypeLangSet;
		    v.u.l = FcLangSetUnion (vle.u.l, vre.u.l);
		    if (!v.u.l)
			v.type = FcTypeVoid;
		    break;
		case FcOpMinus:
		    v.type = FcTypeLangSet;
		    v.u.l = FcLangSetSubtract (vle.u.l, vre.u.l);
		    if (!v.u.l)
			v.type = FcTypeVoid;
		    break;
		default:
		    v.type = FcTypeVoid;
		    break;
		}
		break;
	    default:
		v.type = FcTypeVoid;
		break;
	    }
	}
	else
	    v.type = FcTypeVoid;
	FcValueDestroy (vl);
	FcValueDestroy (vr);
	break;
    case FcOpNot:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	switch ((int) vl.type) {
	case FcTypeBool:
	    v.type = FcTypeBool;
	    v.u.b = !vl.u.b;
	    break;
	default:
	    v.type = FcTypeVoid;
	    break;
	}
	FcValueDestroy (vl);
	break;
    case FcOpFloor:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	switch ((int) vl.type) {
	case FcTypeInteger:
	    v = vl;
	    break;
	case FcTypeDouble:
	    v.type = FcTypeInteger;
	    v.u.i = FcDoubleFloor (vl.u.d);
	    break;
	default:
	    v.type = FcTypeVoid;
	    break;
	}
	FcValueDestroy (vl);
	break;
    case FcOpCeil:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	switch ((int) vl.type) {
	case FcTypeInteger:
	    v = vl;
	    break;
	case FcTypeDouble:
	    v.type = FcTypeInteger;
	    v.u.i = FcDoubleCeil (vl.u.d);
	    break;
	default:
	    v.type = FcTypeVoid;
	    break;
	}
	FcValueDestroy (vl);
	break;
    case FcOpRound:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	switch ((int) vl.type) {
	case FcTypeInteger:
	    v = vl;
	    break;
	case FcTypeDouble:
	    v.type = FcTypeInteger;
	    v.u.i = FcDoubleRound (vl.u.d);
	    break;
	default:
	    v.type = FcTypeVoid;
	    break;
	}
	FcValueDestroy (vl);
	break;
    case FcOpTrunc:
	vl = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	switch ((int) vl.type) {
	case FcTypeInteger:
	    v = vl;
	    break;
	case FcTypeDouble:
	    v.type = FcTypeInteger;
	    v.u.i = FcDoubleTrunc (vl.u.d);
	    break;
	default:
	    v.type = FcTypeVoid;
	    break;
	}
	FcValueDestroy (vl);
	break;
    default:
	v.type = FcTypeVoid;
	break;
    }
    return v;
}

static FcValueList *
FcConfigMatchValueList (FcPattern	*p,
			FcPattern	*p_pat,
			FcMatchKind      kind,
			FcTest		*t,
			FcValueList	*values)
{
    FcValueList	    *ret = 0;
    FcExpr	    *e = t->expr;
    FcValue	    value;
    FcValueList	    *v;

    while (e)
    {
	/* Compute the value of the match expression */
	if (FC_OP_GET_OP (e->op) == FcOpComma)
	{
	    value = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	    e = e->u.tree.right;
	}
	else
	{
	    value = FcConfigEvaluate (p, p_pat, kind, e);
	    e = 0;
	}

	for (v = values; v; v = FcValueListNext(v))
	{
	    /* Compare the pattern value to the match expression value */
	    if (FcConfigCompareValue (&v->value, t->op, &value))
	    {
		if (!ret)
		    ret = v;
	    }
	    else
	    {
		if (t->qual == FcQualAll)
		{
		    ret = 0;
		    break;
		}
	    }
	}
	FcValueDestroy (value);
    }
    return ret;
}

static FcValueList *
FcConfigValues (FcPattern *p, FcPattern *p_pat, FcMatchKind kind, FcExpr *e, FcValueBinding binding)
{
    FcValueList	*l;

    if (!e)
	return 0;
    l = (FcValueList *) malloc (sizeof (FcValueList));
    if (!l)
	return 0;
    if (FC_OP_GET_OP (e->op) == FcOpComma)
    {
	l->value = FcConfigEvaluate (p, p_pat, kind, e->u.tree.left);
	l->next = FcConfigValues (p, p_pat, kind, e->u.tree.right, binding);
    }
    else
    {
	l->value = FcConfigEvaluate (p, p_pat, kind, e);
	l->next = NULL;
    }
    l->binding = binding;
    if (l->value.type == FcTypeVoid)
    {
	FcValueList  *next = FcValueListNext(l);

	free (l);
	l = next;
    }

    return l;
}

static FcBool
FcConfigAdd (FcValueListPtr *head,
	     FcValueList    *position,
	     FcBool	    append,
	     FcValueList    *new,
	     FcObject        object)
{
    FcValueListPtr  *prev, l, last, v;
    FcValueBinding  sameBinding;

    /*
     * Make sure the stored type is valid for built-in objects
     */
    for (l = new; l != NULL; l = FcValueListNext (l))
    {
	if (!FcObjectValidType (object, l->value.type))
	{
	    fprintf (stderr,
		     "Fontconfig warning: FcPattern object %s does not accept value", FcObjectName (object));
	    FcValuePrintFile (stderr, l->value);
	    fprintf (stderr, "\n");

	    if (FcDebug () & FC_DBG_EDIT)
	    {
		printf ("Not adding\n");
	    }

	    return FcFalse;
	}
    }

    if (position)
	sameBinding = position->binding;
    else
	sameBinding = FcValueBindingWeak;
    for (v = new; v != NULL; v = FcValueListNext(v))
	if (v->binding == FcValueBindingSame)
	    v->binding = sameBinding;
    if (append)
    {
	if (position)
	    prev = &position->next;
	else
	    for (prev = head; *prev != NULL;
		 prev = &(*prev)->next)
		;
    }
    else
    {
	if (position)
	{
	    for (prev = head; *prev != NULL;
		 prev = &(*prev)->next)
	    {
		if (*prev == position)
		    break;
	    }
	}
	else
	    prev = head;

	if (FcDebug () & FC_DBG_EDIT)
	{
	    if (*prev == NULL)
		printf ("position not on list\n");
	}
    }

    if (FcDebug () & FC_DBG_EDIT)
    {
	printf ("%s list before ", append ? "Append" : "Prepend");
	FcValueListPrintWithPosition (*head, *prev);
	printf ("\n");
    }

    if (new)
    {
	last = new;
	while (last->next != NULL)
	    last = last->next;

	last->next = *prev;
	*prev = new;
    }

    if (FcDebug () & FC_DBG_EDIT)
    {
	printf ("%s list after ", append ? "Append" : "Prepend");
	FcValueListPrint (*head);
	printf ("\n");
    }

    return FcTrue;
}

static void
FcConfigDel (FcValueListPtr *head,
	     FcValueList    *position)
{
    FcValueListPtr *prev;

    for (prev = head; *prev != NULL; prev = &(*prev)->next)
    {
	if (*prev == position)
	{
	    *prev = position->next;
	    position->next = NULL;
	    FcValueListDestroy (position);
	    break;
	}
    }
}

static void
FcConfigPatternAdd (FcPattern	*p,
		    FcObject	object,
		    FcValueList	*list,
		    FcBool	append)
{
    if (list)
    {
	FcPatternElt    *e = FcPatternObjectInsertElt (p, object);

	if (!e)
	    return;
	FcConfigAdd (&e->values, 0, append, list, object);
    }
}

/*
 * Delete all values associated with a field
 */
static void
FcConfigPatternDel (FcPattern	*p,
		    FcObject	object)
{
    FcPatternElt    *e = FcPatternObjectFindElt (p, object);
    if (!e)
	return;
    while (e->values != NULL)
	FcConfigDel (&e->values, e->values);
}

static void
FcConfigPatternCanon (FcPattern	    *p,
		      FcObject	    object)
{
    FcPatternElt    *e = FcPatternObjectFindElt (p, object);
    if (!e)
	return;
    if (e->values == NULL)
	FcPatternObjectDel (p, object);
}

FcBool
FcConfigSubstituteWithPat (FcConfig    *config,
			   FcPattern   *p,
			   FcPattern   *p_pat,
			   FcMatchKind kind)
{
    FcValue v;
    FcPtrList	    *s;
    FcPtrListIter    iter, iter2;
    FcRule          *r;
    FcRuleSet	    *rs;
    FcValueList	    *l, **value = NULL, *vl;
    FcPattern	    *m;
    FcStrSet	    *strs;
    FcObject	    object = FC_INVALID_OBJECT;
    FcPatternElt    **elt = NULL, *e;
    int		    i, nobjs;
    FcBool	    retval = FcTrue;
    FcTest	    **tst = NULL;

    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }

    if (kind < FcMatchKindBegin || kind >= FcMatchKindEnd)
	return FcFalse;
    s = config->subst[kind];
    if (kind == FcMatchPattern)
    {
	strs = FcGetDefaultLangs ();
	if (strs)
	{
	    FcStrList *l = FcStrListCreate (strs);
	    FcChar8 *lang;
	    FcValue v;
	    FcLangSet *lsund = FcLangSetCreate ();

	    FcLangSetAdd (lsund, (const FcChar8 *)"und");
	    FcStrSetDestroy (strs);
	    while (l && (lang = FcStrListNext (l)))
	    {
		FcPatternElt *e = FcPatternObjectFindElt (p, FC_LANG_OBJECT);

		if (e)
		{
		    FcValueListPtr ll;

		    for (ll = FcPatternEltValues (e); ll; ll = FcValueListNext (ll))
		    {
			FcValue vv = FcValueCanonicalize (&ll->value);

			if (vv.type == FcTypeLangSet)
			{
			    FcLangSet *ls = FcLangSetCreate ();
			    FcBool b;

			    FcLangSetAdd (ls, lang);
			    b = FcLangSetContains (vv.u.l, ls);
			    FcLangSetDestroy (ls);
			    if (b)
				goto bail_lang;
			    if (FcLangSetContains (vv.u.l, lsund))
				goto bail_lang;
			}
			else
			{
			    if (FcStrCmpIgnoreCase (vv.u.s, lang) == 0)
				goto bail_lang;
			    if (FcStrCmpIgnoreCase (vv.u.s, (const FcChar8 *)"und") == 0)
				goto bail_lang;
			}
		    }
		}
		v.type = FcTypeString;
		v.u.s = lang;

		FcPatternObjectAddWithBinding (p, FC_LANG_OBJECT, v, FcValueBindingWeak, FcTrue);
	    }
	bail_lang:
	    FcStrListDone (l);
	    FcLangSetDestroy (lsund);
	}
	if (FcPatternObjectGet (p, FC_PRGNAME_OBJECT, 0, &v) == FcResultNoMatch)
	{
	    FcChar8 *prgname = FcGetPrgname ();
	    if (prgname)
		FcPatternObjectAddString (p, FC_PRGNAME_OBJECT, prgname);
	}
    }

    nobjs = FC_MAX_BASE_OBJECT + config->maxObjects + 2;
    value = (FcValueList **) malloc (SIZEOF_VOID_P * nobjs);
    if (!value)
    {
	retval = FcFalse;
	goto bail1;
    }
    elt = (FcPatternElt **) malloc (SIZEOF_VOID_P * nobjs);
    if (!elt)
    {
	retval = FcFalse;
	goto bail1;
    }
    tst = (FcTest **) malloc (SIZEOF_VOID_P * nobjs);
    if (!tst)
    {
	retval = FcFalse;
	goto bail1;
    }

    if (FcDebug () & FC_DBG_EDIT)
    {
	printf ("FcConfigSubstitute ");
	FcPatternPrint (p);
    }
    FcPtrListIterInit (s, &iter);
    for (; FcPtrListIterIsValid (s, &iter); FcPtrListIterNext (s, &iter))
    {
	rs = (FcRuleSet *) FcPtrListIterGetValue (s, &iter);
	if (FcDebug () & FC_DBG_EDIT)
	{
	    printf ("\nRule Set: %s\n", rs->name);
	}
	FcPtrListIterInit (rs->subst[kind], &iter2);
	for (; FcPtrListIterIsValid (rs->subst[kind], &iter2); FcPtrListIterNext (rs->subst[kind], &iter2))
	{
	    r = (FcRule *) FcPtrListIterGetValue (rs->subst[kind], &iter2);
	    for (i = 0; i < nobjs; i++)
	    {
		elt[i] = NULL;
		value[i] = NULL;
		tst[i] = NULL;
	    }
	    for (; r; r = r->next)
	    {
		switch (r->type) {
		case FcRuleUnknown:
		    /* shouldn't be reached */
		    break;
		case FcRuleTest:
		    object = FC_OBJ_ID (r->u.test->object);
		    /*
		     * Check the tests to see if
		     * they all match the pattern
		     */
		    if (FcDebug () & FC_DBG_EDIT)
		    {
			printf ("FcConfigSubstitute test ");
			FcTestPrint (r->u.test);
		    }
		    if (kind == FcMatchFont && r->u.test->kind == FcMatchPattern)
			m = p_pat;
		    else
			m = p;
		    if (m)
			e = FcPatternObjectFindElt (m, r->u.test->object);
		    else
			e = NULL;
		    /* different 'kind' won't be the target of edit */
		    if (!elt[object] && kind == r->u.test->kind)
		    {
			elt[object] = e;
			tst[object] = r->u.test;
		    }
		    /*
		     * If there's no such field in the font,
		     * then FcQualAll matches while FcQualAny does not
		     */
		    if (!e)
		    {
			if (r->u.test->qual == FcQualAll)
			{
			    value[object] = NULL;
			    continue;
			}
			else
			{
			    if (FcDebug () & FC_DBG_EDIT)
				printf ("No match\n");
			    goto bail;
			}
		    }
		    /*
		     * Check to see if there is a match, mark the location
		     * to apply match-relative edits
		     */
		    vl = FcConfigMatchValueList (m, p_pat, kind, r->u.test, e->values);
		    /* different 'kind' won't be the target of edit */
		    if (!value[object] && kind == r->u.test->kind)
			value[object] = vl;
		    if (!vl ||
			(r->u.test->qual == FcQualFirst && vl != e->values) ||
			(r->u.test->qual == FcQualNotFirst && vl == e->values))
		    {
			if (FcDebug () & FC_DBG_EDIT)
			    printf ("No match\n");
			goto bail;
		    }
		    break;
		case FcRuleEdit:
		    object = FC_OBJ_ID (r->u.edit->object);
		    if (FcDebug () & FC_DBG_EDIT)
		    {
			printf ("Substitute ");
			FcEditPrint (r->u.edit);
			printf ("\n\n");
		    }
		    /*
		     * Evaluate the list of expressions
		     */
		    l = FcConfigValues (p, p_pat,kind,  r->u.edit->expr, r->u.edit->binding);
		    if (tst[object] && (tst[object]->kind == FcMatchFont || kind == FcMatchPattern))
			elt[object] = FcPatternObjectFindElt (p, tst[object]->object);

		    switch (FC_OP_GET_OP (r->u.edit->op)) {
		    case FcOpAssign:
			/*
			 * If there was a test, then replace the matched
			 * value with the new list of values
			 */
			if (value[object])
			{
			    FcValueList	*thisValue = value[object];
			    FcValueList	*nextValue = l;

			    /*
			     * Append the new list of values after the current value
			     */
			    FcConfigAdd (&elt[object]->values, thisValue, FcTrue, l, r->u.edit->object);
			    /*
			     * Delete the marked value
			     */
			    if (thisValue)
				FcConfigDel (&elt[object]->values, thisValue);
			    /*
			     * Adjust a pointer into the value list to ensure
			     * future edits occur at the same place
			     */
			    value[object] = nextValue;
			    break;
			}
			/* fall through ... */
		    case FcOpAssignReplace:
			/*
			 * Delete all of the values and insert
			 * the new set
			 */
			FcConfigPatternDel (p, r->u.edit->object);
			FcConfigPatternAdd (p, r->u.edit->object, l, FcTrue);
			/*
			 * Adjust a pointer into the value list as they no
			 * longer point to anything valid
			 */
			value[object] = NULL;
			break;
		    case FcOpPrepend:
			if (value[object])
			{
			    FcConfigAdd (&elt[object]->values, value[object], FcFalse, l, r->u.edit->object);
			    break;
			}
			/* fall through ... */
		    case FcOpPrependFirst:
			FcConfigPatternAdd (p, r->u.edit->object, l, FcFalse);
			break;
		    case FcOpAppend:
			if (value[object])
			{
			    FcConfigAdd (&elt[object]->values, value[object], FcTrue, l, r->u.edit->object);
			    break;
			}
			/* fall through ... */
		    case FcOpAppendLast:
			FcConfigPatternAdd (p, r->u.edit->object, l, FcTrue);
			break;
		    case FcOpDelete:
			if (value[object])
			{
			    FcConfigDel (&elt[object]->values, value[object]);
			    FcValueListDestroy (l);
			    break;
			}
			/* fall through ... */
		    case FcOpDeleteAll:
			FcConfigPatternDel (p, r->u.edit->object);
			FcValueListDestroy (l);
			break;
		    default:
			FcValueListDestroy (l);
			break;
		    }
		    /*
		     * Now go through the pattern and eliminate
		     * any properties without data
		     */
		    FcConfigPatternCanon (p, r->u.edit->object);

		    if (FcDebug () & FC_DBG_EDIT)
		    {
			printf ("FcConfigSubstitute edit");
			FcPatternPrint (p);
		    }
		    break;
		}
	    }
	bail:;
	}
    }
    if (FcDebug () & FC_DBG_EDIT)
    {
	printf ("FcConfigSubstitute done");
	FcPatternPrint (p);
    }
bail1:
    if (elt)
	free (elt);
    if (value)
	free (value);
    if (tst)
	free (tst);

    return retval;
}

FcBool
FcConfigSubstitute (FcConfig	*config,
		    FcPattern	*p,
		    FcMatchKind	kind)
{
    return FcConfigSubstituteWithPat (config, p, 0, kind);
}

#if defined (_WIN32)

static FcChar8 fontconfig_path[1000] = ""; /* MT-dontcare */
FcChar8 fontconfig_instprefix[1000] = ""; /* MT-dontcare */

#  if (defined (PIC) || defined (DLL_EXPORT))

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  FcChar8 *p;

  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
      if (!GetModuleFileName ((HMODULE) hinstDLL, (LPCH) fontconfig_path,
			      sizeof (fontconfig_path)))
	  break;

      /* If the fontconfig DLL is in a "bin" or "lib" subfolder,
       * assume it's a Unix-style installation tree, and use
       * "etc/fonts" in there as FONTCONFIG_PATH. Otherwise use the
       * folder where the DLL is as FONTCONFIG_PATH.
       */
      p = (FcChar8 *) strrchr ((const char *) fontconfig_path, '\\');
      if (p)
      {
	  *p = '\0';
	  p = (FcChar8 *) strrchr ((const char *) fontconfig_path, '\\');
	  if (p && (FcStrCmpIgnoreCase (p + 1, (const FcChar8 *) "bin") == 0 ||
		    FcStrCmpIgnoreCase (p + 1, (const FcChar8 *) "lib") == 0))
	      *p = '\0';
	  strcat ((char *) fontconfig_instprefix, (char *) fontconfig_path);
	  strcat ((char *) fontconfig_path, "\\etc\\fonts");
      }
      else
          fontconfig_path[0] = '\0';

      break;
  }

  return TRUE;
}

#  endif /* !PIC */

#undef FONTCONFIG_PATH
#define FONTCONFIG_PATH fontconfig_path

#endif /* !_WIN32 */

#ifndef FONTCONFIG_FILE
#define FONTCONFIG_FILE	"fonts.conf"
#endif

static FcChar8 *
FcConfigFileExists (const FcChar8 *dir, const FcChar8 *file)
{
    FcChar8    *path;
    int         size, osize;

    if (!dir)
	dir = (FcChar8 *) "";

    osize = strlen ((char *) dir) + 1 + strlen ((char *) file) + 1;
    /*
     * workaround valgrind warning because glibc takes advantage of how it knows memory is
     * allocated to implement strlen by reading in groups of 4
     */
    size = (osize + 3) & ~3;

    path = malloc (size);
    if (!path)
	return 0;

    strcpy ((char *) path, (const char *) dir);
    /* make sure there's a single separator */
#ifdef _WIN32
    if ((!path[0] || (path[strlen((char *) path)-1] != '/' &&
		      path[strlen((char *) path)-1] != '\\')) &&
	!(file[0] == '/' ||
	  file[0] == '\\' ||
	  (isalpha (file[0]) && file[1] == ':' && (file[2] == '/' || file[2] == '\\'))))
	strcat ((char *) path, "\\");
#else
    if ((!path[0] || path[strlen((char *) path)-1] != '/') && file[0] != '/')
	strcat ((char *) path, "/");
    else
	osize--;
#endif
    strcat ((char *) path, (char *) file);

    if (access ((char *) path, R_OK) == 0)
	return path;

    FcStrFree (path);

    return 0;
}

static FcChar8 **
FcConfigGetPath (void)
{
    FcChar8    **path;
    FcChar8    *env, *e, *colon;
    FcChar8    *dir;
    int	    npath;
    int	    i;

    npath = 2;	/* default dir + null */
    env = (FcChar8 *) getenv ("FONTCONFIG_PATH");
    if (env)
    {
	e = env;
	npath++;
	while (*e)
	    if (*e++ == FC_SEARCH_PATH_SEPARATOR)
		npath++;
    }
    path = calloc (npath, sizeof (FcChar8 *));
    if (!path)
	goto bail0;
    i = 0;

    if (env)
    {
	e = env;
	while (*e)
	{
	    colon = (FcChar8 *) strchr ((char *) e, FC_SEARCH_PATH_SEPARATOR);
	    if (!colon)
		colon = e + strlen ((char *) e);
	    path[i] = malloc (colon - e + 1);
	    if (!path[i])
		goto bail1;
	    strncpy ((char *) path[i], (const char *) e, colon - e);
	    path[i][colon - e] = '\0';
	    if (*colon)
		e = colon + 1;
	    else
		e = colon;
	    i++;
	}
    }

#ifdef _WIN32
	if (fontconfig_path[0] == '\0')
	{
		char *p;
		if(!GetModuleFileName(NULL, (LPCH) fontconfig_path, sizeof(fontconfig_path)))
			goto bail1;
		p = strrchr ((const char *) fontconfig_path, '\\');
		if (p) *p = '\0';
		strcat ((char *) fontconfig_path, "\\fonts");
	}
#endif
    dir = (FcChar8 *) FONTCONFIG_PATH;
    path[i] = malloc (strlen ((char *) dir) + 1);
    if (!path[i])
	goto bail1;
    strcpy ((char *) path[i], (const char *) dir);
    return path;

bail1:
    for (i = 0; path[i]; i++)
	free (path[i]);
    free (path);
bail0:
    return 0;
}

static void
FcConfigFreePath (FcChar8 **path)
{
    FcChar8    **p;

    for (p = path; *p; p++)
	free (*p);
    free (path);
}

static FcBool	_FcConfigHomeEnabled = FcTrue; /* MT-goodenough */

FcChar8 *
FcConfigHome (void)
{
    if (_FcConfigHomeEnabled)
    {
        char *home = getenv ("HOME");

#ifdef _WIN32
	if (home == NULL)
	    home = getenv ("USERPROFILE");
#endif

	return (FcChar8 *) home;
    }
    return 0;
}

FcChar8 *
FcConfigXdgCacheHome (void)
{
    const char *env = getenv ("XDG_CACHE_HOME");
    FcChar8 *ret = NULL;

    if (!_FcConfigHomeEnabled)
	return NULL;
    if (env)
	ret = FcStrCopy ((const FcChar8 *)env);
    else
    {
	const FcChar8 *home = FcConfigHome ();
	size_t len = home ? strlen ((const char *)home) : 0;

	ret = malloc (len + 7 + 1);
	if (ret)
	{
	    if (home)
		memcpy (ret, home, len);
	    memcpy (&ret[len], FC_DIR_SEPARATOR_S ".cache", 7);
	    ret[len + 7] = 0;
	}
    }

    return ret;
}

FcChar8 *
FcConfigXdgConfigHome (void)
{
    const char *env = getenv ("XDG_CONFIG_HOME");
    FcChar8 *ret = NULL;

    if (!_FcConfigHomeEnabled)
	return NULL;
    if (env)
	ret = FcStrCopy ((const FcChar8 *)env);
    else
    {
	const FcChar8 *home = FcConfigHome ();
	size_t len = home ? strlen ((const char *)home) : 0;

	ret = malloc (len + 8 + 1);
	if (ret)
	{
	    if (home)
		memcpy (ret, home, len);
	    memcpy (&ret[len], FC_DIR_SEPARATOR_S ".config", 8);
	    ret[len + 8] = 0;
	}
    }

    return ret;
}

FcChar8 *
FcConfigXdgDataHome (void)
{
    const char *env = getenv ("XDG_DATA_HOME");
    FcChar8 *ret = NULL;

    if (!_FcConfigHomeEnabled)
	return NULL;
    if (env)
	ret = FcStrCopy ((const FcChar8 *)env);
    else
    {
	const FcChar8 *home = FcConfigHome ();
	size_t len = home ? strlen ((const char *)home) : 0;

	ret = malloc (len + 13 + 1);
	if (ret)
	{
	    if (home)
		memcpy (ret, home, len);
	    memcpy (&ret[len], FC_DIR_SEPARATOR_S ".local" FC_DIR_SEPARATOR_S "share", 13);
	    ret[len + 13] = 0;
	}
    }

    return ret;
}

FcBool
FcConfigEnableHome (FcBool enable)
{
    FcBool  prev = _FcConfigHomeEnabled;
    _FcConfigHomeEnabled = enable;
    return prev;
}

FcChar8 *
FcConfigFilename (const FcChar8 *url)
{
    FcChar8    *file, *dir, **path, **p;

    if (!url || !*url)
    {
	url = (FcChar8 *) getenv ("FONTCONFIG_FILE");
	if (!url)
	    url = (FcChar8 *) FONTCONFIG_FILE;
    }
    file = 0;

    if (FcStrIsAbsoluteFilename(url))
	return FcConfigFileExists (0, url);

    if (*url == '~')
    {
	dir = FcConfigHome ();
	if (dir)
	    file = FcConfigFileExists (dir, url + 1);
	else
	    file = 0;
    }
    else
    {
	path = FcConfigGetPath ();
	if (!path)
	    return NULL;
	for (p = path; *p; p++)
	{
	    file = FcConfigFileExists (*p, url);
	    if (file)
		break;
	}
	FcConfigFreePath (path);
    }
    return file;
}

FcChar8 *
FcConfigRealFilename (FcConfig		*config,
		      const FcChar8	*url)
{
    const FcChar8 *sysroot = FcConfigGetSysRoot (config);
    FcChar8 *n = FcConfigFilename (url);
    FcChar8 *nn = NULL;

    if (n)
    {
	FcChar8 buf[FC_PATH_MAX];
	ssize_t len;

	if (sysroot)
	    nn = FcStrBuildFilename (sysroot, n, NULL);
	else
	    nn = FcStrdup (n);
	FcStrFree (n);

	if ((len = FcReadLink (nn, buf, sizeof (buf) - 1)) != -1)
	{
	    buf[len] = 0;

	    if (!FcStrIsAbsoluteFilename (buf))
	    {
		FcChar8 *dirname = FcStrDirname (nn);
		FcStrFree (nn);
		if (!dirname)
		    return NULL;

		FcChar8 *path = FcStrBuildFilename (dirname, buf, NULL);
		FcStrFree (dirname);
		if (!path)
		    return NULL;

		nn = FcStrCanonFilename (path);
		FcStrFree (path);
	    }
	    else
	    {
		FcStrFree (nn);
		nn = FcStrdup (buf);
	    }
	}
    }

    return nn;
}

/*
 * Manage the application-specific fonts
 */

FcBool
FcConfigAppFontAddFile (FcConfig    *config,
			const FcChar8  *file)
{
    FcFontSet	*set;
    FcStrSet	*subdirs;
    FcStrList	*sublist;
    FcChar8	*subdir;

    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }

    subdirs = FcStrSetCreateEx (FCSS_GROW_BY_64);
    if (!subdirs)
	return FcFalse;

    set = FcConfigGetFonts (config, FcSetApplication);
    if (!set)
    {
	set = FcFontSetCreate ();
	if (!set)
	{
	    FcStrSetDestroy (subdirs);
	    return FcFalse;
	}
	FcConfigSetFonts (config, set, FcSetApplication);
    }
	
    if (!FcFileScanConfig (set, subdirs, file, config))
    {
	FcStrSetDestroy (subdirs);
	return FcFalse;
    }
    if ((sublist = FcStrListCreate (subdirs)))
    {
	while ((subdir = FcStrListNext (sublist)))
	{
	    FcConfigAppFontAddDir (config, subdir);
	}
	FcStrListDone (sublist);
    }
    FcStrSetDestroy (subdirs);
    return FcTrue;
}

FcBool
FcConfigAppFontAddDir (FcConfig	    *config,
		       const FcChar8   *dir)
{
    FcFontSet	*set;
    FcStrSet	*dirs;

    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return FcFalse;
    }

    dirs = FcStrSetCreateEx (FCSS_GROW_BY_64);
    if (!dirs)
	return FcFalse;

    set = FcConfigGetFonts (config, FcSetApplication);
    if (!set)
    {
	set = FcFontSetCreate ();
	if (!set)
	{
	    FcStrSetDestroy (dirs);
	    return FcFalse;
	}
	FcConfigSetFonts (config, set, FcSetApplication);
    }

    FcStrSetAddFilename (dirs, dir);

    if (!FcConfigAddDirList (config, FcSetApplication, dirs))
    {
	FcStrSetDestroy (dirs);
	return FcFalse;
    }
    FcStrSetDestroy (dirs);
    return FcTrue;
}

void
FcConfigAppFontClear (FcConfig	    *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return;
    }

    FcConfigSetFonts (config, 0, FcSetApplication);
}

/*
 * Manage filename-based font source selectors
 */

FcBool
FcConfigGlobAdd (FcConfig	*config,
		 const FcChar8  *glob,
		 FcBool		accept)
{
    FcStrSet	*set = accept ? config->acceptGlobs : config->rejectGlobs;

    return FcStrSetAdd (set, glob);
}

static FcBool
FcConfigGlobsMatch (const FcStrSet	*globs,
		    const FcChar8	*string)
{
    int	i;

    for (i = 0; i < globs->num; i++)
	if (FcStrGlobMatch (globs->strs[i], string))
	    return FcTrue;
    return FcFalse;
}

FcBool
FcConfigAcceptFilename (FcConfig	*config,
			const FcChar8	*filename)
{
    if (FcConfigGlobsMatch (config->acceptGlobs, filename))
	return FcTrue;
    if (FcConfigGlobsMatch (config->rejectGlobs, filename))
	return FcFalse;
    return FcTrue;
}

/*
 * Manage font-pattern based font source selectors
 */

FcBool
FcConfigPatternsAdd (FcConfig	*config,
		     FcPattern	*pattern,
		     FcBool	accept)
{
    FcFontSet	*set = accept ? config->acceptPatterns : config->rejectPatterns;

    return FcFontSetAdd (set, pattern);
}

static FcBool
FcConfigPatternsMatch (const FcFontSet	*patterns,
		       const FcPattern	*font)
{
    int i;

    for (i = 0; i < patterns->nfont; i++)
	if (FcListPatternMatchAny (patterns->fonts[i], font))
	    return FcTrue;
    return FcFalse;
}

FcBool
FcConfigAcceptFont (FcConfig	    *config,
		    const FcPattern *font)
{
    if (FcConfigPatternsMatch (config->acceptPatterns, font))
	return FcTrue;
    if (FcConfigPatternsMatch (config->rejectPatterns, font))
	return FcFalse;
    return FcTrue;
}

const FcChar8 *
FcConfigGetSysRoot (const FcConfig *config)
{
    if (!config)
    {
	config = FcConfigGetCurrent ();
	if (!config)
	    return NULL;
    }
    return config->sysRoot;
}

void
FcConfigSetSysRoot (FcConfig      *config,
		    const FcChar8 *sysroot)
{
    FcChar8 *s = NULL;
    FcBool init = FcFalse;

    if (!config)
    {
	/* We can't use FcConfigGetCurrent() here to ensure
	 * the sysroot is set prior to initialize FcConfig,
	 * to avoid loading caches from non-sysroot dirs.
	 * So postpone the initialization later.
	 */
	config = fc_atomic_ptr_get (&_fcConfig);
	if (!config)
	{
	    config = FcConfigCreate ();
	    if (!config)
		return;
	    init = FcTrue;
	}
    }

    if (sysroot)
    {
	s = FcConfigRealPath(sysroot);
	if (!s)
	    return;
    }

    if (config->sysRoot)
	FcStrFree (config->sysRoot);

    config->sysRoot = s;
    if (init)
    {
	config = FcInitLoadOwnConfigAndFonts (config);
	FcConfigSetCurrent (config);
	/* FcConfigSetCurrent() increases the refcount.
	 * decrease it here to avoid the memory leak.
	 */
	FcConfigDestroy (config);
    }
}

FcRuleSet *
FcRuleSetCreate (const FcChar8 *name)
{
    FcRuleSet *ret = (FcRuleSet *) malloc (sizeof (FcRuleSet));
    FcMatchKind k;
    const FcChar8 *p;

    if (!name)
	p = (const FcChar8 *)"";
    else
	p = name;

    if (ret)
    {
	ret->name = FcStrdup (p);
	ret->description = NULL;
	ret->domain = NULL;
	for (k = FcMatchKindBegin; k < FcMatchKindEnd; k++)
	    ret->subst[k] = FcPtrListCreate (FcDestroyAsRule);
	FcRefInit (&ret->ref, 1);
    }

    return ret;
}

void
FcRuleSetDestroy (FcRuleSet *rs)
{
    FcMatchKind k;

    if (!rs)
	return;
    if (FcRefDec (&rs->ref) != 1)
	return;

    if (rs->name)
	FcStrFree (rs->name);
    if (rs->description)
	FcStrFree (rs->description);
    if (rs->domain)
	FcStrFree (rs->domain);
    for (k = FcMatchKindBegin; k < FcMatchKindEnd; k++)
	FcPtrListDestroy (rs->subst[k]);

    free (rs);
}

void
FcRuleSetReference (FcRuleSet *rs)
{
    if (!FcRefIsConst (&rs->ref))
	FcRefInc (&rs->ref);
}

void
FcRuleSetEnable (FcRuleSet	*rs,
		 FcBool		flag)
{
    if (rs)
    {
	rs->enabled = flag;
	/* XXX: we may want to provide a feature
	 * to enable/disable rulesets through API
	 * in the future?
	 */
    }
}

void
FcRuleSetAddDescription (FcRuleSet	*rs,
			 const FcChar8	*domain,
			 const FcChar8	*description)
{
    if (rs->domain)
	FcStrFree (rs->domain);
    if (rs->description)
	FcStrFree (rs->description);

    rs->domain = domain ? FcStrdup (domain) : NULL;
    rs->description = description ? FcStrdup (description) : NULL;
}

int
FcRuleSetAdd (FcRuleSet		*rs,
	      FcRule		*rule,
	      FcMatchKind	kind)
{
    FcPtrListIter iter;
    FcRule *r;
    int n = 0, ret;

    if (!rs ||
       kind < FcMatchKindBegin || kind >= FcMatchKindEnd)
	return -1;
    FcPtrListIterInitAtLast (rs->subst[kind], &iter);
    if (!FcPtrListIterAdd (rs->subst[kind], &iter, rule))
	return -1;

    for (r = rule; r; r = r->next)
    {
	switch (r->type)
	{
	case FcRuleTest:
	    if (r->u.test)
	    {
		if (r->u.test->kind == FcMatchDefault)
		    r->u.test->kind = kind;
		if (n < r->u.test->object)
		    n = r->u.test->object;
	    }
	    break;
	case FcRuleEdit:
	    if (n < r->u.edit->object)
		n = r->u.edit->object;
	    break;
	default:
	    break;
	}
    }
    if (FcDebug () & FC_DBG_EDIT)
    {
	printf ("Add Rule(kind:%d, name: %s) ", kind, rs->name);
	FcRulePrint (rule);
    }
    ret = FC_OBJ_ID (n) - FC_MAX_BASE_OBJECT;

    return ret < 0 ? 0 : ret;
}

void
FcConfigFileInfoIterInit (FcConfig		*config,
			  FcConfigFileInfoIter	*iter)
{
    FcConfig *c;
    FcPtrListIter *i = (FcPtrListIter *)iter;

    if (!config)
	c = FcConfigGetCurrent ();
    else
	c = config;
    FcPtrListIterInit (c->rulesetList, i);
}

FcBool
FcConfigFileInfoIterNext (FcConfig		*config,
			  FcConfigFileInfoIter	*iter)
{
    FcConfig *c;
    FcPtrListIter *i = (FcPtrListIter *)iter;

    if (!config)
	c = FcConfigGetCurrent ();
    else
	c = config;
    if (FcPtrListIterIsValid (c->rulesetList, i))
    {
	FcPtrListIterNext (c->rulesetList, i);
    }
    else
	return FcFalse;

    return FcTrue;
}

FcBool
FcConfigFileInfoIterGet (FcConfig		*config,
			 FcConfigFileInfoIter	*iter,
			 FcChar8		**name,
			 FcChar8		**description,
			 FcBool			*enabled)
{
    FcConfig *c;
    FcRuleSet *r;
    FcPtrListIter *i = (FcPtrListIter *)iter;

    if (!config)
	c = FcConfigGetCurrent ();
    else
	c = config;
    if (!FcPtrListIterIsValid (c->rulesetList, i))
	return FcFalse;
    r = FcPtrListIterGetValue (c->rulesetList, i);
    if (name)
	*name = FcStrdup (r->name && r->name[0] ? r->name : (const FcChar8 *) "fonts.conf");
    if (description)
	*description = FcStrdup (!r->description ? _("No description") :
				 dgettext (r->domain ? (const char *) r->domain : GETTEXT_PACKAGE "-conf",
					   (const char *) r->description));
    if (enabled)
	*enabled = r->enabled;

    return FcTrue;
}

#define __fccfg__
#include "fcaliastail.h"
#undef __fccfg__
