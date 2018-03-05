#!/bin/sh

# Copyright (C) 2017 Karl Tarbe
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
# Test signing
FILE="signing-with-cert-list"
${VALGRIND} "${CERTTOOL}" --p7-sign --load-certificate "${srcdir}/data/pkcs7-chain.pem" --load-privkey "${srcdir}/data/pkcs7-chain-endcert-key.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed"
	exit ${rc}
fi

#test chain verification
FILE="signing-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-ca-certificate "${srcdir}/data/pkcs7-chain-root.pem" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed verification"
	exit ${rc}
fi

#check extraction of embedded data in signature
FILE="signing-cert-list-verify-data"
${VALGRIND} "${CERTTOOL}" --p7-verify --p7-show-data --load-ca-certificate "${srcdir}/data/pkcs7-chain-root.pem" --outfile "${OUTFILE2}" <"${OUTFILE}"
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

rm -f "${OUTFILE}"
rm -f "${OUTFILE2}"

exit 0
