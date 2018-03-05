## Process this file with automake to produce Makefile.in
# Copyright (C) 2012 KU Leuven
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of libdane.
#
# libdane is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# GnuTLS-extra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

ACLOCAL_AMFLAGS = -I ../m4 -I ../gl/m4

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = \
	-I$(srcdir)/../gl		\
	-I$(builddir)/../gl		\
	-I$(builddir)/../lib/includes	\
	-I$(srcdir)/../lib/includes	\
	-I$(srcdir)/includes		\
	-I$(builddir)/includes		\
	-I$(srcdir)/../lib/minitasn1	\
	-I$(srcdir)/../lib

SUBDIRS = includes

libgnutls_dane_la_LDFLAGS = -no-undefined $(COMMON_LINK_FLAGS)

lib_LTLIBRARIES = libgnutls-dane.la

libgnutls_dane_la_SOURCES = dane.c dane-params.c errors.c libdane.map

libgnutls_dane_la_LIBADD = ../gl/libgnu.la \
	../lib/libgnutls.la

libgnutls_dane_la_LDFLAGS += -version-info $(LT_DANE_CURRENT):$(LT_DANE_REVISION):$(LT_DANE_AGE)

libgnutls_dane_la_LIBADD += $(LIBSOCKET) $(UNBOUND_LIBS)

if HAVE_LD_VERSION_SCRIPT
libgnutls_dane_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libdane.map
else
libgnutls_dane_la_LDFLAGS += -export-symbols-regex '^(dane).*'
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnutls-dane.pc
DISTCLEANFILES = $(pkgconfig_DATA)

if HAVE_LD_OUTPUT_DEF
libgnutls_dane_la_LDFLAGS += \
	-Wl,--output-def,libgnutls-dane-$(DLL_VERSION).def
libgnutls_dane-$(DLL_VERSION).def: libgnutls-dane.la

defexecdir = $(bindir)
defexec_DATA = libgnutls-dane-$(DLL_VERSION).def
DISTCLEANFILES += $(defexec_DATA)
endif
