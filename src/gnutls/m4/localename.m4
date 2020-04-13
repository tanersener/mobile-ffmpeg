# localename.m4 serial 7
dnl Copyright (C) 2007, 2009-2020 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_LOCALENAME],
[
  AC_REQUIRE([gl_LOCALE_H_DEFAULTS])
  AC_REQUIRE([gl_LOCALE_T])
  AC_REQUIRE([gt_LC_MESSAGES])
  AC_REQUIRE([gt_INTL_THREAD_LOCALE_NAME])
  AC_REQUIRE([gt_INTL_MACOSX])
  AC_CHECK_HEADERS_ONCE([langinfo.h])
  if test $HAVE_LOCALE_T = 1; then
    AC_CHECK_FUNCS_ONCE([newlocale duplocale freelocale])
    gl_func_newlocale="$ac_cv_func_newlocale"
    gl_func_duplocale="$ac_cv_func_duplocale"
    gl_func_freelocale="$ac_cv_func_freelocale"
  else
    dnl In 2019, some versions of z/OS lack the locale_t type and have broken
    dnl newlocale, duplocale, freelocale functions.
    gl_func_newlocale=no
    gl_func_duplocale=no
    gl_func_freelocale=no
  fi
  if test $gl_func_newlocale != yes; then
    HAVE_NEWLOCALE=0
  fi
  if test $gl_func_duplocale != yes; then
    HAVE_DUPLOCALE=0
  fi
  if test $gl_func_freelocale != yes; then
    HAVE_FREELOCALE=0
  fi
  if test $gt_nameless_locales = yes; then
    REPLACE_NEWLOCALE=1
    REPLACE_DUPLOCALE=1
    REPLACE_FREELOCALE=1
  fi
])
