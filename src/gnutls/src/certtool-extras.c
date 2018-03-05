/*
 * Copyright (C) 2012 Lucas Fisher *lucas.fisher [at] gmail.com*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/openpgp.h>
#include <gnutls/pkcs12.h>
#include <gnutls/pkcs11.h>
#include <gnutls/abstract.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "certtool-common.h"
#include "certtool-cfg.h"


#define MAX_KEYS 256

/* Loads a x509 private key list
 */
gnutls_x509_privkey_t *load_privkey_list(int mand, size_t * privkey_size,
					 common_info_st * info)
{
	static gnutls_x509_privkey_t key[MAX_KEYS];
	char *ptr;
	int ret, i;
	gnutls_datum_t dat, file_data;
	int ptr_size;
	unsigned int flags = 0;
	const char *pass;

	*privkey_size = 0;
	fprintf(stderr, "Loading private key list...\n");

	if (info->privkey == NULL) {
		if (mand) {
			fprintf(stderr, "missing --load-privkey");
			exit(1);
		} else
			return NULL;
	}

	ret = gnutls_load_file(info->privkey, &file_data);
	if (ret < 0) {
		fprintf(stderr, "%s", info->privkey);
		exit(1);
	}

	ptr = (void *) file_data.data;
	ptr_size = file_data.size;

	for (i = 0; i < MAX_KEYS; i++) {
		ret = gnutls_x509_privkey_init(&key[i]);
		if (ret < 0) {
			fprintf(stderr, "privkey_init: %s",
				gnutls_strerror(ret));
			exit(1);
		}

		dat.data = (void *) ptr;
		dat.size = ptr_size;

		ret =
		    gnutls_x509_privkey_import2(key[i], &dat,
						info->incert_format, NULL,
						0);
		if (ret == GNUTLS_E_DECRYPTION_FAILED) {
			pass = get_password(info, &flags, 0);
			ret =
			    gnutls_x509_privkey_import2(key[i], &dat,
							info->
							incert_format,
							pass, flags);
		}

		if (ret < 0 && *privkey_size > 0)
			break;
		if (ret < 0) {
			fprintf(stderr, "privkey_import: %s",
				gnutls_strerror(ret));
			exit(1);
		}

		(*privkey_size)++;

		if (info->incert_format != GNUTLS_X509_FMT_PEM)
			break;

		ptr = strstr(ptr, "---END");
		if (ptr == NULL)
			break;
		ptr++;

		ptr_size = file_data.size;
		ptr_size -=
		    ((unsigned char *) ptr -
				    (unsigned char *) file_data.data);

		if (ptr_size < 0)
			break;

	}

	gnutls_free(file_data.data);
	fprintf(stderr, "Loaded %d private keys.\n", (int) *privkey_size);

	return key;
}
