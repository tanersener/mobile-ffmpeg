%{
/* Parse a string into an internal timestamp.

   Copyright (C) 1999-2000, 2002-2019 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Originally written by Steven M. Bellovin <smb@research.att.com> while
   at the University of North Carolina at Chapel Hill.  Later tweaked by
   a couple of people on Usenet.  Completely overhauled by Rich $alz
   <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.

   Modified by Assaf Gordon <assafgordon@gmail.com> in 2016 to add
   debug output.

   Modified by Paul Eggert <eggert@twinsun.com> in 1999 to do the
   right thing about local DST.  Also modified by Paul Eggert
   <eggert@cs.ucla.edu> in 2004 to support nanosecond-resolution
   timestamps, in 2004 to support TZ strings in dates, and in 2017 to
   check for integer overflow and to support longer-than-'long'
   'time_t' and 'tv_nsec'.  */

#include <config.h>

#include "parse-datetime.h"

#include "intprops.h"
#include "timespec.h"
#include "verify.h"
#include "strftime.h"

/* There's no need to extend the stack, so there's no need to involve
   alloca.  */
#define YYSTACK_USE_ALLOCA 0

/* Tell Bison how much stack space is needed.  20 should be plenty for
   this grammar, which is not right recursive.  Beware setting it too
   high, since that might cause problems on machines whose
   implementations have lame stack-overflow checking.  */
#define YYMAXDEPTH 20
#define YYINITDEPTH YYMAXDEPTH

/* Since the code of parse-datetime.y is not included in the Emacs executable
   itself, there is no need to #define static in this file.  Even if
   the code were included in the Emacs executable, it probably
   wouldn't do any harm to #undef it here; this will only cause
   problems if we try to write to a static variable, which I don't
   think this code needs to do.  */
#ifdef emacs
# undef static
#endif

#include <inttypes.h>
#include <c-ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"

#define _(str) gettext (str)

/* Bison's skeleton tests _STDLIB_H, while some stdlib.h headers
   use _STDLIB_H_ as witness.  Map the latter to the one bison uses.  */
/* FIXME: this is temporary.  Remove when we have a mechanism to ensure
   that the version we're using is fixed, too.  */
#ifdef _STDLIB_H_
# undef _STDLIB_H
# define _STDLIB_H 1
#endif

/* The __attribute__ feature is available in gcc versions 2.5 and later.
   The __-protected variants of the attributes 'format' and 'printf' are
   accepted by gcc versions 2.6.4 (effectively 2.7) and later.
   Enable _GL_ATTRIBUTE_FORMAT only if these are supported too, because
   gnulib and libintl do '#define printf __printf__' when they override
   the 'printf' function.  */
#if 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
# define _GL_ATTRIBUTE_FORMAT(spec) __attribute__ ((__format__ spec))
#else
# define _GL_ATTRIBUTE_FORMAT(spec) /* empty */
#endif

/* Shift A right by B bits portably, by dividing A by 2**B and
   truncating towards minus infinity.  A and B should be free of side
   effects, and B should be in the range 0 <= B <= INT_BITS - 2, where
   INT_BITS is the number of useful bits in an int.  GNU code can
   assume that INT_BITS is at least 32.

   ISO C99 says that A >> B is implementation-defined if A < 0.  Some
   implementations (e.g., UNICOS 9.0 on a Cray Y-MP EL) don't shift
   right in the usual way when A < 0, so SHR falls back on division if
   ordinary A >> B doesn't seem to be the usual signed shift.  */
#define SHR(a, b)       \
  (-1 >> 1 == -1        \
   ? (a) >> (b)         \
   : (a) / (1 << (b)) - ((a) % (1 << (b)) < 0))

#define HOUR(x) (60 * 60 * (x))

#define STREQ(a, b) (strcmp (a, b) == 0)

/* Verify that time_t is an integer as POSIX requires, and that every
   time_t value fits in intmax_t.  Please file a bug report if these
   assumptions are false on your platform.  */
verify (TYPE_IS_INTEGER (time_t));
verify (!TYPE_SIGNED (time_t) || INTMAX_MIN <= TYPE_MINIMUM (time_t));
verify (TYPE_MAXIMUM (time_t) <= INTMAX_MAX);

/* True if N is out of range for time_t.  */
static bool
time_overflow (intmax_t n)
{
  return ! ((TYPE_SIGNED (time_t) ? TYPE_MINIMUM (time_t) <= n : 0 <= n)
            && n <= TYPE_MAXIMUM (time_t));
}

/* Convert a possibly-signed character to an unsigned character.  This is
   a bit safer than casting to unsigned char, since it catches some type
   errors that the cast doesn't.  */
static unsigned char to_uchar (char ch) { return ch; }

static void _GL_ATTRIBUTE_FORMAT ((__printf__, 1, 2))
dbg_printf (char const *msg, ...)
{
  va_list args;
  /* TODO: use gnulib's 'program_name' instead?  */
  fputs ("date: ", stderr);

  va_start (args, msg);
  vfprintf (stderr, msg, args);
  va_end (args);
}


/* An integer value, and the number of digits in its textual
   representation.  */
typedef struct
{
  bool negative;
  intmax_t value;
  ptrdiff_t digits;
} textint;

/* An entry in the lexical lookup table.  */
typedef struct
{
  char const *name;
  int type;
  int value;
} table;

/* Meridian: am, pm, or 24-hour style.  */
enum { MERam, MERpm, MER24 };

/* A reasonable upper bound for the buffer used in debug output.  */
enum { DBGBUFSIZE = 100 };

enum { BILLION = 1000000000, LOG10_BILLION = 9 };

/* Relative times.  */
typedef struct
{
  /* Relative year, month, day, hour, minutes, seconds, and nanoseconds.  */
  intmax_t year;
  intmax_t month;
  intmax_t day;
  intmax_t hour;
  intmax_t minutes;
  intmax_t seconds;
  int ns;
} relative_time;

#if HAVE_COMPOUND_LITERALS
# define RELATIVE_TIME_0 ((relative_time) { 0, 0, 0, 0, 0, 0, 0 })
#else
static relative_time const RELATIVE_TIME_0;
#endif

/* Information passed to and from the parser.  */
typedef struct
{
  /* The input string remaining to be parsed.  */
  const char *input;

  /* N, if this is the Nth Tuesday.  */
  intmax_t day_ordinal;

  /* Day of week; Sunday is 0.  */
  int day_number;

  /* tm_isdst flag for the local zone.  */
  int local_isdst;

  /* Time zone, in seconds east of UT.  */
  int time_zone;

  /* Style used for time.  */
  int meridian;

  /* Gregorian year, month, day, hour, minutes, seconds, and nanoseconds.  */
  textint year;
  intmax_t month;
  intmax_t day;
  intmax_t hour;
  intmax_t minutes;
  struct timespec seconds; /* includes nanoseconds */

  /* Relative year, month, day, hour, minutes, seconds, and nanoseconds.  */
  relative_time rel;

  /* Presence or counts of nonterminals of various flavors parsed so far.  */
  bool timespec_seen;
  bool rels_seen;
  ptrdiff_t dates_seen;
  ptrdiff_t days_seen;
  ptrdiff_t local_zones_seen;
  ptrdiff_t dsts_seen;
  ptrdiff_t times_seen;
  ptrdiff_t zones_seen;
  bool year_seen;

  /* Print debugging output to stderr.  */
  bool parse_datetime_debug;

  /* Which of the 'seen' parts have been printed when debugging.  */
  bool debug_dates_seen;
  bool debug_days_seen;
  bool debug_local_zones_seen;
  bool debug_times_seen;
  bool debug_zones_seen;
  bool debug_year_seen;

  /* The user specified explicit ordinal day value.  */
  bool debug_ordinal_day_seen;

  /* Table of local time zone abbreviations, terminated by a null entry.  */
  table local_time_zone_table[3];
} parser_control;

union YYSTYPE;
static int yylex (union YYSTYPE *, parser_control *);
static int yyerror (parser_control const *, char const *);
static bool time_zone_hhmm (parser_control *, textint, intmax_t);

/* Extract into *PC any date and time info from a string of digits
   of the form e.g., YYYYMMDD, YYMMDD, HHMM, HH (and sometimes YYY,
   YYYY, ...).  */
static void
digits_to_date_time (parser_control *pc, textint text_int)
{
  if (pc->dates_seen && ! pc->year.digits
      && ! pc->rels_seen && (pc->times_seen || 2 < text_int.digits))
    {
      pc->year_seen = true;
      pc->year = text_int;
    }
  else
    {
      if (4 < text_int.digits)
        {
          pc->dates_seen++;
          pc->day = text_int.value % 100;
          pc->month = (text_int.value / 100) % 100;
          pc->year.value = text_int.value / 10000;
          pc->year.digits = text_int.digits - 4;
        }
      else
        {
          pc->times_seen++;
          if (text_int.digits <= 2)
            {
              pc->hour = text_int.value;
              pc->minutes = 0;
            }
          else
            {
              pc->hour = text_int.value / 100;
              pc->minutes = text_int.value % 100;
            }
          pc->seconds.tv_sec = 0;
          pc->seconds.tv_nsec = 0;
          pc->meridian = MER24;
        }
    }
}

/* Increment PC->rel by FACTOR * REL (FACTOR is 1 or -1).  Return true
   if successful, false if an overflow occurred.  */
static bool
apply_relative_time (parser_control *pc, relative_time rel, int factor)
{
  if (factor < 0
      ? (INT_SUBTRACT_WRAPV (pc->rel.ns, rel.ns, &pc->rel.ns)
         | INT_SUBTRACT_WRAPV (pc->rel.seconds, rel.seconds, &pc->rel.seconds)
         | INT_SUBTRACT_WRAPV (pc->rel.minutes, rel.minutes, &pc->rel.minutes)
         | INT_SUBTRACT_WRAPV (pc->rel.hour, rel.hour, &pc->rel.hour)
         | INT_SUBTRACT_WRAPV (pc->rel.day, rel.day, &pc->rel.day)
         | INT_SUBTRACT_WRAPV (pc->rel.month, rel.month, &pc->rel.month)
         | INT_SUBTRACT_WRAPV (pc->rel.year, rel.year, &pc->rel.year))
      : (INT_ADD_WRAPV (pc->rel.ns, rel.ns, &pc->rel.ns)
         | INT_ADD_WRAPV (pc->rel.seconds, rel.seconds, &pc->rel.seconds)
         | INT_ADD_WRAPV (pc->rel.minutes, rel.minutes, &pc->rel.minutes)
         | INT_ADD_WRAPV (pc->rel.hour, rel.hour, &pc->rel.hour)
         | INT_ADD_WRAPV (pc->rel.day, rel.day, &pc->rel.day)
         | INT_ADD_WRAPV (pc->rel.month, rel.month, &pc->rel.month)
         | INT_ADD_WRAPV (pc->rel.year, rel.year, &pc->rel.year)))
    return false;
  pc->rels_seen = true;
  return true;
}

/* Set PC-> hour, minutes, seconds and nanoseconds members from arguments.  */
static void
set_hhmmss (parser_control *pc, intmax_t hour, intmax_t minutes,
            time_t sec, int nsec)
{
  pc->hour = hour;
  pc->minutes = minutes;
  pc->seconds.tv_sec = sec;
  pc->seconds.tv_nsec = nsec;
}

/* Return a textual representation of the day ordinal/number values
   in the parser_control struct (e.g., "last wed", "this tues", "thu").  */
static const char *
str_days (parser_control *pc, char *buffer, int n)
{
  /* TODO: use relative_time_table for reverse lookup.  */
  static char const ordinal_values[][11] = {
     "last",
     "this",
     "next/first",
     "(SECOND)", /* SECOND is commented out in relative_time_table.  */
     "third",
     "fourth",
     "fifth",
     "sixth",
     "seventh",
     "eight",
     "ninth",
     "tenth",
     "eleventh",
     "twelfth"
  };

  static char const days_values[][4] = {
     "Sun",
     "Mon",
     "Tue",
     "Wed",
     "Thu",
     "Fri",
     "Sat"
  };

  int len;

  /* Don't add an ordinal prefix if the user didn't specify it
     (e.g., "this wed" vs "wed").  */
  if (pc->debug_ordinal_day_seen)
    {
      /* Use word description if possible (e.g., -1 = last, 3 = third).  */
      len = (-1 <= pc->day_ordinal && pc->day_ordinal <= 12
             ? snprintf (buffer, n, "%s", ordinal_values[pc->day_ordinal + 1])
             : snprintf (buffer, n, "%"PRIdMAX, pc->day_ordinal));
    }
  else
    {
      buffer[0] = '\0';
      len = 0;
    }

  /* Add the day name */
  if (0 <= pc->day_number && pc->day_number <= 6 && 0 <= len && len < n)
    snprintf (buffer + len, n - len, &" %s"[len == 0],
              days_values[pc->day_number]);
  else
    {
      /* invalid day_number value - should never happen */
    }
  return buffer;
}

/* Convert a time zone to its string representation.  */

enum { TIME_ZONE_BUFSIZE = INT_STRLEN_BOUND (intmax_t) + sizeof ":MM:SS" } ;

static char const *
time_zone_str (int time_zone, char time_zone_buf[TIME_ZONE_BUFSIZE])
{
  char *p = time_zone_buf;
  char sign = time_zone < 0 ? '-' : '+';
  int hour = abs (time_zone / (60 * 60));
  p += sprintf (time_zone_buf, "%c%02d", sign, hour);
  int offset_from_hour = abs (time_zone % (60 * 60));
  if (offset_from_hour != 0)
    {
      int mm = offset_from_hour / 60;
      int ss = offset_from_hour % 60;
      *p++ = ':';
      *p++ = '0' + mm / 10;
      *p++ = '0' + mm % 10;
      if (ss)
        {
          *p++ = ':';
          *p++ = '0' + ss / 10;
          *p++ = '0' + ss % 10;
        }
      *p = '\0';
    }
  return time_zone_buf;
}

/* debugging: print the current time in the parser_control structure.
   The parser will increment "*_seen" members for those which were parsed.
   This function will print only newly seen parts.  */
static void
debug_print_current_time (char const *item, parser_control *pc)
{
  bool space = false;

  if (!pc->parse_datetime_debug)
    return;

  /* no newline, more items printed below */
  dbg_printf (_("parsed %s part: "), item);

  if (pc->dates_seen && !pc->debug_dates_seen)
    {
      /*TODO: use pc->year.negative?  */
      fprintf (stderr, "(Y-M-D) %04"PRIdMAX"-%02"PRIdMAX"-%02"PRIdMAX,
              pc->year.value, pc->month, pc->day);
      pc->debug_dates_seen = true;
      space = true;
    }

  if (pc->year_seen != pc->debug_year_seen)
    {
      if (space)
        fputc (' ', stderr);
      fprintf (stderr, _("year: %04"PRIdMAX), pc->year.value);

      pc->debug_year_seen = pc->year_seen;
      space = true;
    }

  if (pc->times_seen && !pc->debug_times_seen)
    {
      intmax_t sec = pc->seconds.tv_sec;
      fprintf (stderr, &" %02"PRIdMAX":%02"PRIdMAX":%02"PRIdMAX[!space],
               pc->hour, pc->minutes, sec);
      if (pc->seconds.tv_nsec != 0)
        {
          int nsec = pc->seconds.tv_nsec;
          fprintf (stderr, ".%09d", nsec);
        }
      if (pc->meridian == MERpm)
        fputs ("pm", stderr);

      pc->debug_times_seen = true;
      space = true;
    }

  if (pc->days_seen && !pc->debug_days_seen)
    {
      if (space)
        fputc (' ', stderr);
      char tmp[DBGBUFSIZE];
      fprintf (stderr, _("%s (day ordinal=%"PRIdMAX" number=%d)"),
               str_days (pc, tmp, sizeof tmp),
               pc->day_ordinal, pc->day_number);
      pc->debug_days_seen = true;
      space = true;
    }

  /* local zone strings only change the DST settings,
     not the timezone value.  If seen, inform about the DST.  */
  if (pc->local_zones_seen && !pc->debug_local_zones_seen)
    {
      fprintf (stderr, &" isdst=%d%s"[!space],
               pc->local_isdst, pc->dsts_seen ? " DST" : "");
      pc->debug_local_zones_seen = true;
      space = true;
    }

  if (pc->zones_seen && !pc->debug_zones_seen)
    {
      char time_zone_buf[TIME_ZONE_BUFSIZE];
      fprintf (stderr, &" UTC%s"[!space],
               time_zone_str (pc->time_zone, time_zone_buf));
      pc->debug_zones_seen = true;
      space = true;
    }

  if (pc->timespec_seen)
    {
      intmax_t sec = pc->seconds.tv_sec;
      if (space)
        fputc (' ', stderr);
      fprintf (stderr, _("number of seconds: %"PRIdMAX), sec);
    }

  fputc ('\n', stderr);
}

/* Debugging: print the current relative values.  */

static bool
print_rel_part (bool space, intmax_t val, char const *name)
{
  if (val == 0)
    return space;
  fprintf (stderr, &" %+"PRIdMAX" %s"[!space], val, name);
  return true;
}

static void
debug_print_relative_time (char const *item, parser_control const *pc)
{
  bool space = false;

  if (!pc->parse_datetime_debug)
    return;

  /* no newline, more items printed below */
  dbg_printf (_("parsed %s part: "), item);

  if (pc->rel.year == 0 && pc->rel.month == 0 && pc->rel.day == 0
      && pc->rel.hour == 0 && pc->rel.minutes == 0 && pc->rel.seconds == 0
      && pc->rel.ns == 0)
    {
      /* Special case: relative time of this/today/now */
      fputs (_("today/this/now\n"), stderr);
      return;
    }

  space = print_rel_part (space, pc->rel.year, "year(s)");
  space = print_rel_part (space, pc->rel.month, "month(s)");
  space = print_rel_part (space, pc->rel.day, "day(s)");
  space = print_rel_part (space, pc->rel.hour, "hour(s)");
  space = print_rel_part (space, pc->rel.minutes, "minutes");
  space = print_rel_part (space, pc->rel.seconds, "seconds");
  print_rel_part (space, pc->rel.ns, "nanoseconds");

  fputc ('\n', stderr);
}



%}

/* We want a reentrant parser, even if the TZ manipulation and the calls to
   localtime and gmtime are not reentrant.  */
%pure-parser
%parse-param { parser_control *pc }
%lex-param { parser_control *pc }

/* This grammar has 31 shift/reduce conflicts.  */
%expect 31

%union
{
  intmax_t intval;
  textint textintval;
  struct timespec timespec;
  relative_time rel;
}

%token <intval> tAGO
%token tDST

%token tYEAR_UNIT tMONTH_UNIT tHOUR_UNIT tMINUTE_UNIT tSEC_UNIT
%token <intval> tDAY_UNIT tDAY_SHIFT

%token <intval> tDAY tDAYZONE tLOCAL_ZONE tMERIDIAN
%token <intval> tMONTH tORDINAL tZONE

%token <textintval> tSNUMBER tUNUMBER
%token <timespec> tSDECIMAL_NUMBER tUDECIMAL_NUMBER

%type <intval> o_colon_minutes
%type <timespec> seconds signed_seconds unsigned_seconds

%type <rel> relunit relunit_snumber dayshift

%%

spec:
    timespec
  | items
  ;

timespec:
    '@' seconds
      {
        pc->seconds = $2;
        pc->timespec_seen = true;
        debug_print_current_time (_("number of seconds"), pc);
      }
  ;

items:
    /* empty */
  | items item
  ;

item:
    datetime
      {
        pc->times_seen++; pc->dates_seen++;
        debug_print_current_time (_("datetime"), pc);
      }
  | time
      {
        pc->times_seen++;
        debug_print_current_time (_("time"), pc);
      }
  | local_zone
      {
        pc->local_zones_seen++;
        debug_print_current_time (_("local_zone"), pc);
      }
  | zone
      {
        pc->zones_seen++;
        debug_print_current_time (_("zone"), pc);
      }
  | date
      {
        pc->dates_seen++;
        debug_print_current_time (_("date"), pc);
      }
  | day
      {
        pc->days_seen++;
        debug_print_current_time (_("day"), pc);
      }
  | rel
      {
        debug_print_relative_time (_("relative"), pc);
      }
  | number
      {
        debug_print_current_time (_("number"), pc);
      }
  | hybrid
      {
        debug_print_relative_time (_("hybrid"), pc);
      }
  ;

datetime:
    iso_8601_datetime
  ;

iso_8601_datetime:
    iso_8601_date 'T' iso_8601_time
  ;

time:
    tUNUMBER tMERIDIAN
      {
        set_hhmmss (pc, $1.value, 0, 0, 0);
        pc->meridian = $2;
      }
  | tUNUMBER ':' tUNUMBER tMERIDIAN
      {
        set_hhmmss (pc, $1.value, $3.value, 0, 0);
        pc->meridian = $4;
      }
  | tUNUMBER ':' tUNUMBER ':' unsigned_seconds tMERIDIAN
      {
        set_hhmmss (pc, $1.value, $3.value, $5.tv_sec, $5.tv_nsec);
        pc->meridian = $6;
      }
  | iso_8601_time
  ;

iso_8601_time:
    tUNUMBER zone_offset
      {
        set_hhmmss (pc, $1.value, 0, 0, 0);
        pc->meridian = MER24;
      }
  | tUNUMBER ':' tUNUMBER o_zone_offset
      {
        set_hhmmss (pc, $1.value, $3.value, 0, 0);
        pc->meridian = MER24;
      }
  | tUNUMBER ':' tUNUMBER ':' unsigned_seconds o_zone_offset
      {
        set_hhmmss (pc, $1.value, $3.value, $5.tv_sec, $5.tv_nsec);
        pc->meridian = MER24;
      }
  ;

o_zone_offset:
  /* empty */
  | zone_offset
  ;

zone_offset:
    tSNUMBER o_colon_minutes
      {
        pc->zones_seen++;
        if (! time_zone_hhmm (pc, $1, $2)) YYABORT;
      }
  ;

/* Local zone strings affect only the DST setting, and take effect
   only if the current TZ setting is relevant.

   Example 1:
   'EEST' is parsed as tLOCAL_ZONE, as it relates to the effective TZ:
        TZ='Europe/Helsinki' date -d '2016-06-30 EEST'

   Example 2:
   'EEST' is parsed as tDAYZONE:
        TZ='Asia/Tokyo' date -d '2016-06-30 EEST'

   This is implemented by probing the next three calendar quarters
   of the effective timezone and looking for DST changes -
   if found, the timezone name (EEST) is inserted into
   the lexical lookup table with type tLOCAL_ZONE.
   (Search for 'quarter' comment in  'parse_datetime2'.)
*/
local_zone:
    tLOCAL_ZONE
      { pc->local_isdst = $1; }
  | tLOCAL_ZONE tDST
      {
        pc->local_isdst = 1;
        pc->dsts_seen++;
      }
  ;

/* Note 'T' is a special case, as it is used as the separator in ISO
   8601 date and time of day representation.  */
zone:
    tZONE
      { pc->time_zone = $1; }
  | 'T'
      { pc->time_zone = HOUR (7); }
  | tZONE relunit_snumber
      { pc->time_zone = $1;
        if (! apply_relative_time (pc, $2, 1)) YYABORT;
        debug_print_relative_time (_("relative"), pc);
      }
  | 'T' relunit_snumber
      { pc->time_zone = HOUR (7);
        if (! apply_relative_time (pc, $2, 1)) YYABORT;
        debug_print_relative_time (_("relative"), pc);
      }
  | tZONE tSNUMBER o_colon_minutes
      { if (! time_zone_hhmm (pc, $2, $3)) YYABORT;
        if (INT_ADD_WRAPV (pc->time_zone, $1, &pc->time_zone)) YYABORT; }
  | tDAYZONE
      { pc->time_zone = $1 + 60 * 60; }
  | tZONE tDST
      { pc->time_zone = $1 + 60 * 60; }
  ;

day:
    tDAY
      {
        pc->day_ordinal = 0;
        pc->day_number = $1;
      }
  | tDAY ','
      {
        pc->day_ordinal = 0;
        pc->day_number = $1;
      }
  | tORDINAL tDAY
      {
        pc->day_ordinal = $1;
        pc->day_number = $2;
        pc->debug_ordinal_day_seen = true;
      }
  | tUNUMBER tDAY
      {
        pc->day_ordinal = $1.value;
        pc->day_number = $2;
        pc->debug_ordinal_day_seen = true;
      }
  ;

date:
    tUNUMBER '/' tUNUMBER
      {
        pc->month = $1.value;
        pc->day = $3.value;
      }
  | tUNUMBER '/' tUNUMBER '/' tUNUMBER
      {
        /* Interpret as YYYY/MM/DD if the first value has 4 or more digits,
           otherwise as MM/DD/YY.
           The goal in recognizing YYYY/MM/DD is solely to support legacy
           machine-generated dates like those in an RCS log listing.  If
           you want portability, use the ISO 8601 format.  */
        if (4 <= $1.digits)
          {
            if (pc->parse_datetime_debug)
              {
                intmax_t digits = $1.digits;
                dbg_printf (_("warning: value %"PRIdMAX" has %"PRIdMAX" digits. "
                              "Assuming YYYY/MM/DD\n"),
                            $1.value, digits);
              }

            pc->year = $1;
            pc->month = $3.value;
            pc->day = $5.value;
          }
        else
          {
            if (pc->parse_datetime_debug)
              dbg_printf (_("warning: value %"PRIdMAX" has less than 4 digits. "
                            "Assuming MM/DD/YY[YY]\n"),
                          $1.value);

            pc->month = $1.value;
            pc->day = $3.value;
            pc->year = $5;
          }
      }
  | tUNUMBER tMONTH tSNUMBER
      {
        /* E.g., 17-JUN-1992.  */
        pc->day = $1.value;
        pc->month = $2;
        if (INT_SUBTRACT_WRAPV (0, $3.value, &pc->year.value)) YYABORT;
        pc->year.digits = $3.digits;
      }
  | tMONTH tSNUMBER tSNUMBER
      {
        /* E.g., JUN-17-1992.  */
        pc->month = $1;
        if (INT_SUBTRACT_WRAPV (0, $2.value, &pc->day)) YYABORT;
        if (INT_SUBTRACT_WRAPV (0, $3.value, &pc->year.value)) YYABORT;
        pc->year.digits = $3.digits;
      }
  | tMONTH tUNUMBER
      {
        pc->month = $1;
        pc->day = $2.value;
      }
  | tMONTH tUNUMBER ',' tUNUMBER
      {
        pc->month = $1;
        pc->day = $2.value;
        pc->year = $4;
      }
  | tUNUMBER tMONTH
      {
        pc->day = $1.value;
        pc->month = $2;
      }
  | tUNUMBER tMONTH tUNUMBER
      {
        pc->day = $1.value;
        pc->month = $2;
        pc->year = $3;
      }
  | iso_8601_date
  ;

iso_8601_date:
    tUNUMBER tSNUMBER tSNUMBER
      {
        /* ISO 8601 format.  YYYY-MM-DD.  */
        pc->year = $1;
        if (INT_SUBTRACT_WRAPV (0, $2.value, &pc->month)) YYABORT;
        if (INT_SUBTRACT_WRAPV (0, $3.value, &pc->day)) YYABORT;
      }
  ;

rel:
    relunit tAGO
      { if (! apply_relative_time (pc, $1, $2)) YYABORT; }
  | relunit
      { if (! apply_relative_time (pc, $1, 1)) YYABORT; }
  | dayshift
      { if (! apply_relative_time (pc, $1, 1)) YYABORT; }
  ;

relunit:
    tORDINAL tYEAR_UNIT
      { $$ = RELATIVE_TIME_0; $$.year = $1; }
  | tUNUMBER tYEAR_UNIT
      { $$ = RELATIVE_TIME_0; $$.year = $1.value; }
  | tYEAR_UNIT
      { $$ = RELATIVE_TIME_0; $$.year = 1; }
  | tORDINAL tMONTH_UNIT
      { $$ = RELATIVE_TIME_0; $$.month = $1; }
  | tUNUMBER tMONTH_UNIT
      { $$ = RELATIVE_TIME_0; $$.month = $1.value; }
  | tMONTH_UNIT
      { $$ = RELATIVE_TIME_0; $$.month = 1; }
  | tORDINAL tDAY_UNIT
      { $$ = RELATIVE_TIME_0;
        if (INT_MULTIPLY_WRAPV ($1, $2, &$$.day)) YYABORT; }
  | tUNUMBER tDAY_UNIT
      { $$ = RELATIVE_TIME_0;
        if (INT_MULTIPLY_WRAPV ($1.value, $2, &$$.day)) YYABORT; }
  | tDAY_UNIT
      { $$ = RELATIVE_TIME_0; $$.day = $1; }
  | tORDINAL tHOUR_UNIT
      { $$ = RELATIVE_TIME_0; $$.hour = $1; }
  | tUNUMBER tHOUR_UNIT
      { $$ = RELATIVE_TIME_0; $$.hour = $1.value; }
  | tHOUR_UNIT
      { $$ = RELATIVE_TIME_0; $$.hour = 1; }
  | tORDINAL tMINUTE_UNIT
      { $$ = RELATIVE_TIME_0; $$.minutes = $1; }
  | tUNUMBER tMINUTE_UNIT
      { $$ = RELATIVE_TIME_0; $$.minutes = $1.value; }
  | tMINUTE_UNIT
      { $$ = RELATIVE_TIME_0; $$.minutes = 1; }
  | tORDINAL tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = $1; }
  | tUNUMBER tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = $1.value; }
  | tSDECIMAL_NUMBER tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = $1.tv_sec; $$.ns = $1.tv_nsec; }
  | tUDECIMAL_NUMBER tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = $1.tv_sec; $$.ns = $1.tv_nsec; }
  | tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = 1; }
  | relunit_snumber
  ;

relunit_snumber:
    tSNUMBER tYEAR_UNIT
      { $$ = RELATIVE_TIME_0; $$.year = $1.value; }
  | tSNUMBER tMONTH_UNIT
      { $$ = RELATIVE_TIME_0; $$.month = $1.value; }
  | tSNUMBER tDAY_UNIT
      { $$ = RELATIVE_TIME_0;
        if (INT_MULTIPLY_WRAPV ($1.value, $2, &$$.day)) YYABORT; }
  | tSNUMBER tHOUR_UNIT
      { $$ = RELATIVE_TIME_0; $$.hour = $1.value; }
  | tSNUMBER tMINUTE_UNIT
      { $$ = RELATIVE_TIME_0; $$.minutes = $1.value; }
  | tSNUMBER tSEC_UNIT
      { $$ = RELATIVE_TIME_0; $$.seconds = $1.value; }
  ;

dayshift:
    tDAY_SHIFT
      { $$ = RELATIVE_TIME_0; $$.day = $1; }
  ;

seconds: signed_seconds | unsigned_seconds;

signed_seconds:
    tSDECIMAL_NUMBER
  | tSNUMBER
      { if (time_overflow ($1.value)) YYABORT;
        $$.tv_sec = $1.value; $$.tv_nsec = 0; }
  ;

unsigned_seconds:
    tUDECIMAL_NUMBER
  | tUNUMBER
      { if (time_overflow ($1.value)) YYABORT;
        $$.tv_sec = $1.value; $$.tv_nsec = 0; }
  ;

number:
    tUNUMBER
      { digits_to_date_time (pc, $1); }
  ;

hybrid:
    tUNUMBER relunit_snumber
      {
        /* Hybrid all-digit and relative offset, so that we accept e.g.,
           "YYYYMMDD +N days" as well as "YYYYMMDD N days".  */
        digits_to_date_time (pc, $1);
        if (! apply_relative_time (pc, $2, 1)) YYABORT;
      }
  ;

o_colon_minutes:
    /* empty */
      { $$ = -1; }
  | ':' tUNUMBER
      { $$ = $2.value; }
  ;

%%

static table const meridian_table[] =
{
  { "AM",   tMERIDIAN, MERam },
  { "A.M.", tMERIDIAN, MERam },
  { "PM",   tMERIDIAN, MERpm },
  { "P.M.", tMERIDIAN, MERpm },
  { NULL, 0, 0 }
};

static table const dst_table[] =
{
  { "DST", tDST, 0 }
};

static table const month_and_day_table[] =
{
  { "JANUARY",  tMONTH,  1 },
  { "FEBRUARY", tMONTH,  2 },
  { "MARCH",    tMONTH,  3 },
  { "APRIL",    tMONTH,  4 },
  { "MAY",      tMONTH,  5 },
  { "JUNE",     tMONTH,  6 },
  { "JULY",     tMONTH,  7 },
  { "AUGUST",   tMONTH,  8 },
  { "SEPTEMBER",tMONTH,  9 },
  { "SEPT",     tMONTH,  9 },
  { "OCTOBER",  tMONTH, 10 },
  { "NOVEMBER", tMONTH, 11 },
  { "DECEMBER", tMONTH, 12 },
  { "SUNDAY",   tDAY,    0 },
  { "MONDAY",   tDAY,    1 },
  { "TUESDAY",  tDAY,    2 },
  { "TUES",     tDAY,    2 },
  { "WEDNESDAY",tDAY,    3 },
  { "WEDNES",   tDAY,    3 },
  { "THURSDAY", tDAY,    4 },
  { "THUR",     tDAY,    4 },
  { "THURS",    tDAY,    4 },
  { "FRIDAY",   tDAY,    5 },
  { "SATURDAY", tDAY,    6 },
  { NULL, 0, 0 }
};

static table const time_units_table[] =
{
  { "YEAR",     tYEAR_UNIT,      1 },
  { "MONTH",    tMONTH_UNIT,     1 },
  { "FORTNIGHT",tDAY_UNIT,      14 },
  { "WEEK",     tDAY_UNIT,       7 },
  { "DAY",      tDAY_UNIT,       1 },
  { "HOUR",     tHOUR_UNIT,      1 },
  { "MINUTE",   tMINUTE_UNIT,    1 },
  { "MIN",      tMINUTE_UNIT,    1 },
  { "SECOND",   tSEC_UNIT,       1 },
  { "SEC",      tSEC_UNIT,       1 },
  { NULL, 0, 0 }
};

/* Assorted relative-time words.  */
static table const relative_time_table[] =
{
  { "TOMORROW", tDAY_SHIFT,      1 },
  { "YESTERDAY",tDAY_SHIFT,     -1 },
  { "TODAY",    tDAY_SHIFT,      0 },
  { "NOW",      tDAY_SHIFT,      0 },
  { "LAST",     tORDINAL,       -1 },
  { "THIS",     tORDINAL,        0 },
  { "NEXT",     tORDINAL,        1 },
  { "FIRST",    tORDINAL,        1 },
/*{ "SECOND",   tORDINAL,        2 }, */
  { "THIRD",    tORDINAL,        3 },
  { "FOURTH",   tORDINAL,        4 },
  { "FIFTH",    tORDINAL,        5 },
  { "SIXTH",    tORDINAL,        6 },
  { "SEVENTH",  tORDINAL,        7 },
  { "EIGHTH",   tORDINAL,        8 },
  { "NINTH",    tORDINAL,        9 },
  { "TENTH",    tORDINAL,       10 },
  { "ELEVENTH", tORDINAL,       11 },
  { "TWELFTH",  tORDINAL,       12 },
  { "AGO",      tAGO,           -1 },
  { "HENCE",    tAGO,            1 },
  { NULL, 0, 0 }
};

/* The universal time zone table.  These labels can be used even for
   timestamps that would not otherwise be valid, e.g., GMT timestamps
   oin London during summer.  */
static table const universal_time_zone_table[] =
{
  { "GMT",      tZONE,     HOUR ( 0) }, /* Greenwich Mean */
  { "UT",       tZONE,     HOUR ( 0) }, /* Universal (Coordinated) */
  { "UTC",      tZONE,     HOUR ( 0) },
  { NULL, 0, 0 }
};

/* The time zone table.  This table is necessarily incomplete, as time
   zone abbreviations are ambiguous; e.g., Australians interpret "EST"
   as Eastern time in Australia, not as US Eastern Standard Time.
   You cannot rely on parse_datetime to handle arbitrary time zone
   abbreviations; use numeric abbreviations like "-0500" instead.  */
static table const time_zone_table[] =
{
  { "WET",      tZONE,     HOUR ( 0) }, /* Western European */
  { "WEST",     tDAYZONE,  HOUR ( 0) }, /* Western European Summer */
  { "BST",      tDAYZONE,  HOUR ( 0) }, /* British Summer */
  { "ART",      tZONE,    -HOUR ( 3) }, /* Argentina */
  { "BRT",      tZONE,    -HOUR ( 3) }, /* Brazil */
  { "BRST",     tDAYZONE, -HOUR ( 3) }, /* Brazil Summer */
  { "NST",      tZONE,   -(HOUR ( 3) + 30 * 60) }, /* Newfoundland Standard */
  { "NDT",      tDAYZONE,-(HOUR ( 3) + 30 * 60) }, /* Newfoundland Daylight */
  { "AST",      tZONE,    -HOUR ( 4) }, /* Atlantic Standard */
  { "ADT",      tDAYZONE, -HOUR ( 4) }, /* Atlantic Daylight */
  { "CLT",      tZONE,    -HOUR ( 4) }, /* Chile */
  { "CLST",     tDAYZONE, -HOUR ( 4) }, /* Chile Summer */
  { "EST",      tZONE,    -HOUR ( 5) }, /* Eastern Standard */
  { "EDT",      tDAYZONE, -HOUR ( 5) }, /* Eastern Daylight */
  { "CST",      tZONE,    -HOUR ( 6) }, /* Central Standard */
  { "CDT",      tDAYZONE, -HOUR ( 6) }, /* Central Daylight */
  { "MST",      tZONE,    -HOUR ( 7) }, /* Mountain Standard */
  { "MDT",      tDAYZONE, -HOUR ( 7) }, /* Mountain Daylight */
  { "PST",      tZONE,    -HOUR ( 8) }, /* Pacific Standard */
  { "PDT",      tDAYZONE, -HOUR ( 8) }, /* Pacific Daylight */
  { "AKST",     tZONE,    -HOUR ( 9) }, /* Alaska Standard */
  { "AKDT",     tDAYZONE, -HOUR ( 9) }, /* Alaska Daylight */
  { "HST",      tZONE,    -HOUR (10) }, /* Hawaii Standard */
  { "HAST",     tZONE,    -HOUR (10) }, /* Hawaii-Aleutian Standard */
  { "HADT",     tDAYZONE, -HOUR (10) }, /* Hawaii-Aleutian Daylight */
  { "SST",      tZONE,    -HOUR (12) }, /* Samoa Standard */
  { "WAT",      tZONE,     HOUR ( 1) }, /* West Africa */
  { "CET",      tZONE,     HOUR ( 1) }, /* Central European */
  { "CEST",     tDAYZONE,  HOUR ( 1) }, /* Central European Summer */
  { "MET",      tZONE,     HOUR ( 1) }, /* Middle European */
  { "MEZ",      tZONE,     HOUR ( 1) }, /* Middle European */
  { "MEST",     tDAYZONE,  HOUR ( 1) }, /* Middle European Summer */
  { "MESZ",     tDAYZONE,  HOUR ( 1) }, /* Middle European Summer */
  { "EET",      tZONE,     HOUR ( 2) }, /* Eastern European */
  { "EEST",     tDAYZONE,  HOUR ( 2) }, /* Eastern European Summer */
  { "CAT",      tZONE,     HOUR ( 2) }, /* Central Africa */
  { "SAST",     tZONE,     HOUR ( 2) }, /* South Africa Standard */
  { "EAT",      tZONE,     HOUR ( 3) }, /* East Africa */
  { "MSK",      tZONE,     HOUR ( 3) }, /* Moscow */
  { "MSD",      tDAYZONE,  HOUR ( 3) }, /* Moscow Daylight */
  { "IST",      tZONE,    (HOUR ( 5) + 30 * 60) }, /* India Standard */
  { "SGT",      tZONE,     HOUR ( 8) }, /* Singapore */
  { "KST",      tZONE,     HOUR ( 9) }, /* Korea Standard */
  { "JST",      tZONE,     HOUR ( 9) }, /* Japan Standard */
  { "GST",      tZONE,     HOUR (10) }, /* Guam Standard */
  { "NZST",     tZONE,     HOUR (12) }, /* New Zealand Standard */
  { "NZDT",     tDAYZONE,  HOUR (12) }, /* New Zealand Daylight */
  { NULL, 0, 0 }
};

/* Military time zone table.

   Note 'T' is a special case, as it is used as the separator in ISO
   8601 date and time of day representation.  */
static table const military_table[] =
{
  { "A", tZONE, -HOUR ( 1) },
  { "B", tZONE, -HOUR ( 2) },
  { "C", tZONE, -HOUR ( 3) },
  { "D", tZONE, -HOUR ( 4) },
  { "E", tZONE, -HOUR ( 5) },
  { "F", tZONE, -HOUR ( 6) },
  { "G", tZONE, -HOUR ( 7) },
  { "H", tZONE, -HOUR ( 8) },
  { "I", tZONE, -HOUR ( 9) },
  { "K", tZONE, -HOUR (10) },
  { "L", tZONE, -HOUR (11) },
  { "M", tZONE, -HOUR (12) },
  { "N", tZONE,  HOUR ( 1) },
  { "O", tZONE,  HOUR ( 2) },
  { "P", tZONE,  HOUR ( 3) },
  { "Q", tZONE,  HOUR ( 4) },
  { "R", tZONE,  HOUR ( 5) },
  { "S", tZONE,  HOUR ( 6) },
  { "T", 'T',    0 },
  { "U", tZONE,  HOUR ( 8) },
  { "V", tZONE,  HOUR ( 9) },
  { "W", tZONE,  HOUR (10) },
  { "X", tZONE,  HOUR (11) },
  { "Y", tZONE,  HOUR (12) },
  { "Z", tZONE,  HOUR ( 0) },
  { NULL, 0, 0 }
};



/* Convert a time zone expressed as HH:MM into an integer count of
   seconds.  If MM is negative, then S is of the form HHMM and needs
   to be picked apart; otherwise, S is of the form HH.  As specified in
   http://www.opengroup.org/susv3xbd/xbd_chap08.html#tag_08_03, allow
   only valid TZ range, and consider first two digits as hours, if no
   minutes specified.  Return true if successful.  */

static bool
time_zone_hhmm (parser_control *pc, textint s, intmax_t mm)
{
  intmax_t n_minutes;
  bool overflow = false;

  /* If the length of S is 1 or 2 and no minutes are specified,
     interpret it as a number of hours.  */
  if (s.digits <= 2 && mm < 0)
    s.value *= 100;

  if (mm < 0)
    n_minutes = (s.value / 100) * 60 + s.value % 100;
  else
    {
      overflow |= INT_MULTIPLY_WRAPV (s.value, 60, &n_minutes);
      overflow |= (s.negative
                   ? INT_SUBTRACT_WRAPV (n_minutes, mm, &n_minutes)
                   : INT_ADD_WRAPV (n_minutes, mm, &n_minutes));
    }

  if (overflow || ! (-24 * 60 <= n_minutes && n_minutes <= 24 * 60))
    return false;
  pc->time_zone = n_minutes * 60;
  return true;
}

static int
to_hour (intmax_t hours, int meridian)
{
  switch (meridian)
    {
    default: /* Pacify GCC.  */
    case MER24:
      return 0 <= hours && hours < 24 ? hours : -1;
    case MERam:
      return 0 < hours && hours < 12 ? hours : hours == 12 ? 0 : -1;
    case MERpm:
      return 0 < hours && hours < 12 ? hours + 12 : hours == 12 ? 12 : -1;
    }
}

enum { TM_YEAR_BASE = 1900 };
enum { TM_YEAR_BUFSIZE = INT_BUFSIZE_BOUND (int) + 1 };

/* Convert TM_YEAR, a year minus 1900, to a string that is numerically
   correct even if subtracting 1900 would overflow.  */

static char const *
tm_year_str (int tm_year, char buf[TM_YEAR_BUFSIZE])
{
  verify (TM_YEAR_BASE % 100 == 0);
  sprintf (buf, &"-%02d%02d"[-TM_YEAR_BASE <= tm_year],
           abs (tm_year / 100 + TM_YEAR_BASE / 100),
           abs (tm_year % 100));
  return buf;
}

/* Convert a text year number to a year minus 1900, working correctly
   even if the input is in the range INT_MAX .. INT_MAX + 1900 - 1.  */

static bool
to_tm_year (textint textyear, bool debug, int *tm_year)
{
  intmax_t year = textyear.value;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if (0 <= year && textyear.digits == 2)
    {
      year += year < 69 ? 2000 : 1900;
      if (debug)
        dbg_printf (_("warning: adjusting year value %"PRIdMAX
                      " to %"PRIdMAX"\n"),
                    textyear.value, year);
    }

  if (year < 0
      ? INT_SUBTRACT_WRAPV (-TM_YEAR_BASE, year, tm_year)
      : INT_SUBTRACT_WRAPV (year, TM_YEAR_BASE, tm_year))
    {
      if (debug)
        dbg_printf (_("error: out-of-range year %"PRIdMAX"\n"), year);
      return false;
    }

  return true;
}

static table const * _GL_ATTRIBUTE_PURE
lookup_zone (parser_control const *pc, char const *name)
{
  table const *tp;

  for (tp = universal_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  /* Try local zone abbreviations before those in time_zone_table, as
     the local ones are more likely to be right.  */
  for (tp = pc->local_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  for (tp = time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  return NULL;
}

#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.
   The body of this function is taken directly from the GNU C Library;
   see strftime.c.  */
static int
tm_diff (const struct tm *a, const struct tm *b)
{
  /* Compute intervening leap days correctly even if year is negative.
     Take care to avoid int overflow in leap day calculations,
     but it's OK to assume that A and B are close to each other.  */
  int a4 = SHR (a->tm_year, 2) + SHR (TM_YEAR_BASE, 2) - ! (a->tm_year & 3);
  int b4 = SHR (b->tm_year, 2) + SHR (TM_YEAR_BASE, 2) - ! (b->tm_year & 3);
  int a100 = a4 / 25 - (a4 % 25 < 0);
  int b100 = b4 / 25 - (b4 % 25 < 0);
  int a400 = SHR (a100, 2);
  int b400 = SHR (b100, 2);
  int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
  int years = a->tm_year - b->tm_year;
  int days = (365 * years + intervening_leap_days
              + (a->tm_yday - b->tm_yday));
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
                + (a->tm_min - b->tm_min))
          + (a->tm_sec - b->tm_sec));
}
#endif /* ! HAVE_TM_GMTOFF */

static table const *
lookup_word (parser_control const *pc, char *word)
{
  char *p;
  char *q;
  ptrdiff_t wordlen;
  table const *tp;
  bool period_found;
  bool abbrev;

  /* Make it uppercase.  */
  for (p = word; *p; p++)
    *p = c_toupper (to_uchar (*p));

  for (tp = meridian_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* See if we have an abbreviation for a month.  */
  wordlen = strlen (word);
  abbrev = wordlen == 3 || (wordlen == 4 && word[3] == '.');

  for (tp = month_and_day_table; tp->name; tp++)
    if ((abbrev ? strncmp (word, tp->name, 3) : strcmp (word, tp->name)) == 0)
      return tp;

  if ((tp = lookup_zone (pc, word)))
    return tp;

  if (strcmp (word, dst_table[0].name) == 0)
    return dst_table;

  for (tp = time_units_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Strip off any plural and try the units table again.  */
  if (word[wordlen - 1] == 'S')
    {
      word[wordlen - 1] = '\0';
      for (tp = time_units_table; tp->name; tp++)
        if (strcmp (word, tp->name) == 0)
          return tp;
      word[wordlen - 1] = 'S';  /* For "this" in relative_time_table.  */
    }

  for (tp = relative_time_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Military time zones.  */
  if (wordlen == 1)
    for (tp = military_table; tp->name; tp++)
      if (word[0] == tp->name[0])
        return tp;

  /* Drop out any periods and try the time zone table again.  */
  for (period_found = false, p = q = word; (*p = *q); q++)
    if (*q == '.')
      period_found = true;
    else
      p++;
  if (period_found && (tp = lookup_zone (pc, word)))
    return tp;

  return NULL;
}

static int
yylex (union YYSTYPE *lvalp, parser_control *pc)
{
  unsigned char c;

  for (;;)
    {
      while (c = *pc->input, c_isspace (c))
        pc->input++;

      if (c_isdigit (c) || c == '-' || c == '+')
        {
          char const *p;
          int sign;
          intmax_t value = 0;
          if (c == '-' || c == '+')
            {
              sign = c == '-' ? -1 : 1;
              while (c = *++pc->input, c_isspace (c))
                continue;
              if (! c_isdigit (c))
                /* skip the '-' sign */
                continue;
            }
          else
            sign = 0;
          p = pc->input;

          do
            {
              if (INT_MULTIPLY_WRAPV (value, 10, &value))
                return '?';
              if (INT_ADD_WRAPV (value, sign < 0 ? '0' - c : c - '0', &value))
                return '?';
              c = *++p;
            }
          while (c_isdigit (c));

          if ((c == '.' || c == ',') && c_isdigit (p[1]))
            {
              time_t s;
              int ns;
              int digits;

              if (time_overflow (value))
                return '?';
              s = value;

              /* Accumulate fraction, to ns precision.  */
              p++;
              ns = *p++ - '0';
              for (digits = 2; digits <= LOG10_BILLION; digits++)
                {
                  ns *= 10;
                  if (c_isdigit (*p))
                    ns += *p++ - '0';
                }

              /* Skip excess digits, truncating toward -Infinity.  */
              if (sign < 0)
                for (; c_isdigit (*p); p++)
                  if (*p != '0')
                    {
                      ns++;
                      break;
                    }
              while (c_isdigit (*p))
                p++;

              /* Adjust to the timespec convention, which is that
                 tv_nsec is always a positive offset even if tv_sec is
                 negative.  */
              if (sign < 0 && ns)
                {
                  if (s == TYPE_MINIMUM (time_t))
                    return '?';
                  s--;
                  ns = BILLION - ns;
                }

              lvalp->timespec.tv_sec = s;
              lvalp->timespec.tv_nsec = ns;
              pc->input = p;
              return sign ? tSDECIMAL_NUMBER : tUDECIMAL_NUMBER;
            }
          else
            {
              lvalp->textintval.negative = sign < 0;
              lvalp->textintval.value = value;
              lvalp->textintval.digits = p - pc->input;
              pc->input = p;
              return sign ? tSNUMBER : tUNUMBER;
            }
        }

      if (c_isalpha (c))
        {
          char buff[20];
          char *p = buff;
          table const *tp;

          do
            {
              if (p < buff + sizeof buff - 1)
                *p++ = c;
              c = *++pc->input;
            }
          while (c_isalpha (c) || c == '.');

          *p = '\0';
          tp = lookup_word (pc, buff);
          if (! tp)
            {
              if (pc->parse_datetime_debug)
                dbg_printf (_("error: unknown word '%s'\n"), buff);
              return '?';
            }
          lvalp->intval = tp->value;
          return tp->type;
        }

      if (c != '(')
        return to_uchar (*pc->input++);

      ptrdiff_t count = 0;
      do
        {
          c = *pc->input++;
          if (c == '\0')
            return c;
          if (c == '(')
            count++;
          else if (c == ')')
            count--;
        }
      while (count != 0);
    }
}

/* Do nothing if the parser reports an error.  */
static int
yyerror (parser_control const *pc _GL_UNUSED,
         char const *s _GL_UNUSED)
{
  return 0;
}

/* If *TM0 is the old and *TM1 is the new value of a struct tm after
   passing it to mktime_z, return true if it's OK.  It's not OK if
   mktime failed or if *TM0 has out-of-range mainline members.
   The caller should set TM1->tm_wday to -1 before calling mktime,
   as a negative tm_wday is how mktime failure is inferred.  */

static bool
mktime_ok (struct tm const *tm0, struct tm const *tm1)
{
  if (tm1->tm_wday < 0)
    return false;

  return ! ((tm0->tm_sec ^ tm1->tm_sec)
            | (tm0->tm_min ^ tm1->tm_min)
            | (tm0->tm_hour ^ tm1->tm_hour)
            | (tm0->tm_mday ^ tm1->tm_mday)
            | (tm0->tm_mon ^ tm1->tm_mon)
            | (tm0->tm_year ^ tm1->tm_year));
}

/* Debugging: format a 'struct tm' into a buffer, taking the parser's
   timezone information into account (if pc != NULL).  */
static char const *
debug_strfdatetime (struct tm const *tm, parser_control const *pc,
                    char *buf, int n)
{
  /* TODO:
     1. find an optimal way to print date string in a clear and unambiguous
        format.  Currently, always add '(Y-M-D)' prefix.
        Consider '2016y01m10d'  or 'year(2016) month(01) day(10)'.

        If the user needs debug printing, it means he/she already having
        issues with the parsing - better to avoid formats that could
        be mis-interpreted (e.g., just YYYY-MM-DD).

     2. Can strftime be used instead?
        depends if it is portable and can print invalid dates on all systems.

     3. Print timezone information ?

     4. Print DST information ?

     5. Print nanosecond information ?

     NOTE:
     Printed date/time values might not be valid, e.g., '2016-02-31'
     or '2016-19-2016' .  These are the values as parsed from the user
     string, before validation.
  */
  int m = nstrftime (buf, n, "(Y-M-D) %Y-%m-%d %H:%M:%S", tm, 0, 0);

  /* If parser_control information was provided (for timezone),
     and there's enough space in the buffer, add timezone info.  */
  if (pc && m < n && pc->zones_seen)
    {
      int tz = pc->time_zone;

      /* Account for DST if tLOCAL_ZONE was seen.  */
      if (pc->local_zones_seen && !pc->zones_seen && 0 < pc->local_isdst)
        tz += 60 * 60;

      char time_zone_buf[TIME_ZONE_BUFSIZE];
      snprintf (&buf[m], n - m, " TZ=%s", time_zone_str (tz, time_zone_buf));
    }
  return buf;
}

static char const *
debug_strfdate (struct tm const *tm, char *buf, int n)
{
  char tm_year_buf[TM_YEAR_BUFSIZE];
  snprintf (buf, n, "(Y-M-D) %s-%02d-%02d",
            tm_year_str (tm->tm_year, tm_year_buf),
            tm->tm_mon + 1, tm->tm_mday);
  return buf;
}

static char const *
debug_strftime (struct tm const *tm, char *buf, int n)
{
  snprintf (buf, n, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  return buf;
}

/* If mktime_ok failed, display the failed time values,
   and provide possible hints.  Example output:

    date: error: invalid date/time value:
    date:     user provided time: '(Y-M-D) 2006-04-02 02:45:00'
    date:        normalized time: '(Y-M-D) 2006-04-02 03:45:00'
    date:                                             __
    date:      possible reasons:
    date:        non-existing due to daylight-saving time;
    date:        numeric values overflow;
    date:        missing timezone;
 */
static void
debug_mktime_not_ok (struct tm const *tm0, struct tm const *tm1,
                     parser_control const *pc, bool time_zone_seen)
{
  /* TODO: handle t == -1 (as in 'mktime_ok').  */
  char tmp[DBGBUFSIZE];
  int i;
  const bool eq_sec   = (tm0->tm_sec  == tm1->tm_sec);
  const bool eq_min   = (tm0->tm_min  == tm1->tm_min);
  const bool eq_hour  = (tm0->tm_hour == tm1->tm_hour);
  const bool eq_mday  = (tm0->tm_mday == tm1->tm_mday);
  const bool eq_month = (tm0->tm_mon  == tm1->tm_mon);
  const bool eq_year  = (tm0->tm_year == tm1->tm_year);

  const bool dst_shift = eq_sec && eq_min && !eq_hour
                         && eq_mday && eq_month && eq_year;

  if (!pc->parse_datetime_debug)
    return;

  dbg_printf (_("error: invalid date/time value:\n"));
  dbg_printf (_("    user provided time: '%s'\n"),
              debug_strfdatetime (tm0, pc, tmp, sizeof tmp));
  dbg_printf (_("       normalized time: '%s'\n"),
              debug_strfdatetime (tm1, pc, tmp, sizeof tmp));
  /* The format must be aligned with debug_strfdatetime and the two
     DEBUG statements above.  This string is not translated.  */
  i = snprintf (tmp, sizeof tmp,
                "                                 %4s %2s %2s %2s %2s %2s",
                eq_year ? "" : "----",
                eq_month ? "" : "--",
                eq_mday ? "" : "--",
                eq_hour ? "" : "--",
                eq_min ? "" : "--",
                eq_sec ? "" : "--");
  /* Trim trailing whitespace.  */
  if (0 <= i)
    {
      if (sizeof tmp - 1 < i)
        i = sizeof tmp - 1;
      while (0 < i && tmp[i - 1] == ' ')
        --i;
      tmp[i] = '\0';
    }
  dbg_printf ("%s\n", tmp);

  dbg_printf (_("     possible reasons:\n"));
  if (dst_shift)
    dbg_printf (_("       non-existing due to daylight-saving time;\n"));
  if (!eq_mday && !eq_month)
    dbg_printf (_("       invalid day/month combination;\n"));
  dbg_printf (_("       numeric values overflow;\n"));
  dbg_printf ("       %s\n", (time_zone_seen ? _("incorrect timezone")
                              : _("missing timezone")));
}

/* The original interface: run with debug=false and the default timezone.   */
bool
parse_datetime (struct timespec *result, char const *p,
                struct timespec const *now)
{
  char const *tzstring = getenv ("TZ");
  timezone_t tz = tzalloc (tzstring);
  if (!tz)
    return false;
  bool ok = parse_datetime2 (result, p, now, 0, tz, tzstring);
  tzfree (tz);
  return ok;
}

/* Parse a date/time string, storing the resulting time value into *RESULT.
   The string itself is pointed to by P.  Return true if successful.
   P can be an incomplete or relative time specification; if so, use
   *NOW as the basis for the returned time.  Default to timezone
   TZDEFAULT, which corresponds to tzalloc (TZSTRING).  */
bool
parse_datetime2 (struct timespec *result, char const *p,
                 struct timespec const *now, unsigned int flags,
                 timezone_t tzdefault, char const *tzstring)
{
  struct tm tm;
  struct tm tm0;
  char time_zone_buf[TIME_ZONE_BUFSIZE];
  char dbg_tm[DBGBUFSIZE];
  bool ok = false;
  char const *input_sentinel = p + strlen (p);
  char *tz1alloc = NULL;

  /* A reasonable upper bound for the size of ordinary TZ strings.
     Use heap allocation if TZ's length exceeds this.  */
  enum { TZBUFSIZE = 100 };
  char tz1buf[TZBUFSIZE];

  struct timespec gettime_buffer;
  if (! now)
    {
      gettime (&gettime_buffer);
      now = &gettime_buffer;
    }

  time_t Start = now->tv_sec;
  int Start_ns = now->tv_nsec;

  unsigned char c;
  while (c = *p, c_isspace (c))
    p++;

  timezone_t tz = tzdefault;

  /* Store a local copy prior to first "goto".  Without this, a prior use
     below of RELATIVE_TIME_0 on the RHS might translate to an assignment-
     to-temporary, which would trigger a -Wjump-misses-init warning.  */
  const relative_time rel_time_0 = RELATIVE_TIME_0;

  if (strncmp (p, "TZ=\"", 4) == 0)
    {
      char const *tzbase = p + 4;
      ptrdiff_t tzsize = 1;
      char const *s;

      for (s = tzbase; *s; s++, tzsize++)
        if (*s == '\\')
          {
            s++;
            if (! (*s == '\\' || *s == '"'))
              break;
          }
        else if (*s == '"')
          {
            timezone_t tz1;
            char *tz1string = tz1buf;
            char *z;
            if (TZBUFSIZE < tzsize)
              {
                tz1alloc = malloc (tzsize);
                if (!tz1alloc)
                  goto fail;
                tz1string = tz1alloc;
              }
            z = tz1string;
            for (s = tzbase; *s != '"'; s++)
              *z++ = *(s += *s == '\\');
            *z = '\0';
            tz1 = tzalloc (tz1string);
            if (!tz1)
              goto fail;
            tz = tz1;
            tzstring = tz1string;

            p = s + 1;
            while (c = *p, c_isspace (c))
              p++;

            break;
          }
    }

  struct tm tmp;
  if (! localtime_rz (tz, &now->tv_sec, &tmp))
    goto fail;

  /* As documented, be careful to treat the empty string just like
     a date string of "0".  Without this, an empty string would be
     declared invalid when parsed during a DST transition.  */
  if (*p == '\0')
    p = "0";

  parser_control pc;
  pc.input = p;
  pc.parse_datetime_debug = (flags & PARSE_DATETIME_DEBUG) != 0;
  if (INT_ADD_WRAPV (tmp.tm_year, TM_YEAR_BASE, &pc.year.value))
    {
      if (pc.parse_datetime_debug)
        dbg_printf (_("error: initial year out of range\n"));
      goto fail;
    }
  pc.year.digits = 0;
  pc.month = tmp.tm_mon + 1;
  pc.day = tmp.tm_mday;
  pc.hour = tmp.tm_hour;
  pc.minutes = tmp.tm_min;
  pc.seconds.tv_sec = tmp.tm_sec;
  pc.seconds.tv_nsec = Start_ns;
  tm.tm_isdst = tmp.tm_isdst;

  pc.meridian = MER24;
  pc.rel = rel_time_0;
  pc.timespec_seen = false;
  pc.rels_seen = false;
  pc.dates_seen = 0;
  pc.days_seen = 0;
  pc.times_seen = 0;
  pc.local_zones_seen = 0;
  pc.dsts_seen = 0;
  pc.zones_seen = 0;
  pc.year_seen = false;
  pc.debug_dates_seen = false;
  pc.debug_days_seen = false;
  pc.debug_times_seen = false;
  pc.debug_local_zones_seen = false;
  pc.debug_zones_seen = false;
  pc.debug_year_seen = false;
  pc.debug_ordinal_day_seen = false;

#if HAVE_STRUCT_TM_TM_ZONE
  pc.local_time_zone_table[0].name = tmp.tm_zone;
  pc.local_time_zone_table[0].type = tLOCAL_ZONE;
  pc.local_time_zone_table[0].value = tmp.tm_isdst;
  pc.local_time_zone_table[1].name = NULL;

  /* Probe the names used in the next three calendar quarters, looking
     for a tm_isdst different from the one we already have.  */
  {
    int quarter;
    for (quarter = 1; quarter <= 3; quarter++)
      {
        intmax_t iprobe;
        if (INT_ADD_WRAPV (Start, quarter * (90 * 24 * 60 * 60), &iprobe)
            || time_overflow (iprobe))
          break;
        time_t probe = iprobe;
        struct tm probe_tm;
        if (localtime_rz (tz, &probe, &probe_tm) && probe_tm.tm_zone
            && probe_tm.tm_isdst != pc.local_time_zone_table[0].value)
          {
              {
                pc.local_time_zone_table[1].name = probe_tm.tm_zone;
                pc.local_time_zone_table[1].type = tLOCAL_ZONE;
                pc.local_time_zone_table[1].value = probe_tm.tm_isdst;
                pc.local_time_zone_table[2].name = NULL;
              }
            break;
          }
      }
  }
#else
#if HAVE_TZNAME
  {
# if !HAVE_DECL_TZNAME
    extern char *tzname[];
# endif
    int i;
    for (i = 0; i < 2; i++)
      {
        pc.local_time_zone_table[i].name = tzname[i];
        pc.local_time_zone_table[i].type = tLOCAL_ZONE;
        pc.local_time_zone_table[i].value = i;
      }
    pc.local_time_zone_table[i].name = NULL;
  }
#else
  pc.local_time_zone_table[0].name = NULL;
#endif
#endif

  if (pc.local_time_zone_table[0].name && pc.local_time_zone_table[1].name
      && ! strcmp (pc.local_time_zone_table[0].name,
                   pc.local_time_zone_table[1].name))
    {
      /* This locale uses the same abbreviation for standard and
         daylight times.  So if we see that abbreviation, we don't
         know whether it's daylight time.  */
      pc.local_time_zone_table[0].value = -1;
      pc.local_time_zone_table[1].name = NULL;
    }

  if (yyparse (&pc) != 0)
    {
      if (pc.parse_datetime_debug)
        dbg_printf ((input_sentinel <= pc.input
                     ? _("error: parsing failed\n")
                     : _("error: parsing failed, stopped at '%s'\n")),
                    pc.input);
      goto fail;
    }


  /* Determine effective timezone source.  */

  if (pc.parse_datetime_debug)
    {
      dbg_printf (_("input timezone: "));

      if (pc.timespec_seen)
        fprintf (stderr, _("'@timespec' - always UTC"));
      else if (pc.zones_seen)
        fprintf (stderr, _("parsed date/time string"));
      else if (tzstring)
        {
          if (tz != tzdefault)
            fprintf (stderr, _("TZ=\"%s\" in date string"), tzstring);
          else if (STREQ (tzstring, "UTC0"))
            {
              /* Special case: 'date -u' sets TZ="UTC0".  */
              fprintf (stderr, _("TZ=\"UTC0\" environment value or -u"));
            }
          else
            fprintf (stderr, _("TZ=\"%s\" environment value"), tzstring);
        }
      else
        fprintf (stderr, _("system default"));

      /* Account for DST changes if tLOCAL_ZONE was seen.
         local timezone only changes DST and is relative to the
         default timezone.*/
      if (pc.local_zones_seen && !pc.zones_seen && 0 < pc.local_isdst)
        fprintf (stderr, ", dst");

      if (pc.zones_seen)
        fprintf (stderr, " (%s)", time_zone_str (pc.time_zone, time_zone_buf));

      fputc ('\n', stderr);
    }

  if (pc.timespec_seen)
    *result = pc.seconds;
  else
    {
      if (1 < (pc.times_seen | pc.dates_seen | pc.days_seen | pc.dsts_seen
               | (pc.local_zones_seen + pc.zones_seen)))
        {
          if (pc.parse_datetime_debug)
            {
              if (pc.times_seen > 1)
                dbg_printf ("error: seen multiple time parts\n");
              if (pc.dates_seen > 1)
                dbg_printf ("error: seen multiple date parts\n");
              if (pc.days_seen > 1)
                dbg_printf ("error: seen multiple days parts\n");
              if (pc.dsts_seen > 1)
                dbg_printf ("error: seen multiple daylight-saving parts\n");
              if ((pc.local_zones_seen + pc.zones_seen) > 1)
                dbg_printf ("error: seen multiple time-zone parts\n");
            }
          goto fail;
        }

      if (! to_tm_year (pc.year, pc.parse_datetime_debug, &tm.tm_year)
          || INT_ADD_WRAPV (pc.month, -1, &tm.tm_mon)
          || INT_ADD_WRAPV (pc.day, 0, &tm.tm_mday))
        {
          if (pc.parse_datetime_debug)
            dbg_printf (_("error: year, month, or day overflow\n"));
          goto fail;
        }
      if (pc.times_seen || (pc.rels_seen && ! pc.dates_seen && ! pc.days_seen))
        {
          tm.tm_hour = to_hour (pc.hour, pc.meridian);
          if (tm.tm_hour < 0)
            {
              char const *mrd = (pc.meridian == MERam ? "am"
                                 : pc.meridian == MERpm ?"pm" : "");
              if (pc.parse_datetime_debug)
                dbg_printf (_("error: invalid hour %"PRIdMAX"%s\n"),
                            pc.hour, mrd);
              goto fail;
            }
          tm.tm_min = pc.minutes;
          tm.tm_sec = pc.seconds.tv_sec;
          if (pc.parse_datetime_debug)
            dbg_printf ((pc.times_seen
                         ? _("using specified time as starting value: '%s'\n")
                         : _("using current time as starting value: '%s'\n")),
                        debug_strftime (&tm, dbg_tm, sizeof dbg_tm));
        }
      else
        {
          tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
          pc.seconds.tv_nsec = 0;
          if (pc.parse_datetime_debug)
            dbg_printf ("warning: using midnight as starting time: 00:00:00\n");
        }

      /* Let mktime deduce tm_isdst if we have an absolute timestamp.  */
      if (pc.dates_seen | pc.days_seen | pc.times_seen)
        tm.tm_isdst = -1;

      /* But if the input explicitly specifies local time with or without
         DST, give mktime that information.  */
      if (pc.local_zones_seen)
        tm.tm_isdst = pc.local_isdst;

      tm0.tm_sec = tm.tm_sec;
      tm0.tm_min = tm.tm_min;
      tm0.tm_hour = tm.tm_hour;
      tm0.tm_mday = tm.tm_mday;
      tm0.tm_mon = tm.tm_mon;
      tm0.tm_year = tm.tm_year;
      tm0.tm_isdst = tm.tm_isdst;
      tm.tm_wday = -1;

      Start = mktime_z (tz, &tm);

      if (! mktime_ok (&tm0, &tm))
        {
          bool repaired = false;
          bool time_zone_seen = pc.zones_seen != 0;
          if (time_zone_seen)
            {
              /* Guard against falsely reporting errors near the time_t
                 boundaries when parsing times in other time zones.  For
                 example, suppose the input string "1969-12-31 23:00:00 -0100",
                 the current time zone is 8 hours ahead of UTC, and the min
                 time_t value is 1970-01-01 00:00:00 UTC.  Then the min
                 localtime value is 1970-01-01 08:00:00, and mktime will
                 therefore fail on 1969-12-31 23:00:00.  To work around the
                 problem, set the time zone to 1 hour behind UTC temporarily
                 by setting TZ="XXX1:00" and try mktime again.  */

              char tz2buf[sizeof "XXX" - 1 + TIME_ZONE_BUFSIZE];
              tz2buf[0] = tz2buf[1] = tz2buf[2] = 'X';
              time_zone_str (pc.time_zone, &tz2buf[3]);
              timezone_t tz2 = tzalloc (tz2buf);
              if (!tz2)
                {
                  if (pc.parse_datetime_debug)
                    dbg_printf (_("error: tzalloc (\"%s\") failed\n"), tz2buf);
                  goto fail;
                }
              tm.tm_sec = tm0.tm_sec;
              tm.tm_min = tm0.tm_min;
              tm.tm_hour = tm0.tm_hour;
              tm.tm_mday = tm0.tm_mday;
              tm.tm_mon = tm0.tm_mon;
              tm.tm_year = tm0.tm_year;
              tm.tm_isdst = tm0.tm_isdst;
              tm.tm_wday = -1;
              Start = mktime_z (tz2, &tm);
              repaired = mktime_ok (&tm0, &tm);
              tzfree (tz2);
            }

          if (! repaired)
            {
              debug_mktime_not_ok (&tm0, &tm, &pc, time_zone_seen);
              goto fail;
            }
        }

      char dbg_ord[DBGBUFSIZE];

      if (pc.days_seen && ! pc.dates_seen)
        {
          intmax_t dayincr;
          if (INT_MULTIPLY_WRAPV ((pc.day_ordinal
                                   - (0 < pc.day_ordinal
                                      && tm.tm_wday != pc.day_number)),
                                  7, &dayincr)
              || INT_ADD_WRAPV ((pc.day_number - tm.tm_wday + 7) % 7,
                                dayincr, &dayincr)
              || INT_ADD_WRAPV (dayincr, tm.tm_mday, &tm.tm_mday))
            Start = -1;
          else
            {
              tm.tm_isdst = -1;
              Start = mktime_z (tz, &tm);
            }

          if (Start == (time_t) -1)
            {
              if (pc.parse_datetime_debug)
                dbg_printf (_("error: day '%s' "
                              "(day ordinal=%"PRIdMAX" number=%d) "
                              "resulted in an invalid date: '%s'\n"),
                            str_days (&pc, dbg_ord, sizeof dbg_ord),
                            pc.day_ordinal, pc.day_number,
                            debug_strfdatetime (&tm, &pc, dbg_tm,
                                                sizeof dbg_tm));
              goto fail;
            }

          if (pc.parse_datetime_debug)
            dbg_printf (_("new start date: '%s' is '%s'\n"),
                        str_days (&pc, dbg_ord, sizeof dbg_ord),
                        debug_strfdatetime (&tm, &pc, dbg_tm, sizeof dbg_tm));

        }

      if (pc.parse_datetime_debug)
        {
          if (!pc.dates_seen && !pc.days_seen)
            dbg_printf (_("using current date as starting value: '%s'\n"),
                        debug_strfdate (&tm, dbg_tm, sizeof dbg_tm));

          if (pc.days_seen && pc.dates_seen)
            dbg_printf (_("warning: day (%s) ignored when explicit dates "
                          "are given\n"),
                        str_days (&pc, dbg_ord, sizeof dbg_ord));

          dbg_printf (_("starting date/time: '%s'\n"),
                      debug_strfdatetime (&tm, &pc, dbg_tm, sizeof dbg_tm));
        }

      /* Add relative date.  */
      if (pc.rel.year | pc.rel.month | pc.rel.day)
        {
          if (pc.parse_datetime_debug)
            {
              if ((pc.rel.year != 0 || pc.rel.month != 0) && tm.tm_mday != 15)
                dbg_printf (_("warning: when adding relative months/years, "
                              "it is recommended to specify the 15th of the "
                              "months\n"));

              if (pc.rel.day != 0 && tm.tm_hour != 12)
                dbg_printf (_("warning: when adding relative days, "
                              "it is recommended to specify noon\n"));
            }

          int year, month, day;
          if (INT_ADD_WRAPV (tm.tm_year, pc.rel.year, &year)
              || INT_ADD_WRAPV (tm.tm_mon, pc.rel.month, &month)
              || INT_ADD_WRAPV (tm.tm_mday, pc.rel.day, &day))
            {
              if (pc.parse_datetime_debug)
                dbg_printf (_("error: %s:%d\n"), __FILE__, __LINE__);
              goto fail;
            }
          tm.tm_year = year;
          tm.tm_mon = month;
          tm.tm_mday = day;
          tm.tm_hour = tm0.tm_hour;
          tm.tm_min = tm0.tm_min;
          tm.tm_sec = tm0.tm_sec;
          tm.tm_isdst = tm0.tm_isdst;
          Start = mktime_z (tz, &tm);
          if (Start == (time_t) -1)
            {
              if (pc.parse_datetime_debug)
                dbg_printf (_("error: adding relative date resulted "
                              "in an invalid date: '%s'\n"),
                            debug_strfdatetime (&tm, &pc, dbg_tm,
                                                sizeof dbg_tm));
              goto fail;
            }

          if (pc.parse_datetime_debug)
            {
              dbg_printf (_("after date adjustment "
                            "(%+"PRIdMAX" years, %+"PRIdMAX" months, "
                            "%+"PRIdMAX" days),\n"),
                          pc.rel.year, pc.rel.month, pc.rel.day);
              dbg_printf (_("    new date/time = '%s'\n"),
                          debug_strfdatetime (&tm, &pc, dbg_tm,
                                              sizeof dbg_tm));

              /* Warn about crossing DST due to time adjustment.
                 Example: https://bugs.gnu.org/8357
                 env TZ=Europe/Helsinki \
                   date --debug \
                        -d 'Mon Mar 28 00:36:07 2011 EEST 1 day ago'

                 This case is different than DST changes due to time adjustment,
                 i.e., "1 day ago" vs "24 hours ago" are calculated in different
                 places.

                 'tm0.tm_isdst' contains the DST of the input date,
                 'tm.tm_isdst' is the normalized result after calling
                 mktime (&tm).
              */
              if (tm0.tm_isdst != -1 && tm.tm_isdst != tm0.tm_isdst)
                dbg_printf (_("warning: daylight saving time changed after "
                              "date adjustment\n"));

              /* Warn if the user did not ask to adjust days but mday changed,
                 or
                 user did not ask to adjust months/days but the month changed.

                 Example for first case:
                 2016-05-31 + 1 month => 2016-06-31 => 2016-07-01.
                 User asked to adjust month, but the day changed from 31 to 01.

                 Example for second case:
                 2016-02-29 + 1 year => 2017-02-29 => 2017-03-01.
                 User asked to adjust year, but the month changed from 02 to 03.
              */
              if (pc.rel.day == 0
                  && (tm.tm_mday != day
                      || (pc.rel.month == 0 && tm.tm_mon != month)))
                {
                  dbg_printf (_("warning: month/year adjustment resulted in "
                                "shifted dates:\n"));
                  char tm_year_buf[TM_YEAR_BUFSIZE];
                  dbg_printf (_("     adjusted Y M D: %s %02d %02d\n"),
                              tm_year_str (year, tm_year_buf), month + 1, day);
                  dbg_printf (_("   normalized Y M D: %s %02d %02d\n"),
                              tm_year_str (tm.tm_year, tm_year_buf),
                              tm.tm_mon + 1, tm.tm_mday);
                }
            }

        }

      /* The only "output" of this if-block is an updated Start value,
         so this block must follow others that clobber Start.  */
      if (pc.zones_seen)
        {
          intmax_t delta = pc.time_zone, t1;
          bool overflow = false;
#ifdef HAVE_TM_GMTOFF
          long int utcoff = tm.tm_gmtoff;
#else
          time_t t = Start;
          struct tm gmt;
          int utcoff = (gmtime_r (&t, &gmt)
                        ? tm_diff (&tm, &gmt)
                        : (overflow = true, 0));
#endif
          overflow |= INT_SUBTRACT_WRAPV (delta, utcoff, &delta);
          overflow |= INT_SUBTRACT_WRAPV (Start, delta, &t1);
          if (overflow || time_overflow (t1))
            {
              if (pc.parse_datetime_debug)
                dbg_printf (_("error: timezone %d caused time_t overflow\n"),
                            pc.time_zone);
              goto fail;
            }
          Start = t1;
        }

      if (pc.parse_datetime_debug)
        {
          intmax_t Starti = Start;
          dbg_printf (_("'%s' = %"PRIdMAX" epoch-seconds\n"),
                      debug_strfdatetime (&tm, &pc, dbg_tm, sizeof dbg_tm),
                      Starti);
        }


      /* Add relative hours, minutes, and seconds.  On hosts that support
         leap seconds, ignore the possibility of leap seconds; e.g.,
         "+ 10 minutes" adds 600 seconds, even if one of them is a
         leap second.  Typically this is not what the user wants, but it's
         too hard to do it the other way, because the time zone indicator
         must be applied before relative times, and if mktime is applied
         again the time zone will be lost.  */
      {
        intmax_t orig_ns = pc.seconds.tv_nsec;
        intmax_t sum_ns = orig_ns + pc.rel.ns;
        int normalized_ns = (sum_ns % BILLION + BILLION) % BILLION;
        int d4 = (sum_ns - normalized_ns) / BILLION;
        intmax_t d1, t1, d2, t2, t3, t4;
        if (INT_MULTIPLY_WRAPV (pc.rel.hour, 60 * 60, &d1)
            || INT_ADD_WRAPV (Start, d1, &t1)
            || INT_MULTIPLY_WRAPV (pc.rel.minutes, 60, &d2)
            || INT_ADD_WRAPV (t1, d2, &t2)
            || INT_ADD_WRAPV (t2, pc.rel.seconds, &t3)
            || INT_ADD_WRAPV (t3, d4, &t4)
            || time_overflow (t4))
          {
            if (pc.parse_datetime_debug)
              dbg_printf (_("error: adding relative time caused an "
                            "overflow\n"));
            goto fail;
          }

        result->tv_sec = t4;
        result->tv_nsec = normalized_ns;

        if (pc.parse_datetime_debug
            && (pc.rel.hour | pc.rel.minutes | pc.rel.seconds | pc.rel.ns))
          {
            dbg_printf (_("after time adjustment (%+"PRIdMAX" hours, "
                          "%+"PRIdMAX" minutes, "
                          "%+"PRIdMAX" seconds, %+d ns),\n"),
                        pc.rel.hour, pc.rel.minutes, pc.rel.seconds,
                        pc.rel.ns);
            dbg_printf (_("    new time = %"PRIdMAX" epoch-seconds\n"), t4);

            /* Warn about crossing DST due to time adjustment.
               Example: https://bugs.gnu.org/8357
               env TZ=Europe/Helsinki           \
               date --debug                                             \
               -d 'Mon Mar 28 00:36:07 2011 EEST 24 hours ago'

               This case is different than DST changes due to days adjustment,
               i.e., "1 day ago" vs "24 hours ago" are calculated in different
               places.

               'tm.tm_isdst' contains the date after date adjustment.  */
            struct tm lmt;
            if (tm.tm_isdst != -1 && localtime_rz (tz, &result->tv_sec, &lmt)
                && tm.tm_isdst != lmt.tm_isdst)
              dbg_printf (_("warning: daylight saving time changed after "
                            "time adjustment\n"));
          }
      }
    }

  if (pc.parse_datetime_debug)
    {
      /* Special case: using 'date -u' simply set TZ=UTC0 */
      if (! tzstring)
        dbg_printf (_("timezone: system default\n"));
      else if (STREQ (tzstring, "UTC0"))
        dbg_printf (_("timezone: Universal Time\n"));
      else
        dbg_printf (_("timezone: TZ=\"%s\" environment value\n"), tzstring);

      intmax_t sec = result->tv_sec;
      int nsec = result->tv_nsec;
      dbg_printf (_("final: %"PRIdMAX".%09d (epoch-seconds)\n"),
                  sec, nsec);

      struct tm gmt, lmt;
      bool got_utc = !!gmtime_r (&result->tv_sec, &gmt);
      if (got_utc)
        dbg_printf (_("final: %s (UTC)\n"),
                    debug_strfdatetime (&gmt, NULL,
                                        dbg_tm, sizeof dbg_tm));
      if (localtime_rz (tz, &result->tv_sec, &lmt))
        {
#ifdef HAVE_TM_GMTOFF
          bool got_utcoff = true;
          long int utcoff = lmt.tm_gmtoff;
#else
          bool got_utcoff = got_utc;
          int utcoff;
          if (got_utcoff)
            utcoff = tm_diff (&lmt, &gmt);
#endif
          if (got_utcoff)
            dbg_printf (_("final: %s (UTC%s)\n"),
                        debug_strfdatetime (&lmt, NULL, dbg_tm, sizeof dbg_tm),
                        time_zone_str (utcoff, time_zone_buf));
          else
            dbg_printf (_("final: %s (unknown time zone offset)\n"),
                        debug_strfdatetime (&lmt, NULL, dbg_tm, sizeof dbg_tm));
        }
    }

  ok = true;

 fail:
  if (tz != tzdefault)
    tzfree (tz);
  free (tz1alloc);
  return ok;
}

#if TEST

int
main (int ac, char **av)
{
  char buff[BUFSIZ];

  printf ("Enter date, or blank line to exit.\n\t> ");
  fflush (stdout);

  buff[BUFSIZ - 1] = '\0';
  while (fgets (buff, BUFSIZ - 1, stdin) && buff[0])
    {
      struct timespec d;
      struct tm const *tm;
      if (! parse_datetime (&d, buff, NULL))
        printf ("Bad format - couldn't convert.\n");
      else if (! (tm = localtime (&d.tv_sec)))
        {
          intmax_t sec = d.tv_sec;
          printf ("localtime (%"PRIdMAX") failed\n", sec);
        }
      else
        {
          int ns = d.tv_nsec;
          char tm_year_buf[TM_YEAR_BUFSIZE];
          printf ("%s-%02d-%02d %02d:%02d:%02d.%09d\n",
                  tm_year_str (tm->tm_year, tm_year_buf),
                  tm->tm_mon + 1, tm->tm_mday,
                  tm->tm_hour, tm->tm_min, tm->tm_sec, ns);
        }
      printf ("\t> ");
      fflush (stdout);
    }
  return 0;
}
#endif /* TEST */
