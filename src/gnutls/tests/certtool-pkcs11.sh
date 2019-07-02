#!/bin/sh

# Copyright (C) 2016 Nikos Mavrogiannopoulos
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
P11TOOL="${P11TOOL:-../src/p11tool${EXEEXT}}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
SERV="${SERV:-../src/gnutls-serv${EXEEXT}}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
RETCODE=0

if ! test -x "${P11TOOL}"; then
	exit 77
fi

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -x "${SERV}"; then
	exit 77
fi

if ! test -x "${CLI}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute valgrind --leak-check=full"
fi

TMPFILE="verify-pkcs11.debug"
CERTTOOL_PARAM="--stdout-info"

if test "${WINDIR}" != ""; then
	exit 77
fi 

P11TOOL="${VALGRIND} ${P11TOOL} --batch"
SERV="${SERV} -q"

. ${srcdir}/scripts/common.sh

rm -f "${TMPFILE}"

exit_error () {
	echo "check ${TMPFILE} for additional debugging information"
	echo ""
	echo ""
	tail "${TMPFILE}"
	exit 1
}

check_for_datefudge

# $1: token
# $2: PIN
# $3: filename
# $4: label
write_ca_cert () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"
	label="$4"

	echo -n "* Writing the CA certificate... "
	${P11TOOL} ${ADDITIONAL_PARAM} --mark-ca --mark-trusted --no-mark-private --so-login --write --label "$label" --load-certificate "${filename}" "${token}" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

}

# $1: token
# $2: PIN
# $3: filename
write_ca_privkey () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Writing the CA private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label CA-key --load-privkey "${filename}" "${token}" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi
}

# $1: URL
# $2: cert file to verify
verify_certificate_test() {
	url=$1
	file=$2

	echo -n "* Verifying a certificate... "
	datefudge -s "2015-10-10" \
	$CERTTOOL ${ADDITIONAL_PARAM} --verify --load-ca-certificate "$url" --infile "$file" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo "failed $file with $url"
		exit_error
	fi
}

generate_cert() {
	url=$1

	echo -n "* Generating a certificate... "
	$CERTTOOL ${ADDITIONAL_PARAM} --generate-certificate --load-ca-certificate "$url" --load-ca-privkey "${srcdir}/testpkcs11-certs/ca.key" --load-privkey "${srcdir}/testpkcs11-certs/server.key" --template "${srcdir}/testpkcs11-certs/server-tmpl" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo "failed generation with $url"
		exit_error
	fi
}

generate_cert_with_key() {
	ca_url=$1
	ca_key_url=$2

	echo -n "* Generating a certificate (privkey in pkcs11)... "
	$CERTTOOL ${ADDITIONAL_PARAM} --generate-certificate --load-ca-certificate "${ca_url}" --load-ca-privkey "${ca_key_url}" --load-privkey "${srcdir}/testpkcs11-certs/server.key" --template "${srcdir}/testpkcs11-certs/server-tmpl" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo "failed generation with $url"
		exit_error
	fi
}

echo "Testing PKCS11 verification"

# erase SC

type="softhsm"

. "${srcdir}/testpkcs11.${type}"

export GNUTLS_PIN=12345678
export GNUTLS_SO_PIN=00000000

init_card "${GNUTLS_PIN}" "${GNUTLS_SO_PIN}"

# find token name
TOKEN=`${P11TOOL} ${ADDITIONAL_PARAM} --list-tokens pkcs11:token=Nikos|grep URL|grep token=GnuTLS-Test|sed 's/\s*URL\: //g'`

echo "* Token: ${TOKEN}"
if test "x${TOKEN}" = x; then
	echo "Could not find generated token"
	exit_error
fi

write_ca_cert "${TOKEN}" "${GNUTLS_PIN}" "${srcdir}/testpkcs11-certs/ca.crt" "CA"

verify_certificate_test "${TOKEN};object=CA;object-type=cert" "${srcdir}/testpkcs11-certs/server.crt"
verify_certificate_test "${TOKEN};object=CA;object-type=cert" "${srcdir}/testpkcs11-certs/client.crt"
generate_cert "${TOKEN};object=CA;object-type=cert"

write_ca_privkey "${TOKEN}" "${GNUTLS_PIN}" "${srcdir}/testpkcs11-certs/ca.key"

generate_cert_with_key "${TOKEN};object=CA;object-type=cert" "${TOKEN};object=CA-key;object-type=private"

if test ${RETCODE} = 0; then
	echo "* All tests succeeded"
fi
rm -f "${TMPFILE}"

exit 0
