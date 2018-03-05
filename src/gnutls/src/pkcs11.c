/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <config.h>

#include <getpass.h>

#include <gnutls/gnutls.h>
#include <gnutls/pkcs11.h>
#include <gnutls/abstract.h>
#include <stdio.h>
#include <stdlib.h>
#include "p11tool.h"
#include "certtool-cfg.h"
#include "certtool-common.h"
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <common.h>

#ifdef _WIN32
# define sleep(x) Sleep(x*1000)
#endif

static
char *get_single_token_url(common_info_st * info);
static char *_saved_url = NULL;

#define FIX(url, out, det, info) \
	if (url == NULL) { \
		url = get_single_token_url(info); \
		if (url == NULL) { \
			fprintf(stderr, "warning: no token URL was provided for this operation; the available tokens are:\n\n"); \
			pkcs11_token_list(out, det, info, 1); \
			exit(1); \
		} \
		_saved_url = (void*)url; \
	}

#define UNFIX gnutls_free(_saved_url);_saved_url = NULL

#define KEEP_LOGIN_FLAGS(flags) (flags & (GNUTLS_PKCS11_OBJ_FLAG_LOGIN|GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO))

#define CHECK_LOGIN_FLAG(flags) \
	if ((flags & KEEP_LOGIN_FLAGS(flags)) == 0) \
		fprintf(stderr, \
			"warning: --login was not specified and it may be required for this operation.\n")


void
pkcs11_delete(FILE * outfile, const char *url,
	      unsigned int login_flags, common_info_st * info)
{
	int ret;
	unsigned int obj_flags = 0;

	if (login_flags) obj_flags = login_flags;

	pkcs11_common(info);

	if (info->batch == 0) {
		pkcs11_list(outfile, url, PKCS11_TYPE_ALL, login_flags,
			    GNUTLS_PKCS11_URL_LIB, info);
		ret =
		    read_yesno
		    ("Are you sure you want to delete those objects? (y/N): ",
		     0);
		if (ret == 0) {
			exit(1);
		}
	}

	ret = gnutls_pkcs11_delete_url(url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	fprintf(outfile, "\n%d objects deleted\n", ret);

	return;
}

/* lists certificates from a token
 */
void
pkcs11_list(FILE * outfile, const char *url, int type, unsigned int flags,
	    unsigned int detailed, common_info_st * info)
{
	gnutls_pkcs11_obj_t *crt_list;
	unsigned int crt_list_size = 0, i, j;
	int ret, otype;
	char *output, *str;
	int attrs, print_exts = 0;
	gnutls_x509_ext_st *exts;
	unsigned exts_size;
	unsigned int obj_flags = flags;

	pkcs11_common(info);

	FIX(url, outfile, detailed, info);

	gnutls_pkcs11_token_get_flags(url, &flags);
	if (flags & GNUTLS_PKCS11_TOKEN_TRUSTED)
		print_exts = 1;

	if (type == PKCS11_TYPE_TRUSTED) {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_CRT_TRUSTED;
	} else if (type == PKCS11_TYPE_PK) {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_CRT_WITH_PRIVKEY;
	} else if (type == PKCS11_TYPE_CRT_ALL) {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_CRT_ALL;
		if (print_exts != 0) print_exts++;
	} else if (type == PKCS11_TYPE_PRIVKEY) {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_PRIVKEY;
	} else if (type == PKCS11_TYPE_INFO) {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_MATCH;
	} else {
		attrs = GNUTLS_PKCS11_OBJ_ATTR_ALL;
	}

	/* give some initial value to avoid asking for the pkcs11 pin twice.
	 */
	ret =
	    gnutls_pkcs11_obj_list_import_url2(&crt_list, &crt_list_size,
					       url, attrs, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in crt_list_import (1): %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (crt_list_size == 0) {
		fprintf(stderr, "No matching objects found\n");
		exit(2);
	}

	for (i = 0; i < crt_list_size; i++) {
		char buf[128];
		size_t size;
		unsigned int oflags;

		ret =
		    gnutls_pkcs11_obj_export_url(crt_list[i], detailed,
						 &output);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		if (info->only_urls) {
			fprintf(outfile, "%s\n", output);
			gnutls_free(output);
			continue;
		} else {
			fprintf(outfile, "Object %d:\n\tURL: %s\n", i, output);
			gnutls_free(output);
		}

		otype = gnutls_pkcs11_obj_get_type(crt_list[i]);
		fprintf(outfile, "\tType: %s\n",
			gnutls_pkcs11_type_get_name(otype));

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_obj_get_info(crt_list[i],
					       GNUTLS_PKCS11_OBJ_LABEL,
					       buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
		fprintf(outfile, "\tLabel: %s\n", buf);

		oflags = 0;
		ret = gnutls_pkcs11_obj_get_flags(crt_list[i], &oflags);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
		str = gnutls_pkcs11_obj_flags_get_str(oflags);
		if (str != NULL) {
			fprintf(outfile, "\tFlags: %s\n", str);
			gnutls_free(str);
		}

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_obj_get_info(crt_list[i],
					       GNUTLS_PKCS11_OBJ_ID_HEX,
					       buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
		fprintf(outfile, "\tID: %s\n", buf);

		if (otype == GNUTLS_PKCS11_OBJ_X509_CRT && print_exts > 0) {
			ret = gnutls_pkcs11_obj_get_exts(crt_list[i], &exts, &exts_size, 0);
			if (ret >= 0 && exts_size > 0) {
				gnutls_datum_t txt;

				if (print_exts > 1) {
					fprintf(outfile, "\tAttached extensions:\n");
					ret = gnutls_x509_ext_print(exts, exts_size, 0, &txt);
					if (ret >= 0) {
						fprintf(outfile, "%s", (char*)txt.data);
						gnutls_free(txt.data);
					}
				} else {
					fprintf(outfile, "\tAttached extensions:");
					for (j=0;j<exts_size;j++) {
						fprintf(outfile, "%s%s", exts[j].oid, (j!=exts_size-1)?",":" ");
					}
				}
				for (j=0;j<exts_size;j++) {
					gnutls_x509_ext_deinit(&exts[j]);
				}
				gnutls_free(exts);
				fprintf(outfile, "\n");
			}
		}

		fprintf(outfile, "\n");
		gnutls_pkcs11_obj_deinit(crt_list[i]);
	}
	gnutls_free(crt_list);

	UNFIX;
	return;
}

#define TEST_DATA "Test data to sign"

void
pkcs11_test_sign(FILE * outfile, const char *url, unsigned int flags,
	    common_info_st * info)
{
	gnutls_privkey_t privkey;
	gnutls_pubkey_t pubkey;
	int ret;
	gnutls_datum_t data, sig = {NULL, 0};
	int pk;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	data.data = (void*)TEST_DATA;
	data.size = sizeof(TEST_DATA)-1;

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_privkey_import_url(privkey, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot import private key: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, GNUTLS_KEY_DIGITAL_SIGNATURE, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot import public key: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_privkey_sign_data(privkey, GNUTLS_DIG_SHA1, 0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot sign data: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	pk = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);

	fprintf(stderr, "Verifying against private key parameters... ");
	ret = gnutls_pubkey_verify_data2(pubkey, gnutls_pk_to_sign(pk, GNUTLS_DIG_SHA1),
		0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot verify signed data: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	fprintf(stderr, "ok\n");

	/* now try to verify against a public key within the token */
	gnutls_pubkey_deinit(pubkey);
	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_import_url(pubkey, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a corresponding public key object in token: %s\n",
			gnutls_strerror(ret));
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			exit(0);
		exit(1);
	}

	fprintf(stderr, "Verifying against public key in the token... ");
	ret = gnutls_pubkey_verify_data2(pubkey, gnutls_pk_to_sign(pk, GNUTLS_DIG_SHA1),
		0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot verify signed data: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	fprintf(stderr, "ok\n");

	gnutls_free(sig.data);
	gnutls_pubkey_deinit(pubkey);
	gnutls_privkey_deinit(privkey);
	UNFIX;
}

void
pkcs11_export(FILE * outfile, const char *url, unsigned int flags,
	      common_info_st * info)
{
	gnutls_pkcs11_obj_t obj;
	gnutls_datum_t t;
	int ret;
	unsigned int obj_flags = flags;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	ret = gnutls_pkcs11_obj_init(&obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_export3(obj, info->outcert_format, &t);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		exit(1);
	}

	fwrite(t.data, 1, t.size, outfile);
	gnutls_free(t.data);

	if (info->outcert_format == GNUTLS_X509_FMT_PEM)
		fputs("\n\n", outfile);

	gnutls_pkcs11_obj_deinit(obj);

	UNFIX;
	return;
}

void
pkcs11_export_chain(FILE * outfile, const char *url, unsigned int flags,
	      common_info_st * info)
{
	gnutls_pkcs11_obj_t obj;
	gnutls_x509_crt_t xcrt;
	gnutls_datum_t t;
	int ret;
	unsigned int obj_flags = flags;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	ret = gnutls_pkcs11_obj_init(&obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	/* make a crt */
	ret = gnutls_x509_crt_init(&xcrt);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_import_pkcs11(xcrt, obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_export3(obj, GNUTLS_X509_FMT_PEM, &t);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		exit(1);
	}
	fwrite(t.data, 1, t.size, outfile);
	fputs("\n\n", outfile);
	gnutls_free(t.data);

	gnutls_pkcs11_obj_deinit(obj);

	do {
		ret = gnutls_pkcs11_get_raw_issuer(url, xcrt, &t, GNUTLS_X509_FMT_PEM, 0);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fwrite(t.data, 1, t.size, outfile);
		fputs("\n\n", outfile);

		gnutls_x509_crt_deinit(xcrt);

		ret = gnutls_x509_crt_init(&xcrt);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_x509_crt_import(xcrt, &t, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		gnutls_free(t.data);

		ret = gnutls_x509_crt_check_issuer(xcrt, xcrt);
		if (ret != 0) {
			/* self signed */
			break;
		}

	} while(1);

	UNFIX;
	return;
}

/* If there is a single token only present, return its URL.
 */
static
char *get_single_token_url(common_info_st * info)
{
	int ret;
	char *url = NULL, *t = NULL;

	pkcs11_common(info);

	ret = gnutls_pkcs11_token_get_url(0, 0, &url);
	if (ret < 0)
		return NULL;

	ret = gnutls_pkcs11_token_get_url(1, 0, &t);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_free(t);
		gnutls_free(url);
		return NULL;
	}

	return url;
}

static
void print_type(FILE *outfile, unsigned flags)
{
	unsigned print = 0;

	fputs("\tType: ", outfile);
	if (flags & GNUTLS_PKCS11_TOKEN_HW) {
		fputs("Hardware token", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_TRUSTED) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Trust module", outfile);
		print++;
	}

	if (print == 0)
		fputs("Generic token", outfile);
	fputc('\n', outfile);
}

void
pkcs11_token_list(FILE * outfile, unsigned int detailed,
		  common_info_st * info, unsigned brief)
{
	int ret;
	int i;
	char *url;
	char buf[128];
	size_t size;
	unsigned flags;

	pkcs11_common(info);

	for (i = 0;; i++) {
		ret = gnutls_pkcs11_token_get_url(i, detailed, &url);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;

		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		if (brief != 0) {
			fprintf(outfile, "%s\n", url);
			goto cont;
		} else {
			fprintf(outfile, "Token %d:\n\tURL: %s\n", i, url);
		}

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_token_get_info(url,
						 GNUTLS_PKCS11_TOKEN_LABEL,
						 buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fprintf(outfile, "\tLabel: %s\n", buf);

		ret = gnutls_pkcs11_token_get_flags(url, &flags);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		} else {
			print_type(outfile, flags);
		}

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_token_get_info(url,
						 GNUTLS_PKCS11_TOKEN_MANUFACTURER,
						 buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fprintf(outfile, "\tManufacturer: %s\n", buf);

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_token_get_info(url,
						 GNUTLS_PKCS11_TOKEN_MODEL,
						 buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fprintf(outfile, "\tModel: %s\n", buf);

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_token_get_info(url,
						 GNUTLS_PKCS11_TOKEN_SERIAL,
						 buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fprintf(outfile, "\tSerial: %s\n", buf);

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_token_get_info(url,
						 GNUTLS_PKCS11_TOKEN_MODNAME,
						 buf, &size);
		if (ret >= 0) {
			fprintf(outfile, "\tModule: %s\n", buf);
		}
		fprintf(outfile, "\n\n");
 cont:
		gnutls_free(url);

	}

	return;
}

static void find_same_pubkey_with_id(const char *url, gnutls_x509_crt_t crt, gnutls_datum_t *cid, unsigned flags)
{
	gnutls_pkcs11_obj_t *obj_list;
	unsigned int obj_list_size = 0, i;
	int ret;
	gnutls_datum_t praw = {NULL, 0};
	gnutls_datum_t praw2 = {NULL, 0};
	gnutls_pubkey_t pubkey;
	uint8_t buf[128];
	size_t size;
	char *purl;
	unsigned otype;

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "memory error\n");
		exit(1);
	}

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {
		fprintf(stderr, "error: cannot import public key from certificate\n");
		gnutls_pubkey_deinit(pubkey);
		return;
	}

	ret = gnutls_pubkey_export2(pubkey, GNUTLS_X509_FMT_DER, &praw);
	gnutls_pubkey_deinit(pubkey);
	if (ret < 0) {
		fprintf(stderr, "error: cannot export public key\n");
		return;
	}

	ret =
	    gnutls_pkcs11_obj_list_import_url4(&obj_list, &obj_list_size,
					       url, GNUTLS_PKCS11_OBJ_FLAG_PUBKEY|flags);
	if (ret < 0) {
		fprintf(stderr, "Error in obj_list_import (1): %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (obj_list_size == 0)
		return;

	for (i = 0; i < obj_list_size; i++) {
		purl = NULL;

		otype = gnutls_pkcs11_obj_get_type(obj_list[i]);
		if (otype != GNUTLS_PKCS11_OBJ_PUBKEY)
			goto cont;

		ret =
		    gnutls_pkcs11_obj_export_url(obj_list[i], 0,
						 &purl);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			goto cont;
		}

		ret = gnutls_pkcs11_obj_export2(obj_list[i], &praw2);
		if (ret < 0) {
			fprintf(stderr, "error: cannot export object: %s\n", purl);
			goto cont;
		}

		if (praw2.size == praw.size && memcmp(praw2.data, praw.data, praw.size) == 0) {
			/* found - now extract the CKA_ID */

			size = sizeof(buf);
			ret =
			    gnutls_pkcs11_obj_get_info(obj_list[i],
					       GNUTLS_PKCS11_OBJ_ID,
					       buf, &size);
			if (ret < 0) {
				fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
				exit(1);
			}

			cid->data = gnutls_malloc(size);
			cid->size = size;
			if (cid->data == NULL) {
				fprintf(stderr, "memory error\n");
				exit(1);
			}

			memcpy(cid->data, buf, size);

			return;
		}

 cont:
		gnutls_pkcs11_obj_deinit(obj_list[i]);
		gnutls_free(purl);
	}
	gnutls_free(obj_list);

	UNFIX;
	return;
}

static void find_same_privkey_with_id(const char *url, gnutls_x509_crt_t crt, gnutls_datum_t *cid, unsigned flags)
{
	gnutls_pkcs11_obj_t *obj_list;
	unsigned int obj_list_size = 0, i;
	int ret;
	gnutls_datum_t praw = {NULL, 0};
	gnutls_datum_t praw2 = {NULL, 0};
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	uint8_t buf[128];
	size_t size;
	char *purl;
	unsigned otype;

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "memory error\n");
		exit(1);
	}

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {
		fprintf(stderr, "error: cannot import public key from certificate\n");
		gnutls_pubkey_deinit(pubkey);
		return;
	}

	ret = gnutls_pubkey_export2(pubkey, GNUTLS_X509_FMT_DER, &praw);
	gnutls_pubkey_deinit(pubkey);
	if (ret < 0) {
		fprintf(stderr, "error: cannot export public key\n");
		return;
	}

	ret =
	    gnutls_pkcs11_obj_list_import_url4(&obj_list, &obj_list_size,
					       url, GNUTLS_PKCS11_OBJ_FLAG_PRIVKEY|flags);
	if (ret < 0) {
		fprintf(stderr, "Error in obj_list_import (1): %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (obj_list_size == 0)
		return;

	for (i = 0; i < obj_list_size; i++) {
		purl = NULL;
		pubkey = NULL;
		privkey = NULL;

		otype = gnutls_pkcs11_obj_get_type(obj_list[i]);
		if (otype != GNUTLS_PKCS11_OBJ_PRIVKEY)
			goto cont;

		ret =
		    gnutls_pkcs11_obj_export_url(obj_list[i], 0,
						 &purl);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			goto cont;
		}

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0) {
			fprintf(stderr, "memory error\n");
			exit(1);
		}

		ret = gnutls_privkey_import_url(privkey, purl, 0);
		if (ret < 0) {
			fprintf(stderr, "error: cannot import key: %s: %s\n", purl, gnutls_strerror(ret));
			goto cont;
		}

		if (gnutls_privkey_get_pk_algorithm(privkey, NULL) != GNUTLS_PK_RSA) {
			/* it is not possible to obtain parameters from non-RSA private keys in PKCS#11 */
			goto cont;
		}

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0) {
			fprintf(stderr, "memory error\n");
			exit(1);
		}

		ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
		if (ret < 0) {
			fprintf(stderr, "error: cannot import key parameters for '%s': %s\n", purl, gnutls_strerror(ret));
			goto cont;
		}

		ret = gnutls_pubkey_export2(pubkey, GNUTLS_X509_FMT_DER, &praw2);
		if (ret < 0) {
			fprintf(stderr, "error: cannot export pubkey '%s': %s\n", purl, gnutls_strerror(ret));
			goto cont;
		}


		if (praw2.size == praw.size && memcmp(praw2.data, praw.data, praw.size) == 0) {
			/* found - now extract the CKA_ID */

			size = sizeof(buf);
			ret =
			    gnutls_pkcs11_obj_get_info(obj_list[i],
					       GNUTLS_PKCS11_OBJ_ID,
					       buf, &size);
			if (ret < 0) {
				fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
				exit(1);
			}

			cid->data = gnutls_malloc(size);
			cid->size = size;
			if (cid->data == NULL) {
				fprintf(stderr, "memory error\n");
				exit(1);
			}

			memcpy(cid->data, buf, size);

			return;
		}

 cont:
		if (privkey)
			gnutls_privkey_deinit(privkey);
		if (pubkey)
			gnutls_pubkey_deinit(pubkey);
		gnutls_pkcs11_obj_deinit(obj_list[i]);
		gnutls_free(purl);
	}
	gnutls_free(obj_list);
	UNFIX;
	return;
}

void
pkcs11_write(FILE * outfile, const char *url, const char *label,
	     const char *id, unsigned flags, common_info_st * info)
{
	gnutls_x509_crt_t xcrt;
	gnutls_x509_privkey_t xkey;
	gnutls_pubkey_t xpubkey;
	int ret;
	gnutls_datum_t *secret_key;
	unsigned key_usage = 0;
	unsigned char raw_id[128];
	size_t raw_id_size;
	gnutls_datum_t cid = {NULL, 0};

	pkcs11_common(info);

	FIX(url, outfile, 0, info);
	CHECK_LOGIN_FLAG(flags);
	if (label == NULL && info->batch == 0) {
		label = read_str("warning: The object's label was not specified.\nLabel: ");
	}

	if (id != NULL) {
		raw_id_size = sizeof(raw_id);
		ret = gnutls_hex2bin(id, strlen(id), raw_id, &raw_id_size);
		if (ret < 0) {
			fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		cid.data = raw_id;
		cid.size = raw_id_size;
	}

	secret_key = load_secret_key(0, info);
	if (secret_key != NULL) {
		ret =
		    gnutls_pkcs11_copy_secret_key(url, secret_key, label,
						  info->key_usage,
						  flags |
						  GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
	}

	xcrt = load_cert(0, info);
	if (xcrt != NULL) {
		if (cid.data == NULL && !(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_CA) && !(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_DISTRUSTED)) {
			gnutls_datum_t hex;
			/* attempting to discover public keys matching this one,
			 * and if yes, re-use their ID. We don't do it for CAs (trusted/distrusted
			 * or explicitly marked as such. */

			/* try without login */
			find_same_pubkey_with_id(url, xcrt, &cid, 0);

			if (cid.data == NULL && KEEP_LOGIN_FLAGS(flags))
				find_same_pubkey_with_id(url, xcrt, &cid, KEEP_LOGIN_FLAGS(flags));

			if (cid.data) {
				ret = gnutls_hex_encode2(&cid, &hex);
				if (ret < 0) {
					fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
					exit(1);
				}
				fprintf(stderr, "note: will re-use ID %s from corresponding public key\n", hex.data);
				gnutls_free(hex.data);

			} else { /* no luck, try to get a corresponding private key */
				find_same_privkey_with_id(url, xcrt, &cid, KEEP_LOGIN_FLAGS(flags));
				if (cid.data) {
					ret = gnutls_hex_encode2(&cid, &hex);
					if (ret < 0) {
						fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
						exit(1);
					}
					fprintf(stderr, "note: will re-use ID %s from corresponding private key\n", hex.data);
					gnutls_free(hex.data);
				}
			}
		}

		ret = gnutls_pkcs11_copy_x509_crt2(url, xcrt, label, &cid, flags);
		if (ret < 0) {
			fprintf(stderr, "Error writing certificate: %s\n", gnutls_strerror(ret));
			if (((flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_CA) ||
			     (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED)) && 
			    (flags & GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO) == 0)
				fprintf(stderr, "note: some tokens may require security officer login for this operation\n");
			exit(1);
		}

		gnutls_x509_crt_get_key_usage(xcrt, &key_usage, NULL);
		gnutls_x509_crt_deinit(xcrt);
	}

	xkey = load_x509_private_key(0, info);
	if (xkey != NULL) {
		ret =
		    gnutls_pkcs11_copy_x509_privkey2(url, xkey, label,
						     &cid, key_usage|info->key_usage,
						     flags |
						     GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
		gnutls_x509_privkey_deinit(xkey);
	}

	xpubkey = load_pubkey(0, info);
	if (xpubkey != NULL) {
		ret =
		    gnutls_pkcs11_copy_pubkey(url, xpubkey, label,
						     &cid,
						     0, flags);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}
		gnutls_pubkey_deinit(xpubkey);
	}

	if (xkey == NULL && xcrt == NULL && secret_key == NULL && xpubkey == NULL) {
		fprintf(stderr,
			"You must use --load-privkey, --load-certificate, --load-pubkey or --secret-key to load the file to be copied\n");
		exit(1);
	}

	UNFIX;
	return;
}

void
pkcs11_generate(FILE * outfile, const char *url, gnutls_pk_algorithm_t pk,
		unsigned int bits,
		const char *label, const char *id, int detailed,
		unsigned int flags, common_info_st * info)
{
	int ret;
	gnutls_datum_t pubkey;
	gnutls_datum_t cid = {NULL, 0};
	unsigned char raw_id[128];
	size_t raw_id_size;

	pkcs11_common(info);

	FIX(url, outfile, detailed, info);
	CHECK_LOGIN_FLAG(flags);

	if (id != NULL) {
		raw_id_size = sizeof(raw_id);
		ret = gnutls_hex2bin(id, strlen(id), raw_id, &raw_id_size);
		if (ret < 0) {
			fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		cid.data = raw_id;
		cid.size = raw_id_size;
	}

	if (outfile == stderr || outfile == stdout) {
		fprintf(stderr, "warning: no --outfile was specified and the generated public key will be printed on screen.\n");
	}

	if (label == NULL && info->batch == 0) {
		label = read_str("warning: Label was not specified.\nLabel: ");
	}

	fprintf(stderr, "Generating an %s key...\n", gnutls_pk_get_name(pk));

	ret =
	    gnutls_pkcs11_privkey_generate3(url, pk, bits, label, &cid,
					    GNUTLS_X509_FMT_PEM, &pubkey,
					    info->key_usage,
					    flags|GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		if (bits != 1024 && pk == GNUTLS_PK_RSA)
			fprintf(stderr,
				"note: several smart cards do not support arbitrary size keys; try --bits 1024 or 2048.\n");
		exit(1);
	}

	fwrite(pubkey.data, 1, pubkey.size, outfile);
	gnutls_free(pubkey.data);

	UNFIX;
	return;
}

void
pkcs11_export_pubkey(FILE * outfile, const char *url, int detailed, unsigned int flags, common_info_st * info)
{
	int ret;
	gnutls_datum_t pubkey;
	gnutls_pkcs11_privkey_t pkey;

	pkcs11_common(info);

	FIX(url, outfile, detailed, info);
	CHECK_LOGIN_FLAG(flags);

	if (outfile == stderr || outfile == stdout) {
		fprintf(stderr, "warning: no --outfile was specified and the public key will be printed on screen.\n");
		sleep(3);
	}

	ret = gnutls_pkcs11_privkey_init(&pkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_privkey_import_url(pkey, url, 0);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_privkey_export_pubkey(pkey,
						GNUTLS_X509_FMT_PEM, &pubkey,
						flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}
	gnutls_pkcs11_privkey_deinit(pkey);

	fwrite(pubkey.data, 1, pubkey.size, outfile);
	gnutls_free(pubkey.data);

	UNFIX;
	return;
}

void
pkcs11_init(FILE * outfile, const char *url, const char *label,
	    common_info_st * info)
{
	int ret;
	const char *pin;
	char so_pin[32];

	pkcs11_common(info);

	if (url == NULL) {
		fprintf(stderr, "error: no token URL given to initialize!\n");
		exit(1);
	}

	if (label == NULL) {
		fprintf(stderr, "error: no label provided for token initialization!\n");
		exit(1);
	}

	if (info->so_pin != NULL)
		pin = info->so_pin;
	else {
		pin = getenv("GNUTLS_SO_PIN");
		if (pin == NULL && info->batch == 0)
			pin = getpass("Enter Security Officer's PIN: ");
		if (pin == NULL)
			exit(1);
	}

	if (strlen(pin) >= sizeof(so_pin) || pin[0] == '\n')
		exit(1);

	strcpy(so_pin, pin);

	fprintf(stderr, "Initializing token... ");
	ret = gnutls_pkcs11_token_init(url, so_pin, label);
	if (ret < 0) {
		fprintf(stderr, "\nError in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "\nToken was successfully initialized; use --initialize-pin and --initialize-so-pin to set or reset PINs\n");

	return;
}

void
pkcs11_set_pin(FILE * outfile, const char *url, common_info_st * info, unsigned so)
{
	int ret;
	const char *pin;

	pkcs11_common(info);

	if (url == NULL) {
		fprintf(stderr, "error: no token URL given to initialize!\n");
		exit(1);
	}

	fprintf(stderr, "Setting token's user PIN...\n");

	if (so) {
		if (info->so_pin != NULL) {
			pin = info->so_pin;
		} else {
			pin = getenv("GNUTLS_SO_PIN");
			if (pin == NULL && info->batch == 0)
				pin = getpass("Enter Administrators's new PIN: ");
			if (pin == NULL)
				exit(1);
		}
	} else {
		if (info->pin != NULL) {
			pin = info->pin;
		} else {
			pin = getenv("GNUTLS_PIN");
			if (pin == NULL && info->batch == 0)
				pin = getpass("Enter User's new PIN: ");
			if (pin == NULL)
				exit(1);
		}
	}

	if (pin == NULL || pin[0] == '\n')
		exit(1);

	ret = gnutls_pkcs11_token_set_pin(url, NULL, pin, (so!=0)?GNUTLS_PIN_SO:GNUTLS_PIN_USER);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	return;
}

const char *mech_list[] = {
	[0] = "CKM_RSA_PKCS_KEY_PAIR_GEN",
	[1] = "CKM_RSA_PKCS",
	[2] = "CKM_RSA_9796",
	[3] = "CKM_RSA_X_509",
	[4] = "CKM_MD2_RSA_PKCS",
	[5] = "CKM_MD5_RSA_PKCS",
	[6] = "CKM_SHA1_RSA_PKCS",
	[7] = "CKM_RIPEMD128_RSA_PKCS",
	[8] = "CKM_RIPEMD160_RSA_PKCS",
	[9] = "CKM_RSA_PKCS_OAEP",
	[0xa] = "CKM_RSA_X9_31_KEY_PAIR_GEN",
	[0xb] = "CKM_RSA_X9_31",
	[0xc] = "CKM_SHA1_RSA_X9_31",
	[0xd] = "CKM_RSA_PKCS_PSS",
	[0xe] = "CKM_SHA1_RSA_PKCS_PSS",
	[0x10] = "CKM_DSA_KEY_PAIR_GEN",
	[0x11] = "CKM_DSA",
	[0x12] = "CKM_DSA_SHA1",
	[0x20] = "CKM_DH_PKCS_KEY_PAIR_GEN",
	[0x21] = "CKM_DH_PKCS_DERIVE",
	[0x30] = "CKM_X9_42_DH_KEY_PAIR_GEN",
	[0x31] = "CKM_X9_42_DH_DERIVE",
	[0x32] = "CKM_X9_42_DH_HYBRID_DERIVE",
	[0x33] = "CKM_X9_42_MQV_DERIVE",
	[0x40] = "CKM_SHA256_RSA_PKCS",
	[0x41] = "CKM_SHA384_RSA_PKCS",
	[0x42] = "CKM_SHA512_RSA_PKCS",
	[0x43] = "CKM_SHA256_RSA_PKCS_PSS",
	[0x44] = "CKM_SHA384_RSA_PKCS_PSS",
	[0x45] = "CKM_SHA512_RSA_PKCS_PSS",
	[0x100] = "CKM_RC2_KEY_GEN",
	[0x101] = "CKM_RC2_ECB",
	[0x102] = "CKM_RC2_CBC",
	[0x103] = "CKM_RC2_MAC",
	[0x104] = "CKM_RC2_MAC_GENERAL",
	[0x105] = "CKM_RC2_CBC_PAD",
	[0x110] = "CKM_RC4_KEY_GEN",
	[0x111] = "CKM_RC4",
	[0x120] = "CKM_DES_KEY_GEN",
	[0x121] = "CKM_DES_ECB",
	[0x122] = "CKM_DES_CBC",
	[0x123] = "CKM_DES_MAC",
	[0x124] = "CKM_DES_MAC_GENERAL",
	[0x125] = "CKM_DES_CBC_PAD",
	[0x130] = "CKM_DES2_KEY_GEN",
	[0x131] = "CKM_DES3_KEY_GEN",
	[0x132] = "CKM_DES3_ECB",
	[0x133] = "CKM_DES3_CBC",
	[0x134] = "CKM_DES3_MAC",
	[0x135] = "CKM_DES3_MAC_GENERAL",
	[0x136] = "CKM_DES3_CBC_PAD",
	[0x140] = "CKM_CDMF_KEY_GEN",
	[0x141] = "CKM_CDMF_ECB",
	[0x142] = "CKM_CDMF_CBC",
	[0x143] = "CKM_CDMF_MAC",
	[0x144] = "CKM_CDMF_MAC_GENERAL",
	[0x145] = "CKM_CDMF_CBC_PAD",
	[0x200] = "CKM_MD2",
	[0x201] = "CKM_MD2_HMAC",
	[0x202] = "CKM_MD2_HMAC_GENERAL",
	[0x210] = "CKM_MD5",
	[0x211] = "CKM_MD5_HMAC",
	[0x212] = "CKM_MD5_HMAC_GENERAL",
	[0x220] = "CKM_SHA_1",
	[0x221] = "CKM_SHA_1_HMAC",
	[0x222] = "CKM_SHA_1_HMAC_GENERAL",
	[0x230] = "CKM_RIPEMD128",
	[0x231] = "CKM_RIPEMD128_HMAC",
	[0x232] = "CKM_RIPEMD128_HMAC_GENERAL",
	[0x240] = "CKM_RIPEMD160",
	[0x241] = "CKM_RIPEMD160_HMAC",
	[0x242] = "CKM_RIPEMD160_HMAC_GENERAL",
	[0x250] = "CKM_SHA256",
	[0x251] = "CKM_SHA256_HMAC",
	[0x252] = "CKM_SHA256_HMAC_GENERAL",
	[0x260] = "CKM_SHA384",
	[0x261] = "CKM_SHA384_HMAC",
	[0x262] = "CKM_SHA384_HMAC_GENERAL",
	[0x270] = "CKM_SHA512",
	[0x271] = "CKM_SHA512_HMAC",
	[0x272] = "CKM_SHA512_HMAC_GENERAL",
	[0x300] = "CKM_CAST_KEY_GEN",
	[0x301] = "CKM_CAST_ECB",
	[0x302] = "CKM_CAST_CBC",
	[0x303] = "CKM_CAST_MAC",
	[0x304] = "CKM_CAST_MAC_GENERAL",
	[0x305] = "CKM_CAST_CBC_PAD",
	[0x310] = "CKM_CAST3_KEY_GEN",
	[0x311] = "CKM_CAST3_ECB",
	[0x312] = "CKM_CAST3_CBC",
	[0x313] = "CKM_CAST3_MAC",
	[0x314] = "CKM_CAST3_MAC_GENERAL",
	[0x315] = "CKM_CAST3_CBC_PAD",
	[0x320] = "CKM_CAST128_KEY_GEN",
	[0x321] = "CKM_CAST128_ECB",
	[0x322] = "CKM_CAST128_CBC",
	[0x323] = "CKM_CAST128_MAC",
	[0x324] = "CKM_CAST128_MAC_GENERAL",
	[0x325] = "CKM_CAST128_CBC_PAD",
	[0x330] = "CKM_RC5_KEY_GEN",
	[0x331] = "CKM_RC5_ECB",
	[0x332] = "CKM_RC5_CBC",
	[0x333] = "CKM_RC5_MAC",
	[0x334] = "CKM_RC5_MAC_GENERAL",
	[0x335] = "CKM_RC5_CBC_PAD",
	[0x340] = "CKM_IDEA_KEY_GEN",
	[0x341] = "CKM_IDEA_ECB",
	[0x342] = "CKM_IDEA_CBC",
	[0x343] = "CKM_IDEA_MAC",
	[0x344] = "CKM_IDEA_MAC_GENERAL",
	[0x345] = "CKM_IDEA_CBC_PAD",
	[0x350] = "CKM_GENERIC_SECRET_KEY_GEN",
	[0x360] = "CKM_CONCATENATE_BASE_AND_KEY",
	[0x362] = "CKM_CONCATENATE_BASE_AND_DATA",
	[0x363] = "CKM_CONCATENATE_DATA_AND_BASE",
	[0x364] = "CKM_XOR_BASE_AND_DATA",
	[0x365] = "CKM_EXTRACT_KEY_FROM_KEY",
	[0x370] = "CKM_SSL3_PRE_MASTER_KEY_GEN",
	[0x371] = "CKM_SSL3_MASTER_KEY_DERIVE",
	[0x372] = "CKM_SSL3_KEY_AND_MAC_DERIVE",
	[0x373] = "CKM_SSL3_MASTER_KEY_DERIVE_DH",
	[0x374] = "CKM_TLS_PRE_MASTER_KEY_GEN",
	[0x375] = "CKM_TLS_MASTER_KEY_DERIVE",
	[0x376] = "CKM_TLS_KEY_AND_MAC_DERIVE",
	[0x377] = "CKM_TLS_MASTER_KEY_DERIVE_DH",
	[0x380] = "CKM_SSL3_MD5_MAC",
	[0x381] = "CKM_SSL3_SHA1_MAC",
	[0x390] = "CKM_MD5_KEY_DERIVATION",
	[0x391] = "CKM_MD2_KEY_DERIVATION",
	[0x392] = "CKM_SHA1_KEY_DERIVATION",
	[0x3a0] = "CKM_PBE_MD2_DES_CBC",
	[0x3a1] = "CKM_PBE_MD5_DES_CBC",
	[0x3a2] = "CKM_PBE_MD5_CAST_CBC",
	[0x3a3] = "CKM_PBE_MD5_CAST3_CBC",
	[0x3a4] = "CKM_PBE_MD5_CAST128_CBC",
	[0x3a5] = "CKM_PBE_SHA1_CAST128_CBC",
	[0x3a6] = "CKM_PBE_SHA1_RC4_128",
	[0x3a7] = "CKM_PBE_SHA1_RC4_40",
	[0x3a8] = "CKM_PBE_SHA1_DES3_EDE_CBC",
	[0x3a9] = "CKM_PBE_SHA1_DES2_EDE_CBC",
	[0x3aa] = "CKM_PBE_SHA1_RC2_128_CBC",
	[0x3ab] = "CKM_PBE_SHA1_RC2_40_CBC",
	[0x3b0] = "CKM_PKCS5_PBKD2",
	[0x3c0] = "CKM_PBA_SHA1_WITH_SHA1_HMAC",
	[0x400] = "CKM_KEY_WRAP_LYNKS",
	[0x401] = "CKM_KEY_WRAP_SET_OAEP",
	[0x1000] = "CKM_SKIPJACK_KEY_GEN",
	[0x1001] = "CKM_SKIPJACK_ECB64",
	[0x1002] = "CKM_SKIPJACK_CBC64",
	[0x1003] = "CKM_SKIPJACK_OFB64",
	[0x1004] = "CKM_SKIPJACK_CFB64",
	[0x1005] = "CKM_SKIPJACK_CFB32",
	[0x1006] = "CKM_SKIPJACK_CFB16",
	[0x1007] = "CKM_SKIPJACK_CFB8",
	[0x1008] = "CKM_SKIPJACK_WRAP",
	[0x1009] = "CKM_SKIPJACK_PRIVATE_WRAP",
	[0x100a] = "CKM_SKIPJACK_RELAYX",
	[0x1010] = "CKM_KEA_KEY_PAIR_GEN",
	[0x1011] = "CKM_KEA_KEY_DERIVE",
	[0x1020] = "CKM_FORTEZZA_TIMESTAMP",
	[0x1030] = "CKM_BATON_KEY_GEN",
	[0x1031] = "CKM_BATON_ECB128",
	[0x1032] = "CKM_BATON_ECB96",
	[0x1033] = "CKM_BATON_CBC128",
	[0x1034] = "CKM_BATON_COUNTER",
	[0x1035] = "CKM_BATON_SHUFFLE",
	[0x1036] = "CKM_BATON_WRAP",
	[0x1040] = "CKM_ECDSA_KEY_PAIR_GEN",
	[0x1041] = "CKM_ECDSA",
	[0x1042] = "CKM_ECDSA_SHA1",
	[0x1050] = "CKM_ECDH1_DERIVE",
	[0x1051] = "CKM_ECDH1_COFACTOR_DERIVE",
	[0x1052] = "CKM_ECMQV_DERIVE",
	[0x1060] = "CKM_JUNIPER_KEY_GEN",
	[0x1061] = "CKM_JUNIPER_ECB128",
	[0x1062] = "CKM_JUNIPER_CBC128",
	[0x1063] = "CKM_JUNIPER_COUNTER",
	[0x1064] = "CKM_JUNIPER_SHUFFLE",
	[0x1065] = "CKM_JUNIPER_WRAP",
	[0x1070] = "CKM_FASTHASH",
	[0x1080] = "CKM_AES_KEY_GEN",
	[0x1081] = "CKM_AES_ECB",
	[0x1082] = "CKM_AES_CBC",
	[0x1083] = "CKM_AES_MAC",
	[0x1084] = "CKM_AES_MAC_GENERAL",
	[0x1085] = "CKM_AES_CBC_PAD",
	[0x2000] = "CKM_DSA_PARAMETER_GEN",
	[0x2001] = "CKM_DH_PKCS_PARAMETER_GEN",
	[0x2002] = "CKM_X9_42_DH_PARAMETER_GEN",
	[0x1200] = "CKM_GOSTR3410_KEY_PAIR_GEN",
	[0x1201] = "CKM_GOSTR3410",
	[0x1202] = "CKM_GOSTR3410_WITH_GOSTR3411",
	[0x1203] = "CKM_GOSTR3410_KEY_WRAP",
	[0x1204] = "CKM_GOSTR3410_DERIVE",
	[0x1210] = "CKM_GOSTR3411",
	[0x1211] = "CKM_GOSTR3411_HMAC",
	[0x255] = "CKM_SHA224",
	[0x256] = "CKM_SHA224_HMAC",
	[0x257] = "CKM_SHA224_HMAC_GENERAL",
	[0x46] = "CKM_SHA224_RSA_PKCS",
	[0x47] = "CKM_SHA224_RSA_PKCS_PSS",
	[0x396] = "CKM_SHA224_KEY_DERIVATION",
	[0x550] = "CKM_CAMELLIA_KEY_GEN",
	[0x551] = "CKM_CAMELLIA_ECB",
	[0x552] = "CKM_CAMELLIA_CBC",
	[0x553] = "CKM_CAMELLIA_MAC",
	[0x554] = "CKM_CAMELLIA_MAC_GENERAL",
	[0x555] = "CKM_CAMELLIA_CBC_PAD",
	[0x556] = "CKM_CAMELLIA_ECB_ENCRYPT_DATA",
	[0x557] = "CKM_CAMELLIA_CBC_ENCRYPT_DATA"
};

void
pkcs11_mechanism_list(FILE * outfile, const char *url, unsigned int flags,
		      common_info_st * info)
{
	int ret;
	int idx;
	unsigned long mechanism;
	const char *str;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	idx = 0;
	do {
		ret =
		    gnutls_pkcs11_token_get_mechanism(url, idx++,
						      &mechanism);
		if (ret >= 0) {
			str = NULL;
			if (mechanism <
			    sizeof(mech_list) / sizeof(mech_list[0]))
				str = mech_list[mechanism];
			if (str == NULL)
				str = "UNKNOWN";

			fprintf(outfile, "[0x%.4lx] %s\n", mechanism, str);
		}
	}
	while (ret >= 0);


	return;
}

void
pkcs11_get_random(FILE * outfile, const char *url, unsigned bytes,
		  common_info_st * info)
{
	int ret;
	uint8_t *output;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	output = malloc(bytes);
	if (output == NULL) {
		fprintf(stderr, "Memory error\n");
		exit(1);
	}

	ret = gnutls_pkcs11_token_get_random(url, output, bytes);
	if (ret < 0) {
		fprintf(stderr, "gnutls_pkcs11_token_get_random: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	fwrite(output, 1, bytes, outfile);
	free(output);

	return;
}

static
void pkcs11_set_val(FILE * outfile, const char *url, int detailed,
		   unsigned int flags, common_info_st * info,
		   gnutls_pkcs11_obj_info_t val_type, const char *val)
{
	int ret;
	gnutls_pkcs11_obj_t obj;

	pkcs11_common(info);

	FIX(url, outfile, detailed, info);
	CHECK_LOGIN_FLAG(flags);

	ret = gnutls_pkcs11_obj_init(&obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_obj_set_info(obj, val_type, val, strlen(val), flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}
	gnutls_pkcs11_obj_deinit(obj);

	return;
}

void pkcs11_set_id(FILE * outfile, const char *url, int detailed,
		   unsigned int flags, common_info_st * info,
		   const char *id)
{
	pkcs11_set_val(outfile, url, detailed, flags, info, GNUTLS_PKCS11_OBJ_ID_HEX, id);
}

void pkcs11_set_label(FILE * outfile, const char *url, int detailed,
		   unsigned int flags, common_info_st * info,
		   const char *label)
{
	pkcs11_set_val(outfile, url, detailed, flags, info, GNUTLS_PKCS11_OBJ_LABEL, label);
}
