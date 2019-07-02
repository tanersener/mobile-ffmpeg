/* Provide tzset for systems that don't have it or for which it's broken.

   Copyright (C) 2001-2003, 2005-2007, 2009-2019 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* written by Jim Meyering */

#include <config.h>

/* Specification.  */
#include <time.h>

#include "localtime-buffer.h"

/* This is a wrapper for tzset, for systems on which tzset may clobber
   the static buffer used for localtime's result.
   Work around the bug in some systems whereby tzset clobbers the
   static buffer that localtime uses for its return value.  The
   tzset function from Solaris 2.5, 2.5.1, and 2.6 has this problem.  */

void
tzset (void)
#undef tzset
{
#if TZSET_CLOBBERS_LOCALTIME
  /* Save and restore the contents of the buffer used for localtime's
     result around the call to tzset.  */
  struct tm save = *localtime_buffer_addr;
#endif

#if defined _WIN32 && ! defined __CYGWIN__
  /* Rectify the value of the environment variable TZ.
     There are four possible kinds of such values:
       - Traditional US time zone names, e.g. "PST8PDT".  Syntax: see
         <https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tzset>
       - Time zone names based on geography, that contain one or more
         slashes, e.g. "Europe/Moscow".
       - Time zone names based on geography, without slashes, e.g.
         "Singapore".
       - Time zone names that contain explicit DST rules.  Syntax: see
         <http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_03>
     The Microsoft CRT understands only the first kind.  It produces incorrect
     results if the value of TZ is of the other kinds.
     But in a Cygwin environment, /etc/profile.d/tzset.sh sets TZ to a value
     of the second kind for most geographies, or of the first kind in a few
     other geographies.  If it is of the second kind, neutralize it.  For the
     Microsoft CRT, an absent or empty TZ means the time zone that the user
     has set in the Windows Control Panel.
     If the value of TZ is of the third or fourth kind -- Cygwin programs
     understand these syntaxes as well --, it does not matter whether we
     neutralize it or not, since these values occur only when a Cygwin user
     has set TZ explicitly; this case is 1. rare and 2. under the user's
     responsibility.  */
  const char *tz = getenv ("TZ");
  if (tz != NULL && strchr (tz, '/') != NULL)
    _putenv ("TZ=");

  /* On native Windows, tzset() is deprecated.  Use _tzset() instead.  See
     <https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/posix-tzset>
     <https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tzset>  */
  _tzset ();
#elif HAVE_TZSET
  tzset ();
#else
  /* Do nothing.  Avoid infinite recursion.  */
#endif

#if TZSET_CLOBBERS_LOCALTIME
  *localtime_buffer_addr = save;
#endif
}
