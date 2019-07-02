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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <config.h>

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
			app_exit(1); \
		} \
		_saved_url = (void*)url; \
	}

#define UNFIX gnutls_free(_saved_url);_saved_url = NULL

#define KEEP_LOGIN_FLAGS(flags) (flags & (GNUTLS_PKCS11_OBJ_FLAG_LOGIN|GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO))

#define CHECK_LOGIN_FLAG(url, flags) \
	if ((flags & KEEP_LOGIN_FLAGS(flags)) == 0) { \
		unsigned _tflags; \
		int _r = gnutls_pkcs11_token_get_flags(url, &_tflags); \
		if (_r >= 0 && (_tflags & GNUTLS_PKCS11_TOKEN_LOGIN_REQUIRED)) { \
			flags |= GNUTLS_PKCS11_OBJ_FLAG_LOGIN; \
			fprintf(stderr, \
				"note: assuming --login for this operation.\n"); \
		} else { \
			fprintf(stderr, \
				"warning: --login was not specified and it may be required for this operation.\n"); \
		} \
	}


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
			app_exit(1);
		}
	}

	ret = gnutls_pkcs11_delete_url(url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	fprintf(outfile, "\n%d objects deleted\n", ret);

	return;
}

static
const char *get_key_algo_type(gnutls_pkcs11_obj_type_t otype, const char *objurl, unsigned flags, time_t *exp)
{
	int ret;
	gnutls_pubkey_t pubkey = NULL;
	gnutls_privkey_t privkey = NULL;
	gnutls_x509_crt_t crt = NULL;
	static char str[256];
	const char *p;
	unsigned int bits;
	gnutls_pk_algorithm_t pk;
	gnutls_ecc_curve_t curve;

	if (exp)
		*exp = -1;

	switch (otype) {
		case GNUTLS_PKCS11_OBJ_X509_CRT:
			ret = gnutls_x509_crt_init(&crt);
			if (ret < 0)
				goto fail;

			ret = gnutls_x509_crt_import_url(crt, objurl, flags);
			if (ret < 0)
				goto fail;
			ret = gnutls_x509_crt_get_pk_algorithm(crt, &bits);
			if (ret < 0)
				goto fail;
			pk = ret;

			p = gnutls_pk_get_name(pk);
			if (p) {
				if ((pk == GNUTLS_PK_RSA || pk == GNUTLS_PK_DSA) && bits > 0) {
					snprintf(str, sizeof(str), "%s-%d", p, bits);
					p = str;
				} else if (pk == GNUTLS_PK_ECDSA && gnutls_x509_crt_get_pk_ecc_raw(crt, &curve, NULL, NULL) >= 0) {
					snprintf(str, sizeof(str), "%s-%s", p, gnutls_ecc_curve_get_name(curve));
					p = str;
				}
			}

			if (exp)
				*exp = gnutls_x509_crt_get_expiration_time(crt);

			gnutls_x509_crt_deinit(crt);
			return p;
		case GNUTLS_PKCS11_OBJ_PUBKEY:
			ret = gnutls_pubkey_init(&pubkey);
			if (ret < 0)
				goto fail;

			ret = gnutls_pubkey_import_url(pubkey, objurl, flags);
			if (ret < 0)
				goto fail;
			ret = gnutls_pubkey_get_pk_algorithm(pubkey, &bits);
			if (ret < 0)
				goto fail;
			pk = ret;

			p = gnutls_pk_get_name(pk);
			if (p) {
				if ((pk == GNUTLS_PK_RSA || pk == GNUTLS_PK_DSA) && bits > 0) {
					snprintf(str, sizeof(str), "%s-%d", p, bits);
					p = str;
				} else if (pk == GNUTLS_PK_ECDSA && gnutls_pubkey_export_ecc_raw(pubkey, &curve, NULL, NULL) >= 0) {
					snprintf(str, sizeof(str), "%s-%s", p, gnutls_ecc_curve_get_name(curve));
					p = str;
				}
			}

			gnutls_pubkey_deinit(pubkey);
			return p;
		case GNUTLS_PKCS11_OBJ_PRIVKEY:
			ret = gnutls_privkey_init(&privkey);
			if (ret < 0)
				goto fail;

			ret = gnutls_privkey_import_url(privkey, objurl, flags);
			if (ret < 0)
				goto fail;
			ret = gnutls_privkey_get_pk_algorithm(privkey, &bits);
			if (ret < 0)
				goto fail;
			pk = ret;

			p = gnutls_pk_get_name(pk);
			if (p) {
				if ((pk == GNUTLS_PK_RSA || pk == GNUTLS_PK_DSA) && bits > 0) {
					snprintf(str, sizeof(str), "%s-%d", p, bits);
					p = str;
				} else if (pk == GNUTLS_PK_ECDSA && gnutls_privkey_export_ecc_raw(privkey, &curve, NULL, NULL, NULL) >= 0) {
					snprintf(str, sizeof(str), "%s-%s", p, gnutls_ecc_curve_get_name(curve));
					p = str;
				}
			}

			gnutls_privkey_deinit(privkey);
			return p;
		default:
 fail:
			if (crt)
				gnutls_x509_crt_deinit(crt);
			if (pubkey)
				gnutls_pubkey_deinit(pubkey);
			if (privkey)
				gnutls_privkey_deinit(privkey);
			return NULL;
	}
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
	time_t exp;

	pkcs11_common(info);

	FIX(url, outfile, detailed, info);

	ret = gnutls_pkcs11_token_get_flags(url, &flags);
	if (ret < 0) {
		flags = 0;
	}

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
	} else { /* also PKCS11_TYPE_INFO */
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
		app_exit(1);
	}

	if (crt_list_size == 0) {
		fprintf(stderr, "No matching objects found\n");
		app_exit(2);
	}

	for (i = 0; i < crt_list_size; i++) {
		char buf[256];
		size_t size;
		const char *p;
		unsigned int oflags;
		const char *vendor;
		char *objurl;
		char timebuf[SIMPLE_CTIME_BUF_SIZE];

		ret =
		    gnutls_pkcs11_obj_export_url(crt_list[i], detailed,
						 &output);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			app_exit(1);
		}

		if (info->only_urls) {
			fprintf(outfile, "%s\n", output);
			gnutls_free(output);
			continue;
		} else {
			fprintf(outfile, "Object %d:\n\tURL: %s\n", i, output);
		}

		/* copy vendor query (e.g. pin-value) from the original URL */
		vendor = strrchr(url, '?');
		if (vendor) {
			objurl = gnutls_malloc(strlen(output) + strlen(vendor) + 1);
			strcpy(objurl, output);
			strcat(objurl, vendor);
		} else {
			objurl = gnutls_strdup(output);
		}

		p = NULL;
		otype = gnutls_pkcs11_obj_get_type(crt_list[i]);
		if (otype == GNUTLS_PKCS11_OBJ_PRIVKEY ||
		    otype == GNUTLS_PKCS11_OBJ_PUBKEY ||
		    otype == GNUTLS_PKCS11_OBJ_X509_CRT) {
			p = get_key_algo_type(otype, objurl, obj_flags, &exp);
		}

		if (p) {
			fprintf(outfile, "\tType: %s (%s)\n",
				gnutls_pkcs11_type_get_name(otype), p);
		} else {
			fprintf(outfile, "\tType: %s\n",
				gnutls_pkcs11_type_get_name(otype));
		}

		if (otype == GNUTLS_PKCS11_OBJ_X509_CRT && exp != -1) {
			fprintf(outfile, "\tExpires: %s\n", simple_ctime(&exp, timebuf));
		}

		gnutls_free(output);
		gnutls_free(objurl);

		size = sizeof(buf);
		ret =
		    gnutls_pkcs11_obj_get_info(crt_list[i],
					       GNUTLS_PKCS11_OBJ_LABEL,
					       buf, &size);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			app_exit(1);
		}
		fprintf(outfile, "\tLabel: %s\n", buf);

		oflags = 0;
		ret = gnutls_pkcs11_obj_get_flags(crt_list[i], &oflags);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
			app_exit(1);
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
			if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
				fprintf(outfile, "\tID: (too long)\n");
			} else {
				fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
				app_exit(1);
			}
		} else {
			fprintf(outfile, "\tID: %s\n", buf);
		}

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
	gnutls_digest_algorithm_t hash;
	gnutls_sign_algorithm_t sig_algo;

	pkcs11_common(info);

	FIX(url, outfile, 0, info);

	data.data = (void*)TEST_DATA;
	data.size = sizeof(TEST_DATA)-1;

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_privkey_import_url(privkey, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot import private key: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, GNUTLS_KEY_DIGITAL_SIGNATURE, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot import public key: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	pk = gnutls_privkey_get_pk_algorithm(privkey, NULL);

	if (info->hash == GNUTLS_DIG_UNKNOWN)
		hash = GNUTLS_DIG_SHA256;
	else
		hash = info->hash;

	if (info->rsa_pss_sign && pk == GNUTLS_PK_RSA)
		pk = GNUTLS_PK_RSA_PSS;

	sig_algo = gnutls_pk_to_sign(pk, hash);
	if (sig_algo == GNUTLS_SIGN_UNKNOWN) {
		fprintf(stderr, "No supported signature algorithm for %s and %s\n",
			gnutls_pk_get_name(pk), gnutls_digest_get_name(hash));
		app_exit(1);
	}

	fprintf(stderr, "Signing using %s... ", gnutls_sign_get_name(sig_algo));

	ret = gnutls_privkey_sign_data2(privkey, sig_algo, 0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot sign data: %s\n",
			gnutls_strerror(ret));
		/* in case of unsupported signature algorithm allow
		 * calling apps to distinguish error codes (used
		 * by testpkcs11.sh */
		if (ret == GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM)
			app_exit(2);
		app_exit(1);
	}

	fprintf(stderr, "ok\n");

	fprintf(stderr, "Verifying against private key parameters... ");
	ret = gnutls_pubkey_verify_data2(pubkey, sig_algo,
					 0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot verify signed data: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	fprintf(stderr, "ok\n");

	/* now try to verify against a public key within the token */
	gnutls_pubkey_deinit(pubkey);
	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
			__LINE__, gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pubkey_import_url(pubkey, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a corresponding public key object in token: %s\n",
			gnutls_strerror(ret));
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			app_exit(0);
		app_exit(1);
	}

	fprintf(stderr, "Verifying against public key in the token... ");
	ret = gnutls_pubkey_verify_data2(pubkey, sig_algo,
					 0, &data, &sig);
	if (ret < 0) {
		fprintf(stderr, "Cannot verify signed data: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
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
		app_exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pkcs11_obj_export3(obj, info->outcert_format, &t);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		app_exit(1);
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
		app_exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, obj_flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	/* make a crt */
	ret = gnutls_x509_crt_init(&xcrt);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_x509_crt_import_pkcs11(xcrt, obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pkcs11_obj_export3(obj, GNUTLS_X509_FMT_PEM, &t);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__,
				__LINE__, gnutls_strerror(ret));
		app_exit(1);
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
			app_exit(1);
		}

		fwrite(t.data, 1, t.size, outfile);
		fputs("\n\n", outfile);

		gnutls_x509_crt_deinit(xcrt);

		ret = gnutls_x509_crt_init(&xcrt);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
			app_exit(1);
		}

		ret = gnutls_x509_crt_import(xcrt, &t, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fprintf(stderr, "Error in %s:%d: %s\n", __func__,
					__LINE__, gnutls_strerror(ret));
			app_exit(1);
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

	print = 0;
	fputs("\tFlags: ", outfile);
	if (flags & GNUTLS_PKCS11_TOKEN_RNG) {
		fputs("RNG", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_LOGIN_REQUIRED) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Requires login", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_PROTECTED_AUTHENTICATION_PATH) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("External PIN", outfile);
		print++;
	}

	if (!(flags & GNUTLS_PKCS11_TOKEN_INITIALIZED)) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Uninitialized", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_USER_PIN_COUNT_LOW) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("uPIN low count", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_USER_PIN_FINAL_TRY) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Final uPIN attempt", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_USER_PIN_FINAL_TRY) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("uPIN locked", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_SO_PIN_COUNT_LOW) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("SO-PIN low count", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_SO_PIN_FINAL_TRY) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Final SO-PIN attempt", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_SO_PIN_FINAL_TRY) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("SO-PIN locked", outfile);
		print++;
	}

	if (!(flags & GNUTLS_PKCS11_TOKEN_USER_PIN_INITIALIZED)) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("uPIN uninitialized", outfile);
		print++;
	}

	if (flags & GNUTLS_PKCS11_TOKEN_ERROR_STATE) {
		if (print != 0)
			fputs(", ", outfile);
		fputs("Error state", outfile);
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
			app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
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
		app_exit(1);
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
		app_exit(1);
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
				app_exit(1);
			}

			cid->data = gnutls_malloc(size);
			cid->size = size;
			if (cid->data == NULL) {
				fprintf(stderr, "memory error\n");
				app_exit(1);
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
		app_exit(1);
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
		app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
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
				app_exit(1);
			}

			cid->data = gnutls_malloc(size);
			cid->size = size;
			if (cid->data == NULL) {
				fprintf(stderr, "memory error\n");
				app_exit(1);
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

	CHECK_LOGIN_FLAG(url, flags);
	if (label == NULL && info->batch == 0) {
		label = read_str("warning: The object's label was not specified.\nLabel: ");
	}

	if (id != NULL) {
		raw_id_size = sizeof(raw_id);
		ret = gnutls_hex2bin(id, strlen(id), raw_id, &raw_id_size);
		if (ret < 0) {
			fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
			app_exit(1);
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
			app_exit(1);
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
					app_exit(1);
				}
				fprintf(stderr, "note: will re-use ID %s from corresponding public key\n", hex.data);
				gnutls_free(hex.data);

			} else { /* no luck, try to get a corresponding private key */
				find_same_privkey_with_id(url, xcrt, &cid, KEEP_LOGIN_FLAGS(flags));
				if (cid.data) {
					ret = gnutls_hex_encode2(&cid, &hex);
					if (ret < 0) {
						fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
						app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
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
			app_exit(1);
		}
		gnutls_pubkey_deinit(xpubkey);
	}

	if (xkey == NULL && xcrt == NULL && secret_key == NULL && xpubkey == NULL) {
		fprintf(stderr,
			"You must use --load-privkey, --load-certificate, --load-pubkey or --secret-key to load the file to be copied\n");
		app_exit(1);
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

	CHECK_LOGIN_FLAG(url, flags);

	if (id != NULL) {
		raw_id_size = sizeof(raw_id);
		ret = gnutls_hex2bin(id, strlen(id), raw_id, &raw_id_size);
		if (ret < 0) {
			fprintf(stderr, "Error converting hex: %s\n", gnutls_strerror(ret));
			app_exit(1);
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
		app_exit(1);
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

	CHECK_LOGIN_FLAG(url, flags);

	if (outfile == stderr || outfile == stdout) {
		fprintf(stderr, "warning: no --outfile was specified and the public key will be printed on screen.\n");
		sleep(3);
	}

	ret = gnutls_pkcs11_privkey_init(&pkey);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pkcs11_privkey_import_url(pkey, url, 0);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret =
	    gnutls_pkcs11_privkey_export_pubkey(pkey,
						GNUTLS_X509_FMT_PEM, &pubkey,
						flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
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
	char so_pin[MAX_PIN_LEN];

	pkcs11_common(info);

	if (url == NULL) {
		fprintf(stderr, "error: no token URL given to initialize!\n");
		app_exit(1);
	}

	if (label == NULL) {
		fprintf(stderr, "error: no label provided for token initialization!\n");
		app_exit(1);
	}

	if (info->so_pin != NULL) {
		snprintf(so_pin, sizeof(so_pin), "%s", info->so_pin);
	} else {
		getenv_copy(so_pin, sizeof(so_pin), "GNUTLS_SO_PIN");
		if (so_pin[0] == 0 && info->batch == 0)
			getpass_copy(so_pin, sizeof(so_pin), "Enter Security Officer's PIN: ");
		if (so_pin[0] == 0)
			app_exit(1);
	}

	if (so_pin[0] == '\n' || so_pin[0] == 0)
		app_exit(1);

	fprintf(stderr, "Initializing token... ");
	ret = gnutls_pkcs11_token_init(url, so_pin, label);
	if (ret < 0) {
		fprintf(stderr, "\nError in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "\nToken was successfully initialized; use --initialize-pin and --initialize-so-pin to set or reset PINs\n");

	return;
}

void
pkcs11_set_token_pin(FILE * outfile, const char *url, common_info_st * info, unsigned so)
{
	int ret;
	char newpin[MAX_PIN_LEN] = "";

	pkcs11_common(info);

	if (url == NULL) {
		fprintf(stderr, "error: no token URL given to initialize!\n");
		app_exit(1);
	}

	if (so)
		fprintf(stderr, "Setting admin's PIN...\n");
	else
		fprintf(stderr, "Setting user's PIN...\n");

	if (so) {
		getenv_copy(newpin, sizeof(newpin), "GNUTLS_NEW_SO_PIN");
		if (newpin[0] == 0 && info->batch == 0) {
			getpass_copy(newpin, sizeof(newpin), "Enter Administrators's new PIN: ");
		}
	} else {
		if (info->pin != NULL) {
			snprintf(newpin, sizeof(newpin), "%s", info->pin);
		} else {
			getenv_copy(newpin, sizeof(newpin), "GNUTLS_PIN");
			if (newpin[0] == 0 && info->batch == 0)
				getpass_copy(newpin, sizeof(newpin), "Enter User's new PIN: ");
		}
	}

	if (newpin[0] == 0 || newpin[0] == '\n') {
		fprintf(stderr, "No PIN was given to change\n");
		app_exit(1);
	}

	ret = gnutls_pkcs11_token_set_pin(url, NULL, newpin, (so!=0)?GNUTLS_PIN_SO:GNUTLS_PIN_USER);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	return;
}

#include "mech-list.h"

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
		app_exit(1);
	}

	ret = gnutls_pkcs11_token_get_random(url, output, bytes);
	if (ret < 0) {
		fprintf(stderr, "gnutls_pkcs11_token_get_random: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
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

	CHECK_LOGIN_FLAG(url, flags);

	ret = gnutls_pkcs11_obj_init(&obj);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_pkcs11_obj_import_url(obj, url, flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret =
	    gnutls_pkcs11_obj_set_info(obj, val_type, val, strlen(val), flags);
	if (ret < 0) {
		fprintf(stderr, "Error in %s:%d: %s\n", __func__, __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
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
