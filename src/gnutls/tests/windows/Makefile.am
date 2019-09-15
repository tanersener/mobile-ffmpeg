## Process this file with automake to produce Makefile.in
# Copyright (C) 2015 Red Hat, Inc.
#
# This file is part of GnuTLS.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes \
	-I$(top_srcdir)/tests/ \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/gl \
	-I$(top_srcdir)/gl \
	-I.

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(top_builddir)/lib/minitasn1
endif

EXTRA_DIST = ncrypt-int.h check-output

AM_LDFLAGS = -no-install
LDADD = $(top_builddir)/tests/libutils.la \
	$(top_builddir)/gl/libgnu.la \
	$(top_builddir)/lib/libgnutls.la $(LIBSOCKET)

ctests =

if WINDOWS
lib_LTLIBRARIES = ncrypt.la crypt32.la

ncrypt_la_SOURCES = ncrypt.c
crypt32_la_SOURCES = crypt32.c

ncrypt_la_LIBADD = ../../gl/libgnu.la ../../lib/libgnutls.la -lmsvcrt -lkernel32 -luser32
crypt32_la_LIBADD = ../../gl/libgnu.la ../../lib/libgnutls.la -lmsvcrt -lkernel32 -luser32
ncrypt_la_LDFLAGS = -module -no-undefined -avoid-version -Wl,--add-stdcall-alias
crypt32_la_LDFLAGS = -module -no-undefined -avoid-version -Wl,--add-stdcall-alias


cng_windows_DEPENDENCIES = ncrypt.la crypt32.la
cng_windows_CPPFLAGS = $(AM_CPPFLAGS)

LOG_COMPILER = $(srcdir)/check-output

endif

ctests += cng-windows

check_PROGRAMS = $(ctests)
dist_check_SCRIPTS =


TESTS = $(ctests) $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT =						\
	WINEDLLOVERRIDES="crypt32=n,ncrypt=n"			\
	LC_ALL="C"						\
	GNUTLS_TEST_SUITE_RUN=1			\
	EXEEXT=$(EXEEXT)					\
	top_builddir="$(top_builddir)"				\
	srcdir="$(srcdir)"
