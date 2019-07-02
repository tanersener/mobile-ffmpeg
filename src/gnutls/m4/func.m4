# func.m4 serial 2
dnl Copyright (C) 2008-2019 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Written by Simon Josefsson

AC_DEFUN([gl_FUNC],
[
  AC_CACHE_CHECK([whether __func__ is available], [gl_cv_var_func],
     AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[]], [[const char *str = __func__;]])],
       [gl_cv_var_func=yes],
       [gl_cv_var_func=no]))
  if test "$gl_cv_var_func" != yes; then
    AC_DEFINE([__func__], ["<unknown function>"],
              [Define as a replacement for the ISO C99 __func__ variable.])
  fi
])
