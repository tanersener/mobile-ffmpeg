#!/bin/sh

# Copyright (C) 2006-2008, 2010, 2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
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

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

. "${srcdir}/../scripts/common.sh"

TMPFILE=tmp-$$.pem.tmp
TMPFILE1=tmp1-$$.pem.tmp
TMPFILE2=tmp2-$$.pem.tmp

#check whether "funny" spaces can be interpreted
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/funny-spacing.pem" >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Funny-spacing cert decoding failed 1"
	exit ${rc}
fi

#check whether a BMPString attribute can be properly decoded
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/bmpstring.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "BMPString cert decoding failed 1"
	exit ${rc}
fi

check_if_equal "${srcdir}/data/bmpstring.pem" ${TMPFILE} "Algorithm Security Level"
rc=$?

if test "${rc}" != "0"; then
	echo "BMPString cert decoding failed 2"
	exit ${rc}
fi

#check whether complex-cert is decoded as expected
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/complex-cert.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "Complex cert decoding failed 1"
	exit ${rc}
fi

check_if_equal "${srcdir}/data/complex-cert.pem" ${TMPFILE} "Not After:|Algorithm Security Level"
rc=$?

if test "${rc}" != "0"; then
	echo "Complex cert decoding failed 2"
	exit ${rc}
fi

#check whether the cert with many othernames is decoded as expected
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/xmpp-othername.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "XMPP cert decoding failed 1"
	exit ${rc}
fi

check_if_equal "${srcdir}/data/xmpp-othername.pem" ${TMPFILE} "^warning|Not After:|Algorithm Security Level"
rc=$?

if test "${rc}" != "0"; then
	echo "XMPP cert decoding failed 2"
	exit ${rc}
fi

${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/template-krb5name.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "XMPP cert decoding failed 1"
	exit ${rc}
fi

grep "KRB5Principal:" ${TMPFILE} >${TMPFILE1}
grep "KRB5Principal:" "${srcdir}/data/template-krb5name-full.pem" >${TMPFILE2}
check_if_equal ${TMPFILE1} ${TMPFILE2}
rc=$?

if test "${rc}" != "0"; then
	echo "KRB5 principalname cert decoding failed 1"
	exit ${rc}
fi


#check whether the cert with GOST parameters is decoded as expected
if test "${ENABLE_GOST}" = "1"; then
	GOSTCERT="${srcdir}/data/gost-cert.pem"
else
	GOSTCERT="${srcdir}/data/gost-cert-nogost.pem"
fi

${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${GOSTCERT}" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "GOST cert decoding failed 1"
	exit ${rc}
fi

check_if_equal ${TMPFILE} "${GOSTCERT}"
rc=$?

if test "${rc}" != "0"; then
	echo "GOST cert decoding failed 2"
	exit ${rc}
fi

#check whether the cert with GOST 31.10/11-94 parameters is decoded as expected
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/gost94-cert.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "GOST94 cert decoding failed 1"
	exit ${rc}
fi

check_if_equal ${TMPFILE} "${srcdir}/data/gost94-cert.pem" "Algorithm Security Level"
rc=$?

if test "${rc}" != "0"; then
	echo "GOST94 cert decoding failed 2"
	exit ${rc}
fi

${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/multi-value-dn.pem" >${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "MV-DN cert decoding failed 1"
	exit ${rc}
fi

# Needed for FIPS140 mode
check_if_equal "${srcdir}/data/multi-value-dn.pem" ${TMPFILE} "Algorithm Security Level:"
rc=$?

if test "${rc}" != "0"; then
	echo "MV-DN cert decoding failed 2"
	exit ${rc}
fi

#check if --no-text works as expected
${VALGRIND} "${CERTTOOL}" --certificate-info --infile "${srcdir}/data/cert-ecc256.pem" --no-text --outfile ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text -k --certificate-info failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMPFILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text -k --certificate-info failed 2"
	exit 1
fi

#check if --no-text works as expected
${VALGRIND} "${CERTTOOL}" --certificate-pubkey --infile "${srcdir}/data/cert-ecc256.pem" --no-text  --outfile ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text cert pubkey failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMPFILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text cert pubkey failed 2"
	exit 1
fi

#check if --no-text works as expected
${VALGRIND} "${CERTTOOL}" --pubkey-info --infile "${srcdir}/data/cert-ecc256.pem" --no-text  --outfile ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text pubkey info failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMPFILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text pubkey info failed 2"
	exit 1
fi

rm -f ${TMPFILE} ${TMPFILE1} ${TMPFILE2}

exit 0
