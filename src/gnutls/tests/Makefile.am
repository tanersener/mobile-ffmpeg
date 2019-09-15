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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>
#

SUBDIRS = . cert-tests slow

TESTS_ENVIRONMENT =

if WINDOWS
SUBDIRS += windows
endif

if WANT_TEST_SUITE
SUBDIRS += suite
endif

EXTRA_DIST = suppressions.valgrind eagain-common.h cert-common.h test-chains.h \
	ocsp-common.h cmocka-common.h virt-time.h \
	certs/ca-cert-ecc.pem  certs/cert-ecc256.pem  certs/cert-ecc521.pem \
	certs/cert-rsa-2432.pem certs/ecc384.pem certs/ecc.pem hex.h \
	certs/ca-ecc.pem certs/cert-ecc384.pem certs/cert-ecc.pem certs/ecc256.pem \
	certs/ecc521.pem certs/rsa-2432.pem x509cert-dir/ca.pem psk.passwd \
	certs/rawpk_priv.pem certs/rawpk_pub.pem \
	certs/ed25519.pem certs/cert-ed25519.pem \
	system.prio pkcs11/softhsm.h pkcs11/pkcs11-pubkey-import.c gnutls-asan.supp \
	rsa-md5-collision/README safe-renegotiation/README starttls-smtp.txt starttls-ftp.txt \
	starttls-lmtp.txt starttls-pop3.txt starttls-xmpp.txt starttls-nntp.txt starttls-sieve.txt \
	rsa-md5-collision/colliding-chain-md5-2.pem rsa-md5-collision/colliding-chain-md5-1.pem \
	ocsp-tests/certs/ocsp-amazon.com.der ocsp-tests/certs/chain-amazon.com.pem \
	ocsp-tests/certs/chain-amazon.com-unsorted.pem cipher-neg-common.c \
	ocsp-tests/certs/chain-akamai.com.pem ocsp-tests/certs/ocsp-akamai.com.der \
	tls13/ext-parse.h \
	certs-interesting/README.md certs-interesting/cert1.der certs-interesting/cert1.der.err \
	certs-interesting/cert2.der certs-interesting/cert2.der.err certs-interesting/cert3.der \
	certs-interesting/cert3.der.err certs-interesting/cert4.der certs-interesting/cert5.der \
	certs-interesting/cert5.der.err certs-interesting/cert6.der certs-interesting/cert6.der.err \
	certs-interesting/cert7.der certs-interesting/cert8.der \
	certs-interesting/cert9.der certs-interesting/cert10.der \
	certs-interesting/cert3.der.err certs-interesting/cert4.der \
	scripts/common.sh scripts/starttls-common.sh \
	rng-op.c x509sign-verify-common.h common-key-tests.h \
	ocsp-tests/certs/ca.key ocsp-tests/certs/ca.pem ocsp-tests/certs/ocsp-server.key ocsp-tests/certs/ocsp-server.pem ocsp-tests/response1.der \
	ocsp-tests/response2.der ocsp-tests/certs/ocsp_index.txt ocsp-tests/certs/ocsp_index.txt.attr \
	ocsp-tests/response1.pem ocsp-tests/response2.pem \
	ocsp-tests/certs/server_good.key ocsp-tests/certs/server_bad.key ocsp-tests/certs/server_good.template \
	ocsp-tests/certs/server_bad.template ocsp-tests/certs/ocsp-staple-unrelated.der ocsp-tests/suppressions.valgrind \
	data/listings-DTLS1.0 data/listings-SSL3.0 data/listings-TLS1.0 data/listings-TLS1.1 \
	data/listings-legacy1 data/listings-legacy2 data/listings-legacy3 data/listings-legacy4 \
	data/listings-old-SSL3.0-TLS1.1 data/listings-SSL3.0-TLS1.1 \
	p11-kit-trust-data/Example_Root_CA.p11-kit server-kx-neg-common.c \
	p11-kit-trust-data/Example_Root_CA.pem data/test1.cat data/test2.cat \
	data/test1.cat.data data/test2.cat.data data/test1.cat.out data/test2.cat.out \
	data/pkcs7-cat-ca.pem data/long.crl data/long.pem data/large-cert.pem \
	testpkcs11.pkcs15 testpkcs11.softhsm testpkcs11.sc-hsm testpkcs11-certs/ca.crt testpkcs11-certs/ca-tmpl \
	testpkcs11-certs/client.key testpkcs11-certs/server.crt testpkcs11-certs/server-tmpl \
	testpkcs11-certs/ca.key testpkcs11-certs/client.crt testpkcs11-certs/client-tmpl testpkcs11-certs/server.key \
	crt_type-neg-common.c

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
COMMON_DEPS_LDADD = $(LIBSOCKET) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBSECCOMP) $(LIBRT)
COMMON_LDADD = $(COMMON_GNUTLS_LDADD) $(COMMON_DEPS_LDADD)

LDADD = $(COMMON_GNUTLS_LDADD) \
	libutils.la \
	$(COMMON_DEPS_LDADD)

dane_LDADD = $(LDADD) ../libdane/libgnutls-dane.la
dane_strcodes_LDADD = $(LDADD) ../libdane/libgnutls-dane.la

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../lib/minitasn1
endif

noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = utils.h utils.c seccomp.c utils-adv.c
libutils_la_LIBADD = ../lib/libgnutls.la

indirect_tests = tls13/prf-early system-override-hash system-override-sig

ctests = tls13/supported_versions tls13/tls12-no-tls13-exts \
	tls13/post-handshake-with-cert tls13/post-handshake-without-cert \
	tls13/cookie tls13/key_share tls13/prf tls13/post-handshake-with-cert-ticket \
	tls12-rollback-detection tls11-rollback-detection \
	tls12-check-rollback-val tls11-check-rollback-val tls13/hello_random_value \
	tls13/post-handshake-with-psk tls13/post-handshake-with-cert-auto \
	tls13/anti_replay

ctests += tls13/hello_retry_request

ctests += tls13/psk-ext

ctests += tls13/key_update

ctests += tls13/key_update_multiple

ctests += tls13/key_limits

ctests += tls13/multi-ocsp

ctests += tls13/ocsp-client

ctests += tls13/change_cipher_spec

ctests += tls13-cipher-neg

ctests += tls13/no-psk-exts

ctests += tls13/psk-dumbfw

ctests += tls13-early-start

ctests += mini-record-2 simple gnutls_hmac_fast set_pkcs12_cred cert certuniqueid tls-neg-ext-key \
	 mpi certificate_set_x509_crl dn parse_ca x509-dn x509-dn-decode record-sizes \
	 hostname-check cve-2008-4989 pkcs12_s2k chainverify record-sizes-range \
	 crq_key_id x509sign-verify sign-verify cve-2009-1415 cve-2009-1416		\
	 tls10-server-kx-neg tls11-server-kx-neg tls12-server-kx-neg ssl30-server-kx-neg \
	 tls12-cipher-neg tls11-cipher-neg tls10-cipher-neg ssl30-cipher-neg \
	 crq_apis init_roundtrip pkcs12_s2k_pem dn2 tls12-rehandshake-cert-3 \
	 nul-in-x509-names x509_altname pkcs12_encode mini-x509	gnutls_session_set_id \
	 rng-fork mini-eagain-dtls resume-dtls empty_retrieve_function \
	 tls13-rehandshake-cert gnutls_ext_raw_parse handshake-large-cert \
	 x509cert x509cert-tl infoaccess mini-dtls-hello-verify sign-verify-ed25519-rfc8080 \
	 trustdb-tofu dtls-rehandshake-anon mini-alpn mini-dtls-large \
	 mini-termination mini-x509-cas mini-x509-2 pkcs12_simple tls-pthread \
	 mini-emsgsize-dtls chainverify-unsorted mini-overhead tls12-ffdhe \
	 mini-dtls-heartbeat mini-x509-callbacks key-openssl priorities priorities-groups	\
	 gnutls_x509_privkey_import gnutls_x509_crt_list_import time \
	 sign-verify-ext4 tls-neg-ext4-key resume-lifetime memset0 memset1 \
	 mini-dtls-srtp rsa-encrypt-decrypt mini-loss-time gnutls-strcodes \
	 mini-record mini-dtls-record handshake-timeout mini-record-range \
	 cert-status fips-mode-pthread rsa-psk global-init sec-params sign-verify-data \
	 fips-test fips-override-test mini-global-load name-constraints x509-extensions \
	 long-session-id mini-x509-callbacks-intr mini-dtls-lowmtu set_x509_key_file-late \
	 crlverify mini-dtls-discard init_fds mini-record-failure openconnect-dtls12 \
	 tls12-rehandshake-cert-2 custom-urls set_x509_key_mem set_x509_key_file \
	 tls12-rehandshake-cert-auto tls12-rehandshake-set-prio \
	 mini-chain-unsorted x509-verify-with-crl mini-dtls-mtu privkey-verify-broken \
	 mini-dtls-record-asym key-import-export priority-set priority-set2 \
	 pubkey-import-export sign-is-secure spki spki-abstract rsa-rsa-pss \
	 mini-dtls-fork dtls-pthread mini-key-material x509cert-invalid \
	 tls-ext-register tls-supplemental mini-dtls0-9 duplicate-extensions \
	 record-retvals mini-server-name tls-etm tls-force-etm x509-cert-callback alerts \
	 client-sign-md5-rep tls12-invalid-key-exchanges session-rdn-read \
	 tls13-cert-key-exchange x509-cert-callback-ocsp gnutls_ocsp_resp_list_import2 \
	 server-sign-md5-rep privkey-keygen mini-tls-nonblock no-signal pkcs7-gen dtls-etm \
	 x509sign-verify-rsa x509sign-verify-ecdsa x509sign-verify-gost \
	 mini-alignment oids atfork prf psk-file priority-init2 post-client-hello-change-prio \
	 status-request status-request-ok rfc7633-missing sign-verify-ext \
	 fallback-scsv pkcs8-key-decode urls dtls-rehandshake-cert rfc7633-ok \
	 key-usage-rsa key-usage-ecdhe-rsa mini-session-verify-function auto-verify \
	 record-timeouts mini-dtls-hello-verify-48 set-default-prio \
	 tls12-anon-upgrade global-init-override tlsext-decoding rsa-psk-cb \
	 rehandshake-switch-cert rehandshake-switch-cert-allow rehandshake-switch-cert-client \
	 rehandshake-switch-cert-client-allow handshake-versions dtls-handshake-versions \
	 dtls-max-record tls12-max-record alpn-server-prec ocsp-filename-memleak \
	 dh-params rehandshake-ext-secret pcert-list session-export-funcs \
	 handshake-false-start version-checks key-material-dtls key-material-set-dtls \
	 name-constraints-merge crl-basic crq-basic \
	 send-client-cert custom-urls-override hex rehandshake-switch-psk-id \
	 rehandshake-switch-srp-id base64 srpbase64 pkcs1-digest-info set_x509_key \
	 set_x509_key_file_der set_x509_pkcs12_key crt_apis tls12-cert-key-exchange \
	 tls11-cert-key-exchange tls10-cert-key-exchange ssl30-cert-key-exchange \
	 dtls12-cert-key-exchange dtls10-cert-key-exchange x509-cert-callback-legacy \
	 keylog-env ssl2-hello tlsfeature-ext dtls-rehandshake-cert-2 dtls-session-ticket-lost \
	 tlsfeature-crt dtls-rehandshake-cert-3 resume-with-false-start \
	 set_x509_key_file_ocsp client-fastopen rng-sigint srp rng-pthread \
	 safe-renegotiation/srn0 safe-renegotiation/srn1 safe-renegotiation/srn2 \
	 safe-renegotiation/srn3 safe-renegotiation/srn4 safe-renegotiation/srn5 \
	 rsa-illegal-import set_x509_ocsp_multi_invalid set_key set_x509_key_file_ocsp_multi2 \
	 set_x509_ocsp_multi_unknown set_x509_ocsp_multi_pem tls-ext-not-in-dtls \
	 set_key_utf8 set_x509_key_utf8 insecure_key handshake-large-packet \
	 client_dsa_key server_ecdsa_key tls-session-ext-register tls-session-supplemental \
	 multi-alerts naked-alerts pkcs7-cat-parse set_known_dh_params_x509 \
	 set_known_dh_params_anon set_known_dh_params_psk session-tickets-ok \
	 session-tickets-missing set_x509_key_file_legacy status-request-ext \
	 gnutls_x509_crt_sign gnutls_x509_crq_sign dtls-repro-20170915 \
	 rng-no-onload dtls1-2-mtu-check crl_apis cert_verify_inv_utf8 no-extensions \
	 hostname-check-utf8 pkcs8-key-decode-encrypted priority-mix pkcs7 \
	 send-data-before-handshake recv-data-before-handshake crt_inv_write \
	 x509sign-verify-error rng-op-nonce rng-op-random rng-op-key x509-dn-decode-compat \
	 ip-check mini-x509-ipaddr trust-store base64-raw random-art dhex509self \
	 dss-sig-val sign-pk-api tls-session-ext-override record-pad \
	 tls13-server-kx-neg gnutls_ext_raw_parse_dtls key-export-pkcs8 \
	 null_retrieve_function tls-record-size-limit tls-crt_type-neg \
	 resume-with-stek-expiration resume-with-previous-stek rawpk-api \
	 tls-record-size-limit-asym dh-compute ecdh-compute sign-verify-data-newapi \
	 sign-verify-newapi

if HAVE_SECCOMP_TESTS
ctests += dtls-with-seccomp tls-with-seccomp dtls-client-with-seccomp tls-client-with-seccomp
endif

if !DISABLE_SYSTEM_CONFIG
ctests += system-prio-file
endif

if HAVE_CMOCKA
CMOCKA_LDADD = $(COMMON_LDADD) $(CMOCKA_LIBS)
ctests += dtls-sliding-window ip-utils name-constraints-ip conv-utf8 str-unicode str-idna \
	tls10-prf tls12-prf gnutls_record_overhead eagain tls12-rehandshake-cert \
	eagain-auto-auth

gnutls_record_overhead_LDADD = $(CMOCKA_LDADD)
dtls_sliding_window_LDADD = $(CMOCKA_LDADD)
ip_utils_LDADD = $(CMOCKA_LDADD)
name_constraints_ip_LDADD = $(CMOCKA_LDADD)
conv_utf8_LDADD = $(CMOCKA_LDADD)
str_unicode_LDADD = $(CMOCKA_LDADD)
str_idna_LDADD = $(CMOCKA_LDADD)
tls10_prf_LDADD = $(CMOCKA_LDADD)
tls12_prf_LDADD = $(CMOCKA_LDADD)
eagain_LDADD = $(CMOCKA_LDADD)
eagain_auto_auth_LDADD = $(CMOCKA_LDADD)
tls12_rehandshake_cert_LDADD = $(CMOCKA_LDADD)

gnutls_record_overhead_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

ip_utils_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

endif

tls_pthread_LDADD = $(LDADD) -lpthread
fips_mode_pthread_LDADD = $(LDADD) -lpthread
dtls_pthread_LDADD = $(LDADD) -lpthread
rng_pthread_LDADD = $(LDADD) -lpthread

memset0_CFLAGS = -DCHAR=0x0
memset0_SOURCES = memset.c
memset0_LDADD = $(LDADD)

memset1_CFLAGS = -DCHAR=0xa
memset1_SOURCES = memset.c
memset1_LDADD = $(LDADD)

tls12_rollback_detection_CFLAGS = -DTLS12
tls12_rollback_detection_SOURCES = tls13/rnd-rollback-detection.c
tls12_rollback_detection_LDADD = $(LDADD) ../gl/libgnu.la

tls11_rollback_detection_CFLAGS = -DTLS11
tls11_rollback_detection_SOURCES = tls13/rnd-rollback-detection.c
tls11_rollback_detection_LDADD = $(LDADD) ../gl/libgnu.la

tls12_check_rollback_val_CFLAGS = -DTLS12
tls12_check_rollback_val_SOURCES = tls13/rnd-check-rollback-val.c
tls12_check_rollback_val_LDADD = $(LDADD) ../gl/libgnu.la

tls11_check_rollback_val_CFLAGS = -DTLS11
tls11_check_rollback_val_SOURCES = tls13/rnd-check-rollback-val.c
tls11_check_rollback_val_LDADD = $(LDADD) ../gl/libgnu.la

# These tests need gnulib for memmem()
tls12_resume_psk_CFLAGS = -DUSE_PSK -DTLS12
tls12_resume_psk_SOURCES = resume.c
tls12_resume_psk_LDADD = $(LDADD) ../gl/libgnu.la

tls12_resume_anon_CFLAGS = -DUSE_ANON -DTLS12
tls12_resume_anon_SOURCES = resume.c
tls12_resume_anon_LDADD = $(LDADD) ../gl/libgnu.la

tls12_resume_x509_CFLAGS = -DUSE_X509 -DTLS12
tls12_resume_x509_SOURCES = resume.c
tls12_resume_x509_LDADD = $(LDADD) ../gl/libgnu.la

tls13_resume_psk_CFLAGS = -DUSE_PSK -DTLS13
tls13_resume_psk_SOURCES = resume.c
tls13_resume_psk_LDADD = $(LDADD) ../gl/libgnu.la

tls13_resume_x509_CFLAGS = -DUSE_X509 -DTLS13
tls13_resume_x509_SOURCES = resume.c
tls13_resume_x509_LDADD = $(LDADD) ../gl/libgnu.la

dtls_repro_20170915_SOURCES = dtls-repro-20170915.c common-cert-key-exchange.c cert-repro-20170915.h
dtls12_cert_key_exchange_SOURCES = common-cert-key-exchange.c dtls12-cert-key-exchange.c common-cert-key-exchange.h
dtls10_cert_key_exchange_SOURCES = common-cert-key-exchange.c dtls10-cert-key-exchange.c common-cert-key-exchange.h
tls13_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls13-cert-key-exchange.c common-cert-key-exchange.h
tls12_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls12-cert-key-exchange.c common-cert-key-exchange.h
tls11_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls11-cert-key-exchange.c common-cert-key-exchange.h
tls10_cert_key_exchange_SOURCES = common-cert-key-exchange.c tls10-cert-key-exchange.c common-cert-key-exchange.h
ssl30_cert_key_exchange_SOURCES = common-cert-key-exchange.c ssl30-cert-key-exchange.c common-cert-key-exchange.h

if ENABLE_PKCS11
if !WINDOWS
noinst_LTLIBRARIES += libpkcs11mock1.la
libpkcs11mock1_la_SOURCES = pkcs11/pkcs11-mock.c pkcs11/pkcs11-mock.h pkcs11/pkcs11-mock-ext.h
libpkcs11mock1_la_LDFLAGS = -shared -rpath $(pkglibdir) -module -no-undefined -avoid-version
libpkcs11mock1_la_LIBADD =  ../gl/libgnu.la

noinst_LTLIBRARIES += libpkcs11mock2.la
libpkcs11mock2_la_SOURCES = pkcs11/pkcs11-mock2.c
libpkcs11mock2_la_LDFLAGS = -shared -rpath $(pkglibdir) -module -no-undefined -avoid-version
libpkcs11mock2_la_LIBADD =  ../gl/libgnu.la

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

pkcs11_token_raw_SOURCES = pkcs11/pkcs11-token-raw.c
pkcs11_token_raw_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_token_raw_LDADD = $(LDADD) $(LIBDL)

pkcs11_obj_raw_SOURCES = pkcs11/pkcs11-obj-raw.c
pkcs11_obj_raw_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_obj_raw_LDADD = $(LDADD) $(LIBDL)

pkcs11_import_url_privkey_caps_SOURCES = pkcs11/pkcs11-import-url-privkey.c
pkcs11_import_url_privkey_caps_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_import_url_privkey_caps_LDADD = $(LDADD) $(LIBDL)
pkcs11_import_url_privkey_caps_CFLAGS = -DALL_CAPS_URI

pkcs11_privkey_fork_SOURCES = pkcs11/pkcs11-privkey-fork.c
pkcs11_privkey_fork_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_fork_LDADD = $(LDADD) $(LIBDL)

pkcs11_privkey_fork_reinit_SOURCES = pkcs11/pkcs11-privkey-fork-reinit.c
pkcs11_privkey_fork_reinit_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_privkey_fork_reinit_LDADD = $(LDADD) $(LIBDL)

pkcs11_mechanisms_SOURCES = pkcs11/pkcs11-mechanisms.c
pkcs11_mechanisms_DEPENDENCIES = libpkcs11mock1.la libutils.la
pkcs11_mechanisms_LDADD = $(LDADD) $(LIBDL)

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
	pkcs11/pkcs11-privkey-pthread pkcs11/pkcs11-pin-func pkcs11/pkcs11-obj-import \
	pkcs11-privkey-fork-reinit pkcs11-mechanisms pkcs11-privkey-safenet-always-auth \
	pkcs11/pkcs11-rsa-pss-privkey-test pkcs11/tls-neg-pkcs11-key pkcs11/pkcs11-privkey-generate \
	pkcs11/gnutls_x509_crt_list_import_url pkcs11/gnutls_pcert_list_import_x509_file \
	pkcs11/pkcs11-eddsa-privkey-test \
	pkcs11-token-raw pkcs11-obj-raw

if P11KIT_0_23_11_API
ctests += pkcs11-import-url-privkey-caps
endif

endif
endif

if ENABLE_OCSP
ctests += ocsp
endif

if ENABLE_DANE
ctests += dane dane-strcodes
endif

rsa_illegal_import_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)

mini_alignment_CPPFLAGS = $(AM_CPPFLAGS) $(NETTLE_CFLAGS)
mini_alignment_LDADD = $(LDADD) $(NETTLE_LIBS)

if ENABLE_OPENSSL
ctests +=  openssl
openssl_LDADD = ../extra/libgnutls-openssl.la $(LDADD)
endif

if HAVE_FORK
ctests += x509self x509dn anonself pskself dhepskself	\
	setcredcrash tls12-resume-x509 tls12-resume-psk tls12-resume-anon \
	tls13-resume-x509 tls13-resume-psk tls13-early-data tls13-early-data-neg \
	resume-with-record-size-limit
endif

gc_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

mpi_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

atfork_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

pkcs12_s2k_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

name_constraints_merge_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

murmur3_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

tls13_anti_replay_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl	\
	$(NETTLE_CFLAGS)

if ENABLE_PKCS11
if !WINDOWS
ctests += tls13/post-handshake-with-cert-pkcs11 pkcs11/tls-neg-pkcs11-no-key
tls13_post_handshake_with_cert_pkcs11_DEPENDENCIES = libpkcs11mock2.la libutils.la
tls13_post_handshake_with_cert_pkcs11_LDADD = $(LDADD) $(LIBDL)
pkcs11_tls_neg_pkcs11_no_key_DEPENDENCIES = libpkcs11mock2.la libutils.la
pkcs11_tls_neg_pkcs11_no_key_LDADD = $(LDADD) $(LIBDL)
endif
endif

dist_check_SCRIPTS = rfc2253-escape-test rsa-md5-collision/rsa-md5-collision.sh systemkey.sh tls13/prf-early.sh

if !WINDOWS

#
# List of tests not available/functional under windows
#

dist_check_SCRIPTS += dtls/dtls dtls/dtls-resume #dtls/dtls-nb

indirect_tests += dtls-stress

dtls_stress_SOURCES = dtls/dtls-stress.c
dtls_stress_LDADD = $(COMMON_GNUTLS_LDADD) \
	$(COMMON_DEPS_LDADD)

dist_check_SCRIPTS += fastopen.sh pkgconfig.sh starttls.sh starttls-ftp.sh starttls-smtp.sh \
	starttls-lmtp.sh starttls-pop3.sh starttls-xmpp.sh starttls-nntp.sh starttls-sieve.sh \
	ocsp-tests/ocsp-tls-connection ocsp-tests/ocsp-must-staple-connection \
	ocsp-tests/ocsp-test cipher-listings.sh sni-hostname.sh server-multi-keys.sh \
	psktool.sh ocsp-tests/ocsp-load-chain gnutls-cli-save-data.sh gnutls-cli-debug.sh \
	sni-resume.sh ocsp-tests/ocsptool cert-reencoding.sh pkcs7-cat.sh long-crl.sh \
	serv-udp.sh logfile-option.sh gnutls-cli-resume.sh profile-tests.sh

if !DISABLE_SYSTEM_CONFIG
dist_check_SCRIPTS += system-override-sig-hash.sh system-override-versions.sh system-override-invalid.sh \
	system-override-curves.sh system-override-profiles.sh system-override-tls.sh \
	system-override-kx.sh
endif

dist_check_SCRIPTS += gnutls-cli-self-signed.sh gnutls-cli-invalid-crl.sh

if ENABLE_PKCS11
dist_check_SCRIPTS += p11-kit-trust.sh testpkcs11.sh certtool-pkcs11.sh

if HAVE_PKCS11_TRUST_STORE
if P11KIT_0_23_11_API
dist_check_SCRIPTS += p11-kit-load.sh
indirect_tests += pkcs11/list-tokens pkcs11/list-objects
endif
endif

endif
if ENABLE_DANE
dist_check_SCRIPTS += danetool.sh
endif

if ENABLE_TROUSERS
dist_check_SCRIPTS += tpmtool_test.sh
endif

else

TESTS_ENVIRONMENT += WINDOWS=1

win32_certopenstore_SOURCES = win-certopenstore.c
win32_certopenstore_LDADD = $(LDADD) -lcrypt32
ctests += win32-certopenstore

endif

cpptests =
if ENABLE_CXX
if HAVE_CMOCKA

cpptests += sanity-cpp

sanity_cpp_SOURCES = sanity-cpp.cpp
sanity_cpp_LDADD = $(CMOCKA_LDADD) ../lib/libgnutlsxx.la
sanity_cpp_CXXFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gl	\
	-I$(top_builddir)/gl
endif
endif

if !WINDOWS
indirect_tests += datefudge-check
endif

check_PROGRAMS = $(cpptests) $(ctests) $(indirect_tests)
TESTS = $(cpptests) $(ctests) $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT +=						\
	CC="$(CC)"						\
	CFLAGS="$(CFLAGS)"					\
	LC_ALL="C"						\
	LSAN_OPTIONS=suppressions=gnutls-asan.supp		\
	CAFILE=$(srcdir)/cert-tests/data/ca-certs.pem		\
	P11MOCKLIB1=$(abs_builddir)/.libs/libpkcs11mock1.so	\
	P11MOCKLIB2=$(abs_builddir)/.libs/libpkcs11mock2.so	\
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
	GNUTLS_TEST_SUITE_RUN=1					\
	builddir="$(builddir)"					\
	top_builddir="$(top_builddir)"				\
	libdir="$(libdir)"					\
	srcdir="$(srcdir)"

if ENABLE_SSL3
TESTS_ENVIRONMENT += ENABLE_SSL3=1
else
TESTS_ENVIRONMENT += ENABLE_SSL3=0
endif

if WANT_TEST_SUITE

LOG_COMPILER = $(VALGRIND)
endif

distclean-local:
	rm -rf softhsm-*.db softhsm-*.config *.tmp tmp-* x509-crt-list-import-url.config.db
