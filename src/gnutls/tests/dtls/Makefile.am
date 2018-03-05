## Process this file with automake to produce Makefile.in
# Copyright (C) 2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
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

dist_check_SCRIPTS = dtls dtls-nb dtls-resume

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)
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

AM_LDFLAGS = -no-install
LDADD = ../../lib/libgnutls.la \
	../../gl/libgnu.la \
	$(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBRT)

if !WINDOWS

check_PROGRAMS = dtls-stress
TESTS = dtls dtls-resume

endif

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"						\
	top_builddir="$(top_builddir)"				\
	srcdir="$(srcdir)"
