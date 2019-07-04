#!/bin/sh

# Copyright (C) 2018 IBM Corporation
#
# Author: Stefan Berger
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
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
TPMTOOL="${TPMTOOL:-../src/tpmtool${EXEEXT}}"

if [ "$(id -u)" -ne 0 ]; then
	echo "Need to be root to run this test."
	exit 77
fi

if [ -z "$(which swtpm 2>/dev/null)" ]; then
	echo "Need swtpm package to run this test."
	exit 77
fi

if [ -z "$(which tcsd 2>/dev/null)" ]; then
	echo "Need tcsd (TrouSerS) package to run this test."
	exit 77
fi

if [ -z "$(which tpm_createek 2>/dev/null)" ]; then
	echo "Need tpm_createek from tpm-tools package to run this test."
	exit 77
fi

if [ -z "$(which ncat 2>/dev/null)" ]; then
	echo "Need ncat from nmap-ncat package to run this test."
	exit 77
fi

if [ -z "$(which expect 2>/dev/null)" ]; then
	echo "Need expect from expect package to run this test."
	exit 77
fi

$TPMTOOL --help >/dev/null
if [ $? -ne 0 ]; then
	echo "tpmtool cannot show help screen (TPMTOOL=$TPMTOOL)."
	exit 77
fi

$CERTTOOL --help >/dev/null
if [ $? -ne 0 ]; then
	echo "certtool cannot show help screen (CERTTOOL=$CERTTOOL)."
	exit 77
fi

. "${srcdir}/scripts/common.sh"

workdir=$(mktemp -d)

SWTPM_SERVER_PORT=12345
SWTPM_CTRL_PORT=$((SWTPM_SERVER_PORT + 1))
SWTPM_PIDFILE=${workdir}/swtpm.pid
TCSD_LISTEN_PORT=12347
export TSS_TCSD_PORT=$TCSD_LISTEN_PORT

cleanup()
{
	stop_tcsd
	if [ -n "$workdir" ]; then
		rm -rf $workdir
	fi
}

start_swtpm()
{
	local workdir="$1"

	local res

	swtpm socket \
		--flags not-need-init \
		--pid file=$SWTPM_PIDFILE \
		--tpmstate dir=$workdir \
		--server type=tcp,port=$SWTPM_SERVER_PORT,disconnect \
		--ctrl type=tcp,port=$SWTPM_CTRL_PORT &

	if wait_for_file $SWTPM_PIDFILE 3; then
		echo "Starting the swtpm failed"
		return 1
	fi

	SWTPM_PID=$(cat $SWTPM_PIDFILE)
	kill -0 ${SWTPM_PID}
	if [ $? -ne 0 ]; then
		echo "swtpm must have terminated"
		return 1
	fi

	# Send TPM_Startup to TPM
	res="$(/bin/echo -en '\x00\xC1\x00\x00\x00\x0C\x00\x00\x00\x99\x00\x01' |
		ncat localhost ${SWTPM_SERVER_PORT} | od -tx1 -An)"
	exp=' 00 c4 00 00 00 0a 00 00 00 00'
	if [ "$res" != "$exp" ]; then
		echo "Did not get expected response from TPM_Startup(ST_CLEAR)"
		echo "expected: $exp"
		echo "received: $res"
		return 1
	fi

	return 0
}

stop_swtpm()
{
	if [ -n "$SWTPM_PID" ]; then
		terminate_proc $SWTPM_PID
		unset SWTPM_PID
	fi
}

start_tcsd()
{
	local workdir="$1"

	local tcsd_conf=$workdir/tcsd.conf
	local tcsd_system_ps_file=$workdir/system_ps_file
	local tcsd_pidfile=$workdir/tcsd.pid

	start_swtpm "$workdir"
	[ $? -ne 0 ] && return 1

	cat <<_EOF_ > $tcsd_conf
port = $TCSD_LISTEN_PORT
system_ps_file = $tcsd_system_ps_file
_EOF_

	chown tss:tss $tcsd_conf
	chmod 0600 $tcsd_conf

	bash -c "TCSD_USE_TCP_DEVICE=1 TCSD_TCP_DEVICE_PORT=$SWTPM_SERVER_PORT tcsd -c $tcsd_conf -e -f &>/dev/null & echo \$! > $tcsd_pidfile; wait" &
	BASH_PID=$!

	if wait_for_file $tcsd_pidfile 3; then
		echo "Could not get TCSD's PID file"
		return 1
	fi

	TCSD_PID=$(cat $tcsd_pidfile)
	return 0
}

stop_tcsd()
{
	if [ -n "$TCSD_PID" ]; then
		terminate_proc $TCSD_PID
		unset TCSD_PID
	fi
	stop_swtpm
}

run_tpm_takeownership()
{
	local owner_password="$1"
	local srk_password="$2"

	local prg out rc
	local parm_z=""

	if [ -z "$srk_password" ]; then
		parm_z="--srk-well-known"
	fi

	prg="set parm_z \"$parm_z\"
		spawn tpm_takeownership \$parm_z
		expect {
			\"Enter owner password:\"
				{ send \"$owner_password\n\" }
		}
		expect {
			\"Confirm password:\"
				{ send \"$owner_password\n\" }
		}
		if { \$parm_z == \"\" } {
			expect {
				\"Enter SRK password:\"
					{ send \"$srk_password\n\" }
			}
			expect {
				\"Confirm password:\"
					{ send \"$srk_password\n\" }
			}
		}
		expect {
			eof
		}
		catch wait result
		exit [lindex \$result 3]
	"
	out=$(expect -c "$prg")
	rc=$?
	echo "$out"
	return $rc
}

setup_tcsd()
{
	local workdir="$1"
	local owner_password="$2"
	local srk_password="$3"

	local msg

	start_tcsd "$workdir"
	[ $? -ne 0 ] && return 1

	tpm_createek
	[ $? -ne 0 ] && {
		echo "Could not create EK"
		return 1
	}
	msg="$(run_tpm_takeownership "$owner_password" "$srk_password")"
	[ $? -ne 0 ] && {
		echo "Could not take ownership of TPM"
		echo "$msg"
		return 1
	}
	return 0
}

run_tpmtool()
{
	local srk_password="$1"
	local key_password="$2"

	shift 2

	local prg out rc

	prg="spawn $TPMTOOL $@
		expect {
			\"Enter SRK password:\" {
				send \"$srk_password\n\"
				exp_continue
			}
			\"Enter key password:\" {
				send \"$key_password\n\"
				exp_continue
			}
			\"tpmkey:\" {
				exp_continue
			}
			eof
		}
		catch wait result
		exit [lindex \$result 3]
	"
	out=$(expect -c "$prg")
	rc=$?
	echo "$out"
	return $rc
}

tpmtool_test()
{
	local workdir="$1"
	local owner_password="$2"
	local srk_password="$3"
	local key_password="$4"
	local register=$5  # whether to --register the key

	local params msg tpmkeyurl
	local tpmpubkey=${workdir}/tpmpubkey.pem
	local tpmca=${workdir}/tpmca.pem
	local template=${workdir}/template
	local tpmkey=${workdir}/tpmkey.pem # if --register is not used

	setup_tcsd "$workdir" "$owner_password" "$srk_password"
	[ $? -ne 0 ] && return 1

	if [ -z "$srk_password" ]; then
		params="--srk-well-known"
		unset GNUTLS_PIN
	else
		export GNUTLS_PIN="$srk_password"
	fi

	if [ $register -ne 0 ]; then
		# --register key
		msg="$(run_tpmtool "$srk_password" "$key_password" \
			$params --register --generate-rsa --signing)"
		[ $? -ne 0 ] && {
			echo "Could not create TPM signing key"
			echo "$msg"
			return 1
		}
		tpmkeyurl=$(echo "$msg" | sed -n 's/\(tpmkey:uuid=[^;]*\);.*/\1/p')
		[ -z "$tpmkeyurl" ] && {
			echo "Could not get TPM key URL"
			return 1
		}
	else
		msg="$(run_tpmtool "$srk_password" "$key_password" \
			$params --generate-rsa --signing --outfile ${tpmkey})"
		[ $? -ne 0 ] && {
			echo "Could not create TPM signing key"
			echo "$msg"
			return 1
		}
		tpmkeyurl="tpmkey:file=${tpmkey}"
	fi

	if [ $register -ne 0 ]; then
		msg=$(run_tpmtool "$srk_password" "$key_password" \
			$params --test-sign $tpmkeyurl)
		[ $? -ne 0 ] && {
			echo "Could not test sign with key $tpmkeyurl"
			echo "$msg"
			return 1
		}
	fi

	msg=$(run_tpmtool "$srk_password" "$key_password" \
		$params --pubkey "$tpmkeyurl" --outfile "$tpmpubkey")
	[ $? -ne 0 ] && {
		echo "Could not get TPM key's public key"
		echo "$msg"
		return 1
	}

	cat <<_EOF_ >${template}
cn = test
ca
cert_signing_key
expiration_days = 1
_EOF_

	msg=$($CERTTOOL \
		--generate-self-signed \
		--template ${template} \
		--outfile ${tpmca} \
		--load-privkey ${tpmkeyurl} \
		--load-pubkey ${tpmpubkey} 2>&1)
	[ $? -ne 0 ] && {
		echo "Could not create self-signed certificate"
		echo "$msg"
		return 1
	}

	echo "Successfully created TPM root CA cert using key $tpmkeyurl"

	if [ $register -ne 0 ]; then
		if [ -z "$($TPMTOOL --list | grep "${tpmkeyurl}")" ]; then
			echo "TPM key '${tpmkeyurl}' was not found in list of TPM keys"
			return 1
		fi

		msg=$(run_tpmtool "$srk_password" "$key_password" \
			$params --delete $tpmkeyurl)
		[ $? -ne 0 ] && {
			echo "Could not delete TPM key ${tpmkeyurl}"
			echo "$msg"
			return 1
		}

		if [ -n "$($TPMTOOL --list | grep "$tpmkeyurl")" ]; then
			echo "TPM key '$tpmkeyurl' was not properly deleted"
			return 1
		fi
	fi

	stop_tcsd
}

run_tests()
{
	local workdir="$1"

	[ -z "$workdir" ] && {
		echo "No workdir"
		return 1
	}
	local srk_password key_password
	local owner_password="owner"
	local register

	register=1
	# Test with --register; key password is not needed
	for srk_password in "" "s"; do
		tpmtool_test "$workdir" "$owner_password" "$srk_password" "" "$register"
		[ $? -ne 0 ] && return 1
		stop_tcsd
		rm ${workdir}/*
	done

	# Test without --register; the key needs a password, but it has to be the same as the
	# srk_password due to a bug in TrouSerS
	register=0
	for srk_password in "s"; do
		key_password=$srk_password
		tpmtool_test "$workdir" "$owner_password" "$srk_password" "$key_password" "$register"
		[ $? -ne 0 ] && return 1
		stop_tcsd
		rm ${workdir}/*
	done

	echo "Ok"

	return 0
}

trap "cleanup" EXIT QUIT

run_tests "$workdir"
exit $?
