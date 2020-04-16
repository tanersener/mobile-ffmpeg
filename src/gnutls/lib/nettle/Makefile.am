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
# along with this program.  If not, see <https://www.gnu.org/licenses/>

include $(top_srcdir)/lib/common.mk

AM_CFLAGS += $(HOGWEED_CFLAGS) $(GMP_CFLAGS)

AM_CPPFLAGS = \
	-I$(srcdir)/int		\
	-I$(srcdir)/backport		\
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
	gnettle.h rnd-common.h prf.c \
	backport/cfb8.c backport/cfb8.h \
	backport/cmac.c backport/cmac.h \
	backport/xts.c backport/xts.h \
	rnd.c int/rsa-fips.h int/rsa-keygen-fips186.c int/provable-prime.c \
	int/dsa-fips.h int/dsa-keygen-fips186.c int/dsa-validate.c \
	int/tls1-prf.c int/tls1-prf.h \
	int/dsa-compute-k.c int/dsa-compute-k.h \
	int/ecdsa-compute-k.c int/ecdsa-compute-k.h \
	int/mpn-base256.c int/mpn-base256.h

if WINDOWS
libcrypto_la_SOURCES += sysrng-windows.c
else
if HAVE_GETENTROPY
libcrypto_la_SOURCES += sysrng-getentropy.c
else
if HAVE_KERN_ARND
libcrypto_la_SOURCES += sysrng-netbsd.c
else
libcrypto_la_SOURCES += sysrng-linux.c
endif
endif
endif

libcrypto_la_SOURCES += rnd-fuzzer.c

if ENABLE_FIPS140
libcrypto_la_SOURCES += rnd-fips.c int/drbg-aes-self-test.c \
	int/drbg-aes.c int/drbg-aes.h
endif

if ENABLE_GOST
libcrypto_la_SOURCES += \
	gost/nettle-write.h \
	gost/gost28147.c gost/gost28147.h gost/gost-wrap.c \
	gost/gosthash94.c gost/gosthash94.h gost/gosthash94-meta.c \
	gost/streebog.c gost/streebog.h gost/streebog-meta.c \
	gost/hmac-gosthash94.c gost/hmac-streebog.c gost/hmac-gost.h

libcrypto_la_SOURCES += \
	gost/bignum-le.c gost/bignum-le.h

if NEED_GOSTDSA
libcrypto_la_SOURCES += \
	gost/nettle-internal.h \
	gost/ecc-gost256cpa.c gost/ecc-gost256cpa-32.h  gost/ecc-gost256cpa-64.h\
	gost/ecc-gost512a.c gost/ecc-gost512a-32.h gost/ecc-gost512a-64.h \
	gost/ecc-internal.h gost/gmp-glue.h \
	gost/ecc-gostdsa-sign.c gost/ecc-gostdsa-verify.c \
	gost/gostdsa-sign.c gost/gostdsa-verify.c \
	gost/gostdsa.h gost/ecc-gost-curve.h gost/ecc-gost-hash.c
endif

libcrypto_la_SOURCES += \
	gost/gostdsa-mask.c gost/gostdsa-vko.c gost/gostdsa2.h

libcrypto_la_SOURCES += gost_keywrap.c
endif

if NEED_CURVE448
curve448_generated_headers = \
	curve448/ecc-curve448-32.h curve448/ecc-curve448-64.h

BUILT_SOURCES = $(curve448_generated_headers)
EXTRA_DIST = $(curve448_generated_headers) curve448/eccdata.stamp

noinst_PROGRAMS = curve448/eccdata$(EXEEXT)

curve448_eccdata_SOURCES = curve448/eccdata.c
curve448_eccdata_CFLAGS = $(GMP_CFLAGS)
curve448_eccdata_LDADD = $(GMP_LIBS) ../../gl/libgnu.la

curve448/eccdata.stamp: $(curve448_eccdata_SOURCES)
	$(AM_V_GEN)$(MAKE) $(AM_MAKEFLAGS) curve448/eccdata$(EXEEXT) && touch $@

curve448/ecc-curve448-32.h: curve448/eccdata.stamp
	$(AM_V_GEN)curve448/eccdata$(EXEEXT) curve448 38 6 32 > $@T && mv $@T $@

curve448/ecc-curve448-64.h: curve448/eccdata.stamp
	$(AM_V_GEN)curve448/eccdata$(EXEEXT) curve448 38 6 64 > $@T && mv $@T $@

libcrypto_la_SOURCES += \
	curve448/nettle-write.h curve448/gmp-glue.h curve448/gmp-glue.c

libcrypto_la_SOURCES += \
	curve448/sha3.c curve448/sha3.h curve448/sha3-256.c \
	curve448/sha3-permute.c curve448/sha3-internal.h \
	curve448/shake256.c

libcrypto_la_SOURCES += \
	curve448/ecc-internal.h \
	curve448/ecc-add-eh.c curve448/ecc-add-ehh.c curve448/ecc-dup-eh.c \
	curve448/ecc-eh-to-a.c curve448/ecc-mul-a-eh.c curve448/ecc-mul-g-eh.c \
	curve448/ecc-mul-m.c curve448/ecc-mod.c curve448/ecc-mod-arith.c \
	curve448/ecc-mod-inv.c \
	curve448/ecc-a-to-j.c \
	curve448/sec-tabselect.c curve448/cnd-copy.c curve448/sec-add-1.c \
	curve448/ecc-curve448.c $(curve448_genereated_headers) \
	curve448/curve448-eh-to-x.c curve448/curve448.h curve448/curve448-mul.c \
	curve448/curve448-mul-g.c

libcrypto_la_SOURCES += \
	curve448/eddsa.h curve448/eddsa-compress.c curve448/eddsa-decompress.c \
	curve448/eddsa-expand.c curve448/eddsa-hash.c curve448/eddsa-internal.h \
	curve448/eddsa-pubkey.c curve448/eddsa-sign.c curve448/eddsa-verify.c \
	curve448/ed448-shake256.c curve448/ed448-shake256-pubkey.c \
	curve448/ed448-shake256-sign.c curve448/ed448-shake256-verify.c
endif

if NEED_CHACHA
libcrypto_la_SOURCES += \
	chacha/chacha-core-internal.c chacha/chacha-crypt.c \
	chacha/chacha-internal.h chacha/chacha-poly1305.c \
	chacha/chacha-poly1305.h chacha/chacha-set-key.c \
	chacha/chacha-set-nonce.c chacha/chacha.h
endif
