/*
 * Copyright (C) 2009 Behdad Esfahbod
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 */

#include <fribidi.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

/* Glib array types */
typedef struct {
  int capacity;
  int len;
  int *data;
} int_array_t;

typedef struct {
  int capacity;
  int len;
  char *data;
} char_array_t;

#define LINE_SIZE 2048 /* Size of largest example line in BidiTest */
#define ARRAY_CHUNK_SIZE 32
int_array_t *new_int_array()
{
  int_array_t *arr = (int_array_t*)malloc(sizeof(int_array_t));
  arr->len = 0;
  arr->capacity = ARRAY_CHUNK_SIZE;
  arr->data = (int*)malloc(arr->capacity * sizeof(int));

  return arr;
}

void int_array_add(int_array_t *arr, int val)
{
  if (arr->len == arr->capacity)
    {
      arr->capacity += ARRAY_CHUNK_SIZE;
      arr->data = (int*)realloc(arr->data, arr->capacity*sizeof(int));
    }
  arr->data[arr->len++] = val;
}

int *int_array_free(int_array_t *arr, int free_data)
{
  int *data = arr->data;
  if (free_data) {
    data = NULL;
    free(arr->data);
  }
  free(arr);
  return data;
}

char_array_t *new_char_array()
{
  char_array_t *arr = (char_array_t*)malloc(sizeof(char_array_t));
  arr->len = 0;
  arr->capacity = ARRAY_CHUNK_SIZE;
  arr->data = (char*)malloc(arr->capacity);

  return arr;
}

void char_array_add(char_array_t *arr, char val)
{
  if (arr->len == arr->capacity)
    {
      arr->capacity += ARRAY_CHUNK_SIZE;
      arr->data = (char*)realloc(arr->data, arr->capacity * sizeof(char));
    }
  arr->data[arr->len++] = val;
}

char *char_array_free(char_array_t *arr, int free_data)
{
  char *data = arr->data;
  if (free_data) {
    data = NULL;
    free(arr->data);
  }
  free(arr);
  return data;
}

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt); 
    
    vfprintf(stderr, fmt, ap);
    exit(-1);
}

static FriBidiCharType
parse_char_type (const char *s, int len)
{
#define MATCH(name, value) \
    if (!strncmp (name, s, len) && name[len] == '\0') return value;

    MATCH ("L",   FRIBIDI_TYPE_LTR);
    MATCH ("R",   FRIBIDI_TYPE_RTL);
    MATCH ("AL",  FRIBIDI_TYPE_AL);
    MATCH ("EN",  FRIBIDI_TYPE_EN);
    MATCH ("AN",  FRIBIDI_TYPE_AN);
    MATCH ("ES",  FRIBIDI_TYPE_ES);
    MATCH ("ET",  FRIBIDI_TYPE_ET);
    MATCH ("CS",  FRIBIDI_TYPE_CS);
    MATCH ("NSM", FRIBIDI_TYPE_NSM);
    MATCH ("BN",  FRIBIDI_TYPE_BN);
    MATCH ("B",   FRIBIDI_TYPE_BS);
    MATCH ("S",   FRIBIDI_TYPE_SS);
    MATCH ("WS",  FRIBIDI_TYPE_WS);
    MATCH ("ON",  FRIBIDI_TYPE_ON);
    MATCH ("LRE", FRIBIDI_TYPE_LRE);
    MATCH ("RLE", FRIBIDI_TYPE_RLE);
    MATCH ("LRO", FRIBIDI_TYPE_LRO);
    MATCH ("RLO", FRIBIDI_TYPE_RLO);
    MATCH ("PDF", FRIBIDI_TYPE_PDF);
    MATCH ("LRI", FRIBIDI_TYPE_LRI);
    MATCH ("RLI", FRIBIDI_TYPE_RLI);
    MATCH ("FSI", FRIBIDI_TYPE_FSI);
    MATCH ("PDI", FRIBIDI_TYPE_PDI);

    die("Oops. I shouldn't reach here!\n");
    return -1;
}

static FriBidiLevel *
parse_levels_line (const char *line,
		   FriBidiLevel *len)
{
    char_array_t *levels;

    if (!strncmp (line, "@Levels:", 8))
	line += 8;

    levels = new_char_array ();

    while (*line)
    {
	FriBidiLevel l;
	char *end;

	errno = 0;
	l = strtol (line, &end, 10);
	if (errno != EINVAL && line != end)
	{
	  char_array_add (levels, l);
	  line = end;
	  continue;
	}

	while (isspace (*line))
	  line++;

	if (*line == 'x')
	{
	  char_array_add (levels, -1);
	  line++;
	  continue;
	}

	if (!*line)
	  break;

	die("Oops. I shouldn't be here!\n");
    }

    *len = levels->len;
    return (FriBidiLevel *) char_array_free(levels, FALSE);
}

static FriBidiStrIndex *
parse_reorder_line (const char *line,
		    FriBidiStrIndex *len)
{
    int_array_t *map;
    FriBidiStrIndex l;
    char *end;

    if (!strncmp (line, "@Reorder:", 9))
	line += 9;

    map = new_int_array ();

    for(; errno = 0, l = strtol (line, &end, 10), line != end && errno != EINVAL; line = end) {
	int_array_add (map, l);
    }

    *len = map->len;
    return (FriBidiStrIndex *) int_array_free (map, FALSE);
}

static FriBidiCharType *
parse_test_line (const char *line,
	         FriBidiStrIndex *len,
		 int *base_dir_flags)
{
    int_array_t *types;
    FriBidiCharType c;
    const char *end;

    types = new_int_array();

    for(;;) {
	while (isspace (*line))
	    line++;
	end = line;
	while (isalpha (*end))
	    end++;
	if (line == end)
	    break;

	c = parse_char_type (line, end - line);
	int_array_add (types, c);

	line = end;
    }

    if (*line == ';')
	line++;
    *base_dir_flags = strtol (line, NULL, 10);

    *len = types->len;
    return (FriBidiCharType *) int_array_free (types, FALSE);
}

int
main (int argc, char **argv)
{
    FILE *channel;
    char line[LINE_SIZE];
    FriBidiStrIndex *expected_ltor = NULL;
    FriBidiStrIndex expected_ltor_len = 0;
    FriBidiStrIndex *ltor = NULL;
    FriBidiStrIndex ltor_len = 0;
    FriBidiCharType *types = NULL;
    FriBidiStrIndex types_len = 0;
    FriBidiLevel *expected_levels = NULL;
    FriBidiLevel expected_levels_len = 0;
    FriBidiLevel *levels = NULL;
    FriBidiStrIndex levels_len = 0;
    int base_dir_flags, base_dir_mode;
    int numerrs = 0;
    int numtests = 0;
    int line_no = 0;
    int debug = FALSE;
    const char *filename;
    int next_arg;

    if (argc < 2) 
	die ("usage: %s [--debug] test-file-name\n", argv[0]);

    next_arg = 1;
    if (!strcmp (argv[next_arg], "--debug")) {
	debug = TRUE;
	next_arg++;
    }

    filename = argv[next_arg++];

    channel = fopen(filename, "r");
    if (!channel) 
	die ("Failed opening %s\n", filename);

    while (!feof(channel)) {
        fgets(line, LINE_SIZE, channel);
        int len = strlen(line);
        if (len == LINE_SIZE-1)
          die("LINE_SIZE too small at line %d!\n", line_no);

	line_no++;

	if (line[0] == '#')
	    continue;

	if (line[0] == '@')
	{
	    if (!strncmp (line, "@Reorder:", 9)) {
		free (expected_ltor);
		expected_ltor = parse_reorder_line (line, &expected_ltor_len);
		continue;
	    }
	    if (!strncmp (line, "@Levels:", 8)) {
		free (expected_levels);
		expected_levels = parse_levels_line (line, &expected_levels_len);
		continue;
	    }
	    continue;
	}

	/* Test line */
	free (types);
	types = parse_test_line (line, &types_len, &base_dir_flags);

	free (levels);
	levels = malloc (sizeof (FriBidiLevel) * types_len);
	levels_len = types_len;

	free (ltor);
	ltor = malloc (sizeof (FriBidiStrIndex) * types_len);

	/* Test it */
	for (base_dir_mode = 0; base_dir_mode < 3; base_dir_mode++) {
	    FriBidiParType base_dir;
	    int i, j;
	    int matches;

	    if ((base_dir_flags & (1<<base_dir_mode)) == 0)
		continue;

            numtests++;

	    switch (base_dir_mode) {
	    case 0: base_dir = FRIBIDI_PAR_ON;  break;
	    case 1: base_dir = FRIBIDI_PAR_LTR; break;
	    case 2: base_dir = FRIBIDI_PAR_RTL; break;
	    }

	    if (fribidi_get_par_embedding_levels_ex (types,
                                                     NULL, /* Brackets are not used in the BidiTest.txt file */
                                                     types_len,
                                                     &base_dir,
                                                     levels))
            {}

	    for (i = 0; i < types_len; i++)
	        ltor[i] = i;

	    if (fribidi_reorder_line (0 /*FRIBIDI_FLAG_REORDER_NSM*/,
                                      types, types_len,
                                      0, base_dir,
                                      levels,
                                      NULL,
                                      ltor))
            {}

	    j = 0;
	    for (i = 0; i < types_len; i++)
	    	if (!FRIBIDI_IS_EXPLICIT_OR_BN (types[ltor[i]]))
		    ltor[j++] = ltor[i];
	    ltor_len = j;

	    /* Compare */
	    matches = TRUE;
	    if (levels_len != expected_levels_len)
		matches = FALSE;
	    if (matches)
		for (i = 0; i < levels_len; i++)
		    if (levels[i] != expected_levels[i] &&
			expected_levels[i] != (FriBidiLevel) -1) {
			matches = FALSE;
			break;
		    }

	    if (ltor_len != expected_ltor_len)
		matches = FALSE;
	    if (matches)
		for (i = 0; i < ltor_len; i++)
		    if (ltor[i] != expected_ltor[i]) {
			matches = FALSE;
			break;
		    }

	    if (!matches)
	    {
		numerrs++;

		fprintf (stderr, "failure on line %d\n", line_no);
		fprintf (stderr, "input is: %s\n", line);
		fprintf (stderr, "base dir: %s\n",
                         base_dir_mode==0 ? "auto"
                         : base_dir_mode==1 ? "LTR" : "RTL");

		fprintf (stderr, "expected levels:");
		for (i = 0; i < expected_levels_len; i++)
		    if (expected_levels[i] == (FriBidiLevel) -1)
                        fprintf (stderr," x");
		    else
                        fprintf (stderr, " %d", expected_levels[i]);
		fprintf (stderr, "\n");
		fprintf (stderr, "returned levels:");
		for (i = 0; i < levels_len; i++)
                    fprintf (stderr, " %d", levels[i]);
		fprintf (stderr, "\n");

		fprintf (stderr, "expected order:");
		for (i = 0; i < expected_ltor_len; i++)
                    fprintf (stderr, " %d", expected_ltor[i]);
		fprintf (stderr, "\n");
		fprintf (stderr, "returned order:");
		for (i = 0; i < ltor_len; i++)
                    fprintf (stderr, " %d", ltor[i]);
		fprintf (stderr, "\n");

		if (debug)
                  {
		    FriBidiParType base_dir;

		    fribidi_set_debug (1);

		    switch (base_dir_mode) {
		    case 0: base_dir = FRIBIDI_PAR_ON;  break;
		    case 1: base_dir = FRIBIDI_PAR_LTR; break;
		    case 2: base_dir = FRIBIDI_PAR_RTL; break;
		    }

		    if (fribidi_get_par_embedding_levels_ex (types,
                                                             NULL, /* No bracket types */
                                                             types_len,
                                                             &base_dir,
                                                             levels))
                    {}

		    fribidi_set_debug (0);
		}

		fprintf (stderr, "\n");
	    }
	}
    }

    free (ltor);
    free (levels);
    free (expected_ltor);
    free (expected_levels);
    free (types);
    fclose(channel);

    if (numerrs)
        fprintf (stderr, "%d errors out of %d total tests\n", numerrs, numtests);
    else
        printf("No errors found! :-)\n");

    return numerrs;
}
