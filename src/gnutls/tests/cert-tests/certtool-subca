#!/bin/sh

# Copyright (C) 2019 Red Hat, Inc.
#
# Author: Nikos Mavrogiannopoulos
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

# This is a reproducer for #767

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi

ROOT_CA_TMPL=root.ca.$$.tmp
SUB_CA_TMPL=sub.ca.$$.tmp
ROOT_PRIVKEY=root.key.$$.tmp
ROOT_CA_CERT=root.ca.cert.$$.tmp
CSR_FILE=csr.$$.tmp
OUTFILE=out3.$$.tmp

. ${srcdir}/../scripts/common.sh

cat >${ROOT_CA_TMPL} <<_EOF_
organization = "Example"
cn = "Root CA"
expiration_days = 700
ca
cert_signing_key
crl_signing_key
_EOF_

cat >${SUB_CA_TMPL} <<_EOF_
organization = "Example"
cn = "Example CA"
expiration_days = 350
crl_dist_points = "http://crl.example.com/Root_CA.crl"
ca
signing_key
cert_signing_key
crl_signing_key
path_len = 0
_EOF_

${CERTTOOL} --generate-privkey --key-type ecdsa --outfile ${ROOT_PRIVKEY} >/dev/null
if test $? != 0;then
	echo "Error generating privkey"
	exit 1
fi

${CERTTOOL} --generate-self-signed --load-privkey ${ROOT_PRIVKEY} --template ${ROOT_CA_TMPL} > ${ROOT_CA_CERT} 2>&1
if test $? != 0;then
	echo "Error generating root CA"
	exit 1
fi

grep "Digital signature" ${ROOT_CA_CERT} >/dev/null
if test $? = 0;then
	echo "root CA: found the digital signature flag although not specified!"
	exit 1
fi

${CERTTOOL} --generate-request --load-privkey ${ROOT_PRIVKEY} --template ${SUB_CA_TMPL} --outfile ${CSR_FILE}
if test $? != 0;then
	cat ${SUB_CA_TMPL}
	echo "Error generating csr"
	exit 1
fi

${CERTTOOL} --generate-certificate --load-ca-privkey ${ROOT_PRIVKEY} --load-ca-certificate ${ROOT_CA_CERT} --load-request ${CSR_FILE} --template ${SUB_CA_TMPL} >${OUTFILE} 2>&1
if test $? != 0;then
	echo "Error generating sub CA"
	exit 1
fi

grep "Digital signature" ${OUTFILE} >/dev/null
if test $? != 0;then
	echo "Cannot find the digital signature flag!"
	exit 1
fi

rm -f "${ROOT_PRIVKEY}" "${ROOT_CA_CERT}" "${CSR_FILE}" "${ROOT_CA_TMPL}" "${SUB_CA_TMPL}" "${OUTFILE}"

exit 0
