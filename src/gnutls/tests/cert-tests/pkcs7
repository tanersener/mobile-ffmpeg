#!/bin/sh

# Copyright (C) 2015 Nikos Mavrogiannopoulos
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
TMPFILE=tmp-pkcs7.$$.tmp

. ${srcdir}/../scripts/common.sh

check_for_datefudge

if test "${ENABLE_GOST}" = "1" && test "${GNUTLS_FORCE_FIPS_MODE}" != "1"
then
	GOST_P7B="rfc4490.p7b"
else
	GOST_P7B=""
fi

for FILE in single-ca.p7b full.p7b openssl.p7b openssl-keyid.p7b $GOST_P7B; do
${VALGRIND} "${CERTTOOL}" --inder --p7-info --infile "${srcdir}/data/${FILE}"|grep -v "Signing time" >"${OUTFILE}"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 decoding failed"
	exit ${rc}
fi

${DIFF} "${OUTFILE}" "${srcdir}/data/${FILE}.out" >/dev/null
if test "$?" != "0"; then
	echo "${FILE}: PKCS7 decoding didn't produce the correct file"
	exit 1
fi
done

${VALGRIND} "${CERTTOOL}" --inder --p7-info --infile "${srcdir}/data/full.p7b" --outfile "${TMPFILE}" --no-text
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text pkcs7 info failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMPFILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text pkcs7 info failed 2"
	exit 1
fi

# check signatures

for FILE in full.p7b openssl.p7b openssl-keyid.p7b; do
# check validation with date prior to CA issuance
datefudge -s "2011-1-10" \
${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 verification succeeded with invalid date (1)"
	exit 1
fi

# check validation with date prior to intermediate cert issuance
datefudge -s "2011-5-28 08:38:00 UTC" \
${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 verification succeeded with invalid date (2)"
	exit 1
fi

# check validation with date after intermediate cert issuance
datefudge -s "2038-10-13" \
${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 verification succeeded with invalid date (3)"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 verification failed"
	exit ${rc}
fi
done


#check key purpose verification
for FILE in full.p7b; do

${VALGRIND} "${CERTTOOL}" --verify-purpose=1.3.6.1.5.5.7.3.1 --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 verification failed with key purpose"
	exit ${rc}
fi

${VALGRIND} "${CERTTOOL}" --verify-purpose=1.3.6.1.5.5.7.3.3 --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}" >"${OUTFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 verification succeeded with wrong key purpose"
	exit 2
fi

done

# check signature with detached data

FILE="detached.p7b"
${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 verification succeeded without providing detached data"
	exit 2
fi

${VALGRIND} "${CERTTOOL}" --inder --p7-verify --load-data "${srcdir}/data/pkcs7-detached.txt" --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${srcdir}/data/${FILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 verification failed"
	exit ${rc}
fi

# Test cert combination

FILE="p7-combined"

rm -f "${OUTFILE2}"
for i in cert-ecc256.pem cert-ecc521.pem cert-ecc384.pem cert-ecc.pem cert-rsa-2432.pem;do
	cat "${srcdir}/../certs"/$i >>"${OUTFILE2}"
done
${VALGRIND} "${CERTTOOL}" --p7-generate --load-certificate "${OUTFILE2}" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct generation failed"
	exit ${rc}
fi

${DIFF} "${OUTFILE}" "${srcdir}/data/p7-combined.out" >/dev/null
if test "$?" != "0"; then
	echo "${FILE}: PKCS7 generation didn't produce the correct file"
	exit 1
fi

# Test signing
FILE="signing"
${VALGRIND} "${CERTTOOL}" --p7-sign --load-privkey  "${srcdir}/../../doc/credentials/x509/key-rsa.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed"
	exit ${rc}
fi

FILE="signing-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed verification"
	exit ${rc}
fi

#check extraction of embedded data in signature
FILE="signing-verify-data"
${VALGRIND} "${CERTTOOL}" --p7-verify --p7-show-data --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --outfile "${OUTFILE2}" <"${OUTFILE}"
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

FILE="signing-detached"
${VALGRIND} "${CERTTOOL}" --p7-detached-sign --load-privkey  "${srcdir}/../../doc/credentials/x509/key-rsa.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing-detached failed"
	exit ${rc}
fi

FILE="signing-detached-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --load-data "${srcdir}/data/pkcs7-detached.txt" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing-detached failed verification"
	exit ${rc}
fi

# Test signing with broken algorithms
FILE="signing-broken"
${VALGRIND} "${CERTTOOL}" --hash md5 --p7-sign --load-privkey  "${srcdir}/../../doc/credentials/x509/key-rsa.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing-broken failed"
	exit ${rc}
fi

FILE="signing-verify-broken"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" <"${OUTFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "${FILE}: PKCS7 struct verification succeeded with broken algo"
	exit 1
fi

FILE="signing-time"
${VALGRIND} "${CERTTOOL}" --p7-detached-sign --p7-time --load-privkey  "${srcdir}/../../doc/credentials/x509/key-rsa.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
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
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" --load-data "${srcdir}/data/pkcs7-detached.txt" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing with time failed verification"
	exit ${rc}
fi

FILE="rsa-pss-signing"
${VALGRIND} "${CERTTOOL}" --p7-sign --load-privkey  "${srcdir}/../../doc/credentials/x509/key-rsa-pss.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa-pss.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed"
	exit ${rc}
fi

FILE="rsa-pss-signing-verify"
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa-pss.pem" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 struct signing failed verification"
	exit ${rc}
fi

# Test BER encoding, see RFC 4134 Section 4.5
# SHA1 signature, so --verify-allow-broken
FILE="rfc4134-4.5"
${VALGRIND} "${CERTTOOL}" --p7-verify --verify-allow-broken --load-ca-certificate "${srcdir}/data/rfc4134-ca-rsa.pem" --infile "${srcdir}/data/rfc4134-4.5.p7b" --inder
rc=$?

if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 BER parsing/decoding failed"
	exit ${rc}
fi

if test "x$ENABLE_GOST" = "x1"  && test "x${GNUTLS_FORCE_FIPS_MODE}" != "x1"
then
	FILE="gost01-signing"
	${VALGRIND} "${CERTTOOL}" --p7-sign --load-privkey  "${srcdir}/../../doc/credentials/x509/key-gost01.pem" --load-certificate "${srcdir}/../../doc/credentials/x509/cert-gost01.pem" --infile "${srcdir}/data/pkcs7-detached.txt" >"${OUTFILE}"
	rc=$?

	if test "${rc}" != "0"; then
		echo "${FILE}: PKCS7 struct signing failed"
		exit ${rc}
	fi

	FILE="gost01-signing-verify"
	${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-gost01.pem" <"${OUTFILE}"
	rc=$?

	if test "${rc}" != "0"; then
		echo "${FILE}: PKCS7 struct signing failed verification"
		exit ${rc}
	fi
fi

rm -f "${OUTFILE}"
rm -f "${OUTFILE2}"
rm -f "${TMPFILE}"

exit 0
