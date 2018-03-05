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
TEMPLFILE=template.$$.tmp
CAFILE=ca.$$.tmp
SUBCAFILE=subca.$$.tmp
SUBSUBCAFILE=subsubca.$$.tmp
TMPFILE=sha2.$$.tmp
USERFILE=user.$$.tmp
VERIFYFILE=verify.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

echo ca > $TEMPLFILE
echo "cn = SHA 512 CA" >> $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-self-signed --template $TEMPLFILE \
	--load-privkey "${srcdir}/data/key-ca.pem" \
	--outfile $CAFILE \
	--hash sha512 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo ca > $TEMPLFILE
echo "cn = SHA 384 sub-CA" >> $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-certificate --template $TEMPLFILE \
	--load-ca-privkey "${srcdir}/data/key-ca.pem" \
	--load-ca-certificate $CAFILE \
	--load-privkey "${srcdir}/data/key-subca.pem" \
	--outfile $SUBCAFILE \
	--hash sha384 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo ca > $TEMPLFILE
echo "cn = SHA 256 sub-sub-CA" >> $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-certificate --template $TEMPLFILE \
	--load-ca-privkey "${srcdir}/data/key-subca.pem" \
	--load-ca-certificate $SUBCAFILE \
	--load-privkey "${srcdir}/data/key-subsubca.pem" \
	--outfile $SUBSUBCAFILE \
	--hash sha256 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo "cn = End-user" > $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-certificate --template $TEMPLFILE \
	--load-ca-privkey "${srcdir}/data/key-subsubca.pem" \
	--load-ca-certificate $SUBSUBCAFILE \
	--load-privkey "${srcdir}/data/key-user.pem" \
	--outfile $USERFILE >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

num=`cat $USERFILE $SUBSUBCAFILE $SUBCAFILE $CAFILE | "${CERTTOOL}" --verify-chain | tee $VERIFYFILE | grep -c Verified`
#cat verify

if test "${num}" != "4"; then
	echo Verification failure
	exit 1
fi

rm -f $VERIFYFILE $USERFILE $SUBSUBCAFILE $SUBCAFILE $CAFILE $TEMPLFILE $TMPFILE

exit 0
