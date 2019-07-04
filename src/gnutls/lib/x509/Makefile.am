## Process this file with automake to produce Makefile.in
# Copyright (C) 2003-2012 Free Software Foundation, Inc.
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 3 of
# the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = \
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl		\
	-I$(srcdir)/../includes		\
	-I$(builddir)/../includes	\
	-I$(srcdir)/..	

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../minitasn1
endif

EXTRA_DIST = supported_exts.gperf
BUILT_SOURCES = supported_exts.h

noinst_LTLIBRARIES = libgnutls_x509.la

libgnutls_x509_la_SOURCES =	\
	common.c key_encode.c	\
	common.h key_decode.c	\
	time.c			\
	crl.c			\
	crl_write.c		\
	crq.c			\
	dn.c			\
	attributes.c		\
	attributes.h		\
	prov-seed.c		\
	prov-seed.h		\
	extensions.c		\
	mpi.c			\
	output.c		\
	pkcs12.c		\
	pkcs12_bag.c		\
	pkcs12_encr.c		\
	pkcs7.c			\
	pkcs7-attrs.c		\
	pkcs7-crypt.c pkcs7_int.h \
	privkey.c		\
	privkey_pkcs8.c		\
	privkey_pkcs8_pbes1.c	\
	privkey_openssl.c	\
	hostname-verify.c	\
	sign.c			\
	verify.c		\
	x509.c x509_dn.c	\
	x509_int.h		\
	x509_write.c		\
	name_constraints.c	\
	verify-high.c		\
	verify-high2.c		\
	verify-high.h		\
	x509_ext.c		\
	email-verify.c		\
	pkcs7-output.c		\
	virt-san.c		\
	virt-san.h		\
	spki.c			\
	x509_ext_int.h		\
	tls_features.c		\
	krb5.c krb5.h		\
	ip.c ip.h ip-in-cidr.h  \
	supported_exts.h ocsp.h

if ENABLE_OCSP
libgnutls_x509_la_SOURCES += ocsp.c ocsp_output.c
endif

supported_exts.h: $(srcdir)/supported_exts.gperf
	-gperf --global-table -t $^ > $@-tmp \
	&& sed 's/^const struct supported_exts_st \*/static const struct supported_exts_st \*/' <$@-tmp >$@ \
	&& rm -f $@-tmp
