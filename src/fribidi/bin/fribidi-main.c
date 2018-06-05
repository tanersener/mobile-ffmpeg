/* FriBidi
 * fribidi-main.c - command line program for libfribidi
 *
 * Authors:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld, 1999, 2000
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002 Behdad Esfahbod
 * Copyright (C) 1999,2000 Dov Grobgeld
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

#include <fribidi.h>
#include <fribidi-deprecated.h>

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
# if STDC_HEADERS && HAVE_MEMORY_H
# else
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "getopt.h"

#define appname "fribidi"

#define MAX_STR_LEN 65000


#define ALLOCATE(tp,ln) ((tp *) fribidi_malloc (sizeof (tp) * (ln)))

static void
die2 (
  const char *fmt,
  const char *arg
)
{
  fprintf (stderr, "%s: ", appname);
  if (fmt)
    fprintf (stderr, fmt, arg);
  fprintf (stderr, "Try `%s --help' for more information.\n", appname);
  exit (-1);
}

#define die1(msg) die2("%s", msg)

fribidi_boolean do_break, do_pad, do_mirror, do_reorder_nsm, do_clean;
fribidi_boolean show_input, show_visual, show_basedir;
fribidi_boolean show_ltov, show_vtol, show_levels;
const int default_text_width = 80;
int text_width;
const char *char_set;
const char *bol_text, *eol_text;
FriBidiParType input_base_direction;
int char_set_num;

static void
help (
  void
)
{
  /* Break help string into little ones, to assure ISO C89 conformance */
  printf ("Usage: " appname " [OPTION]... [FILE]...\n"
	  "A command line interface for the " FRIBIDI_NAME " library.\n"
	  "Convert a logical string to visual.\n"
	  "\n"
	  "  -h, --help            Display this information and exit\n"
	  "  -V, --version         Display version information and exit\n"
	  "  -v, --verbose         Verbose mode, same as --basedir --ltov --vtol\n"
	  "                        --levels --changes\n");
  printf ("  -d, --debug           Output debug information\n"
	  "  -t, --test            Test " FRIBIDI_NAME
	  ", same as --clean --nobreak\n"
	  "                        --showinput --reordernsm --width %d\n",
	  default_text_width);
  printf ("  -c, --charset CS      Specify character set, default is %s\n"
	  "      --charsetdesc CS  Show descriptions for character set CS and exit\n"
	  "      --caprtl          Old style: set character set to CapRTL\n",
	  char_set);
  printf ("      --showinput       Output the input string too\n"
	  "      --nopad           Do not right justify RTL lines\n"
	  "      --nobreak         Do not break long lines\n"
	  "  -w, --width W         Screen width for padding, default is %d, but if\n"
	  "                        environment variable COLUMNS is defined, its value\n"
	  "                        will be used, --width overrides both of them.\n",
	  default_text_width);
  printf
    ("  -B, --bol BOL         Output string BOL before the visual string\n"
     "  -E, --eol EOL         Output string EOL after the visual string\n"
     "      --rtl             Force base direction to RTL\n"
     "      --ltr             Force base direction to LTR\n"
     "      --wrtl            Set base direction to RTL if no strong character found\n");
  printf
    ("      --wltr            Set base direction to LTR if no strong character found\n"
     "                        (default)\n"
     "      --nomirror        Turn mirroring off, to do it later\n"
     "      --reordernsm      Reorder NSM sequences to follow their base character\n"
     "      --clean           Remove explicit format codes in visual string\n"
     "                        output, currently does not affect other outputs\n"
     "      --basedir         Output Base Direction\n");
  printf ("      --ltov            Output Logical to Visual position map\n"
	  "      --vtol            Output Visual to Logical position map\n"
	  "      --levels          Output Embedding Levels\n"
	  "      --novisual        Do not output the visual string, to be used with\n"
	  "                        --basedir, --ltov, --vtol, --levels, --changes\n");
  printf ("  All string indexes are zero based\n" "\n" "Output:\n"
	  "  For each line of input, output something like this:\n"
	  "    [input-str` => '][BOL][[padding space]visual-str][EOL]\n"
	  "    [\\n base-dir][\\n ltov-map][\\n vtol-map][\\n levels][\\n changes]\n");

  {
    int i;
    printf ("\n" "Available character sets:\n");
    for (i = 1; i <= FRIBIDI_CHAR_SETS_NUM; i++)
      printf ("  * %-10s: %-25s%1s\n",
	      fribidi_char_set_name (i), fribidi_char_set_title (i),
	      (fribidi_char_set_desc (i) ? "X" : ""));
    printf
      ("  X: Character set has descriptions, use --charsetdesc to see\n");
  }

  printf ("\nReport bugs online at\n<" FRIBIDI_BUGREPORT ">.\n");
  exit (0);
}

static void
version (
  void
)
{
  printf (appname " %s", fribidi_version_info);
  exit (0);
}

static char *
my_fribidi_strdup (
  char *s
)
{
  char *m;

  m = fribidi_malloc (strlen (s) + 1);
  if (!m)
    return NULL;

  strcpy (m, s);

  return m;
}

int
main (
  int argc,
  char *argv[]
)
{
  int exit_val;
  fribidi_boolean file_found;
  char *s;
  FILE *IN;

  text_width = default_text_width;
  do_break = true;
  do_pad = true;
  do_mirror = true;
  do_clean = false;
  do_reorder_nsm = false;
  show_input = false;
  show_visual = true;
  show_basedir = false;
  show_ltov = false;
  show_vtol = false;
  show_levels = false;
  char_set = "UTF-8";
  bol_text = NULL;
  eol_text = NULL;
  input_base_direction = FRIBIDI_PAR_ON;

  if ((s = (char *) getenv ("COLUMNS")))
    {
      int i;

      i = atoi (s);
      if (i > 0)
	text_width = i;
    }

#define CHARSETDESC 257
#define CAPRTL 258

  /* Parse the command line with getopt library */
  /* Must set argv[0], getopt uses it to generate error messages */
  argv[0] = appname;
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"verbose", 0, 0, 'v'},
	{"debug", 0, 0, 'd'},
	{"test", 0, 0, 't'},
	{"charset", 1, 0, 'c'},
	{"charsetdesc", 1, 0, CHARSETDESC},
	{"caprtl", 0, 0, CAPRTL},
	{"showinput", 0, (int *) (void *) &show_input, true},
	{"nopad", 0, (int *) (void *) &do_pad, false},
	{"nobreak", 0, (int *) (void *) &do_break, false},
	{"width", 1, 0, 'w'},
	{"bol", 1, 0, 'B'},
	{"eol", 1, 0, 'E'},
	{"nomirror", 0, (int *) (void *) &do_mirror, false},
	{"reordernsm", 0, (int *) (void *) &do_reorder_nsm, true},
	{"clean", 0, (int *) (void *) &do_clean, true},
	{"ltr", 0, (int *) (void *) &input_base_direction, FRIBIDI_PAR_LTR},
	{"rtl", 0, (int *) (void *) &input_base_direction, FRIBIDI_PAR_RTL},
	{"wltr", 0, (int *) (void *) &input_base_direction,
	 FRIBIDI_PAR_WLTR},
	{"wrtl", 0, (int *) (void *) &input_base_direction,
	 FRIBIDI_PAR_WRTL},
	{"basedir", 0, (int *) (void *) &show_basedir, true},
	{"ltov", 0, (int *) (void *) &show_ltov, true},
	{"vtol", 0, (int *) (void *) &show_vtol, true},
	{"levels", 0, (int *) (void *) &show_levels, true},
	{"novisual", 0, (int *) (void *) &show_visual, false},
	{0, 0, 0, 0}
      };

      c =
	getopt_long (argc, argv, "hVvdtc:w:B:E:", long_options,
		     &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'h':
	  help ();
	  break;
	case 'V':
	  version ();
	  break;
	case 'v':
	  show_basedir = show_ltov = show_vtol = show_levels = true;
	  break;
	case 'w':
	  text_width = atoi (optarg);
	  if (text_width <= 0)
	    die2 ("invalid screen width `%s'\n", optarg);
	  break;
	case 'B':
	  bol_text = optarg;
	  break;
	case 'E':
	  eol_text = optarg;
	  break;
	case 'd':
	  if (!fribidi_set_debug (true))
	    die1
	      ("lib" FRIBIDI
	       " must be compiled with DEBUG option to enable\nturn debug info on.\n");
	  break;
	case 't':
	  do_clean = show_input = do_reorder_nsm = true;
	  do_break = false;
	  text_width = default_text_width;
	  break;
	case 'c':
	  char_set = my_fribidi_strdup (optarg);
	  if (!char_set)
	    die1 ("memory allocation failed for char_set!");
	  break;
	case CAPRTL:
	  char_set = "CapRTL";
	  break;
	case CHARSETDESC:
	  char_set = optarg;
	  char_set_num = fribidi_parse_charset (char_set);
	  if (!char_set_num)
	    die2 ("unrecognized character set `%s'\n", char_set);
	  if (!fribidi_char_set_desc (char_set_num))
	    die2 ("no description available for character set `%s'\n",
		  fribidi_char_set_name (char_set_num));
	  else
	    printf ("Descriptions for character set %s:\n"
		    "\n" "%s", fribidi_char_set_title (char_set_num),
		    fribidi_char_set_desc (char_set_num));
	  exit (0);
	  break;
	case ':':
	case '?':
	  die2 (NULL, NULL);
	  break;
	default:
	  break;
	}
    }

  char_set_num = fribidi_parse_charset (char_set);

  if (!char_set_num)
    die2 ("unrecognized character set `%s'\n", char_set);

FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
  fribidi_set_mirroring (do_mirror);
  fribidi_set_reorder_nsm (do_reorder_nsm);
FRIBIDI_END_IGNORE_DEPRECATIONS
  exit_val = 0;
  file_found = false;
  while (optind < argc || !file_found)
    {
      const char *filename;

      filename = optind < argc ? argv[optind++] : "-";
      file_found = true;

      /* Open the infile for reading */
      if (filename[0] == '-' && !filename[1])
	{
	  IN = stdin;
	}
      else
	{
	  IN = fopen (filename, "r");
	  if (!IN)
	    {
	      fprintf (stderr, "%s: %s: no such file or directory\n",
		       appname, filename);
	      exit_val = 1;
	      continue;
	    }
	}

      /* Read and process input one line at a time */
      {
	char S_[MAX_STR_LEN];
	int padding_width, break_width;

	padding_width = show_input ? (text_width - 10) / 2 : text_width;
	break_width = do_break ? padding_width : 3 * MAX_STR_LEN;

	while (fgets (S_, sizeof (S_) - 1, IN))
	  {
	    const char *new_line, *nl_found;
	    FriBidiChar logical[MAX_STR_LEN];
	    char outstring[MAX_STR_LEN];
	    FriBidiParType base;
	    FriBidiStrIndex len;

	    nl_found = "";
	    S_[sizeof (S_) - 1] = 0;
	    len = strlen (S_);
	    /* chop */
	    if (S_[len - 1] == '\n')
	      {
		len--;
		S_[len] = '\0';
		new_line = "\n";
	      }
	    else
	      new_line = "";
	    /* TODO: handle \r */

	    len = fribidi_charset_to_unicode (char_set_num, S_, len, logical);

	    {
	      FriBidiChar *visual;
	      FriBidiStrIndex *ltov, *vtol;
	      FriBidiLevel *levels;
	      fribidi_boolean log2vis;

	      visual = show_visual ? ALLOCATE (FriBidiChar,
					       len + 1
	      ) : NULL;
	      ltov = show_ltov ? ALLOCATE (FriBidiStrIndex,
					   len + 1
	      ) : NULL;
	      vtol = show_vtol ? ALLOCATE (FriBidiStrIndex,
					   len + 1
	      ) : NULL;
	      levels = show_levels ? ALLOCATE (FriBidiLevel,
					       len + 1
	      ) : NULL;

	      /* Create a bidi string. */
	      base = input_base_direction;
FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
	      log2vis = fribidi_log2vis (logical, len, &base,
					 /* output */
					 visual, ltov, vtol, levels);
FRIBIDI_END_IGNORE_DEPRECATIONS
	      if (log2vis)
		{

		  if (show_input)
		    printf ("%-*s => ", padding_width, S_);

		  /* Remove explicit marks, if asked for. */
FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
		  if (do_clean)
		    len =
		      fribidi_remove_bidi_marks (visual, len, ltov, vtol,
						 levels);
FRIBIDI_END_IGNORE_DEPRECATIONS

		  if (show_visual)
		    {
		      printf ("%s", nl_found);

		      if (bol_text)
			printf ("%s", bol_text);

		      /* Convert it to input charset and print. */
		      {
			FriBidiStrIndex idx, st;
			for (idx = 0; idx < len;)
			  {
			    FriBidiStrIndex wid, inlen;

			    wid = break_width;
			    st = idx;
			    if (char_set_num != FRIBIDI_CHAR_SET_CAP_RTL)
			      while (wid > 0 && idx < len)
				{
				  wid -=
				    FRIBIDI_IS_EXPLICIT_OR_ISOLATE_OR_BN_OR_NSM
				    (fribidi_get_bidi_type (visual[idx])) ? 0
				    : 1;
				  idx++;
				}
			    else
			      while (wid > 0 && idx < len)
				{
				  wid--;
				  idx++;
				}
			    if (wid < 0 && idx - st > 1)
			      idx--;
			    inlen = idx - st;

			    fribidi_unicode_to_charset (char_set_num,
							visual + st, inlen,
							outstring);
			    if (FRIBIDI_IS_RTL (base))
			      printf ("%*s",
				      (int) (do_pad ? (padding_width +
						       strlen (outstring) -
						       (break_width -
							wid)) : 0),
				      outstring);
			    else
			      printf ("%s", outstring);
			    if (idx < len)
			      printf ("\n");
			  }
		      }
		      if (eol_text)
			printf ("%s", eol_text);

		      nl_found = "\n";
		    }
		  if (show_basedir)
		    {
		      printf ("%s", nl_found);
		      printf ("Base direction: %s",
			      (FRIBIDI_DIR_TO_LEVEL (base) ? "R" : "L"));
		      nl_found = "\n";
		    }
		  if (show_ltov)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) ltov[i]);
		      nl_found = "\n";
		    }
		  if (show_vtol)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) vtol[i]);
		      nl_found = "\n";
		    }
		  if (show_levels)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%d ", (int) levels[i]);
		      nl_found = "\n";
		    }
		}
	      else
		{
		  exit_val = 2;
		}

	      if (show_visual)
		free (visual);
	      if (show_ltov)
		free (ltov);
	      if (show_vtol)
		free (vtol);
	      if (show_levels)
		free (levels);
	    }

	    if (*nl_found)
	      printf ("%s", new_line);
	  }
      }
    }

  return exit_val;
}

/* Editor directions:
 * vim:textwidth=78:tabstop=8:shiftwidth=2:autoindent:cindent
 */
