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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

#set -e

srcdir="${srcdir:-.}"
builddir="${builddir:-.}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
P11TOOL="${P11TOOL:-../src/p11tool${EXEEXT}}"
DIFF="${DIFF:-diff}"
PKGCONFIG="${PKG_CONFIG:-$(which pkg-config)}"
TMP_SOFTHSM_DIR="./softhsm-load.$$.tmp"
P11DIR="p11-kit-conf.$$.tmp"
PIN=1234
PUK=1234

for lib in ${libdir} ${libdir}/pkcs11 /usr/lib64/pkcs11/ /usr/lib/pkcs11/ /usr/lib/x86_64-linux-gnu/pkcs11/;do
	if test -f "${lib}/p11-kit-trust.so"; then
		TRUST_MODULE="${lib}/p11-kit-trust.so"
		echo "located ${MODULE}"
		break
	fi
done

for lib in ${libdir} ${libdir}/pkcs11 /usr/lib64/pkcs11/ /usr/lib/pkcs11/ /usr/lib/x86_64-linux-gnu/pkcs11/ /usr/lib/softhsm/;do
	if test -f "${lib}/libsofthsm2.so"; then
		SOFTHSM_MODULE="${lib}/libsofthsm2.so"
		echo "located ${MODULE}"
		break
	fi
done

${PKGCONFIG} --version >/dev/null || exit 77

${PKGCONFIG} --atleast-version=0.23.10 p11-kit-1
if test $? != 0;then
	echo p11-kit 0.23.10 is required
	exit 77
fi

if ! test -f "${TRUST_MODULE}"; then
	echo "p11-kit trust module was not found"
	exit 77
fi

if ! test -f "${SOFTHSM_MODULE}"; then
	echo "softhsm module was not found"
	exit 77
fi

# Create pkcs11.conf with two modules, a trusted (p11-kit-trust)
# and softhsm (not trusted)
mkdir -p ${P11DIR}

cat <<_EOF_ >${P11DIR}/p11-kit-trust.module
module: p11-kit-trust.so
trust-policy: yes
_EOF_

cat <<_EOF_ >${P11DIR}/softhsm.module
module: libsofthsm2.so
_EOF_

# Setup softhsm
rm -rf ${TMP_SOFTHSM_DIR}
mkdir -p ${TMP_SOFTHSM_DIR}
SOFTHSM2_CONF=${TMP_SOFTHSM_DIR}/conf
export SOFTHSM2_CONF
echo "objectstore.backend = file" > "${SOFTHSM2_CONF}"
echo "directories.tokendir = ${TMP_SOFTHSM_DIR}" >> "${SOFTHSM2_CONF}"

softhsm2-util --init-token --slot 0 --label "GnuTLS-Test" --so-pin "${PUK}" --pin "${PIN}" >/dev/null #2>&1
if test $? != 0; then
	echo "failed to initialize softhsm"
	exit 1
fi

GNUTLS_PIN="${PIN}" ${P11TOOL} --login --label GnuTLS-Test-RSA --generate-privkey rsa --provider "${SOFTHSM_MODULE}" pkcs11: --outfile /dev/null
if test $? != 0; then
	echo "failed to generate privkey"
	exit 1
fi

FILTERTOKEN="sed s/token=.*//g"

# Check whether both are listed

nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -a|${FILTERTOKEN}|sort -u|wc -l)
#nr=$(${P11TOOL} --list-tokens|grep 'Module:'|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error: did not find 2 modules ($nr)"
	${builddir}/pkcs11/list-tokens -o ${P11DIR}
	exit 1
fi

# Check whether whether list-tokens will list the trust module
# if we only load softhsm. It shouldn't as we only load the
# trust module when needed (e.g., verification).

nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -m -s "${SOFTHSM_MODULE}"|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 1;then
	echo "Error: did not find softhsm module"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -m -s "${SOFTHSM_MODULE}"
	exit 1
fi

# Check whether both modules are found when gnutls_pkcs11_init
# is not called but a pkcs11 operation is called.
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -d|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error in test 1: did not find 2 modules"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -d
	exit 1
fi

# Check whether both modules are found when gnutls_pkcs11_init 
# is called with the auto flag
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -a|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error in test 2: did not find 2 modules"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -a
	exit 1
fi

# Check whether only trusted modules are listed when the
# trusted flag is given to gnutls_pkcs11_init().
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -t|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 1;then
	echo "Error in test 3: did not find the trusted module"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -t
	exit 1
fi

# Check whether only trusted is listed after certificate verification
# is performed.
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -v|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 1;then
	echo "Error in test 4: did not find 1 module"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -v
	exit 1
fi

# Check whether only trusted is listed when gnutls_pkcs11_init
# is called with manual flag and a certificate verification is performed.
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -m -v|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 1;then
	echo "Error in test 5: did not find 1 module"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -m -v
	exit 1
fi

# Check whether all modules are listed after certificate verification
# is performed then a PKCS#11 function is called.
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -v -d|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error in test 6: did not find all modules"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -v -d
	exit 1
fi

# Check whether all modules are listed after a private key operation.
nr=$(${builddir}/pkcs11/list-tokens -o ${P11DIR} -p|${FILTERTOKEN}|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error in test 7: did not find all modules"
	${builddir}/pkcs11/list-tokens -o ${P11DIR} -p
	exit 1
fi

# Check whether public key and privkey are listed.
nr=$(GNUTLS_PIN="${PIN}" ${builddir}/pkcs11/list-objects -o ${P11DIR} -t all pkcs11:token=GnuTLS-Test|sort -u|wc -l)
if test "$nr" != 2;then
	echo "Error in test 8: did not find all objects"
	${builddir}/pkcs11/list-objects -o ${P11DIR} -t all pkcs11:token=GnuTLS-Test
	exit 1
fi

# Check whether all privkeys are listed even if trust module is registered.
nr=$(GNUTLS_PIN="${PIN}" ${builddir}/pkcs11/list-objects -o ${P11DIR} -t privkey pkcs11:|sort -u|wc -l)
if test "$nr" != 1;then
	echo "Error in test 9: did not find privkey objects"
	${builddir}/pkcs11/list-objects -o ${P11DIR} -t privkey pkcs11:
	exit 1
fi

rm -f ${P11DIR}/*
rm -rf ${TMP_SOFTHSM_DIR}

exit 0
