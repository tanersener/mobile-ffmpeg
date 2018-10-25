/*
 * fontconfig/test/test-conf.c
 *
 * Copyright © 2000 Keith Packard
 * Copyright © 2018 Akira TAGOH
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
#include <stdio.h>
#include <string.h>
#include <fontconfig/fontconfig.h>
#include <json.h>

struct _FcConfig {
    FcStrSet	*configDirs;	    /* directories to scan for fonts */
    FcStrSet	*fontDirs;
    FcStrSet	*cacheDirs;
    FcStrSet	*configFiles;	    /* config files loaded */
    void	*subst[FcMatchKindEnd];
    int		maxObjects;	    /* maximum number of tests in all substs */
    FcStrSet	*acceptGlobs;
    FcStrSet	*rejectGlobs;
    FcFontSet	*acceptPatterns;
    FcFontSet	*rejectPatterns;
    FcFontSet	*fonts[FcSetApplication + 1];
};

static FcPattern *
build_pattern (json_object *obj)
{
    json_object_iter iter;
    FcPattern *pat = FcPatternCreate ();

    json_object_object_foreachC (obj, iter)
    {
	FcValue v;

	if (json_object_get_type (iter.val) == json_type_boolean)
	{
	    v.type = FcTypeBool;
	    v.u.b = json_object_get_boolean (iter.val);
	}
	else if (json_object_get_type (iter.val) == json_type_double)
	{
	    v.type = FcTypeDouble;
	    v.u.d = json_object_get_double (iter.val);
	}
	else if (json_object_get_type (iter.val) == json_type_int)
	{
	    v.type = FcTypeInteger;
	    v.u.i = json_object_get_int (iter.val);
	}
	else if (json_object_get_type (iter.val) == json_type_string)
	{
	    v.type = FcTypeString;
	    v.u.s = json_object_get_string (iter.val);
	}
	else if (json_object_get_type (iter.val) == json_type_null)
	{
	    v.type = FcTypeVoid;
	}
	else
	{
	    fprintf (stderr, "W: unexpected object to build a pattern: (%s %s)", iter.key, json_type_to_name (json_object_get_type (iter.val)));
	    continue;
	}
	FcPatternAdd (pat, iter.key, v, FcTrue);
    }
    return pat;
}

static FcBool
build_fonts (FcConfig *config, json_object *root)
{
    json_object *fonts;
    FcFontSet *fs;
    int i, n;

    if (!json_object_object_get_ex (root, "fonts", &fonts) ||
	json_object_get_type (fonts) != json_type_array)
    {
	fprintf (stderr, "W: No fonts defined\n");
	return FcFalse;
    }
    fs = FcFontSetCreate ();
    n = json_object_array_length (fonts);
    for (i = 0; i < n; i++)
    {
	json_object *obj = json_object_array_get_idx (fonts, i);
	FcPattern *pat;

	if (json_object_get_type (obj) != json_type_object)
	    continue;
	pat = build_pattern (obj);
	FcFontSetAdd (fs, pat);
    }
    /* FcConfigSetFonts (config, fs, FcSetSystem); */
    if (config->fonts[FcSetSystem])
	FcFontSetDestroy (config->fonts[FcSetSystem]);
    config->fonts[FcSetSystem] = fs;

    return FcTrue;
}

static FcBool
run_test (FcConfig *config, json_object *root)
{
    json_object *tests;
    FcFontSet *fs;
    int i, n, fail = 0;

    if (!json_object_object_get_ex (root, "tests", &tests) ||
	json_object_get_type (tests) != json_type_array)
    {
	fprintf (stderr, "W: No test cases defined\n");
	return FcFalse;
    }
    fs = FcFontSetCreate ();
    n = json_object_array_length (tests);
    for (i = 0; i < n; i++)
    {
	json_object *obj = json_object_array_get_idx (tests, i);
	json_object_iter iter;
	FcPattern *query, *result;
	const char *method;

	if (json_object_get_type (obj) != json_type_object)
	    continue;
	json_object_object_foreachC (obj, iter)
	{
	    if (strcmp (iter.key, "method") == 0)
	    {
		if (json_object_get_type (iter.val) != json_type_string)
		{
		    fprintf (stderr, "W: invalid type of method: (%s)\n", json_type_to_name (json_object_get_type (iter.val)));
		    continue;
		}
		method = json_object_get_string (iter.val);
	    }
	    else if (strcmp (iter.key, "query") == 0)
	    {
		if (json_object_get_type (iter.val) != json_type_object)
		{
		    fprintf (stderr, "W: invalid type of query: (%s)\n", json_type_to_name (json_object_get_type (iter.val)));
		    continue;
		}
		query = build_pattern (iter.val);
	    }
	    else if (strcmp (iter.key, "result") == 0)
	    {
		if (json_object_get_type (iter.val) != json_type_object)
		{
		    fprintf (stderr, "W: invalid type of result: (%s)\n", json_type_to_name (json_object_get_type (iter.val)));
		    continue;
		}
		result = build_pattern (iter.val);
	    }
	    else
	    {
		fprintf (stderr, "W: unknown object: %s\n", iter.key);
	    }
	}
	if (strcmp (method, "match") == 0)
	{
	    FcPattern *match;
	    FcResult res;

	    FcConfigSubstitute (config, query, FcMatchPattern);
	    FcDefaultSubstitute (query);
	    match = FcFontMatch (config, query, &res);
	    if (match)
	    {
		FcPatternIter iter;
		int x, vc;

		FcPatternIterStart (result, &iter);
		do
		{
		    vc = FcPatternIterValueCount (result, &iter);
		    for (x = 0; x < vc; x++)
		    {
			FcValue vr, vm;

			if (FcPatternIterGetValue (result, &iter, x, &vr, NULL) != FcResultMatch)
			{
			    fprintf (stderr, "E: unable to obtain a value from the expected result\n");
			    fail++;
			    goto bail;
			}
			if (FcPatternGet (match, FcPatternIterGetObject (result, &iter), x, &vm) != FcResultMatch)
			{
			    vm.type = FcTypeVoid;
			}
			if (!FcValueEqual (vm, vr))
			{
			    printf ("E: failed to compare %s:\n", FcPatternIterGetObject (result, &iter));
			    printf ("   actual result:");
			    FcValuePrint (vm);
			    printf ("\n   expected result:");
			    FcValuePrint (vr);
			    printf ("\n");
			    fail++;
			    goto bail;
			}
		    }
		} while (FcPatternIterNext (result, &iter));
	    bail:;
	    }
	    else
	    {
		fprintf (stderr, "E: no match\n");
		fail++;
	    }
	}
	else
	{
	    fprintf (stderr, "W: unknown testing method: %s\n", method);
	}
    }

    return fail == 0;
}

static FcBool
run_scenario (FcConfig *config, char *file)
{
    FcBool ret = FcTrue;
    json_object *root, *scenario;

    root = json_object_from_file (file);
    if (!root)
    {
	fprintf (stderr, "E: Unable to read the file: %s\n", file);
	return FcFalse;
    }
    if (!build_fonts (config, root))
    {
	ret = FcFalse;
	goto bail1;
    }
    if (!run_test (config, root))
    {
	ret = FcFalse;
	goto bail1;
    }

bail1:
    json_object_put (root);

    return ret;
}

static FcBool
load_config (FcConfig *config, char *file)
{
    FILE *fp;
    long len;
    char *buf = NULL;
    FcBool ret = FcTrue;

    if ((fp = fopen(file, "rb")) == NULL)
	return FcFalse;
    fseek (fp, 0L, SEEK_END);
    len = ftell (fp);
    fseek (fp, 0L, SEEK_SET);
    buf = malloc (sizeof (char) * (len + 1));
    if (!buf)
    {
	ret = FcFalse;
	goto bail1;
    }
    fread (buf, (size_t)len, sizeof (char), fp);
    buf[len] = 0;

    ret = FcConfigParseAndLoadFromMemory (config, buf, FcTrue);
bail1:
    fclose (fp);
    if (buf)
	free (buf);

    return ret;
}

int
main (int argc, char **argv)
{
    FcConfig *config;
    int retval = 0;

    if (argc < 3)
    {
	fprintf(stderr, "Usage: %s <conf file> <test scenario>\n", argv[0]);
	return 1;
    }

    config = FcConfigCreate ();
    if (!load_config (config, argv[1]))
    {
	fprintf(stderr, "E: Failed to load config\n");
	retval = 1;
	goto bail1;
    }
    if (!run_scenario (config, argv[2]))
    {
	retval = 1;
	goto bail1;
    }
bail1:
    FcConfigDestroy (config);

    return retval;
}
