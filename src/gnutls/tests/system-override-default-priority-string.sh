#!/bin/sh
# Copyright (C) 2019 Canonical, Ltd.
#
# Author: Dimitri John Ledkov
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
STOCK_PRIORITY="${GNUTLS_SYSTEM_PRIORITY_FILE:-./system.prio}"
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

export GNUTLS_DEBUG_LEVEL=3
KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

# Try whether a client connection with priority string None succeeds
export GNUTLS_SYSTEM_PRIORITY_FILE="${srcdir}/system-override-default-priority-string.none.config"
eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

export GNUTLS_SYSTEM_PRIORITY_FILE="${STOCK_PRIORITY}"
"${CLI}" -p "${PORT}" 127.0.0.1 --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (1)"
kill ${PID}
wait

# Try whether a client connection to an tls1.3 only server succeeds
export GNUTLS_SYSTEM_PRIORITY_FILE="${srcdir}/system-override-default-priority-string.only-tls13.config"
eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

export GNUTLS_SYSTEM_PRIORITY_FILE="${STOCK_PRIORITY}"
"${CLI}" -p "${PORT}" 127.0.0.1 --priority "NORMAL:-VERS-TLS1.3" --insecure --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (2)"

export GNUTLS_SYSTEM_PRIORITY_FILE="${STOCK_PRIORITY}"
"${CLI}" -p "${PORT}" 127.0.0.1 --priority "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3" --insecure --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (1)"

kill ${PID}
wait

# Check that a bad (empty) default-priority-string results in an built-one being used, when non-strict
export GNUTLS_SYSTEM_PRIORITY_FILE="${srcdir}/system-override-default-priority-string.bad.config"
unset GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID
eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

"${CLI}" -p "${PORT}" 127.0.0.1 --insecure --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (2)"

kill ${PID}
wait

exit 0
