/*
 * fontconfig/fc-lang/fc-lang.c
 *
 * Copyright © 2002 Keith Packard
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

#include "fccharset.c"
#include "fcstr.c"
#include "fcserialize.c"

/*
 * fc-lang
 *
 * Read a set of language orthographies and build C declarations for
 * charsets which can then be used to identify which languages are
 * supported by a given font.  Note that this uses some utilities
 * from the fontconfig library, so the necessary file is simply
 * included in this compilation.  A couple of extra utility
 * functions are also needed in slightly modified form
 */

FcPrivate void
FcCacheObjectReference (void *object FC_UNUSED)
{
}

FcPrivate void
FcCacheObjectDereference (void *object FC_UNUSED)
{
}

FcPrivate FcChar8 *
FcLangNormalize (const FcChar8 *lang FC_UNUSED)
{
    return NULL;
}

int FcDebugVal;

FcChar8 *
FcConfigHome (void)
{
    return (FcChar8 *) getenv ("HOME");
}

static void 
fatal (const char *file, int lineno, const char *msg)
{
    if (lineno)
	fprintf (stderr, "%s:%d: %s\n", file, lineno, msg);
    else
	fprintf (stderr, "%s: %s\n", file, msg);
    exit (1);
}

static char *
get_line (FILE *f, char *buf, int *lineno)
{
    char    *hash;
    char    *line;
    int	    end;

next:
    line = buf;
    if (!fgets (line, 1024, f))
	return 0;
    ++(*lineno);
    hash = strchr (line, '#');
    if (hash)
	*hash = '\0';

    while (line[0] && isspace (line[0]))
      line++;
    end = strlen (line);
    while (end > 0 && isspace (line[end-1]))
      line[--end] = '\0';

    if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
      goto next;

    return line;
}

static char	*dir = 0;

static FILE *
scanopen (char *file)
{
    FILE    *f;

    f = fopen (file, "r");
    if (!f && dir)
    {
	char	path[1024];
	
	strcpy (path, dir);
	strcat (path, "/");
	strcat (path, file);
	f = fopen (path, "r");
    }
    return f;
}

/*
 * build a single charset from a source file
 *
 * The file format is quite simple, either
 * a single hex value or a pair separated with a dash
 *
 * Comments begin with '#'
 */

static FcCharSet *
scan (FILE *f, char *file, FcCharSetFreezer *freezer)
{
    FcCharSet	    *c = 0;
    FcCharSet	    *n;
    FcBool	    del;
    int		    start, end, ucs4;
    char	    buf[1024];
    char	    *line;
    int		    lineno = 0;

    while ((line = get_line (f, buf, &lineno)))
    {
	if (!strncmp (line, "include", 7))
	{
	    FILE *included_f;
	    char *included_file;
	    included_file = strchr (line, ' ');
            if (!included_file)
                fatal (file, lineno,
                       "invalid syntax, expected: include filename");
	    while (isspace(*included_file))
		included_file++;
	    included_f = scanopen (included_file);
	    if (!included_f)
		fatal (included_file, 0, "can't open");
	    n = scan (included_f, included_file, freezer);
	    fclose (included_f);
	    if (!c)
		c = FcCharSetCreate ();
	    if (!FcCharSetMerge (c, n, NULL))
		fatal (file, lineno, "out of memory");
	    FcCharSetDestroy (n);
	    continue;
	}
	del = FcFalse;
	if (line[0] == '-')
	{
	  del = FcTrue;
	  line++;
	}
	if (strchr (line, '-'))
	{
	    if (sscanf (line, "%x-%x", &start, &end) != 2)
		fatal (file, lineno, "parse error");
	}
	else if (strstr (line, ".."))
	{
	    if (sscanf (line, "%x..%x", &start, &end) != 2)
		fatal (file, lineno, "parse error");
	}
	else
	{
	    if (sscanf (line, "%x", &start) != 1)
		fatal (file, lineno, "parse error");
	    end = start;
	}
	if (!c)
	    c = FcCharSetCreate ();
	for (ucs4 = start; ucs4 <= end; ucs4++)
	{
	    if (!((del ? FcCharSetDelChar : FcCharSetAddChar) (c, ucs4)))
		fatal (file, lineno, "out of memory");
	}
    }
    n = (FcCharSet *) FcCharSetFreeze (freezer, c);
    FcCharSetDestroy (c);
    return n;
}

/*
 * Convert a file name into a name suitable for C declarations
 */
static char *
get_name (char *file)
{
    char    *name;
    char    *dot;

    dot = strchr (file, '.');
    if (!dot)
	dot = file + strlen(file);
    name = malloc (dot - file + 1);
    strncpy (name, file, dot - file);
    name[dot-file] = '\0';
    return name;
}

/*
 * Convert a C name into a language name
 */
static char *
get_lang (char *name)
{
    char    *lang = malloc (strlen (name) + 1);
    char    *l = lang;
    char    c;

    while ((c = *name++))
    {
	if (isupper ((int) (unsigned char) c))
	    c = tolower ((int) (unsigned char) c);
	if (c == '_')
	    c = '-';
	if (c == ' ')
	    continue;
	*l++ = c;
    }
    *l++ = '\0';
    return lang;
}

typedef struct _Entry {
    int id;
    char *file;
} Entry;

static int compare (const void *a, const void *b)
{
    const Entry *as = a, *bs = b;
    return FcStrCmpIgnoreCase ((const FcChar8 *) as->file, (const FcChar8 *) bs->file);
}

#define MAX_LANG	    1024
#define MAX_LANG_SET_MAP    ((MAX_LANG + 31) / 32)

#define BitSet(map, i)   ((map)[(entries[i].id)>>5] |= ((FcChar32) 1 << ((entries[i].id) & 0x1f)))

int
main (int argc FC_UNUSED, char **argv)
{
    static Entry	entries[MAX_LANG + 1];
    static FcCharSet	*sets[MAX_LANG];
    static int		duplicate[MAX_LANG];
    static int		country[MAX_LANG];
    static char		*names[MAX_LANG];
    static char		*langs[MAX_LANG];
    static int		off[MAX_LANG];
    FILE	*f;
    int		ncountry = 0;
    int		i = 0;
    int		nsets = 0;
    int		argi;
    FcCharLeaf	**leaves;
    int		total_leaves = 0;
    int		l, sl, tl, tn;
    static char		line[1024];
    static FcChar32	map[MAX_LANG_SET_MAP];
    int		num_lang_set_map;
    int		setRangeStart[26];
    int		setRangeEnd[26];
    FcChar8	setRangeChar;
    FcCharSetFreezer	*freezer;
    
    freezer = FcCharSetFreezerCreate ();
    if (!freezer)
	fatal (argv[0], 0, "out of memory");
    argi = 1;
    while (argv[argi])
    {
	if (!strcmp (argv[argi], "-d"))
	{
	    argi++;
	    dir = argv[argi++];
	    continue;
	}
	if (i == MAX_LANG)
	    fatal (argv[0], 0, "Too many languages");
	entries[i].id = i;
	entries[i].file = argv[argi++];
	i++;
    }
    entries[i].file = 0;
    qsort (entries, i, sizeof (Entry), compare);
    i = 0;
    while (entries[i].file)
    {
	f = scanopen (entries[i].file);
	if (!f)
	    fatal (entries[i].file, 0, strerror (errno));
	sets[i] = scan (f, entries[i].file, freezer);
	names[i] = get_name (entries[i].file);
	langs[i] = get_lang(names[i]);
	if (strchr (langs[i], '-'))
	    country[ncountry++] = i;

	total_leaves += sets[i]->num;
	i++;
	fclose (f);
    }
    nsets = i;
    sets[i] = 0;
    leaves = malloc (total_leaves * sizeof (FcCharLeaf *));
    tl = 0;
    /*
     * Find unique leaves
     */
    for (i = 0; sets[i]; i++)
    {
	for (sl = 0; sl < sets[i]->num; sl++)
	{
	    for (l = 0; l < tl; l++)
		if (leaves[l] == FcCharSetLeaf(sets[i], sl))
		    break;
	    if (l == tl)
		leaves[tl++] = FcCharSetLeaf(sets[i], sl);
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
    
    printf ("/* total size: %d unique leaves: %d */\n\n",
	    total_leaves, tl);

    /*
     * Find duplicate charsets
     */
    duplicate[0] = -1;
    for (i = 1; sets[i]; i++)
    {
	int j;

	duplicate[i] = -1;
	for (j = 0; j < i; j++)
	    if (sets[j] == sets[i])
	    {
		duplicate[i] = j;
		break;
	    }
    }

    tn = 0;
    for (i = 0; sets[i]; i++) {
	if (duplicate[i] >= 0)
	    continue;
	off[i] = tn;
	tn += sets[i]->num;
    }

    printf ("#define LEAF0       (%d * sizeof (FcLangCharSet))\n", nsets);
    printf ("#define OFF0        (LEAF0 + %d * sizeof (FcCharLeaf))\n", tl);
    printf ("#define NUM0        (OFF0 + %d * sizeof (uintptr_t))\n", tn);
    printf ("#define SET(n)      (n * sizeof (FcLangCharSet) + offsetof (FcLangCharSet, charset))\n");
    printf ("#define OFF(s,o)    (OFF0 + o * sizeof (uintptr_t) - SET(s))\n");
    printf ("#define NUM(s,n)    (NUM0 + n * sizeof (FcChar16) - SET(s))\n");
    printf ("#define LEAF(o,l)   (LEAF0 + l * sizeof (FcCharLeaf) - (OFF0 + o * sizeof (intptr_t)))\n");
    printf ("#define fcLangCharSets (fcLangData.langCharSets)\n");
    printf ("#define fcLangCharSetIndices (fcLangData.langIndices)\n");
    printf ("#define fcLangCharSetIndicesInv (fcLangData.langIndicesInv)\n");
    printf ("\n");
    
    printf ("static const struct {\n"
	    "    FcLangCharSet  langCharSets[%d];\n"
	    "    FcCharLeaf     leaves[%d];\n"
	    "    uintptr_t      leaf_offsets[%d];\n"
	    "    FcChar16       numbers[%d];\n"
	    "    FcChar%s       langIndices[%d];\n"
	    "    FcChar%s       langIndicesInv[%d];\n"
	    "} fcLangData = {\n",
	    nsets, tl, tn, tn,
	    nsets < 256 ? "8 " : "16", nsets, nsets < 256 ? "8 " : "16", nsets);
	
    /*
     * Dump sets
     */

    printf ("{\n");
    for (i = 0; sets[i]; i++)
    {
	int	j = duplicate[i];

	if (j < 0)
	    j = i;

	printf ("    { \"%s\", "
		" { FC_REF_CONSTANT, %d, OFF(%d,%d), NUM(%d,%d) } }, /* %d */\n",
		langs[i],
		sets[j]->num, i, off[j], i, off[j], i);
    }
    printf ("},\n");
    
    /*
     * Dump leaves
     */
    printf ("{\n");
    for (l = 0; l < tl; l++)
    {
	printf ("    { { /* %d */", l);
	for (i = 0; i < 256/32; i++)
	{
	    if (i % 4 == 0)
		printf ("\n   ");
	    printf (" 0x%08x,", leaves[l]->map[i]);
	}
	printf ("\n    } },\n");
    }
    printf ("},\n");

    /*
     * Dump leaves
     */
    printf ("{\n");
    for (i = 0; sets[i]; i++)
    {
	int n;
	
	if (duplicate[i] >= 0)
	    continue;
	printf ("    /* %s */\n", names[i]);
	for (n = 0; n < sets[i]->num; n++)
	{
	    if (n % 4 == 0)
		printf ("   ");
	    for (l = 0; l < tl; l++)
		if (leaves[l] == FcCharSetLeaf(sets[i], n))
		    break;
	    if (l == tl)
		fatal (names[i], 0, "can't find leaf");
	    printf (" LEAF(%3d,%3d),", off[i], l);
	    if (n % 4 == 3)
		printf ("\n");
	}
	if (n % 4 != 0)
	    printf ("\n");
    }
    printf ("},\n");
	

    printf ("{\n");
    for (i = 0; sets[i]; i++)
    {
	int n;
	
	if (duplicate[i] >= 0)
	    continue;
	printf ("    /* %s */\n", names[i]);
	for (n = 0; n < sets[i]->num; n++)
	{
	    if (n % 8 == 0)
		printf ("   ");
	    printf (" 0x%04x,", FcCharSetNumbers (sets[i])[n]);
	    if (n % 8 == 7)
		printf ("\n");
	}
	if (n % 8 != 0)
	    printf ("\n");
    }
    printf ("},\n");

    /* langIndices */
    printf ("{\n");
    for (i = 0; sets[i]; i++)
    {
	printf ("    %d, /* %s */\n", entries[i].id, names[i]);
    }
    printf ("},\n");

    /* langIndicesInv */
    printf ("{\n");
    {
	static int		entries_inv[MAX_LANG];
	for (i = 0; sets[i]; i++)
	  entries_inv[entries[i].id] = i;
	for (i = 0; sets[i]; i++)
	    printf ("    %d, /* %s */\n", entries_inv[i], names[entries_inv[i]]);
    }
    printf ("}\n");

    printf ("};\n\n");

    printf ("#define NUM_LANG_CHAR_SET	%d\n", i);
    num_lang_set_map = (i + 31) / 32;
    printf ("#define NUM_LANG_SET_MAP	%d\n", num_lang_set_map);
    /*
     * Dump indices with country codes
     */
    if (ncountry)
    {
	int	c;
	int	ncountry_ent = 0;
	printf ("\n");
	printf ("static const FcChar32 fcLangCountrySets[][NUM_LANG_SET_MAP] = {\n");
	for (c = 0; c < ncountry; c++)
	{
	    i = country[c];
	    if (i >= 0)
	    {
		int lang = strchr (langs[i], '-') - langs[i];
		int d, k;

		for (k = 0; k < num_lang_set_map; k++)
		    map[k] = 0;

		BitSet (map, i);
		for (d = c + 1; d < ncountry; d++)
		{
		    int j = country[d];
		    if (j >= 0 && !strncmp (langs[j], langs[i], lang + 1))
		    {
			BitSet(map, j);
			country[d] = -1;
		    }
		}
		printf ("    {");
		for (k = 0; k < num_lang_set_map; k++)
		    printf (" 0x%08x,", map[k]);
		printf (" }, /* %*.*s */\n",
			lang, lang, langs[i]);
		++ncountry_ent;
	    }
	}
	printf ("};\n\n");
	printf ("#define NUM_COUNTRY_SET %d\n", ncountry_ent);
    }
    

    /*
     * Find ranges for each letter for faster searching
     */
    setRangeChar = 'a';
    memset(setRangeStart, '\0', sizeof (setRangeStart));
    memset(setRangeEnd, '\0', sizeof (setRangeEnd));
    for (i = 0; sets[i]; i++)
    {
	char	c = names[i][0];
	
	while (setRangeChar <= c && c <= 'z')
	    setRangeStart[setRangeChar++ - 'a'] = i;
    }
    while (setRangeChar <= 'z') /* no language code starts with these letters */
	setRangeStart[setRangeChar++ - 'a'] = i;

    for (setRangeChar = 'a'; setRangeChar < 'z'; setRangeChar++)
	setRangeEnd[setRangeChar - 'a'] = setRangeStart[setRangeChar+1-'a'] - 1;
    setRangeEnd[setRangeChar - 'a'] = i - 1;
    
    /*
     * Dump sets start/finish for the fastpath
     */
    printf ("\n");
    printf ("static const FcLangCharSetRange  fcLangCharSetRanges[] = {\n");
	printf ("\n");
    for (setRangeChar = 'a'; setRangeChar <= 'z' ; setRangeChar++)
    {
	printf ("    { %d, %d }, /* %c */\n",
		setRangeStart[setRangeChar - 'a'],
		setRangeEnd[setRangeChar - 'a'], setRangeChar);
    }
    printf ("};\n\n");
 
    while (fgets (line, sizeof (line), stdin))
	fputs (line, stdout);
    
    fflush (stdout);
    exit (ferror (stdout));
}
