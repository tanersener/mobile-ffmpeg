## Process this file with automake to produce Makefile.in
# Copyright (C) 2000-2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 3 of
# the License, or (at your option) any later version.
#
# The GnuTLS is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

ACLOCAL_AMFLAGS = -I ../m4 -I ../gl/m4

EXTRA_DIST = priority_options.gperf common.mk
BUILT_SOURCES = pkix_asn1_tab.c gnutls_asn1_tab.c priority_options.h

SUBDIRS = includes x509 auth ext algorithms extras accelerated

if ENABLE_MINITASN1
SUBDIRS += minitasn1
endif

localedir = $(datadir)/locale

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\"		\
	-I$(srcdir)/../gl			\
	-I$(builddir)/../gl			\
	-I$(builddir)/includes			\
	-I$(builddir)/x509			\
	-I$(srcdir)/includes			\
	-I$(builddir)/includes			\
	-I$(srcdir)/x509			\
	$(LIBTASN1_CFLAGS)			\
	$(P11_KIT_CFLAGS)

if !HAVE_LIBUNISTRING
SUBDIRS += unistring
AM_CPPFLAGS += -I$(srcdir)/unistring/ -I$(builddir)/unistring/
endif

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/minitasn1
endif

# Pkg-config script.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnutls.pc
DISTCLEANFILES = $(pkgconfig_DATA)

lib_LTLIBRARIES = libgnutls.la

SRP_COBJECTS = srp.c

PSK_COBJECTS = psk.c

COBJECTS = range.c record.c compress.c debug.c cipher.c gthreads.h handshake-tls13.c \
	mbuffers.c buffers.c handshake.c num.c errors.c dh.c kx.c cipher-cbc.c \
	priority.c hash_int.c cipher_int.c session.c db.c x509_b64.c	\
	hello_ext.c auth.c sslv2_compat.c datum.c session_pack.c mpi.c \
	pk.c cert-cred.c global.c constate.c anon_cred.c pkix_asn1_tab.c gnutls_asn1_tab.c	\
	mem.c fingerprint.c tls-sig.c ecc.c alert.c privkey_raw.c atomic.h	\
	system/certs.c system/threads.c system/fastopen.c system/sockets.c	\
	str-iconv.c system.c profiles.c profiles.h \
	str.c str-unicode.c str-idna.c state.c cert-cred-x509.c file.c supplemental.c	\
	random.c crypto-api.c crypto-api.h privkey.c pcert.c pubkey.c locks.c dtls.c \
	system_override.c crypto-backend.c verify-tofu.c pin.c tpm.c fips.c	\
	safe-memfuncs.c atfork.c atfork.h randomart.c \
	system-keys.h urls.c urls.h prf.c auto-verify.c dh-session.c \
	cert-session.c handshake-checks.c dtls-sw.c dh-primes.c openpgp_compat.c \
	crypto-selftests.c crypto-selftests-pk.c secrets.c extv.c extv.h \
	hello_ext_lib.c hello_ext_lib.h ocsp-api.c stek.c cert-cred-rawpk.c

if WINDOWS
COBJECTS += system/keys-win.c
else
COBJECTS += system/keys-dummy.c
endif

COBJECTS += tls13/encrypted_extensions.c tls13/encrypted_extensions.h \
	tls13/certificate_request.c tls13/certificate_request.h \
	tls13/certificate_verify.c tls13/certificate_verify.h \
	tls13-sig.c tls13-sig.h \
	tls13/finished.c tls13/finished.h \
	tls13/key_update.c tls13/key_update.h \
	tls13/hello_retry.c tls13/hello_retry.h \
	tls13/session_ticket.c tls13/session_ticket.h \
	tls13/certificate.c tls13/certificate.h \
	tls13/early_data.c tls13/early_data.h \
	tls13/post_handshake.c \
	tls13/psk_ext_parser.c tls13/psk_ext_parser.h \
	tls13/anti_replay.c tls13/anti_replay.h

if ENABLE_PKCS11
COBJECTS += pkcs11.c pkcs11x.c pkcs11_privkey.c pkcs11_write.c pkcs11_secret.c \
	pkcs11_int.c
endif

if ENABLE_NETTLE
SUBDIRS += nettle
endif

HFILES = abstract_int.h debug.h cipher.h	 \
	buffers.h errors.h gnutls_int.h dtls.h	 \
	handshake.h num.h algorithms.h		 \
	dh.h kx.h hash_int.h cipher_int.h	 \
	db.h auth.h hello_ext.h	handshake-defs.h \
	x509_b64.h sslv2_compat.h datum.h \
	mpi.h pk.h record.h		\
	constate.h global.h tls-sig.h mem.h	\
	session_pack.h str.h str_array.h		\
	state.h x509.h crypto-backend.h			\
	srp.h auth/srp_kx.h auth/srp_passwd.h	\
	file.h supplemental.h crypto.h random.h system.h\
	locks.h mbuffers.h ecc.h pin.h fips.h \
	priority_options.h secrets.h stek.h cert-cred.h

if ENABLE_PKCS11
HFILES += pkcs11_int.h pkcs11x.h
endif

libgnutls_la_SOURCES = $(HFILES) $(COBJECTS) $(SRP_COBJECTS)	\
	$(PSK_COBJECTS) \
	gnutls.asn pkix.asn libgnutls.map

libgnutls_la_LDFLAGS = -no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(COMMON_LINK_FLAGS)

libgnutls_la_LIBADD = ../gl/libgnu.la x509/libgnutls_x509.la \
	ext/libgnutls_ext.la \
	auth/libgnutls_auth.la algorithms/libgnutls_alg.la \
	extras/libgnutls_extras.la
thirdparty_libadd = $(LTLIBZ) $(LTLIBINTL) $(LIBSOCKET) $(LTLIBNSL) \
	$(P11_KIT_LIBS) $(LIB_SELECT)

if HAVE_LIBIDN2
thirdparty_libadd += $(LIBIDN2_LIBS)
endif

if HAVE_LIBUNISTRING
thirdparty_libadd += $(LIBUNISTRING)
else
libgnutls_la_LIBADD += unistring/libunistring.la
endif

if ENABLE_NETTLE
libgnutls_la_LIBADD += accelerated/libaccelerated.la
endif

if !WINDOWS
thirdparty_libadd += $(LTLIBPTHREAD)
endif

if NEEDS_LIBRT
thirdparty_libadd += $(LTLIBRT)
endif

if ENABLE_FIPS140
thirdparty_libadd += $(FIPS140_LIBS) $(LTLIBDL)
endif

if ENABLE_TROUSERS
thirdparty_libadd += $(LTLIBDL)
endif

if HAVE_LD_VERSION_SCRIPT
libgnutls_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libgnutls.map
else
libgnutls_la_LDFLAGS += -export-symbols-regex '^(gnutls|_gnutls).*'
endif

if ENABLE_MINITASN1
libgnutls_la_LIBADD += minitasn1/libminitasn1.la
else
thirdparty_libadd += $(LIBTASN1_LIBS)
endif

if ENABLE_NETTLE
thirdparty_libadd += $(NETTLE_LIBS) $(HOGWEED_LIBS) $(GMP_LIBS)
libgnutls_la_LIBADD += nettle/libcrypto.la
endif

if HAVE_LD_OUTPUT_DEF
libgnutls_la_LDFLAGS += -Wl,--output-def,libgnutls-$(DLL_VERSION).def
libgnutls-$(DLL_VERSION).def: libgnutls.la
defexecdir = $(libdir)
defexec_DATA = libgnutls-$(DLL_VERSION).def
DISTCLEANFILES += $(defexec_DATA)
endif

if WINDOWS
thirdparty_libadd += -lcrypt32
endif

if MACOSX
libgnutls_la_LDFLAGS += -framework Security -framework CoreFoundation
endif

libgnutls_la_LIBADD += $(thirdparty_libadd)

# C++ library

if ENABLE_CXX
libgnutlsxx_la_CPPFLAGS = -I$(top_srcdir)/includes -I$(top_builddir)/includes -I$(srcdir)/includes

AM_CXXFLAGS = \
	-I$(srcdir)/includes	\
	-I$(builddir)/includes

lib_LTLIBRARIES += libgnutlsxx.la

libgnutlsxx_la_SOURCES = gnutlsxx.cpp

libgnutlsxx_la_LDFLAGS = -no-undefined \
	-version-info $(CXX_LT_CURRENT):$(CXX_LT_REVISION):$(CXX_LT_AGE)

libgnutlsxx_la_LIBADD = libgnutls.la

endif

pkix_asn1_tab.c: $(srcdir)/pkix.asn
	-asn1Parser $(srcdir)/pkix.asn pkix_asn1_tab.c

gnutls_asn1_tab.c: $(srcdir)/gnutls.asn
	-asn1Parser $(srcdir)/gnutls.asn gnutls_asn1_tab.c

priority_options.h: $(srcdir)/priority_options.gperf
	-gperf --global-table -t $^ > $@-tmp \
	&& sed 's/^const struct priority_options_st \*/static const struct priority_options_st \*/' <$@-tmp >$@ \
	&& rm -f $@-tmp
