# parse-datetime.m4 serial 22
dnl Copyright (C) 2002-2006, 2008-2019 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Define HAVE_COMPOUND_LITERALS if the C compiler supports compound literals
dnl as in ISO C99.
dnl Note that compound literals such as (struct s) { 3, 4 } can be used for
dnl initialization of stack-allocated variables, but are not constant
dnl expressions and therefore cannot be used as initializer for global or
dnl static variables (even though gcc supports this in pre-C99 mode).
AC_DEFUN([gl_C_COMPOUND_LITERALS],
[
  AC_CACHE_CHECK([for compound literals], [gl_cv_compound_literals],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[struct s { int i, j; };]],
      [[struct s t = (struct s) { 3, 4 };
        if (t.i != 0) return 0;]])],
    gl_cv_compound_literals=yes,
    gl_cv_compound_literals=no)])
  if test $gl_cv_compound_literals = yes; then
    AC_DEFINE([HAVE_COMPOUND_LITERALS], [1],
      [Define if you have compound literals.])
  fi
])

AC_DEFUN([gl_PARSE_DATETIME],
[
  dnl Prerequisites of lib/parse-datetime.h.
  AC_REQUIRE([AM_STDBOOL_H])
  AC_REQUIRE([gl_TIMESPEC])

  dnl Prerequisites of lib/parse-datetime.y.
  AC_REQUIRE([gl_BISON])
  AC_REQUIRE([gl_C_COMPOUND_LITERALS])
  AC_STRUCT_TIMEZONE
  AC_REQUIRE([gl_CLOCK_TIME])
  AC_REQUIRE([gl_TM_GMTOFF])
])
