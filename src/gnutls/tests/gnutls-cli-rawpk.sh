#!/bin/sh

# Copyright (C) 2019 Tom Vrancken (dev@tomvrancken.nl)
#
# Author: Tom Vrancken
#         Nikos Mavrogiannopoulos
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
CERTFILE1=rawpk-script1.$$.pem.tmp
CERTFILE2=rawpk-script2.$$.pem.tmp

TMPFILE=rawpk-script.$$.log

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

echo "Checking whether we can connect with raw public-keys"

cat <<__EOF__ >${CERTFILE1}
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

cat <<__EOF__ >${CERTFILE2}
-----BEGIN PUBLIC KEY-----
MIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEAtfCQ/U13p076UUEggNm8
mI+bEilHrvKiyZpY/NZ2JYhmU+n0ChuH6XweXuiuW2gKR/Fl0vpGAxhugQAR3lMS
0XJZjg4z1gFP0BMeR0YYQuWe2Gfg24fUNPluHCf+dJMHAadIVKyUvVKFPDTwu6yU
lVafn0uoMme8b51yAwQAeXGOJnTFHSaRhuDWAokaTsW0kdAr5FSxMadSTVuDNSav
ic6qSUimJvPjT+URzpIjy/2MG8aj00At1MSj8sC/g5JuBvMKXe4mg6/Xd1YsP9gT
GhpfuBzwt+H4HDm4MzwNqdWNJ7pAhdcBOztqsScZJUqmGT742Fr0OjYYOFvHYLcx
SMD65huLlKTiQwg0HocVFA/VilLLV65cW+AUakr0YCXKyucPz4omVy/RINi663Bn
UyIR3pkOwbr71syuiU1S9cMnG0BxHsPmbvgaYttnCwp0cexB8MrJNJv5FcKLOGUL
MMCvSIT7elYwGN2p5gtAg1RLmyVvubTmkOAmxAgybgqHAgMBAAE=
-----END PUBLIC KEY-----

-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
__EOF__

trap "cleanup" EXIT QUIT

cleanup()
{
    kill ${PID} >/dev/null 2>&1
    rm -f ${CERTFILE1} ${CERTFILE2}

}

echo " * testing server X.509, client RAW"

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${CERTFILE1} --x509certfile ${CERTFILE1} --priority NORMAL:-CTYPE-CLI-ALL:+CTYPE-CLI-RAWPK --require-client-cert
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:-CTYPE-CLI-ALL:+CTYPE-CLI-RAWPK --no-ca-verification --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2}  >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:+CTYPE-CLI-ALL --no-ca-verification --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2}  >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL --no-ca-verification --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2}  >${TMPFILE} 2>&1 </dev/null
grep "Certificate is required" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake has unexpectedly succeeded. See ${TMPFILE} for details."
	exit 1
fi

kill ${PID}
wait

echo " * testing server RAW, client none"
eval "${GETPORT}"
launch_server $$ --echo --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2} --priority NORMAL:+CTYPE-SRV-RAWPK
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:-CTYPE-SRV-ALL:+CTYPE-SRV-RAWPK --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:+CTYPE-SRV-ALL --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if test $? = 0;then
	echo "Handshake has unexpectedly succeeded. See ${TMPFILE} for details."
	exit 1
fi


kill ${PID}
wait

echo " * testing server RAW, client RAW"
eval "${GETPORT}"
launch_server $$ --echo --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2} --priority NORMAL:+CTYPE-SRV-RAWPK:-CTYPE-CLI-ALL:+CTYPE-CLI-RAWPK --require-client-cert
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:-CTYPE-ALL:+CTYPE-SRV-RAWPK:+CTYPE-CLI-RAWPK --no-ca-verification --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2}  >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:+CTYPE-SRV-ALL:+CTYPE-CLI-ALL --no-ca-verification --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2}  >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if test $? = 0;then
	echo "Handshake has unexpectedly succeeded. See ${TMPFILE} for details."
	exit 1
fi


kill ${PID}
wait

echo " * testing server X.509+RAW, client none"

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${CERTFILE1} --x509certfile ${CERTFILE1} --rawpkkeyfile ${CERTFILE2} --rawpkfile ${CERTFILE2} --priority NORMAL:+CTYPE-SRV-RAWPK
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:-CTYPE-SRV-ALL:+CTYPE-SRV-RAWPK --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:-CTYPE-SRV-ALL:+CTYPE-SRV-X509 --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL:+CTYPE-SRV-ALL --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

${VALGRIND} "${CLI}" -p "${PORT}" localhost --priority NORMAL --no-ca-verification >${TMPFILE} 2>&1 </dev/null
grep "Handshake was completed" ${TMPFILE}
if ! test $? = 0;then
	echo "Handshake failed. See ${TMPFILE} for details."
	exit 1
fi

kill ${PID}
wait


rm -f ${TMPFILE} ${CERTFILE1} ${CERTFILE2}

exit 0
