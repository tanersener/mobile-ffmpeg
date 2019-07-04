# serial 12

# Copyright (C) 2003, 2007, 2009-2019 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# See if we have a working tzset function.
# If so, arrange to compile the wrapper function.
# For at least Solaris 2.5.1 and 2.6, this is necessary
# because tzset can clobber the contents of the buffer
# used by localtime.

# Written by Paul Eggert and Jim Meyering.

AC_DEFUN([gl_FUNC_TZSET],
[
  AC_REQUIRE([gl_HEADER_TIME_H_DEFAULTS])
  AC_REQUIRE([gl_LOCALTIME_BUFFER_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CHECK_FUNCS_ONCE([tzset])
  if test $ac_cv_func_tzset = no; then
    HAVE_TZSET=0
  fi
  gl_FUNC_TZSET_CLOBBER
  REPLACE_TZSET=0
  case "$gl_cv_func_tzset_clobber" in
    *yes)
      REPLACE_TZSET=1
      AC_DEFINE([TZSET_CLOBBERS_LOCALTIME], [1],
        [Define if tzset clobbers localtime's static buffer.])
      gl_LOCALTIME_BUFFER_NEEDED
      ;;
  esac
  case "$host_os" in
    mingw*) REPLACE_TZSET=1 ;;
  esac
])

# Set gl_cv_func_tzset_clobber.
AC_DEFUN([gl_FUNC_TZSET_CLOBBER],
[
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether tzset clobbers localtime buffer],
                 [gl_cv_func_tzset_clobber],
    [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <time.h>
#include <stdlib.h>

int
main ()
{
  time_t t1 = 853958121;
  struct tm *p, s;
  putenv ("TZ=GMT0");
  p = localtime (&t1);
  s = *p;
  putenv ("TZ=EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00");
  tzset ();
  return (p->tm_year != s.tm_year
          || p->tm_mon != s.tm_mon
          || p->tm_mday != s.tm_mday
          || p->tm_hour != s.tm_hour
          || p->tm_min != s.tm_min
          || p->tm_sec != s.tm_sec);
}
  ]])],
       [gl_cv_func_tzset_clobber=no],
       [gl_cv_func_tzset_clobber=yes],
       [case "$host_os" in
                         # Guess all is fine on glibc systems.
          *-gnu* | gnu*) gl_cv_func_tzset_clobber="guessing no" ;;
                         # Guess all is fine on musl systems.
          *-musl*)       gl_cv_func_tzset_clobber="guessing no" ;;
                         # Guess no on native Windows.
          mingw*)        gl_cv_func_tzset_clobber="guessing no" ;;
                         # If we don't know, assume the worst.
          *)             gl_cv_func_tzset_clobber="guessing yes" ;;
        esac
       ])
    ])

  AC_DEFINE([HAVE_RUN_TZSET_TEST], [1],
    [Define to 1 if you have run the test for working tzset.])
])
