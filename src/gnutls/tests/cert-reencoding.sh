#!/bin/sh

# Test case: Try to establish TLS connections with gnutls-cli and
# check the validity of the server certificate via OCSP
#
# Copyright (C) 2016 Thomas Klute
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
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
OCSPTOOL="${OCSPTOOL:-../src/ocsptool${EXEEXT}}"
GNUTLS_SERV="${GNUTLS_SERV:-../src/gnutls-serv${EXEEXT}}"
GNUTLS_CLI="${GNUTLS_CLI:-../src/gnutls-cli${EXEEXT}}"
DIFF="${DIFF:-diff}"
SERVER_CERT_FILE="cert.$$.pem.tmp"
SERVER_KEY_FILE="key.$$.pem.tmp"
CLIENT_CERT_FILE="cli-cert.$$.pem.tmp"
CLIENT_KEY_FILE="cli-key.$$.pem.tmp"
CA_FILE="ca.$$.pem.tmp"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -x "${OCSPTOOL}"; then
	exit 77
fi

if ! test -x "${GNUTLS_SERV}"; then
	exit 77
fi

if ! test -x "${GNUTLS_CLI}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi

export TZ="UTC"

. "${srcdir}/scripts/common.sh"

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT

# Port to use for OCSP server, must match the OCSP URI set in the
# server_*.pem certificates
eval "${GETPORT}"

# Check for OpenSSL
OPENSSL=`which openssl`
if ! test -x "${OPENSSL}"; then
    echo "You need openssl to run this test."
    exit 77
fi

# Check for datefudge
TSTAMP=`datefudge "2006-09-23" date -u +%s || true`
if test "$TSTAMP" != "1158969600"; then
    echo $TSTAMP
    echo "You need datefudge to run this test."
    exit 77
fi

SERVER_PID=""
TLS_SERVER_PID=""
stop_servers ()
{
	test -z "${SERVER_PID}" || kill "${SERVER_PID}"
	rm -f "${SERVER_CERT_FILE}"
	rm -f "${SERVER_KEY_FILE}"
	rm -f "${CLIENT_CERT_FILE}"
	rm -f "${CLIENT_KEY_FILE}"
	rm -f "${CA_FILE}"
}
trap stop_servers 1 15 2 EXIT


cat >${CA_FILE} <<_EOF
-----BEGIN CERTIFICATE-----
MIIC6jCCAdKgAwIBAgIBATANBgkqhkiG9w0BAQsFADAmMSQwIgYDVQQDDBtvcGVu
c2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgwHhcNMTgwMjA5MTY1MjI3WhcNMjMwMjA4
MTY1MjI4WjAmMSQwIgYDVQQDDBtvcGVuc2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgw
ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDUJDl3RHbicnWWcxinW2k5
Bz3IJ6NWhETGaLycsc9yWXf1HKxb5TQuiwmiTAgKo9vXHQrtKq991279QYtw7sGC
bNJKfGMBFTBBMIFnNcT/ckhPxs8eVOY/cpiVnrZZfim1D7/nqmRQk5n8Si6Kx9se
U9PEwZnOdABeiCcCxxAXqSXw/3kZNgWY9Byf6gdGNWcsClTiu4tHRtk05dnJnyuL
ruGFnQXElq+CofayEsWJ6WXH0R2uhy7tOrI8Twu+XYxcQiMXaz93vePzOuxjY/tH
leg26xNgRmtaM9xJ+mlQkbULK32AiUROq51PvJaXDJMg9se6sZndBCgxrKm7YAL/
AgMBAAGjIzAhMA4GA1UdDwEB/wQEAwICpDAPBgNVHRMBAf8EBTADAQH/MA0GCSqG
SIb3DQEBCwUAA4IBAQBmI5HiWNgYwC87MLWsicGquhcBm9letaY+s1HUKgJNBbh1
jaKoHv+asWySrWVcPp1qszD7/clzuoVL6XdEPNet9L41NiaU3B7IeMgJWAz/nR2b
JdcazzsUkaXFVhdFkyx2mIqSX2yjPgUjehKXuOZ8bbfbWYF8IeKGVc1YHawKPdSE
vFFf5U09f3TmQWAh3o/FNt+cQCi8TkIRnLndvKTZ/PxHPi3o8cm60a+FGqKoT00d
eC6+8RSihgV3y214DP+llakiCuQDVnMCccdU8EZ+AgKaKhxUXIh3CZR/dRvBtzcA
I1YtLa8NGet7Qjnpwsc+NuFb0pJNoiVtKw3EejUA
-----END CERTIFICATE-----
_EOF

cat >${SERVER_KEY_FILE} <<_EOF
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAwZMHoY9/cUE5OqeTK/Ch1uHOAp0FRBtBcK95Kiq2tzVWcy2Q
0UP5oOMCRnMeh4bFin++fPwvF1jC7OdXMaV1S+FgwVjpi7BAI/bxG6jycEguEfrV
gon57qj6OfnHtvWjsL2SCdMioDAB5dbgR8JWGykf7CkVHgh1Lm1FcdbyTR/vw7yP
yPQ+YEj1dY4fURVEXc9yxJ+J5lY/vkESSunsGO+EUGoiFjQ1mTh3JMcohRoNvuB+
p4/g+wr7LfajkGDqtxM9YmVarrLNa9VvABF6NaO+W4ejvFUkz5+EZrc3MtLc6X60
OlbM2u4VywenuVH5ZYYoZ0lZS6p0MZuq3zMgTQIDAQABAoIBAHohjw4DILBPK5Fz
SyrM/v85pqYFdd4bqDU1sSfGnVOIZovy8szlq2kz8SqL1XZCtP4GTSREZF3BlfKs
n1nmf9QpVceHloqY4E8Qrdz6wkPPdqnHbdCXx0Yp/P55NuWbo/SOFsb2HIGe6IOg
CA+ecH9gehChdv5k7bImJUuHB4dahsUSVuH5c1d76d+R6NYxCAppc3oy8LVTCGgU
ig1Vfp9ismASLeqlPAXDniIb1yCbnsqE09GCSrOQtZ6yVR8trqxZAO+2xx9fTScx
1n+YLLxgzrSQhmPzNxKbOjNyq9xCZiB8uwih8U0nwSnRwdDIgJchp8367rchyBmq
gNcL9S0CgYEA6g9MWMXViUnFkaZPxV9lbHqRkbZjTgdZ6HUpVAJkTPYmLCVSH09A
c7FKLuSLH2rlabg/xnzcmqSdJsU7zDst8+w6kxRujFPw9gZUweCaM4bA4cGnvMj/
4uhy6CkBAEW/009wXqqahQhtkEuw2yXhAkHUR+wbIeTye9uJkWiJj4cCgYEA07g1
RmIMLmxrt/0CoOUG1Z5yvxlaI1Cp+ZGPpCtp6pBkwFFzd6SzlnMvfqrF2z9fezsv
8c+dWbRfLItmn5PvjIhXq3hhvD6MJeXaPXZAAVUC5pKiu707BnGeu+Lk0exZMAgB
pS2WJKZCZaC0NUJ27m/Wwh2W+shk5rnFI8MkvosCgYEA4pTKsMlbTRsIUlYwtP4D
fj8tOmTYv0mohLsetf/WvxYun9/FHyAmYZkIGlsOPuzJh01hF7H6EQ44P7cBi1Ti
yFYv4gAOgHQmONSqKkFWpXjWsfU5fy0JYczqp8pB+NSMvXASdOIs0Yn2HpDXdV62
8uttJ+7t2SL8hmBhTU1olXMCgYBDGSQ5NCWsKMxSuSq2Fx99YAP5sG0yuAPGhm1B
mEivACgOE0JG7rnDuqmYuUKPY5w9D9r4BdZWcaWgFmXluRq4LRWr0DEZWbFM6XWq
+Oj8AxcyP9K3MRedyTCHVzcxmHgDkuYClVn6L37nenDiWDgdBPDJAFuzCwN/Y+yo
ktX4VQKBgAUqF/Inpj8X8l07FmZWNmnN/9ZmwxGfRUrVc4Ug/gdZJdKewrXoRlV5
5w6uSWxkTcNfGJ71lNtu84Fckn2vb+yga5XR42SH3+fIVpF6FrxLnWbVTjZ5ADrS
m1rOyDBy8Y9mwODrhICRnn4Q/oL+2EOFK8qSACJ0Ox8pSfJxZUcI
-----END RSA PRIVATE KEY-----
_EOF

cat >${SERVER_CERT_FILE} <<_EOF
-----BEGIN CERTIFICATE-----
MIIENzCCAx+gAwIBAgIBGzANBgkqhkiG9w0BAQsFADAmMSQwIgYDVQQDDBtvcGVu
c2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgwHhcNMTgwMjI3MjAzMDQwWhcNMjAwMjI3
MjAzMDQxWjAXMRUwEwYDVQQDEwwxMC4xMy4xMjkuNDcwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDBkwehj39xQTk6p5Mr8KHW4c4CnQVEG0Fwr3kqKra3
NVZzLZDRQ/mg4wJGcx6HhsWKf758/C8XWMLs51cxpXVL4WDBWOmLsEAj9vEbqPJw
SC4R+tWCifnuqPo5+ce29aOwvZIJ0yKgMAHl1uBHwlYbKR/sKRUeCHUubUVx1vJN
H+/DvI/I9D5gSPV1jh9RFURdz3LEn4nmVj++QRJK6ewY74RQaiIWNDWZOHckxyiF
Gg2+4H6nj+D7Cvst9qOQYOq3Ez1iZVquss1r1W8AEXo1o75bh6O8VSTPn4Rmtzcy
0tzpfrQ6Vsza7hXLB6e5UfllhihnSVlLqnQxm6rfMyBNAgMBAAGjggF9MIIBeTAO
BgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIw
ADCCAUIGA1UdEQSCATkwggE1ggprdWJlcm5ldGVzghJrdWJlcm5ldGVzLmRlZmF1
bHSCFmt1YmVybmV0ZXMuZGVmYXVsdC5zdmOCJGt1YmVybmV0ZXMuZGVmYXVsdC5z
dmMuY2x1c3Rlci5sb2NhbIIJbG9jYWxob3N0gglvcGVuc2hpZnSCEW9wZW5zaGlm
dC5kZWZhdWx0ghVvcGVuc2hpZnQuZGVmYXVsdC5zdmOCI29wZW5zaGlmdC5kZWZh
dWx0LnN2Yy5jbHVzdGVyLmxvY2FsggwxMC4xMy4xMjkuNDeCCTEyNy4wLjAuMYIK
MTcyLjE3LjAuMYIKMTcyLjMwLjAuMYINMTkyLjE2OC4xMjQuMYIMMTkyLjE2OC4y
My4xhwQKDYEvhwR/AAABhwSsEQABhwSsHgABhwTAqHwBhwTAqBcBMA0GCSqGSIb3
DQEBCwUAA4IBAQCGE5PJQn6cSj6MQe3GPcKTitLGHp94dJttB2v4Q9Gj9aKF9fI+
fbNb7Kgh8fPRKxVqU400aio2aQiCONIe7PlnjKjWTkJjKgntcOx/UAES1q13fRIU
mF/tH5jX6JlL0VbvQ97vYar0pXfF97A2JDjFB1cj0axULPA+RH+Z2QgQ4slzPtz9
CvRFdicHLHCiqPVKFB5vTmtNoG/Hzjd8rMPpyW9bf8PeNeyUjV5g/JGUyqfovHiW
2OC96XKWOgCnPYLZ5LAtk33GyMnL+sxWzr4Kgp4Q3OfBFYVthG0NInIu0zYorR8G
avi3GlxRtyRyE79dmPPXA2kLNFApSq3o2jkV
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIC6jCCAdKgAwIBAgIBATANBgkqhkiG9w0BAQsFADAmMSQwIgYDVQQDDBtvcGVu
c2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgwHhcNMTgwMjA5MTY1MjI3WhcNMjMwMjA4
MTY1MjI4WjAmMSQwIgYDVQQDDBtvcGVuc2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgw
ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDUJDl3RHbicnWWcxinW2k5
Bz3IJ6NWhETGaLycsc9yWXf1HKxb5TQuiwmiTAgKo9vXHQrtKq991279QYtw7sGC
bNJKfGMBFTBBMIFnNcT/ckhPxs8eVOY/cpiVnrZZfim1D7/nqmRQk5n8Si6Kx9se
U9PEwZnOdABeiCcCxxAXqSXw/3kZNgWY9Byf6gdGNWcsClTiu4tHRtk05dnJnyuL
ruGFnQXElq+CofayEsWJ6WXH0R2uhy7tOrI8Twu+XYxcQiMXaz93vePzOuxjY/tH
leg26xNgRmtaM9xJ+mlQkbULK32AiUROq51PvJaXDJMg9se6sZndBCgxrKm7YAL/
AgMBAAGjIzAhMA4GA1UdDwEB/wQEAwICpDAPBgNVHRMBAf8EBTADAQH/MA0GCSqG
SIb3DQEBCwUAA4IBAQBmI5HiWNgYwC87MLWsicGquhcBm9letaY+s1HUKgJNBbh1
jaKoHv+asWySrWVcPp1qszD7/clzuoVL6XdEPNet9L41NiaU3B7IeMgJWAz/nR2b
JdcazzsUkaXFVhdFkyx2mIqSX2yjPgUjehKXuOZ8bbfbWYF8IeKGVc1YHawKPdSE
vFFf5U09f3TmQWAh3o/FNt+cQCi8TkIRnLndvKTZ/PxHPi3o8cm60a+FGqKoT00d
eC6+8RSihgV3y214DP+llakiCuQDVnMCccdU8EZ+AgKaKhxUXIh3CZR/dRvBtzcA
I1YtLa8NGet7Qjnpwsc+NuFb0pJNoiVtKw3EejUA
-----END CERTIFICATE-----
_EOF

cat >${CLIENT_KEY_FILE} <<_EOF
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAtOVer+O8g8uO4vWtkdlZBoPPKV94gXqVrM4Xere9ZnQCPtvt
p7bOOOZ/5S5NjYvwC2fAlZvd5yvxorq3+a45yTkd2ep8HFLJ23nAH8/WG2VIAxRY
B244Qzr/GBmbgR9ugJMEQRQNq5uSifSnYxh1M8UcE90p9MhAJV9XKIPxRrJ68SoK
8F0zZzpag4UftvVe4NbzfE1yVoEHQWFoA481iap3Z2bykIPWNUun/fVmeYHML931
qqKTlhrxzwDnxomt/LHMtmdGve1i5YuULq2u7t8ofPLRV0czDfJhwfmncY7n9y5g
a0LRhOEI2xKsrXA9Yvkui3h0TzuvFtCqM02XRQIDAQABAoIBAQCKyI7kkuxGkR2G
ssX/Z6kNfoKpUz242LuMYHFTDTSaLdarM0AZs/5zWSQ2SFfniL0ZgvgV0AdnHCe+
mVIclLZw0wk77tJZSIrlf3sO7P1u9z1QX4NJ8B3qNpEPhFXxspOswR46b5AtYKYE
gVcKh/EjTs5DzyIpUpkkEwljZBbwDSL0ORN4yu7iRl+UTCTCc4WuLTwpqAxSzdj2
dnWh9dsbBURvtPCE5pd6isYUsNWZWw7rxywJryzIXxFjgEI2gZVpzu3qZSsgbfzb
2vu8HROiuXiJOI1p43LYVpc2YQJn2BuiKqJdWLC11KAJA7OXMoFw5j2hXKVI+Odc
UFVJhd4BAoGBAMQX4G+iC2yoWHqbeBfTWrh2jKdjsz1THWIbR8AgXnYh1N6WCnC6
7idFsjnZHND0YK4oYEbs/a78c2Yb7O8xkqQSQKvdDltfeYStd2wlBXvLB3StM86V
mSY+WneVVkZAkyD+crmYSRlyKtmaLs2bB48bTtwJqxxMTUXFvPGImOOVAoGBAOwo
8Eh9DAuD6p9fnu1WyuA+78dwHSEz+Kn5VruZfRgdfyqnRz50Ukpc8/leSJXLRC+h
oVLtPkwPwePpZ+5dTk6nanMe5teLdKffkbG3VW8OwhkOq9+siR9yZDpugCR/GDC9
o8zolqtABOO1M49tIOhtgcLEJI7EasOiLQH5i/jxAoGBAL92JLA6wvbbxFAqPm7c
8aZMMfc6NIb7ASSKSFtB/5lOXR7b1uPM0L1NosAyyZ0IDuHdEGwP934EhdQ8DfJa
L7i9DaIA24TBys+N452W5CzDxsrYVk4t6PPbS8+Y4z0CzeUYLAIku7L5svb2QR6F
cTL8UdosIoMlyQkIEfyvB8ClAoGAdxzC7NzdWWWEzjOtdioDk41K5T3AA4IyFpEj
VOW6uZIPFNVgUrja1JUDnTAXzi3Cy39rXec5N6Xu9mRAPnKjT3qTb1MTvX2iLhXO
Z2N/3M8FyRukRuHAG7NXqD0Zts6/xb2ww2ZAsElO7gbz5ZB2O6UYAMNraPLaoqfG
qatTFRECgYEAlqX225YTimBQYBsQhzuc624z4gFeSvQAIQUt7TYi76ROuGAeGpOg
wDCJ9nL+GQK3lLfvq4rJ7osj+Nb6dAZ8ReUDfdiQJPnXzR14TlFFuOB2F5dY7g0R
1q68UhDRGxZPpbzniB3XORUn0l7GGyDRG1cwd45NyVhh1Z+4OijFUlU=
-----END RSA PRIVATE KEY-----
_EOF

cat >${CLIENT_CERT_FILE} <<_EOF
-----BEGIN CERTIFICATE-----
MIIDJDCCAgygAwIBAgIBCDANBgkqhkiG9w0BAQsFADAmMSQwIgYDVQQDDBtvcGVu
c2hpZnQtc2lnbmVyQDE1MTgxOTUxNDgwHhcNMTgwMjA5MTY1MjI4WhcNMjAwMjA5
MTY1MjI5WjBOMTUwHAYDVQQKExVzeXN0ZW06Y2x1c3Rlci1hZG1pbnMwFQYDVQQK
Ew5zeXN0ZW06bWFzdGVyczEVMBMGA1UEAxMMc3lzdGVtOmFkbWluMIIBIjANBgkq
hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtOVer+O8g8uO4vWtkdlZBoPPKV94gXqV
rM4Xere9ZnQCPtvtp7bOOOZ/5S5NjYvwC2fAlZvd5yvxorq3+a45yTkd2ep8HFLJ
23nAH8/WG2VIAxRYB244Qzr/GBmbgR9ugJMEQRQNq5uSifSnYxh1M8UcE90p9MhA
JV9XKIPxRrJ68SoK8F0zZzpag4UftvVe4NbzfE1yVoEHQWFoA481iap3Z2bykIPW
NUun/fVmeYHML931qqKTlhrxzwDnxomt/LHMtmdGve1i5YuULq2u7t8ofPLRV0cz
DfJhwfmncY7n9y5ga0LRhOEI2xKsrXA9Yvkui3h0TzuvFtCqM02XRQIDAQABozUw
MzAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDAYDVR0TAQH/
BAIwADANBgkqhkiG9w0BAQsFAAOCAQEAI5ZEs4jVJ8TFkyntHOSnQieK6KRQoQlR
cERjbdd4VNwC935WO2hn2a0L09HB2RuE9RNLVyNfl2xqZJLs60htwMXtQMTWUcZl
b7t3usT24eZ6/pLiNR2TfptAXnOIKC9Eyl+2AtX4J0F1A7UdQ/Lx2v0sxkyi4m6/
t4mLiXznv3sQDt1gTwqzk96ri4cRRWas+4xrekrgpW1Ihm9EKHpcWPA+1sEVTQts
GsdY90vo8XLmGADA5W65O7fLb5OenYKrY71nAnRRrg2btclWuF1IwBQ9gTW3DV32
efG5F9prPIasEpbVUhgSHxVXBr/9SAol4b44FvMRhZc3YwbGjFWdHQ==
-----END CERTIFICATE-----
_EOF

echo "=== Bringing TLS server up ==="

TESTDATE="2018-03-01"

# Start OpenSSL TLS server
#
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${OPENSSL}" s_server -cert ${SERVER_CERT_FILE} -key ${SERVER_KEY_FILE} \
	  -CAfile ${CA_FILE} -port ${PORT} -Verify 1 -verify_return_error -www
SERVER_PID="${!}"
wait_server "${SERVER_PID}"

datefudge -s "${TESTDATE}" \
      "${GNUTLS_CLI}" --x509certfile ${CLIENT_CERT_FILE} \
      --x509keyfile ${CLIENT_KEY_FILE} --x509cafile=${CA_FILE} \
      --port="${PORT}" localhost </dev/null
rc=$?

if test "${rc}" != "0"; then
    echo "Failed to connect to server"
    exit 1
fi

kill ${SERVER_PID}

rm -f "${SERVER_CERT_FILE}"
rm -f "${SERVER_KEY_FILE}"
rm -f "${CLIENT_CERT_FILE}"
rm -f "${CLIENT_KEY_FILE}"
rm -f "${CA_FILE}"

exit 0
