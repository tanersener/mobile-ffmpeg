## Process this file with automake to produce Makefile.in
# Copyright (C) 2011-2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GNUTLS.
#
# The GNUTLS library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 3 of
# the License, or (at your option) any later version.
#
# The GNUTLS library is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

include $(top_srcdir)/lib/common.mk

SUBDIRS = 
AM_CPPFLAGS = \
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl		\
	-I$(srcdir)/../includes		\
	-I$(builddir)/../../gl		\
	-I$(builddir)/../includes	\
	-I$(srcdir)/..

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../minitasn1
endif

noinst_LTLIBRARIES = libaccelerated.la

EXTRA_DIST = accelerated.h cryptodev.h
libaccelerated_la_SOURCES = accelerated.c cryptodev.c cryptodev-gcm.c
libaccelerated_la_LIBADD =

if ENABLE_NETTLE
if ASM_X86
SUBDIRS += x86
AM_CFLAGS += -DASM_X86
libaccelerated_la_LIBADD += x86/libx86.la
endif

if ASM_AARCH64
SUBDIRS += aarch64
AM_CFLAGS += -DASM_AARCH64
libaccelerated_la_LIBADD += aarch64/libaarch64.la
endif
endif

if ASM_X86_32
AM_CFLAGS += -DASM_X86_32
endif

if ASM_X86_64
AM_CFLAGS += -DASM_X86_64
endif
