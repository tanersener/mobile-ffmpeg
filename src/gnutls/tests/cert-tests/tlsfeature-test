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

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"
TMPFILE=tlsfeature.$$.tmp
TMPFILE2=tlsfeature-2.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

#
# Test certificate generation
#
datefudge -s "2007-04-22" \
"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-tlsfeature.tmpl" \
		--outfile "${TMPFILE}" 2>/dev/null
rc=$?

${DIFF} --ignore-matching-lines "Algorithm Security Level" "${srcdir}/data/template-tlsfeature.pem" "${TMPFILE}" >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Cert generation test failed"
	exit ${rc}
fi

#
# Test certificate printing
#
rm -f "${TMPFILE}"
rm -f "${TMPFILE2}"
"${CERTTOOL}" -i \
		--infile "${srcdir}/data/template-tlsfeature.pem" --outfile "${TMPFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "Cert printing (0) failed"
	exit ${rc}
fi

grep -A 2 "TLS Features" "${TMPFILE}" >"${TMPFILE2}" 2>/dev/null
rc=$?

if test "${rc}" != "0"; then
	echo "Cert printing (1) failed"
	exit ${rc}
fi

grep "17" "${TMPFILE2}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "Cert printing (1) failed"
	exit ${rc}
fi

grep "Status Request(5)" "${TMPFILE2}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "Cert printing (2) failed"
	exit ${rc}
fi


#
# Test certificate request generation
#

datefudge -s "2007-04-22" \
"${CERTTOOL}" --generate-request \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-tlsfeature.tmpl" \
		--outfile "${TMPFILE}" -d 4 #2>/dev/null
rc=$?
if test "${rc}" != "0"; then
	echo "CSR generation test (0) failed"
	exit ${rc}
fi

${DIFF} --ignore-matching-lines "Algorithm Security Level" "${srcdir}/data/template-tlsfeature.csr" "${TMPFILE}" #>/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "CSR generation test (1) failed"
	exit ${rc}
fi

#
# Test certificate request printing
#
rm -f "${TMPFILE}"
rm -f "${TMPFILE2}"
"${CERTTOOL}" --crq-info \
		--infile "${srcdir}/data/template-tlsfeature.csr" --outfile "${TMPFILE}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "CSR printing (0) failed"
	exit ${rc}
fi

grep -A 2 "TLS Features" "${TMPFILE}" >"${TMPFILE2}" 2>/dev/null
rc=$?

if test "${rc}" != "0"; then
	echo "CSR printing (1) failed"
	exit ${rc}
fi

grep "17" "${TMPFILE2}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "CSR printing (2) failed"
	exit ${rc}
fi

grep "Status Request(5)" "${TMPFILE2}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "CSR printing (3) failed"
	exit ${rc}
fi

#
# Test certificate generation after a request
#
datefudge -s "2007-04-22" \
"${CERTTOOL}" --generate-certificate \
		--load-privkey "${srcdir}/data/template-test.key" \
		--load-ca-privkey "${srcdir}/data/template-test.key" \
		--load-ca-certificate "${srcdir}/data/template-tlsfeature.pem" \
		--template "${srcdir}/templates/template-tlsfeature-crq.tmpl" \
		--load-request "${TMPFILE}" >"${TMPFILE2}" 2>&1

grep -A 2 "TLS Features" "${TMPFILE2}" >"${TMPFILE}" 2>/dev/null
rc=$?

if test "${rc}" != "0"; then
	echo "Cert generation (csr) (0) failed"
	exit ${rc}
fi

grep "17" "${TMPFILE}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "Cert generation (csr) (1) failed"
	exit ${rc}
fi

grep "Status Request(5)" "${TMPFILE}" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "Cert generation (csr) (2) failed"
	exit ${rc}
fi


rm -f "${TMPFILE}"
rm -f "${TMPFILE2}"

exit 0
