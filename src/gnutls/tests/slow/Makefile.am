## Process this file with automake to produce Makefile.in
# Copyright (C) 2010, 2012 Free Software Foundation, Inc.
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
	-I$(top_srcdir)/gl

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(top_builddir)/lib/minitasn1
endif

AM_LDFLAGS = -no-install
LDADD = ../libutils.la \
	../../gl/libgnu.la \
	$(top_builddir)/lib/libgnutls.la $(LIBSOCKET)

cipher_test_CPPFLAGS = $(AM_CPPFLAGS)
cipher_override2_CPPFLAGS = $(AM_CPPFLAGS)

ctests = gendh

if ENABLE_NETTLE
cipher_override_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS) $(HOGWEED_CFLAGS)
mac_override_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS) $(HOGWEED_CFLAGS)
cipher_override_LDFLAGS = $(NETTLE_LIBS) $(HOGWEED_LIBS) $(GMP_LIBS) $(LDADD)
mac_override_LDFLAGS = $(NETTLE_LIBS) $(HOGWEED_LIBS) $(GMP_LIBS) $(LDADD)
endif


dist_check_SCRIPTS = test-ciphers.sh override-ciphers test-hash-large.sh test-ciphers-common.sh \
	test-ciphers-api.sh
check_PROGRAMS = $(ctests) cipher-test cipher-api-test cipher-override mac-override cipher-override2 hash-large crypto
TESTS = $(ctests) test-ciphers.sh override-ciphers test-hash-large.sh crypto test-ciphers-api.sh

if HAVE_LIBCRYPTO
cipher_openssl_compat_LDADD = $(LDADD) $(LIBCRYPTO)

dist_check_SCRIPTS += test-ciphers-openssl.sh
check_PROGRAMS += cipher-openssl-compat
TESTS += test-ciphers-openssl.sh
endif

EXTRA_DIST = README gnutls-asan.supp

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"						\
	LSAN_OPTIONS=suppressions=gnutls-asan.supp		\
	OPENSSL_ia32cap=0x00000000				\
	top_builddir="$(top_builddir)"				\
	srcdir="$(srcdir)"

if WINDOWS
TESTS_ENVIRONMENT += WINDOWS=1
endif
