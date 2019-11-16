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
launch_server $$ --echo --priority "NORMAL:+SHA256" --x509keyfile ${KEY} --x509certfile ${CERT}
PID=$!
wait_server ${PID}

#successful case, test whether the ciphers we disable below work
echo "Sanity testing"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-CIPHER-ALL:+AES-128-GCM:-GROUP-ALL:+GROUP-FFDHE2048 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage1: expected connection to succeed (1)"

datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-ALL:+VERS-TLS1.2:-CIPHER-ALL:+AES-128-CBC:+AES-256-CBC:-MAC-ALL:+SHA1 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage1: expected connection to succeed (2)"

cat <<_EOF_ > ${TMPFILE}
[overrides]

tls-disabled-cipher = aes-128-gcm
tls-disabled-cipher = aes-128-cbc
tls-disabled-mac = sha1
tls-disabled-group = group-ffdhe2048
_EOF_

GNUTLS_SYSTEM_PRIORITY_FILE=${TMPFILE}
export GNUTLS_DEBUG_LEVEL=3
export GNUTLS_SYSTEM_PRIORITY_FILE

echo "Testing TLS1.3"
echo " * sanity"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage2: expected connection to succeed (1)"

echo " * fallback to good options"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-CIPHER-ALL:+AES-128-GCM:+AES-256-GCM:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-FFDHE3072 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage2: expected connection to succeed (2)"

echo " * disabled cipher"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-CIPHER-ALL:+AES-128-GCM --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null && #>/dev/null &&
	fail ${PID} "stage2: expected connection to fail (1)"

echo " * disabled group"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-GROUP-ALL:+GROUP-FFDHE2048 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null &&
	fail ${PID} "stage2: expected connection to fail (2)"

echo "Testing TLS1.2"
echo " * sanity"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-ALL:+VERS-TLS1.2 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage3: expected connection to succeed (1)"

echo " * fallback to good options"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-ALL:+VERS-TLS1.2:-CIPHER-ALL:+AES-128-CBC:+AES-256-CBC:+AES-256-GCM:-MAC-ALL:+SHA1:+AEAD --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null ||
	fail ${PID} "stage3: expected connection to succeed (2)"

echo " * disabled cipher"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-ALL:+VERS-TLS1.2:-CIPHER-ALL:+AES-128-CBC --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null &&
	fail ${PID} "stage3: expected connection to fail (1)"

echo " * disabled MAC"
datefudge "2017-11-22" \
"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-ALL:+VERS-TLS1.2:-MAC-ALL:+SHA1 --verify-hostname localhost --x509cafile "${srcdir}/certs/ca-cert-ecc.pem" </dev/null >/dev/null &&
	fail ${PID} "stage3: expected connection to fail (2)"


kill ${PID}
wait

rm -f ${TMPFILE}

exit 0
