## Process this file with automake to produce Makefile.in
# Copyright (C) 2004-2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavroyanopoulos
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

AM_CFLAGS += $(HOGWEED_CFLAGS) $(GMP_CFLAGS)

AM_CPPFLAGS = \
	-I$(srcdir)/int		\
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl		\
	-I$(srcdir)/../includes		\
	-I$(builddir)/../includes	\
	-I$(builddir)/../../gl          \
	-I$(srcdir)/..

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../minitasn1
endif

noinst_LTLIBRARIES = libcrypto.la

libcrypto_la_SOURCES = pk.c mpi.c mac.c cipher.c init.c \
	gnettle.h rnd-common.h rnd-common.c \
	rnd.c int/rsa-fips.h int/rsa-keygen-fips186.c int/provable-prime.c \
	int/dsa-fips.h int/dsa-keygen-fips186.c int/dsa-validate.c

if WINDOWS
libcrypto_la_SOURCES += sysrng-windows.c
else
if HAVE_GETENTROPY
libcrypto_la_SOURCES += sysrng-getentropy.c
else
libcrypto_la_SOURCES += sysrng-linux.c
endif
endif

if ENABLE_FIPS140
libcrypto_la_SOURCES += rnd-fips.c int/drbg-aes-self-test.c \
	int/drbg-aes.c int/drbg-aes.h
endif
