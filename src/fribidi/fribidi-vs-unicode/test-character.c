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
#include <glib.h>
#include <ctype.h>
#include <errno.h>

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
  GArray *code_points_array, *levels_array, *visual_ordering_array;
  char *end;
  int level;


  code_points_array = g_array_new (FALSE, FALSE, sizeof (FriBidiChar));
  levels_array = g_array_new (FALSE, FALSE, sizeof (FriBidiLevel));

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
      g_array_append_val (code_points_array, c);

      line = end;
    }

  *code_points_len = code_points_array->len;
  *code_points = (FriBidiChar *) g_array_free (code_points_array, FALSE);

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
          g_array_append_val (levels_array, level);
          line = end;
          continue;
        }

      while (isspace (*line))
        line++;

      if (*line == 'x')
        {
          level = (FriBidiLevel) -1;
          g_array_append_val (levels_array, level);
          line++;
          continue;
        }

      if (*line == ';')
        break;

      g_assert_not_reached ();
    }

  if (levels_array->len != *code_points_len)
    die("Oops! Different lengths for levels and codepoints at line %d!\n", line_no);

  *resolved_levels = (FriBidiLevel*)g_array_free (levels_array, FALSE);

  if (*line == ';')
    line++;
  else
    die("Oops! Didn't find expected ; at line %d\n", line_no);

  /* Field 4 - resulting visual ordering */
  visual_ordering_array = g_array_new (FALSE, FALSE, sizeof(int));
  for(; errno = 0, level = strtol (line, &end, 10), line != end && errno != EINVAL; line = end) {
    g_array_append_val (visual_ordering_array, level);
  }

  *visual_ordering_len = visual_ordering_array->len;
  *visual_ordering = (int*)g_array_free (visual_ordering_array, FALSE);
}

int
main (int argc, char **argv)
{
  GError *error;
  int next_arg;
  GIOChannel *channel;
  GIOStatus status;
  const char *filename;
  gchar *line = NULL;
  gsize length, terminator_pos;
  int numerrs = 0;
  int line_no = 0;
  FriBidiChar *code_points = NULL;
  int code_points_len = 0;
  int expected_ltor_len = 0;
  int base_dir_mode = 0, paragraph_dir;
  FriBidiLevel *expected_levels = NULL;
  int *expected_ltor = NULL;
  int resolved_paragraph_embedding_level;
  FriBidiLevel *levels = NULL;
  FriBidiCharType *types = NULL;
  FriBidiBracketType *bracket_types = NULL;
  FriBidiStrIndex *ltor = NULL;
  int ltor_len;
  gboolean debug = FALSE;

  if (argc < 2)
    {
      g_printerr ("usage: %s [--debug] test-file-name\n", argv[0]);
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

  error = NULL;
  channel = g_io_channel_new_file (filename, "r", &error);
  if (!channel)
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  fribidi_set_debug(debug);

  while (TRUE)
    {
      error = NULL;
      g_free (line);
      status = g_io_channel_read_line (channel, &line, &length, &terminator_pos, &error);
      switch (status)
        {
        case G_IO_STATUS_ERROR:
          g_printerr ("%s\n", error->message);
          exit (1);

        case G_IO_STATUS_EOF:
          goto done;

        case G_IO_STATUS_AGAIN:
          continue;

        case G_IO_STATUS_NORMAL:
          line[terminator_pos] = '\0';
          break;
	}

      line_no++;

      if (line[0] == '#' || line[0] == '\0')
        continue;

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
      g_free(bracket_types);
      bracket_types = g_malloc ( sizeof(FriBidiBracketType) * code_points_len);

      g_free(types);
      types = g_malloc ( sizeof(FriBidiCharType) * code_points_len);

      g_free(levels);
      levels = g_malloc (sizeof (FriBidiLevel) * code_points_len);

      g_free (ltor);
      ltor = g_malloc (sizeof (FriBidiStrIndex) * code_points_len);


      {
        FriBidiParType base_dir;
        int i, j;
        gboolean matches;
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

        if ((paragraph_dir & (1<<base_dir_mode)) == 0)
          continue;

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

            g_printerr ("failure on line %d\n", line_no);
            g_printerr ("input is: %s\n", line);
            g_printerr ("base dir: %s\n", paragraph_dir==0 ? "LTR"
                        : paragraph_dir==1 ? "RTL" : "AUTO");

            g_printerr ("expected levels:");
            for (i = 0; i < code_points_len; i++)
              if (expected_levels[i] == (FriBidiLevel) -1)
                g_printerr (" x");
              else
                g_printerr (" %d", expected_levels[i]);
            g_printerr ("\n");
            g_printerr ("returned levels:");
            for (i = 0; i < levels_len; i++)
              g_printerr (" %d", levels[i]);
            g_printerr ("\n");

            g_printerr ("expected order:");
            for (i = 0; i < expected_ltor_len; i++)
              g_printerr (" %d", expected_ltor[i]);
            g_printerr ("\n");
            g_printerr ("returned order:");
            for (i = 0; i < ltor_len; i++)
              g_printerr (" %d", ltor[i]);
            g_printerr ("\n");

            if (debug)
              {
                FriBidiParType base_dir;

                fribidi_set_debug (1);

                switch (base_dir_mode)
                  {
                  case 0: base_dir = FRIBIDI_PAR_ON;  break;
                  case 1: base_dir = FRIBIDI_PAR_LTR; break;
                  case 2: base_dir = FRIBIDI_PAR_RTL; break;
                  }

                if (fribidi_get_par_embedding_levels_ex (types,
                                                         bracket_types,
                                                         types_len,
                                                         &base_dir,
                                                         levels))
                {}

                fribidi_set_debug (0);
              }

            g_printerr ("\n");
          }
      }
    }

done:
  if (error)
    g_error_free (error);

  if (numerrs)
    g_printerr ("%d errors\n", numerrs);
  else
    printf("No errors found! :-)\n");

  return numerrs;
}
