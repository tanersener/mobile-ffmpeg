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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

srcdir="${srcdir:-.}"
PSKTOOL="${PSKTOOL:-../src/psktool${EXEEXT}}"
TMPFILE=psktool.$$.tmp

if ! test -x "${PSKTOOL}"; then
	exit 77
fi

if test "${WINDIR}" != ""; then
	exit 77
fi 

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi


. "${srcdir}/scripts/common.sh"

echo "Checking PSK tool basic operations"

# echo create a user and check whether a key is available
"${PSKTOOL}" -p ${TMPFILE} -u test
if test $? != 0;then
	echo "password generation failed..."
	exit 1
fi

grep 'test:' ${TMPFILE} >/dev/null 2>&1
if test $? != 0;then
	echo "could not find generated user..."
	exit 1
fi

KEY=$(cat ${TMPFILE} |cut -d ':' -f 2)

if test "${#KEY}" != 64;then
	echo "the generated key is not 256-bits"
	exit 1
fi


# Create second user and check whether both exist

"${PSKTOOL}" -p ${TMPFILE} -u user2
if test $? != 0;then
	echo "password generation failed..."
	exit 1
fi

grep 'test:' ${TMPFILE} >/dev/null 2>&1
if test $? != 0;then
	echo "could not find first generated user..."
	exit 1
fi

grep 'user2:' ${TMPFILE} >/dev/null 2>&1
if test $? != 0;then
	echo "could not find second generated user..."
	exit 1
fi

rm -f $TMPFILE

exit 0
