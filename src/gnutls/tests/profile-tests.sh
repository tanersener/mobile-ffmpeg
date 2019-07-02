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

# This program tests whether the profile keywords work as expected

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

CAFILE="./profile-ca.$$.tmp"
CERT="./profile-cert.$$.tmp"


echo "Testing with a 256 bit ECDSA key"

cat >${CAFILE} <<_EOF_
-----BEGIN CERTIFICATE-----
MIIBZjCCAQugAwIBAgIUT/9x+s6cBhBHWoZH5fBi9c0aBPswCgYIKoZIzj0EAwIw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzAxNTdaGA85OTk5MTIzMTIzNTk1
OVowDzENMAsGA1UEAxMEQ0EtMDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABI7d
qggkXNbYfXi5rMqdvvX26GJ02A63B5sueaS0w1LITLeMb0mhx4trpXMkJ3lr05lY
JCfr6sUTAlYLMBLZJ+ajQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUD
AwcGADAdBgNVHQ4EFgQUUkk7xPS5Uf53q8YLEhz5KGqeZH0wCgYIKoZIzj0EAwID
SQAwRgIhAKL/lPu6hOTwA/FfB+dMkkVeeZA+6CeXgbnxeA6HXy3bAiEAvO3+1VhR
RIHc3JBuIsLlrwaovXAZHgXNGV2WalixDHI=
-----END CERTIFICATE-----
_EOF_
cat >${CERT} <<_EOF_
-----BEGIN CERTIFICATE-----
MIIBnTCCAUOgAwIBAgIUUoqE4mD73XmLCryaMad6AXl6TjAwCgYIKoZIzj0EAwIw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzAxNTdaGA85OTk5MTIzMTIzNTk1
OVowEzERMA8GA1UEAxMIc2VydmVyLTEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC
AAScHgQMZCm5GLjGs64tN8hmK+KmDOTBU0fyqc9Tle6WjgFFBzPeHv8vLcrp5HTI
mNtKFNCaLN73r9h8xk3qG2pno3cwdTAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuC
CWxvY2FsaG9zdDAPBgNVHQ8BAf8EBQMDB4AAMB0GA1UdDgQWBBRpzYoZdeLYgscj
yokMBbda3FnghzAfBgNVHSMEGDAWgBRSSTvE9LlR/nerxgsSHPkoap5kfTAKBggq
hkjOPQQDAgNIADBFAiATJTdJ176UocB1BGDTTwJAuNKurPFZzlEaeYHS3tetXAIh
AP/RStdc8DV/AtHZOF1/FF3fB/tS3d+vb2f0QsTbcl5f
-----END CERTIFICATE-----
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIG5Gt+KTDxw5cevzwL0Sfo2AJZNeVtu3GHSnpICvsSiBoAoGCCqGSM49
AwEHoUQDQgAEnB4EDGQpuRi4xrOuLTfIZivipgzkwVNH8qnPU5Xulo4BRQcz3h7/
Ly3K6eR0yJjbShTQmize96/YfMZN6htqZw==
-----END EC PRIVATE KEY-----
_EOF_
KEY="${CERT}"

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL" --x509keyfile ${KEY} --x509certfile ${CERT}
PID=$!
wait_server ${PID}

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_VERY_WEAK --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (1)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LOW --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (2)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LEGACY --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (3)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_HIGH --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (4)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_ULTRA --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null &&
	fail ${PID} "expected connection to fail (1)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_FUTURE --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null &&
	fail ${PID} "expected connection to fail (2)"

kill ${PID}
wait


echo "Testing with a 384 bit ECDSA key"

cat >${CAFILE} <<_EOF_
-----BEGIN CERTIFICATE-----
MIIBojCCASigAwIBAgIUFMelLI8WwXyoyKjZGXXXcLb4N1EwCgYIKoZIzj0EAwMw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzA2MDNaGA85OTk5MTIzMTIzNTk1
OVowDzENMAsGA1UEAxMEQ0EtMDB2MBAGByqGSM49AgEGBSuBBAAiA2IABNxXKt1I
dpBTxQ5oefACUoUgdEwLNkbrjMeEYbB1Wz9d5Uk9nJPjQOGx85ct3FysauMxzBGy
BKnBEYViamZiffXu3zzNlIZY+tCbc3MUqs6q60CuNIw4UjakKhgD6II2MKNDMEEw
DwYDVR0TAQH/BAUwAwEB/zAPBgNVHQ8BAf8EBQMDBwYAMB0GA1UdDgQWBBQJ9QXM
rPF8/z2VviCfhSp2ezf1AjAKBggqhkjOPQQDAwNoADBlAjEA5nmuJqRQFLgHYnN5
MRmMfT+TvkLL+MPBo9lK8cbFzweV/PdySLRKNylOH4y70UyzAjBk3kFH7KC1AGMz
+A87+Rx+7BHOIdKIp91wx8LhMIdbeX9yi3w6YRsjHoLxKtJ8FYE=
-----END CERTIFICATE-----
_EOF_
cat >${CERT} <<_EOF_
-----BEGIN CERTIFICATE-----
MIIB2DCCAWCgAwIBAgIUJiHZy9J/MQzCJPjaP3Zy+JTXHgowCgYIKoZIzj0EAwMw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzA2MDNaGA85OTk5MTIzMTIzNTk1
OVowEzERMA8GA1UEAxMIc2VydmVyLTEwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAATP
agsLKT6MLGFsxWyBjDmyrfcAreBZtGDe9tS8jYItbM8y/ulvjCnwW/dwmVBe6UKX
n7WIJ7nxvp/j0k59TwpMxfpSn51NhiaViMQ4ZxA34qm+H3gUl8r1GC9I/EPTYe2j
dzB1MAwGA1UdEwEB/wQCMAAwFAYDVR0RBA0wC4IJbG9jYWxob3N0MA8GA1UdDwEB
/wQFAwMHgAAwHQYDVR0OBBYEFO2V2sn+n3Kj0sA2leiLp/RQDmt/MB8GA1UdIwQY
MBaAFAn1Bcys8Xz/PZW+IJ+FKnZ7N/UCMAoGCCqGSM49BAMDA2YAMGMCL37ZZOM0
fKI8jzlZRF64IOB/hVbvMD5WOMqFN/M8BjbPSywuRy9/JIq0KiFw3IKUAjAJZSsJ
fd8/9po81LJwyfUF/fTwPa7CNExb4BoDRtDDc7s/ciXI/13rxwkJnlAytwI=
-----END CERTIFICATE-----
-----BEGIN EC PRIVATE KEY-----
MIGlAgEBBDEAtrbWqGFyxd+qLlU0VHGvS5CpuAg0fPvODXzu8qHGREvxMYJL5d0I
YfU7emquAuq/oAcGBSuBBAAioWQDYgAEz2oLCyk+jCxhbMVsgYw5sq33AK3gWbRg
3vbUvI2CLWzPMv7pb4wp8Fv3cJlQXulCl5+1iCe58b6f49JOfU8KTMX6Up+dTYYm
lYjEOGcQN+Kpvh94FJfK9RgvSPxD02Ht
-----END EC PRIVATE KEY-----
_EOF_
KEY="${CERT}"

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL" --x509keyfile ${KEY} --x509certfile ${CERT}
PID=$!
wait_server ${PID}

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_VERY_WEAK --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (1)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LOW --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (2)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LEGACY --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (3)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_HIGH --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (4)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_ULTRA --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (5)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_FUTURE --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null &&
	fail ${PID} "expected connection to fail (1)"

kill ${PID}
wait

echo "Testing with a 521 bit ECDSA key"

cat >${CAFILE} <<_EOF_
-----BEGIN CERTIFICATE-----
MIIB7TCCAU6gAwIBAgIUW9MXlkeIARoHEeP+DmgMfSOh9xkwCgYIKoZIzj0EAwQw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzE4MDVaGA85OTk5MTIzMTIzNTk1
OVowDzENMAsGA1UEAxMEQ0EtMDCBmzAQBgcqhkjOPQIBBgUrgQQAIwOBhgAEASRD
p6ArQF3bkC7rMzUo6RGle3LCDVkrVrcS0vMRKz6D436g/yO0+om5Xbny/z3Weo4x
E8dat+dQp2sHurso6ByhAbm08MqxKUqaU4G69xvTYTOSMljDtx/3upsF955J5/CT
/F8czPBR9jebQZOCXWI0clpFSTGTYFnqHVlyTTwCgd87o0MwQTAPBgNVHRMBAf8E
BTADAQH/MA8GA1UdDwEB/wQFAwMHBgAwHQYDVR0OBBYEFI2SeRAmyVkAAEabKWfy
SREfJqJfMAoGCCqGSM49BAMEA4GMADCBiAJCAc8sUwRR5Q5u52YSdaEiHgnWlNTJ
nP7ckTAiSCEmhp2L8wdvG2274oTjvw3gbUHLc310AAoIvUcZfaXB6zooIpl9AkIB
NK1JHzm60+USUDxJoQngtl8KdM9jR9UmjZ5hVhd/k5FeNYbb6Z+kuIasE4SlnJnd
VIEgdnjXtlI3n052VLjDKg4=
-----END CERTIFICATE-----
_EOF_
cat >${CERT} <<_EOF_
-----BEGIN CERTIFICATE-----
MIICJDCCAYagAwIBAgIUTNrzhsX4+TV92p8tYrrUclDsYsUwCgYIKoZIzj0EAwQw
DzENMAsGA1UEAxMEQ0EtMDAgFw0xOTA1MjAxMzE4MDVaGA85OTk5MTIzMTIzNTk1
OVowEzERMA8GA1UEAxMIc2VydmVyLTEwgZswEAYHKoZIzj0CAQYFK4EEACMDgYYA
BAGAb9ToCqbQ8wImyiIN3Zf3T8WrwB/R28f0w8wq0W5a71FGayY0VU5exSBV7nnj
X8xFwUb+BpIVRQ4ZsryQCDDANACxXE3hwae59mqO9JhrTUQL7KyDaZ8W6KbACn8h
fYsOay/3ub0wdNdG8aJIcZzmrX1DNM0Jt/rW1d2nzuv6lZqCfqN3MHUwDAYDVR0T
AQH/BAIwADAUBgNVHREEDTALgglsb2NhbGhvc3QwDwYDVR0PAQH/BAUDAweAADAd
BgNVHQ4EFgQUv46ZnyF9oFn6yVCPl8WJ2InprhowHwYDVR0jBBgwFoAUjZJ5ECbJ
WQAARpspZ/JJER8mol8wCgYIKoZIzj0EAwQDgYsAMIGHAkIAh0/UdYPTSWmtTRNZ
d1VGCBW+Pw9aMkSTd8byWgle8+z1aQdZYQF46MHDuRC3zkooAYXPjbYCbLba5W/x
K1MVvfoCQThH3TCLj/Qci1788SNJ2bvN4bGe9m71cRhJWOXx5GRUHjvRJ5dttllq
dPzh992Fym1fGoyKne2xm172IG2LvTI0
-----END CERTIFICATE-----
-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIBZEu+h1ouDy17i0vGtm39PIrwWCGmjiQkCp1HnPSGod6SM2O3j4Mf
PH5pp8dPYx0LmHXTe+/P/oiIf128sSlsIGCgBwYFK4EEACOhgYkDgYYABAGAb9To
CqbQ8wImyiIN3Zf3T8WrwB/R28f0w8wq0W5a71FGayY0VU5exSBV7nnjX8xFwUb+
BpIVRQ4ZsryQCDDANACxXE3hwae59mqO9JhrTUQL7KyDaZ8W6KbACn8hfYsOay/3
ub0wdNdG8aJIcZzmrX1DNM0Jt/rW1d2nzuv6lZqCfg==
-----END EC PRIVATE KEY-----
_EOF_
KEY="${CERT}"

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL" --x509keyfile ${KEY} --x509certfile ${CERT}
PID=$!
wait_server ${PID}

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_VERY_WEAK --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (1)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LOW --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (2)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_LEGACY --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (3)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_HIGH --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (4)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_ULTRA --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (5)"

"${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:%PROFILE_FUTURE --verify-hostname localhost --x509cafile "${CAFILE}" </dev/null >/dev/null ||
	fail ${PID} "expected connection to succeed (6)"

kill ${PID}
wait

rm -f ${TMPFILE} ${CAFILE} ${CERT}

exit 0
