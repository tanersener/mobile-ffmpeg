#!/bin/sh

# Copyright (C) 2006-2012 Free Software Foundation, Inc.
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
ac_cv_sizeof_time_t="${ac_cv_sizeof_time_t:-8}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

export TZ="UTC"
TMPFILE=tmp-tt.pem.$$.tmp

. ${srcdir}/../scripts/common.sh

check_for_datefudge

echo "Running test for ${ac_cv_sizeof_time_t}-byte time_t"

# Note that in rare cases this test may fail because the
# time set using datefudge could have changed since the generation
# (if example the system was busy)

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-test.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-test.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 1 failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-utf8.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-utf8.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 2 (UTF8) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-dn.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-dn.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 3 (DN) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

echo "Running test for certificate generation with --generate-self-signed"

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-certificate \
		--load-privkey "${srcdir}/data/template-test.key" \
		--load-ca-privkey "${srcdir}/../../doc/credentials/x509/ca-key.pem" \
		--load-ca-certificate "${srcdir}/../../doc/credentials/x509/ca.pem" \
		--template "${srcdir}/templates/template-dn.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-sgenerate.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 3-a non-self-signed generation failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-dn-err.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null
rc=$?

if test "${rc}" = "0"; then
	echo "Test 3 (DN-err) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-overflow.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-overflow.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 4 (overflow1) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

# The following test works in 64-bit systems

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-overflow2.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

rc=$?
if test "${ac_cv_sizeof_time_t}" -lt 8;then
	if test "$rc" = "0"; then
		echo "Test 5-1 (overflow2) succeeded unexpectedly with 32-bit time_t"
		exit ${rc}
	fi
else
	if test "$rc" != "0"; then
		echo "Test 5-1 (overflow2) failed"
		exit ${rc}
	fi

	${DIFF} "${srcdir}/data/template-overflow2.pem" ${TMPFILE} #>/dev/null 2>&1
	rc=$?

	# We're done.
	if test "${rc}" != "0"; then
		echo "Test 5-2 (overflow2) failed"
		exit ${rc}
	fi

fi
rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-date.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-date.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 6 (explicit dates) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-dates-after2038.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null
rc=$?
if test "${ac_cv_sizeof_time_t}" -lt 8;then
	if test "$rc" = "0"; then
		echo "Test 6-2 (explicit dates) succeeded unexpectedly with 32-bit long"
		exit ${rc}
	fi
else
	if test "$rc" != "0"; then
		echo "Test 6-2 (explicit dates) failed"
		exit ${rc}
	fi

	${DIFF} "${srcdir}/data/template-dates-after2038.pem" ${TMPFILE} >/dev/null 2>&1
	rc=$?

	if test "${rc}" != "0"; then
		echo "Test 6-3 (explicit dates) failed"
		exit ${rc}
	fi
fi

rm -f ${TMPFILE}

# Test name constraints generation

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-nc.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-nc.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 7 (name constraints) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}


# Test the GeneralizedTime support
if test "${ac_cv_sizeof_time_t}" = 8;then
	# we should test that on systems which have 64-bit time_t.
	datefudge -s "2051-04-22" \
			"${CERTTOOL}" --generate-self-signed \
				--load-privkey "${srcdir}/data/template-test.key" \
				--template "${srcdir}/templates/template-generalized.tmpl" \
				--outfile ${TMPFILE} 2>/dev/null

	${DIFF} "${srcdir}/data/template-generalized.pem" ${TMPFILE} >/dev/null 2>&1
	rc=$?

	# We're done.
	if test "${rc}" != "0"; then
		echo "Test 8 (generalizedTime) failed"
		exit ${rc}
	fi
fi

rm -f ${TMPFILE}

# Test unique ID field generation

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-unique.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-unique.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 9 (unique ID) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

# Test generation with very long dns names

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-long-dns.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/long-dns.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 10 (long dns) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

# Test generation with larger serial number

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-long-serial.tmpl" \
		--outfile ${TMPFILE} 2>/dev/null

${DIFF} "${srcdir}/data/long-serial.pem" ${TMPFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 11 (long serial) failed"
	exit ${rc}
fi

rm -f ${TMPFILE}

exit 0
