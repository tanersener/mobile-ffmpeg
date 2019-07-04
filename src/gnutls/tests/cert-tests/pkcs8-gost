#!/bin/sh

# Copyright (C) 2018 Dmitry Eremin-Solenikov
# Copyright (C) 2004-2006, 2010, 2012 Free Software Foundation, Inc.
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
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
TMPFILE=pkcs8-gost-decode.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

ret=0
# key-gost12-512.p8 is not supported for now: it uses curve TC26-512-B
for p8 in "key-gost01.p8" "key-gost12-256.p8" "key-gost01-2.p8" "key-gost12-256-2.p8" "key-gost01-2-enc.p8 Пароль%20для%20PFX" "key-gost12-256-2-enc.p8 Пароль%20для%20PFX"; do
	set -- ${p8}
	file="$1"
	passwd=$(echo $2|sed 's/%20/ /g')
	${VALGRIND} "${CERTTOOL}" --key-info --pkcs8 --password "${passwd}" \
		--infile "${srcdir}/data/${file}" --outfile $TMPFILE \
		--pkcs-cipher none
	rc=$?
	if test ${rc} != 0; then
		echo "PKCS8 FATAL ${p8}"
		ret=1
		continue
	fi

	${DIFF} "${srcdir}/data/${1}.txt" $TMPFILE
	rc=$?
	if test ${rc} != 0; then
		cat $TMPFILE
		echo "PKCS8 FATAL TXT ${p8}"
		ret=1
	else
		echo "PKCS8 OK ${p8}"
	fi
done

rm -f $TMPFILE

echo "PKCS8 DONE (rc $ret)"
exit $ret
