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


init_card () {
	PIN="$1"
	PUK=3537363231383830
	export GNUTLS_SO_PIN="${PUK}"

	echo -n "* Erasing smart card... "
	sc-hsm-tool --initialize --so-pin "${PUK}" --pin "${PIN}" --label=GnuTLS-Test >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Initializing smart card... "
	TOKEN=`${P11TOOL} ${ADDITIONAL_PARAM} --list-tokens pkcs11:token=Nikos|grep URL|grep token=GnuTLS-Test|sed 's/\s*URL\: //g'`
	if test -z "${TOKEN}"; then
		echo "Could not find initialized card"
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --initialize "${TOKEN}" --set-so-pin "${PUK}" --set-pin "${PIN}" --label "GnuTLS-Test" >>"${TMPFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi
}
