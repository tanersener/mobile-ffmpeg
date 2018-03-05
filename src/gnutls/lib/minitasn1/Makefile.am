## Process this file with automake to produce Makefile.in
# Copyright (C) 2002-2010, 2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# The GnuTLS is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with GnuTLS; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = -DASN1_BUILDING		\
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl          \
	-I$(srcdir)/..

noinst_LTLIBRARIES = libminitasn1.la

libminitasn1_la_SOURCES = libtasn1.h gstr.h int.h parser_aux.h		\
        structure.h element.h decoding.c gstr.c errors.c parser_aux.c	\
        structure.c element.c coding.c version.c
