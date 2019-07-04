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
# along with this program.  If not, see <https://www.gnu.org/licenses/>

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

EXTRA_DIST = README license.txt files.mk

noinst_LTLIBRARIES = libx86.la

libx86_la_SOURCES = x86-common.c aes-x86.h x86-common.h sha-x86-ssse3.c sha-x86.h hmac-x86-ssse3.c \
	aes-gcm-x86-ssse3.c aes-gcm-x86-aesni.c aes-cbc-x86-ssse3.c aes-cbc-x86-aesni.c aes-gcm-aead.h \
	aes-ccm-x86-aesni.c

if ENABLE_PADLOCK
libx86_la_SOURCES += sha-padlock.c hmac-padlock.c aes-padlock.c aes-gcm-padlock.c \
	aes-padlock.h sha-padlock.h
endif

include files.mk

if ASM_X86_64
AM_CFLAGS += -DASM_X86_64 -DASM_X86
libx86_la_SOURCES += aes-gcm-x86-pclmul.c aes-gcm-x86-pclmul-avx.c

if WINDOWS
libx86_la_SOURCES += $(X86_64_FILES_COFF)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_64_PADLOCK_FILES_COFF)
endif
endif

if MACOSX
libx86_la_SOURCES += $(X86_64_FILES_MACOSX)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_64_PADLOCK_FILES_MACOSX)
endif
endif

if ELF
libx86_la_SOURCES += $(X86_64_FILES_ELF)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_64_PADLOCK_FILES_ELF)
endif
endif

else #ASM_X86_64
AM_CFLAGS += -DASM_X86_32 -DASM_X86

if WINDOWS
libx86_la_SOURCES += $(X86_FILES_COFF)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_PADLOCK_FILES_COFF)
endif
endif

if MACOSX
libx86_la_SOURCES += $(X86_FILES_MACOSX)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_PADLOCK_FILES_MACOSX)
endif
endif

if ELF
libx86_la_SOURCES += $(X86_FILES_ELF)
if ENABLE_PADLOCK
libx86_la_SOURCES += $(X86_PADLOCK_FILES_ELF)
endif
endif

endif #ASM_X86_64
