/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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

#ifndef SOFTHSM_H
# define SOFTHSM_H

#include <sys/stat.h>

#define SOFTHSM_V2

#ifdef SOFTHSM_V1
# define SOFTHSM_URL "pkcs11:model=SoftHSM;manufacturer=SoftHSM;serial=1;token=test"
# define LIB1 "/usr/lib64/pkcs11/libsofthsm.so"
# define LIB2 "/usr/lib/pkcs11/libsofthsm.so"
# define LIB3 "/usr/lib/softhsm/libsofthsm.so"
# define LIB4 "/usr/local/lib/softhsm/libsofthsm.so"
# define SOFTHSM_BIN1 "/usr/bin/softhsm"
# define SOFTHSM_BIN2 "/usr/local/bin/softhsm"
# define SOFTHSM_ENV "SOFTHSM_CONF"
#else
# define SOFTHSM_URL "pkcs11:model=SoftHSM%20v2;manufacturer=SoftHSM%20project;token=test"
# define LIB1 "/usr/lib64/pkcs11/libsofthsm2.so"
# define LIB2 "/usr/lib/pkcs11/libsofthsm2.so"
# define LIB3 "/usr/lib/softhsm/libsofthsm2.so"
# define LIB4 "/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so"
# define SOFTHSM_BIN1 "/usr/bin/softhsm2-util"
# define SOFTHSM_BIN2 "/usr/local/bin/softhsm2-util"
# define SOFTHSM_ENV "SOFTHSM2_CONF"
#endif


inline static const char *softhsm_lib(void) 
{
	const char *lib;

	if (sizeof(long) == 8 && access(LIB1, R_OK) == 0) {
		lib = LIB1;
	} else if (access(LIB2, R_OK) == 0) {
		lib = LIB2;
	} else if (access(LIB3, R_OK) == 0) {
		lib = LIB3;
	} else if (sizeof(long) == 8 && access(LIB4, R_OK) == 0) {
		lib = LIB4;
	} else {
		fprintf(stderr, "cannot find softhsm module\n");
		exit(77);
	}

	return lib;
}

inline static const char *softhsm_bin(void) 
{
	const char *bin;

	if (access(SOFTHSM_BIN1, X_OK) == 0) {
		bin = SOFTHSM_BIN1;
	} else if (access(SOFTHSM_BIN2, X_OK) == 0) {
		bin = SOFTHSM_BIN2;
	} else {
		fprintf(stderr, "cannot find softhsm bin\n");
		exit(77);
	}

	return bin;
}

static
void set_softhsm_conf(const char *config)
{
	char buf[128];
	char db_dir[128];
	FILE *fp;

	snprintf(db_dir, sizeof(db_dir), "%s.db", config);

	unsetenv(SOFTHSM_ENV);
	remove(config);
	fp = fopen(config, "w");
	if (fp == NULL) {
		fprintf(stderr, "error writing %s\n", config);
		exit(1);
	}

#ifdef SOFTHSM_V1
	remove(db_dir);
	snprintf(buf, sizeof(buf), "0:./%s\n", db_dir);
	fputs(buf, fp);
#else
	fputs("directories.tokendir = ", fp);
	fputs(db_dir, fp);
	fputs("\n", fp);
	fputs("objectstore.backend = file\n", fp);

	if (strlen(db_dir) < 6) {
		fprintf(stderr, "too short name for db: %s\n", db_dir);
		exit(1);
	}
	snprintf(buf, sizeof(buf), "rm -rf %s\n", db_dir);
	system(buf);
	mkdir(db_dir, 0755);
#endif
	fclose(fp);

	setenv(SOFTHSM_ENV, config, 0);
}

#endif
