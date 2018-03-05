## Process this file with automake to produce Makefile.in
# Copyright (C) 2007-2008, 2010, 2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
#
# This file is part of GnuTLS.
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

EXTRA_DIST = certs/ca.key certs/ca.pem certs/ocsp-server.key certs/ocsp-server.pem response1.der \
	response2.der certs/ocsp_index.txt certs/ocsp_index.txt.attr \
	certs/server_good.key certs/server_bad.key certs/server_good.template \
	certs/server_bad.template certs/ocsp-staple-unrelated.der suppressions.valgrind \
	certs/ocsp-akamai.com.der certs/cert-akamai.com.pem

dist_check_SCRIPTS =

if !WINDOWS
dist_check_SCRIPTS += ocsp-tls-connection ocsp-must-staple-connection ocsp-test \
	ocsp-ecdsa-test
endif

TESTS = $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT) \
	LC_ALL="C"		\
	VALGRIND="$(VALGRIND)"	\
	LIBTOOL="$(LIBTOOL)"	\
	top_builddir="$(top_builddir)"	\
	srcdir="$(srcdir)"

if WINDOWS
TESTS_ENVIRONMENT += WINDOWS=1
endif
