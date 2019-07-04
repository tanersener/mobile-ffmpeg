/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
 * <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>	/* for random */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

/* Gnulib portability files. */
#include <getpass.h>
#include <minmax.h>

#include <srptool-args.h>

/* This may need some rewrite. A lot of stuff which should be here
 * are in the library, which is not good.
 */

int crypt_int(const char *username, const char *passwd, int salt,
	      const char *tpasswd_conf, const char *tpasswd, int uindex);
static int read_conf_values(gnutls_datum_t * g, gnutls_datum_t * n,
			    char *str);
static int _verify_passwd_int(const char *username, const char *passwd,
			      char *verifier, const char *salt,
			      const gnutls_datum_t * g,
			      const gnutls_datum_t * n);

static void print_num(const char *msg, const gnutls_datum_t * num)
{
	unsigned int i;

	printf("%s:\t", msg);

	for (i = 0; i < num->size; i++) {
		if (i != 0 && i % 12 == 0)
			printf("\n\t");
		else if (i != 0 && i != num->size)
			printf(":");
		printf("%.2x", num->data[i]);
	}
	printf("\n\n");

}

static int generate_create_conf(const char *tpasswd_conf)
{
	FILE *fd;
	char line[5 * 1024];
	int index = 1, srp_idx;
	gnutls_datum_t g, n;
	gnutls_datum_t str_g, str_n;

	fd = fopen(tpasswd_conf, "w");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open file '%s'\n", tpasswd_conf);
		return -1;
	}

	for (index = 1; index <= 5; index++) {

		if (index == 1) {
			srp_idx = 2;
			n = gnutls_srp_1536_group_prime;
			g = gnutls_srp_1536_group_generator;
		} else if (index == 2) {
			srp_idx = 3;
			n = gnutls_srp_2048_group_prime;
			g = gnutls_srp_2048_group_generator;
		} else if (index == 3) {
			srp_idx = 4;
			n = gnutls_srp_3072_group_prime;
			g = gnutls_srp_3072_group_generator;
		} else if (index == 4) {
			srp_idx = 5;
			n = gnutls_srp_4096_group_prime;
			g = gnutls_srp_4096_group_generator;
		} else if (index == 5) {
			srp_idx = 7;
			n = gnutls_srp_8192_group_prime;
			g = gnutls_srp_8192_group_generator;
		} else {
			fprintf(stderr, "Unknown index: %d\n", index);
			return -1;
		}

		printf("\nGroup %d, of %d bits:\n", srp_idx, n.size * 8);
		print_num("Generator", &g);
		print_num("Prime", &n);

		if (gnutls_srp_base64_encode_alloc(&n, &str_n) < 0) {
			fprintf(stderr, "Could not encode\n");
			fclose(fd);
			return -1;
		}

		if (gnutls_srp_base64_encode_alloc(&g, &str_g) < 0) {
			fprintf(stderr, "Could not encode\n");
			fclose(fd);
			return -1;
		}

		sprintf(line, "%d:%s:%s\n", srp_idx, str_n.data, str_g.data);

		gnutls_free(str_n.data);
		gnutls_free(str_g.data);

		fwrite(line, 1, strlen(line), fd);

	}

	fclose(fd);

	return 0;

}

/* The format of a tpasswd file is:
 * username:verifier:salt:index
 *
 * index is the index of the prime-generator pair in tpasswd.conf
 */
static int
_verify_passwd_int(const char *username, const char *passwd,
		   char *verifier, const char *salt,
		   const gnutls_datum_t * g, const gnutls_datum_t * n)
{
	char _salt[1024];
	gnutls_datum_t tmp, raw_salt, new_verifier;
	size_t salt_size;
	char *pos;

	if (salt == NULL || verifier == NULL)
		return -1;

	if (strlen(salt) >= sizeof(_salt)) {
		fprintf(stderr, "Too long salt.\n");
		return -1;
	}

	/* copy salt, and null terminate after the ':' */
	strcpy(_salt, salt);
	pos = strchr(_salt, ':');
	if (pos != NULL)
		*pos = 0;

	/* convert salt to binary. */
	tmp.data = (void *) _salt;
	tmp.size = strlen(_salt);

	if (gnutls_srp_base64_decode_alloc(&tmp, &raw_salt) < 0) {
		fprintf(stderr, "Could not decode salt.\n");
		return -1;
	}

	if (gnutls_srp_verifier
	    (username, passwd, &raw_salt, g, n, &new_verifier) < 0) {
		fprintf(stderr, "Could not make the verifier\n");
		return -1;
	}

	free(raw_salt.data);

	/* encode the verifier into _salt */
	salt_size = sizeof(_salt);
	memset(_salt, 0, salt_size);
	if (gnutls_srp_base64_encode(&new_verifier, _salt, &salt_size) < 0) {
		fprintf(stderr, "Encoding error\n");
		return -1;
	}

	free(new_verifier.data);

	if (strncmp(verifier, _salt, strlen(_salt)) == 0) {
		fprintf(stderr, "Password verified\n");
		return 0;
	} else {
		fprintf(stderr, "Password does NOT match\n");
	}
	return -1;
}

static int filecopy(const char *src, const char *dst)
{
	FILE *fd, *fd2;
	char line[5 * 1024];
	char *p;

	fd = fopen(dst, "w");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open '%s' for write\n", dst);
		return -1;
	}

	fd2 = fopen(src, "r");
	if (fd2 == NULL) {
		/* empty file */
		fclose(fd);
		return 0;
	}

	line[sizeof(line) - 1] = 0;
	do {
		p = fgets(line, sizeof(line) - 1, fd2);
		if (p == NULL)
			break;

		fputs(line, fd);
	}
	while (1);

	fclose(fd);
	fclose(fd2);

	return 0;
}

/* accepts password file */
static int find_strchr(const char *username, const char *file)
{
	FILE *fd;
	char *pos;
	char line[5 * 1024];
	unsigned int i;

	fd = fopen(file, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open file '%s'\n", file);
		return -1;
	}

	while (fgets(line, sizeof(line), fd) != NULL) {
		/* move to first ':' */
		i = 0;
		while ((line[i] != ':') && (line[i] != '\0')
		       && (i < sizeof(line))) {
			i++;
		}
		if (strncmp(username, line, MAX(i, strlen(username))) == 0) {
			/* find the index */
			pos = strrchr(line, ':');
			pos++;
			fclose(fd);
			return atoi(pos);
		}
	}

	fclose(fd);
	return -1;
}

/* Parses the tpasswd files, in order to verify the given
 * username/password pair.
 */
static int
verify_passwd(const char *conffile, const char *tpasswd,
	      const char *username, const char *passwd)
{
	FILE *fd;
	char line[5 * 1024];
	unsigned int i;
	gnutls_datum_t g, n;
	int iindex;
	char *p, *pos;

	iindex = find_strchr(username, tpasswd);
	if (iindex == -1) {
		fprintf(stderr, "Cannot find '%s' in %s\n", username,
			tpasswd);
		return -1;
	}

	fd = fopen(conffile, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot find %s\n", conffile);
		return -1;
	}

	do {
		p = fgets(line, sizeof(line) - 1, fd);
	}
	while (p != NULL && atoi(p) != iindex);

	if (p == NULL) {
		fprintf(stderr, "Cannot find entry in %s\n", conffile);
		return -1;
	}
	line[sizeof(line) - 1] = 0;

	fclose(fd);

	if ((iindex = read_conf_values(&g, &n, line)) < 0) {
		fprintf(stderr, "Cannot parse conf file '%s'\n", conffile);
		return -1;
	}

	fd = fopen(tpasswd, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open file '%s'\n", tpasswd);
		return -1;
	}

	while (fgets(line, sizeof(line), fd) != NULL) {
		/* move to first ':' 
		 * This is the actual verifier.
		 */
		i = 0;
		while ((line[i] != ':') && (line[i] != '\0')
		       && (i < sizeof(line))) {
			i++;
		}
		if (strncmp(username, line, MAX(i, strlen(username))) == 0) {
			char *verifier_pos, *salt_pos;

			pos = strchr(line, ':');
			fclose(fd);
			if (pos == NULL) {
				fprintf(stderr,
					"Cannot parse conf file '%s'\n",
					conffile);
				return -1;
			}
			pos++;
			verifier_pos = pos;

			/* Move to the salt */
			pos = strchr(pos, ':');
			if (pos == NULL) {
				fprintf(stderr,
					"Cannot parse conf file '%s'\n",
					conffile);
				return -1;
			}
			pos++;
			salt_pos = pos;

			return _verify_passwd_int(username, passwd,
						  verifier_pos, salt_pos,
						  &g, &n);
		}
	}

	fclose(fd);
	return -1;

}

#define KPASSWD "/etc/tpasswd"
#define KPASSWD_CONF "/etc/tpasswd.conf"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

int main(int argc, char **argv)
{
	const char *passwd;
	int salt_size, ret;
	const char *fpasswd, *fpasswd_conf;
	const char *username;
#ifndef _WIN32
	struct passwd *pwd;
#endif

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	umask(066);

	optionProcess(&srptoolOptions, argc, argv);

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(OPT_VALUE_DEBUG);

	if (HAVE_OPT(CREATE_CONF)) {
		return generate_create_conf(OPT_ARG(CREATE_CONF));
	}

	if (HAVE_OPT(PASSWD))
		fpasswd = OPT_ARG(PASSWD);
	else
		fpasswd = (char *) KPASSWD;

	if (HAVE_OPT(PASSWD_CONF))
		fpasswd_conf = OPT_ARG(PASSWD_CONF);
	else
		fpasswd_conf = (char *) KPASSWD_CONF;

	if (HAVE_OPT(USERNAME))
		username = OPT_ARG(USERNAME);
	else {
#ifndef _WIN32
		pwd = getpwuid(getuid());

		if (pwd == NULL) {
			fprintf(stderr, "No such user\n");
			return -1;
		}

		username = pwd->pw_name;
#else
		fprintf(stderr, "Please specify a user\n");
		return -1;
#endif
	}

	salt_size = 16;

	passwd = getpass("Enter password: ");
	if (passwd == NULL) {
		fprintf(stderr, "Please specify a password\n");
		return -1;
	}

/* not ready yet */
	if (HAVE_OPT(VERIFY)) {
		return verify_passwd(fpasswd_conf, fpasswd,
				     username, passwd);
	}


	return crypt_int(username, passwd, salt_size,
			 fpasswd_conf, fpasswd, OPT_VALUE_INDEX);

}

static char *_srp_crypt(const char *username, const char *passwd,
			int salt_size, const gnutls_datum_t * g,
			const gnutls_datum_t * n)
{
	unsigned char salt[128];
	static char result[1024];
	gnutls_datum_t dat_salt, txt_salt;
	gnutls_datum_t verifier, txt_verifier;

	if ((unsigned) salt_size > sizeof(salt))
		return NULL;

	/* generate the salt
	 */
	if (gnutls_rnd(GNUTLS_RND_NONCE, salt, salt_size) < 0) {
		fprintf(stderr, "Could not create nonce\n");
		return NULL;
	}

	dat_salt.data = salt;
	dat_salt.size = salt_size;

	if (gnutls_srp_verifier
	    (username, passwd, &dat_salt, g, n, &verifier) < 0) {
		fprintf(stderr, "Error getting verifier\n");
		return NULL;
	}

	/* base64 encode the verifier */
	if (gnutls_srp_base64_encode_alloc(&verifier, &txt_verifier) < 0) {
		fprintf(stderr, "Error encoding\n");
		free(verifier.data);
		return NULL;
	}

	free(verifier.data);

	if (gnutls_srp_base64_encode_alloc(&dat_salt, &txt_salt) < 0) {
		fprintf(stderr, "Error encoding\n");
		return NULL;
	}

	sprintf(result, "%s:%s", txt_verifier.data, txt_salt.data);
	free(txt_salt.data);
	free(txt_verifier.data);

	return result;

}


int
crypt_int(const char *username, const char *passwd, int salt_size,
	  const char *tpasswd_conf, const char *tpasswd, int uindex)
{
	FILE *fd;
	char *cr;
	gnutls_datum_t g, n;
	char line[5 * 1024];
	char *p, *pp;
	int iindex;
	char tmpname[1024];

	fd = fopen(tpasswd_conf, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot find %s\n", tpasswd_conf);
		return -1;
	}

	do {			/* find the specified uindex in file */
		p = fgets(line, sizeof(line) - 1, fd);
	}
	while (p != NULL && (iindex = atoi(p)) != uindex);

	if (p == NULL) {
		fprintf(stderr, "Cannot find entry in %s\n", tpasswd_conf);
		return -1;
	}
	line[sizeof(line) - 1] = 0;

	fclose(fd);
	if ((iindex = read_conf_values(&g, &n, line)) < 0) {
		fprintf(stderr, "Cannot parse conf file '%s'\n",
			tpasswd_conf);
		return -1;
	}

	cr = _srp_crypt(username, passwd, salt_size, &g, &n);
	if (cr == NULL) {
		fprintf(stderr, "Cannot _srp_crypt()...\n");
		return -1;
	} else {
		/* delete previous entry */
		struct stat st;
		FILE *fd2;
		int put;

		if (strlen(tpasswd) + 5 > sizeof(tmpname)) {
			fprintf(stderr, "file '%s' is tooooo long\n",
				tpasswd);
			return -1;
		}

		snprintf(tmpname, sizeof(tmpname), "%s.tmp", tpasswd);

		if (stat(tmpname, &st) != -1) {
			fprintf(stderr, "file '%s' is locked\n", tpasswd);
			return -1;
		}

		if (filecopy(tpasswd, tmpname) != 0) {
			fprintf(stderr, "Cannot copy '%s' to '%s'\n",
				tpasswd, tmpname);
			return -1;
		}

		fd = fopen(tpasswd, "w");
		if (fd == NULL) {
			fprintf(stderr, "Cannot open '%s' for write\n",
				tpasswd);
			(void)remove(tmpname);
			return -1;
		}

		fd2 = fopen(tmpname, "r");
		if (fd2 == NULL) {
			fprintf(stderr, "Cannot open '%s' for read\n",
				tmpname);
			(void)remove(tmpname);
			return -1;
		}

		put = 0;
		do {
			p = fgets(line, sizeof(line) - 1, fd2);
			if (p == NULL)
				break;

			pp = strchr(line, ':');
			if (pp == NULL)
				continue;

			if (strncmp(p, username,
				    MAX(strlen(username),
					(unsigned int) (pp - p))) == 0) {
				put = 1;
				fprintf(fd, "%s:%s:%u\n", username, cr,
					iindex);
			} else {
				fputs(line, fd);
			}
		}
		while (1);

		if (put == 0) {
			fprintf(fd, "%s:%s:%u\n", username, cr, iindex);
		}

		fclose(fd);
		fclose(fd2);

		(void)remove(tmpname);

	}


	return 0;
}



/* this function parses tpasswd.conf file. Format is:
 * int(index):base64(n):base64(g)
 */
static int
read_conf_values(gnutls_datum_t * g, gnutls_datum_t * n, char *str)
{
	char *p;
	int len;
	int index, ret;
	gnutls_datum_t dat;

	index = atoi(str);

	p = strrchr(str, ':');	/* we have g */
	if (p == NULL) {
		return -1;
	}

	*p = '\0';
	p++;

	/* read the generator */
	len = strlen(p);
	if (p[len - 1] == '\n')
		len--;

	dat.data = (void *) p;
	dat.size = len;
	ret = gnutls_srp_base64_decode_alloc(&dat, g);

	if (ret < 0) {
		fprintf(stderr, "Decoding error\n");
		return -1;
	}

	/* now go for n - modulo */
	p = strrchr(str, ':');	/* we have n */
	if (p == NULL) {
		return -1;
	}

	*p = '\0';
	p++;

	dat.data = (void *) p;
	dat.size = strlen(p);

	ret = gnutls_srp_base64_decode_alloc(&dat, n);

	if (ret < 0) {
		fprintf(stderr, "Decoding error\n");
		free(g->data);
		return -1;
	}

	return index;
}
