#!/bin/sh

# Copyright (C) 2016 Red Hat, Inc.
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
TEMPLATE_FILE="ms-out.$$.tmpl.tmp"
SERVER_CERT_FILE="ms-cert.$$.pem.tmp"
OCSP_RESPONSE_FILE="ms-resp.$$.tmp"
OCSP_REQ_FILE="ms-req.$$.tmp"

export TZ="UTC"

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

. "${srcdir}/scripts/common.sh"

check_for_datefudge

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT

# Port to use for OCSP server, must match the OCSP URI set in the
# server_*.pem certificates
eval "${GETPORT}"
OCSP_PORT=$PORT

# Maximum timeout for server startup (OCSP and TLS)
SERVER_START_TIMEOUT=10

# Check for OpenSSL
OPENSSL=`which openssl`
if ! test -x "${OPENSSL}"; then
    echo "You need openssl to run this test."
    exit 77
fi

CERTDATE="2016-04-28"
TESTDATE="2016-04-29"
EXP_OCSP_DATE="2016-03-27"

OCSP_PID=""
TLS_SERVER_PID=""
stop_servers ()
{
    test -z "${OCSP_PID}" || kill "${OCSP_PID}"
    test -z "${TLS_SERVER_PID}" || kill "${TLS_SERVER_PID}"
    rm -f "$TEMPLATE_FILE"
    rm -f "$SERVER_CERT_FILE"
    rm -f "$OCSP_RESPONSE_FILE"
    rm -f "$OCSP_REQ_FILE"
}
trap stop_servers 1 15 2 EXIT

echo "=== Generating good server certificate ==="

rm -f "$TEMPLATE_FILE"
cp "${srcdir}/ocsp-tests/certs/server_good.template" "$TEMPLATE_FILE"
chmod u+w "$TEMPLATE_FILE"
echo "ocsp_uri=http://localhost:${OCSP_PORT}/ocsp/" >>"$TEMPLATE_FILE"
echo "tls_feature = 5" >>"$TEMPLATE_FILE"

# Generate certificates with the random port
datefudge -s "${CERTDATE}" ${CERTTOOL} \
	--generate-certificate --load-ca-privkey "${srcdir}/ocsp-tests/certs/ca.key" \
	--load-ca-certificate "${srcdir}/ocsp-tests/certs/ca.pem" \
	--load-privkey "${srcdir}/ocsp-tests/certs/server_good.key" \
	--template "${TEMPLATE_FILE}" --outfile "${SERVER_CERT_FILE}" 2>/dev/null

echo "=== Bringing OCSP server up ==="

INDEXFILE="ocsp_index.txt"
ATTRFILE="${INDEXFILE}.attr"
cp "${srcdir}/ocsp-tests/certs/ocsp_index.txt" ${INDEXFILE}
cp "${srcdir}/ocsp-tests/certs/ocsp_index.txt.attr" ${ATTRFILE}

# Start OpenSSL OCSP server
#
# WARNING: As of version 1.0.2g, OpenSSL OCSP cannot bind the TCP port
# if started repeatedly in a short time, probably a lack of
# SO_REUSEADDR usage.
PORT=${OCSP_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${OPENSSL}" ocsp -index "${INDEXFILE}" -text \
	  -port "${OCSP_PORT}" \
	  -rsigner "${srcdir}/ocsp-tests/certs/ocsp-server.pem" \
	  -rkey "${srcdir}/ocsp-tests/certs/ocsp-server.key" \
	  -CA "${srcdir}/ocsp-tests/certs/ca.pem"
OCSP_PID="${!}"
wait_server "${OCSP_PID}"

echo "=== Verifying OCSP server is up ==="

# Port probing (as done in wait_port) makes the OpenSSL OCSP server
# crash due to the "invalid request", so try proper requests
t=0
while test "${t}" -lt "${SERVER_START_TIMEOUT}"; do
    # Run a test request to make sure the server works
    datefudge "${TESTDATE}" \
	      ${VALGRIND} "${OCSPTOOL}" --ask \
	      --load-cert "${SERVER_CERT_FILE}" \
	      --load-issuer "${srcdir}/ocsp-tests/certs/ca.pem" \
	      --outfile "${OCSP_RESPONSE_FILE}"
    rc=$?
    if test "${rc}" = "0"; then
	break
    else
	t=`expr ${t} + 1`
	sleep 1
    fi
done
# Fail if the final OCSP request failed
if test "${rc}" != "0"; then
    echo "OCSP server check failed."
    exit ${rc}
fi

#echo "placed staple in ${OCSP_RESPONSE_FILE}"

echo "=== Test 1: Server with valid certificate - no staple ==="

PORT=${TLS_SERVER_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}"
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "1"; then
    echo "Connecting to server with valid certificate and no staple succeeded"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID

echo "=== Test 2: Server with valid certificate - valid staple ==="

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT
PORT=${TLS_SERVER_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ocsp-response="${OCSP_RESPONSE_FILE}" --ignore-ocsp-response-errors
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "0"; then
    echo "Connecting to server with valid certificate and valid staple failed"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID

echo "=== Test 3: Server with valid certificate - invalid staple ==="

head -c 64 /dev/urandom >"${OCSP_RESPONSE_FILE}"

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT
PORT=${TLS_SERVER_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ocsp-response="${OCSP_RESPONSE_FILE}" --ignore-ocsp-response-errors
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "1"; then
    echo "Connecting to server with valid certificate and invalid staple succeeded"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID

echo "=== Test 4: Server with valid certificate - unrelated cert staple ==="

rm -f "${OCSP_RESPONSE_FILE}"
cp "${srcdir}/ocsp-tests/certs/ocsp-staple-unrelated.der" "${OCSP_RESPONSE_FILE}"

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT
PORT=${TLS_SERVER_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ocsp-response="${OCSP_RESPONSE_FILE}" --ignore-ocsp-response-errors
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "1"; then
    echo "Connecting to server with valid certificate and invalid staple succeeded"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID


echo "=== Test 5: Server with valid certificate - expired staple ==="

rm -f "${OCSP_RESPONSE_FILE}"

# Generate an OCSP response which expires in 2 days and use it after
# a month. gnutls server doesn't send such a staple to clients.
${VALGRIND} ${OCSPTOOL} --generate-request --load-issuer "${srcdir}/ocsp-tests/certs/ocsp-server.pem" --load-cert "${SERVER_CERT_FILE}" --outfile "${OCSP_REQ_FILE}"
datefudge -s ${EXP_OCSP_DATE} \
	${OPENSSL} ocsp -index "${INDEXFILE}" -rsigner "${srcdir}/ocsp-tests/certs/ocsp-server.pem" -rkey "${srcdir}/ocsp-tests/certs/ocsp-server.key" -CA "${srcdir}/ocsp-tests/certs/ca.pem" -reqin "${OCSP_REQ_FILE}" -respout "${OCSP_RESPONSE_FILE}" -ndays 2

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT
PORT=${TLS_SERVER_PORT}

TIMEOUT=$(which timeout)
if test -n "$TIMEOUT";then
${TIMEOUT} 30 "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ocsp-response="${OCSP_RESPONSE_FILE}"
if test $? != 1;then
    echo "Running gnutls-serv with an expired response, succeeds!"
    exit ${rc}
fi
fi

echo "=== Test 5.1: Server with valid certificate - expired staple (ignoring errors) ==="

launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ignore-ocsp-response-errors \
	  --ocsp-response="${OCSP_RESPONSE_FILE}"
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "1"; then
    echo "Connecting to server with valid certificate and expired staple succeeded"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID

echo "=== Test 6: Server with valid certificate - old staple ==="

# This case is funny. OCSP doesn't mandate an expiration date for a response so
# we are left to decide what to do with responses that don't contain the NextUpdate
# field. Here we test whether a month-old response with no clear expiration is rejected.

rm -f "${OCSP_RESPONSE_FILE}"

${VALGRIND} ${OCSPTOOL} --generate-request --load-issuer "${srcdir}/ocsp-tests/certs/ocsp-server.pem" --load-cert "${SERVER_CERT_FILE}" --outfile "${OCSP_REQ_FILE}"
datefudge -s ${EXP_OCSP_DATE} \
	${OPENSSL} ocsp -index ${INDEXFILE} -rsigner "${srcdir}/ocsp-tests/certs/ocsp-server.pem" -rkey "${srcdir}/ocsp-tests/certs/ocsp-server.key" -CA "${srcdir}/ocsp-tests/certs/ca.pem" -reqin "${OCSP_REQ_FILE}" -respout "${OCSP_RESPONSE_FILE}"

eval "${GETPORT}"
# Port for gnutls-serv
TLS_SERVER_PORT=$PORT
PORT=${TLS_SERVER_PORT}
launch_bare_server $$ \
	  datefudge "${TESTDATE}" \
	  "${GNUTLS_SERV}" --echo --disable-client-cert \
	  --x509keyfile="${srcdir}/ocsp-tests/certs/server_good.key" \
	  --x509certfile="${SERVER_CERT_FILE}" \
	  --port="${TLS_SERVER_PORT}" \
	  --ocsp-response="${OCSP_RESPONSE_FILE}" --ignore-ocsp-response-errors
TLS_SERVER_PID="${!}"
wait_server $TLS_SERVER_PID

wait_for_port "${TLS_SERVER_PORT}"

echo "test 123456" | \
    datefudge -s "${TESTDATE}" \
	      "${GNUTLS_CLI}" --ocsp --x509cafile="${srcdir}/ocsp-tests/certs/ca.pem" \
	      --port="${TLS_SERVER_PORT}" localhost
rc=$?

if test "${rc}" != "1"; then
    echo "Connecting to server with valid certificate and old staple succeeded"
    exit ${rc}
fi

kill "${TLS_SERVER_PID}"
wait "${TLS_SERVER_PID}"
unset TLS_SERVER_PID


kill ${OCSP_PID}
wait ${OCSP_PID}
unset OCSP_PID

rm -f "${OCSP_RESPONSE_FILE}"
rm -f "${OCSP_REQ_FILE}"
rm -f "${SERVER_CERT_FILE}"
rm -f "${TEMPLATE_FILE}"
rm -f "${INDEXFILE}" "${ATTRFILE}"

exit 0
