#!/bin/sh

# Copyright (C) 2010-2016 Free Software Foundation, Inc.
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
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

srcdir="${srcdir:-.}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
SED="${SED:-sed}"
unset RETCODE

TMPFILE=cipher-listings.$$.tmp
TMPFILE2=cipher-listings2.$$.tmp

if ! test -x "${CLI}"; then
	exit 77
fi

if test "${WINDIR}" != ""; then
	exit 77
fi 

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi


. "${srcdir}/scripts/common.sh"

echo "Checking ciphersuite listings"

tab=$(printf '\t')
check()
{
	prio=$2
	name=$1
	echo checking $prio
	"${CLI}" --list --priority $prio|grep -v ^Certificate|grep -v ^Ciphers|grep -v ^MACs|grep -v ^Key|grep -v Compression|grep -v ^Groups|grep -v ^Elliptic|${SED} -e 's/'"${tab}"'SSL3.0$//g' -e 's/'"${tab}"'TLS1.0$//g'|grep -v ^PK>$TMPFILE
	cat ${srcdir}/data/listings-$name|${SED} 's/'"${tab}"'SSL3.0$//g' >$TMPFILE2
	${DIFF} ${TMPFILE} ${TMPFILE2}
	if test $? != 0;then
		echo Error checking $prio with $name
		echo output in ${TMPFILE}
		exit 1
	fi
}

${CLI} --fips140-mode
if test $? = 0;then
	echo "Cannot run this test in FIPS140-2 mode"
	exit 77
fi

# We check whether the ciphersuites listed by gnutls-cli
# for specific (legacy) protocols remain constant. We
# don't check newer protocols as these change more often.

# This is a unit test for gnutls_priority_get_cipher_suite_index

if test "${ENABLE_SSL3}" = "1";then
check SSL3.0 "NORMAL:-VERS-ALL:+VERS-SSL3.0:+ARCFOUR-128"
fi
check TLS1.0 "NORMAL:-VERS-ALL:+VERS-TLS1.0"
check TLS1.1 "NORMAL:-VERS-ALL:+VERS-TLS1.1"
check SSL3.0-TLS1.1 "NORMAL:-VERS-ALL:+VERS-TLS1.0:+VERS-SSL3.0:+VERS-TLS1.1"
check DTLS1.0 "NORMAL:-VERS-ALL:+VERS-DTLS1.0"
# Priority strings prior to 3.6.x did not require the +GROUP option; here we
# test whether these work as expected.
check legacy1 "NONE:+VERS-TLS-ALL:+MAC-ALL:+RSA:+AES-128-GCM:+SIGN-ALL:+COMP-NULL"
check legacy2 "NONE:+VERS-TLS-ALL:+MAC-ALL:+RSA:+CAMELLIA-256-GCM:+SIGN-ALL:+COMP-NULL"
check legacy3 "NONE:+VERS-TLS-ALL:+MAC-ALL:+RSA:+CAMELLIA-256-GCM:+SIGN-ALL:+COMP-NULL:+CTYPE-OPENPGP"
check legacy4 "NONE:+VERS-TLS-ALL:+MAC-ALL:+RSA:+CAMELLIA-256-GCM:+SIGN-ALL:+COMP-NULL:-CTYPE-OPENPGP"


rm -f ${TMPFILE}
rm -f ${TMPFILE2}
exit 0
