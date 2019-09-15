/*
 * Copyright (C) 2007-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2017 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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

/* Functions for printing X.509 Certificate structures
 */

#include "gnutls_int.h"
#include <common.h>
#include <x509.h>
#include <x509_int.h>
#include <num.h>
#include "errors.h"
#include "hello_ext.h"
#include "ip.h"

#define addf _gnutls_buffer_append_printf
#define adds _gnutls_buffer_append_str

#define NON_NULL(x) (((x)!=NULL)?((char*)(x)):"")
#define ERROR_STR (char*) "(error)"

static void print_idn_name(gnutls_buffer_st *str, const char *prefix, const char *type, gnutls_datum_t *name)
{
	unsigned printable = 1;
	unsigned is_printed = 0;
	gnutls_datum_t out = {NULL, 0};
	int ret;

	if (!_gnutls_str_is_print((char*)name->data, name->size))
		printable = 0;

	is_printed = 0;
	if (!printable) {
		addf(str,  _("%s%s: %.*s (contains illegal chars)\n"), prefix, type, name->size, NON_NULL(name->data));
		is_printed = 1;
	} else if (name->data != NULL) {
		if (strstr((char*)name->data, "xn--") != NULL) {
			ret = gnutls_idna_reverse_map((char*)name->data, name->size, &out, 0);
			if (ret >= 0) {
				addf(str,  _("%s%s: %.*s (%s)\n"), prefix, type, name->size, NON_NULL(name->data), out.data);
				is_printed = 1;
				gnutls_free(out.data);
			}
		}
	}

	if (is_printed == 0) {
		addf(str,  _("%s%s: %.*s\n"), prefix, type, name->size, NON_NULL(name->data));
	}
}

static void print_idn_email(gnutls_buffer_st *str, const char *prefix, const char *type, gnutls_datum_t *name)
{
	unsigned printable = 1;
	unsigned is_printed = 0;
	gnutls_datum_t out = {NULL, 0};
	int ret;

	if (!_gnutls_str_is_print((char*)name->data, name->size))
		printable = 0;

	is_printed = 0;
	if (!printable) {
		addf(str,  _("%s%s: %.*s (contains illegal chars)\n"), prefix, type, name->size, NON_NULL(name->data));
		is_printed = 1;
	} else if (name->data != NULL) {
		if (strstr((char*)name->data, "xn--") != NULL) {
			ret = _gnutls_idna_email_reverse_map((char*)name->data, name->size, &out);
			if (ret >= 0) {
				addf(str,  _("%s%s: %.*s (%s)\n"), prefix, type, name->size, NON_NULL(name->data), out.data);
				is_printed = 1;
				gnutls_free(out.data);
			}
		}
	}

	if (is_printed == 0) {
		addf(str,  _("%s%s: %.*s\n"), prefix, type, name->size, NON_NULL(name->data));
	}
}

static void
print_name(gnutls_buffer_st *str, const char *prefix, unsigned type, gnutls_datum_t *name, unsigned ip_is_cidr)
{
	char *sname = (char*)name->data;
	char str_ip[64];
	const char *p;

	if ((type == GNUTLS_SAN_DNSNAME || type == GNUTLS_SAN_OTHERNAME_XMPP
	     || type == GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL
	     || type == GNUTLS_SAN_RFC822NAME
	     || type == GNUTLS_SAN_URI) && sname != NULL && strlen(sname) != name->size) {
		adds(str,
		     _("warning: SAN contains an embedded NUL, "
			      "replacing with '!'\n"));
		while (strlen(sname) < name->size)
			name->data[strlen(sname)] = '!';
	}

	switch (type) {
	case GNUTLS_SAN_DNSNAME:
		print_idn_name(str, prefix, "DNSname", name);
		break;

	case GNUTLS_SAN_RFC822NAME:
		print_idn_email(str, prefix, "RFC822Name", name);
		break;

	case GNUTLS_SAN_URI:
		addf(str,  _("%sURI: %.*s\n"), prefix, name->size, NON_NULL(name->data));
		break;

	case GNUTLS_SAN_IPADDRESS:
		if (!ip_is_cidr)
			p = _gnutls_ip_to_string(name->data, name->size, str_ip, sizeof(str_ip));
		else
			p = _gnutls_cidr_to_string(name->data, name->size, str_ip, sizeof(str_ip));
		if (p == NULL)
			p = ERROR_STR;
		addf(str, "%sIPAddress: %s\n", prefix, p);
		break;

	case GNUTLS_SAN_DN:
		addf(str,  _("%sdirectoryName: %.*s\n"), prefix, name->size, NON_NULL(name->data));
		break;

	case GNUTLS_SAN_REGISTERED_ID:
			addf(str,  _("%sRegistered ID: %.*s\n"), prefix, name->size, NON_NULL(name->data));
			break;

	case GNUTLS_SAN_OTHERNAME_XMPP:
		addf(str,  _("%sXMPP Address: %.*s\n"), prefix, name->size, NON_NULL(name->data));
		break;

	case GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL:
		addf(str,  _("%sKRB5Principal: %.*s\n"), prefix, name->size, NON_NULL(name->data));
		break;

	default:
		addf(str,  _("%sUnknown name: "), prefix);
		_gnutls_buffer_hexprint(str, name->data, name->size);
		adds(str, "\n");
		break;
	}
}

static char *get_pk_name(gnutls_x509_crt_t cert, unsigned *bits)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	oid_size = sizeof(oid);
	int ret;

	ret = gnutls_x509_crt_get_pk_algorithm(cert, bits);
	if (ret > 0) {
		const char *name = gnutls_pk_algorithm_get_name(ret);

		if (name != NULL)
			return gnutls_strdup(name);
	}

	ret = gnutls_x509_crt_get_pk_oid(cert, oid, &oid_size);
	if (ret < 0)
		return NULL;

	return gnutls_strdup(oid);
}

static char *crq_get_pk_name(gnutls_x509_crq_t crq)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	oid_size = sizeof(oid);
	int ret;

	ret = gnutls_x509_crq_get_pk_algorithm(crq, NULL);
	if (ret > 0) {
		const char *name = gnutls_pk_algorithm_get_name(ret);

		if (name != NULL)
			return gnutls_strdup(name);
	}

	ret = gnutls_x509_crq_get_pk_oid(crq, oid, &oid_size);
	if (ret < 0)
		return NULL;

	return gnutls_strdup(oid);
}

static char *get_sign_name(gnutls_x509_crt_t cert, int *algo)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	oid_size = sizeof(oid);
	int ret;

	*algo = 0;

	ret = gnutls_x509_crt_get_signature_algorithm(cert);
	if (ret > 0) {
		const char *name = gnutls_sign_get_name(ret);

		*algo = ret;

		if (name != NULL)
			return gnutls_strdup(name);
	}

	ret = gnutls_x509_crt_get_signature_oid(cert, oid, &oid_size);
	if (ret < 0)
		return NULL;

	return gnutls_strdup(oid);
}

static char *crq_get_sign_name(gnutls_x509_crq_t crq)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	oid_size = sizeof(oid);
	int ret;

	ret = gnutls_x509_crq_get_signature_algorithm(crq);
	if (ret > 0) {
		const char *name = gnutls_sign_get_name(ret);

		if (name != NULL)
			return gnutls_strdup(name);
	}

	ret = gnutls_x509_crq_get_signature_oid(crq, oid, &oid_size);
	if (ret < 0)
		return NULL;

	return gnutls_strdup(oid);
}

static char *crl_get_sign_name(gnutls_x509_crl_t crl, int *algo)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	oid_size = sizeof(oid);
	int ret;

	*algo = 0;

	ret = gnutls_x509_crl_get_signature_algorithm(crl);
	if (ret > 0) {
		const char *name = gnutls_sign_get_name(ret);

		*algo = ret;

		if (name != NULL)
			return gnutls_strdup(name);
	}

	ret = gnutls_x509_crl_get_signature_oid(crl, oid, &oid_size);
	if (ret < 0)
		return NULL;

	return gnutls_strdup(oid);
}


static void print_proxy(gnutls_buffer_st * str, gnutls_datum_t *der)
{
	int pathlen;
	char *policyLanguage;
	char *policy;
	size_t npolicy;
	int err;

	err = gnutls_x509_ext_import_proxy(der, &pathlen, &policyLanguage,
					&policy, &npolicy);
	if (err < 0) {
		addf(str, "error: get_proxy: %s\n", gnutls_strerror(err));
		return;
	}

	if (pathlen >= 0)
		addf(str, _("\t\t\tPath Length Constraint: %d\n"),
		     pathlen);
	addf(str, _("\t\t\tPolicy Language: %s"), policyLanguage);
	if (strcmp(policyLanguage, "1.3.6.1.5.5.7.21.1") == 0)
		adds(str, " (id-ppl-inheritALL)\n");
	else if (strcmp(policyLanguage, "1.3.6.1.5.5.7.21.2") == 0)
		adds(str, " (id-ppl-independent)\n");
	else
		adds(str, "\n");
	if (npolicy) {
		adds(str, _("\t\t\tPolicy:\n\t\t\t\tASCII: "));
		_gnutls_buffer_asciiprint(str, policy, npolicy);
		adds(str, _("\n\t\t\t\tHexdump: "));
		_gnutls_buffer_hexprint(str, policy, npolicy);
		adds(str, "\n");
	}
	gnutls_free(policy);
	gnutls_free(policyLanguage);
}


static void print_nc(gnutls_buffer_st * str, const char* prefix, gnutls_datum_t *der)
{
	gnutls_x509_name_constraints_t nc;
	int ret;
	unsigned idx = 0;
	gnutls_datum_t name;
	unsigned type;
	char new_prefix[16];

	ret = gnutls_x509_name_constraints_init(&nc);
	if (ret < 0)
		return;

	ret = gnutls_x509_ext_import_name_constraints(der, nc, 0);
	if (ret < 0)
		goto cleanup;

	snprintf(new_prefix, sizeof(new_prefix), "%s\t\t\t\t", prefix);

	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, idx++, &type, &name);

		if (ret >= 0) {
			if (idx == 1)
				addf(str,  _("%s\t\t\tPermitted:\n"), prefix);

			print_name(str, new_prefix, type, &name, 1);
		}
	} while (ret == 0);

	idx = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, idx++, &type, &name);

		if (ret >= 0) {
			if (idx == 1)
				addf(str,  _("%s\t\t\tExcluded:\n"), prefix);

			print_name(str, new_prefix, type, &name, 1);
		}
	} while (ret == 0);

cleanup:
	gnutls_x509_name_constraints_deinit(nc);
}

static void print_aia(gnutls_buffer_st * str, const gnutls_datum_t *der)
{
	int err;
	int seq;
	gnutls_datum_t san = { NULL, 0 }, oid = {NULL, 0};
	gnutls_x509_aia_t aia;
	unsigned int san_type;
	
	err = gnutls_x509_aia_init(&aia);
	if (err < 0)
		return;

	err = gnutls_x509_ext_import_aia(der, aia, 0);
	if (err < 0) {
		addf(str, "error: get_aia: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}

	for (seq=0;;seq++) {
		err = gnutls_x509_aia_get(aia, seq, &oid, &san_type, &san);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			goto cleanup;
		if (err < 0) {
			addf(str, "error: aia_get: %s\n",
			     gnutls_strerror(err));
			goto cleanup;
		}

		if (strcmp((char*)oid.data, GNUTLS_OID_AD_OCSP) == 0)
			addf(str, _("\t\t\tAccess Method: %s (%s)\n"), GNUTLS_OID_AD_OCSP, "id-ad-ocsp");
		else if (strcmp((char*)oid.data, GNUTLS_OID_AD_CAISSUERS) == 0)
			addf(str, _("\t\t\tAccess Method: %s (%s)\n"), GNUTLS_OID_AD_CAISSUERS, "id-ad-caIssuers");
		else {
			addf(str, _("\t\t\tAccess Method: %s (%s)\n"), (char*)oid.data, "UNKNOWN");
		}

		adds(str, "\t\t\tAccess Location ");
		print_name(str, "", san_type, &san, 0);
	}

cleanup:
	gnutls_x509_aia_deinit(aia);
}

static void print_ski(gnutls_buffer_st * str, gnutls_datum_t *der)
{
	gnutls_datum_t id = {NULL, 0};
	int err;

	err = gnutls_x509_ext_import_subject_key_id(der, &id);
	if (err < 0) {
		addf(str, "error: get_subject_key_id: %s\n",
		     gnutls_strerror(err));
		return;
	}

	adds(str, "\t\t\t");
	_gnutls_buffer_hexprint(str, id.data, id.size);
	adds(str, "\n");

	gnutls_free(id.data);
}

#define TYPE_CRT 2
#define TYPE_CRQ 3

typedef union {
	gnutls_x509_crt_t crt;
	gnutls_x509_crq_t crq;
} cert_type_t;

static void
print_aki_gn_serial(gnutls_buffer_st * str, gnutls_x509_aki_t aki)
{
	gnutls_datum_t san, other_oid, serial;
	unsigned int alt_type;
	int err;

	err =
	    gnutls_x509_aki_get_cert_issuer(aki,
					    0, &alt_type, &san, &other_oid, &serial);
	if (err < 0) {
		addf(str, "error: gnutls_x509_aki_get_cert_issuer: %s\n",
		     gnutls_strerror(err));
		return;
	}

	print_name(str, "\t\t\t", alt_type, &san, 0);

	adds(str, "\t\t\tserial: ");
	_gnutls_buffer_hexprint(str, serial.data, serial.size);
	adds(str, "\n");
}

static void print_aki(gnutls_buffer_st * str, gnutls_datum_t *der)
{
	int err;
	gnutls_x509_aki_t aki;
	gnutls_datum_t id;

	err = gnutls_x509_aki_init(&aki);
	if (err < 0) {
		addf(str, "error: gnutls_x509_aki_init: %s\n",
		     gnutls_strerror(err));
		return;
	}

	err = gnutls_x509_ext_import_authority_key_id(der, aki, 0);
	if (err < 0) {
		addf(str, "error: gnutls_x509_ext_import_authority_key_id: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}

	err = gnutls_x509_aki_get_id(aki, &id);
	if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		/* Check if an alternative name is there */
		print_aki_gn_serial(str, aki);
		goto cleanup;
	} else if (err < 0) {
		addf(str, "error: gnutls_x509_aki_get_id: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}
	
	adds(str, "\t\t\t");
	_gnutls_buffer_hexprint(str, id.data, id.size);
	adds(str, "\n");

 cleanup:
	gnutls_x509_aki_deinit(aki);
}

static void
print_key_usage2(gnutls_buffer_st * str, const char *prefix, unsigned int key_usage)
{
	if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE)
		addf(str, _("%sDigital signature.\n"), prefix);
	if (key_usage & GNUTLS_KEY_NON_REPUDIATION)
		addf(str, _("%sNon repudiation.\n"), prefix);
	if (key_usage & GNUTLS_KEY_KEY_ENCIPHERMENT)
		addf(str, _("%sKey encipherment.\n"), prefix);
	if (key_usage & GNUTLS_KEY_DATA_ENCIPHERMENT)
		addf(str, _("%sData encipherment.\n"), prefix);
	if (key_usage & GNUTLS_KEY_KEY_AGREEMENT)
		addf(str, _("%sKey agreement.\n"), prefix);
	if (key_usage & GNUTLS_KEY_KEY_CERT_SIGN)
		addf(str, _("%sCertificate signing.\n"), prefix);
	if (key_usage & GNUTLS_KEY_CRL_SIGN)
		addf(str, _("%sCRL signing.\n"), prefix);
	if (key_usage & GNUTLS_KEY_ENCIPHER_ONLY)
		addf(str, _("%sKey encipher only.\n"), prefix);
	if (key_usage & GNUTLS_KEY_DECIPHER_ONLY)
		addf(str, _("%sKey decipher only.\n"), prefix);
}

static void
print_key_usage(gnutls_buffer_st * str, const char *prefix, gnutls_datum_t *der)
{
	unsigned int key_usage;
	int err;

	err = gnutls_x509_ext_import_key_usage(der, &key_usage);
	if (err < 0) {
		addf(str, "error: get_key_usage: %s\n",
		     gnutls_strerror(err));
		return;
	}

	print_key_usage2(str, prefix, key_usage);
}

static void
print_private_key_usage_period(gnutls_buffer_st * str, const char *prefix, gnutls_datum_t *der)
{
	time_t activation, expiration;
	int err;
	char s[42];
	struct tm t;
	size_t max;

	err = gnutls_x509_ext_import_private_key_usage_period(der, &activation, &expiration);
	if (err < 0) {
		addf(str, "error: get_private_key_usage_period: %s\n",
		     gnutls_strerror(err));
		return;
	}

	max = sizeof(s);

	if (gmtime_r(&activation, &t) == NULL)
		addf(str, "error: gmtime_r (%ld)\n",
		     (unsigned long) activation);
	else if (strftime(s, max, "%a %b %d %H:%M:%S UTC %Y", &t) == 0)
		addf(str, "error: strftime (%ld)\n",
		     (unsigned long) activation);
	else
		addf(str, _("\t\t\tNot Before: %s\n"), s);

	if (gmtime_r(&expiration, &t) == NULL)
		addf(str, "error: gmtime_r (%ld)\n",
		     (unsigned long) expiration);
	else if (strftime(s, max, "%a %b %d %H:%M:%S UTC %Y", &t) == 0)
		addf(str, "error: strftime (%ld)\n",
		     (unsigned long) expiration);
	else
		addf(str, _("\t\t\tNot After: %s\n"), s);

}

static void print_crldist(gnutls_buffer_st * str, gnutls_datum_t *der)
{
	int err;
	int indx;
	gnutls_x509_crl_dist_points_t dp;
	unsigned int flags, type;
	gnutls_datum_t dist;

	err = gnutls_x509_crl_dist_points_init(&dp);
	if (err < 0) {
		addf(str, "error: gnutls_x509_crl_dist_points_init: %s\n",
		     gnutls_strerror(err));
		return;
	}

	err = gnutls_x509_ext_import_crl_dist_points(der, dp, 0);
	if (err < 0) {
		addf(str, "error: gnutls_x509_ext_import_crl_dist_points: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}

	for (indx = 0;; indx++) {
		err =
		    gnutls_x509_crl_dist_points_get(dp, indx, &type, &dist, &flags);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			goto cleanup;
		else if (err < 0) {
			addf(str, "error: get_crl_dist_points: %s\n",
			     gnutls_strerror(err));
			return;
		}

		print_name(str, "\t\t\t", type, &dist, 0);
	}
 cleanup:
	gnutls_x509_crl_dist_points_deinit(dp);
}

static void
print_key_purpose(gnutls_buffer_st * str, const char *prefix, gnutls_datum_t *der)
{
	int indx;
	gnutls_datum_t oid;
	char *p;
	int err;
	gnutls_x509_key_purposes_t purposes;
	
	err = gnutls_x509_key_purpose_init(&purposes);
	if (err < 0) {
		addf(str, "error: gnutls_x509_key_purpose_init: %s\n",
		     gnutls_strerror(err));
		return;
	}

	err = gnutls_x509_ext_import_key_purposes(der, purposes, 0);
	if (err < 0) {
		addf(str, "error: gnutls_x509_ext_import_key_purposes: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}

	for (indx = 0;; indx++) {
		err = gnutls_x509_key_purpose_get(purposes, indx, &oid);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			goto cleanup;
		else if (err < 0) {
			addf(str, "error: gnutls_x509_key_purpose_get: %s\n",
			     gnutls_strerror(err));
			goto cleanup;
		}

		p = (void*)oid.data;
		if (strcmp(p, GNUTLS_KP_TLS_WWW_SERVER) == 0)
			addf(str, _("%s\t\t\tTLS WWW Server.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_TLS_WWW_CLIENT) == 0)
			addf(str, _("%s\t\t\tTLS WWW Client.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_CODE_SIGNING) == 0)
			addf(str, _("%s\t\t\tCode signing.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_EMAIL_PROTECTION) == 0)
			addf(str, _("%s\t\t\tEmail protection.\n"),
			     prefix);
		else if (strcmp(p, GNUTLS_KP_TIME_STAMPING) == 0)
			addf(str, _("%s\t\t\tTime stamping.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_OCSP_SIGNING) == 0)
			addf(str, _("%s\t\t\tOCSP signing.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_IPSEC_IKE) == 0)
			addf(str, _("%s\t\t\tIpsec IKE.\n"), prefix);
		else if (strcmp(p, GNUTLS_KP_ANY) == 0)
			addf(str, _("%s\t\t\tAny purpose.\n"), prefix);
		else
			addf(str, "%s\t\t\t%s\n", prefix, p);
	}
 cleanup:
	gnutls_x509_key_purpose_deinit(purposes);
}

static void
print_basic(gnutls_buffer_st * str, const char *prefix, gnutls_datum_t *der)
{
	int pathlen;
	unsigned ca;
	int err;

	err = gnutls_x509_ext_import_basic_constraints(der, &ca, &pathlen);
	if (err < 0) {
		addf(str, "error: get_basic_constraints: %s\n",
		     gnutls_strerror(err));
		return;
	}

	if (ca == 0)
		addf(str, _("%s\t\t\tCertificate Authority (CA): FALSE\n"),
		     prefix);
	else
		addf(str, _("%s\t\t\tCertificate Authority (CA): TRUE\n"),
		     prefix);

	if (pathlen >= 0)
		addf(str, _("%s\t\t\tPath Length Constraint: %d\n"),
		     prefix, pathlen);
}


static void
print_altname(gnutls_buffer_st * str, const char *prefix, gnutls_datum_t *der)
{
	unsigned int altname_idx;
	gnutls_subject_alt_names_t names;
	unsigned int type;
	gnutls_datum_t san;
	gnutls_datum_t othername;
	char pfx[16];
	int err;

	err = gnutls_subject_alt_names_init(&names);
	if (err < 0) {
		addf(str, "error: gnutls_subject_alt_names_init: %s\n",
		     gnutls_strerror(err));
		return;
	}

	err = gnutls_x509_ext_import_subject_alt_names(der, names, 0);
	if (err < 0) {
		addf(str, "error: gnutls_x509_ext_import_subject_alt_names: %s\n",
		     gnutls_strerror(err));
		goto cleanup;
	}

	for (altname_idx = 0;; altname_idx++) {
		err = gnutls_subject_alt_names_get(names, altname_idx,
						   &type, &san, &othername);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		else if (err < 0) {
			addf(str,
			     "error: gnutls_subject_alt_names_get: %s\n",
			     gnutls_strerror(err));
			break;
		}


		if (type == GNUTLS_SAN_OTHERNAME) {
			unsigned vtype;
			gnutls_datum_t virt;

			err = gnutls_x509_othername_to_virtual((char*)othername.data, &san, &vtype, &virt);
			if (err >= 0) {
				snprintf(pfx, sizeof(pfx), "%s\t\t\t", prefix);
				print_name(str, pfx, vtype, &virt, 0);
				gnutls_free(virt.data);
				continue;
			}

			addf(str,
			     _("%s\t\t\totherName OID: %.*s\n"),
			     prefix, (int)othername.size, (char*)othername.data);
			addf(str, _("%s\t\t\totherName DER: "),
				     prefix);
			_gnutls_buffer_hexprint(str, san.data, san.size);
			addf(str, _("\n%s\t\t\totherName ASCII: "),
				     prefix);
			_gnutls_buffer_asciiprint(str, (char*)san.data, san.size);
				addf(str, "\n");
		} else {

			snprintf(pfx, sizeof(pfx), "%s\t\t\t", prefix);
			print_name(str, pfx, type, &san, 0);
		}
	}

 cleanup:
	gnutls_subject_alt_names_deinit(names);
}

static void
guiddump(gnutls_buffer_st * str, const char *data, size_t len,
	 const char *spc)
{
	size_t j;

	if (spc)
		adds(str, spc);
	addf(str, "{");
	addf(str, "%.2X", (unsigned char) data[3]);
	addf(str, "%.2X", (unsigned char) data[2]);
	addf(str, "%.2X", (unsigned char) data[1]);
	addf(str, "%.2X", (unsigned char) data[0]);
	addf(str, "-");
	addf(str, "%.2X", (unsigned char) data[5]);
	addf(str, "%.2X", (unsigned char) data[4]);
	addf(str, "-");
	addf(str, "%.2X", (unsigned char) data[7]);
	addf(str, "%.2X", (unsigned char) data[6]);
	addf(str, "-");
	addf(str, "%.2X", (unsigned char) data[8]);
	addf(str, "%.2X", (unsigned char) data[9]);
	addf(str, "-");
	for (j = 10; j < 16; j++) {
		addf(str, "%.2X", (unsigned char) data[j]);
	}
	addf(str, "}\n");
}

static void
print_unique_ids(gnutls_buffer_st * str, const gnutls_x509_crt_t cert)
{
	int result;
	char buf[256];		/* if its longer, we won't bother to print it */
	size_t buf_size = 256;

	result =
	    gnutls_x509_crt_get_issuer_unique_id(cert, buf, &buf_size);
	if (result >= 0) {
		addf(str, ("\tIssuer Unique ID:\n"));
		_gnutls_buffer_hexdump(str, buf, buf_size, "\t\t\t");
		if (buf_size == 16) {	/* this could be a GUID */
			guiddump(str, buf, buf_size, "\t\t\t");
		}
	}

	buf_size = 256;
	result =
	    gnutls_x509_crt_get_subject_unique_id(cert, buf, &buf_size);
	if (result >= 0) {
		addf(str, ("\tSubject Unique ID:\n"));
		_gnutls_buffer_hexdump(str, buf, buf_size, "\t\t\t");
		if (buf_size == 16) {	/* this could be a GUID */
			guiddump(str, buf, buf_size, "\t\t\t");
		}
	}
}

static void print_tlsfeatures(gnutls_buffer_st * str, const char *prefix, const gnutls_datum_t *der)
{
	int err;
	int seq;
	gnutls_x509_tlsfeatures_t features;
	const char *name;
	unsigned int feature;

	err = gnutls_x509_tlsfeatures_init(&features);
	if (err < 0)
		return;

	err = gnutls_x509_ext_import_tlsfeatures(der, features, 0);
	if (err < 0) {
		addf(str, "error: get_tlsfeatures: %s\n",
			 gnutls_strerror(err));
		goto cleanup;
	}

	for (seq=0;;seq++) {
		err = gnutls_x509_tlsfeatures_get(features, seq, &feature);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			goto cleanup;
		if (err < 0) {
			addf(str, "error: get_tlsfeatures: %s\n",
				 gnutls_strerror(err));
			goto cleanup;
		}

		name = gnutls_ext_get_name(feature);
		if (name == NULL)
			addf(str, "%s\t\t\t%u\n", prefix, feature);
		else
			addf(str, "%s\t\t\t%s(%u)\n", prefix, name, feature);
	}

cleanup:
	gnutls_x509_tlsfeatures_deinit(features);
}

struct ext_indexes_st {
	int san;
	int ian;
	int proxy;
	int basic;
	int keyusage;
	int keypurpose;
	int ski;
	int aki, nc;
	int crldist, pkey_usage_period;
	int tlsfeatures;
};

static void print_extension(gnutls_buffer_st * str, const char *prefix,
			    struct ext_indexes_st *idx, const char *oid,
			    unsigned critical, gnutls_datum_t *der)
{
	int err;
	unsigned j;
	char pfx[16];

	if (strcmp(oid, "2.5.29.19") == 0) {
		if (idx->basic) {
			addf(str,
			     "warning: more than one basic constraint\n");
		}

		addf(str, _("%s\t\tBasic Constraints (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_basic(str, prefix, der);
		idx->basic++;

	} else if (strcmp(oid, "2.5.29.14") == 0) {
		if (idx->ski) {
			addf(str,
			     "warning: more than one SKI extension\n");
		}

		addf(str,
		     _("%s\t\tSubject Key Identifier (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_ski(str, der);

		idx->ski++;
	} else if (strcmp(oid, "2.5.29.32") == 0) {
		struct gnutls_x509_policy_st policy;
		gnutls_x509_policies_t policies;
		const char *name;
		int x;

		err = gnutls_x509_policies_init(&policies);
		if (err < 0) {
			addf(str,
			     "error: certificate policies: %s\n",
			     gnutls_strerror(err));
			return;
		}

		err = gnutls_x509_ext_import_policies(der, policies, 0);
		if (err < 0) {
			addf(str,
			     "error: certificate policies import: %s\n",
			     gnutls_strerror(err));
			gnutls_x509_policies_deinit(policies);
			return;
		}

		for (x = 0;; x++) {
			err = gnutls_x509_policies_get(policies, x, &policy);
			if (err ==
			    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;

			if (err < 0) {
				addf(str,
				     "error: certificate policy: %s\n",
				     gnutls_strerror(err));
				break;
			}

			if (x == 0)
				addf(str,
				     "%s\t\tCertificate Policies (%s):\n",
				     prefix,
				     critical ? _("critical") :
				     _("not critical"));

			addf(str, "%s\t\t\t%s\n", prefix, policy.oid);
			for (j = 0; j < policy.qualifiers; j++) {
				if (policy.qualifier[j].type ==
				    GNUTLS_X509_QUALIFIER_URI)
					name = "URI";
				else if (policy.qualifier[j].
					 type ==
					 GNUTLS_X509_QUALIFIER_NOTICE)
					name = "Note";
				else
					name = "Unknown qualifier";
				addf(str, "%s\t\t\t\t%s: %s\n",
				     prefix, name,
				     policy.qualifier[j].data);
			}
		}
		gnutls_x509_policies_deinit(policies);
	} else if (strcmp(oid, "2.5.29.54") == 0) {
		unsigned int skipcerts;

		err = gnutls_x509_ext_import_inhibit_anypolicy(der, &skipcerts);
		if (err < 0) {
			addf(str,
			     "error: certificate inhibit any policy import: %s\n",
			     gnutls_strerror(err));
			return;
		}

		addf(str,
		     "%s\t\tInhibit anyPolicy skip certs: %u (%s)\n",
			     prefix, skipcerts,
			     critical ? _("critical") :
			     _("not critical"));

	} else if (strcmp(oid, "2.5.29.35") == 0) {

		if (idx->aki) {
			addf(str,
			     "warning: more than one AKI extension\n");
		}

		addf(str,
		     _("%s\t\tAuthority Key Identifier (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_aki(str, der);

		idx->aki++;
	} else if (strcmp(oid, "2.5.29.15") == 0) {
		if (idx->keyusage) {
			addf(str,
			     "warning: more than one key usage extension\n");
		}

		addf(str, _("%s\t\tKey Usage (%s):\n"), prefix,
			     critical ? _("critical") : _("not critical"));

		snprintf(pfx, sizeof(pfx), "%s\t\t\t", prefix);
		print_key_usage(str, pfx, der);

		idx->keyusage++;
	} else if (strcmp(oid, "2.5.29.16") == 0) {
		if (idx->pkey_usage_period) {
			addf(str,
			     "warning: more than one private key usage period extension\n");
		}

		addf(str,
		     _("%s\t\tPrivate Key Usage Period (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_private_key_usage_period(str, prefix, der);

		idx->pkey_usage_period++;
	} else if (strcmp(oid, "2.5.29.37") == 0) {
		if (idx->keypurpose) {
			addf(str,
			     "warning: more than one key purpose extension\n");
		}

		addf(str, _("%s\t\tKey Purpose (%s):\n"), prefix,
		     critical ? _("critical") : _("not critical"));

		print_key_purpose(str, prefix, der);
		idx->keypurpose++;
	} else if (strcmp(oid, "2.5.29.17") == 0) {
		if (idx->san) {
			addf(str,
			     "warning: more than one SKI extension\n");
		}

		addf(str,
		     _("%s\t\tSubject Alternative Name (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));
			print_altname(str, prefix, der);
		idx->san++;
	} else if (strcmp(oid, "2.5.29.18") == 0) {
		if (idx->ian) {
			addf(str,
			     "warning: more than one Issuer AltName extension\n");
		}

		addf(str,
		     _("%s\t\tIssuer Alternative Name (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_altname(str, prefix, der);

		idx->ian++;
	} else if (strcmp(oid, "2.5.29.31") == 0) {
		if (idx->crldist) {
			addf(str,
			     "warning: more than one CRL distribution point\n");
		}

		addf(str,
		     _("%s\t\tCRL Distribution points (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_crldist(str, der);
		idx->crldist++;
	} else if (strcmp(oid, "1.3.6.1.5.5.7.1.14") == 0) {
		if (idx->proxy) {
			addf(str,
			     "warning: more than one proxy extension\n");
		}

		addf(str,
			     _
		     ("%s\t\tProxy Certificate Information (%s):\n"),
		     prefix,
		     critical ? _("critical") : _("not critical"));

		print_proxy(str, der);

		idx->proxy++;
	} else if (strcmp(oid, "1.3.6.1.5.5.7.1.1") == 0) {
		addf(str, _("%s\t\tAuthority Information "
			    "Access (%s):\n"), prefix,
		     critical ? _("critical") : _("not critical"));

		print_aia(str, der);
	} else if (strcmp(oid, "2.5.29.30") == 0) {
		if (idx->nc) {
			addf(str,
			     "warning: more than one name constraints extension\n");
		}
		idx->nc++;

		addf(str, _("%s\t\tName Constraints (%s):\n"), prefix,
		     critical ? _("critical") : _("not critical"));

		print_nc(str, prefix, der);
	} else if (strcmp(oid, GNUTLS_X509EXT_OID_TLSFEATURES) == 0) {
		if (idx->tlsfeatures) {
			addf(str,
				 "warning: more than one tlsfeatures extension\n");
		}

		addf(str, _("%s\t\tTLS Features (%s):\n"),
			 prefix,
			 critical ? _("critical") : _("not critical"));

		print_tlsfeatures(str, prefix, der);

		idx->tlsfeatures++;
	} else {
		addf(str, _("%s\t\tUnknown extension %s (%s):\n"),
		     prefix, oid,
		     critical ? _("critical") : _("not critical"));

		addf(str, _("%s\t\t\tASCII: "), prefix);
		_gnutls_buffer_asciiprint(str, (char*)der->data, der->size);

		addf(str, "\n");
		addf(str, _("%s\t\t\tHexdump: "), prefix);
		_gnutls_buffer_hexprint(str, (char*)der->data, der->size);
		adds(str, "\n");
	}
}

static void
print_extensions(gnutls_buffer_st * str, const char *prefix, int type,
		 cert_type_t cert)
{
	unsigned i;
	int err;
	gnutls_datum_t der = {NULL, 0};
	struct ext_indexes_st idx;

	memset(&idx, 0, sizeof(idx));

	for (i = 0;; i++) {
		char oid[MAX_OID_SIZE] = "";
		size_t sizeof_oid = sizeof(oid);
		unsigned int critical;

		if (type == TYPE_CRT)
			err =
			    gnutls_x509_crt_get_extension_info(cert.crt, i,
							       oid,
							       &sizeof_oid,
							       &critical);

		else if (type == TYPE_CRQ)
			err =
			    gnutls_x509_crq_get_extension_info(cert.crq, i,
							       oid,
							       &sizeof_oid,
							       &critical);
		else {
			gnutls_assert();
			return;
		}

		if (err < 0) {
			if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			addf(str, "error: get_extension_info: %s\n",
			     gnutls_strerror(err));
			continue;
		}

		if (i == 0)
			addf(str, _("%s\tExtensions:\n"), prefix);

		if (type == TYPE_CRT)
			err = gnutls_x509_crt_get_extension_data2(cert.crt, i, &der);
		else
			err = gnutls_x509_crq_get_extension_data2(cert.crq, i, &der);

		if (err < 0) {
			der.data = NULL;
			der.size = 0;
		}

		print_extension(str, prefix, &idx, oid, critical, &der);
		gnutls_free(der.data);
	}
}

static void reverse_datum(gnutls_datum_t *d)
{
	unsigned int i;
	unsigned char c;

	for (i = 0; i < d->size / 2; i++) {
		c = d->data[i];
		d->data[i] = d->data[d->size - i - 1];
		d->data[d->size - i - 1] = c;
	}
}

static void
print_pubkey(gnutls_buffer_st * str, const char *key_name,
	     gnutls_pubkey_t pubkey, gnutls_x509_spki_st *spki,
	     gnutls_certificate_print_formats_t format)
{
	int err;
	const char *name;
	unsigned bits;
	unsigned pk;

	err = gnutls_pubkey_get_pk_algorithm(pubkey, &bits);
	if (err < 0) {
		addf(str, "error: get_pk_algorithm: %s\n",
		     gnutls_strerror(err));
		return;
	}

	pk = err;

	name = gnutls_pk_algorithm_get_name(pk);
	if (name == NULL)
		name = _("unknown");

	addf(str, _("\t%sPublic Key Algorithm: %s\n"), key_name, name);

	addf(str, _("\tAlgorithm Security Level: %s (%d bits)\n"),
	     gnutls_sec_param_get_name(gnutls_pk_bits_to_sec_param
				       (err, bits)), bits);

	if (spki && pk == GNUTLS_PK_RSA_PSS && spki->pk == pk) {
		addf(str, _("\t\tParameters:\n"));
		addf(str, "\t\t\tHash Algorithm: %s\n",
		     gnutls_digest_get_name(spki->rsa_pss_dig));
		addf(str, "\t\t\tSalt Length: %d\n", spki->salt_size);
	}

	switch (pk) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		{
			gnutls_datum_t m, e;

			err = gnutls_pubkey_get_pk_rsa_raw(pubkey, &m, &e);
			if (err < 0)
				addf(str, "error: get_pk_rsa_raw: %s\n",
				     gnutls_strerror(err));
			else {
				if (format ==
				    GNUTLS_CRT_PRINT_FULL_NUMBERS) {
					addf(str,
					     _("\t\tModulus (bits %d): "),
					     bits);
					_gnutls_buffer_hexprint(str,
								m.data,
								m.size);
					adds(str, "\n");
					addf(str,
					     _("\t\tExponent (bits %d): "),
					     e.size * 8);
					_gnutls_buffer_hexprint(str,
								e.data,
								e.size);
					adds(str, "\n");
				} else {
					addf(str,
					     _("\t\tModulus (bits %d):\n"),
					     bits);
					_gnutls_buffer_hexdump(str, m.data,
							       m.size,
							       "\t\t\t");
					addf(str,
					     _
					     ("\t\tExponent (bits %d):\n"),
					     e.size * 8);
					_gnutls_buffer_hexdump(str, e.data,
							       e.size,
							       "\t\t\t");
				}

				gnutls_free(m.data);
				gnutls_free(e.data);
			}

		}
		break;

	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_ECDSA:
		{
			gnutls_datum_t x, y;
			gnutls_ecc_curve_t curve;

			err =
			    gnutls_pubkey_get_pk_ecc_raw(pubkey, &curve,
							 &x, &y);
			if (err < 0) {
				addf(str, "error: get_pk_ecc_raw: %s\n",
				     gnutls_strerror(err));
			} else {
				addf(str, _("\t\tCurve:\t%s\n"),
				     gnutls_ecc_curve_get_name(curve));
				if (format ==
				    GNUTLS_CRT_PRINT_FULL_NUMBERS) {
					adds(str, _("\t\tX: "));
					_gnutls_buffer_hexprint(str,
								x.data,
								x.size);
					adds(str, "\n");
					if (y.size > 0) {
						adds(str, _("\t\tY: "));
						_gnutls_buffer_hexprint(str,
									y.data,
									y.size);
						adds(str, "\n");
					}
				} else {
					adds(str, _("\t\tX:\n"));
					_gnutls_buffer_hexdump(str, x.data,
							       x.size,
							       "\t\t\t");
					if (y.size > 0) {
						adds(str, _("\t\tY:\n"));
						_gnutls_buffer_hexdump(str, y.data,
								       y.size,
								       "\t\t\t");
					}
				}

				gnutls_free(x.data);
				gnutls_free(y.data);

			}
		}
		break;
	case GNUTLS_PK_DSA:
		{
			gnutls_datum_t p, q, g, y;

			err =
			    gnutls_pubkey_get_pk_dsa_raw(pubkey, &p, &q,
							 &g, &y);
			if (err < 0)
				addf(str, "error: get_pk_dsa_raw: %s\n",
				     gnutls_strerror(err));
			else {
				if (format ==
				    GNUTLS_CRT_PRINT_FULL_NUMBERS) {
					addf(str,
					     _
					     ("\t\tPublic key (bits %d): "),
					     bits);
					_gnutls_buffer_hexprint(str,
								y.data,
								y.size);
					adds(str, "\n");
					adds(str, _("\t\tP: "));
					_gnutls_buffer_hexprint(str,
								p.data,
								p.size);
					adds(str, "\n");
					adds(str, _("\t\tQ: "));
					_gnutls_buffer_hexprint(str,
								q.data,
								q.size);
					adds(str, "\n");
					adds(str, _("\t\tG: "));
					_gnutls_buffer_hexprint(str,
								g.data,
								g.size);
					adds(str, "\n");
				} else {
					addf(str,
					     _
					     ("\t\tPublic key (bits %d):\n"),
					     bits);
					_gnutls_buffer_hexdump(str, y.data,
							       y.size,
							       "\t\t\t");
					adds(str, _("\t\tP:\n"));
					_gnutls_buffer_hexdump(str, p.data,
							       p.size,
							       "\t\t\t");
					adds(str, _("\t\tQ:\n"));
					_gnutls_buffer_hexdump(str, q.data,
							       q.size,
							       "\t\t\t");
					adds(str, _("\t\tG:\n"));
					_gnutls_buffer_hexdump(str, g.data,
							       g.size,
							       "\t\t\t");
				}

				gnutls_free(p.data);
				gnutls_free(q.data);
				gnutls_free(g.data);
				gnutls_free(y.data);

			}
		}
		break;

	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		{
			gnutls_datum_t x, y;
			gnutls_ecc_curve_t curve;
			gnutls_digest_algorithm_t digest;
			gnutls_gost_paramset_t param;

			err =
			    gnutls_pubkey_export_gost_raw2(pubkey, &curve,
							   &digest,
							   &param,
							   &x, &y, 0);
			if (err < 0)
				addf(str, "error: get_pk_gost_raw: %s\n",
				     gnutls_strerror(err));
			else {
				addf(str, _("\t\tCurve:\t%s\n"),
				     gnutls_ecc_curve_get_name(curve));
				addf(str, _("\t\tDigest:\t%s\n"),
				     gnutls_digest_get_name(digest));
				addf(str, _("\t\tParamSet: %s\n"),
				     gnutls_gost_paramset_get_name(param));
				reverse_datum(&x);
				reverse_datum(&y);
				if (format ==
				    GNUTLS_CRT_PRINT_FULL_NUMBERS) {
					adds(str, _("\t\tX: "));
					_gnutls_buffer_hexprint(str,
								x.data,
								x.size);
					adds(str, "\n");
					adds(str, _("\t\tY: "));
					_gnutls_buffer_hexprint(str,
								y.data,
								y.size);
					adds(str, "\n");
				} else {
					adds(str, _("\t\tX:\n"));
					_gnutls_buffer_hexdump(str, x.data,
							       x.size,
							       "\t\t\t");
					adds(str, _("\t\tY:\n"));
					_gnutls_buffer_hexdump(str, y.data,
							       y.size,
							       "\t\t\t");
				}

				gnutls_free(x.data);
				gnutls_free(y.data);

			}
		}
		break;

	default:
		break;
	}
}

static int
print_crt_sig_params(gnutls_buffer_st * str, gnutls_x509_crt_t crt,
		     gnutls_certificate_print_formats_t format)
{
	int ret;
	gnutls_pk_algorithm_t pk;
	gnutls_x509_spki_st params;
	gnutls_sign_algorithm_t sign;

	sign = gnutls_x509_crt_get_signature_algorithm(crt);
	pk = gnutls_sign_get_pk_algorithm(sign);
	if (pk == GNUTLS_PK_RSA_PSS) {
		ret = _gnutls_x509_read_sign_params(crt->cert,
						    "signatureAlgorithm",
						    &params);
		if (ret < 0) {
			addf(str, "error: read_pss_params: %s\n",
			     gnutls_strerror(ret));
		} else
			addf(str, "\t\tSalt Length: %d\n", params.salt_size);
	}

	return 0;
}

static void print_pk_name(gnutls_buffer_st * str, gnutls_x509_crt_t crt)
{
	const char *p;
	char *name = get_pk_name(crt, NULL);
	if (name == NULL)
		p = _("unknown");
	else
		p = name;

	addf(str, "\tSubject Public Key Algorithm: %s\n", p);
	gnutls_free(name);
}

static int
print_crt_pubkey(gnutls_buffer_st * str, gnutls_x509_crt_t crt,
		 gnutls_certificate_print_formats_t format)
{
	gnutls_pubkey_t pubkey = NULL;
	gnutls_x509_spki_st params;
	int ret, pk;

	ret = _gnutls_x509_crt_read_spki_params(crt, &params);
	if (ret < 0)
		return ret;

	pk = gnutls_x509_crt_get_pk_algorithm(crt, NULL);
	if (pk < 0) {
		gnutls_assert();
		pk = GNUTLS_PK_UNKNOWN;
	}

	if (pk == GNUTLS_PK_UNKNOWN) {
		print_pk_name(str, crt); /* print basic info only */
		return 0;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return ret;

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {
		if (ret != GNUTLS_E_UNIMPLEMENTED_FEATURE)
			addf(str, "error importing public key: %s\n", gnutls_strerror(ret));
		print_pk_name(str, crt); /* print basic info only */
		ret = 0;
		goto cleanup;
	}

	print_pubkey(str, _("Subject "), pubkey, &params, format);
	ret = 0;

 cleanup:
	if (pubkey)
		gnutls_pubkey_deinit(pubkey);

	return ret;
}

static void
print_cert(gnutls_buffer_st * str, gnutls_x509_crt_t cert,
	   gnutls_certificate_print_formats_t format)
{
	/* Version. */
	{
		int version = gnutls_x509_crt_get_version(cert);
		if (version < 0)
			addf(str, "error: get_version: %s\n",
			     gnutls_strerror(version));
		else
			addf(str, _("\tVersion: %d\n"), version);
	}

	/* Serial. */
	{
		char serial[128];
		size_t serial_size = sizeof(serial);
		int err;

		err =
		    gnutls_x509_crt_get_serial(cert, serial, &serial_size);
		if (err < 0)
			addf(str, "error: get_serial: %s\n",
			     gnutls_strerror(err));
		else {
			adds(str, _("\tSerial Number (hex): "));
			_gnutls_buffer_hexprint(str, serial, serial_size);
			adds(str, "\n");
		}
	}

	/* Issuer. */
	if (format != GNUTLS_CRT_PRINT_UNSIGNED_FULL) {
		gnutls_datum_t dn;
		int err;

		err = gnutls_x509_crt_get_issuer_dn3(cert, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("\tIssuer:\n"));
		} else if (err < 0) {
			addf(str, "error: get_issuer_dn: %s\n",
			     gnutls_strerror(err));
		} else {
			addf(str, _("\tIssuer: %s\n"), dn.data);
			gnutls_free(dn.data);
		}
	}

	/* Validity. */
	{
		time_t tim;

		adds(str, _("\tValidity:\n"));

		tim = gnutls_x509_crt_get_activation_time(cert);
		if (tim != -1) {
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld)\n",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%a %b %d %H:%M:%S UTC %Y",
				  &t) == 0)
				addf(str, "error: strftime (%ld)\n",
				     (unsigned long) tim);
			else
				addf(str, _("\t\tNot Before: %s\n"), s);
		} else {
			addf(str, _("\t\tNot Before: %s\n"), _("unknown"));
		}

		tim = gnutls_x509_crt_get_expiration_time(cert);
		if (tim != -1) {
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld)\n",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%a %b %d %H:%M:%S UTC %Y",
				  &t) == 0)
				addf(str, "error: strftime (%ld)\n",
				     (unsigned long) tim);
			else
				addf(str, _("\t\tNot After: %s\n"), s);
		} else {
			addf(str, _("\t\tNot After: %s\n"), _("unknown"));
		}
	}

	/* Subject. */
	{
		gnutls_datum_t dn;
		int err;

		err = gnutls_x509_crt_get_dn3(cert, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("\tSubject:\n"));
		} else if (err < 0) {
			addf(str, "error: get_dn: %s\n",
			     gnutls_strerror(err));
		} else {
			addf(str, _("\tSubject: %s\n"), dn.data);
			gnutls_free(dn.data);
		}
	}

	/* SubjectPublicKeyInfo. */
	print_crt_pubkey(str, cert, format);

	print_unique_ids(str, cert);

	/* Extensions. */
	if (gnutls_x509_crt_get_version(cert) >= 3) {
		cert_type_t ccert;

		ccert.crt = cert;
		print_extensions(str, "", TYPE_CRT, ccert);
	}

	/* Signature. */
	if (format != GNUTLS_CRT_PRINT_UNSIGNED_FULL) {
		int err;
		size_t size = 0;
		char *buffer = NULL;
		char *name;
		const char *p;

		name = get_sign_name(cert, &err);
		if (name == NULL)
			p = _("unknown");
		else
			p = name;

		addf(str, _("\tSignature Algorithm: %s\n"), p);
		gnutls_free(name);

		print_crt_sig_params(str, cert, format);

		if (err != GNUTLS_SIGN_UNKNOWN && gnutls_sign_is_secure2(err, GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS) == 0) {
			adds(str,
			     _("warning: signed using a broken signature "
			       "algorithm that can be forged.\n"));
		}

		err = gnutls_x509_crt_get_signature(cert, buffer, &size);
		if (err != GNUTLS_E_SHORT_MEMORY_BUFFER) {
			addf(str, "error: get_signature: %s\n",
			     gnutls_strerror(err));
			return;
		}

		buffer = gnutls_malloc(size);
		if (!buffer) {
			addf(str, "error: malloc: %s\n",
			     gnutls_strerror(GNUTLS_E_MEMORY_ERROR));
			return;
		}

		err = gnutls_x509_crt_get_signature(cert, buffer, &size);
		if (err < 0) {
			gnutls_free(buffer);
			addf(str, "error: get_signature2: %s\n",
			     gnutls_strerror(err));
			return;
		}

		adds(str, _("\tSignature:\n"));
		_gnutls_buffer_hexdump(str, buffer, size, "\t\t");

		gnutls_free(buffer);
	}
}

static void
print_fingerprint(gnutls_buffer_st * str, gnutls_x509_crt_t cert)
{
	int err;
	char buffer[MAX_HASH_SIZE];
	size_t size = sizeof(buffer);

	adds(str, _("\tFingerprint:\n"));

	err = gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_SHA1, buffer, &size);
	if (err < 0) {
		addf(str, "error: get_fingerprint: %s\n",
		     gnutls_strerror(err));
		return;
	}

	adds(str, _("\t\tsha1:"));
	_gnutls_buffer_hexprint(str, buffer, size);
	adds(str, "\n");

	size = sizeof(buffer);
	err = gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_SHA256, buffer, &size);
	if (err < 0) {
		addf(str, "error: get_fingerprint: %s\n",
		     gnutls_strerror(err));
		return;
	}
	adds(str, _("\t\tsha256:"));
	_gnutls_buffer_hexprint(str, buffer, size);
	adds(str, "\n");
}

typedef int get_id_func(void *obj, unsigned, unsigned char*, size_t*);

static void print_obj_id(gnutls_buffer_st *str, const char *prefix, void *obj, get_id_func *get_id)
{
	unsigned char sha1_buffer[MAX_HASH_SIZE];
	unsigned char sha2_buffer[MAX_HASH_SIZE];
	int err;
	size_t sha1_size, sha2_size;

	sha1_size = sizeof(sha1_buffer);
	err = get_id(obj, GNUTLS_KEYID_USE_SHA1, sha1_buffer, &sha1_size);
	if (err == GNUTLS_E_UNIMPLEMENTED_FEATURE) /* unsupported algo */
		return;

	if (err < 0) {
		addf(str, "error: get_key_id(sha1): %s\n", gnutls_strerror(err));
		return;
	}

	sha2_size = sizeof(sha2_buffer);
	err = get_id(obj, GNUTLS_KEYID_USE_SHA256, sha2_buffer, &sha2_size);
	if (err == GNUTLS_E_UNIMPLEMENTED_FEATURE) /* unsupported algo */
		return;

	if (err < 0) {
		addf(str, "error: get_key_id(sha256): %s\n", gnutls_strerror(err));
		return;
	}

	addf(str, _("%sPublic Key ID:\n%s\tsha1:"), prefix, prefix);
	_gnutls_buffer_hexprint(str, sha1_buffer, sha1_size);
	addf(str, "\n%s\tsha256:", prefix);
	_gnutls_buffer_hexprint(str, sha2_buffer, sha2_size);
	adds(str, "\n");

	addf(str, _("%sPublic Key PIN:\n%s\tpin-sha256:"), prefix, prefix);
	_gnutls_buffer_base64print(str, sha2_buffer, sha2_size);
	adds(str, "\n");

	return;
}

static void print_keyid(gnutls_buffer_st * str, gnutls_x509_crt_t cert)
{
	int err;
	const char *name;
	unsigned int bits;
	unsigned char sha1_buffer[MAX_HASH_SIZE];
	size_t sha1_size;

	err = gnutls_x509_crt_get_pk_algorithm(cert, &bits);
	if (err < 0)
		return;

	print_obj_id(str, "\t", cert, (get_id_func*)gnutls_x509_crt_get_key_id);

	if (IS_EC(err)) {
		gnutls_ecc_curve_t curve;

		err = gnutls_x509_crt_get_pk_ecc_raw(cert, &curve, NULL, NULL);
		if (err < 0)
			return;

		name = gnutls_ecc_curve_get_name(curve);
		bits = 0;
	} else if (IS_GOSTEC(err)) {
		gnutls_ecc_curve_t curve;

		err = gnutls_x509_crt_get_pk_gost_raw(cert, &curve, NULL, NULL, NULL, NULL);
		if (err < 0)
			return;

		name = gnutls_ecc_curve_get_name(curve);
		bits = 0;
	} else {
		name = gnutls_pk_get_name(err);
	}

	if (name == NULL)
		return;

	sha1_size = sizeof(sha1_buffer);
	err = gnutls_x509_crt_get_key_id(cert, GNUTLS_KEYID_USE_SHA1, sha1_buffer, &sha1_size);
	if (err == GNUTLS_E_UNIMPLEMENTED_FEATURE) /* unsupported algo */
		return;
}

static void
print_other(gnutls_buffer_st * str, gnutls_x509_crt_t cert,
	    gnutls_certificate_print_formats_t format)
{
	if (format != GNUTLS_CRT_PRINT_UNSIGNED_FULL) {
		print_fingerprint(str, cert);
	}
	print_keyid(str, cert);
}

static void print_oneline(gnutls_buffer_st * str, gnutls_x509_crt_t cert)
{
	int err;

	/* Subject. */
	{
		gnutls_datum_t dn;

		err = gnutls_x509_crt_get_dn3(cert, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("no subject,"));
		} else if (err < 0) {
			addf(str, "unknown subject (%s), ",
			     gnutls_strerror(err));
		} else {
			addf(str, "subject `%s', ", dn.data);
			gnutls_free(dn.data);
		}
	}

	/* Issuer. */
	{
		gnutls_datum_t dn;

		err = gnutls_x509_crt_get_issuer_dn3(cert, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("no issuer,"));
		} else if (err < 0) {
			addf(str, "unknown issuer (%s), ",
			     gnutls_strerror(err));
		} else {
			addf(str, "issuer `%s', ", dn.data);
			gnutls_free(dn.data);
		}
	}

	{
		char serial[128];
		size_t serial_size = sizeof(serial);

		err =
		    gnutls_x509_crt_get_serial(cert, serial, &serial_size);
		if (err >= 0) {
			adds(str, "serial 0x");
			_gnutls_buffer_hexprint(str, serial, serial_size);
			adds(str, ", ");
		}
	}

	/* Key algorithm and size. */
	{
		unsigned int bits;
		const char *p;
		char *name = get_pk_name(cert, &bits);
		if (name == NULL)
			p = _("unknown");
		else
			p = name;
		addf(str, "%s key %d bits, ", p, bits);
		gnutls_free(name);
	}

	/* Signature Algorithm. */
	{
		char *name = get_sign_name(cert, &err);
		const char *p;

		if (name == NULL)
			p = _("unknown");
		else
			p = name;

		if (err != GNUTLS_SIGN_UNKNOWN && gnutls_sign_is_secure2(err, GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS) == 0)
			addf(str, _("signed using %s (broken!), "), p);
		else
			addf(str, _("signed using %s, "), p);
		gnutls_free(name);
	}

	/* Validity. */
	{
		time_t tim;

		tim = gnutls_x509_crt_get_activation_time(cert);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "unknown activation (%ld), ",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%Y-%m-%d %H:%M:%S UTC",
				  &t) == 0)
				addf(str, "failed activation (%ld), ",
				     (unsigned long) tim);
			else
				addf(str, "activated `%s', ", s);
		}

		tim = gnutls_x509_crt_get_expiration_time(cert);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "unknown expiry (%ld), ",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%Y-%m-%d %H:%M:%S UTC",
				  &t) == 0)
				addf(str, "failed expiry (%ld), ",
				     (unsigned long) tim);
			else
				addf(str, "expires `%s', ", s);
		}
	}

	{
		int pathlen;
		char *policyLanguage;

		err = gnutls_x509_crt_get_proxy(cert, NULL,
						&pathlen, &policyLanguage,
						NULL, NULL);
		if (err == 0) {
			addf(str, "proxy certificate (policy=");
			if (strcmp(policyLanguage, "1.3.6.1.5.5.7.21.1") ==
			    0)
				addf(str, "id-ppl-inheritALL");
			else if (strcmp
				 (policyLanguage,
				  "1.3.6.1.5.5.7.21.2") == 0)
				addf(str, "id-ppl-independent");
			else
				addf(str, "%s", policyLanguage);
			if (pathlen >= 0)
				addf(str, ", pathlen=%d), ", pathlen);
			else
				addf(str, "), ");
			gnutls_free(policyLanguage);
		}
	}

	{
		unsigned char buffer[MAX_HASH_SIZE];
		size_t size = sizeof(buffer);

		err = gnutls_x509_crt_get_key_id(cert, GNUTLS_KEYID_USE_SHA256,
						 buffer, &size);
		if (err >= 0) {
			addf(str, "pin-sha256=\"");
			_gnutls_buffer_base64print(str, buffer, size);
			adds(str, "\"");
		}
	}

}

/**
 * gnutls_x509_crt_print:
 * @cert: The data to be printed
 * @format: Indicate the format to use
 * @out: Newly allocated datum with null terminated string.
 *
 * This function will pretty print a X.509 certificate, suitable for
 * display to a human.
 *
 * If the format is %GNUTLS_CRT_PRINT_FULL then all fields of the
 * certificate will be output, on multiple lines.  The
 * %GNUTLS_CRT_PRINT_ONELINE format will generate one line with some
 * selected fields, which is useful for logging purposes.
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_print(gnutls_x509_crt_t cert,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out)
{
	gnutls_buffer_st str;
	int ret;

	if (format == GNUTLS_CRT_PRINT_COMPACT) {
		_gnutls_buffer_init(&str);

		print_oneline(&str, cert);

		ret = _gnutls_buffer_append_data(&str, "\n", 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		print_keyid(&str, cert);

		return _gnutls_buffer_to_datum(&str, out, 1);
	} else if (format == GNUTLS_CRT_PRINT_ONELINE) {
		_gnutls_buffer_init(&str);

		print_oneline(&str, cert);

		return _gnutls_buffer_to_datum(&str, out, 1);
	} else {
		_gnutls_buffer_init(&str);

		_gnutls_buffer_append_str(&str,
					  _
					  ("X.509 Certificate Information:\n"));

		print_cert(&str, cert, format);

		_gnutls_buffer_append_str(&str, _("Other Information:\n"));

		print_other(&str, cert, format);

		return _gnutls_buffer_to_datum(&str, out, 1);
	}
}

static void
print_crl(gnutls_buffer_st * str, gnutls_x509_crl_t crl, int notsigned)
{
	/* Version. */
	{
		int version = gnutls_x509_crl_get_version(crl);
		if (version == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
			adds(str, _("\tVersion: 1 (default)\n"));
		else if (version < 0)
			addf(str, "error: get_version: %s\n",
			     gnutls_strerror(version));
		else
			addf(str, _("\tVersion: %d\n"), version);
	}

	/* Issuer. */
	if (!notsigned) {
		gnutls_datum_t dn;
		int err;

		err = gnutls_x509_crl_get_issuer_dn3(crl, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("\tIssuer:\n"));
		} else if (err < 0) {
			addf(str, "error: get_issuer_dn: %s\n",
			     gnutls_strerror(err));
		} else {
			addf(str, _("\tIssuer: %s\n"), dn.data);
			gnutls_free(dn.data);
		}
	}

	/* Validity. */
	{
		time_t tim;

		adds(str, _("\tUpdate dates:\n"));

		tim = gnutls_x509_crl_get_this_update(crl);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld)\n",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%a %b %d %H:%M:%S UTC %Y",
				  &t) == 0)
				addf(str, "error: strftime (%ld)\n",
				     (unsigned long) tim);
			else
				addf(str, _("\t\tIssued: %s\n"), s);
		}

		tim = gnutls_x509_crl_get_next_update(crl);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (tim == -1)
				addf(str, "\t\tNo next update time.\n");
			else if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld)\n",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%a %b %d %H:%M:%S UTC %Y",
				  &t) == 0)
				addf(str, "error: strftime (%ld)\n",
				     (unsigned long) tim);
			else
				addf(str, _("\t\tNext at: %s\n"), s);
		}
	}

	/* Extensions. */
	if (gnutls_x509_crl_get_version(crl) >= 2) {
		size_t i;
		int err = 0;
		int aki_idx = 0;
		int crl_nr = 0;

		for (i = 0;; i++) {
			char oid[MAX_OID_SIZE] = "";
			size_t sizeof_oid = sizeof(oid);
			unsigned int critical;

			err = gnutls_x509_crl_get_extension_info(crl, i,
								 oid,
								 &sizeof_oid,
								 &critical);
			if (err < 0) {
				if (err ==
				    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
					break;
				addf(str,
				     "error: get_extension_info: %s\n",
				     gnutls_strerror(err));
				continue;
			}

			if (i == 0)
				adds(str, _("\tExtensions:\n"));

			if (strcmp(oid, "2.5.29.20") == 0) {
				char nr[128];
				size_t nr_size = sizeof(nr);

				if (crl_nr) {
					addf(str,
					     "warning: more than one CRL number\n");
				}

				err =
				    gnutls_x509_crl_get_number(crl, nr,
							       &nr_size,
							       &critical);

				addf(str, _("\t\tCRL Number (%s): "),
				     critical ? _("critical") :
				     _("not critical"));

				if (err < 0)
					addf(str,
					     "error: get_number: %s\n",
					     gnutls_strerror(err));
				else {
					_gnutls_buffer_hexprint(str, nr,
								nr_size);
					addf(str, "\n");
				}

				crl_nr++;
			} else if (strcmp(oid, "2.5.29.35") == 0) {
				gnutls_datum_t der;

				if (aki_idx) {
					addf(str,
					     "warning: more than one AKI extension\n");
				}

				addf(str,
				     _
				     ("\t\tAuthority Key Identifier (%s):\n"),
				     critical ? _("critical") :
				     _("not critical"));

				err = gnutls_x509_crl_get_extension_data2(crl, i, &der);
				if (err < 0) {
					addf(str,
					     "error: get_extension_data2: %s\n",
					     gnutls_strerror(err));
					continue;
				}
				print_aki(str, &der);
				gnutls_free(der.data);

				aki_idx++;
			} else {
				gnutls_datum_t der;

				addf(str,
				     _("\t\tUnknown extension %s (%s):\n"),
				     oid,
				     critical ? _("critical") :
				     _("not critical"));

				err =
				    gnutls_x509_crl_get_extension_data2(crl,
								       i,
								       &der);
				if (err < 0) {
					addf(str,
					     "error: get_extension_data2: %s\n",
					     gnutls_strerror(err));
					continue;
				}

				adds(str, _("\t\t\tASCII: "));
				_gnutls_buffer_asciiprint(str, (char*)der.data, der.size);
				adds(str, "\n");

				adds(str, _("\t\t\tHexdump: "));
				_gnutls_buffer_hexprint(str, der.data, der.size);
				adds(str, "\n");

				gnutls_free(der.data);
			}
		}
	}


	/* Revoked certificates. */
	{
		int num = gnutls_x509_crl_get_crt_count(crl);
		gnutls_x509_crl_iter_t iter = NULL;
		int j;

		if (num)
			addf(str, _("\tRevoked certificates (%d):\n"),
			     num);
		else
			adds(str, _("\tNo revoked certificates.\n"));

		for (j = 0; j < num; j++) {
			unsigned char serial[128];
			size_t serial_size = sizeof(serial);
			int err;
			time_t tim;

			err =
			    gnutls_x509_crl_iter_crt_serial(crl, &iter, serial,
							   &serial_size,
							   &tim);
			if (err < 0) {
				addf(str, "error: iter_crt_serial: %s\n",
				     gnutls_strerror(err));
				break;
			} else {
				char s[42];
				size_t max = sizeof(s);
				struct tm t;

				adds(str, _("\t\tSerial Number (hex): "));
				_gnutls_buffer_hexprint(str, serial,
							serial_size);
				adds(str, "\n");

				if (gmtime_r(&tim, &t) == NULL)
					addf(str,
					     "error: gmtime_r (%ld)\n",
					     (unsigned long) tim);
				else if (strftime
					 (s, max,
					  "%a %b %d %H:%M:%S UTC %Y",
					  &t) == 0)
					addf(str,
					     "error: strftime (%ld)\n",
					     (unsigned long) tim);
				else
					addf(str,
					     _("\t\tRevoked at: %s\n"), s);
			}
		}
		gnutls_x509_crl_iter_deinit(iter);
	}

	/* Signature. */
	if (!notsigned) {
		int err;
		size_t size = 0;
		char *buffer = NULL;
		char *name;
		const char *p;

		name = crl_get_sign_name(crl, &err);
		if (name == NULL)
			p = _("unknown");
		else
			p = name;

		addf(str, _("\tSignature Algorithm: %s\n"), p);
		gnutls_free(name);

		if (err != GNUTLS_SIGN_UNKNOWN && gnutls_sign_is_secure2(err, GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS) == 0) {
			adds(str,
			     _("warning: signed using a broken signature "
			       "algorithm that can be forged.\n"));
		}

		err = gnutls_x509_crl_get_signature(crl, buffer, &size);
		if (err != GNUTLS_E_SHORT_MEMORY_BUFFER) {
			addf(str, "error: get_signature: %s\n",
			     gnutls_strerror(err));
			return;
		}

		buffer = gnutls_malloc(size);
		if (!buffer) {
			addf(str, "error: malloc: %s\n",
			     gnutls_strerror(GNUTLS_E_MEMORY_ERROR));
			return;
		}

		err = gnutls_x509_crl_get_signature(crl, buffer, &size);
		if (err < 0) {
			gnutls_free(buffer);
			addf(str, "error: get_signature2: %s\n",
			     gnutls_strerror(err));
			return;
		}

		adds(str, _("\tSignature:\n"));
		_gnutls_buffer_hexdump(str, buffer, size, "\t\t");

		gnutls_free(buffer);
	}
}

/**
 * gnutls_x509_crl_print:
 * @crl: The data to be printed
 * @format: Indicate the format to use
 * @out: Newly allocated datum with null terminated string.
 *
 * This function will pretty print a X.509 certificate revocation
 * list, suitable for display to a human.
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_print(gnutls_x509_crl_t crl,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out)
{
	gnutls_buffer_st str;

	_gnutls_buffer_init(&str);

	_gnutls_buffer_append_str
	    (&str, _("X.509 Certificate Revocation List Information:\n"));

	print_crl(&str, crl, format == GNUTLS_CRT_PRINT_UNSIGNED_FULL);

	return _gnutls_buffer_to_datum(&str, out, 1);
}

static int
print_crq_sig_params(gnutls_buffer_st * str, gnutls_x509_crq_t crt,
		     gnutls_certificate_print_formats_t format)
{
	int ret;
	gnutls_pk_algorithm_t pk;
	gnutls_x509_spki_st params;
	gnutls_sign_algorithm_t sign;

	sign = gnutls_x509_crq_get_signature_algorithm(crt);
	pk = gnutls_sign_get_pk_algorithm(sign);
	if (pk == GNUTLS_PK_RSA_PSS) {
		ret = _gnutls_x509_read_sign_params(crt->crq,
						    "signatureAlgorithm",
						    &params);
		if (ret < 0) {
			addf(str, "error: read_pss_params: %s\n",
			     gnutls_strerror(ret));
		} else
			addf(str, "\t\tSalt Length: %d\n", params.salt_size);
	}

	return 0;
}

static int
print_crq_pubkey(gnutls_buffer_st * str, gnutls_x509_crq_t crq,
		 gnutls_certificate_print_formats_t format)
{
	gnutls_pubkey_t pubkey;
	gnutls_x509_spki_st params;
	int ret;

	ret = _gnutls_x509_crq_read_spki_params(crq, &params);
	if (ret < 0)
		return ret;

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return ret;

	ret = gnutls_pubkey_import_x509_crq(pubkey, crq, 0);
	if (ret < 0)
		goto cleanup;

	print_pubkey(str, _("Subject "), pubkey, &params, format);
	ret = 0;

      cleanup:
	gnutls_pubkey_deinit(pubkey);

	if (ret < 0) { /* print only name */
		const char *p;
		char *name = crq_get_pk_name(crq);
		if (name == NULL)
			p = _("unknown");
		else
			p = name;

		addf(str, "\tSubject Public Key Algorithm: %s\n", p);
		gnutls_free(name);
		ret = 0;
	}

	return ret;
}

static void
print_crq(gnutls_buffer_st * str, gnutls_x509_crq_t cert,
	  gnutls_certificate_print_formats_t format)
{
	/* Version. */
	{
		int version = gnutls_x509_crq_get_version(cert);
		if (version < 0)
			addf(str, "error: get_version: %s\n",
			     gnutls_strerror(version));
		else
			addf(str, _("\tVersion: %d\n"), version);
	}

	/* Subject */
	{
		gnutls_datum_t dn;
		int err;

		err = gnutls_x509_crq_get_dn3(cert, &dn, 0);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			addf(str, _("\tSubject:\n"));
		} else if (err < 0) {
			addf(str, "error: get_dn: %s\n",
				     gnutls_strerror(err));
		} else {
			addf(str, _("\tSubject: %s\n"), dn.data);
			gnutls_free(dn.data);
		}
	}

	{
		char *name;
		const char *p;

		print_crq_pubkey(str, cert, format);

		name = crq_get_sign_name(cert);
		if (name == NULL)
			p = _("unknown");
		else
			p = name;

		addf(str, _("\tSignature Algorithm: %s\n"), p);

		gnutls_free(name);

		print_crq_sig_params(str, cert, format);
	}

	/* parse attributes */
	{
		size_t i;
		int err = 0;
		int extensions = 0;
		int challenge = 0;

		for (i = 0;; i++) {
			char oid[MAX_OID_SIZE] = "";
			size_t sizeof_oid = sizeof(oid);

			err =
			    gnutls_x509_crq_get_attribute_info(cert, i,
							       oid,
							       &sizeof_oid);
			if (err < 0) {
				if (err ==
				    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
					break;
				addf(str,
				     "error: get_extension_info: %s\n",
				     gnutls_strerror(err));
				continue;
			}

			if (i == 0)
				adds(str, _("\tAttributes:\n"));

			if (strcmp(oid, "1.2.840.113549.1.9.14") == 0) {
				cert_type_t ccert;

				if (extensions) {
					addf(str,
					     "warning: more than one extensionsRequest\n");
				}

				ccert.crq = cert;
				print_extensions(str, "\t", TYPE_CRQ,
						 ccert);

				extensions++;
			} else if (strcmp(oid, "1.2.840.113549.1.9.7") ==
				   0) {
				char *pass;
				size_t size;

				if (challenge) {
					adds(str,
					     "warning: more than one Challenge password attribute\n");
				}

				err =
				    gnutls_x509_crq_get_challenge_password
				    (cert, NULL, &size);
				if (err < 0
				    && err !=
				    GNUTLS_E_SHORT_MEMORY_BUFFER) {
					addf(str,
					     "error: get_challenge_password: %s\n",
					     gnutls_strerror(err));
					continue;
				}

				size++;

				pass = gnutls_malloc(size);
				if (!pass) {
					addf(str, "error: malloc: %s\n",
					     gnutls_strerror
					     (GNUTLS_E_MEMORY_ERROR));
					continue;
				}

				err =
				    gnutls_x509_crq_get_challenge_password
				    (cert, pass, &size);
				if (err < 0)
					addf(str,
					     "error: get_challenge_password: %s\n",
					     gnutls_strerror(err));
				else
					addf(str,
					     _
					     ("\t\tChallenge password: %s\n"),
					     pass);

				gnutls_free(pass);

				challenge++;
			} else {
				char *buffer;
				size_t extlen = 0;

				addf(str, _("\t\tUnknown attribute %s:\n"),
				     oid);

				err =
				    gnutls_x509_crq_get_attribute_data
				    (cert, i, NULL, &extlen);
				if (err < 0) {
					addf(str,
					     "error: get_attribute_data: %s\n",
					     gnutls_strerror(err));
					continue;
				}

				buffer = gnutls_malloc(extlen);
				if (!buffer) {
					addf(str, "error: malloc: %s\n",
					     gnutls_strerror
					     (GNUTLS_E_MEMORY_ERROR));
					continue;
				}

				err =
				    gnutls_x509_crq_get_attribute_data
				    (cert, i, buffer, &extlen);
				if (err < 0) {
					gnutls_free(buffer);
					addf(str,
					     "error: get_attribute_data2: %s\n",
					     gnutls_strerror(err));
					continue;
				}

				adds(str, _("\t\t\tASCII: "));
				_gnutls_buffer_asciiprint(str, buffer,
							  extlen);
				adds(str, "\n");

				adds(str, _("\t\t\tHexdump: "));
				_gnutls_buffer_hexprint(str, buffer,
							extlen);
				adds(str, "\n");

				gnutls_free(buffer);
			}
		}
	}
}

static void print_crq_other(gnutls_buffer_st * str, gnutls_x509_crq_t crq)
{
	int ret;

	/* on unknown public key algorithms don't print the key ID */
	ret = gnutls_x509_crq_get_pk_algorithm(crq, NULL);
	if (ret < 0)
		return;

	print_obj_id(str, "\t", crq, (get_id_func*)gnutls_x509_crq_get_key_id);
}

/**
 * gnutls_x509_crq_print:
 * @crq: The data to be printed
 * @format: Indicate the format to use
 * @out: Newly allocated datum with null terminated string.
 *
 * This function will pretty print a certificate request, suitable for
 * display to a human.
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_print(gnutls_x509_crq_t crq,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out)
{
	gnutls_buffer_st str;

	_gnutls_buffer_init(&str);

	_gnutls_buffer_append_str
	    (&str, _("PKCS #10 Certificate Request Information:\n"));

	print_crq(&str, crq, format);

	_gnutls_buffer_append_str(&str, _("Other Information:\n"));

	print_crq_other(&str, crq);

	return _gnutls_buffer_to_datum(&str, out, 1);
}

static void
print_pubkey_other(gnutls_buffer_st * str, gnutls_pubkey_t pubkey,
		   gnutls_certificate_print_formats_t format)
{
	int ret;
	unsigned int usage;

	ret = gnutls_pubkey_get_key_usage(pubkey, &usage);
	if (ret < 0) {
		addf(str, "error: get_key_usage: %s\n",
		     gnutls_strerror(ret));
		return;
	}

	adds(str, "\n");
	if (pubkey->key_usage) {
		adds(str, _("Public Key Usage:\n"));
		print_key_usage2(str, "\t", pubkey->key_usage);
	}

	/* on unknown public key algorithms don't print the key ID */
	ret = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);
	if (ret < 0)
		return;

	print_obj_id(str, "", pubkey, (get_id_func*)gnutls_pubkey_get_key_id);
}

/**
 * gnutls_pubkey_print:
 * @pubkey: The data to be printed
 * @format: Indicate the format to use
 * @out: Newly allocated datum with null terminated string.
 *
 * This function will pretty print public key information, suitable for
 * display to a human.
 *
 * Only %GNUTLS_CRT_PRINT_FULL and %GNUTLS_CRT_PRINT_FULL_NUMBERS
 * are implemented.
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.5
 **/
int
gnutls_pubkey_print(gnutls_pubkey_t pubkey,
		    gnutls_certificate_print_formats_t format,
		    gnutls_datum_t * out)
{
	gnutls_buffer_st str;

	_gnutls_buffer_init(&str);

	_gnutls_buffer_append_str(&str, _("Public Key Information:\n"));

	print_pubkey(&str, "", pubkey, NULL, format);
	print_pubkey_other(&str, pubkey, format);

	return _gnutls_buffer_to_datum(&str, out, 1);
}

/**
 * gnutls_x509_ext_print:
 * @exts: The data to be printed
 * @exts_size: the number of available structures
 * @format: Indicate the format to use
 * @out: Newly allocated datum with null terminated string.
 *
 * This function will pretty print X.509 certificate extensions, 
 * suitable for display to a human.
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_ext_print(gnutls_x509_ext_st *exts, unsigned int exts_size,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out)
{
	gnutls_buffer_st str;
	struct ext_indexes_st idx;
	unsigned i;

	memset(&idx, 0, sizeof(idx));
	_gnutls_buffer_init(&str);

	for (i=0;i<exts_size;i++)
		print_extension(&str, "", &idx, (char*)exts[i].oid, exts[i].critical, &exts[i].data);

	return _gnutls_buffer_to_datum(&str, out, 1);
}
