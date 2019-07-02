/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* Functions for operating in an SRP passwd file are included here */

#include "gnutls_int.h"

#ifdef ENABLE_SRP

#include "x509_b64.h"
#include "errors.h"
#include <auth/srp_passwd.h>
#include <auth/srp_kx.h>
#include "auth.h"
#include "srp.h"
#include "dh.h"
#include "debug.h"
#include <str.h>
#include <datum.h>
#include <num.h>
#include <random.h>
#include <algorithms.h>

static int _randomize_pwd_entry(SRP_PWD_ENTRY * entry,
				gnutls_srp_server_credentials_t cred,
				const char * username);

/* this function parses tpasswd.conf file. Format is:
 * string(username):base64(v):base64(salt):int(index)
 */
static int parse_tpasswd_values(SRP_PWD_ENTRY * entry, char *str)
{
	char *p;
	int len, ret;
	uint8_t *verifier;
	size_t verifier_size;
	int indx;

	p = strrchr(str, ':');	/* we have index */
	if (p == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	indx = atoi(p);
	if (indx == 0) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	/* now go for salt */
	p = strrchr(str, ':');	/* we have salt */
	if (p == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	len = strlen(p);

	entry->salt.size =
	    _gnutls_sbase64_decode(p, len, &entry->salt.data);

	if (entry->salt.size <= 0) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	/* now go for verifier */
	p = strrchr(str, ':');	/* we have verifier */
	if (p == NULL) {
		_gnutls_free_datum(&entry->salt);
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	len = strlen(p);
	ret = _gnutls_sbase64_decode(p, len, &verifier);
	if (ret <= 0) {
		gnutls_assert();
		_gnutls_free_datum(&entry->salt);
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	verifier_size = ret;
	entry->v.data = verifier;
	entry->v.size = verifier_size;

	/* now go for username */
	*p = '\0';

	entry->username = gnutls_strdup(str);
	if (entry->username == NULL) {
		_gnutls_free_datum(&entry->salt);
		_gnutls_free_key_datum(&entry->v);
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return indx;
}


/* this function parses tpasswd.conf file. Format is:
 * int(index):base64(n):int(g)
 */
static int parse_tpasswd_conf_values(SRP_PWD_ENTRY * entry, char *str)
{
	char *p;
	int len;
	uint8_t *tmp;
	int ret;

	p = strrchr(str, ':');	/* we have g */
	if (p == NULL) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	/* read the generator */
	len = strlen(p);
	if (p[len - 1] == '\n' || p[len - 1] == ' ')
		len--;
	ret = _gnutls_sbase64_decode(p, len, &tmp);

	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	entry->g.data = tmp;
	entry->g.size = ret;

	/* now go for n - modulo */
	p = strrchr(str, ':');	/* we have n */
	if (p == NULL) {
		_gnutls_free_datum(&entry->g);
		gnutls_assert();
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	*p = '\0';
	p++;

	len = strlen(p);
	ret = _gnutls_sbase64_decode(p, len, &tmp);

	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(&entry->g);
		return GNUTLS_E_SRP_PWD_PARSING_ERROR;
	}

	entry->n.data = tmp;
	entry->n.size = ret;

	return 0;
}


/* this function opens the tpasswd.conf file and reads the g and n
 * values. They are put in the entry.
 */
static int
pwd_read_conf(const char *pconf_file, SRP_PWD_ENTRY * entry, int idx)
{
	FILE *fd;
	char *line = NULL;
	size_t line_size = 0;
	unsigned i, len;
	char indexstr[10];
	int ret;

	snprintf(indexstr, sizeof(indexstr), "%u", (unsigned int) idx);

	fd = fopen(pconf_file, "r");
	if (fd == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	len = strlen(indexstr);
	while (getline(&line, &line_size, fd) > 0) {
		/* move to first ':' */
		i = 0;
		while ((i < line_size) && (line[i] != ':')
		       && (line[i] != '\0')) {
			i++;
		}

		if (strncmp(indexstr, line, MAX(i, len)) == 0) {
			if ((idx =
			     parse_tpasswd_conf_values(entry,
						       line)) >= 0) {
				ret = 0;
				goto cleanup;
			} else {
				ret = GNUTLS_E_SRP_PWD_ERROR;
				goto cleanup;
			}
		}
	}
	ret = GNUTLS_E_SRP_PWD_ERROR;

cleanup:
	zeroize_key(line, line_size);
	free(line);
	fclose(fd);
	return ret;

}

int
_gnutls_srp_pwd_read_entry(gnutls_session_t state, char *username,
			   SRP_PWD_ENTRY ** _entry)
{
	gnutls_srp_server_credentials_t cred;
	FILE *fd = NULL;
	char *line = NULL;
	size_t line_size = 0;
	unsigned i, len;
	int ret;
	int idx;
	SRP_PWD_ENTRY *entry = NULL;

	*_entry = gnutls_calloc(1, sizeof(SRP_PWD_ENTRY));
	if (*_entry == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	entry = *_entry;

	cred = (gnutls_srp_server_credentials_t)
	    _gnutls_get_cred(state, GNUTLS_CRD_SRP);
	if (cred == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		goto cleanup;
	}

	/* if the callback which sends the parameters is
	 * set, use it.
	 */
	if (cred->pwd_callback != NULL) {
		ret = cred->pwd_callback(state, username, &entry->salt,
					 &entry->v, &entry->g, &entry->n);

		if (ret == 1) {	/* the user does not exist */
			if (entry->g.size != 0 && entry->n.size != 0) {
				ret = _randomize_pwd_entry(entry, cred, username);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
				return 0;
			} else {
				gnutls_assert();
				ret = -1;	/* error in the callback */
			}
		}

		if (ret < 0) {
			gnutls_assert();
			ret = GNUTLS_E_SRP_PWD_ERROR;
			goto cleanup;
		}

		return 0;
	}

	/* The callback was not set. Proceed.
	 */

	if (cred->password_file == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_SRP_PWD_ERROR;
		goto cleanup;
	}

	/* Open the selected password file.
	 */
	fd = fopen(cred->password_file, "r");
	if (fd == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_SRP_PWD_ERROR;
		goto cleanup;
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
			if ((idx = parse_tpasswd_values(entry, line)) >= 0) {
				/* Keep the last index in memory, so we can retrieve fake parameters (g,n)
				 * when the user does not exist.
				 */
				if (pwd_read_conf
				    (cred->password_conf_file, entry,
				     idx) == 0) {
					ret = 0;
					goto found;
				} else {
					gnutls_assert();
					ret = GNUTLS_E_SRP_PWD_ERROR;
					goto cleanup;
				}
			} else {
				gnutls_assert();
				ret = GNUTLS_E_SRP_PWD_ERROR;
				goto cleanup;
			}
		}
	}

	/* user was not found. Fake him. Actually read the g,n values from
	 * the last index found and randomize the entry.
	 */
	if (pwd_read_conf(cred->password_conf_file, entry, 1) == 0) {
		ret = _randomize_pwd_entry(entry, cred, username);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = 0;
		goto found;
	}

	ret = GNUTLS_E_SRP_PWD_ERROR;

cleanup:
	gnutls_assert();
	_gnutls_srp_entry_free(entry);

found:
	zeroize_key(line, line_size);
	free(line);
	if (fd)
		fclose(fd);
	return ret;
}

/* Randomizes the given password entry. It actually sets the verifier
 * to random data and sets the salt based on fake_salt_seed and
 * username. Returns 0 on success.
 */
static int _randomize_pwd_entry(SRP_PWD_ENTRY * entry,
				gnutls_srp_server_credentials_t sc,
				const char * username)
{
	int ret;
	const mac_entry_st *me = mac_to_entry(SRP_FAKE_SALT_MAC);
	mac_hd_st ctx;
	size_t username_len = strlen(username);

	if (entry->g.size == 0 || entry->n.size == 0) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	entry->v.data = gnutls_malloc(20);
	entry->v.size = 20;
	if (entry->v.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = gnutls_rnd(GNUTLS_RND_NONCE, entry->v.data, 20);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* Always allocate and work with the output size of the MAC,
	 * even if they don't need salts that long, for convenience.
	 *
	 * In case an error occurs 'entry' (and the salt inside)
	 * is deallocated by our caller: _gnutls_srp_pwd_read_entry().
	 */
	entry->salt.data = gnutls_malloc(me->output_size);
	if (entry->salt.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = _gnutls_mac_init(&ctx, me, sc->fake_salt_seed.data,
			sc->fake_salt_seed.size);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_mac(&ctx, "salt", 4);
	_gnutls_mac(&ctx, username, username_len);
	_gnutls_mac_deinit(&ctx, entry->salt.data);

	/* Set length to the actual number of bytes they asked for.
	 * This is always less than or equal to the output size of
	 * the MAC, enforced by gnutls_srp_set_server_fake_salt_seed().
	 */
	entry->salt.size = sc->fake_salt_length;

	return 0;
}

/* Free all the entry parameters, except if g and n are
 * the static ones defined in gnutls.h
 */
void _gnutls_srp_entry_free(SRP_PWD_ENTRY * entry)
{
	_gnutls_free_key_datum(&entry->v);
	_gnutls_free_datum(&entry->salt);

	if ((entry->g.data != gnutls_srp_1024_group_generator.data) &&
	    (entry->g.data != gnutls_srp_1536_group_generator.data) &&
	    (entry->g.data != gnutls_srp_2048_group_generator.data) &&
	    (entry->g.data != gnutls_srp_3072_group_generator.data) &&
	    (entry->g.data != gnutls_srp_4096_group_generator.data) &&
	    (entry->g.data != gnutls_srp_8192_group_generator.data))
		_gnutls_free_datum(&entry->g);

	if (entry->n.data != gnutls_srp_1024_group_prime.data &&
	    entry->n.data != gnutls_srp_1536_group_prime.data &&
	    entry->n.data != gnutls_srp_2048_group_prime.data &&
	    entry->n.data != gnutls_srp_3072_group_prime.data &&
	    entry->n.data != gnutls_srp_4096_group_prime.data &&
	    entry->n.data != gnutls_srp_8192_group_prime.data)
		_gnutls_free_datum(&entry->n);

	gnutls_free(entry->username);
	gnutls_free(entry);
}

#endif				/* ENABLE SRP */
