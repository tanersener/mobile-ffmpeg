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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

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

KEY1=${srcdir}/../doc/credentials/x509/example.com-key.pem
CERT1=${srcdir}/../doc/credentials/x509/example.com-cert.pem
CA1=${srcdir}/../doc/credentials/x509/ca.pem

echo "Checking SNI hostname in gnutls-cli"

OPTS="--sni-hostname example.com --verify-hostname example.com"
NOOPTS="--sni-hostname noexample.com --verify-hostname example.com"

eval "${GETPORT}"
launch_server $$ --echo --sni-hostname-fatal --sni-hostname example.com --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${OPTS} --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509cafile ${CA1} </dev/null >/dev/null || \
	fail ${PID} "1. handshake should have succeeded!"

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${NOOPTS} --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509cafile ${CA1} </dev/null >/dev/null && \
	fail ${PID} "2. handshake should have failed!"

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${OPTS} --priority "NORMAL" --x509cafile ${CA1} </dev/null >/dev/null || \
	fail ${PID} "3. handshake should have succeeded!"

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${NOOPTS} --priority "NORMAL" --x509cafile ${CA1} </dev/null >/dev/null && \
	fail ${PID} "4. handshake should have failed!"

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --sni-hostname example.com --priority "NORMAL" --x509cafile ${CA1} </dev/null >/dev/null && \
	fail ${PID} "5. handshake should have failed!"

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --sni-hostname example.com. --verify-hostname example.com. --priority "NORMAL" --x509cafile ${CA1} </dev/null >/dev/null || \
	fail ${PID} "6. handshake should have succeeded!"

kill ${PID}
wait

exit 0
