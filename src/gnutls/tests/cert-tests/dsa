#!/bin/sh

# Copyright (C) 2010-2012 Free Software Foundation, Inc.
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
SERV="${SERV:-../../src/gnutls-serv}"
CLI="${CLI:-../../src/gnutls-cli}"
DEBUG=""
unset RETCODE

CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

if test "${WINDIR}" != ""; then
	exit 77
fi 

SERV="${SERV} -q"

. "${srcdir}/../scripts/common.sh"

size=`${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/dsa-pubkey-1018.pem"|grep "Algorithm Secur"|cut -d '(' -f 2|cut -d ' ' -f 1`

if test "${size}" != "1024"; then
	echo "The prime size (${size}) doesn't match the expected: 1024"
	exit 1
fi


echo "Checking various DSA key sizes (port ${PORT})"

# DSA 1024 + TLS 1.0

echo "Checking DSA-1024 with TLS 1.0"

eval "${GETPORT}"
launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.0:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1" --x509certfile "${srcdir}/data/cert.dsa.1024.pem" --x509keyfile "${srcdir}/data/dsa.1024.pem"
PID=$!
wait_server "${PID}"

PRIO="--priority NORMAL:+DHE-DSS:+SIGN-DSA-SHA512:+SIGN-DSA-SHA384:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1"
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 1024 key and TLS 1.0!"

echo "Checking server DSA-1024 with client DSA-1024 and TLS 1.0"

#try with client key of 1024 bits (should succeed) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.1024.pem" --x509keyfile "${srcdir}/data/dsa.1024.pem" </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 1024 key and TLS 1.0!"

echo "Checking server DSA-1024 with client DSA-2048 and TLS 1.0"

#try with client key of 2048 bits (should fail) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.2048.pem" --x509keyfile "${srcdir}/data/dsa.2048.pem" </dev/null >/dev/null 2>&1 && \
	fail "${PID}" "Succeeded connection to a server with a client DSA 2048 key and TLS 1.0!"

echo "Checking server DSA-1024 with client DSA-3072 and TLS 1.0"

#try with client key of 3072 bits (should fail) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.3072.pem" --x509keyfile "${srcdir}/data/dsa.3072.pem" </dev/null >/dev/null 2>&1 && \
	fail "${PID}" "Succeeded connection to a server with a client DSA 3072 key and TLS 1.0!"

kill "${PID}"
wait

# DSA 1024 + TLS 1.2

echo "Checking DSA-1024 with TLS 1.2"

eval "${GETPORT}"
launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1" --x509certfile "${srcdir}/data/cert.dsa.1024.pem" --x509keyfile "${srcdir}/data/dsa.1024.pem"
PID=$!
wait_server "${PID}"

"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 1024 key and TLS 1.2!"

echo "Checking server DSA-1024 with client DSA-1024 and TLS 1.2"

#try with client key of 1024 bits (should succeed) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.1024.pem" --x509keyfile "${srcdir}/data/dsa.1024.pem" </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 1024 key and TLS 1.2!"

echo "Checking server DSA-1024 with client DSA-2048 and TLS 1.2"

#try with client key of 2048 bits (should succeed) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.2048.pem" --x509keyfile "${srcdir}/data/dsa.2048.pem" </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with a client DSA 2048 key and TLS 1.2!"

echo "Checking server DSA-1024 with client DSA-3072 and TLS 1.2"

#try with client key of 3072 bits (should succeed) 
"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure --x509certfile "${srcdir}/data/cert.dsa.3072.pem" --x509keyfile "${srcdir}/data/dsa.3072.pem" </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with a client DSA 3072 key and TLS 1.2!"

kill "${PID}"
wait

# DSA 2048 + TLS 1.0

#echo "Checking DSA-2048 with TLS 1.0"

#eval "${GETPORT}"
#launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.0" --x509certfile "${srcdir}/data/cert.dsa.2048.pem" --x509keyfile "${srcdir}/data/dsa.2048.pem"
#PID=$!
#wait_server "${PID}"

#"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null 2>&1 && \
# fail "${PID}" "Succeeded connection to a server with DSA 2048 key and TLS 1.0. Should have failed!"

#kill "${PID}"
#wait

# DSA 2048 + TLS 1.2
echo "Checking DSA-2048 with TLS 1.2"

eval "${GETPORT}"
launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1" --x509certfile "${srcdir}/data/cert.dsa.2048.pem" --x509keyfile "${srcdir}/data/dsa.2048.pem"
PID=$!
wait_server "${PID}"

"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 2048 key and TLS 1.2!"

kill "${PID}"
wait

# DSA 3072 + TLS 1.0

#echo "Checking DSA-3072 with TLS 1.0"

#launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.0" --x509certfile "${srcdir}/data/cert.dsa.3072.pem" --x509keyfile "${srcdir}/data/dsa.3072.pem"
#PID=$!
#wait_server "${PID}"
#
#"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null 2>&1 && \
#	fail "${PID}" "Succeeded connection to a server with DSA 3072 key and TLS 1.0. Should have failed!"
#
#kill "${PID}"
#wait

# DSA 3072 + TLS 1.2

echo "Checking DSA-3072 with TLS 1.2"

eval "${GETPORT}"
launch_server $$ --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1" --x509certfile "${srcdir}/data/cert.dsa.3072.pem" --x509keyfile "${srcdir}/data/dsa.3072.pem"
PID=$!
wait_server "${PID}"

"${CLI}" ${DEBUG} ${PRIO} -p "${PORT}" 127.0.0.1 --insecure </dev/null >/dev/null || \
	fail "${PID}" "Failed connection to a server with DSA 3072 key and TLS 1.2!"

kill "${PID}"
wait

exit 0
