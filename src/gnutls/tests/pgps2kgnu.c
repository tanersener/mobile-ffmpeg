/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Daniel Kahn Gillmor <dkg@fifthhorseman.net>

 * pgps2kgnu: test GNU extensions to the OpenPGP S2K specification.
 *	    at the moment, we just test the "GNU dummy" S2K
 *	    extension.

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
#include <utils.h>
#include <gnutls/gnutls.h>
#include <gnutls/openpgp.h>

static char dummy_key[] =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
    "Version: GnuPG v1.4.9 (GNU/Linux)\n"
    "\n"
    "lQCVBEO3YdABBACRqqEnucag4+vyZny2M67Pai5+5suIRRvY+Ly8Ms5MvgCi3EVV\n"
    "xT05O/+0ShiRaf+QicCOFrhbU9PZzzU+seEvkeW2UCu4dQfILkmj+HBEIltGnHr3\n"
    "G0yegHj5pnqrcezERURf2e17gGFWX91cXB9Cm721FPXczuKraphKwCA9PwARAQAB\n"
    "/gNlAkdOVQG0OURlbW9uc3RyYXRpb24gS2V5IGZvciBTMksgR05VIGV4dGVuc2lv\n"
    "biAxMDAxIC0tIGdudS1kdW1teYi8BBMBAgAmBQJDt2HQAhsDBQkB4TOABgsJCAcD\n"
    "AgQVAggDBBYCAwECHgECF4AACgkQQZUwSa4UDezTOQP/TMQXUVrWzHYZGopoPZ2+\n"
    "ZS3qddiznBHsgb7MGYg1KlTiVJSroDUBCHIUJvdQKZV9zrzrFl47D07x6hGyUPHV\n"
    "aZXvuITW8t1o5MMHkCy3pmJ2KgfDvdUxrBvLfgPMICA4c6zA0mWquee43syEW9NY\n"
    "g3q61iPlQwD1J1kX1wlimLCdAdgEQ7dh0AEEANAwa63zlQbuy1Meliy8otwiOa+a\n"
    "mH6pxxUgUNggjyjO5qx+rl25mMjvGIRX4/L1QwIBXJBVi3SgvJW1COZxZqBYqj9U\n"
    "8HVT07mWKFEDf0rZLeUE2jTm16cF9fcW4DQhW+sfYm+hi2sY3HeMuwlUBK9KHfW2\n"
    "+bGeDzVZ4pqfUEudABEBAAEAA/0bemib+wxub9IyVFUp7nPobjQC83qxLSNzrGI/\n"
    "RHzgu/5CQi4tfLOnwbcQsLELfker2hYnjsLrT9PURqK4F7udrWEoZ1I1LymOtLG/\n"
    "4tNZ7Mnul3wRC2tCn7FKx8sGJwGh/3li8vZ6ALVJAyOia5TZ/buX0+QZzt6+hPKk\n"
    "7MU1WQIA4bUBjtrsqDwro94DvPj3/jBnMZbXr6WZIItLNeVDUcM8oHL807Am97K1\n"
    "ueO/f6v1sGAHG6lVPTmtekqPSTWBfwIA7CGFvEyvSALfB8NUa6jtk27NCiw0csql\n"
    "kuhCmwXGMVOiryKEfegkIahf2bAd/gnWHPrpWp7bUE20v8YoW22I4wIAhnm5Wr5Q\n"
    "Sy7EHDUxmJm5TzadFp9gq08qNzHBpXSYXXJ3JuWcL1/awUqp3tE1I6zZ0hZ38Ia6\n"
    "SdBMN88idnhDPqPoiKUEGAECAA8FAkO3YdACGyAFCQHhM4AACgkQQZUwSa4UDezm\n"
    "vQP/ZhK+2ly9oI2z7ZcNC/BJRch0/ybQ3haahII8pXXmOThpZohr/LUgoWgCZdXg\n"
    "vP6yiszNk2tIs8KphCAw7Lw/qzDC2hEORjWO4f46qk73RAgSqG/GyzI4ltWiDhqn\n"
    "vnQCFl3+QFSe4zinqykHnLwGPMXv428d/ZjkIc2ju8dRsn4=\n"
    "=CR5w\n" "-----END PGP PRIVATE KEY BLOCK-----\n";

/* Test capability of reading the gnu-dummy OpenPGP S2K extension. 
   See: doc/DETAILS from gnupg
	http://lists.gnu.org/archive/html/gnutls-devel/2008-08/msg00023.html
*/

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

int main(int argc, char **argv)
{
	int rc;
	gnutls_datum_t keydatum =
	    { (unsigned char *) dummy_key, strlen(dummy_key) };
	gnutls_openpgp_privkey_t key;

	if (argc > 1) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(9);
	}

	rc = global_init();
	if (rc) {
		printf("global_init rc %d: %s\n", rc, gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_openpgp_privkey_init(&key);
	if (rc) {
		printf("gnutls_openpgp_privkey_init rc %d: %s\n",
			rc, gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_openpgp_privkey_import(key, &keydatum,
					   GNUTLS_OPENPGP_FMT_BASE64, NULL,
					   0);
	if (rc) {
		printf("gnutls_openpgp_privkey_import rc %d: %s\n",
			rc, gnutls_strerror(rc));
		return 1;
	}

	gnutls_openpgp_privkey_deinit(key);

	gnutls_global_deinit();

	return 0;
}
