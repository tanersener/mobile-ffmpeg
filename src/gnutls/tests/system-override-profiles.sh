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
#

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

check_for_datefudge

CERT="${srcdir}/certs/cert-ecc256.pem"
KEY="${srcdir}/certs/ecc256.pem"

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL" --x509keyfile ${KEY} --x509certfile ${CERT}
PID=$!
wait_server ${PID}

# successful case, 224 bit min-profile, 256 bit key
cat <<_EOF_ > ${TMPFILE}
[overrides]

# 224 bits
min-verification-profile=medium
_EOF_

export GNUTLS_DEBUG_LEVEL=3
unset GNUTLS_SYSTEM_PRIORITY_FILE

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (1)"

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LOW --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (2)"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_MEDIUM --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (3)"

# failure case, 384 bit min-profile, 256 bit key
cat <<_EOF_ > ${TMPFILE}
[overrides]

min-verification-profile=ultra
_EOF_

unset GNUTLS_SYSTEM_PRIORITY_FILE

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null ||
	fail "expected connection to succeed (1)"

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LOW --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (1)"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_MEDIUM --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" --logfile ${TMPFILE2} </dev/null >/dev/null &&
	fail "expected connection to fail (2)"

kill ${PID}
wait

exit 0
