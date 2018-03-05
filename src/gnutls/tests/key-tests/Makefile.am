## Process this file with automake to produce Makefile.in
# Copyright (C) 2007-2008, 2010, 2012 Free Software Foundation, Inc.
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

EXTRA_DIST = data/key-ca.pem data/key-user.pem \
	data/ca-gnutls-keyid.pem data/ca-no-keyid.pem data/ca-weird-keyid.pem \
	data/key-ca-1234.p8 data/key-ca-empty.p8 data/key-ca-null.p8 \
	data/openssl-key-ecc.p8 data/key-ecc.p8 data/key-ecc.pem suppressions.valgrind \
	data/encpkcs8.pem data/unencpkcs8.pem data/enc2pkcs8.pem \
	data/openssl-3des.p8 data/openssl-3des.p8.txt data/openssl-aes128.p8 \
	data/openssl-aes128.p8.txt data/openssl-aes256.p8 data/openssl-aes256.p8.txt \
	data/cert.dsa.1024.pem data/cert.dsa.2048.pem data/cert.dsa.3072.pem \
	data/dsa.1024.pem data/dsa.2048.pem data/dsa.3072.pem data/dsa-pubkey-1018.pem \
	data/bad-key.pem data/p8key-illegal.pem data/key-illegal.pem data/pkcs8-pbes2-sha256.pem \
	data/pkcs8-pbes1-des-md5.pem data/pkcs8-invalid8.der data/key-invalid1.der

dist_check_SCRIPTS = key-id pkcs8 pkcs8-decode dsa ecdsa illegal-rsa pkcs8-invalid key-invalid

TESTS = key-id ecdsa pkcs8-invalid key-invalid

if !ENABLE_FIPS140
TESTS += pkcs8-decode pkcs8
endif

if !WITH_OLD_NETTLE
TESTS += illegal-rsa
endif

if !WINDOWS
TESTS += dsa
endif

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	ASAN_OPTIONS="detect_leaks=0:exitcode=6" \
	LC_ALL="C"						\
	VALGRIND="$(VALGRIND)"					\
	top_builddir="$(top_builddir)"				\
	srcdir="$(srcdir)"

if WANT_TEST_SUITE
LOG_COMPILER = $(VALGRIND)
endif
