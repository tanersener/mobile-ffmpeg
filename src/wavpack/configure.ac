# wavpack 5.1.0 configure.ac

AC_INIT(wavpack, 5.1.0, bryant@wavpack.com)
AC_CONFIG_SRCDIR([src/pack.c])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

LIBWAVPACK_MAJOR=5
LIBWAVPACK_MINOR=1
LIBWAVPACK_MICRO=0
LIBWAVPACK_VERSION_STRING=$LIBWAVPACK_MAJOR.$LIBWAVPACK_MINOR.$LIBWAVPACK_MICRO

AC_DEFINE_UNQUOTED(LIBWAVPACK_MAJOR, $LIBWAVPACK_MAJOR, [libwavpack major version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_MINOR, $LIBWAVPACK_MINOR, [libwavpack minor version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_MICRO, $LIBWAVPACK_MICRO, [libwavpack micro version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_VERSION_STRING, "$LIBWAVPACK_VERSION_STRING", [libwavpack version string])

# set libtool versioning
# +1 :  0 : +1   == new interface that does not break old one.
# +1 :  0 :  0   == changed/removed an interface. Breaks old apps.
#  ? : +1 :  ?   == internal changes that doesn't break anything.
# CURRENT : REVISION : AGE
LT_CURRENT=3
LT_REVISION=0
LT_AGE=2

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Check for os version
VERSION_OS=$(uname -s)
AC_DEFINE_UNQUOTED(VERSION_OS, "$VERSION_OS", [os version])

# Check host so we can infer target CPU (and assembly optimizations)
AC_CANONICAL_HOST

# Check for processor characteristics
AC_C_BIGENDIAN(AC_DEFINE([HIGHFIRST], [1], [big-endian machine]))

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AM_PROG_CC_C_O
AM_PROG_AS

AC_HEADER_STDC

# Checks for libraries.
AC_CHECK_LIBM
AC_SUBST(LIBM)

# Check for large files support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

# Check if cross-compiling on mingw
AC_MINGW32

AC_ARG_ENABLE([apps],
    AS_HELP_STRING([--disable-apps], [build only libwavpack (removes ICONV dependency)]))

AM_CONDITIONAL([ENABLE_APPS], [test "x$enable_apps" != "xno"])

if test "x$enable_apps" != "xno" ; then
if test "x$MINGW32" != "xyes" ; then
# Check for iconv
AC_ARG_WITH(iconv, [[  --with-iconv[=DIR]      add ICONV support [yes]]])
if test "$with_iconv" = "no" ; then
    AC_MSG_ERROR([[iconv is required for apps, use --disable-apps to build only libwavpack]])
else
    if test "$with_iconv" != "yes" -a "$with_iconv" != "" ; then
       ICONV_CFLAGS="${CPPFLAGS} -I$with_iconv/include"
       ICONV_LIBS="-L$with_iconv/lib"
    fi

    AC_CHECK_HEADER(iconv.h,
        AC_MSG_CHECKING(for iconv)
        AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
            AC_MSG_RESULT(yes)
            WITH_ICONV=1],[
            AC_MSG_RESULT(no)
            AC_MSG_CHECKING(for iconv in -liconv)

            old_cflags="${CFLAGS}"
            old_ldflags="${LDFLAGS}"
            old_libs="${LIBS}"
            CFLAGS="${ICONV_CFLAGS} ${CFLAGS}"
            LDFLAGS="${LDFLAGS} ${ICONV_LIBS}"
            LIBS="${LIBS} -liconv"

            AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
                AC_MSG_RESULT(yes)
                WITH_ICONV=1
                ICONV_LIBS="${ICONV_LIBS} -liconv"
                ICONV="-liconv"],[
                AC_MSG_ERROR([[Can't find iconv libraries.]])])
            CFLAGS=$old_cflags
            LDFLAGS=$old_ldflags
            LIBS=$old_libs
            ]),
            AC_MSG_ERROR([[Can't find iconv headers.]]))
fi
AC_SUBST(ICONV_LIBS)
AC_SUBST(ICONV)
fi
fi

AC_MSG_CHECKING([if we are building for a Windows host])
AS_CASE([$host_os],
    [*mingw*], [windows_host=yes],
    [windows_host=no])
AC_MSG_RESULT([$windows_host])

AM_CONDITIONAL([WINDOWS_HOST], [test "x$windows_host" = "xyes"])

AC_ARG_ENABLE([legacy],
    AS_HELP_STRING([--enable-legacy], [decode legacy (< 4.0) WavPack files]))

AS_IF([test "x$enable_legacy" = "xyes"],
    [AC_DEFINE([ENABLE_LEGACY])])

AM_CONDITIONAL([ENABLE_LEGACY], [test "x$enable_legacy" == "xyes"])

AC_ARG_ENABLE([dsd],
    AS_HELP_STRING([--disable-dsd], [disable support for WavPack DSD files]))

AS_IF([test "x$enable_dsd" != "xno"],
    [AC_DEFINE([ENABLE_DSD])])

AM_CONDITIONAL([ENABLE_DSD], [test "x$enable_dsd" != "xno"])

AC_ARG_ENABLE([rpath],
    AS_HELP_STRING([--enable-rpath], [hardcode library path in executables]))

AM_CONDITIONAL([ENABLE_RPATH], [test "x$enable_rpath" == "xyes"])

AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--enable-tests], [build libwavpack test program (requires Pthreads)]))

AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" == "xyes"])

AC_ARG_ENABLE([asm],
    [AS_HELP_STRING([--disable-asm], [disable assembly optimizations])],,
    [enable_asm=check])

asm_selected=none

AS_IF([test "x$enable_asm" != "xno"],[
  AC_MSG_CHECKING(if assembly optimizations are available)
  AS_CASE([$host_cpu],
    [i386|i486|i586|i686|i786], [AC_DEFINE([OPT_ASM_X86]) asm_selected=x86],
    [x86_64], [AC_DEFINE([OPT_ASM_X64]) asm_selected=x64],
    [arm*], [AC_DEFINE([OPT_ASM_ARM]) asm_selected=arm],
    [AS_IF([test "x$enable_asm" = "xyes"],[AC_MSG_ERROR(no assembly code for CPU $host_cpu)])]
  )
  AC_MSG_RESULT($asm_selected)
])

AM_CONDITIONAL([ENABLE_X86ASM], [test "x$asm_selected" = "xx86"])
AM_CONDITIONAL([ENABLE_X64ASM], [test "x$asm_selected" = "xx64"])
AM_CONDITIONAL([ENABLE_ARMASM], [test "x$asm_selected" = "xarm"])

dnl Check for __builtin_clz
AC_CACHE_CHECK([for __builtin_clz], ac_cv_have___builtin_clz,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[return __builtin_clz(1)]])],
               [ac_cv_have___builtin_clz="yes"], [ac_cv_have___builtin_clz="no"]))
if test "$ac_cv_have___builtin_clz" = "yes"
then
    AC_DEFINE([HAVE___BUILTIN_CLZ])
fi

dnl Check for __builtin_ctz
AC_CACHE_CHECK([for __builtin_ctz], ac_cv_have___builtin_ctz,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[return __builtin_ctz(1)]])],
               [ac_cv_have___builtin_ctz="yes"], [ac_cv_have___builtin_ctz="no"]))
if test "$ac_cv_have___builtin_ctz" = "yes"
then
    AC_DEFINE([HAVE___BUILTIN_CTZ])
fi

AC_ARG_ENABLE(man,
              [AC_HELP_STRING([--enable-man],
                              [regenerate man pages from Docbook [default=no]])],enable_man=yes,
              enable_man=no)

if test "${enable_man}" != no; then
  dnl
  dnl Check for xsltproc
  dnl
  AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man=no
  fi

  dnl check for DocBook DTD and stylesheets in the local catalog.
  JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
     [DocBook XML DTD V4.1.2],,enable_man=no)
  JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
     [DocBook XSL Stylesheets],,enable_man=no)
fi

AM_CONDITIONAL(ENABLE_MAN, test x$enable_man != xno)

AC_CONFIG_FILES(
Makefile
wavpack.pc
src/Makefile
include/Makefile
cli/Makefile
man/Makefile)
AC_OUTPUT
