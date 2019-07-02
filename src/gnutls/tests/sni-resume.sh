#!/bin/sh

# Copyright (C) 2017 Thomas Klute
#
# Author: Thomas Klute
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

PRIORITY="NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:+ANON-ECDH"

echo "Checking if the SNI extension is parsed in gnutls-serv during" \
     "cache-based session resumption"

TMPFILE="servoutput.$$.tmp"

eval "${GETPORT}"
launch_server $$ --echo --priority ${PRIORITY} --sni-hostname-fatal \
	      --sni-hostname server.example.com --noticket 2>${TMPFILE}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --sni-hostname server.example.com \
	 --priority ${PRIORITY} </dev/null >/dev/null \
	 --resume \
    || fail ${PID} "connection and resumption should have succeeded!"

kill ${PID}
wait

ret=0
cat "${TMPFILE}"
# The --sni-hostname-fatal option rejects only clients which send a
# server name that does not match the expected one, not clients that
# do not send an SNI extension at all. Check if the server logged a
# missing extension.
if grep "client did not include SNI extension" "${TMPFILE}" >/dev/null; then
    ret=1
    echo "SNI data missing unexpectedly!"
fi
rm "${TMPFILE}"

exit ${ret}
