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
TMPFILE=self-signed.$$.pem.tmp

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

check_for_datefudge

echo "Checking whether connecting to a self signed certificate returns the expected error"

cat <<__EOF__ >${TMPFILE}
-----BEGIN CERTIFICATE-----
MIIEXDCCAsSgAwIBAgIIWkcxay17JgYwDQYJKoZIhvcNAQELBQAwQjEaMBgGA1UE
AxMRdGVzdC1hbGwtZGVmYXVsdHMxCzAJBgNVBAYTAkFUMRcwFQYKCZImiZPyLGQB
GRYHYmVidC5kZTAeFw0xNzEyMzAwNjI1NDhaFw0xOTEyMjAwNjI1NTFaMEIxGjAY
BgNVBAMTEXRlc3QtYWxsLWRlZmF1bHRzMQswCQYDVQQGEwJBVDEXMBUGCgmSJomT
8ixkARkWB2JlYnQuZGUwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQCv
M1Y+Q4goCtHi3SLHTBQ14LS6NI4UbEa8YZFfaOfmOOufzwdNUntUkSA2PPS7mQ55
SN+Sdel1x4f4EjfxCWhj0j0Y26OmJS+wYNz3oOdoKThLq4Mn5SumO7mhU684mZTi
EP2qrxFeYvQqQBdjv8rfP2LJ+RsB/3CiwWdkx4qeudoSUCqzWo8e6K2ul0JJuk+Z
fvqkPpDl+cVTikmxNwqjAt4Ef9oiT1YjUIBUae+RCdNZEa6d2AhW+4bD+vl0Pci+
EBPzhLeR8iYuIEX66Tpv8AUvv412SuvVZbizGP5EDH4gkWtNWem9yNPCHA9rBqrC
6Nib4TPPLm1aN4mJyLdoQ1gD0STHcFADo+1H0JDywzxlgkks9cj5sQmApO7+AuGs
JoUDAp4g4LHnBw/H/5esVta5Pn7GThKwu7PRY0Y59ZKQrT5deXm9TeySdav+9wR/
5aiIZpAsAM5zWnN5qAP58Xl+pa0qN48GPcwmAsa4Zh9ehGhzR00MFHD3V8i0rFcC
AwEAAaNWMFQwDAYDVR0TAQH/BAIwADAUBgNVHREEDTALgglsb2NhbGhvc3QwDwYD
VR0PAQH/BAUDAwegADAdBgNVHQ4EFgQU0tGobgnLApQRxvbzIhOT2gAUQAYwDQYJ
KoZIhvcNAQELBQADggGBAHmqS1jOY5J9ad63aFXaei0lZhTnYCsFOGWuyLqZtz9K
21n0V8WVXeGmBjXkYNS3LCwPwFqKsp3vhsh4Hw5cyKkfQIri0HlWASYiJCPZxDLH
odVJSOPV65Q+gmhT/ltHK5CW4DJ2Gy82vPEFqw3+Kca28IJ0m2wr0FlhOCvnHUa7
GMS/+SdaMbsi1Eui0wUG/xWw8/2kY26IjhDJHrsTUjpYQ+vTy5oOjyq6Yf15Orjw
tJTwGgRcfoiBGhzMgTbUfFCO33L6f0u/WR/sI7DYDO/6JW1USnTrMuwEL6/jMNAw
QPl6irVOy/UwcIcLIBw8ta5cR8JVbhYuV7cUT9qDVwCOqotkwjDVsH2aztLLLr5d
ywMQXvXh2UI4jSujWf9vYY3F7GkDGy/cOwVprZoAe0mXwvuCvDyNZqXJTxKtq/w9
ZwOveNtNeXHOJljseNXLQPCfCcQ6mEUNjqwo2eDqH0OtJsQRN2CAUsn+YBnAALrv
P4J46RbB7bnzQ9kHiv6KuA==
-----END CERTIFICATE-----

-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
__EOF__

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${TMPFILE} --x509certfile ${TMPFILE}
PID=$!
wait_server ${PID}

datefudge "2018-1-1" \
${VALGRIND} "${CLI}" -p "${PORT}" localhost >${TMPFILE} 2>&1 </dev/null && \
	fail ${PID} "1. handshake should have failed!"


kill ${PID}
wait

grep -E "Status: The certificate is NOT trusted. The certificate issuer is unknown.[[:space:]]*\$" ${TMPFILE}
if ! test $? = 0;then
	echo "Did not find the expected error code"
	grep "Status:" ${TMPFILE}
	exit 1
fi

rm -f ${TMPFILE}

exit 0
