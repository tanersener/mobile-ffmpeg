/* FriBidi
 * gen-brackets-tab.c - generate brackets.tab.i
 *
 * Author:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld 2017
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002,2004 Behdad Esfahbod
 * Copyright (C) 2017 Dov Grobgeld
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
 * 
 * For licensing issues, contact <fribidi.license@gmail.com>.
 */

#include <common.h>
#include <ctype.h>
#include <fribidi-unicode.h>

#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "packtab.h"

#define appname "gen-brackets-tab"
#define outputname "brackets.tab.i"

static void
die (
  const char *msg
)
{
  fprintf (stderr, appname ": %s\n", msg);
  exit (1);
}

static void
die2 (
  const char *fmt,
  const char *p
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, p);
  fprintf (stderr, "\n");
  exit (1);
}

static void
die3 (
  const char *fmt,
  const char *p,
  const char *q
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, p, q);
  fprintf (stderr, "\n");
  exit (1);
}

static void
die4 (
  const char *fmt,
  unsigned long l,
  unsigned long p,
  unsigned long q
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, l, p, q);
  fprintf (stderr, "\n");
  exit (1);
}

#define table_name "Brk"
#define macro_name "FRIBIDI_GET_BRACKETS"

static signed int table[FRIBIDI_UNICODE_CHARS];
static signed int equiv_table[FRIBIDI_UNICODE_CHARS];
static char buf[4000];
static signed long max_dist;

static void
init (
  void
)
{
  max_dist = 0;
}

static void
clear_tabs (
  void
)
{
  register FriBidiChar c;

  for (c = 0; c < FRIBIDI_UNICODE_CHARS; c++)
    {
      table[c] = 0;
      equiv_table[c] = 0;
    }
}

static signed int table[FRIBIDI_UNICODE_CHARS];
static char buf[4000];

/* Read the canonical mapping of unicode characters and store them in the
   equiv_table array. */
static void
read_unicode_data_txt_equivalence (
  FILE *f
)
{
  unsigned long c, l;

  l = 0;
  while (fgets (buf, sizeof buf, f))
    {
      int i;
      const char *s = buf;
      char ce_string[256]; /* For parsing the equivalence */
      char *p = NULL;
      int ce, in_tag;

      l++;

      while (*s == ' ')
	s++;

      if (s[0] == '#' || s[0] == '\0' || s[0] == '\n')
	continue;
      /*  Field:       0 ; 1    ; 2    ; 3    ; 4    ; 5           */
      i = sscanf (s, "%lx;%*[^;];%*[^;];%*[^;];%*[^;];%[^;]", &c, ce_string);
      if (c >= FRIBIDI_UNICODE_CHARS)
        {
          fprintf (stderr, "invalid input at line %ld: %s", l, s);
          exit(1);
        }
      if (i==1)
        continue;

      /* split and parse ce */
      p = ce_string;
      ce = -1;
      in_tag = 0;
      while(*p)
        {
          if (*p==';')
            break;
          else if (*p=='<')
            in_tag = 1;
          else if (*p=='>')
            in_tag = 0;
          else if (!in_tag && isalnum(*p))
            {
              /* Assume we got a hexa decimal */
              ce = strtol(p,NULL,16);
              break;
            }
          p++;
        }

      /* FIXME: We don't handle First..Last parts of UnicodeData.txt,
       * but it works, since all those are LTR. */
      equiv_table[c] = ce;
    }
}

static void
read_bidi_brackets_txt (
  FILE *f
)
{
  unsigned long l;

  l = 0;
  while (fgets (buf, sizeof buf, f))
    {
      unsigned long i, j;
      signed long dist;
      int k;
      const char *s = buf;
      char open_close;

      l++;

      while (*s == ' ')
	s++;

      if (s[0] == '#' || s[0] == '\0' || s[0] == '\n')
	continue;

      k = sscanf (s, "%lx; %lx; %c", &i, &j, &open_close);
      if (k != 3 || i >= FRIBIDI_UNICODE_CHARS || j >= FRIBIDI_UNICODE_CHARS)
	die4 ("invalid pair in input at line %ld: %04lX, %04lX", l, i, j);

      /* Open braces map to themself */
      if (open_close=='o')
        j = i;
      
      /* Turn j into the unicode equivalence if it exists */
      if (equiv_table[j])
        {
          /* printf("Found match for %04x->%04x\n", j, equiv_table[j]); */
          j = equiv_table[j];
        }

      dist = ((signed long) j - (signed long) i);
      table[i] = dist;
      if (dist > max_dist)
	max_dist = dist;
      else if (-dist > max_dist)
	max_dist = -dist;
    }
}

static void
read_data (
  const char *bracket_datafile_type,
  const char *bracket_datafile_name,
  const char *uni_datafile_type,
  const char *uni_datafile_name
)
{
  FILE *f;

  clear_tabs ();

  if (!(f = fopen (uni_datafile_name, "rt")))
    die2 ("error: cannot open `%s' for reading", bracket_datafile_name);

  if (!strcmp (uni_datafile_type, "UnicodeData.txt"))
    read_unicode_data_txt_equivalence (f);
  else
    die2 ("error: unknown data-file-type %s", uni_datafile_type);

  fprintf (stderr, "Reading `%s'\n", bracket_datafile_name);
  if (!(f = fopen (bracket_datafile_name, "rt")))
    die2 ("error: cannot open `%s' for reading", bracket_datafile_name);

  if (!strcmp (bracket_datafile_type, "BidiBrackets.txt"))
    read_bidi_brackets_txt (f);
  else
    die2 ("error: unknown data-file-type %s", bracket_datafile_type);

  fclose (f);
}

static void
gen_brackets_tab (
  int max_depth,
  const char *data_file_type
)
{
  int key_bytes;
  const char *key_type;

  printf ("/* " outputname "\n * generated by " appname " (" FRIBIDI_NAME " "
	  FRIBIDI_VERSION ")\n" " * from the file %s of Unicode version "
	  FRIBIDI_UNICODE_VERSION ". */\n\n", data_file_type);

  printf ("#define PACKTAB_UINT8 uint8_t\n"
	  "#define PACKTAB_UINT16 uint16_t\n"
	  "#define PACKTAB_UINT32 uint32_t\n\n");

  key_bytes = max_dist <= 0x7f ? 1 : max_dist < 0x7fff ? 2 : 4;
  key_type = key_bytes == 1 ? "int8_t" : key_bytes == 2 ?
    "int16_t" : "int32_t";

  if (!pack_table
      (table, FRIBIDI_UNICODE_CHARS, key_bytes, 0, max_depth, 1, NULL,
       key_type, table_name, macro_name "_DELTA", stdout))
    die ("error: insufficient memory, decrease max_depth");

  printf ("#undef PACKTAB_UINT8\n"
	  "#undef PACKTAB_UINT16\n" "#undef PACKTAB_UINT32\n\n");

  printf ("#define " macro_name "(x) ((x) + " macro_name "_DELTA(x))\n\n");

  printf ("/* End of generated " outputname " */\n");
}

int
main (
  int argc,
  const char **argv
)
{
  const char *bracket_datafile_type = "BidiBrackets.txt";
  const char *uni_datafile_type = "UnicodeData.txt";

  if (argc < 4)
    die3 ("usage:\n  " appname " max-depth /path/to/%s /path/to/%s [junk...]",
	  bracket_datafile_type,
          uni_datafile_type);

  {
    int max_depth = atoi (argv[1]);
    const char *bracket_datafile_name = argv[2];
    const char *uni_datafile_name = argv[3];

    if (max_depth < 2)
      die ("invalid depth");

    init ();
    read_data (bracket_datafile_type, bracket_datafile_name,
               uni_datafile_type, uni_datafile_name);
    gen_brackets_tab (max_depth, bracket_datafile_type);
  }

  return 0;
}
