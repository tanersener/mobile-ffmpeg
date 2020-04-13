# valgrind-tests.m4 serial 4
dnl Copyright (C) 2008-2020 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson

# gl_VALGRIND_TESTS()
# -------------------
# Check if valgrind is available, and set VALGRIND to it if available.
AC_DEFUN([gl_VALGRIND_TESTS],
[
  AC_ARG_ENABLE([valgrind-tests],
    AS_HELP_STRING([--enable-valgrind-tests],
                   [try to run self tests under valgrind]),
    [opt_valgrind_tests=$enableval], [opt_valgrind_tests=no])

  # Run self-tests under valgrind?
  if test "$opt_valgrind_tests" = "yes" && test "$cross_compiling" = no; then
    AC_CHECK_PROGS([VALGRIND], [valgrind])

    if test "$VALGRIND"; then
      AC_CACHE_CHECK([for valgrind options for tests],
        [gl_cv_opt_valgrind_tests],
        [gl_cv_opt_valgrind_tests="-q --error-exitcode=1 --leak-check=full --track-origins=yes --gen-suppressions=all --suppressions=\$(srcdir)/suppressions.valgrind"
         $VALGRIND $gl_valgrind_opts ls > /dev/null 2>&1 ||
           gl_cv_opt_valgrind_tests=no])

      if test "$gl_cv_opt_valgrind_tests" != no; then
        VALGRIND="$VALGRIND $gl_cv_opt_valgrind_tests"
      else
        VALGRIND=
      fi
    fi
  fi
])
