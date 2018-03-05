## Process this file with automake to produce Makefile.in
# Copyright (C) 2004-2012 Free Software Foundation, Inc.
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

SUBDIRS = . cert-tests ocsp-tests key-tests slow dtls windows

if WANT_TEST_SUITE
SUBDIRS += suite
endif

EXTRA_DIST = suppressions.valgrind eagain-common.h cert-common.h test-chains.h \
	certs/ca-cert-ecc.pem  certs/cert-ecc256.pem  certs/cert-ecc521.pem \
	certs/cert-rsa-2432.pem certs/ecc384.pem certs/ecc.pem \
	certs/ca-ecc.pem certs/cert-ecc384.pem certs/cert-ecc.pem certs/ecc256.pem \
	certs/ecc521.pem certs/rsa-2432.pem x509cert-dir/ca.pem psk.passwd \
	system.prio pkcs11/softhsm.h pkcs11/pkcs11-pubkey-import.c gnutls-asan.supp \
	rsa-md5-collision/README safe-renegotiation/README starttls-smtp.txt starttls-ftp.txt \
	starttls-lmtp.txt starttls-pop3.txt starttls-nntp.txt starttls-sieve.txt \
	rsa-md5-collision/colliding-chain-md5-2.pem rsa-md5-collision/colliding-chain-md5-1.pem \
	certs-interesting/README.md certs-interesting/cert1.der certs-interesting/cert1.der.err \
	certs-interesting/cert2.der certs-interesting/cert2.der.err certs-interesting/cert3.der \
	certs-interesting/cert3.der.err certs-interesting/cert4.der certs-interesting/cert5.der \
	certs-interesting/cert6.der certs-interesting/cert7.der certs-interesting/cert8.der \
	scripts/common.sh scripts/starttls-common.sh certs-interesting/cert5.der.err \
	cert-repro-20170915.h

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)
AM_CPPFLAGS = \
	$(P11_KIT_CFLAGS) \
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/libdane/includes	\
	-I$(top_builddir)/libdane/includes	\
	-I$(top_srcdir)/extra/includes		\
	-I$(top_builddir)/extra/includes	\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/doc/examples

AM_LDFLAGS = -no-install
COMMON_GNUTLS_LDADD = ../lib/libgnutls.la
COMMON_DEPS_LDADD = $(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBSECCOMP)
COMMON_LDADD = $(COMMON_GNUTLS_LDADD) $(COMMON_DEPS_LDADD)

LDADD = $(COMMON_GNUTLS_LDADD) \
	libutils.la \
	$(COMMON_DEPS_LDADD)

dane_LDADD = $(LDADD) ../libdane/libgnutls-dane.la

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../lib/minitasn1
endif

noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = utils.h utils.c seccomp.c utils-adv.c
libutils_la_LIBADD = ../lib/libgnutls.la

ctests = mini-record-2 simple gc set_pkcs12_cred cert certuniqueid	\
	 mpi certificate_set_x509_crl dn parse_ca x509-dn x509-dn-decode record-sizes \
	 hostname-check cve-2008-4989 pkcs12_s2k chainverify record-sizes-range \
	 crq_key_id x509sign-verify cve-2009-1415 cve-2009-1416		\
	 crq_apis init_roundtrip pkcs12_s2k_pem dn2 mini-eagain	tls-rehandshake-cert-3 \
	 nul-in-x509-names x509_altname pkcs12_encode mini-x509		\
	 tls-rehandshake-cert rng-fork mini-eagain-dtls resume-dtls \
	 x509cert x509cert-tl infoaccess mini-dtls-hello-verify \
	 mini-tdb dtls-rehandshake-anon mini-alpn mini-dtls-large \
	 mini-termination mini-x509-cas mini-x509-2 pkcs12_simple \
	 mini-emsgsize-dtls chainverify-unsorted mini-overhead \
	 mini-dtls-heartbeat mini-x509-callbacks key-openssl priorities	\
	 mini-dtls-srtp rsa-encrypt-decrypt mini-loss-time \
	 mini-record mini-dtls-record mini-handshake-timeout mini-record-range \
	 mini-cert-status rsa-psk global-init sec-params \
	 fips-test mini-global-load name-constraints x509-extensions \
	 long-session-id mini-x509-callbacks-intr mini-dtls-lowmtu \
	 crlverify mini-dtls-discard init_fds mini-record-failure \
	 tls-rehandshake-cert-2 custom-urls set_x509_key_mem set_x509_key_file \
	 mini-chain-unsorted x509-verify-with-crl mini-dtls-mtu privkey-verify-broken \
	 mini-dtls-record-asym openpgp-callback key-import-export \
	 mini-dtls-fork mini-dtls-pthread mini-key-material x509cert-invalid \
	 tls-ext-register tls-supplemental mini-dtls0-9 \
	 mini-record-retvals mini-server-name tls-etm x509-cert-callback \
	 sign-md5-rep keygen mini-tls-nonblock no-signal pkcs7-gen dtls-etm \
	 x509sign-verify2 mini-alignment oids atfork prf psk-file \
	 status-request status-request-ok status-request-missing \
	 fallback-scsv pkcs8-key-decode urls dtls-rehandshake-cert \
	 key-usage-rsa key-usage-ecdhe-rsa mini-session-verify-function auto-verify \
	 record-timeouts mini-dtls-hello-verify-48 mini-x509-default-prio \
	 mini-x509-dual global-init-override tlsext-decoding rsa-psk-cb \
	 rehandshake-switch-cert rehandshake-switch-cert-allow rehandshake-switch-cert-client \
	 rehandshake-switch-cert-client-allow handshake-versions dtls-handshake-versions \
	 dtls-max-record tls-max-record alpn-server-prec ocsp-filename-memleak \
	 dh-params rehandshake-ext-secret pcert-list session-export-funcs \
	 handshake-false-start version-checks key-material-dtls key-material-set-dtls \
	 system-prio-file name-constraints-merge crl-basic crq-basic \
	 send-client-cert custom-urls-override hex rehandshake-switch-psk-id \
	 rehandshake-switch-srp-id base64 srpbase64 pkcs1-digest-info set_x509_key \
	 set_x509_key_file_der set_x509_pkcs12_key crt_apis tls1.2-cert-key-exchange \
	 tls1.1-cert-key-exchange tls1.0-cert-key-exchange ssl3.0-cert-key-exchange \
	 dtls1.2-cert-key-exchange dtls1.0-cert-key-exchange x509-cert-callback-legacy \
	 keylog-env ssl2-hello tlsfeature-ext dtls-rehandshake-cert-2 \
	 tlsfeature-crt dtls-rehandshake-cert-3 resume-with-false-start \
	 set_x509_key_file_ocsp client-fastopen rng-sigint srp rng-pthread \
	 safe-renegotiation/srn0 safe-renegotiation/srn1 safe-renegotiation/srn2 \
	 safe-renegotiation/srn3 safe-renegotiation/srn4 safe-renegotiation/srn5 \
	 rsa-illegal-import set_x509_key_file_ocsp_multi set_key set_x509_key_file_ocsp_multi2 \
	 set_key_utf8 set_x509_key_utf8 insecure_key handshake-large-packet \
	 client_dsa_key server_ecdsa_key tls-session-ext-register tls-session-supplemental \
	 multi-alerts naked-alerts pkcs7-cat-parse set_known_dh_params_x509 \
	 set_known_dh_params_anon set_known_dh_params_psk session-tickets-ok \
	 session-tickets-missing set_x509_key_file_legacy status-request-ext \
	 rng-no-onload dtls1-2-mtu-check crl_apis cert_verify_inv_utf8 \
	 hostname-check-utf8 pkcs8-key-decode-encrypted priority-mix trust-store \
	 dtls-repro-20170915

if HAVE_SECCOMP_TESTS
ctests += dtls-with-seccomp tls-with-seccomp dtls-client-with-seccomp tls-client-with-seccomp
endif

if HAVE_CMOCKA
CMOCKA_LDADD = $(COMMON_LDADD) $(CMOCKA_LIBS)
ctests += dtls-sliding-window ip-utils name-constraints-ip conv-utf8 str-unicode str-idna

dtls_sliding_window_LDADD = $(CMOCKA_LDADD)
ip_utils_LDADD = $(CMOCKA_LDADD)
name_constraints_ip_LDADD = $(CMOCKA_LDADD)
conv_utf8_LDADD = $(CMOCKA_LDADD)
str_unicode_LDADD = $(CMOCKA_LDADD)
str_idna_LDADD = $(CMOCKA_LDADD)

endif

mini_dtls_pthread_LDADD = $(LDADD) -lpthread
rng_pthread_LDADD = $(LDADD) -lpthread

# These tests need gnulib for memmem()
resume_psk_CFLAGS = -DUSE_PSK
resume_psk_SOURCES = resume.c
resume_psk_LDADD = $(LDADD) ../gl/libgnu.la

resume_anon_CFLAGS = -DUSE_ANON
resume_anon_SOURCES = resume.c
resume_anon_LDADD = $(LDADD) ../gl/libgnu.la

resume_x509_CFLAGS = -DUSE_X509
resume_x509_SOURCES = resume.c
resume_x509_LDADD = $(LDADD) ../gl/libgnu.la

dtls1_2_cert_key_exchange_SOURCES = common-cert-key-exchange.c dtls1.2-cert-key-exchange.c common-cert-key-exchange.h
dtls1_0_cert_key_exchange_SOURCES = common-cert-key-exchange.c dtls1.0-cert-key-exchange.c common-cert-key-exchange.h
tls1_2_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls1.2-cert-key-exchange.c common-cert-key-exchange.h
tls1_1_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls1.1-cert-key-exchange.c common-cert-key-exchange.h
tls1_0_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls1.0-cert-key-exchange.c common-cert-key-exchange.h
ssl3_0_cert_key_exchange_SOURCES = common-cert-key-exchange.c ssl3.0-cert-key-exchange.c common-cert-key-exchange.h

if ENABLE_PKCS11
if !WINDOWS
noinst_LTLIBRARIES += libpkcs11mock1.la
libpkcs11mock1_la_SOURCES = pkcs11/pkcs11-mock.c pkcs11/pkcs11-mock.h pkcs11/pkcs11-mock-ext.h
libpkcs11mock1_la_LDFLAGS = -shared -rpath $(pkglibdir) -module -no-undefined -avoid-version
libpkcs11mock1_la_LIBADD =  ../gl/libgnu.la

pkcs11_cert_import_url_exts_SOURCES = pkcs11/pkcs11-cert-import-url-exts.c
pkcs11_cert_import_url_exts_DEPENDENCIES = libpkcs11mock1.la libutils.la

pkcs11_cert_import_url4_exts_SOURCES = pkcs11/pkcs11-cert-import-url4-exts.c
pkcs11_cert_import_url4_exts_DEPENDENCIES = libpkcs11mock1.la libutils.la

pkcs11_get_exts_SOURCES = pkcs11/pkcs11-get-exts.c
pkcs11_get_exts_DEPENDENCIES = libpkcs11mock1.la libutils.la

pkcs11_get_raw_issuer_exts_SOURCES = pkcs11/pkcs11-get-raw-issuer-exts.c
pkcs11_get_raw_issuer_exts_DEPENDENCIES = libpkcs11mock1.la libutils.la

pkcs11_import_url_privkey_SOURCES = pkcs11/pkcs11-import-url-privkey.c
pkcs11_import_url_privkey_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_import_url_privkey_LDADD = $(LDADD) $(LIBDL)

pkcs11_privkey_fork_SOURCES = pkcs11/pkcs11-privkey-fork.c
pkcs11_privkey_fork_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_fork_LDADD = $(LDADD) $(LIBDL)

pkcs11_privkey_export_SOURCES = pkcs11/pkcs11-privkey-export.c
pkcs11_privkey_export_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_export_LDADD = $(LDADD) $(LIBDL)

pkcs11_privkey_always_auth_SOURCES = pkcs11/pkcs11-privkey-always-auth.c
pkcs11_privkey_always_auth_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_always_auth_LDADD = $(LDADD) $(LIBDL)

pkcs11_privkey_safenet_always_auth_SOURCES = pkcs11/pkcs11-privkey-safenet-always-auth.c
pkcs11_privkey_safenet_always_auth_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_safenet_always_auth_LDADD = $(LDADD) $(LIBDL)

pkcs11_pkcs11_privkey_pthread_LDADD = $(LDADD) -lpthread

ctests += pkcs11-cert-import-url-exts pkcs11-get-exts pkcs11-get-raw-issuer-exts \
	pkcs11-cert-import-url4-exts pkcs11/pkcs11-chainverify pkcs11/pkcs11-get-issuer pkcs11/pkcs11-is-known \
	pkcs11/pkcs11-combo pkcs11/pkcs11-privkey pkcs11/pkcs11-pubkey-import-rsa pkcs11/pkcs11-pubkey-import-ecdsa \
	pkcs11-import-url-privkey pkcs11-privkey-fork pkcs11/pkcs11-ec-privkey-test \
	pkcs11-privkey-always-auth pkcs11-privkey-export pkcs11/pkcs11-import-with-pin \
	pkcs11/pkcs11-privkey-pthread pkcs11-privkey-safenet-always-auth


endif
endif

if ENABLE_OCSP
ctests += ocsp
endif

if ENABLE_DANE
ctests += dane
endif

rsa_illegal_import_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)

mini_alignment_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)
mini_alignment_LDADD = $(LDADD) $(NETTLE_LIBS)

if ENABLE_OPENSSL
if !ENABLE_FIPS140
ctests +=  openssl
openssl_LDADD = ../extra/libgnutls-openssl.la $(LDADD)
endif
endif

if ENABLE_OPENPGP
ctests += openpgp-auth openpgp-auth2 openpgp-keyring pgps2kgnu
endif

if HAVE_FORK
ctests += x509self x509dn anonself pskself dhepskself	\
	setcredcrash resume-x509 resume-psk resume-anon

if ENABLE_OPENPGP
ctests += openpgpself 
endif

endif

gc_CPPFLAGS = $(CPPFLAGS) \
	$(NETTLE_CFLAGS)			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/lib/minitasn1		\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl

mpi_CPPFLAGS = $(CPPFLAGS) \
	$(NETTLE_CFLAGS)			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/lib/minitasn1		\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl

atfork_CPPFLAGS = $(CPPFLAGS) \
	$(NETTLE_CFLAGS)			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/lib/minitasn1		\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl

pkcs12_s2k_CPPFLAGS = $(CPPFLAGS) \
	$(NETTLE_CFLAGS)			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/lib/minitasn1		\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl

name_constraints_merge_CPPFLAGS = $(CPPFLAGS) \
	$(NETTLE_CFLAGS)			\
	-I$(top_srcdir)/lib/includes		\
	-I$(top_builddir)/lib/includes		\
	-I$(top_srcdir)/lib/minitasn1		\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl

check_PROGRAMS = $(ctests)
dist_check_SCRIPTS = rfc2253-escape-test rsa-md5-collision/rsa-md5-collision.sh

if !WINDOWS
dist_check_SCRIPTS += fastopen.sh pkgconfig.sh starttls.sh starttls-ftp.sh starttls-smtp.sh \
	starttls-lmtp.sh starttls-pop3.sh starttls-nntp.sh starttls-sieve.sh sni-hostname.sh \
	gnutls-cli-save-data.sh
if ENABLE_DANE
dist_check_SCRIPTS += danetool.sh
endif

dist_check_SCRIPTS += gnutls-cli-self-signed.sh

endif

TESTS = $(ctests) $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT =						\
	CFLAGS="$(CFLAGS)"					\
	LC_ALL="C"						\
	LSAN_OPTIONS=suppressions=gnutls-asan.supp		\
	CAFILE=$(srcdir)/cert-tests/data/ca-certs.pem		\
	P11MOCKLIB1=$(abs_builddir)/.libs/libpkcs11mock1.so	\
	PKCS12_MANY_CERTS_FILE=$(srcdir)/cert-tests/data/pkcs12_5certs.p12	\
	PKCS12FILE=$(srcdir)/cert-tests/data/client.p12		\
	PKCS12PASSWORD=foobar					\
	PKCS12FILE_2=$(srcdir)/cert-tests/data/pkcs12_2certs.p12	\
	PKCS12PASSWORD_2=""					\
	PKCS12PATH=$(srcdir)/cert-tests/data/			\
	X509CERTDIR=$(srcdir)/x509cert-dir/			\
	GNUTLS_SYSTEM_PRIORITY_FILE=$(srcdir)/system.prio	\
	PSK_FILE=$(srcdir)/psk.passwd				\
	OPENSSL_ia32cap=0x00000000				\
	EXEEXT=$(EXEEXT)					\
	builddir="$(builddir)"					\
	top_builddir="$(top_builddir)"				\
	srcdir="$(srcdir)"

if WANT_TEST_SUITE
LOG_COMPILER = $(VALGRIND)
endif
