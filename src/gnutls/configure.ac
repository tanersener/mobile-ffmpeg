dnl Process this file with autoconf to produce a configure script.
# Copyright (C) 2000-2012, 2016 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos, Simon Josefsson
#
# This file is part of GnuTLS.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
# USA

AC_PREREQ(2.63)

dnl when updating version also update LT_REVISION in m4/hooks.m4
AC_INIT([GnuTLS], [3.6.8], [bugs@gnutls.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4 src/gl/m4 src/libopts/m4 lib/unistring/m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.12.2 foreign subdir-objects no-dist-gzip dist-xz -Wall -Wno-override])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])

AC_MSG_RESULT([***
*** Checking for compilation programs...
])

dnl Checks for programs.
PKG_PROG_PKG_CONFIG
AC_PROG_CC
gl_EARLY
ggl_EARLY
unistring_EARLY
AM_PROG_AS
AM_PROG_AR
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_YACC
AC_PROG_SED

AC_USE_SYSTEM_EXTENSIONS

#
# Require C99 support
#    
AC_PROG_CC_C99
if test "$ac_cv_prog_cc_c99" = "no"; then
  AC_MSG_WARN([[Compiler does not support C99. It may not be able to compile the project.]])
fi

AX_CODE_COVERAGE

AM_MAINTAINER_MODE([enable])

AC_ARG_ENABLE(bash-tests,
  AS_HELP_STRING([--disable-bash-tests], [skip some tests that badly need bash]),
    enable_bash_tests=$enableval, enable_bash_tests=yes)
AM_CONDITIONAL(DISABLE_BASH_TESTS, test "$enable_bash_tests" != "yes")

AC_ARG_ENABLE(doc,
  AS_HELP_STRING([--disable-doc], [don't generate any documentation]),
    enable_doc=$enableval, enable_doc=yes)
AM_CONDITIONAL(ENABLE_DOC, test "$enable_doc" != "no")

AC_ARG_ENABLE(manpages,
  AS_HELP_STRING([--enable-manpages], [install manpages even if disable-doc is given]),
    enable_manpages=$enableval,enable_manpages=auto)

if test "${enable_manpages}" = "auto";then
  enable_manpages="${enable_doc}"
fi

AM_CONDITIONAL(ENABLE_MANPAGES, test "$enable_manpages" != "no")

AC_ARG_ENABLE(tools,
  AS_HELP_STRING([--disable-tools], [don't compile any tools]),
    enable_tools=$enableval, enable_tools=yes)
AM_CONDITIONAL(ENABLE_TOOLS, test "$enable_tools" != "no")

# For includes/gnutls/gnutls.h.in.
AC_SUBST(MAJOR_VERSION, `echo $PACKAGE_VERSION | sed 's/\(.*\)\..*\..*/\1/g'`)
AC_SUBST(MINOR_VERSION, `echo $PACKAGE_VERSION | sed 's/.*\.\(.*\)\..*/\1/g'`)
AC_SUBST(PATCH_VERSION, [[`echo $PACKAGE_VERSION | sed 's/.*\..*\.\([0-9]*\).*/\1/g'`]])
AC_SUBST(NUMBER_VERSION, `printf "0x%02x%02x%02x" $MAJOR_VERSION $MINOR_VERSION $PATCH_VERSION`)

dnl C and C++ capabilities
AC_C_INLINE
AC_HEADER_STDC

# For the C++ code
AC_ARG_ENABLE(cxx,
  AS_HELP_STRING([--disable-cxx], [unconditionally disable the C++ library]),
    use_cxx=$enableval, use_cxx=yes)
if test "$use_cxx" != "no"; then
  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])], use_cxx=yes, use_cxx=no)
  AC_LANG_POP(C++)
fi
AM_CONDITIONAL(ENABLE_CXX, test "$use_cxx" != "no")

dnl Detect windows build
use_accel=yes
case "$host" in
  *android*)
    have_android=yes
    have_elf=yes
  ;;
  *mingw32* | *mingw64*)
    have_win=yes
    AC_DEFINE([_UNICODE], [1], [Defined to 1 for Unicode (wide chars) APIs])
  ;;
  *darwin*)
    have_macosx=yes
    save_LDFLAGS="$LDFLAGS"
    dnl Try to use -no_weak_imports if available. This makes sure we
    dnl error out when linking to a function that doesn't exist in the
    dnl intended minimum runtime version.
    LDFLAGS="$LDFLAGS -Wl,-no_weak_imports"
    AC_MSG_CHECKING([whether the linker supports -Wl,-no_weak_imports])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])],
      [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no); LDFLAGS="$save_LDFLAGS"])
  ;;
  *solaris*)
    have_elf=yes
    use_accel=no
    AC_MSG_WARN([[
***
*** In solaris hardware acceleration is disabled by default due to issues
*** with the assembler. Use --enable-hardware-acceleration to enable it.
*** ]])
  ;;
  *)
    have_elf=yes
  ;;
esac

AM_CONDITIONAL(ANDROID, test "$have_android" = yes)
AM_CONDITIONAL(WINDOWS, test "$have_win" = yes)
AM_CONDITIONAL(MACOSX, test "$have_macosx" = yes)
AM_CONDITIONAL(ELF, test "$have_elf" = yes)

dnl Hardware Acceleration
AC_ARG_ENABLE(hardware-acceleration,
  AS_HELP_STRING([--disable-hardware-acceleration], [unconditionally disable hardware acceleration]),
    use_accel=$enableval)
hw_accel=none


use_padlock=no
if test "$use_accel" != "no"; then
case $host_cpu in
  armv8 | aarch64)
      hw_accel="aarch64"
      case $host_os in
        *_ilp32)
	  dnl ILP32 not supported in assembler yet
	  hw_accel="none"
        ;;
      esac
  ;;
  i?86 | x86_64 | amd64)
      AC_CHECK_HEADERS(cpuid.h)
      if test "$host_cpu" = "x86_64" || test "$host_cpu" = "amd64"; then
        hw_accel="x86-64"
      else
        hw_accel="x86"
      fi
      use_padlock=yes
  ;;
  *)
  ;;
esac

fi

AC_ARG_ENABLE(tls13-interop,
  AS_HELP_STRING([--disable-tls13-interop], [disable TLS1.3 interoperability testing with openssl]),
    enable_tls13_interop=$enableval, enable_tls13_interop=yes)

AM_CONDITIONAL(ENABLE_TLS13_INTEROP, test "$enable_tls13_interop" != "no")

dnl Check for iovec type
AC_CHECK_MEMBERS([struct iovec.iov_base],
      [
        AC_SUBST([DEFINE_IOVEC_T], ["#include <sys/uio.h>
typedef struct iovec giovec_t;"])
      ],
      [
        AC_SUBST([DEFINE_IOVEC_T], ["typedef struct {
    void *iov_base;
    size_t iov_len;
} giovec_t;"])
      ],
      [#include <sys/uio.h>
      ])
AM_SUBST_NOTMAKE([DEFINE_IOVEC_T])

dnl Need netinet/tcp.h for TCP_FASTOPEN
AC_CHECK_HEADERS([netinet/tcp.h])
AC_CHECK_HEADERS([stdatomic.h])

dnl This ensures that we link with the right library for atomic operations on Linux SPARC
save_LIBS=$LIBS
AC_SEARCH_LIBS([__atomic_load_4], [atomic], [], [AC_MSG_NOTICE([Could not detect libatomic])])
LIBS=$save_LIBS

AS_IF([test "$ac_cv_search___atomic_load_4" = "none required" || test "$ac_cv_search___atomic_load_4" = "no"],
      [AC_SUBST([LIBATOMIC_LIBS], [])],
      [AC_SUBST([LIBATOMIC_LIBS], [$ac_cv_search___atomic_load_4])])

dnl We use its presence to detect C11 threads
AC_CHECK_HEADERS([threads.h])

AC_ARG_ENABLE(padlock,
  AS_HELP_STRING([--disable-padlock], [unconditionally disable padlock acceleration]),
    use_padlock=$enableval)

if test "$use_padlock" != "no"; then
    AC_DEFINE([ENABLE_PADLOCK], 1, [Enable padlock acceleration])
    AC_SUBST([ENABLE_PADLOCK])
fi
AM_CONDITIONAL(ENABLE_PADLOCK, test "$use_padlock" = "yes")
AM_CONDITIONAL(ASM_AARCH64, test x"$hw_accel" = x"aarch64")
AM_CONDITIONAL(ASM_X86_64, test x"$hw_accel" = x"x86-64")
AM_CONDITIONAL(ASM_X86_32, test x"$hw_accel" = x"x86")
AM_CONDITIONAL(ASM_X86, test x"$hw_accel" = x"x86" || test x"$hw_accel" = x"x86-64")
AM_CONDITIONAL(HAVE_GCC_GNU89_INLINE_OPTION, test "$gnu89_inline" = "yes"])
AM_CONDITIONAL(HAVE_GCC, test "$GCC" = "yes")

dnl check for getrandom()
rnd_variant="auto-detect"
AC_MSG_CHECKING([for getrandom])
AC_LINK_IFELSE([AC_LANG_PROGRAM([
	   #include <sys/random.h>],[
		   getrandom(0, 0, 0);
		  ])],
		  [AC_MSG_RESULT(yes)
		   AC_DEFINE([HAVE_GETRANDOM], 1, [Enable the Linux getrandom function])
		   rnd_variant=getrandom],
		  [AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for getentropy])
AC_LINK_IFELSE([AC_LANG_PROGRAM([
	   #include <unistd.h>
           #ifdef __APPLE__
           #include <sys/random.h>
           #endif
           #ifdef __linux__
           #error 1
           #endif
	   ],[
		   getentropy(0, 0);
		  ])],
		  [AC_MSG_RESULT(yes)
		   AC_DEFINE([HAVE_GETENTROPY], 1, [Enable the OpenBSD getentropy function])
		   rnd_variant=getentropy],
		  [AC_MSG_RESULT(no)])

AM_CONDITIONAL(HAVE_GETENTROPY, test "$rnd_variant" = "getentropy")

dnl Try the hooks.m4
LIBGNUTLS_HOOKS
LIBGNUTLS_EXTRA_HOOKS

AC_ARG_ENABLE(tests,
  AS_HELP_STRING([--disable-tests], [don't compile or run any tests]),
    enable_tests=$enableval, enable_tests=$enable_tools)
AM_CONDITIONAL(ENABLE_TESTS, test "$enable_tests" != "no")

AC_ARG_ENABLE(fuzzer-target,
  AS_HELP_STRING([--enable-fuzzer-target], [make a library intended for testing - not production]),
    enable_fuzzer_target=$enableval, enable_fuzzer_target=no)
if test "$enable_fuzzer_target" != "no";then
	AC_DEFINE([FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION], 1, [Enable fuzzer target -not for production])
fi

dnl
dnl check for gtk-doc
dnl
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])
# needed for some older versions of gtk-doc
m4_ifdef([GTK_DOC_USE_LIBTOOL], [], [
AM_CONDITIONAL([GTK_DOC_USE_LIBTOOL], false)
])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])

AC_C_BIGENDIAN

dnl No fork on MinGW, disable some self-tests until we fix them.
dnl Check clock_gettime and pthread_mutex_lock in libc (avoid linking to other libs)
AC_CHECK_FUNCS([fork setitimer getrusage getpwuid_r nanosleep daemon getpid localtime mmap explicit_bzero],,)
dnl Manually check some functions by including headers first. On macOS, you
dnl normally only have the latest SDK available, containing all existing
dnl functions, but having them restricted according to target version in
dnl headers. If we bypass the headers and just try linking (as AC_CHECK_FUNCS
dnl does), we will accidentally detect functions which we shouldn't use. Set
dnl ac_cv_func_* as well, to avoid later AC_CHECK_FUNCS from other included
dnl scripts from overriding it.
AC_MSG_CHECKING([for clock_gettime])
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <time.h>], [clock_gettime(0, 0);])],
  [AC_MSG_RESULT(yes); ac_cv_func_clock_gettime=yes
  AC_DEFINE([HAVE_CLOCK_GETTIME], 1, [Define to 1 if you have the `clock_gettime' function.])],
  [AC_MSG_RESULT(no); ac_cv_func_clock_gettime=no])
AC_MSG_CHECKING([for fmemopen])
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [fmemopen(0, 0, 0);])],
  [AC_MSG_RESULT(yes); ac_cv_func_fmemopen=yes
  AC_DEFINE([HAVE_FMEMOPEN], 1, [Define to 1 if you have the `fmemopen' function.])],
  [AC_MSG_RESULT(no); ac_cv_func_fmemopen=no])

AM_CONDITIONAL(HAVE_FORK, test "$ac_cv_func_fork" != "no")

AC_CHECK_FUNCS([__register_atfork secure_getenv getauxval],,)

AC_ARG_ENABLE(seccomp-tests,
  AS_HELP_STRING([--enable-seccomp-tests], [unconditionally enable tests with seccomp]),
    seccomp_tests=$enableval, seccomp_tests=no)

AM_CONDITIONAL(HAVE_SECCOMP_TESTS, test "$seccomp_tests" = "yes")

# check for libseccomp - used in test programs
AC_LIB_HAVE_LINKFLAGS(seccomp,, [#include <seccomp.h>  
], [seccomp_init(0);])

# check for libcrypto - used in test programs
AC_LIB_HAVE_LINKFLAGS(crypto,, [#include <openssl/evp.h>  
], [EVP_CIPHER_CTX_init(NULL);])

AM_CONDITIONAL(HAVE_LIBCRYPTO, test "$HAVE_LIBCRYPTO" = "yes")

AC_LIB_HAVE_LINKFLAGS(rt,, [#include <time.h>
#include <signal.h>
], [timer_create (0,0,0);])

if test "$have_win" != "yes";then
  AC_CHECK_FUNCS([pthread_mutex_lock],,)
  if test "$ac_cv_func_pthread_mutex_lock" != "yes";then
    AC_LIB_HAVE_LINKFLAGS(pthread,, [#include <pthread.h>], [pthread_mutex_lock (0);])
  fi
fi

if test "$ac_cv_func_nanosleep" != "yes";then
  AC_LIB_HAVE_LINKFLAGS(rt,, [#include <time.h>], [nanosleep (0, 0);])
  gnutls_needs_librt=yes
fi

if test "$ac_cv_func_clock_gettime" != "yes";then
  AC_LIB_HAVE_LINKFLAGS(rt,, [#include <time.h>], [clock_gettime (0, 0);])
  gnutls_needs_librt=yes
fi

AC_ARG_WITH(included-unistring, AS_HELP_STRING([--with-included-unistring],
                                 [disable linking with system libunistring]),
            included_unistring="$withval", 
            included_unistring=no)

if test "$included_unistring" = yes;then
  ac_have_unistring=no
else
  save_LIBS=$LIBS
  AC_SEARCH_LIBS(u8_normalize, unistring, [
    included_unistring=no
    ac_have_unistring=yes
    AC_SUBST([LIBUNISTRING], [$ac_cv_search_u8_normalize])
  ], [
    ac_cv_libunistring=no
    AC_MSG_ERROR([[
  ***
  *** Libunistring was not found. To use the included one, use --with-included-unistring
    ]])
  ])
  LIBS=$save_LIBS
fi

AM_CONDITIONAL(HAVE_LIBUNISTRING, test "$ac_have_unistring" = "yes")

dnl Note that g*l_INIT are run after we check for library capabilities,
dnl to prevent issues from caching lib dependencies. See discussion
dnl in https://bugs.gentoo.org/show_bug.cgi?id=494940 and
dnl https://gnu-autoconf.7623.n7.nabble.com/Correct-way-to-check-for-clock-gettime-td12276.html
gl_INIT
ggl_INIT
unistring_INIT

# disable the extended test suite at tests/suite if asked, or if we are not running in git master
AC_ARG_ENABLE(full-test-suite,
  AS_HELP_STRING([--disable-full-test-suite], [disable running very slow components of test suite]),
    full_test_suite=$enableval, full_test_suite=yes)

# test if we are in git master or in release build. In release
# builds we do not use valgrind.
SUITE_FILE="${srcdir}/tests/suite/mini-eagain2.c"
if test "$full_test_suite" = yes && test ! -f "$SUITE_FILE";then
	full_test_suite=no
fi

AM_CONDITIONAL(WANT_TEST_SUITE, test "$full_test_suite" = "yes")

dnl GCC warnings to enable

AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--disable-gcc-warnings],
		  [turn off lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=yes]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Wtype-limits], [WSTACK_CFLAGS])

  nw="$nw -Wsystem-headers"         # Don't let system headers trigger warnings
  nw="$nw -Wc++-compat"             # We don't care about C++ compilers
  nw="$nw -Wundef"                  # Warns on '#if GNULIB_FOO' etc in gnulib
  nw="$nw -Wtraditional"            # Warns on #elif which we use often
  nw="$nw -Wpadded"                 # Our structs are not padded
  nw="$nw -Wtraditional-conversion" # Too many warnings for now
  nw="$nw -Wswitch-default"         # Too many warnings for now
  nw="$nw -Wformat-y2k"             # Too many warnings for now
  nw="$nw -Woverlength-strings"     # We use some in tests/
  nw="$nw -Wvla"                    # There is no point to avoid C99 variable length arrays
  nw="$nw -Wformat-nonliteral"      # Incompatible with gettext _()
  nw="$nw -Wformat-signedness"      # Too many to handle
  nw="$nw -Wstrict-overflow"
  nw="$nw -Wmissing-noreturn"
  nw="$nw -Winline"                 # Too compiler dependent
  nw="$nw -Wsuggest-attribute=pure" # Is it worth using attributes?
  nw="$nw -Wsuggest-attribute=const" # Is it worth using attributes?
  nw="$nw -Wsuggest-attribute=noreturn" # Is it worth using attributes?
  nw="$nw -Wstack-protector"        # Some functions cannot be protected
  nw="$nw -Wunsafe-loop-optimizations" # Warnings with no point
  nw="$nw -Wredundant-decls"        # Some files cannot be compiled with that (gl_fd_to_handle)

  gl_MANYWARN_ALL_GCC([ws])
  gl_MANYWARN_COMPLEMENT(ws, [$ws], [$nw])
  for w in $ws; do
    gl_WARN_ADD([$w])
  done
  gl_WARN_ADD([-Wno-missing-field-initializers]) # We need this one
  gl_WARN_ADD([-Wno-unused-parameter]) # Too many warnings for now
  gl_WARN_ADD([-Wno-format-truncation]) # Many warnings with no point
  gl_WARN_ADD([-Wimplicit-fallthrough=2])
  gl_WARN_ADD([-Wabi=11])
  gl_WARN_ADD([-fdiagnostics-show-option])
fi

AC_SUBST([WERROR_CFLAGS])
AC_SUBST([WSTACK_CFLAGS])
AC_SUBST([WARN_CFLAGS])

dnl Programs for compilation or development
AC_PROG_LN_S
LT_INIT([disable-static,win32-dll,shared])



AC_LIB_HAVE_LINKFLAGS(dl,, [#include <dlfcn.h>], [dladdr (0, 0);])

AC_ARG_ENABLE(fips140-mode,
  AS_HELP_STRING([--enable-fips140-mode], [enable FIPS140-2 mode]),
    enable_fips=$enableval, enable_fips=no)
AM_CONDITIONAL(ENABLE_FIPS140, test "$enable_fips" = "yes")
if [ test "$enable_fips" = "yes" ];then
  if test "x$HAVE_LIBDL" = "xyes";then
    AC_DEFINE([ENABLE_FIPS140], 1, [Enable FIPS140-2 mode])
    AC_SUBST([FIPS140_LIBS], $LIBDL)
    AC_ARG_WITH(fips140-key, AS_HELP_STRING([--with-fips140-key],
                                 [specify the FIPS140 HMAC key for integrity]),
            fips_key="$withval", 
            fips_key="orboDeJITITejsirpADONivirpUkvarP")

    AC_DEFINE_UNQUOTED([FIPS_KEY], ["$fips_key"], [The FIPS140-2 integrity key])
  else
    enable_fips=no
    AC_MSG_WARN([[
*** 
*** This system is not supported in FIPS140 mode.
*** libdl and dladdr() are required.
*** ]])
  fi
fi

PKG_CHECK_MODULES(CMOCKA, [cmocka >= 1.0.1], [with_cmocka=yes], [with_cmocka=no])
AM_CONDITIONAL(HAVE_CMOCKA, test "$with_cmocka" != "no")

AC_ARG_WITH(idn, AS_HELP_STRING([--without-idn],
                                [disable support for IDNA]),
            try_libidn2="$withval", 
            try_libidn2=yes)

idna_support=no
with_libidn2=no

if test "$try_libidn2" = yes;then
    save_LIBS=$LIBS
    AC_SEARCH_LIBS(idn2_lookup_u8, idn2, [
      with_libidn2=yes;
      idna_support="IDNA 2008 (libidn2)"
      AC_DEFINE([HAVE_LIBIDN2], 1, [Define if IDNA 2008 support is enabled.])
      AC_SUBST([LIBIDN2_CFLAGS], [])
      AC_SUBST([LIBIDN2_LIBS], [-lidn2]) dnl used in gnutls.pc.in
dnl enable once libidn2.pc is widespread; and remove LIBIDN2_LIBS from gnutls.pc.in (Libs.private)
dnl      if test "x$GNUTLS_REQUIRES_PRIVATE" = "x"; then
dnl        GNUTLS_REQUIRES_PRIVATE="Requires.private: libidn2"
dnl      else
dnl        GNUTLS_REQUIRES_PRIVATE="${GNUTLS_REQUIRES_PRIVATE}, libidn2"
dnl      fi
    ],[
      with_libidn2=no;
      AC_MSG_WARN(*** LIBIDN2 was not found. You will not be able to use IDN2008 support)
    ])
    LIBS=$save_LIBS
else
  with_libidn2=no
fi

AM_CONDITIONAL(HAVE_LIBIDN2, test "$with_libidn2" != "no")

AC_ARG_ENABLE(non-suiteb-curves,
  AS_HELP_STRING([--disable-non-suiteb-curves], [disable curves not in SuiteB]),
    enable_non_suiteb=$enableval, enable_non_suiteb=yes)

if test "$enable_non_suiteb" = "yes";then
	dnl nettle_secp_192r1 is not really a function
	AC_CHECK_LIB(hogweed, nettle_secp_192r1, enable_non_suiteb=yes, enable_non_suiteb=no, [$HOGWEED_LIBS $NETTLE_LIBS])

	if test "$enable_non_suiteb" = "yes";then
		AC_DEFINE([ENABLE_NON_SUITEB_CURVES], 1, [Enable all curves])
	fi
fi
AM_CONDITIONAL(ENABLE_NON_SUITEB_CURVES, test "$enable_non_suiteb" = "yes")

# We MUST require a Nettle version that has rsa_sec_decrypt now.
save_LIBS=$LIBS
LIBS="$LIBS $HOGWEED_LIBS $NETTLE_LIBS"
AC_CHECK_FUNCS(nettle_rsa_sec_decrypt,
    [],
    [AC_MSG_ERROR([Nettle lacks the required rsa_sec_decrypt function])]
)
LIBS=$save_LIBS

# Check if nettle has CFB8 support
save_LIBS=$LIBS
LIBS="$LIBS $NETTLE_LIBS"
AC_CHECK_FUNCS(nettle_cfb8_encrypt)
LIBS=$save_LIBS

# Check if nettle has CMAC support
save_LIBS=$LIBS
LIBS="$LIBS $NETTLE_LIBS"
AC_CHECK_FUNCS(nettle_cmac128_update)
LIBS=$save_LIBS

# Check if nettle has XTS support
save_LIBS=$LIBS
LIBS="$LIBS $NETTLE_LIBS"
AC_CHECK_FUNCS(xts_encrypt_message)
LIBS=$save_LIBS


AC_MSG_CHECKING([whether to build libdane])
AC_ARG_ENABLE(libdane,
    AS_HELP_STRING([--disable-libdane],
                   [disable the built of libdane]),
    enable_dane=$enableval, enable_dane=yes)
AC_MSG_RESULT($enable_dane)

if test "$enable_dane" != "no"; then
    LIBS="$oldlibs -lunbound"
    AC_MSG_CHECKING([for unbound library])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
		   #include <unbound.h>],[
		   struct ub_ctx* ctx;
		   ctx = ub_ctx_create();])],
		  [AC_MSG_RESULT(yes)
		   AC_SUBST([UNBOUND_LIBS], [-lunbound])
		   AC_SUBST([UNBOUND_CFLAGS], [])
		   AC_DEFINE([HAVE_DANE], 1, [Enable the DANE library])
		   enable_dane=yes],
		  [AC_MSG_RESULT(no)
		   AC_MSG_WARN([[
*** 
*** libunbound was not found. Libdane will not be built.
*** ]])
		  enable_dane=no])
    LIBS="$oldlibs"
fi

AM_CONDITIONAL(ENABLE_DANE, test "$enable_dane" = "yes")

AC_ARG_WITH(unbound-root-key-file, AS_HELP_STRING([--with-unbound-root-key-file],
                                 [specify the unbound root key file]),
            unbound_root_key_file="$withval", 
if test "$have_win" = yes; then
    unbound_root_key_file="C:\\Program Files\\Unbound\\root.key"
else
    if test -f /var/lib/unbound/root.key;then
	unbound_root_key_file="/var/lib/unbound/root.key"
    else
        if test -f /usr/share/dns/root.key;then
	    unbound_root_key_file="/usr/share/dns/root.key"
        else
            unbound_root_key_file="/etc/unbound/root.key"
        fi
    fi
fi
)

AC_DEFINE_UNQUOTED([UNBOUND_ROOT_KEY_FILE],
  ["$unbound_root_key_file"], [The DNSSEC root key file])

AC_ARG_WITH(system-priority-file, AS_HELP_STRING([--with-system-priority-file],
                                 [specify the system priority file]),
            system_priority_file="$withval", 
system_priority_file="/etc/gnutls/default-priorities"
)

AC_DEFINE_UNQUOTED([SYSTEM_PRIORITY_FILE],
  ["$system_priority_file"], [The system priority file])

AC_ARG_WITH(default-priority-string, AS_HELP_STRING([--with-default-priority-string],
                                 [specify the default priority string (e.g. @SYSTEM)]),
            prio_string="$withval", 
            prio_string="NORMAL")

AC_DEFINE_UNQUOTED([DEFAULT_PRIORITY_STRING], ["$prio_string"], [The default priority string])

dnl Check for p11-kit
P11_KIT_MINIMUM=0.23.1
AC_ARG_WITH(p11-kit,
	AS_HELP_STRING([--without-p11-kit],
		[Build without p11-kit and PKCS#11 support]))
if test "$with_p11_kit" != "no"; then
	PKG_CHECK_MODULES(P11_KIT, [p11-kit-1 >= $P11_KIT_MINIMUM], [with_p11_kit=yes], [with_p11_kit=no])
	if test "$with_p11_kit" != "no";then
		AC_DEFINE([ENABLE_PKCS11], 1, [Build PKCS#11 support])
		if test "x$GNUTLS_REQUIRES_PRIVATE" = "x"; then
			GNUTLS_REQUIRES_PRIVATE="Requires.private: p11-kit-1"
		else
			GNUTLS_REQUIRES_PRIVATE="${GNUTLS_REQUIRES_PRIVATE}, p11-kit-1"
		fi
	else
	        with_p11_kit=no
	        AC_MSG_ERROR([[
*** 
*** p11-kit >= $P11_KIT_MINIMUM was not found. To disable PKCS #11 support 
*** use --without-p11-kit, otherwise you may get p11-kit from
*** https://p11-glue.freedesktop.org/p11-kit.html
*** ]])
	fi
fi

AM_CONDITIONAL(P11KIT_0_23_11_API, $PKG_CONFIG --atleast-version=0.23.11 p11-kit-1)

AM_CONDITIONAL(ENABLE_PKCS11, test "$with_p11_kit" != "no")

AC_ARG_WITH(tpm,
	AS_HELP_STRING([--without-tpm],
		[Disable TPM (trousers) support.]),
		[with_tpm=$withval], [with_tpm=yes])
if test "$with_tpm" != "no"; then
    LIBS="$oldlibs -ltspi"
    AC_MSG_CHECKING([for tss library])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
		   #include <trousers/tss.h>
		   #include <trousers/trousers.h>],[
		   int err = Tspi_Context_Create((void *)0);
		   Trspi_Error_String(err);])],
		  [AC_MSG_RESULT(yes)
		   AC_SUBST([TSS_LIBS], [-ltspi])
		   AC_SUBST([TSS_CFLAGS], [])
		   AC_DEFINE([HAVE_TROUSERS], 1, [Enable TPM])
		   with_tpm=yes],
		  [AC_MSG_RESULT(no)
		   AC_MSG_WARN([[
*** 
*** trousers was not found. TPM support will be disabled.
*** ]])
		  with_tpm=no])
    LIBS="$oldlibs"
fi

AM_CONDITIONAL(ENABLE_TROUSERS, test "$with_tpm" != "no")

for l in /usr/lib64 /usr/lib /lib64 /lib /usr/lib/x86_64-linux-gnu/; do
    if test -f "${l}/libtspi.so.1";then
        default_trousers_lib="${l}/libtspi.so.1"
        break
    fi
done

AC_ARG_WITH(trousers-lib, AS_HELP_STRING([--with-trousers-lib=LIB],
                                 [set the location of the trousers library]),
            ac_trousers_lib=$withval, ac_trousers_lib=$default_trousers_lib)

if test "$with_tpm" != "no" && test -z "$ac_trousers_lib"; then
    AC_MSG_ERROR([[
  ***
  *** unable to find trousers library, please specify with --with-trousers-lib=<lib file>
  ***
  ]])
fi

AC_DEFINE_UNQUOTED([TROUSERS_LIB], ["$ac_trousers_lib"], [the location of the trousers library])
AC_SUBST(TROUSERS_LIB)

AM_MISSING_PROG([AUTOGEN], [autogen])

included_libopts=no
if test "$enable_tools" != "no" || test "$enable_doc" != "no"; then
	AC_CHECK_PROGS([autogen], [autogen])

	if test -z "$autogen"; then
		AC_MSG_WARN([[
***
*** autogen not found. Will not link against system libopts.
*** ]])
		dnl simulate specifying option on the command line
		enable_local_libopts=yes
	fi
	LIBOPTS_CHECK([src/libopts])
	if test "$NEED_LIBOPTS_DIR" = "true";then
		dnl replace libopts-generated files with distributed backups, if present
		included_libopts=yes
	fi
else
	# Need to ensure the relevant conditionals get set
	gl_STDNORETURN_H
	AM_CONDITIONAL([INSTALL_LIBOPTS],[false])
fi

AM_CONDITIONAL(NEED_LIBOPTS, test "$included_libopts" = "yes")

# For minitasn1.
AC_CHECK_SIZEOF(unsigned long int, 4)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(time_t, 4)

# export for use in scripts
AC_SUBST(ac_cv_sizeof_time_t)

AC_SUBST(GNUTLS_REQUIRES_PRIVATE)


AC_ARG_WITH([default-trust-store-pkcs11],
  [AS_HELP_STRING([--with-default-trust-store-pkcs11=URI],
    [use the given pkcs11 uri as default trust store])])

if test "x$with_default_trust_store_pkcs11" != x; then
  if test "x$with_p11_kit" = xno; then
    AC_MSG_ERROR([cannot use pkcs11 store without p11-kit])
  fi
  AC_DEFINE_UNQUOTED([DEFAULT_TRUST_STORE_PKCS11],
    ["$with_default_trust_store_pkcs11"], [use the given pkcs11 uri as default trust store])
fi

AM_CONDITIONAL([HAVE_PKCS11_TRUST_STORE], [test -n "${with_default_trust_store_pkcs11}"])

AC_ARG_WITH([default-trust-store-dir],
  [AS_HELP_STRING([--with-default-trust-store-dir=DIR],
    [use the given directory as default trust store])])

if test "x$with_default_trust_store_dir" != x; then
  AC_DEFINE_UNQUOTED([DEFAULT_TRUST_STORE_DIR],
    ["$with_default_trust_store_dir"], [use the given directory as default trust store])
fi

dnl auto detect https://lists.gnu.org/archive/html/help-gnutls/2012-05/msg00004.html
AC_ARG_WITH([default-trust-store-file],
  [AS_HELP_STRING([--with-default-trust-store-file=FILE],
    [use the given file default trust store])], with_default_trust_store_file="$withval",
  [if test "$build" = "$host" && test x$with_default_trust_store_pkcs11 = x && test x$with_default_trust_store_dir = x && test x$have_macosx = x;then
  for i in \
    /etc/ssl/ca-bundle.pem \
    /etc/ssl/certs/ca-certificates.crt \
    /etc/pki/tls/cert.pem \
    /usr/local/share/certs/ca-root-nss.crt \
    /etc/ssl/cert.pem
    do
    if test -e "$i"; then
      with_default_trust_store_file="$i"
      break
    fi
  done
  fi]
)

if test "$with_default_trust_store_file" = "no";then
  with_default_trust_store_file=""
fi

AC_ARG_WITH([default-crl-file],
  [AS_HELP_STRING([--with-default-crl-file=FILE],
    [use the given CRL file as default])])

AC_ARG_WITH([default-blacklist-file],
  [AS_HELP_STRING([--with-default-blacklist-file=FILE],
    [use the given certificate blacklist file as default])])

if test "x$with_default_trust_store_file" != x; then
  AC_DEFINE_UNQUOTED([DEFAULT_TRUST_STORE_FILE],
    ["$with_default_trust_store_file"], [use the given file default trust store])
fi

if test "x$with_default_crl_file" != x; then
  AC_DEFINE_UNQUOTED([DEFAULT_CRL_FILE],
    ["$with_default_crl_file"], [use the given CRL file])
fi

if test "x$with_default_blacklist_file" != x; then
  AC_DEFINE_UNQUOTED([DEFAULT_BLACKLIST_FILE],
    ["$with_default_blacklist_file"], [use the given certificate blacklist file])
fi

dnl Guile bindings.
AC_MSG_CHECKING([whether building Guile bindings])
AC_ARG_ENABLE(guile,
	AS_HELP_STRING([--enable-guile], [build GNU Guile bindings]),
		[opt_guile_bindings=$enableval], [opt_guile_bindings=yes])
AC_MSG_RESULT($opt_guile_bindings)

AC_ARG_WITH([guile-site-dir], AS_HELP_STRING([--with-guile-site-dir=DIR],
    [guile site directory for gnutls, default is guile system settings]),
    [guilesitedir="${withval}"], [guilesitedir='$(GUILE_SITE)'])
AC_ARG_WITH([guile-site-ccache-dir], AS_HELP_STRING([--with-guile-site-ccache-dir=DIR],
    [guile ccache directory for gnutls, default is guile system settings]),
    [guilesiteccachedir="${withval}"], [guilesiteccachedir='$(GUILE_SITE_CCACHE)'])
AC_ARG_WITH([guile-extension-dir], AS_HELP_STRING([--with-guile-extension-dir=DIR],
    [guile extension directory for gnutls, default is guile system settings]),
    [guileextensiondir="${withval}"], [guileextensiondir='$(GUILE_EXTENSION)'])
AC_SUBST([guilesitedir])
AC_SUBST([guilesiteccachedir])
AC_SUBST([guileextensiondir])

if test "$opt_guile_bindings" = "yes"; then
   AC_MSG_RESULT([***
*** Detecting GNU Guile...
])

   AC_PATH_PROG([guile_snarf], [guile-snarf])
   if test "x$guile_snarf" = "x"; then
      AC_MSG_WARN([`guile-snarf' from Guile not found.  Guile bindings not built.])
      opt_guile_bindings=no
   else
      dnl Check for 'guild', which can be used to compile Scheme code
      dnl on Guile 2.x.
      AC_PATH_PROG([GUILD], [guild])
      AC_SUBST([GUILD])

      GUILE_PKG
      GUILE_PROGS
      GUILE_SITE_DIR
      GUILE_FLAGS

      # Backward compatibility with <guile-2.2 m4 macro that is used
      # due to autreconf of several CI machine.
      # We need to guess the locations of ccache and extension
      if test -z "${GUILE_SITE_CCACHE}"; then
          AC_MSG_NOTICE([Found <guile-2.2 m4, macro emulating])

          AC_MSG_CHECKING([for GUILE_SITE_CCACHE via pkg-config])
          GUILE_SITE_CCACHE=`$PKG_CONFIG --variable=siteccachedir guile-$GUILE_EFFECTIVE_VERSION`
          AC_MSG_RESULT([${GUILE_SITE_CCACHE}])
          if test -z "${GUILE_SITE_CCACHE}"; then
                  AC_MSG_CHECKING([for GUILE_SITE_CCACHE via guile])
                  GUILE_SITE_CCACHE=`$GUILE -c "(display (if (defined? '%site-ccache-dir) (%site-ccache-dir) \"\"))"`
                  AC_MSG_RESULT([${GUILE_SITE_CCACHE}])
          fi
          AC_SUBST([GUILE_SITE_CCACHE])

          AC_MSG_CHECKING([for GUILE_EXTENSION])
          GUILE_EXTENSION=`$PKG_CONFIG --print-errors --variable=extensiondir guile-$GUILE_EFFECTIVE_VERSION`
          AC_MSG_RESULT([${GUILE_EXTENSION}])
          AC_SUBST([GUILE_EXTENSION])
      fi

      save_CFLAGS="$CFLAGS"
      save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GUILE_CFLAGS"
      LIBS="$LIBS $GUILE_LDFLAGS"
      AC_MSG_CHECKING([whether GNU Guile is recent enough])
      AC_LINK_IFELSE([AC_LANG_PROGRAM([], [scm_from_locale_string ("")])],
        [], [opt_guile_bindings=no])
      CFLAGS="$save_CFLAGS"
      LIBS="$save_LIBS"

      if test "$opt_guile_bindings" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_MSG_CHECKING([whether gcc supports -fgnu89-inline])
        _gcc_cflags_save="$CFLAGS"
        CFLAGS="${CFLAGS} -fgnu89-inline"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
                          gnu89_inline=yes, gnu89_inline=no)
        AC_MSG_RESULT($gnu89_inline)
        CFLAGS="$_gcc_cflags_save"

	# Optional Guile functions.
	save_CFLAGS="$CFLAGS"
	save_LIBS="$LIBS"
	CFLAGS="$CFLAGS $GUILE_CFLAGS"
	LIBS="$LIBS $GUILE_LDFLAGS"
	AC_CHECK_FUNCS([scm_gc_malloc_pointerless])
	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
      else
        AC_MSG_RESULT([no])
        AC_MSG_WARN([A sufficiently recent GNU Guile not found.  Guile bindings not built.])
        opt_guile_bindings=no
      fi
   fi
fi

AM_CONDITIONAL([HAVE_GUILE], [test "$opt_guile_bindings" = "yes"])
AM_CONDITIONAL([HAVE_GUILD], [test "x$GUILD" != "x"])

LIBGNUTLS_LIBS="-L${libdir} -lgnutls $LIBS"
LIBGNUTLS_CFLAGS="-I${includedir}"
AC_SUBST(LIBGNUTLS_LIBS)
AC_SUBST(LIBGNUTLS_CFLAGS)

AM_CONDITIONAL(NEEDS_LIBRT, test "$gnutls_needs_librt" = "yes")

AC_DEFINE([GNUTLS_COMPAT_H], 1, [Make sure we don't use old features in code.])
AC_DEFINE([GNUTLS_INTERNAL_BUILD], 1, [We allow temporarily usage of deprecated functions - until they are removed.])

AC_DEFINE([fread_file], [_gnutls_fread_file], [static lib rename])
AC_DEFINE([read_file], [_gnutls_read_file], [static lib rename])
AC_DEFINE([read_binary_file], [_gnutls_read_binary_file], [static lib rename])

dnl Some variables needed in makefiles
YEAR=`date +%Y`
AC_SUBST([YEAR], $YEAR)

AC_CONFIG_FILES([guile/pre-inst-guile], [chmod +x guile/pre-inst-guile])
AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  doc/credentials/Makefile
  doc/credentials/srp/Makefile
  doc/credentials/x509/Makefile
  doc/doxygen/Doxyfile
  doc/examples/Makefile
  doc/latex/Makefile
  doc/manpages/Makefile
  doc/reference/Makefile
  doc/reference/version.xml
  doc/scripts/Makefile
  extra/Makefile
  extra/includes/Makefile
  libdane/Makefile
  libdane/includes/Makefile
  libdane/gnutls-dane.pc
  gl/Makefile
  gl/tests/Makefile
  guile/Makefile
  guile/src/Makefile
  lib/Makefile
  lib/accelerated/Makefile
  lib/accelerated/x86/Makefile
  lib/accelerated/aarch64/Makefile
  lib/algorithms/Makefile
  lib/auth/Makefile
  lib/ext/Makefile
  lib/extras/Makefile
  lib/gnutls.pc
  lib/includes/Makefile
  lib/includes/gnutls/gnutls.h
  lib/minitasn1/Makefile
  lib/nettle/Makefile
  lib/x509/Makefile
  lib/unistring/Makefile
  po/Makefile.in
  src/Makefile
  src/args-std.def
  src/gl/Makefile
  tests/Makefile
  tests/windows/Makefile
  tests/cert-tests/Makefile
  tests/slow/Makefile
  tests/suite/Makefile
  fuzz/Makefile
])

AC_OUTPUT

dnl  Warning flags:        errors: ${WERROR_CFLAGS} warnings: ${WARN_CFLAGS}
AC_MSG_NOTICE([summary of build options:

  version:              ${VERSION} shared $LT_CURRENT:$LT_REVISION:$LT_AGE
  Host/Target system:   ${host}
  Build system:         ${build}
  Install prefix:       ${prefix}
  Compiler:             ${CC}
  Valgrind:             $opt_valgrind_tests ${VALGRIND}
  CFlags:               ${CFLAGS}
  Library types:        Shared=${enable_shared}, Static=${enable_static}
  Local libopts:        ${included_libopts}
  Local libtasn1:       ${included_libtasn1}
  Local unistring:      ${included_unistring}
  Use nettle-mini:      ${mini_nettle}
  Documentation:        ${enable_doc} (manpages: ${enable_manpages})
])

AC_MSG_NOTICE([External hardware support:

  /dev/crypto:          $enable_cryptodev
  Hardware accel:       $hw_accel
  Padlock accel:        $use_padlock
  Random gen. variant:  $rnd_variant
  PKCS#11 support:      $with_p11_kit
  TPM support:          $with_tpm
])
if test -n "$ac_trousers_lib";then
AC_MSG_NOTICE([
  TPM library:          $ac_trousers_lib
])
fi

AC_MSG_NOTICE([Optional features:
(note that included applications might not compile properly
if features are disabled)

  SSL3.0 support:       $ac_enable_ssl3
  SSL2.0 client hello:  $ac_enable_ssl2
  Allow SHA1 sign:      $ac_allow_sha1
  DTLS-SRTP support:    $ac_enable_srtp
  ALPN support:         $ac_enable_alpn
  OCSP support:         $ac_enable_ocsp
  SRP support:          $ac_enable_srp
  PSK support:          $ac_enable_psk
  DHE support:          $ac_enable_dhe
  ECDHE support:        $ac_enable_ecdhe
  GOST support:         $ac_enable_gost
  Anon auth support:    $ac_enable_anon
  Heartbeat support:    $ac_enable_heartbeat
  IDNA support:         $idna_support
  Non-SuiteB curves:    $enable_non_suiteb
  FIPS140 mode:         $enable_fips
])

AC_MSG_NOTICE([Optional libraries:

  Guile wrappers:       $opt_guile_bindings
  C++ library:          $use_cxx
  DANE library:         $enable_dane
  OpenSSL compat:       $enable_openssl
])

AC_MSG_NOTICE([System files:

  Trust store pkcs11:   $with_default_trust_store_pkcs11
  Trust store dir:      $with_default_trust_store_dir
  Trust store file:     $with_default_trust_store_file
  Blacklist file:       $with_default_blacklist_file
  CRL file:             $with_default_crl_file
  Priority file:        $system_priority_file
  DNSSEC root key file: $unbound_root_key_file
])

if test ! -f "$unbound_root_key_file"; then
AC_MSG_WARN([[
*** 
*** The DNSSEC root key file in $unbound_root_key_file was not found. 
*** This file is needed for the verification of DNSSEC responses.
*** Use the command: unbound-anchor -a "$unbound_root_key_file"
*** to generate or update it.
*** ]])
fi

if test "${enable_static}" != no;then
AC_MSG_WARN([[
*** GnuTLS will be build as a static library. That means that library
*** constructors for gnutls_global_init will not be made available to
*** linking applications. If you are building that library for arbitrary
*** applications to link, do not enable static linking.
]])
fi

if test "$enable_fuzzer_target" != "no";then
AC_MSG_WARN([[
*** This version of the library is for fuzzying purposes and is intentionally broken!
]])
fi
