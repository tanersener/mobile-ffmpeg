/*
 * fontconfig/fc-case/fc-case.c
 *
 * Copyright © 2004 Keith Packard
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

#include "fcint.h"
#include <ctype.h>

#define MAX_OUT	    32
#define MAX_LINE    8192

typedef enum _caseFoldClass { CaseFoldCommon, CaseFoldFull, CaseFoldSimple, CaseFoldTurkic } CaseFoldClass;

typedef struct _caseFoldClassMap {
    const char	    *name;
    CaseFoldClass   class;
} CaseFoldClassMap;

static const CaseFoldClassMap	caseFoldClassMap[] = {
    { "C", CaseFoldCommon },
    { "F", CaseFoldFull },
    { "S", CaseFoldSimple },
    { "T", CaseFoldTurkic },
    { 0, 0 }
};

typedef struct _caseFoldRaw {
    FcChar32	    upper;
    CaseFoldClass   class;
    int		    nout;
    FcChar32	    lower[MAX_OUT];
} CaseFoldRaw;

static void
panic (const char *reason)
{
    fprintf (stderr, "fc-case: panic %s\n", reason);
    exit (1);
}

int			maxExpand;
static FcCaseFold	*folds;
int			nfolds;

static FcCaseFold *
addFold (void)
{
    if (folds)
	folds = realloc (folds, (nfolds + 1) * sizeof (FcCaseFold));
    else
	folds = malloc (sizeof (FcCaseFold));
    if (!folds)
	panic ("out of memory");
    return &folds[nfolds++];
}

static int
ucs4_to_utf8 (FcChar32	ucs4,
	      FcChar8	dest[FC_UTF8_MAX_LEN])
{
    int	bits;
    FcChar8 *d = dest;
    
    if      (ucs4 <       0x80) {  *d++=  ucs4;                         bits= -6; }
    else if (ucs4 <      0x800) {  *d++= ((ucs4 >>  6) & 0x1F) | 0xC0;  bits=  0; }
    else if (ucs4 <    0x10000) {  *d++= ((ucs4 >> 12) & 0x0F) | 0xE0;  bits=  6; }
    else if (ucs4 <   0x200000) {  *d++= ((ucs4 >> 18) & 0x07) | 0xF0;  bits= 12; }
    else if (ucs4 <  0x4000000) {  *d++= ((ucs4 >> 24) & 0x03) | 0xF8;  bits= 18; }
    else if (ucs4 < 0x80000000) {  *d++= ((ucs4 >> 30) & 0x01) | 0xFC;  bits= 24; }
    else return 0;

    for ( ; bits >= 0; bits-= 6) {
	*d++= ((ucs4 >> bits) & 0x3F) | 0x80;
    }
    return d - dest;
}

static int
utf8_size (FcChar32 ucs4)
{
    FcChar8 utf8[FC_UTF8_MAX_LEN];
    return ucs4_to_utf8 (ucs4, utf8 );
}

static FcChar8	*foldChars;
static int	nfoldChars;
static int	maxFoldChars;
static FcChar32	minFoldChar;
static FcChar32	maxFoldChar;

static void
addChar (FcChar32 c)
{
    FcChar8	utf8[FC_UTF8_MAX_LEN];
    int		len;
    int		i;

    len = ucs4_to_utf8 (c, utf8);
    if (foldChars)
	foldChars = realloc (foldChars, (nfoldChars + len) * sizeof (FcChar8));
    else
	foldChars = malloc (sizeof (FcChar8) * len);
    if (!foldChars)
	panic ("out of memory");
    for (i = 0; i < len; i++)
	foldChars[nfoldChars + i] = utf8[i];
    nfoldChars += len;
}

static int
foldExtends (FcCaseFold *fold, CaseFoldRaw *raw)
{
    switch (fold->method) {
    case FC_CASE_FOLD_RANGE:
	if ((short) (raw->lower[0] - raw->upper) != fold->offset)
	    return 0;
	if (raw->upper != fold->upper + fold->count)
	    return 0;
	return 1;
    case FC_CASE_FOLD_EVEN_ODD:
	if ((short) (raw->lower[0] - raw->upper) != 1)
	    return 0;
	if (raw->upper != fold->upper + fold->count + 1)
	    return 0;
	return 1;
    case FC_CASE_FOLD_FULL:
	break;
    }
    return 0;
}
	    
static const char *
case_fold_method_name (FcChar16 method)
{
    switch (method) {
    case FC_CASE_FOLD_RANGE:	return "FC_CASE_FOLD_RANGE,";
    case FC_CASE_FOLD_EVEN_ODD: return "FC_CASE_FOLD_EVEN_ODD,";
    case FC_CASE_FOLD_FULL:	return "FC_CASE_FOLD_FULL,";
    default:			return "unknown";
    }
}

static void
dump (void)
{
    int	    i;
    
    printf (   "#define FC_NUM_CASE_FOLD	%d\n", nfolds);
    printf (   "#define FC_NUM_CASE_FOLD_CHARS	%d\n", nfoldChars);
    printf (   "#define FC_MAX_CASE_FOLD_CHARS	%d\n", maxFoldChars);
    printf (   "#define FC_MAX_CASE_FOLD_EXPAND	%d\n", maxExpand);
    printf (   "#define FC_MIN_FOLD_CHAR	0x%08x\n", minFoldChar);
    printf (   "#define FC_MAX_FOLD_CHAR	0x%08x\n", maxFoldChar);
    printf (   "\n");
    
    /*
     * Dump out ranges
     */
    printf ("static const FcCaseFold    fcCaseFold[FC_NUM_CASE_FOLD] = {\n");
    for (i = 0; i < nfolds; i++)
    {
	printf ("    { 0x%08x, %-22s 0x%04x, %6d },\n",
		folds[i].upper, case_fold_method_name (folds[i].method),
		folds[i].count, folds[i].offset);
    }
    printf ("};\n\n");

    /*
     * Dump out "other" values
     */

    printf ("static const FcChar8	fcCaseFoldChars[FC_NUM_CASE_FOLD_CHARS] = {\n");
    for (i = 0; i < nfoldChars; i++)
    {
	printf ("0x%02x", foldChars[i]);
	if (i != nfoldChars - 1)
	{
	    if ((i & 0xf) == 0xf) 
		printf (",\n");
	    else
		printf (",");
	}
    }
    printf ("\n};\n");
}

/*
 * Read the standard Unicode CaseFolding.txt file
 */
#define SEP "; \t\n"

static int
parseRaw (char *line, CaseFoldRaw *raw)
{
    char    *tok, *end;
    int	    i;
    
    if (!isxdigit (line[0]))
	return 0;
    /*
     * Get upper case value
     */
    tok = strtok (line, SEP);
    if (!tok || tok[0] == '#')
	return 0;
    raw->upper = strtol (tok, &end, 16);
    if (end == tok)
	return 0;
    /*
     * Get class
     */
    tok = strtok (NULL, SEP);
    if (!tok || tok[0] == '#')
	return 0;
    for (i = 0; caseFoldClassMap[i].name; i++)
	if (!strcmp (tok, caseFoldClassMap[i].name))
	{
	    raw->class = caseFoldClassMap[i].class;
	    break;
	}
    if (!caseFoldClassMap[i].name)
	return 0;
	
    /*
     * Get list of result characters
     */
    for (i = 0; i < MAX_OUT; i++)
    {
	tok = strtok (NULL, SEP);
	if (!tok || tok[0] == '#')
	    break;
	raw->lower[i] = strtol (tok, &end, 16);
	if (end == tok)
	    break;
    }
    if (i == 0)
	return 0;
    raw->nout = i;
    return 1;
}

static int
caseFoldReadRaw (FILE *in, CaseFoldRaw *raw)
{
    char    line[MAX_LINE];

    for (;;)
    {
	if (!fgets (line, sizeof (line) - 1, in))
	    return 0;
	if (parseRaw (line, raw))
	    return 1;
    }
}

int
main (int argc, char **argv)
{
    FcCaseFold		*fold = 0;
    CaseFoldRaw		raw;
    int			i;
    FILE		*caseFile;
    char		line[MAX_LINE];
    int			expand;

    if (argc != 2)
	panic ("usage: fc-case CaseFolding.txt");
    caseFile = fopen (argv[1], "r");
    if (!caseFile)
	panic ("can't open case folding file");
    
    while (caseFoldReadRaw (caseFile, &raw))
    {
	if (!minFoldChar)
	    minFoldChar = raw.upper;
	maxFoldChar = raw.upper;
	switch (raw.class) {
	case CaseFoldCommon:
	case CaseFoldFull:
	    if (raw.nout == 1)
	    {
		if (fold && foldExtends (fold, &raw))
		    fold->count = raw.upper - fold->upper + 1;
		else
		{
		    fold = addFold ();
		    fold->upper = raw.upper;
		    fold->offset = raw.lower[0] - raw.upper;
		    if (fold->offset == 1)
			fold->method = FC_CASE_FOLD_EVEN_ODD;
		    else
			fold->method = FC_CASE_FOLD_RANGE;
		    fold->count = 1;
		}
		expand = utf8_size (raw.lower[0]) - utf8_size(raw.upper);
	    }
	    else
	    {
		fold = addFold ();
		fold->upper = raw.upper;
		fold->method = FC_CASE_FOLD_FULL;
		fold->offset = nfoldChars;
		for (i = 0; i < raw.nout; i++)
		    addChar (raw.lower[i]);
		fold->count = nfoldChars - fold->offset;
		if (fold->count > maxFoldChars)
		    maxFoldChars = fold->count;
		expand = fold->count - utf8_size (raw.upper);
	    }
	    if (expand > maxExpand)
		maxExpand = expand;
	    break;
	case CaseFoldSimple:
	    break;
	case CaseFoldTurkic:
	    break;
	}
    }
    /*
     * Scan the input until the marker is found
     */
    
    while (fgets (line, sizeof (line), stdin))
    {
	if (!strncmp (line, "@@@", 3))
	    break;
	fputs (line, stdout);
    }
    
    /*
     * Dump these tables
     */
    dump ();
    
    /*
     * And flush out the rest of the input file
     */

    while (fgets (line, sizeof (line), stdin))
	fputs (line, stdout);
    
    fflush (stdout);
    exit (ferror (stdout));
}
