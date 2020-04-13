# serial 7

# Copyright (C) 2002, 2005, 2009-2020 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_BISON],
[
  # parse-datetime.y works with bison only.
  : ${YACC='bison -o y.tab.c'}
dnl
dnl Declaring YACC & YFLAGS precious will not be necessary after GNULIB
dnl requires an Autoconf greater than 2.59c, but it will probably still be
dnl useful to override the description of YACC in the --help output, re
dnl parse-datetime.y assuming 'bison -o y.tab.c'.
  AC_ARG_VAR([YACC],
[The "Yet Another C Compiler" implementation to use.  Defaults to
'bison -o y.tab.c'.  Values other than 'bison -o y.tab.c' will most likely
break on most systems.])dnl
  AC_ARG_VAR([YFLAGS],
[YFLAGS contains the list arguments that will be passed by default to Bison.
This script will default YFLAGS to the empty string to avoid a default value of
'-d' given by some make applications.])dnl
])
