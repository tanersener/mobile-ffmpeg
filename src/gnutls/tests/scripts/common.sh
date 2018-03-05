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
GETPORT='rc=0; myrandom=$(date +%N | sed s/^0*//)
    while test $rc = 0;do
	PORT="$(((($$<<15)|$myrandom) % 63001 + 2000))"
	check_if_port_in_use $PORT;rc=$?
    done
'

check_for_datefudge() {
	TSTAMP=`datefudge -s "2006-09-23" date -u +%s || true`
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

wait_for_port()
{
	local ret
	local PORT="$1"
	sleep 4

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
			sleep 20
		fi
	done
	return $ret
}

launch_server() {
	PARENT="$1"
	shift

	wait_for_free_port ${PORT}
	${SERV} ${DEBUG} -p "${PORT}" $* >/dev/null 2>&1 &
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
	${SERV} $* >/dev/null 2>&1 &
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

