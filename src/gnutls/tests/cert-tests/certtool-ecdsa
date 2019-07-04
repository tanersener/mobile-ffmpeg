#!/bin/sh

# Copyright (C) 2014-2018 Nikos Mavrogiannopoulos
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
KEYFILE=ecdsa-privkey.$$.tmp
TMPFILE=ecdsa.$$.tmp

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

${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/cert-ecc256-full.pem" --outfile "${TMPFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "There was an issue parsing the certificate"
	exit 1
fi

check_if_equal ${TMPFILE} "${srcdir}/data/cert-ecc256-full.pem" "Not After:"
if test $? != 0;then
	echo "Error in parsing ECDSA cert"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --pubkey-info --infile "${srcdir}/data/pubkey-ecc256.pem" --outfile "${TMPFILE}"
rc=$?
if test "${rc}" != "0"; then
	echo "Could not read an ECDSA public key"
	exit 1
fi

check_if_equal ${TMPFILE} "${srcdir}/data/pubkey-ecc256.pem"
if test $? != 0;then
	echo "Error in parsing ECDSA public key"
	exit 1
fi


# Create an ECDSA
${VALGRIND} "${CERTTOOL}" --generate-privkey --pkcs8 --password '' \
        --ecdsa --outfile "$KEYFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate an ECDSA key"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -k --password '' --infile "$KEYFILE" >/dev/null
rc=$?
if test "${rc}" != "0"; then
	echo "Could not read generated an ECDSA key"
	exit 1
fi

rm -f "${TMPFILE}" "${KEYFILE}"

exit 0
