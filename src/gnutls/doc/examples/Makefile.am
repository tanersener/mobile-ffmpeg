## Process this file with automake to produce Makefile.in
# Copyright (C) 2005-2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
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
AM_CPPFLAGS = \
	-I$(top_srcdir)/lib/includes -I$(top_builddir)/lib/includes	\
	-I$(top_srcdir)/extra/includes \
	-I$(top_srcdir)/src/gl \
	-I$(top_builddir)/src/gl

# Gnulib warns and suggests use of fseeko instead of fseek, which is
# used in ex-cert-select.c, but certificate files will not be > 4 GB,
# so we just silence the warning instead of fixing the code here.
AM_CPPFLAGS += -D_GL_NO_LARGE_FILES -DNO_LIBCURL

AM_LDFLAGS = -no-install
LDADD = libexamples.la				\
	../../lib/libgnutls.la			\
	../../gl/libgnu.la			\
	../../src/gl/libgnu_gpl.la		\
	$(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB)

CXX_LDADD = ../../lib/libgnutlsxx.la \
	$(LDADD)

EXTRA_DIST = tlsproxy/buffer.c tlsproxy/buffer.h tlsproxy/crypto-gnutls.c \
	tlsproxy/crypto-gnutls.h tlsproxy/LICENSE tlsproxy/tlsproxy.c \
	tlsproxy/README.md tlsproxy/Makefile

noinst_PROGRAMS = ex-client-resume ex-client-dtls
noinst_PROGRAMS += ex-cert-select ex-client-x509
noinst_PROGRAMS += ex-serv-dtls
noinst_PROGRAMS += print-ciphersuites
if ENABLE_OCSP
noinst_PROGRAMS += ex-serv-x509
endif

if ENABLE_CXX
ex_cxx_SOURCES = ex-cxx.cpp
ex_cxx_LDADD = $(CXX_LDADD)
noinst_PROGRAMS += ex-cxx
endif

if ENABLE_ANON
noinst_PROGRAMS += ex-client-anon ex-serv-anon
endif

if ENABLE_PKCS11
noinst_PROGRAMS += ex-cert-select-pkcs11
endif

if ENABLE_PSK
noinst_PROGRAMS += ex-client-psk
noinst_PROGRAMS += ex-serv-psk
endif

if ENABLE_SRP
noinst_PROGRAMS += ex-client-srp ex-serv-srp
endif

noinst_LTLIBRARIES = libexamples.la

if ENABLE_OCSP
noinst_PROGRAMS += ex-ocsp-client
endif

libexamples_la_SOURCES = examples.h ex-alert.c ex-pkcs12.c \
	ex-session-info.c ex-x509-info.c ex-verify.c	\
	ex-client-x509-3.1.c \
	tcp.c udp.c ex-pkcs11-list.c verify.c ex-verify-ssh.c
