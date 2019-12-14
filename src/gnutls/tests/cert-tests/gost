#!/bin/sh

# Copyright (C) 2016-2017 Free Software Foundation, Inc.
#
# Author: Dmitry Eremin-Solenikov
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
TMPFILE=gost.$$.tmp
TMPCA=gost-ca.$$.tmp
TMPCAKEY=gost-ca-key.$$.tmp
TMPSUBCA=gost-subca.$$.tmp
TMPSUBCAKEY=gost-subca-key.$$.tmp
TMPKEY=gost-key.$$.tmp
TMPTEMPL=template.$$.tmp
TMPUSER=user.$$.tmp
VERIFYOUT=verify.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

echo ca > $TMPTEMPL
echo "cn = GOST STREEBOG 256 CA" >> $TMPTEMPL

"${CERTTOOL}" --generate-privkey --key-type gost12-512 --curve TC26-512-A > $TMPCAKEY 2>/dev/null
#"${CERTTOOL}" --generate-privkey --key-type gost12-256 --curve CryptoPro-XchA > $TMPCAKEY 2>/dev/null

"${CERTTOOL}" -d 2 --generate-self-signed --template $TMPTEMPL \
	--load-privkey $TMPCAKEY \
	--outfile $TMPCA \
	>$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo ca > $TMPTEMPL
"${CERTTOOL}" --generate-privkey --key-type gost12-256 --curve CryptoPro-A  > $TMPSUBCAKEY 2>/dev/null
echo "cn = GOST STREEBOG-256 Mid CA" >> $TMPTEMPL

"${CERTTOOL}" -d 2 --generate-certificate --template $TMPTEMPL \
	--load-ca-privkey $TMPCAKEY \
	--load-ca-certificate $TMPCA \
	--load-privkey $TMPSUBCAKEY \
	--outfile $TMPSUBCA \
	>$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo "cn = End-user" > $TMPTEMPL

"${CERTTOOL}" --generate-privkey --key-type gost01 --curve CryptoPro-XchA > $TMPKEY 2>/dev/null

"${CERTTOOL}" -d 2 --generate-certificate --template $TMPTEMPL \
	--load-ca-privkey $TMPSUBCAKEY \
	--load-ca-certificate $TMPSUBCA \
	--load-privkey $TMPKEY \
	--outfile $TMPUSER >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

cat $TMPUSER $TMPSUBCA $TMPCA > $TMPFILE
"${CERTTOOL}" --verify-chain <$TMPFILE > $VERIFYOUT

if [ $? != 0 ]; then
	cat $VERIFYOUT
	exit 1
fi

echo "cn = End-user" > $TMPTEMPL

"${CERTTOOL}" --generate-privkey --key-type gost01 --curve TC26-256-B > $TMPKEY 2>/dev/null

"${CERTTOOL}" -d 2 --generate-certificate --template $TMPTEMPL \
	--load-ca-privkey $TMPSUBCAKEY \
	--load-ca-certificate $TMPSUBCA \
	--load-privkey $TMPKEY \
	--outfile $TMPUSER >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

cat $TMPUSER $TMPSUBCA $TMPCA > $TMPFILE
"${CERTTOOL}" --verify-chain <$TMPFILE > $VERIFYOUT

if [ $? != 0 ]; then
	cat $VERIFYOUT
	exit 1
fi

"${CERTTOOL}" -i < "${srcdir}"/data/grfc.crt --outfile $TMPFILE
if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

if ! cmp "${srcdir}"/data/grfc.crt $TMPFILE ; then
	cat $TMPFILE
	exit 1
fi

"${CERTTOOL}" -i < "${srcdir}"/data/gost-cert-ca.pem --outfile $TMPFILE
if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

if ! cmp "${srcdir}"/data/gost-cert-ca.pem $TMPFILE ; then
	cat $TMPFILE
	exit 1
fi

"${CERTTOOL}" -i < "${srcdir}"/data/gost-cert-new.pem --outfile $TMPFILE
if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

if ! cmp "${srcdir}"/data/gost-cert-new.pem $TMPFILE ; then
	cat $TMPFILE
	exit 1
fi

"${CERTTOOL}" --verify --load-ca-certificate "${srcdir}"/data/gost-cert-ca.pem --infile "${srcdir}"/data/gost-cert-new.pem --outfile $TMPFILE
if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

rm -f $VERIFYOUT $TMPUSER $TMPCA $TMPSUBCA $TMPTEMPL $TMPFILE
rm -f $TMPSUBCAKEY $TMPCAKEY $TMPKEY

exit 0
