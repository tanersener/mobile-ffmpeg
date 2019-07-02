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

echo "Checking whether logfile option works."

KEY1=${srcdir}/../doc/credentials/x509/example.com-key.pem
CERT1=${srcdir}/../doc/credentials/x509/example.com-cert.pem
CA1=${srcdir}/../doc/credentials/x509/ca.pem
PSK=${srcdir}/psk.passwd

TMPFILE1=save-data1.$$.tmp
TMPFILE2=save-data2.$$.tmp

OPTS="--sni-hostname example.com --verify-hostname example.com"

eval "${GETPORT}"
launch_server $$ --echo --priority NORMAL:+ECDHE-PSK:+DHE-PSK:+PSK --pskpasswd=${PSK}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:+ECDHE-PSK:+DHE-PSK:+PSK --pskusername=jas --pskkey=9e32cf7786321a828ef7668f09fb35db </dev/null >${TMPFILE2}

kill ${PID}
wait

if test -f ${TMPFILE1};then
	echo "Logfile should not be created!"
	exit 1
fi
if ! test -s ${TMPFILE2};then
	echo "Stdout should not be empty!"
	exit 1
fi
if grep -q "Handshake was completed" ${TMPFILE2};then
	echo "Find the expected output!"
else
	echo "Cannot find the expected output!"
	exit 1
fi

rm -f ${TMPFILE1} ${TMPFILE2}

eval "${GETPORT}"
launch_server $$ --echo --priority NORMAL:+ECDHE-PSK:+DHE-PSK:+PSK --pskpasswd=${PSK}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --logfile ${TMPFILE1} --priority NORMAL:+ECDHE-PSK:+DHE-PSK:+PSK --pskusername=jas --pskkey=9e32cf7786321a828ef7668f09fb35db </dev/null >${TMPFILE2}

kill ${PID}
wait

if ! test -f ${TMPFILE1};then
	echo "Logfile shoule be created!"
	exit 1
fi
if test -s ${TMPFILE2};then
	echo "Stdout should be empty!"
	exit 1
fi

if grep -q "Handshake was completed" ${TMPFILE1}; then
	echo "Found the expected output!"
else
	echo "Cannot find the expected output!"
	exit 1
fi
rm -f ${TMPFILE1} ${TMPFILE2}


echo "x509 functionality test"
eval "${GETPORT}"
launch_server $$ --echo --sni-hostname-fatal --sni-hostname example.com --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${OPTS} --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509cafile ${CA1}  </dev/null >${TMPFILE2}
kill ${PID}
wait

if test -f ${TMPFILE1};then
        echo "Logfile should not be created!"
        exit 1
fi
if ! test -s ${TMPFILE2};then
        echo "Stdout should not be empty!"
        exit 1
fi
if grep -q "Handshake was completed" ${TMPFILE2};then
        echo "Find the expected output!"
else
        echo "Cannot find the expected output!"
        exit 1
fi

rm -f ${TMPFILE1} ${TMPFILE2}

eval "${GETPORT}"
launch_server $$ --echo --sni-hostname-fatal --sni-hostname example.com --x509keyfile ${KEY1} --x509certfile ${CERT1}
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 ${OPTS} --priority "NORMAL:-VERS-ALL:+VERS-TLS1.2" --x509cafile ${CA1} --logfile ${TMPFILE1} </dev/null >${TMPFILE2}
kill ${PID}
wait

if ! test -f ${TMPFILE1};then
       echo "Logfile shoule be created!"
       exit 1
fi
if test -s ${TMPFILE2};then
       echo "Stdout should be empty!"
       exit 1
fi

if grep -q "Handshake was completed" ${TMPFILE1}; then
       echo "Found the expected output!"
else
       echo "Cannot find the expected output!"
       exit 1
fi
rm -f ${TMPFILE1} ${TMPFILE2}

exit 0
