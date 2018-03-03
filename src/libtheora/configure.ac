dnl Process this file with autoconf to produce a configure script

dnl ------------------------------------------------
dnl Initialization and Versioning
dnl ------------------------------------------------

AC_INIT(libtheora,[1.1.1])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([lib/fdct.c])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

dnl Library versioning
dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0

TH_LIB_CURRENT=3
TH_LIB_REVISION=10
TH_LIB_AGE=3
AC_SUBST(TH_LIB_CURRENT)
AC_SUBST(TH_LIB_REVISION)
AC_SUBST(TH_LIB_AGE)

THDEC_LIB_CURRENT=2
THDEC_LIB_REVISION=4
THDEC_LIB_AGE=1
AC_SUBST(THDEC_LIB_CURRENT)
AC_SUBST(THDEC_LIB_REVISION)
AC_SUBST(THDEC_LIB_AGE)

THENC_LIB_CURRENT=2
THENC_LIB_REVISION=2
THENC_LIB_AGE=1
AC_SUBST(THENC_LIB_CURRENT)
AC_SUBST(THENC_LIB_REVISION)
AC_SUBST(THENC_LIB_AGE)

dnl Extra linker options (for version script)
THEORA_LDFLAGS=""

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

AM_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
        AC_MSG_WARN([*** doxygen not found, API documentation will not be built])
fi

dnl Check for tools used to build the format specification
BUILD_SPEC="false"
ac_build_spec=yes
AC_ARG_ENABLE(spec,
     [  --disable-spec          do not build spec ],
     [
       if test "x$enableval" = "xno"; then
         ac_build_spec=$enableval
       fi
     ], [
       ac_build_spec=yes
     ] )
if test "x$ac_build_spec" = "xyes"; then
  AC_CHECK_PROG(HAVE_PDFLATEX, pdflatex, yes)
  AC_CHECK_PROG(HAVE_BIBTEX, bibtex, yes)
  AC_CHECK_PROG(HAVE_TRANSFIG, fig2dev, yes)
  if test -r doc/spec/spec.tex; then
   if test "x$HAVE_PDFLATEX" = "xyes"; then
    if test "x$HAVE_BIBTEX" = "xyes"; then
      if test "x$HAVE_TRANSFIG" = "xyes"; then
        tex_pkg_list=`fgrep usepackage doc/spec/spec.tex | grep \{ | grep -v ltablex`
        tex_pkg_ok="yes"
        for pkg_line in $tex_pkg_list; do
	  pkg_name=`echo $pkg_line | sed -e 's/.*{\(.*\)}.*/\1/'`
	  AC_MSG_CHECKING([for Tex package $pkg_name])
          cat >conftest.tex <<_ACEOF
\\documentclass{book}
$pkg_line
\\begin{document}
Hello World.
\\end{document}
_ACEOF
	  if pdflatex -interaction batchmode -halt-on-error conftest < /dev/null > /dev/null 2>&1; then
            AC_MSG_RESULT([ok])
          else
	    tex_pkg_ok="no"
	    AC_MSG_RESULT([no])
          fi
        done
        if test -w conftest.tex; then rm conftest.tex; fi
        if test -w conftest.tex; then rm conftest.aux; fi
        if test -w conftest.pdf; then rm conftest.pdf; fi
        if test "x$tex_pkg_ok" = "xyes"; then
          BUILD_SPEC="true"
        fi
      fi
    fi
   fi
  fi
fi
AM_CONDITIONAL(BUILD_SPEC, $BUILD_SPEC)
if test $BUILD_SPEC = "false"; then
	AC_MSG_WARN([*** Format Specification will not built.])
fi

dnl Check for valgrind
VALGRIND_ENVIRONMENT=""
ac_enable_valgrind=no
AC_ARG_ENABLE(valgrind-testing,
     [  --enable-valgrind-testing     enable running of tests inside Valgrind ],     [ ac_enable_valgrind=yes ], [ ac_enable_valgrind=no] )

if test "x${ac_enable_valgrind}" = xyes ; then
  if test "x${enable_shared}" = xyes ; then
    VALGRIND_ENVIRONMENT="libtool --mode=execute "
  fi

  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "x$HAVE_VALGRIND" = xyes ; then
    VALGRIND_ENVIRONMENT="$VALGRIND_ENVIRONMENT valgrind -q --leak-check=yes --show-reachable=yes --num-callers=100"
    AC_SUBST(VALGRIND_ENVIRONMENT)
    TESTS_INFO="Type 'make check' to run test suite. Tests will be run under:
  ${VALGRIND_ENVIRONMENT}"
  else
    TESTS_INFO="Type 'make check' to run test suite (Valgrind not found)"
  fi
else
  TESTS_INFO="Type 'make check' to run test suite (Valgrind testing not enabled)"
fi

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

dnl Set some target options

cflags_save="$CFLAGS"
if test -z "$GCC"; then
        case $host in 
        *)
                DEBUG="-g -DDEBUG"
                CFLAGS="-O"
                PROFILE="-g -p -DDEBUG" ;;
        esac
else

        case $host in 
        *)
                DEBUG="-g -Wall -Wno-parentheses -DDEBUG -D__NO_MATH_INLINES"
                CFLAGS="-Wall -Wno-parentheses -O3 -fforce-addr -fomit-frame-pointer -finline-functions -funroll-loops"
                PROFILE="-Wall -Wno-parentheses -pg -g -O3 -fno-inline-functions -DDEBUG";;
        esac
fi
CFLAGS="$CFLAGS $cflags_save"

cpu_x86_64=no
cpu_x86_32=no
AC_ARG_ENABLE(asm,
    [  --disable-asm           disable assembly optimizations ],
    [ ac_enable_asm=$enableval ], [ ac_enable_asm=yes] )

if test "x${ac_enable_asm}" = xyes; then
  cpu_optimization="no optimization for your platform, please send a patch"
  case $target_cpu in
  i[[3456]]86)
    cpu_x86_32=yes 
    cpu_optimization="32 bit x86"
    AC_DEFINE([OC_X86_ASM], [],  [make use of x86 asm optimization])
    if test "x$target_vendor" = "xapple"; then
      THEORA_LDFLAGS="$THEORA_LDFLAGS  -Wl,-read_only_relocs,suppress"
    fi
      ;;
  x86_64)
    cpu_x86_64=yes
    cpu_optimization="64 bit x86"
    AC_DEFINE([OC_X86_ASM], [],  [make use of x86 asm optimization])
    AC_DEFINE([OC_X86_64_ASM], [],  [make use of x86_64 asm optimization])
    ;;
  esac
else
  cpu_optimization="disabled"
fi
AM_CONDITIONAL([CPU_x86_64], [test x$cpu_x86_64 = xyes])
AM_CONDITIONAL([CPU_x86_32], [test x$cpu_x86_32 = xyes])

# Test whenever ld supports -version-script
AC_PROG_LD
AC_PROG_LD_GNU
AC_MSG_CHECKING([how to control symbol export])

THDEC_VERSION_ARG=""
THENC_VERSION_ARG=""
TH_VERSION_ARG=""
if test "x$lt_cv_prog_gnu_ld" = "xyes"; then
 case "$target_os" in
  *mingw*)
    THEORA_LDFLAGS="$THEORA_LDFLAGS -no-undefined"
    THDEC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/xmingw32/libtheoradec-all.def"
    THENC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/xmingw32/libtheoraenc-all.def"
    THENC_VERSION_ARG="$THENC_VERSION_ARG -ltheoradec"
    THC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/libtheora.def"
    AC_MSG_RESULT([-export-symbols])
    ;;
  linux* | solaris* )
    THDEC_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script-dec'
    THENC_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script-enc'
    TH_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script'
    AC_MSG_RESULT([--version-script])
    ;;
  *)
    # build without versioning
    AC_MSG_RESULT([no])
    ;;
 esac
else
 case "$target_os" in
  darwin*)
    THDEC_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theoradec.exp'
    THENC_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theoraenc.exp'
    TH_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theora.exp'
    AC_MSG_RESULT([-exported_symbols_list])
    ;;
  *)
    # build without versioning
    AC_MSG_RESULT([no])
    ;;
 esac
fi
 
THEORADEC_LDFLAGS="$THEORA_LDFLAGS $THDEC_VERSION_ARG"
THEORAENC_LDFLAGS="$THEORA_LDFLAGS $THENC_VERSION_ARG"
THEORA_LDFLAGS="$THEORA_LDFLAGS $TH_VERSION_ARG"
AC_SUBST(THEORADEC_LDFLAGS)
AC_SUBST(THEORAENC_LDFLAGS)
AC_SUBST(THEORA_LDFLAGS)

dnl --------------------------------------------------
dnl Checks for support libraries and headers
dnl --------------------------------------------------

dnl check for Ogg
HAVE_OGG=no

dnl first check through pkg-config since it's more flexible

dnl check for pkg-config itself so we don't try the m4 macro without pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(OGG, ogg >= 1.1, HAVE_OGG=yes, HAVE_OGG=no)
fi
if test "x$HAVE_OGG" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_OGG(, AC_MSG_ERROR([
    libogg is required to build this package!
    please see http://www.xiph.org/ for how to
    obtain a copy.
  ]))
  cflags_save=$CFLAGS
  libs_save=$LIBS
  CFLAGS="$CFLAGS $OGG_CFLAGS"
  LIBS="$LIBS $OGG_LIBS"
  AC_CHECK_FUNC(oggpackB_read, , [
    AC_MSG_ERROR([newer libogg version (1.1 or later) required])
  ])
  CFLAGS=$cflags_save
  LIBS=$libs_save
fi


dnl check for Vorbis
HAVE_VORBIS=no

dnl first check through pkg-config since it's more flexible

if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(VORBIS, vorbis >= 1.0.1, HAVE_VORBIS=yes, HAVE_VORBIS=no)
  dnl also set VORBISENC_LIBS since an examples needs it
  dnl the old .m4 sets this to a value to use on top of VORBIS_LIBS,
  dnl so we do the same here.
  VORBISENC_LIBS="-lvorbisenc"
  AC_SUBST(VORBISENC_LIBS)
fi
if test "x$HAVE_VORBIS" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_VORBIS(HAVE_VORBIS=yes, HAVE_VORBIS=no)
fi

dnl check for SDL
HAVE_SDL=no

AM_PATH_SDL(,[
  HAVE_SDL=yes
  SDL_LIBS=`$SDL_CONFIG --libs`
],AC_MSG_WARN([*** Unable to find SDL -- Not compiling example players ***]))

dnl check for OSS
HAVE_OSS=no
AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h],[
  HAVE_OSS=yes
  break
])
if test x$HAVE_OSS != xyes; then
  AC_MSG_WARN([OSS audio support not found -- not compiling player_example])
fi

dnl OpenBSD needs -lossaudio to use the oss interface
OSS_LIBS=
case "$target_os" in
  openbsd*)
    OSS_LIBS='-lossaudio'
    ;;
esac
AC_SUBST(OSS_LIBS)

dnl check for libpng
HAVE_PNG=no
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(PNG, libpng, HAVE_PNG=yes, HAVE_PNG=no)
fi
AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)

dnl check for libcairo
HAVE_CAIRO=no
AC_ARG_ENABLE(telemetry,
    [  --enable-telemetry      enable debugging output controls ],
    [ ac_enable_telemetry=$enableval ], [ ac_enable_telemetry=no] )

if test "x${ac_enable_telemetry}" = xyes; then
   if test "x$HAVE_PKG_CONFIG" = "xyes"
   then
     PKG_CHECK_MODULES(CAIRO, cairo, HAVE_CAIRO=yes, HAVE_CAIRO=no)
     AC_DEFINE([HAVE_CAIRO], [],  [libcairo is available for visual debugging output])  
   fi
   if test x$HAVE_CAIRO != xyes; then
     AC_MSG_WARN([libcairo not found -- not compiling telemetry output support ])
   fi
   AC_SUBST(CAIRO_CFLAGS)
   AC_SUBST(CAIRO_LIBS)
fi

dnl --------------------------------------------------
dnl Overall build configuration options
dnl --------------------------------------------------

dnl Configuration option for building of floating point code.

ac_enable_float=yes
AC_ARG_ENABLE(float,
     [  --disable-float         disable use of floating point code ],
     [ ac_enable_float=$enableval ], [ ac_enable_float=yes] )

if test "x${ac_enable_float}" != xyes ; then
    AC_DEFINE([THEORA_DISABLE_FLOAT], [], 
  [Define to exclude floating point code from the build])
fi
AM_CONDITIONAL(THEORA_DISABLE_FLOAT, [test "x${ac_enable_float}" != xyes])

dnl Configuration option for building of encoding support.

ac_enable_encode=yes
AC_ARG_ENABLE(encode,
     [  --disable-encode        disable encoding support ],
     [ ac_enable_encode=$enableval ], [ ac_enable_encode=yes] )

if test "x${ac_enable_encode}" != xyes ; then
    AC_DEFINE([THEORA_DISABLE_ENCODE], [],
  [Define to exclude encode support from the build])
else
    if test x$HAVE_VORBIS = xyes; then
      BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES encoder_example\$(EXEEXT)"
    else
      AC_MSG_NOTICE([Vorbis missing, cannot build example encoder])
    fi
fi
AM_CONDITIONAL(THEORA_DISABLE_ENCODE, [test "x${ac_enable_encode}" != xyes])

dnl Configuration option for examples

ac_enable_examples=yes
AC_ARG_ENABLE(examples,
     [  --disable-examples      disable examples ],
     [ ac_enable_examples=$enableval ], [ ac_enable_examples=yes] )
AM_CONDITIONAL(THEORA_ENABLE_EXAMPLES, [test "x${ac_enable_examples}" != xno])

dnl --------------------------------------------------
dnl Check for headers
dnl --------------------------------------------------

dnl none here

dnl --------------------------------------------------
dnl Check for typedefs, structures, etc
dnl --------------------------------------------------

dnl none

dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

dnl OpenBSD needs -lcompat for ftime() used by dump_video.c
AC_SEARCH_LIBS([ftime], [compat])

dnl substitute the included getopt if the system doesn't support long options
AC_CHECK_FUNC(getopt_long,
              [GETOPT_OBJS=''],
              [GETOPT_OBJS='getopt.$(OBJEXT) getopt1.$(OBJEXT)'])
AC_SUBST(GETOPT_OBJS)

if test x$HAVE_SDL = xyes -a x$HAVE_OSS = xyes -a x$HAVE_VORBIS = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES player_example\$(EXEEXT)"
fi
if test x$HAVE_PNG = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES png2theora\$(EXEEXT)"
fi
AC_SUBST(BUILDABLE_EXAMPLES)

dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

AC_SUBST(DEBUG)
AC_SUBST(PROFILE)

AC_OUTPUT([
  Makefile 
  lib/Makefile
  include/Makefile include/theora/Makefile
  examples/Makefile
  doc/Makefile doc/Doxyfile doc/spec/Makefile
  tests/Makefile
  m4/Makefile
  libtheora.spec
  theora.pc
  theora-uninstalled.pc
  theoradec.pc
  theoradec-uninstalled.pc
  theoraenc.pc
  theoraenc-uninstalled.pc
])

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${datadir}/doc)

if test $HAVE_DOXYGEN = "false"; then
  doc_build="no"
else
  doc_build="yes"
fi
if test $BUILD_SPEC = "false"; then
  spec_build="no"
else
  spec_build="yes"
fi

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    Encoding support: ........... ${ac_enable_encode}
    Floating point support: ..... ${ac_enable_float}
    Assembly optimization: ...... ${cpu_optimization}
    Debugging telemetry: ........ ${ac_enable_telemetry}
    Build example code: ......... ${ac_enable_examples}
    API Documentation: .......... ${doc_build}
    Format Documentation: ....... ${spec_build}

  Installation paths:

    libtheora: ................... ${LIBDIR}
    C header files: .............. ${INCLUDEDIR}/theora
    Documentation: ............... ${DOCDIR}/$PACKAGE

  Building:

    Type 'make' to compile $PACKAGE.

    Type 'make install' to install $PACKAGE.

    ${TESTS_INFO}

  Example programs will be built but not installed.
------------------------------------------------------------------------
])

