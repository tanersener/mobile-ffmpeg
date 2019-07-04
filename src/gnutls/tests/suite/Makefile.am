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
	-I$(top_srcdir)/doc/examples	\
	-I$(top_srcdir)/tests/suite/ecore/src/include \
	-I$(top_srcdir)/tests/suite/ecore/src/lib

EXTRA_DIST = cbc-record-check.sh

check_PROGRAMS = rng prime-check
AM_LDFLAGS = -no-install
LDADD = ../../lib/libgnutls.la \
	../../gl/libgnu.la \
	../libutils.la \
	$(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB)

prime_check_LDADD = $(LDADD) -lhogweed -lgmp

libecore_la_CPPFLAGS = -I$(top_srcdir)/tests/suite/ecore/		\
	-I$(top_srcdir)/tests/suite/ecore/src/include			\
	-I$(top_srcdir)/tests/suite/ecore/src/lib -DHAVE_CONFIG_H	\
	-D__UNUSED__= -DVMAJ=1 -D VMIC=0 -DVMIN=0 -DVREV=0		\
	-DEFL_HAVE_POSIX_THREADS=1 -DEFL_HAVE_POSIX_THREADS_SPINLOCK=1	\
	-DEFL_HAVE_THREADS=1 -DEINA_BUILD_CHAINED_POOL=1		\
	-DEINA_STATIC_BUILD_CHAINED_POOL=1 -DHAVE_CLOCK_GETTIME		\
	-DHAVE_GETTIMEOFDAY -DPACKAGE_LIB_DIR=\"/usr/lib\"		\
	-DMODULE_ARCH=\"unix\" -DSHARED_LIB_SUFFIX=\".so\"

libecore_la_CFLAGS = -w
nodist_libecore_la_SOURCES = ecore/src/lib/ecore_anim.c			\
	ecore/src/lib/ecore_app.c ecore/src/lib/ecore.c			\
	ecore/src/lib/ecore_events.c ecore/src/lib/ecore_exe.c		\
	ecore/src/lib/ecore_getopt.c ecore/src/lib/ecore_glib.c		\
	ecore/src/lib/ecore_idle_enterer.c				\
	ecore/src/lib/ecore_idle_exiter.c ecore/src/lib/ecore_idler.c	\
	ecore/src/lib/ecore_job.c ecore/src/lib/ecore_main.c		\
	ecore/src/lib/ecore_pipe.c ecore/src/lib/ecore_poll.c		\
	ecore/src/lib/ecore_signal.c ecore/src/lib/ecore_thread.c	\
	ecore/src/lib/ecore_time.c ecore/src/lib/ecore_timer.c		\
	ecore/src/lib/eina_accessor.c ecore/src/lib/eina_array.c	\
	ecore/src/lib/eina_benchmark.c ecore/src/lib/eina_binshare.c	\
	ecore/src/lib/eina_chained_mempool.c				\
	ecore/src/lib/eina_convert.c ecore/src/lib/eina_counter.c	\
	ecore/src/lib/eina_cpu.c ecore/src/lib/eina_error.c		\
	ecore/src/lib/eina_file.c ecore/src/lib/eina_fp.c		\
	ecore/src/lib/eina_hamster.c ecore/src/lib/eina_hash.c		\
	ecore/src/lib/eina_inlist.c ecore/src/lib/eina_iterator.c	\
	ecore/src/lib/eina_lalloc.c ecore/src/lib/eina_list.c		\
	ecore/src/lib/eina_log.c ecore/src/lib/eina_magic.c		\
	ecore/src/lib/eina_main.c ecore/src/lib/eina_matrixsparse.c	\
	ecore/src/lib/eina_mempool.c ecore/src/lib/eina_module.c	\
	ecore/src/lib/eina_quadtree.c ecore/src/lib/eina_rbtree.c	\
	ecore/src/lib/eina_rectangle.c					\
	ecore/src/lib/eina_safety_checks.c ecore/src/lib/eina_sched.c	\
	ecore/src/lib/eina_share_common.c ecore/src/lib/eina_strbuf.c	\
	ecore/src/lib/eina_strbuf_common.c ecore/src/lib/eina_str.c	\
	ecore/src/lib/eina_stringshare.c ecore/src/lib/eina_tiler.c	\
	ecore/src/lib/eina_unicode.c ecore/src/lib/eina_ustrbuf.c	\
	ecore/src/lib/eina_ustringshare.c ecore/src/lib/eina_value.c


EXTRA_DIST += testcompat-main-polarssl testcompat-main-openssl \
	      testcompat-common params.dh tls-fuzzer/tls-fuzzer-common.sh

scripts_to_test = chain.sh \
	testrng.sh testcompat-polarssl.sh testcompat-openssl.sh \
	testrandom.sh tls-fuzzer/tls-fuzzer-nocert.sh \
	tls-fuzzer/tls-fuzzer-cert.sh tls-fuzzer/tls-fuzzer-alpn.sh \
	tls-fuzzer/tls-fuzzer-nocert-tls13.sh tls-fuzzer/tls-fuzzer-psk.sh \
	multi-ticket-reception.sh

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"			\
	VALGRIND="$(VALGRIND)"		\
	top_builddir="$(top_builddir)"	\
	srcdir="$(srcdir)"		\
	ASAN_OPTIONS="detect_leaks=0"	\
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

if ENABLE_DANE
scripts_to_test += testdane.sh
endif

if !MACOSX
if !WINDOWS
noinst_LTLIBRARIES = libecore.la

mini_record_timing_LDADD = -lrt $(LDADD)
eagain_cli_LDADD = libecore.la -lrt -lm $(LIBDL) -lpthread $(LDADD)
nodist_eagain_cli_SOURCES = mini-eagain2.c

noinst_PROGRAMS = eagain-cli mini-record-timing

scripts_to_test += eagain.sh
endif
endif

nodist_check_SCRIPTS = $(scripts_to_test)

TESTS = $(scripts_to_test) prime-check

prime_check_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)

TEST_EXTENSIONS = .sh

if WANT_TEST_SUITE
LOG_COMPILER = $(VALGRIND)
SH_LOG_COMPILER =
endif
