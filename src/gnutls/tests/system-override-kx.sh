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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

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

# We intentionally add stray spaces and tabs to check our parser
cat <<_EOF_ > ${TMPFILE}
[overrides]

tls-disabled-kx = dhe-dss
tls-disabled-kx = dhe-rsa
tls-disabled-kx = unknown
_EOF_

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"
export GNUTLS_DEBUG_LEVEL=3

# Try whether a client connection with a disabled KX algorithm will succeed.

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

unset GNUTLS_SYSTEM_PRIORITY_FILE

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-KX-ALL:+DHE-RSA --insecure --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (1)"

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-KX-ALL:+DHE-RSA --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (2)"

# test whether the unknown KX will be caught
GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID=1
export GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to succeed (3)"

unset GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID

kill ${PID}
wait

# Try whether a server connection with a disabled KX will succeed.

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

unset GNUTLS_SYSTEM_PRIORITY_FILE

"${CLI}" -p "${PORT}" 127.0.0.1 --priority "NORMAL:-KX-ALL:+DHE-RSA" --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (2)"

kill ${PID}
wait

exit 0
