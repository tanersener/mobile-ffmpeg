#!/bin/sh

# Copyright (C) 2014 Nikos Mavrogiannopoulos
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
OUTFILE=cert-pss-privkey.$$.tmp
TMPFILE=cert-pss.$$.tmp
TMPFILE2=cert2-pss.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

# Create an RSA-PSS private key, restricted to the use with RSA-PSS
${VALGRIND} "${CERTTOOL}" --generate-privkey \
        --key-type rsa-pss --outfile "$OUTFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an RSA-PSS key"
	exit 1
fi

# check whether description is present
grep 'modulus:' ${OUTFILE}
if test $? != 0;then
	cat ${OUTFILE}
	echo "PKCS#8 file does not contain modulus text"
	exit 1
fi

for i in sha256 sha384 sha512;do
if test "${GNUTLS_FORCE_FIPS_MODE}" = 1 && test "$i" != sha384;then
	continue
fi

# Create an RSA-PSS private key, restricted to the use with RSA-PSS
${VALGRIND} "${CERTTOOL}" --generate-privkey --pkcs8 --password '' \
        --key-type rsa-pss --hash $i --outfile "$OUTFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an RSA-PSS key ($i)"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -k --password '' --infile "$OUTFILE" >/dev/null
rc=$?
if test "${rc}" != "0"; then
	echo "Could not read generated an RSA-PSS key ($i)"
	exit 1
fi

# Create an RSA-PSS certificate from an RSA-PSS private key
${VALGRIND} "${CERTTOOL}" --generate-self-signed \
	--pkcs8 --load-privkey "$OUTFILE" --password '' \
	--template "${srcdir}/templates/template-test.tmpl" \
	--outfile "${TMPFILE}" --hash $i
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an RSA-PSS certificate from an RSA-PSS key ($i)"
	exit 1
fi

rm -f "${TMPFILE}"

# Create an RSA-PSS certificate from an RSA-PSS private key, with
# mismatched parameters
for j in sha256 sha384 sha512;do
${VALGRIND} "${CERTTOOL}" --generate-self-signed \
	--pkcs8 --load-privkey "$OUTFILE" --password '' \
	--template "${srcdir}/templates/template-test.tmpl" \
	--outfile "${TMPFILE}" --hash $j
rc=$?

if test "$j" != "$j" && "${rc}" = "0"; then
	echo "Unexpectedly succeeded to generate an RSA-PSS certificate ($j != $i)"
	exit 1
fi
done
rm -f "${TMPFILE}"

# Create an RSA-PSS certificate from an RSA key
${VALGRIND} "${CERTTOOL}" --generate-certificate --key-type rsa-pss \
	    --load-privkey "${srcdir}/../../doc/credentials/x509/key-rsa.pem" \
	    --load-ca-privkey "${srcdir}/../../doc/credentials/x509/ca-key.pem" \
	    --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" \
	    --template "${srcdir}/templates/template-test.tmpl" \
	    --outfile "${TMPFILE}" --hash $i
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an RSA-PSS certificate $i"
	exit 1
fi

${CERTTOOL} -i --infile ${TMPFILE}|grep -i "Subject Public Key Algorithm: RSA-PSS"
if test $? != 0;then
	echo "Generated certificate is not RSA-PSS"
	cat ${TMPFILE}
	exit 1
fi

rm -f "${TMPFILE}"

# Create an RSA certificate from an RSA key, with wrong key-type, should fail
${VALGRIND} "${CERTTOOL}" --generate-certificate --key-type ecdsa \
	    --load-privkey "${srcdir}/../../doc/credentials/x509/key-rsa.pem" \
	    --load-ca-privkey "${srcdir}/../../doc/credentials/x509/ca-key.pem" \
	    --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" \
	    --template "${srcdir}/templates/template-test.tmpl" \
	    --outfile "${TMPFILE}"
rc=$?

if test "${rc}" = "0"; then
	echo "Succeeded with wrong key type"
	exit 1
fi

# Create an RSA certificate from an RSA key, and sign it with RSA-PSS
${VALGRIND} "${CERTTOOL}" --generate-certificate --rsa --sign-params rsa-pss \
	    --load-privkey "${srcdir}/../../doc/credentials/x509/key-rsa.pem" \
	    --load-ca-privkey "${srcdir}/../../doc/credentials/x509/ca-key.pem" \
	    --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" \
	    --template "${srcdir}/templates/template-test.tmpl" \
	    --outfile "${TMPFILE}" --hash $i
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an RSA-PSS certificate"
	exit 1
fi

${CERTTOOL} -i --infile ${TMPFILE}|tr -d '\r' > ${TMPFILE2}
grep -i 'Subject Public Key Algorithm: RSA$' ${TMPFILE2} >/dev/null
if test $? != 0;then
	echo "Generated certificate is not RSA"
	cat ${TMPFILE}
	exit 1
fi

grep -i "Signature Algorithm: RSA-PSS" ${TMPFILE2} 
if test $? != 0;then
	echo "Generated certificate is not signed with RSA-PSS"
	cat ${TMPFILE}
	exit 1
fi

grep -i "Signature Algorithm: RSA-PSS-${i}" ${TMPFILE2} 
if test $? != 0;then
	echo "Generated certificate is not signed with RSA-PSS-${i}"
	cat ${TMPFILE}
	exit 1
fi

rm -f "${TMPFILE}"
rm -f "${TMPFILE2}"

done

# Convert an RSA-PSS key to an RSA key
#

${VALGRIND} "${CERTTOOL}" --to-rsa --infile "${srcdir}/data/key-rsa-pss.pem" --outfile ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "Could not convert an RSA-PSS certificate"
	exit 1
fi

${DIFF} "${srcdir}/data/key-rsa-pss-raw.pem" ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "RSA-PSS decoding failed"
	exit ${rc}
fi

echo "RSA-PSS to RSA conversion was successful"

rm -f "${TMPFILE}"

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

datefudge "2012-11-22" \
${VALGRIND} "${CERTTOOL}" --verify --load-ca-certificate "${srcdir}/data/cert-rsa-pss.pem" --infile "${srcdir}/data/cert-rsa-pss.pem"
rc=$?

if test "${rc}" != "0"; then
	echo "There was an issue verifying the certificate"
	exit 1
fi

rm -f "${TMPFILE}"

exit 0
