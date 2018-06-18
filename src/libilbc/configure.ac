#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([libilbc], [2.0.2], [iLBC Codec])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar dist-bzip2 foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Setup for libtool
LT_INIT

# Checks for programs.
AC_PROG_CC

DOLT

AC_SUBST(LIBTOOL_DEPS)

AX_PTHREAD([
    AC_DEFINE([WEBRTC_POSIX], [1],
              [Define if pthreads is available])
    LIBS="$PTHREAD_LIBS $LIBS"
    CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
    CC="$PTHREAD_CC"
])

AC_SUBST(PTHREAD_LIBS)

# soname version to use
# goes by ‘current[:revision[:age]]’ with the soname ending up as
# current.age.revision.
LIBILBC_VERSION=2:2:0
AC_SUBST(LIBILBC_VERSION)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
                 libilbc.pc])
AC_OUTPUT
