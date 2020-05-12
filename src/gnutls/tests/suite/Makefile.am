## Process this file with automake to produce Makefile.in
# Copyright (C) 2004-2012 Free Software Foundation, Inc.
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

AM_CFLAGS = $(WARN_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/gl			\
	-I$(top_builddir)/gl			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/extra/includes	\
	-I$(top_builddir)/extra/includes	\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/tests			\
	-I$(top_srcdir)/doc/examples

check_PROGRAMS = rng prime-check
AM_LDFLAGS = -no-install
LDADD = ../../lib/libgnutls.la \
	../../gl/libgnu.la \
	../libutils.la \
	$(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB)

prime_check_LDADD = $(LDADD) -lhogweed -lgmp

scripts_to_test = chain.sh \
	testrng.sh testcompat-polarssl.sh testcompat-openssl.sh \
	testrandom.sh tls-fuzzer/tls-fuzzer-nocert.sh \
	tls-fuzzer/tls-fuzzer-cert.sh tls-fuzzer/tls-fuzzer-alpn.sh \
	tls-fuzzer/tls-fuzzer-nocert-tls13.sh tls-fuzzer/tls-fuzzer-psk.sh \
	tls-fuzzer/tls-fuzzer-nolimit.sh tls-fuzzer/tls-fuzzer-nolimit-tls13.sh \
	multi-ticket-reception.sh

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"			\
	VALGRIND="$(VALGRIND)"		\
	top_builddir="$(top_builddir)"	\
	srcdir="$(srcdir)"		\
	ASAN_OPTIONS="detect_leaks=0"	\
	GNUTLS_TEST_SUITE_RUN=1		\
	GNUTLS_SYSTEM_PRIORITY_FILE=$(abs_top_srcdir)/tests/system.prio \
	OPENSSL_ia32cap=0x00000000

if ENABLE_NON_SUITEB_CURVES
TESTS_ENVIRONMENT += ENABLE_NON_SUITEB_CURVES=1
endif

if ENABLE_SSL3
scripts_to_test += tls-fuzzer/tls-fuzzer-nocert-ssl3.sh
TESTS_ENVIRONMENT += ENABLE_SSL3=1
endif

if ENABLE_TLS13_INTEROP
scripts_to_test += testcompat-tls13-openssl.sh
endif

if ENABLE_OLDGNUTLS_INTEROP
scripts_to_test += testcompat-oldgnutls.sh
endif

if ENABLE_DANE
scripts_to_test += testdane.sh
endif

if !MACOSX
if !WINDOWS
mini_record_timing_LDADD = -lrt $(LDADD)
nodist_mini_record_timing_SOURCES = mini-record-timing.c
eagain_cli_LDADD = $(LIBEV_LIBS) -lrt -lm $(LIBDL) -lpthread $(LDADD)
nodist_eagain_cli_SOURCES = eagain-cli.c

noinst_PROGRAMS = eagain-cli mini-record-timing

scripts_to_test += eagain.sh
endif
endif

nodist_prime_check_SOURCES = prime-check.c
nodist_rng_SOURCES = rng.c

nodist_check_SCRIPTS = $(scripts_to_test)

TESTS = $(scripts_to_test) prime-check

prime_check_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)

TEST_EXTENSIONS = .sh

if WANT_TEST_SUITE
LOG_COMPILER = $(VALGRIND)
SH_LOG_COMPILER =
endif
