#!/bin/sh

# Copyright (C) 2017 Red Hat, Inc.
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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi

OUTFILE=out-pkcs7.$$.tmp
OUTFILE2=out2-pkcs7.$$.tmp

. ${srcdir}/../scripts/common.sh

check_for_datefudge

KEY="${srcdir}/../certs/ed25519.pem"
CERT="${srcdir}/../certs/cert-ed25519.pem"

# Test verification of saved file
FILE="${srcdir}/data/pkcs7-eddsa-sig.p7s"
${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-certificate "${CERT}" --infile "${FILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct verification failed"
	exit ${rc}
fi

# Test signing
FILE="signing"
${VALGRIND} "${CERTTOOL}" --p7-sign --load-privkey  "${KEY}" --load-certificate "${CERT}" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed"
	exit ${rc}
fi

FILE="signing-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${CERT}" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed verification"
	exit ${rc}
fi

#check extraction of embedded data in signature
FILE="signing-verify-data"
${VALGRIND} "${CERTTOOL}" --p7-verify --p7-show-data --load-certificate "${CERT}" --outfile "${OUTFILE2}" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed verification with data"
	exit ${rc}
fi

cmp "${OUTFILE2}" "${srcdir}/data/pkcs7-detached.txt"
rc=$?
if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 data detaching failed"
	exit ${rc}
fi

FILE="signing-time"
${VALGRIND} "${CERTTOOL}" --p7-detached-sign --p7-time --load-privkey  "${KEY}" --load-certificate "${CERT}" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing with time failed"
	exit ${rc}
fi

${VALGRIND} "${CERTTOOL}" --p7-info --infile "${OUTFILE}" >"${OUTFILE2}"
grep '1.2.840.113549.1.9.3: 06092a864886f70d010701' ${OUTFILE2} >/dev/null 2>&1
if test $? != 0;then
	echo "Content-Type was not set in attributes"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --p7-info <"${OUTFILE}"|grep "Signing time:" "${OUTFILE}" >/dev/null 2>&1
if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing with time failed. No time was found."
	exit ${rc}
fi

FILE="signing-time-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${CERT}" --load-data "${srcdir}/data/pkcs7-detached.txt" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing with time failed verification"
	exit ${rc}
fi

rm -f "${OUTFILE}"
rm -f "${OUTFILE2}"

exit 0
