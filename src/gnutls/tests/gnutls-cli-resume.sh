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

SERV="${SERV} -q"

. "${srcdir}/scripts/common.sh"

KEY1=${srcdir}/../doc/credentials/x509/key-rsa.pem
CERT1=${srcdir}/../doc/credentials/x509/cert-rsa.pem

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

run_server_test() {
	local priority=$1
	local id=$2
	local TMPFILE=resume.$$-$i.tmp

	"${CLI}" -p "${PORT}" 127.0.0.1 --logfile=${TMPFILE} --priority="${priority}" --resume --insecure </dev/null >/dev/null || \
		exit 1
	grep -H "* This is a resumed session" ${TMPFILE} ||
		exit 1

	rm -f ${TMPFILE}

	exit 0
}

echo "Checking whether session resumption works reliably under TLS1.3"
PRIORITY="NORMAL:-VERS-ALL:+VERS-TLS1.3"
WAITPID=""

i=0
while [ $i -lt 10 ]
do
	run_server_test "${PRIORITY}" $i &
	WAITPID="$WAITPID $!"
	i=`expr $i + 1`
done

for i in "$WAITPID";do
	wait $i
	test $? != 0 && exit 1
done

echo "Checking whether session resumption works reliably under TLS1.2"
PRIORITY="NORMAL:-VERS-ALL:+VERS-TLS1.2"
WAITPID=""

i=0
while [ $i -lt 10 ]
do
	run_server_test "${PRIORITY}" $i &
	WAITPID="$WAITPID $!"
	i=`expr $i + 1`
done

for i in "$WAITPID";do
	wait $i
	test $? != 0 && exit 1
done

kill ${PID}
wait

exit 0
