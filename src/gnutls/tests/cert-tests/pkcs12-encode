#!/bin/sh

# Copyright (C) 2004-2012 Free Software Foundation, Inc.
# Copyright (C) 2017 Red Hat, Inc.
#
# Author: Simon Josefsson
#
# This file is part of GnuTLS.
#
# GnuTLS is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
#
# GnuTLS is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=1"
fi

DIFF="${DIFF:-diff -b -B}"
DEBUG=""

TMPFILE=pkcs12.$$.tmp
TMPFILE_PEM=pkcs12.$$.pem.tmp

# test whether we can encode a certificate, a key and a CA
${VALGRIND} "${CERTTOOL}" --to-p12 --password 123456 --p12-name "my-key" --load-certificate "${srcdir}/../certs/cert-ecc256.pem" --load-privkey "${srcdir}/../certs/ecc256.pem" --load-ca-certificate "${srcdir}/../certs/ca-cert-ecc.pem" --outder --outfile $TMPFILE >/dev/null
rc=$?
if test ${rc} != 0; then
	echo "PKCS12 FATAL encoding 2"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --p12-info --inder --password 123456 --infile $TMPFILE >${TMPFILE_PEM} 2>/dev/null
rc=$?
if test ${rc} != 0; then
	echo "PKCS12 FATAL decrypting/decoding 2"
	exit 1
fi

grep "BEGIN ENCRYPTED PRIVATE KEY" ${TMPFILE_PEM} >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	exit ${rc}
fi

count=`grep -c "BEGIN CERTIFICATE" ${TMPFILE_PEM}`

if test "$count" != "2"; then
	echo "Only one certificate was included"
	exit 1
fi

# Check whether we can encode a PKCS#12 file with cert / key and CRL
${VALGRIND} "${CERTTOOL}" --to-p12 --password 123456 --pkcs-cipher aes-128 --p12-name "my-combo-key" --load-crl "${srcdir}/data/crl-demo1.pem" --load-certificate "${srcdir}/../certs/cert-ecc256.pem" --load-privkey "${srcdir}/../certs/ecc256.pem" --load-ca-certificate "${srcdir}/../certs/ca-cert-ecc.pem" --outder --outfile $TMPFILE >/dev/null
rc=$?
if test ${rc} != 0; then
	echo "PKCS12 FATAL encoding 3"
	exit 1
fi

# Check whether the contents are the expected ones
${VALGRIND} "${CERTTOOL}" --p12-info --inder --password 123456 --infile $TMPFILE >${TMPFILE_PEM} 2>/dev/null
rc=$?
if test ${rc} != 0; then
	echo "PKCS12 FATAL decrypting/decoding 3"
	exit 1
fi

grep "BEGIN CERTIFICATE" ${TMPFILE_PEM} >/dev/null 2>&1
if test "$?" != "0"; then
	exit ${rc}
fi

grep "BEGIN CRL" ${TMPFILE_PEM} >/dev/null 2>&1
if test "$?" != "0"; then
	exit ${rc}
fi

grep "BEGIN ENCRYPTED PRIVATE KEY" ${TMPFILE_PEM} >/dev/null 2>&1
if test "$?" != "0"; then
	exit ${rc}
fi

rm -f ${TMPFILE_PEM} $TMPFILE

exit ${ret}
