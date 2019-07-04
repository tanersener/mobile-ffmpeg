#!/bin/sh

# Copyright (C) 2017 Nikos Mavrogiannopoulos
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

srcdir="${srcdir:-.}"
SERV="${SERV:-../src/gnutls-serv${EXEEXT}}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
unset RETCODE

if ! test -x "${SERV}"; then
	exit 77
fi

if ! test -x "${CLI}"; then
	exit 77
fi

if test "${WINDIR}" != ""; then
	exit 77
fi 

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi


SERV="${SERV} -q"

. "${srcdir}/scripts/common.sh"

check_for_datefudge

echo "Checking whether server can utilize multiple keys"

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem
KEY2=${srcdir}/../doc/credentials/x509/key-ecc.pem
CERT2=${srcdir}/../doc/credentials/x509/cert-ecc.pem
KEY3=${srcdir}/../doc/credentials/x509/key-rsa-pss.pem
CERT3=${srcdir}/../doc/credentials/x509/cert-rsa-pss.pem
CAFILE=${srcdir}/../doc/credentials/x509/ca.pem
TMPFILE=outcert.$$.tmp

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA" --x509keyfile ${KEY1} --x509certfile ${CERT1} \
	--x509keyfile ${KEY2} --x509certfile ${CERT2} --x509keyfile ${KEY3} --x509certfile ${CERT3}
PID=$!
wait_server ${PID}

timeout 1800 datefudge "2017-08-9" \
"${CLI}" -p "${PORT}" localhost --x509cafile ${CAFILE} --priority "NORMAL:-KX-ALL:+ECDHE-RSA" </dev/null || \
	fail ${PID} "1. handshake with RSA should have succeeded!"

timeout 1800 datefudge "2017-08-9" \
"${CLI}" -p "${PORT}" localhost --x509cafile ${CAFILE} --priority "NORMAL:-KX-ALL:+ECDHE-ECDSA" </dev/null || \
	fail ${PID} "2. handshake with ECC should have succeeded!"

timeout 1800 datefudge "2017-08-9" \
"${CLI}" -p "${PORT}" localhost --x509cafile ${CAFILE} --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-SHA256" --save-cert ${TMPFILE} </dev/null || \
	fail ${PID} "3. handshake with RSA should have succeeded!"

cmp ${TMPFILE} ${CERT1}
if test $? != 0;then
	fail ${PID} "3. the certificate used by server was not the expected"
fi

timeout 1800 datefudge "2017-08-9" \
"${CLI}" -p "${PORT}" localhost --x509cafile ${CAFILE} --priority "NORMAL:-KX-ALL:+ECDHE-RSA:+SIGN-RSA-SHA256:+SIGN-RSA-PSS-RSAE-SHA256" --save-cert ${TMPFILE} </dev/null || \
	fail ${PID} "4. handshake with RSA should have succeeded!"


# check whether the server used the RSA-PSS certificate when we asked for RSA-PSS signature
timeout 1800 datefudge "2017-08-9" \
"${CLI}" -p "${PORT}" localhost --x509cafile ${CAFILE} --priority "NORMAL:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256" --save-cert ${TMPFILE} </dev/null || \
	fail ${PID} "4. handshake with RSA-PSS and SHA256 should have succeeded!"

cmp ${TMPFILE} ${CERT3}
if test $? != 0;then
	fail ${PID} "4. the certificate used by server was not the expected"
fi

kill ${PID}
wait

exit 0
