/*
 * Copyright (C) 2015, 2017 Dov Grobgeld
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

#include "fribidi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define FALSE 0
#define TRUE 1
#define LINE_SIZE 2048 /* Size of biggest line in test file */

/* Glib like arrays */
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

static
FriBidiChar parse_uni_char(const char *start, int len)
{
  return strtoul(start, NULL, 16);
}

static
void parse_test_line (char *line,
                      int line_no,
                      FriBidiChar **code_points,      /* Field 0 */
                      int *code_points_len,
                      int *paragraph_dir,    /* Field 1 */
                      int *resolved_paragraph_embedding_level,   /* Field 2 */
                      FriBidiLevel **resolved_levels,   /* Field 3 */
                      int **visual_ordering,    /* Field 4 */
                      int *visual_ordering_len
                      )
{
  int_array_t *code_points_array, *visual_ordering_array;
  char_array_t *levels_array;
  char *end;
  int level;

  code_points_array = new_int_array ();
  levels_array = new_char_array ();

  /* Field 0. Code points */
  for(;;)
    {
      FriBidiChar c;
      while (isspace (*line))
        line++;
      end = line;
      while (isxdigit (*end))
        end++;
      if (line == end)
        break;

      c = parse_uni_char (line, end - line);
      int_array_add(code_points_array, c);

      line = end;
    }

  *code_points_len = code_points_array->len;
  *code_points = (FriBidiChar *) int_array_free (code_points_array, FALSE);

  if (*line == ';')
    line++;
  else
    die("Oops! Didn't find expected ;\n");

  /* Field 1. Paragraph direction */
  end = line;
  while (isdigit (*end))
    end++;
  *paragraph_dir = atoi(line);
  line = end;

  if (*line == ';')
    line++;
  else
    die("Oops! Didn't find expected ;\n");

  /* Field 2. resolved paragraph_dir */
  end = line;
  while (isdigit (*end))
    end++;
  *resolved_paragraph_embedding_level = atoi(line);
  line = end;

  if (*line == ';')
    line++;
  else
    die("Oops! Didn't find expected ; at line %d\n", line_no);

  while (*line)
    {
      FriBidiLevel level;
      char *end;

      errno = 0;
      level = strtol (line, &end, 10);
      if (errno != EINVAL && line != end)
        {
          char_array_add (levels_array, level);
          line = end;
          continue;
        }

      while (isspace (*line))
        line++;

      if (*line == 'x')
        {
          level = (FriBidiLevel) -1;
          char_array_add (levels_array, level);
          line++;
          continue;
        }

      if (*line == ';')
        break;

      die("Oops! I shouldn't be here!\n");
    }

  if (levels_array->len != *code_points_len)
    die("Oops! Different lengths for levels and codepoints at line %d!\n", line_no);

  *resolved_levels = (FriBidiLevel*)char_array_free (levels_array, FALSE);

  if (*line == ';')
    line++;
  else
    die("Oops! Didn't find expected ; at line %d\n", line_no);

  /* Field 4 - resulting visual ordering */
  visual_ordering_array = new_int_array ();
  for(; errno = 0, level = strtol (line, &end, 10), line != end && errno != EINVAL; line = end) {
    int_array_add (visual_ordering_array, level);
  }

  *visual_ordering_len = visual_ordering_array->len;
  *visual_ordering = (int*)int_array_free (visual_ordering_array, FALSE);
}

int
main (int argc, char **argv)
{
  int next_arg;
  FILE *channel;
  const char *filename;
  char line[LINE_SIZE];
  int numerrs = 0;
  int line_no = 0;
  FriBidiChar *code_points = NULL;
  int code_points_len = 0;
  int expected_ltor_len = 0;
  int paragraph_dir = 0;
  FriBidiLevel *expected_levels = NULL;
  int *expected_ltor = NULL;
  int resolved_paragraph_embedding_level;
  FriBidiLevel *levels = NULL;
  FriBidiCharType *types = NULL;
  FriBidiBracketType *bracket_types = NULL;
  FriBidiStrIndex *ltor = NULL;
  int ltor_len;
  int debug = FALSE;

  if (argc < 2)
    {
      fprintf (stderr, "usage: %s [--debug] test-file-name\n", argv[0]);
      exit (1);
    }

  next_arg = 1;
  while(next_arg < argc && argv[next_arg][0]=='-')
    {
      const char *arg = argv[next_arg++];
      if (strcmp(arg, "--debug")==0)
        {
          debug=TRUE;
          continue;
        }
      die("Unknown option %s!\n", arg);
    }

    filename = argv[next_arg++];

    channel = fopen(filename, "r");
    if (!channel) 
	die ("Failed opening %s\n", filename);

    while (!feof(channel)) {
      fgets(line, LINE_SIZE, channel);
      int len = strlen(line);
      if (len == LINE_SIZE-1)
        die("LINE_SIZE=%d too small at line %d!\n", LINE_SIZE, line_no);

      line_no++;

      if (line[0] == '#' || line[0] == '\n')
        continue;

      free (code_points);
      free (expected_levels);
      free (expected_ltor);
      free (bracket_types);
      free (types);
      free (levels);
      free (ltor);

      parse_test_line (line,
                       line_no,
                       &code_points,      /* Field 0 */
                       &code_points_len,
                       &paragraph_dir,    /* Field 1 */
                       &resolved_paragraph_embedding_level,   /* Field 2 */
                       &expected_levels,   /* Field 3 */
                       &expected_ltor,    /* Field 4 */
                       &expected_ltor_len
                       );

      /* Test it */
      bracket_types = malloc ( sizeof(FriBidiBracketType) * code_points_len);
      types = malloc ( sizeof(FriBidiCharType) * code_points_len);
      levels = malloc (sizeof (FriBidiLevel) * code_points_len);
      ltor = malloc (sizeof (FriBidiStrIndex) * code_points_len);


      {
        FriBidiParType base_dir;
        int i, j;
        int matches;
        int types_len = code_points_len;
        int levels_len = types_len;
        FriBidiBracketType NoBracket = FRIBIDI_NO_BRACKET;

        for (i=0; i<code_points_len; i++)
          {
            types[i] = fribidi_get_bidi_type(code_points[i]);

            /* Note the optimization that a bracket is always
               of type neutral */
            if (types[i] == FRIBIDI_TYPE_ON)
                bracket_types[i] = fribidi_get_bracket(code_points[i]);
            else
                bracket_types[i] = NoBracket;
          }

        switch (paragraph_dir)
          {
          case 0: base_dir = FRIBIDI_PAR_LTR; break;
          case 1: base_dir = FRIBIDI_PAR_RTL; break;
          case 2: base_dir = FRIBIDI_PAR_ON;  break;
          }

        if (fribidi_get_par_embedding_levels_ex (types,
                                                 bracket_types,
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
        if (matches)
          for (i = 0; i < code_points_len; i++)
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
            fprintf (stderr, "base dir: %s\n", paragraph_dir==0 ? "LTR"
                        : paragraph_dir==1 ? "RTL" : "AUTO");

            fprintf (stderr, "expected levels:");
            for (i = 0; i < code_points_len; i++)
              if (expected_levels[i] == (FriBidiLevel) -1)
                fprintf (stderr, " x");
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
                fribidi_set_debug (1);

                if (fribidi_get_par_embedding_levels_ex (types,
                                                         bracket_types,
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

  if (numerrs)
    fprintf (stderr, "%d errors\n", numerrs);
  else
    printf("No errors found! :-)\n");

  free (code_points);
  free (expected_levels);
  free (expected_ltor);
  free (bracket_types);
  free (types);
  free (levels);
  free (ltor);

  return numerrs;
}
