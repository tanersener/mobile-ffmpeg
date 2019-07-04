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
KEYFILE=eddsa-privkey.$$.tmp
TMPFILE=eddsa.$$.tmp
TMPFILE2=eddsa2.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	exit 77
fi

. "${srcdir}/../scripts/common.sh"

# Test certificate in draft-ietf-curdle-pkix-04
${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/cert-eddsa.pem" --outfile "${TMPFILE}"

if test $? != 0; then
	echo "There was an issue parsing the certificate"
	exit 1
fi

check_if_equal ${TMPFILE} "${srcdir}/data/cert-eddsa.pem" "Not After:"
if test $? != 0;then
	echo "Error in parsing EdDSA cert"
	exit 1
fi

# Test public key in draft-ietf-curdle-pkix-04
${VALGRIND} "${CERTTOOL}" --pubkey-info --infile "${srcdir}/data/pubkey-eddsa.pem" --outfile "${TMPFILE}"
if test $? != 0; then
	echo "Could not read an EdDSA public key"
	exit 1
fi

check_if_equal ${TMPFILE} "${srcdir}/data/pubkey-eddsa.pem"
if test $? != 0;then
	echo "Error in parsing EdDSA public key"
	exit 1
fi


# Create an RSA-PSS private key, restricted to the use with RSA-PSS
${VALGRIND} "${CERTTOOL}" --generate-privkey --pkcs8 --password '' \
        --key-type eddsa --outfile "$KEYFILE"

if test $? != 0; then
	echo "Could not generate an EdDSA key"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -k --password '' --infile "$KEYFILE"
if test $? != 0; then
	echo "Could not read generated an EdDSA key"
	exit 1
fi


# Create an EdDSA certificate from an EdDSA private key
${VALGRIND} "${CERTTOOL}" --generate-self-signed \
	--pkcs8 --load-privkey "$KEYFILE" --password '' \
	--template "${srcdir}/templates/template-test.tmpl" \
	--outfile "${TMPFILE}"

if test $? != 0; then
	echo "Could not generate an EdDSA certificate from an EdDSA key"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --verify --load-ca-certificate "${TMPFILE}" --infile "${TMPFILE}"
if test $? != 0; then
	echo "There was an issue verifying the generated certificate (1)"
	exit 1
fi

# Create an EdDSA certificate from an RSA key
${VALGRIND} "${CERTTOOL}" --generate-certificate --key-type eddsa \
	    --load-privkey ${KEYFILE} \
	    --load-ca-privkey "${srcdir}/../../doc/credentials/x509/ca-key.pem" \
	    --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" \
	    --template "${srcdir}/templates/template-test.tmpl" \
	    --outfile "${TMPFILE}" 2>/dev/null

if test $? != 0; then
	echo "Could not generate an EdDSA certificate $i"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --verify --load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" --infile "${TMPFILE}"
if test $? != 0; then
	echo "There was an issue verifying the generated certificate (2)"
	exit 1
fi

rm -f "${TMPFILE}" "${TMPFILE2}"
rm -f "${KEYFILE}"


check_for_datefudge

# Test certificate chain using Ed25519
datefudge "2017-7-6" \
${VALGRIND} "${CERTTOOL}" --verify-chain --infile ${srcdir}/data/chain-eddsa.pem

if test $? != 0; then
	echo "There was an issue verifying the Ed25519 chain"
	exit 1
fi


exit 0
