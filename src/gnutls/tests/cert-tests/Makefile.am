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

EXTRA_DIST = data/ca-no-pathlen.pem data/no-ca-or-pathlen.pem data/aki-cert.pem \
	data/template-test.key data/template-test.pem templates/template-test.tmpl \
	data/funny-spacing.pem data/ca-certs.pem data/dane-test.rr data/cert-ecc256.pem \
	data/bmpstring.pem data/template-utf8.pem templates/template-utf8.tmpl \
	templates/template-dn.tmpl data/template-dn.pem data/complex-cert.pem \
	data/template-overflow.pem templates/template-overflow.tmpl data/template-overflow2.pem \
	templates/template-overflow2.tmpl data/template-crq.pem \
	templates/template-date.tmpl data/template-date.pem templates/template-dn-err.tmpl \
	templates/template-nc.tmpl data/template-nc.pem data/xmpp-othername.pem \
	suppressions.valgrind data/csr-invalid.der data/invalid-sig2.pem data/invalid-sig3.pem \
	data/invalid-sig.pem email-certs/chain.exclude.test.example.com email-certs/chain.test.example.com \
	email-certs/chain.invalid.example.com email-certs/chain.test.example.com-2 \
	data/single-ca.p7b data/single-ca.p7b.out data/full.p7b data/full.p7b.out data/detached.p7b \
	data/pkcs7-detached.txt data/p7-combined.out data/template-generalized.pem \
	templates/template-generalized.tmpl data/privkey1.pem data/privkey2.pem data/privkey3.pem \
	data/name-constraints-ip.pem data/cert-invalid-utf8.der data/very-long-dn.pem \
	data/provable3072.pem data/provable2048.pem data/provable-dsa2048.pem \
	data/provable-dsa2048-fips.pem templates/template-crq.tmpl \
	data/provable3072.new.pem data/provable2048.new.pem data/provable-dsa2048.new.pem \
	data/provable-dsa2048-fips.new.pem \
	templates/template-unique.tmpl data/template-unique.pem \
	templates/template-othername.tmpl data/template-othername.pem \
	templates/template-othername-xmpp.tmpl data/template-othername-xmpp.pem \
	templates/template-krb5name.tmpl \
	data/template-krb5name.pem data/template-krb5name-full.pem data/template-test-ecc.key \
	data/template-rsa-sha3-256.pem data/template-rsa-sha3-512.pem data/template-rsa-sha3-224.pem \
	data/template-rsa-sha3-384.pem data/long-oids.pem \
	data/name-constraints-ip2.pem data/chain-md5.pem data/gost-cert.pem \
	templates/template-tlsfeature.tmpl data/userid.pem \
	data/template-tlsfeature.pem data/template-tlsfeature.csr \
	templates/template-tlsfeature-crq.tmpl templates/arb-extensions.tmpl data/arb-extensions.pem \
	data/arb-extensions.csr data/pkcs1-pad-ok.pem data/pkcs1-pad-broken.pem \
	data/pkcs1-pad-ok2.pem data/pkcs1-pad-broken2.pem data/pkcs1-pad-broken3.pem \
	data/client.p12 data/noclient.p12 data/unclient.p12 data/pkcs12_2certs.p12 \
	data/pkcs12_5certs.p12 data/test-null.p12 data/cert-ca.p12 data/sha256.p12 \
	data/key-ca.pem data/key-subca.pem data/key-subsubca.pem data/key-user.pem \
	data/key-dsa.pem data/key-ca-dsa.pem data/key-subca-dsa.pem \
	data/ca-public.gpg data/srv-public-all-signed.gpg data/srv-secret.gpg \
	data/ca-secret.gpg data/srv-public.gpg data/srv-public-127.0.0.1-signed.gpg	\
	data/srv-public-localhost-signed.gpg data/selfsigs/alice-mallory-badsig18.pub \
	data/selfsigs/alice-mallory-irrelevantsig.pub data/selfsigs/alice-mallory-nosig18.pub \
	data/selfsigs/alice.pub data/key-utf8-1.p12 data/key-utf8-2.p12 \
	data/code-signing-ca.pem data/code-signing-cert.pem data/multi-value-dn.pem \
	data/pkcs7-cat-ca.pem data/pkcs7-cat.p7 data/openssl.p7b data/openssl.p7b.out \
	data/openssl-keyid.p7b data/openssl-keyid.p7b.out data/openssl.p12 \
	data/openpgp-invalid1.pub data/openpgp-invalid2.pub data/openpgp-invalid3.pub \
	data/openpgp-invalid4.pub data/openpgp-invalid5.pub \
	data/openpgp-invalid5.pub data/openpgp-invalid6.pub \
	data/openpgp-invalid7.pub data/openpgp-invalid8.pub \
	data/key-corpus-rc2-1.p12 data/key-corpus-rc2-2.p12 data/key-corpus-rc2-3.p12 \
	data/pkcs7-chain.pem data/pkcs7-chain-root.pem \
	data/pkcs7-chain-endcert-key.pem data/openssl-invalid-time-format.pem

dist_check_SCRIPTS = pathlen aki certtool invalid-sig email crq \
	pkcs7 pkcs7-broken-sigs privkey-import name-constraints certtool-long-cn crl provable-privkey \
	provable-privkey-new \
	provable-dh userid sha2-test sha2-dsa-test provable-privkey-dsa2048 \
	provable-privkey-rsa2048 provable-privkey-gen-default pkcs7-constraints \
	pkcs7-constraints2 certtool-long-oids pkcs7-cat pkcs12-corner-cases \
	pkcs7-list-sign

if WANT_TEST_SUITE
dist_check_SCRIPTS += provable-dh-default
endif

if !ENABLE_FIPS140
dist_check_SCRIPTS += pkcs12
endif

if ENABLE_OPENPGP
# The selftest is disabled until we can make it work under Wine and
# under Debian buildds (problem with 127.0.0.2?).
dist_check_SCRIPTS += openpgp-selfsigs
dist_check_SCRIPTS += openpgp-cert-parser
if !WINDOWS
dist_check_SCRIPTS += openpgp-certs
endif
endif

if !HAVE_BUGGY_LIBIDN
dist_check_SCRIPTS += certtool-utf8
endif

if !WINDOWS
dist_check_SCRIPTS += template-test pem-decoding othername-test krb5-test sha3-test md5-test \
	tlsfeature-test template-exts-test pkcs1-pad pkcs12-utf8
endif

if ENABLE_DANE
dist_check_SCRIPTS += dane
endif

TESTS = $(dist_check_SCRIPTS)

# Set detect_leaks=0 to ASAN. It seems it is detecting many leaks in tools
# which are not trivial, and makes no point to address.
TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"				\
	VALGRIND="$(VALGRIND)"			\
	LIBTOOL="$(LIBTOOL)"			\
	top_builddir="$(top_builddir)"		\
	ASAN_OPTIONS="detect_leaks=0:exitcode=6" \
	srcdir="$(srcdir)"

if ENABLE_FIPS140
TESTS_ENVIRONMENT += FIPS140=1
endif

if WINDOWS
TESTS_ENVIRONMENT += WINDOWS=1
endif
