# parse-datetime.m4 serial 23
dnl Copyright (C) 2002-2006, 2008-2020 Free Software Foundation, Inc.
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
  dnl parse-datetime.c is generated from parse-datetime.y. It requires bison,
  dnl because parse-datetime.y uses bison specific features. It requires at
  dnl least bison-2.4 for %define api.pure.
  dnl bison is only needed for the maintainer (who touches parse-datetime.y).
  dnl But in order to avoid separate Makefiles or --enable-maintainer-mode,
  dnl we put the rule in general Makefile. Now, some people carelessly touch
  dnl the files or have a broken "make" program, hence the parse-datetime.c
  dnl rule will sometimes fire. To avoid an error, defines PARSE_DATETIME_BISON
  dnl to ":" if it is not present or too old.
  AC_CHECK_PROGS([PARSE_DATETIME_BISON], [bison])
  if test -z "$PARSE_DATETIME_BISON"; then
    ac_verc_fail=yes
  else
    dnl Found it, now check the version.
    AC_MSG_CHECKING([version of bison])
changequote(<<,>>)dnl
    ac_prog_version=`$PARSE_DATETIME_BISON --version 2>&1 | sed -n 's/^.*GNU Bison.* \([0-9]*\.[0-9.]*\).*$/\1/p'`
    case $ac_prog_version in
      '') ac_prog_version="v. ?.??, bad"; ac_verc_fail=yes;;
      1.* | 2.[0-3] | 2.[0-3].*)
changequote([,])dnl
         ac_prog_version="$ac_prog_version, bad"; ac_verc_fail=yes;;
      *) ac_prog_version="$ac_prog_version, ok"; ac_verc_fail=no;;
    esac
    AC_MSG_RESULT([$ac_prog_version])
  fi
  if test $ac_verc_fail = yes; then
    PARSE_DATETIME_BISON=:
  fi
  AC_SUBST([PARSE_DATETIME_BISON])

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
