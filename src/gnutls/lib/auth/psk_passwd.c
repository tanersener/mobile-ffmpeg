/*
 * Copyright (C) 2005-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Functions for operating in an PSK passwd file are included here */

#include "gnutls_int.h"

#ifdef ENABLE_PSK

#include "x509_b64.h"
#include "errors.h"
#include <auth/psk_passwd.h>
#include <auth/psk.h>
#include "auth.h"
#include "dh.h"
#include "debug.h"
#include <str.h>
#include <datum.h>
#include <num.h>
#include <random.h>


/* this function parses passwd.psk file. Format is:
 * string(username):hex(passwd)
 */
static int pwd_put_values(gnutls_datum_t * psk, char *str)
{
	char *p;
	int len, ret;
	gnutls_datum_t tmp;

	p = strchr(str, ':');
	if (p == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	/* skip username
	 */

	/* read the key
	 */
	len = strlen(p);
	if (p[len - 1] == '\n' || p[len - 1] == ' ')
		len--;

	tmp.data = (void*)p;
	tmp.size = len;
	ret = gnutls_hex_decode2(&tmp, psk);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

}


/* Randomizes the given password entry. It actually sets a random password. 
 * Returns 0 on success.
 */
static int _randomize_psk(gnutls_datum_t * psk)
{
	int ret;

	psk->data = gnutls_malloc(16);
	if (psk->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	psk->size = 16;

	ret = gnutls_rnd(GNUTLS_RND_NONCE, (char *) psk->data, 16);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/* Returns the PSK key of the given user. 
 * If the user doesn't exist a random password is returned instead.
 */
int
_gnutls_psk_pwd_find_entry(gnutls_session_t session, char *username,
			   gnutls_datum_t * psk)
{
	gnutls_psk_server_credentials_t cred;
	FILE *fd;
	char *line = NULL;
	size_t line_size = 0;
	unsigned i, len;
	int ret;

	cred = (gnutls_psk_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* if the callback which sends the parameters is
	 * set, use it.
	 */
	if (cred->pwd_callback != NULL) {
		ret = cred->pwd_callback(session, username, psk);

		if (ret == 1) {	/* the user does not exist */
			ret = _randomize_psk(psk);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
			return 0;
		}

		if (ret < 0) {
			gnutls_assert();
			return GNUTLS_E_SRP_PWD_ERROR;
		}

		return 0;
	}

	/* The callback was not set. Proceed.
	 */
	if (cred->password_file == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_ERROR;
	}

	/* Open the selected password file.
	 */
	fd = fopen(cred->password_file, "r");
	if (fd == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_ERROR;
	}

	len = strlen(username);
	while (getline(&line, &line_size, fd) > 0) {
		/* move to first ':' */
		i = 0;
		while ((i < line_size) && (line[i] != '\0')
		       && (line[i] != ':')) {
			i++;
		}

		if (strncmp(username, line, MAX(i, len)) == 0) {
			ret = pwd_put_values(psk, line);
			if (ret < 0) {
				gnutls_assert();
				ret = GNUTLS_E_SRP_PWD_ERROR;
				goto cleanup;
			}
			ret = 0;
			goto cleanup;
		}
	}

	/* user was not found. Fake him. 
	 */
	ret = _randomize_psk(psk);
	if (ret < 0) {
		goto cleanup;
	}

	ret = 0;
cleanup:
	if (fd != NULL)
		fclose(fd);

	zeroize_key(line, line_size);
	free(line);

	return ret;

}


#endif				/* ENABLE PSK */
