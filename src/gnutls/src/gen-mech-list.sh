#!/bin/sh

# Copyright (C) 2017-2018 Red Hat, Inc.
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

HEADER=`pkg-config --cflags-only-I p11-kit-1|awk '{print $1}'|sed 's/-I//g'`
HEADER="${HEADER}/p11-kit/pkcs11.h"

echo "const char *mech_list[] = {"

# Exclude duplicate and uninteresting entries
EXCLUDED="(CKM_VENDOR_DEFINED\s|CKM_CAST128_MAC\s|CKM_CAST128_KEY_GEN\s|CKM_CAST128_ECB\s|CKM_CAST128_CBC\s|CKM_CAST128_MAC_GENERAL\s|CKM_CAST128_CBC_PAD\s|CKM_PBE_MD5_CAST128_CBC\s|CKM_PBE_SHA1_CAST128_CBC\s|CKM_EC_KEY_PAIR_GEN\s)"

TMPFILE=list.$$.tmp

cat ${HEADER}|grep -E "define\sCKM_"|grep -vE "${EXCLUDED}"|awk '{print "\t["$3"] = \""$2"\","}' |sort -u

echo "};"

