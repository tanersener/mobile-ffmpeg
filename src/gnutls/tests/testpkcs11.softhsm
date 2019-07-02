#!/bin/sh

# Copyright (C) 2013 Nikos Mavrogiannopoulos
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

for i in /usr/lib64/pkcs11 /usr/lib/softhsm /usr/lib/x86_64-linux-gnu/softhsm /usr/lib /usr/lib64/softhsm;do
	if test -f "$i/libsofthsm2.so"; then
		ADDITIONAL_PARAM="--provider $i/libsofthsm2.so"
		break
	else
		if test -f "$i/libsofthsm.so";then
			ADDITIONAL_PARAM="--provider $i/libsofthsm.so"
			break
		fi
	fi
done

init_card () {
	PIN="$1"
	PUK="$2"

	if test -x "/usr/bin/softhsm2-util"; then
		export SOFTHSM2_CONF="softhsm-testpkcs11.$$.config.tmp"
		SOFTHSM_TOOL="/usr/bin/softhsm2-util"
		${SOFTHSM_TOOL} --version|grep "2.0.0" >/dev/null 2>&1
		if test $? = 0; then
			echo "softhsm2-util 2.0.0 is broken"
			export BROKEN_SOFTHSM2=1
		fi
	fi

	if test -x "/usr/bin/softhsm"; then
		export SOFTHSM_CONF="softhsm-testpkcs11.$$.config.tmp"
		SOFTHSM_TOOL="/usr/bin/softhsm"
	fi

	if test -z "${SOFTHSM_TOOL}"; then
		echo "Could not find softhsm(2) tool"
		exit 77
	fi

	if test -z "${SOFTHSM_CONF}"; then
		rm -rf ./softhsm-testpkcs11.$$.tmp
		mkdir -p ./softhsm-testpkcs11.$$.tmp
		echo "objectstore.backend = file" > "${SOFTHSM2_CONF}"
		echo "directories.tokendir = ./softhsm-testpkcs11.$$.tmp" >> "${SOFTHSM2_CONF}"

	else
		rm -rf ./softhsm-testpkcs11.$$.tmp
		echo "0:./softhsm-testpkcs11.$$.tmp" > "${SOFTHSM_CONF}"
	fi


	echo -n "* Initializing smart card... "
	${SOFTHSM_TOOL} --init-token --slot 0 --label "GnuTLS-Test" --so-pin "${PUK}" --pin "${PIN}" >/dev/null #2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi
}
