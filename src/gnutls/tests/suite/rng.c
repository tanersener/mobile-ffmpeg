/*
 * Copyright (C) 2014 Nikos Mavrogiannopouls
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
 * along with GnuTLS.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

int main(int argc, char **argv)
{
	unsigned char buf[64];
	unsigned level, nbytes;
	FILE *fp;
	unsigned i;

	gnutls_global_init();

	if (argc != 4) {
		fprintf(stderr, "args %d\nusage: %s [nonce|key] [nbytes] [outfile]\n", argc, argv[0]);
		exit(1);
	}

	if (strcasecmp(argv[1], "nonce") == 0) {
		level = GNUTLS_RND_NONCE;
	} else if (strcasecmp(argv[1], "key") == 0) {
		level = GNUTLS_RND_KEY;
	} else {
		fprintf(stderr, "don't know %s\n", argv[1]);
		fprintf(stderr, "usage: %s [nonce|key] [nbytes] [outfile]\n", argv[0]);
		exit(1);
	}

	nbytes = atoi(argv[2]);

	fp = fopen(argv[3], "w");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s\n", argv[3]);
		exit(1);
	}

	for (i = 0; i < nbytes; i+=sizeof(buf)) {
		if (gnutls_rnd(level, buf, sizeof(buf)) < 0)
			exit(2);

		fwrite(buf, 1, sizeof(buf), fp);
	}
	fclose(fp);

	gnutls_global_deinit();
	exit(0);
}
