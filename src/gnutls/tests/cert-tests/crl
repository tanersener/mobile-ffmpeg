#!/bin/sh

# Copyright (C) 2015 Nikos Mavrogiannopoulos
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

export TZ="UTC"

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"
ac_cv_sizeof_time_t="${ac_cv_sizeof_time_t:-8}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi

OUTFILE=out-crl.$$.tmp
INFOFILE=out-crl-info.$$.tmp
OUTFILE2=out2-crl.$$.tmp
TMPFILE=crl.$$.tmp
TMP2FILE=crl2.$$.tmp

echo "crl_next_update = 43" >$TMPFILE
echo "crl_number = 7" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CRL generation failed"
	exit ${rc}
fi

${VALGRIND} "${CERTTOOL}" --crl-info --infile ${OUTFILE} --no-text --outfile ${TMP2FILE}
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text crl info failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMP2FILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text crl info failed 2"
	exit 1
fi

grep "Revoked certificates (152)" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL generation didn't succeed as expected"
	exit 1
fi

sed 's/\r$//' <"${INFOFILE}" | grep "CRL Number (not critical): 07$" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL generation didn't succeed as expected (2)"
	grep "CRL Number (not critical):" "${INFOFILE}"
	exit 1
fi

# check appending a certificate

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-crl "${OUTFILE}" --load-certificate "${srcdir}/data/cert-ecc256.pem" --template \
	"${TMPFILE}" -d 9 >${OUTFILE2} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CRL appending failed"
	exit ${rc}
fi

grep "Revoked certificates (153)" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL appending didn't succeed as expected"
	exit 1
fi

grep "Serial Number (hex): 07" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL appending didn't succeed as expected (2)"
	exit 1
fi

# check the dates

echo "crl_this_update_date = \"2004-03-29 16:21:42\"" >$TMPFILE
echo "crl_next_update_date = \"2006-03-29 13:21:42\"" >>$TMPFILE
echo "crl_number = 8" >>$TMPFILE
echo "crl_revocation_date = \"2003-02-01 10:00:00\"" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/cert-ecc256.pem" --template \
	"${TMPFILE}" -d 9 >${OUTFILE2} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CRL date setting failed"
	exit ${rc}
fi

grep "Revoked at: Sat Feb 01 10:00:00 UTC 2003" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL date setting didn't succeed as expected"
	exit 1
fi

grep "Issued: Mon Mar 29 16:21:42 UTC 2004" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL date setting didn't succeed as expected (2)"
	exit 1
fi

grep "Next at: Wed Mar 29 13:21:42 UTC 2006" "${INFOFILE}" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL date setting didn't succeed as expected (3)"
	exit 1
fi

# Check hex serial number
echo "crl_next_update = 43" >$TMPFILE
echo "crl_number = 0x1234567890abcdef1234567890abcdef12345678" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CRL hex number failed"
	exit ${rc}
fi

sed 's/\r$//' <"${INFOFILE}" | grep "CRL Number (not critical): 1234567890abcdef1234567890abcdef12345678$" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL hex number didn't succeed as expected"
	grep "CRL Number (not critical):" "${INFOFILE}"
	exit 1
fi

# Check default CRL number
echo "crl_next_update = 43" >$TMPFILE

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

datefudge -s "2020-01-20 10:00:00" ${VALGRIND} \
	"${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key" \
	--load-ca-certificate "${srcdir}/data/template-test.pem" \
	--load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CRL default number failed"
	exit ${rc}
fi

sed 's/\r$//' <"${INFOFILE}" | grep "CRL Number (not critical): 5e257a20[0-9a-f]\{30\}$" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL default number didn't succeed as expected"
	grep "CRL Number (not critical):" "${INFOFILE}"
	exit 1
fi

if test "${ac_cv_sizeof_time_t}" = 8;then
	# we should test that on systems which have 64-bit time_t
	datefudge -s "2138-01-20 10:00:00" ${VALGRIND} \
		"${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key" \
		--load-ca-certificate "${srcdir}/data/template-test.pem" \
		--load-certificate "${srcdir}/data/ca-certs.pem" --template \
		"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
	rc=$?

	# We're done.
	if test "${rc}" != "0"; then
		echo "CRL default number 2 failed"
		exit ${rc}
	fi

	sed 's/\r$//' <"${INFOFILE}" | grep "CRL Number (not critical): 013c1972a0[0-9a-f]\{30\}$" >/dev/null 2>&1
	if test "$?" != "0"; then
		echo "CRL default number 2 didn't succeed as expected"
		grep "CRL Number (not critical):" "${INFOFILE}"
		exit 1
	fi
fi

# Check large decimal CRL number
echo "crl_next_update = 43" >$TMPFILE
echo "crl_number = 1234567890123456789012345678" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "CRL large decimal number succeeded when shouldn't"
	exit ${rc}
fi

sed 's/\r$//' <"${INFOFILE}" | grep "error parsing number: 1234567890123456789012345678" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL large number didn't fail as expected"
	exit 1
fi

# Check invalid hex number
echo "crl_next_update = 43" >$TMPFILE
echo "crl_number = 0xsomething" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "CRL invalid hex number succeeded when shouldn't"
	exit ${rc}
fi

sed 's/\r$//' <"${INFOFILE}" | grep "error parsing number: 0xsomething" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL invalid hex number didn't fail as expected"
	exit 1
fi

# Check invalid number
echo "crl_next_update = 43" >$TMPFILE
echo "crl_number = something" >>$TMPFILE

${VALGRIND} "${CERTTOOL}" --generate-crl --load-ca-privkey "${srcdir}/data/template-test.key"  --load-ca-certificate \
	"${srcdir}/data/template-test.pem" --load-certificate "${srcdir}/data/ca-certs.pem" --template \
	"${TMPFILE}" >${OUTFILE} 2>${INFOFILE}
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "CRL invalid number succeeded when shouldn't"
	exit ${rc}
fi

sed 's/\r$//' <"${INFOFILE}" | grep "error parsing number: something" >/dev/null 2>&1
if test "$?" != "0"; then
	echo "CRL invalid number didn't fail as expected"
	exit 1
fi

# Check CRL verification

## CRL validation is expected to succeed
${VALGRIND} "${CERTTOOL}" --verify-crl --infile "${srcdir}/data/ca-crl-valid.crl"  --load-ca-certificate \
	"${srcdir}/data/ca-crl-valid.pem" >${OUTFILE} 2>${INFOFILE}
rc=$?
if test "${rc}" != "0"; then
	echo "CRL verification failed"
	exit ${rc}
fi

## CRL validation is expected to fail because the CA doesn't have the CRLSign key usage flag
${VALGRIND} "${CERTTOOL}" --verify-crl --infile "${srcdir}/data/ca-crl-invalid.crl"  --load-ca-certificate \
	"${srcdir}/data/ca-crl-invalid.pem" >${OUTFILE} 2>${INFOFILE}
rc=$?
if test "${rc}" = "0"; then
	echo "CRL verification succeeded when shouldn't"
	exit 1
fi

rm -f "${OUTFILE}"
rm -f "${INFOFILE}"
rm -f "${OUTFILE2}"
rm -f "${TMPFILE}"
rm -f "${TMP2FILE}"

exit 0
