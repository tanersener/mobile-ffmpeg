#!/bin/sh

# Copyright (C) 2018 Red Hat, Inc.
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
TMPFILE=crl-inv.$$.pem.tmp
CAFILE=crl-inv-ca.$$.pem.tmp
CRLFILE=crl-inv-crl.$$.pem.tmp

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

echo "Checking whether connecting to a server but with an invalid CRL provided, returns the expected error"

cat <<__EOF__ >${TMPFILE}
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAxfNimQ1uOFXUSVCm0lBems4HpfLkW1Ykf5qLd9kdoHte7YAs
BHjFPaPSdXitYI36YMwqVcXT6RDJa0mcAV3QmMMxAnpKq7LIDVC9BNgjc7Dq5ou5
X2wNKrs3ygqg6HR87nJaw9TFqKetoP9mX37igBc2QWg5Fx6/Gem57hwD+mBEs+Hv
jd7q4wDlLaNS/165DBECr5dDUAIVr0bh0+1s/rDzIpjuq1qtN7b0C1rmWlsyphYu
aYm07X7x6hZcjvAoM3w5FLzbOnS6QrBeQOc2J6VBHqaHMKEVc+Dwt+Ggn0De0QCK
ucQRUCO5DQDUZnVLZhUpObvm1cBvQd5Db15IiwIDAQABAoIBACUSqhqkC0p9uJ5q
fnPRHYa8o24PCXmZrog/d3NgtE3EDUlJwfSscbRTpCzgBwiYTpYmZp9dx4xU2oQ/
avpOiayykdE2+hkiCJmFz4DCwhD+x1+aN4OJhwXDvnUWfIBMoME/pYQbq1Ek5j3K
1293IhB/SGgDjv2ngn7l6S6RDKWtYZry61oMEoVRy96FJ+88o5khlvfWE2zF2+M6
T2qFbzO29oq++cDSIlgm9eSN6gG5uzZcxqTapEvWrRdKZfEqcyGJuysQbShrASvI
GvJclewdnguBW2+X/bwABSEaG7AdPZJdfQJayk97gKJ8xpFZLY9auub7O/0z1CJi
lFsj4LECgYEA5TY8Z73ODtR87HEE3uUqiix4wPO4yJXWfZUwxNAyet2Jx5e5HYvL
iEkbZdadlKtSoPTnVSu6OZxhWZVBS5WoxxijBneDvh7I6gN8eVtch9EJVmJig6Eg
kHTo5Z2ZwheGe/RxB3ml3IT2IAdr5+QE6CfVBNA0fzVTItCLgO3YI/8CgYEA3RXZ
yskckcbCr1rceRmQ8CPbKg1bWGujLMpTILW0/Ii51PMredyG3E063G4kbMOFRmVj
eI5AFgZX7w5N4vjaf8PbOhsqrQvQ/UglB1fD0tLX8LgF9xwh7P1Y4VLHFMEGJUy1
PEGVCT0FIe2REGxAmyELaP8SSvW8fGjXJSp2K3UCgYBSlq5BOxTKJyo0D60Pm0cu
rkN8UtUcAVFdwqnl4Javyq9gaXzb9okJvD3Q/fmdnfWR5WyNNcpOA9jX7H2wfGZq
BqiHJf0kPfdqyoLJP3Ahx+IzbBPPFfmj01wvkA/c7ZkZhMRNSznGMWp1s/bfgTt7
Yw7QQy0HQPGJs9bwR8L/hQKBgQCXFvvEbjSsG12pYTsTN7mpo5d/4ajvgH//eDXf
QM7zVq1JLvYjTeaMX+s+Abe67NQEC/4ywWRiqOsnYGsyFkec0UjdKPu9TzoAHnHP
1tbpGVaiF+Fbw0ocH/fB5URQlqmQjB+/kkI8EguT6DsfMhvk6GxX0Rm7SL0LeMqv
h5lCkQKBgAR2U6cjbzJRhDyEOmUJH2keYHDwWUMx8ypvfhbPiPJyTC2sDcRrMrnO
WB3NtiB88aLFPjZ7sFZYE5plCESGkxK4Y21/UJHlw3I7X4JKYslE7dMq8Qzbv58r
23fZkHop4UJ1bHk7O4FRL3brU6KlIzZTOXzEeP+MRRehhwzkwpxf
-----END RSA PRIVATE KEY-----
-----BEGIN CERTIFICATE-----
MIIDiTCCAkGgAwIBAgIUEOtG5aJHVFm4ARA8uv4bJ/OqL4YwPQYJKoZIhvcNAQEK
MDCgDTALBglghkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMC
AUAwDzENMAsGA1UEAxMEQ0EtMDAgFw0xODA5MTgwNjQyMzdaGA85OTk5MTIzMTIz
NTk1OVowEzERMA8GA1UEAxMIc2VydmVyLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQDF82KZDW44VdRJUKbSUF6azgel8uRbViR/mot32R2ge17tgCwE
eMU9o9J1eK1gjfpgzCpVxdPpEMlrSZwBXdCYwzECekqrssgNUL0E2CNzsOrmi7lf
bA0quzfKCqDodHzuclrD1MWop62g/2ZffuKAFzZBaDkXHr8Z6bnuHAP6YESz4e+N
3urjAOUto1L/XrkMEQKvl0NQAhWvRuHT7Wz+sPMimO6rWq03tvQLWuZaWzKmFi5p
ibTtfvHqFlyO8CgzfDkUvNs6dLpCsF5A5zYnpUEepocwoRVz4PC34aCfQN7RAIq5
xBFQI7kNANRmdUtmFSk5u+bVwG9B3kNvXkiLAgMBAAGjdzB1MAwGA1UdEwEB/wQC
MAAwFAYDVR0RBA0wC4IJbG9jYWxob3N0MA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0O
BBYEFJVJTYVERYv5/qI31HwTDqATv4GRMB8GA1UdIwQYMBaAFBnn35UaLvLuW/YH
E3v2gKntMzNNMD0GCSqGSIb3DQEBCjAwoA0wCwYJYIZIAWUDBAIBoRowGAYJKoZI
hvcNAQEIMAsGCWCGSAFlAwQCAaIDAgFAA4IBAQCPVloFdhqdJqGjhxpl2Wv2ftD3
w+IeHSqURyCeijUCIOkqMlA3085nuoULiJ0p1ryi8rRWOvNjRsRQ30/lnLsxfmMh
oVR+g9uq0YZcFqkeRL5aDTrfJZWFeVSqXuuJvhyw1el5hs4bDSahMFB+dx2G+3zX
Ycd4Sq3sXDkdLnfD9GSeVvvbzAb7Z7qD1cdh1HaEnX2fsXT69czsFiaTgknr3Vxc
P0yFZVNCT360EVsduLkLWnCqZYVWWDFUlut7SOwhsYUx2ZOoM4RuBy+uDF2PM8BP
BkgYEHeWFA31nnwBNePyvWrAZ1DguOvnETSMB/+8zDX3+teNZNNdTVTQ6ypQ
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDgTCCAjmgAwIBAgIUE8klaC2IZj3Tr2/jEVEiJGj8piYwPQYJKoZIhvcNAQEK
MDCgDTALBglghkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMC
AUAwDzENMAsGA1UEAxMEQ0EtMDAgFw0xODA5MTgwNjQyMzdaGA85OTk5MTIzMTIz
NTk1OVowDzENMAsGA1UEAxMEQ0EtMDCCAVIwPQYJKoZIhvcNAQEKMDCgDTALBglg
hkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMCAUADggEPADCC
AQoCggEBALUJWYFxTq3vWG+hZq8KsRe0YRf5pqftxR21uZ7MSr25Muo7/s69toZG
7SaV1ZFp2n+Njm96nRLDqCc7cnaPLpKeMBFI84pQOYMdJs2mxs7wrBvejTBpxw3f
o1L2cJWznXZwvDQd+iz3qt62kF53tjpUzQ0Cqn6AMU961+H99Tq39iONcAvmTYeT
Bf+P4jhg3h5cOkdhsB4zrr0ek0OdgSdHiTIWvmYbEvizwhBc8pLOc007FkslqlQ5
b7Fplx/B+v/etqUoW7/742phxJhTjhRW75BWoCiQyhglwUfpDv0tXnMXousXdwaQ
Ao1EM1v/OCsYj/U2u10Bo/5y1q6Jjz8CAwEAAaNDMEEwDwYDVR0TAQH/BAUwAwEB
/zAPBgNVHQ8BAf8EBQMDBwQAMB0GA1UdDgQWBBQZ59+VGi7y7lv2BxN79oCp7TMz
TTA9BgkqhkiG9w0BAQowMKANMAsGCWCGSAFlAwQCAaEaMBgGCSqGSIb3DQEBCDAL
BglghkgBZQMEAgGiAwIBQAOCAQEAMAgvcHqmjz1Ox5USoup5pe6HWPKtOR5pVGX2
1zAk1wq7GoTKvo5QA6HtNR0ex1A2//XhklAKcqsIv1ELEh/3K/L0dEuaN4Zs784e
zaP0g/Ax6X3ClrHgARA4FA6MtaQblezj+7Zfc6cg1gKtfYleiOoK/Q+kk6JxOYAH
Lz9MF/6bZ8mYJQv8DURSp2p5NVWSEjbQV5IG2dw/eknZtbFaN5b+db3eVtrK0ZeS
l1e3hTwopCLNoh4qHUW/qKl0l1Gt7kPPxAsRReOxdcb1Pv73iuK7w5wbPyyWp0kM
FQj9tqRIMQZIer3gaURWG8OZfntCAvtlSSwc1PjwLBXO9ZvNBw==
-----END CERTIFICATE-----
__EOF__

cat <<__EOF__ >${CRLFILE}
-----BEGIN X509 CRL-----
MIIB/TCBtgIBATA9BgkqhkiG9w0BAQowMKANMAsGCWCGSAFlAwQCAaEaMBgGCSqG
SIb3DQEBCDALBglghkgBZQMEAgGiAwIBQDAPMQ0wCwYDVQQDEwRDQS0wFw0xODA5
MTgwNjQyMzdaFw0xOTA5MTgwNjQyMzdaMACgQTA/MB8GA1UdIwQYMBaAFBnn35Ua
LvLuW/YHE3v2gKntMzNNMBwGA1UdFAQVAhNboJ5dKaGvdv1Vo9o1XXTbeiMKMD0G
CSqGSIb3DQEBCjAwoA0wCwYJYIZIAWUDBAIBoRowGAYJKoZIhvcNAQEIMAsGCWCG
SAFlAwQCAaIDAgFAA4IBAQBgodBpVGTDHV4HBSgNPUnz7BH/BdRX1OPB8oYclDtv
l0xTzRR4qm/dMU3N3iH7vMk2y8U/TwD7NueyUnumt0vATTfjR2cle5lu2czksYsR
e4As9cI5cb4Sk+cf3/HyAVwnmZemTAA+cAJHkL6p7E+mSUoBVB6m8h8d6RH8jXmO
BXBE3z1xVITqahDdD6sLaR5jpnOtg/1nBAW8Hzr2p8tjEwhI8TCfZXbL9Q6fZtTr
apDrIx0D/G4hDKmmtQeY2q3RCOSJldg4YzUjjuhWs6BahHj9jDJpz02180ao7bda
eoNetNEqNvBvFvkO9gtgSzOzS34taiMpkIBwBbCNkm4p
-----END X509 CRL-----
__EOF__

cat <<__EOF__ >${CAFILE}
-----BEGIN CERTIFICATE-----
MIIDgTCCAjmgAwIBAgIUE8klaC2IZj3Tr2/jEVEiJGj8piYwPQYJKoZIhvcNAQEK
MDCgDTALBglghkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMC
AUAwDzENMAsGA1UEAxMEQ0EtMDAgFw0xODA5MTgwNjQyMzdaGA85OTk5MTIzMTIz
NTk1OVowDzENMAsGA1UEAxMEQ0EtMDCCAVIwPQYJKoZIhvcNAQEKMDCgDTALBglg
hkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMCAUADggEPADCC
AQoCggEBALUJWYFxTq3vWG+hZq8KsRe0YRf5pqftxR21uZ7MSr25Muo7/s69toZG
7SaV1ZFp2n+Njm96nRLDqCc7cnaPLpKeMBFI84pQOYMdJs2mxs7wrBvejTBpxw3f
o1L2cJWznXZwvDQd+iz3qt62kF53tjpUzQ0Cqn6AMU961+H99Tq39iONcAvmTYeT
Bf+P4jhg3h5cOkdhsB4zrr0ek0OdgSdHiTIWvmYbEvizwhBc8pLOc007FkslqlQ5
b7Fplx/B+v/etqUoW7/742phxJhTjhRW75BWoCiQyhglwUfpDv0tXnMXousXdwaQ
Ao1EM1v/OCsYj/U2u10Bo/5y1q6Jjz8CAwEAAaNDMEEwDwYDVR0TAQH/BAUwAwEB
/zAPBgNVHQ8BAf8EBQMDBwQAMB0GA1UdDgQWBBQZ59+VGi7y7lv2BxN79oCp7TMz
TTA9BgkqhkiG9w0BAQowMKANMAsGCWCGSAFlAwQCAaEaMBgGCSqGSIb3DQEBCDAL
BglghkgBZQMEAgGiAwIBQAOCAQEAMAgvcHqmjz1Ox5USoup5pe6HWPKtOR5pVGX2
1zAk1wq7GoTKvo5QA6HtNR0ex1A2//XhklAKcqsIv1ELEh/3K/L0dEuaN4Zs784e
zaP0g/Ax6X3ClrHgARA4FA6MtaQblezj+7Zfc6cg1gKtfYleiOoK/Q+kk6JxOYAH
Lz9MF/6bZ8mYJQv8DURSp2p5NVWSEjbQV5IG2dw/eknZtbFaN5b+db3eVtrK0ZeS
l1e3hTwopCLNoh4qHUW/qKl0l1Gt7kPPxAsRReOxdcb1Pv73iuK7w5wbPyyWp0kM
FQj9tqRIMQZIer3gaURWG8OZfntCAvtlSSwc1PjwLBXO9ZvNBw==
-----END CERTIFICATE-----
__EOF__

eval "${GETPORT}"
launch_server $$ --echo --x509keyfile ${TMPFILE} --x509certfile ${TMPFILE}
PID=$!
wait_server ${PID}

datefudge "2018-9-19" \
${VALGRIND} "${CLI}" -p "${PORT}" localhost --x509crlfile ${CRLFILE} --x509cafile ${CAFILE} >${TMPFILE} 2>&1 </dev/null && \
	fail ${PID} "1. handshake should have failed!"


kill ${PID}
wait

grep -E "Error setting the x509 CRL file: Error in the CRL verification.[[:space:]]*\$" ${TMPFILE}
if ! test $? = 0;then
	echo "Did not find the expected error code"
	cat ${TMPFILE}
	exit 1
fi

rm -f ${TMPFILE} ${CAFILE} ${CRLFILE}

exit 0
