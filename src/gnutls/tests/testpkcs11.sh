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

srcdir="${srcdir:-.}"
P11TOOL="${P11TOOL:-../src/p11tool${EXEEXT}}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
SERV="${SERV:-../src/gnutls-serv${EXEEXT}}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
RETCODE=0

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

if ! test -x "${P11TOOL}"; then
	exit 77
fi

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -x "${SERV}"; then
	exit 77
fi

if ! test -x "${CLI}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute valgrind --leak-check=full"
fi

TMPFILE="testpkcs11.$$.tmp"
LOGFILE="testpkcs11.debug.log"
CERTTOOL_PARAM="--stdout-info"

if test "${WINDIR}" != ""; then
	exit 77
fi

ASAN_OPTIONS="detect_leaks=0"
export ASAN_OPTIONS

have_ed25519=0

P11TOOL="${VALGRIND} ${P11TOOL} --batch"
SERV="${SERV} -q"

. ${srcdir}/scripts/common.sh

rm -f "${LOGFILE}"

exit_error () {
	echo "check ${LOGFILE} for additional debugging information"
	echo ""
	echo ""
	tail "${LOGFILE}"
	exit 1
}

# $1: token
# $2: PIN
# $3: filename
# ${srcdir}/testpkcs11-certs/client.key
write_privkey () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Writing a client private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label gnutls-client2 --load-privkey "${filename}" "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Checking whether object was marked private... "
	${P11TOOL} ${ADDITIONAL_PARAM} --list-privkeys "${token};object=gnutls-client2" 2>/dev/null | grep 'Label\:' >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo "private object was public"
		exit_error
	fi
	echo ok

	echo -n "* Checking whether object was marked sensitive... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-privkeys "${token};object=gnutls-client2" | grep "CKA_SENSITIVE" >/dev/null 2>&1
	if test $? != 0; then
		echo "private object was not sensitive"
		exit_error
	fi
	echo ok
}

# $1: token
# $2: PIN
# $3: filename
write_serv_privkey () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Writing the server private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label serv-key --load-privkey "${filename}" "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

}

# $1: token
# $2: PIN
# $3: filename
write_serv_pubkey () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Writing the server public key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label serv-pubkey --load-pubkey "${filename}" "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	#verify it being written
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all "${token};object=serv-pubkey;type=public" >>"${LOGFILE}" 2>&1
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all "${token};object=serv-pubkey;type=public"|grep "Public key" >/dev/null 2>&1
	if test $? != 0;then
		echo "Cannot verify the existence of the written pubkey"
		exit_error
	fi
}

# $1: token
# $2: PIN
# $3: filename
write_serv_cert () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Writing the server certificate... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --no-mark-private --label serv-cert --load-certificate "${filename}" "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

}

# $1: token
# $2: PIN
test_delete_cert () {
	export GNUTLS_PIN="$2"
	filename="$3"
	token="$1"

	echo -n "* Deleting the server certificate... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --delete "${token};object=serv-cert;object-type=cert" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi
}

# $1: token
# $2: PIN
# $3: bits
generate_rsa_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating RSA private key ("${bits}")... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --id 000102030405 --label gnutls-client --generate-rsa --bits "${bits}" "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi

	echo -n "* Checking whether generated private key was marked private... "
	${P11TOOL} ${ADDITIONAL_PARAM} --list-privkeys "${token};object=gnutls-client" 2>/dev/null | grep 'Label\:' >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo "private object was public"
		exit_error
	fi
	echo ok

	echo -n "* Checking whether private key was marked sensitive... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-privkeys "${token};object=gnutls-client" | grep "CKA_SENSITIVE" >/dev/null 2>&1
	if test $? != 0; then
		echo "private object was not sensitive"
		exit_error
	fi
	echo ok
}

# $1: token
# $2: PIN
# $3: bits
generate_temp_rsa_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating RSA private key ("${bits}")... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --label temp-rsa-"${bits}" --generate-rsa --bits "${bits}" "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi

#  if test ${RETCODE} = 0; then
#    echo -n "* Testing private key flags... "
#    ${P11TOOL} ${ADDITIONAL_PARAM} --login --list-keys "${token};object=gnutls-client2;object-type=private" >tmp-client-2.pub 2>>"${LOGFILE}"
#    if test $? != 0; then
#      echo failed
#      exit_error
#    fi
#
#    grep CKA_WRAP tmp-client-2.pub >>"${LOGFILE}" 2>&1
#    if test $? != 0; then
#      echo "failed (no CKA_WRAP)"
#      exit_error
#    else
#      echo ok
#    fi
#  fi
}

generate_temp_dsa_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating DSA private key ("${bits}")... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --label temp-dsa-"${bits}" --generate-dsa --bits "${bits}" "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi
}

generate_temp_ed25519_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating ed25519 private key ("${bits}")... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login -d 3 --label temp-ed25519 --generate-privkey ed25519 "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi
}

# $1: token
# $2: PIN
delete_temp_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	type="$3"

	test "${RETCODE}" = "0" || return

	echo -n "* Deleting private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --delete "${token};object=temp-${type};object-type=private" >>"${LOGFILE}" 2>&1

	if test $? != 0; then
		echo failed
		RETCODE=1
		return
	fi

	RETCODE=0
	echo ok
}

# $1: token
# $2: PIN
export_pubkey_of_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"

	echo -n "* Exporting public key of generated private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --export-pubkey "${token};object=gnutls-client;object-type=private" --outfile tmp-client-2.pub >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit 1
	fi

	${DIFF} tmp-client.pub tmp-client-2.pub
	if test $? != 0; then
		echo keys differ
		exit 1
	fi

	echo ok
}

# $1: token
# $2: SO PIN
list_pubkey_as_so () {
	export GNUTLS_SO_PIN="$2"
	token="$1"

	echo -n "* Exporting public key as SO... "
	${P11TOOL} ${ADDITIONAL_PARAM} --so-login --list-all "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit 1
	fi

	echo ok
}

# $1: token
# $2: PIN
list_privkey_without_pin_env () {
	token="$1"
	pin="$2"

	echo -n "* List private key without GNUTLS_PIN... "
	unset GNUTLS_PIN
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all-privkeys "${token}?pin-value=${pin}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit 1
	fi

	echo ok
}

# $1: token
# $2: PIN
change_id_of_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"

	echo -n "* Change the CKA_ID of generated private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --set-id "01a1b103" "${token};object=gnutls-client;id=%00%01%02%03%04%05;object-type=private" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-privkeys "${token};object=gnutls-client;object-type=private;id=%01%a1%b1%03" 2>&1 | grep 'ID: 01:a1:b1:03' >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "ID didn't change"
		exit_error
	fi

	echo ok
}

# $1: token
# $2: PIN
change_label_of_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"

	echo -n "* Change the CKA_LABEL of generated private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --set-label "new-label" "${token};object=gnutls-client;object-type=private" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-privkeys "${token};object=new-label;object-type=private" 2>&1 |grep 'Label: new-label' >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "label didn't change"
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --set-label "gnutls-client" "${token};object=new-label;object-type=private" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	echo ok
}

# $1: token
# $2: PIN
# $3: bits
generate_temp_ecc_privkey () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating ECC private key (${bits})... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --label "temp-ecc-${bits}" --generate-ecc --bits "${bits}" "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi
}

# $1: token
# $2: PIN
# $3: bits
# The same as generate_temp_ecc_privkey but no explicit login is performed.
# p11tool should detect that login is required for the operation.
generate_temp_ecc_privkey_no_login () {
	export GNUTLS_PIN="$2"
	token="$1"
	bits="$3"

	echo -n "* Generating ECC private key without --login (${bits})... "
	${P11TOOL} ${ADDITIONAL_PARAM} --label "temp-ecc-no-${bits}" --generate-ecc --bits "${bits}" "${token}" --outfile tmp-client.pub >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi
}

# $1: name
# $2: label prefix
# $3: generate option
# $4: token
# $5: PIN
# $6: bits
import_privkey () {
	export GNUTLS_PIN="$5"
	name="$1"
	prefix="$2"
	gen_option="$3"
	token="$4"
	bits="$6"

	outfile="tmp-${prefix}-${bits}.pem"

	echo -n "* Importing ${name} private key (${bits})... "

	"${CERTTOOL}" ${CERTTOOL_PARAM} --generate-privkey "${gen_option}" --pkcs8 --password= --outfile "${outfile}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit 1
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label "${prefix}-${bits}" --load-privkey "${outfile}" "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi
}

import_temp_rsa_privkey () {
	import_privkey RSA temp-rsa --rsa $@
}

import_temp_ecc_privkey () {
	import_privkey ECC temp-ecc --ecc $@
}

import_temp_ed25519_privkey () {
	import_privkey ed25519 temp-ed25519 --key-type ed25519 $@
}

import_temp_dsa_privkey () {
	import_privkey DSA temp-dsa --dsa $@
}

# $1: token
# $2: PIN
# $3: cakey: ${srcdir}/testpkcs11-certs/ca.key
# $4: cacert: ${srcdir}/testpkcs11-certs/ca.crt
#
# Tests writing a certificate which corresponds to the given key,
# as well as the CA certificate, and tries to export them.
write_certificate_test () {
	export GNUTLS_PIN="$2"
	token="$1"
	cakey="$3"
	cacert="$4"
	pubkey="$5"

	echo -n "* Generating client certificate... "
	"${CERTTOOL}" ${CERTTOOL_PARAM} ${ADDITIONAL_PARAM}  --generate-certificate --load-ca-privkey "${cakey}"  --load-ca-certificate "${cacert}"  \
	--template ${srcdir}/testpkcs11-certs/client-tmpl --load-privkey "${token};object=gnutls-client;object-type=private" \
	--load-pubkey "$pubkey" --outfile tmp-client.crt >>"${LOGFILE}" 2>&1

	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Writing client certificate... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --id "01a1b103" --label gnutls-client --load-certificate tmp-client.crt "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Checking whether ID was correctly set... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-certs "${token};object=gnutls-client;object-type=private;id=%01%a1%b1%03" 2>&1 | grep 'ID: 01:a1:b1:03' >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "ID was not set on copy"
		exit_error
	fi
	echo ok

	if test -n "${BROKEN_SOFTHSM2}";then
		return
	fi

	echo -n "* Checking whether object was public... "
	${P11TOOL} ${ADDITIONAL_PARAM} --list-all-certs "${token};object=gnutls-client;id=%01%a1%b1%03" 2>&1 | grep 'ID: 01:a1:b1:03' >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "certificate object was not public"
		exit_error
	fi
	echo ok

	if test -n "${BROKEN_SOFTHSM2}";then
		return
	fi

	echo -n "* Writing certificate of client's CA... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --mark-trusted --mark-ca --write --label gnutls-ca --load-certificate "${cacert}" "${token}" >>"${LOGFILE}" 2>&1
	ret=$?
	if test ${ret} != 0; then
		echo "Failed with PIN, trying to write with so PIN" >>"${LOGFILE}"
		${P11TOOL} ${ADDITIONAL_PARAM} --so-login --mark-ca --write --mark-trusted --label gnutls-ca --load-certificate "${cacert}" "${token}" >>"${LOGFILE}" 2>&1
		ret=$?
	fi

	if test ${ret} = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Testing certificate flags... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all-certs "${token};object=gnutls-ca;object-type=cert" >${TMPFILE} 2>&1
	grep Flags ${TMPFILE}|head -n 1 >tmp-client-2.pub 2>>"${LOGFILE}"
	if test $? != 0; then
		echo failed
		exit_error
	fi

	grep CKA_TRUSTED tmp-client-2.pub >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "failed (no CKA_TRUSTED)"
		#exit_error
	fi

	grep "CKA_CERTIFICATE_CATEGORY=CA" tmp-client-2.pub >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "failed (no CKA_CERTIFICATE_CATEGORY=CA)"
		#exit_error
	fi

	echo ok

	echo -n "* Checking output of certificate"
	grep "Expires: Sun Dec 13 08:24:54 2020" ${TMPFILE} >/dev/null
	if test $? != 0;then
		echo "failed. Expiration time not found"
		exit_error
	fi

	grep "X.509 Certificate (RSA-1024)" ${TMPFILE} >/dev/null
	if test $? != 0;then
		echo "failed. Certificate type and size not found."
		exit_error
	fi

	grep "Label: gnutls-ca" ${TMPFILE} >/dev/null
	if test $? != 0;then
		echo "failed. Certificate label not found."
		exit_error
	fi

	grep "Flags: CKA_CERTIFICATE_CATEGORY=CA; CKA_TRUSTED;" ${TMPFILE} >/dev/null
	if test $? != 0;then
		echo "failed. Object flags were not found."
		exit_error
	fi

	echo ok
	rm -f ${TMPFILE}

	echo -n "* Trying to obtain back the cert... "
	${P11TOOL} ${ADDITIONAL_PARAM} --export "${token};object=gnutls-ca;object-type=cert" --outfile crt1.tmp >>"${LOGFILE}" 2>&1
	${DIFF} crt1.tmp "${srcdir}/testpkcs11-certs/ca.crt"
	if test $? != 0; then
		echo "failed. Exported certificate differs (crt1.tmp)!"
		exit_error
	fi
	rm -f crt1.tmp
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi

	echo -n "* Trying to obtain the full chain... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --export-chain "${token};object=gnutls-client;object-type=cert"|"${CERTTOOL}" ${CERTTOOL_PARAM}  -i --outfile crt1.tmp >>"${LOGFILE}" 2>&1

	cat tmp-client.crt ${srcdir}/testpkcs11-certs/ca.crt|"${CERTTOOL}" ${CERTTOOL_PARAM}  -i >crt2.tmp
	${DIFF} crt1.tmp crt2.tmp
	if test $? != 0; then
		echo "failed. Exported certificate chain differs!"
		exit_error
	fi
	rm -f crt1.tmp crt2.tmp
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit_error
	fi
}

# $1: token
# $2: PIN
# $3: cakey: ${srcdir}/testpkcs11-certs/ca.key
# $4: cacert: ${srcdir}/testpkcs11-certs/ca.crt
#
# Tests writing a certificate which corresponds to the given key,
# and verifies whether the ID is the same. Should utilize the
# ID of the public key.
write_certificate_id_test_rsa () {
	export GNUTLS_PIN="$2"
	token="$1"
	cakey="$3"
	cacert="$4"

	echo -n "* Generating RSA private key on HSM... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --label xxx1-rsa --generate-rsa --bits 1024 "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi

	echo -n "* Checking whether right ID is set on copy... "
	"${CERTTOOL}" ${CERTTOOL_PARAM} ${ADDITIONAL_PARAM}  --generate-certificate --load-ca-privkey "${cakey}"  --load-ca-certificate "${cacert}"  \
	--template ${srcdir}/testpkcs11-certs/client-tmpl --load-privkey "${token};object=xxx1-rsa;object-type=private" \
	--outfile tmp-client.crt >>"${LOGFILE}" 2>&1

	if test $? != 0; then
		echo failed
		exit_error
	fi

	id=$(${P11TOOL} ${ADDITIONAL_PARAM} --list-all "${token};object=xxx1-rsa;object-type=public" 2>&1 | grep 'ID: '|sed -e 's/ID://' -e 's/^[ \t]*//' -e 's/[ \t]*$//')
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label tmp-xxx1-rsa --load-certificate tmp-client.crt "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-certs "${token};object=tmp-xxx1-rsa;object-type=cert" 2>&1 | grep "ID: ${id}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "ID '$id' was not set on copy"
		exit_error
	fi
	echo ok
}

# $1: token
# $2: PIN
# $3: cakey: ${srcdir}/testpkcs11-certs/ca.key
# $4: cacert: ${srcdir}/testpkcs11-certs/ca.crt
#
# Tests writing a certificate which corresponds to the given key,
# and verifies whether the ID is the same. Should utilize the
# ID of the private key.
write_certificate_id_test_rsa2 () {
	export GNUTLS_PIN="$2"
	token="$1"
	cakey="$3"
	cacert="$4"
	tmpkey="key.$$.tmp"

	echo -n "* Generating RSA private key... "
	${CERTTOOL} ${ADDITIONAL_PARAM} --generate-privkey --bits 1024 --outfile ${tmpkey} >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi

	echo -n "* Checking whether right ID is set on copy... "
	"${CERTTOOL}" ${CERTTOOL_PARAM} ${ADDITIONAL_PARAM}  --generate-certificate --load-ca-privkey "${cakey}"  --load-ca-certificate "${cacert}"  \
	--template ${srcdir}/testpkcs11-certs/client-tmpl --load-privkey ${tmpkey} \
	--outfile tmp-client.crt >>"${LOGFILE}" 2>&1

	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label xxx2-rsa --load-privkey ${tmpkey} "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	id=$(${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all "${token};object=xxx2-rsa;object-type=private" 2>&1 | grep 'ID: '|sed -e 's/ID://' -e 's/^[ \t]*//' -e 's/[ \t]*$//')

	rm -f ${tmpkey}
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label tmp-xxx2-rsa --load-certificate tmp-client.crt "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-certs "${token};object=tmp-xxx2-rsa;object-type=cert" 2>&1 | grep "ID: ${id}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "ID '$id' was not set on copy"
		exit_error
	fi
	echo ok
}

# $1: token
# $2: PIN
# $3: cakey: ${srcdir}/testpkcs11-certs/ca.key
# $4: cacert: ${srcdir}/testpkcs11-certs/ca.crt
#
# Tests writing a certificate which corresponds to the given key,
# and verifies whether the ID is the same. Should utilize the
# ID of the private key.
write_certificate_id_test_ecdsa () {
	export GNUTLS_PIN="$2"
	token="$1"
	cakey="$3"
	cacert="$4"
	tmpkey="key.$$.tmp"

	echo -n "* Generating ECDSA private key... "
	${CERTTOOL} ${ADDITIONAL_PARAM} --generate-privkey --ecdsa --outfile ${tmpkey} >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo ok
	else
		echo failed
		exit 1
	fi

	echo -n "* Checking whether right ID is set on copy... "
	"${CERTTOOL}" ${CERTTOOL_PARAM} ${ADDITIONAL_PARAM}  --generate-certificate --load-ca-privkey "${cakey}"  --load-ca-certificate "${cacert}"  \
	--template ${srcdir}/testpkcs11-certs/client-tmpl --load-privkey ${tmpkey} \
	--outfile tmp-client.crt >>"${LOGFILE}" 2>&1

	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label xxx-ecdsa --load-privkey ${tmpkey} "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	id=$(${P11TOOL} ${ADDITIONAL_PARAM} --login --list-all "${token};object=xxx-ecdsa;object-type=private" 2>&1 | grep 'ID: '|sed -e 's/ID://' -e 's/^[ \t]*//' -e 's/[ \t]*$//')

	rm -f ${tmpkey}
	${P11TOOL} ${ADDITIONAL_PARAM} --login --write --label tmp-xxx-ecdsa --load-certificate tmp-client.crt "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi

	${P11TOOL} ${ADDITIONAL_PARAM} --login --list-certs "${token};object=tmp-xxx-ecdsa;object-type=cert" 2>&1 | grep "ID: ${id}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "ID '$id' was not set on copy"
		exit_error
	fi
	echo ok
}

test_sign () {
	export GNUTLS_PIN="$2"
	token="$1"

	echo -n "* Testing signatures using the private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --test-sign "${token};object=serv-key" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "failed. Cannot test signatures."
		exit_error
	fi
	echo ok

	echo -n "* Testing RSA-PSS signatures using the private key... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --sign-params rsa-pss --test-sign "${token};object=serv-key" >>"${LOGFILE}" 2>&1
	rc=$?
	if test $rc != 0; then
		if test $rc = 2; then
			echo "failed. RSA-PSS not supported."
		else
			echo "failed. Cannot test signatures."
			exit_error
		fi
	else
		echo ok
	fi

	echo -n "* Testing signatures using the private key (with ID)... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --test-sign "${token};id=%ac%1d%7a%39%cb%72%17%94%66%6c%74%44%73%40%91%44%c0%a0%43%7d" >>"${LOGFILE}" 2>&1
	${P11TOOL} ${ADDITIONAL_PARAM} --login --test-sign "${token};id=%ac%1d%7a%39%cb%72%17%94%66%6c%74%44%73%40%91%44%c0%a0%43%7d" 2>&1|grep "Verifying against public key in the token..."|grep ok >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "failed. Cannot test signatures with ID."
		exit_error
	fi
	echo ok
}

# This tests the signing operation as well as the usage of --set-pin
test_sign_set_pin () {
	pin="$2"
	token="$1"

	unset GNUTLS_PIN

	echo -n "* Testing signatures using the private key and --set-pin... "
	${P11TOOL} ${ADDITIONAL_PARAM} --login --set-pin ${pin} --test-sign "${token};object=serv-key" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo "failed. Cannot test signatures."
		exit_error
	fi
	echo ok

	export GNUTLS_PIN=${pin}
}

# $1: token
# $2: PIN
# $3: certfile
# $4: keyfile
# $5: cafile
#
# Tests using a certificate and key pair using gnutls-serv and gnutls-cli.
use_certificate_test () {
	export GNUTLS_PIN="$2"
	token="$1"
	certfile="$3"
	keyfile="$4"
	cafile="$5"
	txt="$6"

	echo -n "* Using PKCS #11 with gnutls-cli (${txt})... "
	# start server
	eval "${GETPORT}"
	launch_pkcs11_server $$ "${ADDITIONAL_PARAM}" --echo --priority NORMAL --x509certfile="${certfile}" \
		--x509keyfile="$keyfile" --x509cafile="${cafile}" \
		--verify-client-cert --require-client-cert >>"${LOGFILE}" 2>&1

	PID=$!
	wait_server ${PID}

	# connect to server using SC
	${VALGRIND} "${CLI}" ${ADDITIONAL_PARAM} -p "${PORT}" localhost --priority NORMAL --x509cafile="${cafile}" </dev/null >>"${LOGFILE}" 2>&1 && \
		fail ${PID} "Connection should have failed!"

	${VALGRIND} "${CLI}" ${ADDITIONAL_PARAM} -p "${PORT}" localhost --priority NORMAL --x509certfile="${certfile}" \
	--x509keyfile="$keyfile" --x509cafile="${cafile}" </dev/null >>"${LOGFILE}" 2>&1 || \
		fail ${PID} "Connection (with files) should have succeeded!"

	${VALGRIND} "${CLI}" ${ADDITIONAL_PARAM} -p "${PORT}" localhost --priority NORMAL --x509certfile="${token};object=gnutls-client;object-type=cert" \
		--x509keyfile="${token};object=gnutls-client;object-type=private" \
		--x509cafile="${cafile}" </dev/null >>"${LOGFILE}" 2>&1 || \
		fail ${PID} "Connection (with SC) should have succeeded!"

	kill ${PID}
	wait

	echo ok
}

reset_pins () {
	token="$1"
	UPIN="$2"
	SOPIN="$3"
	NEWPIN=88654321
	LARGE_NEWPIN="1234123412341234123412341234123" #31 chars
	TOO_LARGE_NEWPIN="12341234123412341234123412341234" #32 chars

	echo -n "* Setting SO PIN... "
	# Test admin PIN
	GNUTLS_NEW_SO_PIN="${NEWPIN}" \
	GNUTLS_SO_PIN="${SOPIN}" \
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-so-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	# reset back
	echo -n "* Re-setting SO PIN... "
	TMP="${NEWPIN}"
	GNUTLS_SO_PIN="${TMP}" \
	GNUTLS_NEW_SO_PIN="${SOPIN}" \
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-so-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Setting too large SO PIN... "
	GNUTLS_NEW_SO_PIN="${TOO_LARGE_NEWPIN}" \
	GNUTLS_SO_PIN="${SOPIN}" \
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-so-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Setting large SO PIN... "
	GNUTLS_NEW_SO_PIN="${LARGE_NEWPIN}" \
	GNUTLS_SO_PIN="${SOPIN}" \
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-so-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	# reset back
	echo -n "* Re-setting SO PIN... "
	TMP="${LARGE_NEWPIN}"
	GNUTLS_SO_PIN="${TMP}" \
	GNUTLS_NEW_SO_PIN="${SOPIN}" \
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-so-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	NEWPIN=977654321
	# Test user PIN
	echo -n "* Setting user PIN... "
	export GNUTLS_SO_PIN="${SOPIN}"
	export GNUTLS_PIN="${NEWPIN}"
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Re-setting user PIN... "
	export GNUTLS_PIN="${UPIN}"
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Setting too large user PIN... "
	export GNUTLS_SO_PIN="${SOPIN}"
	export GNUTLS_PIN="${TOO_LARGE_NEWPIN}"
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? = 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Setting large user PIN... "
	export GNUTLS_SO_PIN="${SOPIN}"
	export GNUTLS_PIN="${LARGE_NEWPIN}"
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok

	echo -n "* Re-setting user PIN... "
	export GNUTLS_PIN="${UPIN}"
	${P11TOOL} ${ADDITIONAL_PARAM} --login --initialize-pin "${token}" >>"${LOGFILE}" 2>&1
	if test $? != 0; then
		echo failed
		exit_error
	fi
	echo ok
}



echo "Testing PKCS11 support"

# erase SC

type="$1"

if test -z "${type}"; then
	echo "usage: $0: [pkcs15|softhsm|sc-hsm]"
	if test -x "/usr/bin/softhsm" || test -x "/usr/bin/softhsm2-util"; then
		echo "assuming 'softhsm'"
		echo ""
		type=softhsm
	else
		exit 77
	fi

fi

. "${srcdir}/testpkcs11.${type}"

export TEST_PIN=12345678
export TEST_SO_PIN=00000001

init_card "${TEST_PIN}" "${TEST_SO_PIN}"


# find token name
TOKEN=`${P11TOOL} ${ADDITIONAL_PARAM} --list-tokens pkcs11:token=Nikos|grep URL|grep token=GnuTLS-Test|sed 's/\s*URL\: //g'`

echo "* Token: ${TOKEN}"
if test "x${TOKEN}" = x; then
	echo "Could not find generated token"
	exit_error
fi

${P11TOOL} ${ADDITIONAL_PARAM} --list-mechanisms ${TOKEN}|grep 25519 >/dev/null
if test $? = 0;then
	have_ed25519=1
fi

${P11TOOL} ${ADDITIONAL_PARAM} --list-mechanisms ${TOKEN} > ${TMPFILE}

# Verify that we output flags correctly
if grep AES_CTR ${TMPFILE} | grep -v "keysize range (16, 32)" ; then
	echo "Keysize range error"
	exit_error
fi

if grep AES_CTR ${TMPFILE} | grep -v "encrypt decrypt" ; then
	echo "Error with encrypt/decrypt flags"
	exit_error
fi

if grep KEY_WRAP ${TMPFILE} | grep -v "wrap.unwrap" ; then
	echo "Error with wrap/unwrap flags"
	exit_error
fi

if grep AES_CMAC ${TMPFILE} | grep -v "sign verify" ; then
	echo "Error with sign/verify flags"
	exit_error
fi

if grep "CKM_SHA256 " ${TMPFILE} | grep -v "digest" ; then
	echo "Error with digest flags"
	exit_error
fi

reset_pins "${TOKEN}" "${TEST_PIN}" "${TEST_SO_PIN}"

#write a given privkey
write_privkey "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/client.key"

generate_temp_ecc_privkey "${TOKEN}" "${TEST_PIN}" 256
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ecc-256

generate_temp_ecc_privkey_no_login "${TOKEN}" "${TEST_PIN}" 256
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ecc-no-256

generate_temp_ecc_privkey "${TOKEN}" "${TEST_PIN}" 384
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ecc-384

if test $have_ed25519 != 0;then
	generate_temp_ed25519_privkey "${TOKEN}" "${TEST_PIN}" ed25519
	delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ed25519
fi

generate_temp_rsa_privkey "${TOKEN}" "${TEST_PIN}" 2048
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" rsa-2048

generate_temp_dsa_privkey "${TOKEN}" "${TEST_PIN}" 3072
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" dsa-3072

import_temp_rsa_privkey "${TOKEN}" "${TEST_PIN}" 1024
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" rsa-1024
import_temp_ecc_privkey "${TOKEN}" "${TEST_PIN}" 256
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ecc-256
import_temp_dsa_privkey "${TOKEN}" "${TEST_PIN}" 2048
delete_temp_privkey "${TOKEN}" "${TEST_PIN}" dsa-2048

if test $have_ed25519 != 0;then
	import_temp_ed25519_privkey "${TOKEN}" "${TEST_PIN}" ed25519
	delete_temp_privkey "${TOKEN}" "${TEST_PIN}" ed25519
fi

generate_rsa_privkey "${TOKEN}" "${TEST_PIN}" 1024
change_id_of_privkey "${TOKEN}" "${TEST_PIN}"
export_pubkey_of_privkey "${TOKEN}" "${TEST_PIN}"
change_label_of_privkey "${TOKEN}" "${TEST_PIN}"
list_pubkey_as_so "${TOKEN}" "${TEST_SO_PIN}"
list_privkey_without_pin_env "${TOKEN}" "${TEST_PIN}"

write_certificate_test "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/ca.key" "${srcdir}/testpkcs11-certs/ca.crt" tmp-client.pub
write_serv_privkey "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/server.key"
write_serv_cert "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/server.crt"

write_serv_pubkey "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/server.crt"
test_sign "${TOKEN}" "${TEST_PIN}"

use_certificate_test "${TOKEN}" "${TEST_PIN}" "${TOKEN};object=serv-cert;object-type=cert" "${TOKEN};object=serv-key;object-type=private" "${srcdir}/testpkcs11-certs/ca.crt" "full URLs"

use_certificate_test "${TOKEN}" "${TEST_PIN}" "${TOKEN};object=serv-cert" "${TOKEN};object=serv-key" "${srcdir}/testpkcs11-certs/ca.crt" "abbrv URLs"

write_certificate_id_test_rsa "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/ca.key" "${srcdir}/testpkcs11-certs/ca.crt"
write_certificate_id_test_rsa2 "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/ca.key" "${srcdir}/testpkcs11-certs/ca.crt"
write_certificate_id_test_ecdsa "${TOKEN}" "${TEST_PIN}" "${srcdir}/testpkcs11-certs/ca.key" "${srcdir}/testpkcs11-certs/ca.crt"

test_delete_cert "${TOKEN}" "${TEST_PIN}"

test_sign_set_pin "${TOKEN}" "${TEST_PIN}"

if test ${RETCODE} = 0; then
	echo "* All smart cards tests succeeded"
fi
rm -f tmp-client.crt tmp-client.pub tmp-client-2.pub "${LOGFILE}" "${TMPFILE}"

exit 0
