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

AM_LIBTOOLFLAGS=--tag=CC

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = -I$(srcdir)/../../../gl		\
	-I$(builddir)/../../../gl		\
	-I$(srcdir)/../../includes	\
	-I$(builddir)/../../includes	\
	-I$(srcdir)/../../ \
	-I$(srcdir)/../

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../../minitasn1
endif

#ensure that we have all aarch64 instruction sets enabled for the assembler
AM_CCASFLAGS = -Wa,-march=all

EXTRA_DIST = README

noinst_LTLIBRARIES = libaarch64.la

libaarch64_la_SOURCES = aarch64-common.c aarch64-common.h sha-aarch64.h sha-aarch64.c \
	hmac-sha-aarch64.c aes-cbc-aarch64.c aes-gcm-aarch64.c aes-aarch64.h aes-ccm-aarch64.c

if ASM_AARCH64
libaarch64_la_SOURCES += elf/sha1-armv8.s elf/sha512-armv8.s elf/sha256-armv8.s \
	elf/aes-aarch64.s elf/ghash-aarch64.s

endif #ASM_AARCH64
