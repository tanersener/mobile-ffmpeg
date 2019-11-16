# Copyright (C) 2011-2016 Free Software Foundation, Inc.
# Copyright (C) 2015-2016 Red Hat, Inc.
#
# This file is part of GnuTLS.
#
# The launch_server() function was contributed by Cedric Arbogast.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

export TZ="UTC"

# Check for a utility to list ports.  Both ss and netstat will list
# ports for normal users, and have similar semantics, so put the
# command in the caller's PFCMD, or exit, indicating an unsupported
# test.  Prefer ss from iproute2 over the older netstat.
have_port_finder() {
	for file in $(which ss 2> /dev/null) /*bin/ss /usr/*bin/ss /usr/local/*bin/ss;do
		if test -x "$file";then
			PFCMD="$file";return 0
		fi
	done

	if test -z "$PFCMD";then
	for file in $(which netstat 2> /dev/null) /bin/netstat /usr/bin/netstat /usr/local/bin/netstat;do
		if test -x "$file";then
			PFCMD="$file";return 0
		fi
	done
	fi

	if test -z "$PFCMD";then
		echo "neither ss nor netstat found"
		exit 1
	fi
}

check_if_port_in_use() {
	local PORT="$1"
	local PFCMD; have_port_finder
	$PFCMD -an|grep "[\:\.]$PORT" >/dev/null 2>&1
}

check_if_port_listening() {
	local PORT="$1"
	local PFCMD; have_port_finder
	$PFCMD -anl|grep "[\:\.]$PORT"|grep LISTEN >/dev/null 2>&1
}

# Find a port number not currently in use.
GETPORT='
    rc=0
    unset myrandom
    while test $rc = 0; do
        if test -n "$RANDOM"; then myrandom=$(($RANDOM + $RANDOM)); fi
        if test -z "$myrandom"; then myrandom=$(date +%N | sed s/^0*//); fi
        if test -z "$myrandom"; then myrandom=0; fi
        PORT="$(((($$<<15)|$myrandom) % 63001 + 2000))"
        check_if_port_in_use $PORT;rc=$?
    done
'

check_for_datefudge() {
	# On certain platforms running datefudge date fails (e.g., x86 datefudge
	# with x86-64 date app).
	if test "${SKIP_DATEFUDGE_CHECK}" = 1;then
		return
	fi

	TSTAMP=`datefudge -s "2006-09-23" "${top_builddir}/tests/datefudge-check" || true`
	if test "$TSTAMP" != "1158969600" || test "$WINDOWS" = 1; then
	echo $TSTAMP
		echo "You need datefudge to run this test"
		exit 77
	fi
}

fail() {
   PID="$1"
   shift
   echo "Failure: $1" >&2
   [ -n "${PID}" ] && kill ${PID}
   exit 1
}

exit_if_non_x86()
{
which lscpu >/dev/null 2>&1
if test $? = 0;then
        $(which lscpu)|grep Architecture|grep x86
        if test $? != 0;then
                echo "non-x86 CPU detected"
                exit 0
        fi
fi
}

exit_if_non_padlock()
{
which lscpu >/dev/null 2>&1
if test $? = 0;then
        $(which lscpu)|grep Flags|grep phe
        if test $? != 0;then
                echo "non-Via padlock CPU detected"
                exit 0
        fi
fi
}

wait_for_port()
{
	local ret
	local PORT="$1"
	sleep 1

	for i in 1 2 3 4 5 6;do
		check_if_port_listening ${PORT}
		ret=$?
		if test $ret != 0;then
		check_if_port_in_use ${PORT}
			echo try $i
			sleep 2
		else
			break
		fi
	done
	return $ret
}

wait_for_free_port()
{
	local ret
	local PORT="$1"

	for i in 1 2 3 4 5 6;do
		check_if_port_in_use ${PORT}
		ret=$?
		if test $ret != 0;then
			break
		else
			sleep 2
		fi
	done
	return $ret
}

launch_server() {
	PARENT="$1"
	shift

	wait_for_free_port ${PORT}
	${SERV} ${DEBUG} -p "${PORT}" $* >${LOGFILE-/dev/null} &
}

launch_pkcs11_server() {
	PARENT="$1"
	shift
	PROVIDER="$1"
	shift

	wait_for_free_port ${PORT}

	${VALGRIND} ${SERV} ${PROVIDER} ${DEBUG} -p "${PORT}" $* &
}

launch_bare_server() {
	PARENT="$1"
	shift

	wait_for_free_port ${PORT}
	${SERV} $* >${LOGFILE-/dev/null} &
}

wait_server() {
	local PID=$1
	trap "test -n \"${PID}\" && kill ${PID};exit 1" 1 15 2
	wait_for_port $PORT
	if test $? != 0;then
		echo "Server $PORT did not come up"
		kill $PID
		exit 1
	fi
}

wait_udp_server() {
	local PID=$1
	trap "test -n \"${PID}\" && kill ${PID};exit 1" 1 15 2
	sleep 4
}

if test -x /usr/bin/lockfile-create;then
LOCKFILE="lockfile-create global"
UNLOCKFILE="lockfile-remove global"
else
LOCKFILE="lockfile global.lock"
UNLOCKFILE="rm -f global.lock"
fi

create_testdir() {
	local PREFIX=$1
	d=`mktemp -d -t ${PREFIX}.XXXXXX`
	if test $? -ne 0; then
		d=${TMPDIR}/${PREFIX}.$$
		mkdir "$d" || exit 1
	fi
	trap "test -e \"$d\" && rm -rf \"$d\"" 1 15 2
	echo "$d"
}

wait_for_file() {
	local filename="$1"
	local timeout="$2"

	local loops=$((timeout * 10)) loop=0

	while test $loop -lt $loops; do
		[ -f "$filename" ] && {
			#allow file to be written to
			sleep 0.2
			return 1
		}
		sleep 0.1
		loop=$((loop+1))
	done
	return 0
}

# Kill a process quietly
# @1: signal, e.g. -9
# @2: pid
kill_quiet() {
	local sig="$1"
	local pid="$2"

	sh -c "kill $sig $pid 2>/dev/null"
	return $?
}

# Terminate a process first using SIGTERM, wait 1s and if still avive use
# SIGKILL
# @1: pid
terminate_proc() {
	local pid="$1"

	local ctr=0

	kill_quiet -15 $pid
	while [ $ctr -lt 10 ]; do
		sleep 0.1
		kill -0 $pid 2>/dev/null
		[ $? -ne 0 ] && return
		ctr=$((ctr + 1))
	done
	kill_quiet -9 $pid
	sleep 0.1
}

# $1, $2: the two files to check for equality
# $3: Strings to be ignored, separated by |
check_if_equal() {
	if test -n "$3"; then
		local tmp1=`basename "$1"`"1.tmp"
		local tmp2=`basename "$2"`"2.tmp"
		egrep -v "$3" "$1" | tr -d '\r' >"$tmp1"
		egrep -v "$3" "$2" | tr -d '\r' >"$tmp2"
		diff -b -B "$tmp1" "$tmp2"
		local rc=$?
		rm -f "$tmp1" "$tmp2"
		return $rc
	fi

	diff -b -B "$1" "$2"
	return $?
}
