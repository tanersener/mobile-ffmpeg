#!/bin/sh

# Copyright (C) 2017 Red Hat, Inc.
#
# This file is part of p11-kit.
#
# p11-kit is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
#
# p11-kit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#set -e

srcdir="${srcdir:-.}"
P11TOOL="${P11TOOL:-../src/p11tool${EXEEXT}}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"

EXPORTED_FILE=out.$$.tmp
DER_FILE=out-der.$$.tmp
TMPFILE=out-tmp.$$.tmp

for lib in ${libdir} ${libdir}/pkcs11 /usr/lib64/pkcs11/ /usr/lib/pkcs11/ /usr/lib/x86_64-linux-gnu/pkcs11/;do
	if test -f "${lib}/p11-kit-trust.so"; then
		MODULE="${lib}/p11-kit-trust.so"
		echo "located ${MODULE}"
		break
	fi
done

if ! test -x "${P11TOOL}"; then
	echo "p11tool was not found"
	exit 77
fi

if ! test -f "${MODULE}"; then
	echo "p11-kit trust module was not found"
	exit 77
fi

TRUST_PATH="${srcdir}/p11-kit-trust-data/"
CACERT=${TRUST_PATH}/Example_Root_CA.pem

# Test whether a CA extracted from a trust store can retrieve stapled
# extensions.

OPTS="--provider ${MODULE} --provider-opts trusted,p11-kit:paths=\"${TRUST_PATH}\""

# Informational
${P11TOOL} --list-all-certs ${OPTS} 'pkcs11:'


####
# Test 1: Extract the CA certificate from store

${P11TOOL} --export 'pkcs11:object=Example%20CA' ${OPTS} --outder --outfile ${EXPORTED_FILE}
if test "$?" != "0"; then
	echo "Exporting failed (1)"
	exit 1
fi

${CERTTOOL} -i --infile ${CACERT} --outder --outfile ${DER_FILE}
if test "$?" != "0"; then
	echo "Exporting failed (2)"
	exit 1
fi

${DIFF} ${EXPORTED_FILE} ${DER_FILE}
if test "$?" != "0"; then
	echo "Files ${EXPORTED_FILE} and ${DER_FILE} are not identical"
	exit 1
fi

rm -f ${EXPORTED_FILE} ${DER_FILE} ${TMPFILE}

echo "Root CA retrieval test passed..."

####
# Test 2: Extract the certificate from store with the stapled data

${P11TOOL} --export-stapled 'pkcs11:object=Example%20CA' ${OPTS} --outder --outfile ${EXPORTED_FILE}
if test "$?" != "0"; then
	echo "Exporting failed (3)"
	exit 1
fi

${CERTTOOL} -i --infile ${CACERT} --outder --outfile ${DER_FILE}
if test "$?" != "0"; then
	echo "Exporting failed (4)"
	exit 1
fi

${DIFF} ${EXPORTED_FILE} ${DER_FILE}
if test "$?" = "0"; then
	echo "Files are identical; no extensions were stapled"
	exit 1
fi

${CERTTOOL} -i --inder --infile ${EXPORTED_FILE} --outfile ${TMPFILE}
if test "$?" != "0"; then
	echo "PEM converting failed"
	exit 1
fi

grep -i "Name Constraints" ${TMPFILE}
if test "$?" != "0"; then
	cat ${TMPFILE}
	echo "No name constraints found (1)"
	exit 1
fi

grep -i "Permitted" ${TMPFILE}
if test "$?" != "0"; then
	cat ${TMPFILE}
	echo "No name constraints found (2)"
	exit 1
fi

grep -i "DNSname: example.com" ${TMPFILE}
if test "$?" != "0"; then
	cat ${TMPFILE}
	echo "No name constraints found (3)"
	exit 1
fi

echo "Root CA with stapled extensions retrieval test passed..."

rm -f ${EXPORTED_FILE} ${DER_FILE} ${TMPFILE}
exit 0
