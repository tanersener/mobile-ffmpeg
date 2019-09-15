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

srcdir="${srcdir:-.}"
SERV="${SERV:-../src/gnutls-serv${EXEEXT}}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
TMPFILE=config.$$.tmp
TMPFILE2=log.$$.tmp
export GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID=1

if ! test -x "${SERV}"; then
	exit 77
fi

if ! test -x "${CLI}"; then
	exit 77
fi

if test "${WINDIR}" != ""; then
	exit 77
fi

. "${srcdir}/scripts/common.sh"

cat <<_EOF_ > ${TMPFILE}
[overrides]

disabled-version = tls1.2
disabled-version = tls1.3
_EOF_

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"
export GNUTLS_DEBUG_LEVEL=3

"${CLI}" --list|grep Protocols >${TMPFILE2}
cat ${TMPFILE2}
if grep "VERS-TLS1.2" ${TMPFILE2} || grep "VERS-TLS1.3" ${TMPFILE2};then
	echo "Found disabled protocol with --list"
	exit 1
fi

"${CLI}" --priority NORMAL --list|grep Protocols >${TMPFILE2}
cat ${TMPFILE2}
if grep "VERS-TLS1.2" ${TMPFILE2} || grep "VERS-TLS1.3" ${TMPFILE2};then
	echo "Found disabled protocol with --list --priority NORMAL"
	exit 1
fi

# Try whether a client connection with these protocols will succeed.

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

unset GNUTLS_SYSTEM_PRIORITY_FILE

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2:+VERS-TLS1.3" --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (1)"

kill ${PID}
wait

# Try whether a server connection with these protocols will succeed.

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL" --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

unset GNUTLS_SYSTEM_PRIORITY_FILE

"${CLI}" -p "${PORT}" 127.0.0.1 --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2:+VERS-TLS1.3" --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (2)"

kill ${PID}
wait

exit 0
