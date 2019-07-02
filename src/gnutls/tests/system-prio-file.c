/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

#include "utils.h"

/* This test verifies the correct operation of system-wide priority
 * strings. The test suite sets the GNUTLS_SYSTEM_PRIORITY_FILE environment
 * variable to specify the test file (system.prio).
 */

char *_gnutls_resolve_priorities(const char* priorities);

static void
try_prio(const char *prio, const char *expected_str)
{
	char *p;

	/* this must be called once in the program
	 */
	global_init();

	p = _gnutls_resolve_priorities(prio);
	if (p == NULL && expected_str == NULL)
		goto ok;

	if (p == NULL || expected_str == NULL || strcmp(p, expected_str) != 0) {
		fail("test: %s: error; got: %s, expected: %s\n", prio, p, expected_str);
		exit(1);
	}

 ok:
	gnutls_free(p);
	gnutls_global_deinit();
}

void doit(void)
{
	try_prio("NORMAL", "NORMAL");
	try_prio("SUITEB192", "SUITEB192");
	try_prio("@HELLO1", "NORMAL");
	try_prio("@HELLO1:+AES-256-CBC:+AEAD", "NORMAL:+AES-256-CBC:+AEAD");
	try_prio("@HELLO2", "NORMAL:+AES-128-CBC");
	try_prio("@HELLO3", "NONE:+VERS-TLS-ALL:-VERS-SSL3.0:+AEAD:+SHA1:+SHA256:+SHA384:+ECDHE-RSA:+ECDHE-ECDSA:+RSA:+DHE-RSA:+DHE-DSS:+AES-256-GCM:+AES-256-CBC:+CAMELLIA-256-GCM:+CAMELLIA-256-CBC:+AES-128-GCM:+AES-128-CBC:+CAMELLIA-128-GCM:+CAMELLIA-128-CBC:+3DES-CBC:+SIGN-ALL:-SIGN-RSA-MD5:+CURVE-ALL:+COMP-NULL:%PROFILE_LOW");
	try_prio("@HELLO1,HELLO2", "NORMAL");
	try_prio("@HELLO1,HELLO2:+AES-128-CBC", "NORMAL:+AES-128-CBC");
	try_prio("@HELLO1,HELLO1", "NORMAL");
	try_prio("@HELLO1,", "NORMAL");
	try_prio("@HELLO2,HELLO1", "NORMAL:+AES-128-CBC");
	try_prio("@HELLO2,HELLO1,@HELLONO", "NORMAL:+AES-128-CBC");
	try_prio("@HELLO2,HELLO1,@HELLO3", "NORMAL:+AES-128-CBC");
	try_prio("@HELLONO,HELLO1", "NORMAL");
	try_prio("@HELLONO,HELLONO2,HELLO1", "NORMAL");
	try_prio("@HELLONO,HELLONO2,HELLO1:+AES-128-CBC", "NORMAL:+AES-128-CBC");
	try_prio("@HELLONO", NULL);
	try_prio("@HELLONO,", NULL);
	try_prio("@HELLONO:+AES-128-CBC", NULL);
	try_prio("@HELLONO,:+AES-128-CBC", NULL);
}
