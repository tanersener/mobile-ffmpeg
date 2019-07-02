#!/bin/sh

# Copyright (C) 2010-2016 Free Software Foundation, Inc.
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

echo "Checking whether saving OCSP response and cert succeeds"

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem
OCSP1=${srcdir}/ocsp-tests/response1.der

TMPFILE1=save-data1.$$.tmp
TMPFILE2=save-data2.$$.tmp

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${KEY1} --x509certfile ${CERT1} --ocsp-response=${OCSP1} --ignore-ocsp-response-errors -d 6
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --save-cert ${TMPFILE1} --save-ocsp ${TMPFILE2} </dev/null >/dev/null && \
	fail ${PID} "1. handshake should have failed!"


kill ${PID}
wait

if ! test -f ${TMPFILE1};then
	echo "Could not retrieve certificate"
	exit 1
fi

if ! test -f ${TMPFILE2};then
	echo "Could not retrieve OCSP response"
	exit 1
fi

rm -f ${TMPFILE1} ${TMPFILE2}

exit 0
