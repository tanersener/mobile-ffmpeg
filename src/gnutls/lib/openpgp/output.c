/*
 * Copyright (C) 2007-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Functions for printing X.509 Certificate structures
 */

#include "gnutls_int.h"
#include <gnutls/openpgp.h>
#include "errors.h"
#include <extras/randomart.h>

#define addf _gnutls_buffer_append_printf
#define adds _gnutls_buffer_append_str

static void
print_key_usage(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert,
		unsigned int idx)
{
	unsigned int key_usage;
	int err;

	adds(str, _("\t\tKey Usage:\n"));


	if (idx == (unsigned int) -1)
		err = gnutls_openpgp_crt_get_key_usage(cert, &key_usage);
	else
		err =
		    gnutls_openpgp_crt_get_subkey_usage(cert, idx,
							&key_usage);
	if (err < 0) {
		addf(str, _("error: get_key_usage: %s\n"),
		     gnutls_strerror(err));
		return;
	}

	if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE)
		adds(str, _("\t\t\tDigital signatures.\n"));
	if (key_usage & GNUTLS_KEY_KEY_ENCIPHERMENT)
		adds(str, _("\t\t\tCommunications encipherment.\n"));
	if (key_usage & GNUTLS_KEY_DATA_ENCIPHERMENT)
		adds(str, _("\t\t\tStorage data encipherment.\n"));
	if (key_usage & GNUTLS_KEY_KEY_AGREEMENT)
		adds(str, _("\t\t\tAuthentication.\n"));
	if (key_usage & GNUTLS_KEY_KEY_CERT_SIGN)
		adds(str, _("\t\t\tCertificate signing.\n"));
}

/* idx == -1 indicates main key
 * otherwise the subkey.
 */
static void
print_key_id(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert, int idx)
{
	gnutls_openpgp_keyid_t id;
	int err;

	if (idx < 0)
		err = gnutls_openpgp_crt_get_key_id(cert, id);
	else
		err = gnutls_openpgp_crt_get_subkey_id(cert, idx, id);

	if (err < 0)
		addf(str, "error: get_key_id: %s\n", gnutls_strerror(err));
	else {
		adds(str, _("\tID (hex): "));
		_gnutls_buffer_hexprint(str, id, sizeof(id));
		addf(str, "\n");
	}

}

/* idx == -1 indicates main key
 * otherwise the subkey.
 */
static void
print_key_fingerprint(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert)
{
	uint8_t fpr[128];
	size_t fpr_size = sizeof(fpr);
	int err;
	const char *name;
	char *p;
	unsigned int bits;

	err = gnutls_openpgp_crt_get_fingerprint(cert, fpr, &fpr_size);
	if (err < 0)
		addf(str, "error: get_fingerprint: %s\n",
		     gnutls_strerror(err));
	else {
		adds(str, _("\tFingerprint (hex): "));
		_gnutls_buffer_hexprint(str, fpr, fpr_size);
		addf(str, "\n");
	}

	err = gnutls_openpgp_crt_get_pk_algorithm(cert, &bits);
	if (err < 0)
		return;

	name = gnutls_pk_get_name(err);
	if (name == NULL)
		return;

	p = _gnutls_key_fingerprint_randomart(fpr, fpr_size, name, bits,
					      "\t\t");
	if (p == NULL)
		return;

	adds(str, _("\tFingerprint's random art:\n"));
	adds(str, p);
	adds(str, "\n");

	gnutls_free(p);
}

static void
print_key_revoked(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert,
		  int idx)
{
	int err;

	if (idx < 0)
		err = gnutls_openpgp_crt_get_revoked_status(cert);
	else
		err =
		    gnutls_openpgp_crt_get_subkey_revoked_status(cert,
								 idx);

	if (err != 0)
		adds(str, _("\tRevoked: True\n"));
	else
		adds(str, _("\tRevoked: False\n"));
}

static void
print_key_times(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert, int idx)
{
	time_t tim;

	adds(str, _("\tTime stamps:\n"));

	if (idx == -1)
		tim = gnutls_openpgp_crt_get_creation_time(cert);
	else
		tim =
		    gnutls_openpgp_crt_get_subkey_creation_time(cert, idx);

	{
		char s[42];
		size_t max = sizeof(s);
		struct tm t;

		if (gmtime_r(&tim, &t) == NULL)
			addf(str, "error: gmtime_r (%ld)\n",
			     (unsigned long) tim);
		else if (strftime(s, max, "%a %b %d %H:%M:%S UTC %Y", &t)
			 == 0)
			addf(str, "error: strftime (%ld)\n",
			     (unsigned long) tim);
		else
			addf(str, _("\t\tCreation: %s\n"), s);
	}

	if (idx == -1)
		tim = gnutls_openpgp_crt_get_expiration_time(cert);
	else
		tim =
		    gnutls_openpgp_crt_get_subkey_expiration_time(cert,
								  idx);
	{
		char s[42];
		size_t max = sizeof(s);
		struct tm t;

		if (tim == 0) {
			adds(str, _("\t\tExpiration: Never\n"));
		} else {
			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld)\n",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%a %b %d %H:%M:%S UTC %Y",
				  &t) == 0)
				addf(str, "error: strftime (%ld)\n",
				     (unsigned long) tim);
			else
				addf(str, _("\t\tExpiration: %s\n"), s);
		}
	}
}

static void
print_key_info(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert, int idx)
{
	int err;
	unsigned int bits;

	if (idx == -1)
		err = gnutls_openpgp_crt_get_pk_algorithm(cert, &bits);
	else
		err =
		    gnutls_openpgp_crt_get_subkey_pk_algorithm(cert, idx,
							       &bits);

	if (err < 0)
		addf(str, "error: get_pk_algorithm: %s\n",
		     gnutls_strerror(err));
	else {
		const char *name = gnutls_pk_algorithm_get_name(err);
		if (name == NULL)
			name = _("unknown");

		addf(str, _("\tPublic Key Algorithm: %s\n"), name);
		addf(str, _("\tKey Security Level: %s\n"),
		     gnutls_sec_param_get_name(gnutls_pk_bits_to_sec_param
					       (err, bits)));

		switch (err) {
		case GNUTLS_PK_RSA:
			{
				gnutls_datum_t m, e;

				if (idx == -1)
					err =
					    gnutls_openpgp_crt_get_pk_rsa_raw
					    (cert, &m, &e);
				else
					err =
					    gnutls_openpgp_crt_get_subkey_pk_rsa_raw
					    (cert, idx, &m, &e);

				if (err < 0)
					addf(str,
					     "error: get_pk_rsa_raw: %s\n",
					     gnutls_strerror(err));
				else {
					addf(str,
					     _("\t\tModulus (bits %d):\n"),
					     bits);
					_gnutls_buffer_hexdump(str, m.data,
							       m.size,
							       "\t\t\t");
					adds(str, _("\t\tExponent:\n"));
					_gnutls_buffer_hexdump(str, e.data,
							       e.size,
							       "\t\t\t");

					gnutls_free(m.data);
					gnutls_free(e.data);
				}

			}
			break;

		case GNUTLS_PK_DSA:
			{
				gnutls_datum_t p, q, g, y;

				if (idx == -1)
					err =
					    gnutls_openpgp_crt_get_pk_dsa_raw
					    (cert, &p, &q, &g, &y);
				else
					err =
					    gnutls_openpgp_crt_get_subkey_pk_dsa_raw
					    (cert, idx, &p, &q, &g, &y);
				if (err < 0)
					addf(str,
					     "error: get_pk_dsa_raw: %s\n",
					     gnutls_strerror(err));
				else {
					addf(str,
					     _
					     ("\t\tPublic key (bits %d):\n"),
					     bits);
					adds(str, _("\t\tY:\n"));
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

					gnutls_free(p.data);
					gnutls_free(q.data);
					gnutls_free(g.data);
					gnutls_free(y.data);
					adds(str, "\n");
				}
			}
			break;

		default:
			break;
		}
	}
}

static void print_cert(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert)
{
	int i, subkeys;
	int err;

	print_key_revoked(str, cert, -1);

	/* Version. */
	{
		int version = gnutls_openpgp_crt_get_version(cert);
		if (version < 0)
			addf(str, "error: get_version: %s\n",
			     gnutls_strerror(version));
		else
			addf(str, _("\tVersion: %d\n"), version);
	}

	/* ID. */
	print_key_id(str, cert, -1);

	print_key_fingerprint(str, cert);

	/* Names. */
	i = 0;
	do {
		char *dn;
		size_t dn_size = 0;

		err = gnutls_openpgp_crt_get_name(cert, i, NULL, &dn_size);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;

		if (err != GNUTLS_E_SHORT_MEMORY_BUFFER) {
			addf(str, "error: get_name: %s\n",
			     gnutls_strerror(err));
		} else {
			dn = gnutls_malloc(dn_size);
			if (!dn)
				addf(str, "error: malloc (%d): %s\n",
				     (int) dn_size,
				     gnutls_strerror
				     (GNUTLS_E_MEMORY_ERROR));
			else {
				err =
				    gnutls_openpgp_crt_get_name(cert, i,
								dn,
								&dn_size);
				if (err < 0
				    && err !=
				    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
				    && err != GNUTLS_E_OPENPGP_UID_REVOKED)
					addf(str, "error: get_name: %s\n",
					     gnutls_strerror(err));
				else if (err >= 0)
					addf(str, _("\tName[%d]: %s\n"), i,
					     dn);
				else if (err ==
					 GNUTLS_E_OPENPGP_UID_REVOKED)
					addf(str,
					     _("\tRevoked Name[%d]: %s\n"),
					     i, dn);

				gnutls_free(dn);
			}
		}

		i++;
	}
	while (err >= 0);

	print_key_times(str, cert, -1);

	print_key_info(str, cert, -1);
	print_key_usage(str, cert, -1);

	subkeys = gnutls_openpgp_crt_get_subkey_count(cert);
	if (subkeys < 0)
		return;

	for (i = 0; i < subkeys; i++) {
		addf(str, _("\n\tSubkey[%d]:\n"), i);

		print_key_revoked(str, cert, i);
		print_key_id(str, cert, i);
		print_key_times(str, cert, i);
		print_key_info(str, cert, i);
		print_key_usage(str, cert, i);
	}

}

static void
print_oneline(gnutls_buffer_st * str, gnutls_openpgp_crt_t cert)
{
	int err, i;

	i = 0;
	do {
		char *dn;
		size_t dn_size = 0;

		err = gnutls_openpgp_crt_get_name(cert, i, NULL, &dn_size);
		if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;

		if (err != GNUTLS_E_SHORT_MEMORY_BUFFER) {
			addf(str, "unknown name (%s), ",
			     gnutls_strerror(err));
		} else {
			dn = gnutls_malloc(dn_size);
			if (!dn)
				addf(str, "unknown name (%s), ",
				     gnutls_strerror
				     (GNUTLS_E_MEMORY_ERROR));
			else {
				err =
				    gnutls_openpgp_crt_get_name(cert, i,
								dn,
								&dn_size);
				if (err < 0
				    && err !=
				    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
				    && err != GNUTLS_E_OPENPGP_UID_REVOKED)
					addf(str, "unknown name (%s), ",
					     gnutls_strerror(err));
				else if (err >= 0)
					addf(str, _("name[%d]: %s, "), i,
					     dn);
				else if (err ==
					 GNUTLS_E_OPENPGP_UID_REVOKED)
					addf(str,
					     _("revoked name[%d]: %s, "),
					     i, dn);

				gnutls_free(dn);
			}
		}

		i++;
	}
	while (err >= 0);

	{
		char fpr[128];
		size_t fpr_size = sizeof(fpr);

		err =
		    gnutls_openpgp_crt_get_fingerprint(cert, fpr,
						       &fpr_size);
		if (err < 0)
			addf(str, "error: get_fingerprint: %s\n",
			     gnutls_strerror(err));
		else {
			adds(str, _("fingerprint: "));
			_gnutls_buffer_hexprint(str, fpr, fpr_size);
			addf(str, ", ");
		}
	}

	{
		time_t tim;

		tim = gnutls_openpgp_crt_get_creation_time(cert);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (gmtime_r(&tim, &t) == NULL)
				addf(str, "error: gmtime_r (%ld), ",
				     (unsigned long) tim);
			else if (strftime
				 (s, max, "%Y-%m-%d %H:%M:%S UTC",
				  &t) == 0)
				addf(str, "error: strftime (%ld), ",
				     (unsigned long) tim);
			else
				addf(str, _("created: %s, "), s);
		}

		tim = gnutls_openpgp_crt_get_expiration_time(cert);
		{
			char s[42];
			size_t max = sizeof(s);
			struct tm t;

			if (tim == 0)
				adds(str, _("never expires, "));
			else {
				if (gmtime_r(&tim, &t) == NULL)
					addf(str,
					     "error: gmtime_r (%ld), ",
					     (unsigned long) tim);
				else if (strftime
					 (s, max, "%Y-%m-%d %H:%M:%S UTC",
					  &t) == 0)
					addf(str,
					     "error: strftime (%ld), ",
					     (unsigned long) tim);
				else
					addf(str, _("expires: %s, "), s);
			}
		}
	}

	{
		unsigned int bits = 0;
		gnutls_pk_algorithm_t algo =
		    gnutls_openpgp_crt_get_pk_algorithm(cert, &bits);
		const char *algostr = gnutls_pk_algorithm_get_name(algo);

		if (algostr)
			addf(str, _("key algorithm %s (%d bits)"), algostr,
			     bits);
		else
			addf(str, _("unknown key algorithm (%d)"), algo);
	}
}

/**
 * gnutls_openpgp_crt_print:
 * @cert: The structure to be printed
 * @format: Indicate the format to use
 * @out: Newly allocated datum with (0) terminated string.
 *
 * This function will pretty print an OpenPGP certificate, suitable
 * for display to a human.
 *
 * The format should be (0) for future compatibility.
 *
 * The output @out needs to be deallocate using gnutls_free().
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_print(gnutls_openpgp_crt_t cert,
			 gnutls_certificate_print_formats_t format,
			 gnutls_datum_t * out)
{
	gnutls_buffer_st str;
	int ret;

	_gnutls_buffer_init(&str);

	if (format == GNUTLS_CRT_PRINT_ONELINE) {
		print_oneline(&str, cert);
	} else if (format == GNUTLS_CRT_PRINT_COMPACT) {
		print_oneline(&str, cert);

		ret = _gnutls_buffer_append_data(&str, "\n", 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		print_key_fingerprint(&str, cert);
	} else {
		_gnutls_buffer_append_str(&str,
					  _
					  ("OpenPGP Certificate Information:\n"));
		print_cert(&str, cert);
	}

	return _gnutls_buffer_to_datum(&str, out, 1);
}
