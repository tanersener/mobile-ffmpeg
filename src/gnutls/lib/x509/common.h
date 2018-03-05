/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
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

#ifndef COMMON_H
#define COMMON_H

#include <algorithms.h>
#include <abstract_int.h>
#include <x509/x509_int.h>
#include <fips.h>

#define MAX_STRING_LEN 512

#define GNUTLS_XML_SHOW_ALL 1

#define PEM_CRL "X509 CRL"
#define PEM_X509_CERT "X509 CERTIFICATE"
#define PEM_X509_CERT2 "CERTIFICATE"
#define PEM_PKCS7 "PKCS7"
#define PEM_PKCS12 "PKCS12"
#define PEM_PK "PUBLIC KEY"

/* public key algorithm's OIDs
 */
#define PK_PKIX1_RSA_OID "1.2.840.113549.1.1.1"
#define PK_X509_RSA_OID "2.5.8.1.1"
#define PK_DSA_OID "1.2.840.10040.4.1"
#define PK_GOST_R3410_94_OID "1.2.643.2.2.20"
#define PK_GOST_R3410_2001_OID "1.2.643.2.2.19"

/* signature OIDs
 */
#define SIG_DSA_SHA1_OID "1.2.840.10040.4.3"
/* those two from draft-ietf-pkix-sha2-dsa-ecdsa-06 */
#define SIG_DSA_SHA224_OID "2.16.840.1.101.3.4.3.1"
#define SIG_DSA_SHA256_OID "2.16.840.1.101.3.4.3.2"
#define SIG_DSA_SHA384_OID "2.16.840.1.101.3.4.3.3"
#define SIG_DSA_SHA512_OID "2.16.840.1.101.3.4.3.4"

#define SIG_RSA_MD5_OID "1.2.840.113549.1.1.4"
#define SIG_RSA_MD2_OID "1.2.840.113549.1.1.2"
#define SIG_RSA_SHA1_OID "1.2.840.113549.1.1.5"
#define SIG_RSA_SHA224_OID "1.2.840.113549.1.1.14"
#define SIG_RSA_SHA256_OID "1.2.840.113549.1.1.11"
#define SIG_RSA_SHA384_OID "1.2.840.113549.1.1.12"
#define SIG_RSA_SHA512_OID "1.2.840.113549.1.1.13"
#define SIG_RSA_RMD160_OID "1.3.36.3.3.1.2"
#define SIG_GOST_R3410_94_OID "1.2.643.2.2.4"
#define SIG_GOST_R3410_2001_OID "1.2.643.2.2.3"
#define ISO_SIG_RSA_SHA1_OID "1.3.14.3.2.29"

#define SIG_DSA_SHA3_224_OID "2.16.840.1.101.3.4.3.5"
#define SIG_DSA_SHA3_256_OID "2.16.840.1.101.3.4.3.6"
#define SIG_DSA_SHA3_384_OID "2.16.840.1.101.3.4.3.7"
#define SIG_DSA_SHA3_512_OID "2.16.840.1.101.3.4.3.8"

#define SIG_ECDSA_SHA3_224_OID "2.16.840.1.101.3.4.3.9"
#define SIG_ECDSA_SHA3_256_OID "2.16.840.1.101.3.4.3.10"
#define SIG_ECDSA_SHA3_384_OID "2.16.840.1.101.3.4.3.11"
#define SIG_ECDSA_SHA3_512_OID "2.16.840.1.101.3.4.3.12"

#define SIG_RSA_SHA3_224_OID "2.16.840.1.101.3.4.3.13"
#define SIG_RSA_SHA3_256_OID "2.16.840.1.101.3.4.3.14"
#define SIG_RSA_SHA3_384_OID "2.16.840.1.101.3.4.3.15"
#define SIG_RSA_SHA3_512_OID "2.16.840.1.101.3.4.3.16"

#define XMPP_OID "1.3.6.1.5.5.7.8.5"
#define KRB5_PRINCIPAL_OID "1.3.6.1.5.2.2"

#define ASN1_NULL "\x05\x00"
#define ASN1_NULL_SIZE 2

int _gnutls_x509_set_time(ASN1_TYPE c2, const char *where, time_t tim,
			  int force_general);
int
_gnutls_x509_set_raw_time(ASN1_TYPE c2, const char *where, time_t tim);

int _gnutls_x509_decode_string(unsigned int etype,
			       const uint8_t * der, size_t der_size,
			       gnutls_datum_t * output,
			       unsigned allow_ber);

int _gnutls_x509_encode_string(unsigned int etype,
			       const void *input_data, size_t input_size,
			       gnutls_datum_t * output);

int _gnutls_x509_dn_to_string(const char *OID, void *value,
			      int value_size, gnutls_datum_t * out);
const char *_gnutls_ldap_string_to_oid(const char *str, unsigned str_len);

time_t _gnutls_x509_get_time(ASN1_TYPE c2, const char *when, int general);

gnutls_x509_subject_alt_name_t _gnutls_x509_san_find_type(char *str_type);

int _gnutls_x509_der_encode_and_copy(ASN1_TYPE src, const char *src_name,
				     ASN1_TYPE dest, const char *dest_name,
				     int str);
int _gnutls_x509_der_encode(ASN1_TYPE src, const char *src_name,
			    gnutls_datum_t * res, int str);

#define _gnutls_x509_export_int(asn1, format, header, out, out_size) \
  _gnutls_x509_export_int_named(asn1, "", format, header, out, out_size)

int _gnutls_x509_export_int_named(ASN1_TYPE asn1_data, const char *name,
				  gnutls_x509_crt_fmt_t format,
				  const char *pem_header,
				  unsigned char *output_data,
				  size_t * output_data_size);

#define _gnutls_x509_export_int2(asn1, format, header, out) \
  _gnutls_x509_export_int_named2(asn1, "", format, header, out)
int _gnutls_x509_export_int_named2(ASN1_TYPE asn1_data, const char *name,
				   gnutls_x509_crt_fmt_t format,
				   const char *pem_header,
				   gnutls_datum_t * out);

int _gnutls_x509_read_value(ASN1_TYPE c, const char *root,
			    gnutls_datum_t * ret);
int _gnutls_x509_read_null_value(ASN1_TYPE c, const char *root,
			    gnutls_datum_t * ret);
int _gnutls_x509_read_string(ASN1_TYPE c, const char *root,
			     gnutls_datum_t * ret, unsigned int etype,
			     unsigned allow_ber);
int _gnutls_x509_write_value(ASN1_TYPE c, const char *root,
			     const gnutls_datum_t * data);

int _gnutls_x509_write_string(ASN1_TYPE c, const char *root,
			      const gnutls_datum_t * data,
			      unsigned int etype);

int _gnutls_x509_encode_and_write_attribute(const char *given_oid,
					    ASN1_TYPE asn1_struct,
					    const char *where,
					    const void *data,
					    int sizeof_data, int multi);
int _gnutls_x509_decode_and_read_attribute(ASN1_TYPE asn1_struct,
					   const char *where, char *oid,
					   int oid_size,
					   gnutls_datum_t * value,
					   int multi, int octet);

int _gnutls_x509_get_pk_algorithm(ASN1_TYPE src, const char *src_name,
				  unsigned int *bits);

int
_gnutls_x509_get_signature_algorithm(ASN1_TYPE src, const char *src_name);

int _gnutls_x509_encode_and_copy_PKI_params(ASN1_TYPE dst,
					    const char *dst_name,
					    gnutls_pk_algorithm_t
					    pk_algorithm,
					    gnutls_pk_params_st * params);
int _gnutls_x509_encode_PKI_params(gnutls_datum_t * der,
				   gnutls_pk_algorithm_t,
				   gnutls_pk_params_st * params);
int _gnutls_asn1_copy_node(ASN1_TYPE * dst, const char *dst_name,
			   ASN1_TYPE src, const char *src_name);

int _gnutls_x509_get_signed_data(ASN1_TYPE src, const gnutls_datum_t *der,
				 const char *src_name,
				 gnutls_datum_t * signed_data);
int _gnutls_x509_get_signature(ASN1_TYPE src, const char *src_name,
			       gnutls_datum_t * signature);


int _gnutls_get_asn_mpis(ASN1_TYPE asn, const char *root,
			 gnutls_pk_params_st * params);

int _gnutls_get_key_id(gnutls_pk_algorithm_t pk, gnutls_pk_params_st *,
		       unsigned char *output_data,
		       size_t * output_data_size, unsigned flags);

void _asnstr_append_name(char *name, size_t name_size, const char *part1,
			 const char *part2);

/* Given a @c2 which it returns an allocated DER encoding of @whom in @out */
inline static int
_gnutls_x509_get_raw_field(ASN1_TYPE c2, const char *whom, gnutls_datum_t *out)
{
	return _gnutls_x509_der_encode(c2, whom, out, 0);
}

int
_gnutls_x509_get_raw_field2(ASN1_TYPE c2, gnutls_datum_t * raw,
			 const char *whom, gnutls_datum_t * dn);

unsigned
_gnutls_check_if_same_key(gnutls_x509_crt_t cert1,
			  gnutls_x509_crt_t cert2,
			  unsigned is_ca);

unsigned
_gnutls_check_if_same_key2(gnutls_x509_crt_t cert1,
			   gnutls_datum_t *cert2bin);

unsigned
_gnutls_check_valid_key_id(gnutls_datum_t *key_id,
			   gnutls_x509_crt_t cert, time_t now);

unsigned _gnutls_check_key_purpose(gnutls_x509_crt_t cert, const char *purpose, unsigned no_any);

time_t _gnutls_x509_generalTime2gtime(const char *ttime);

int _gnutls_get_extension(ASN1_TYPE asn, const char *root,
		  const char *extension_id, int indx,
		  gnutls_datum_t * ret, unsigned int *_critical);

int _gnutls_set_extension(ASN1_TYPE asn, const char *root,
		  const char *ext_id,
		  const gnutls_datum_t * ext_data, unsigned int critical);

int _gnutls_strdatum_to_buf(gnutls_datum_t * d, void *buf,
			    size_t * sizeof_buf);

unsigned _gnutls_is_same_dn(gnutls_x509_crt_t cert1, gnutls_x509_crt_t cert2);

int _gnutls_copy_string(gnutls_datum_t* str, uint8_t *out, size_t *out_size);
int _gnutls_copy_data(gnutls_datum_t* str, uint8_t *out, size_t *out_size);

int _gnutls_x509_decode_ext(const gnutls_datum_t *der, gnutls_x509_ext_st *out);
int x509_raw_crt_to_raw_pubkey(const gnutls_datum_t * cert,
			   gnutls_datum_t * rpubkey);

int x509_crt_to_raw_pubkey(gnutls_x509_crt_t crt,
			   gnutls_datum_t * rpubkey);

typedef void (*gnutls_cert_vfunc)(gnutls_x509_crt_t);

gnutls_x509_crt_t *_gnutls_sort_clist(gnutls_x509_crt_t
				     sorted[DEFAULT_MAX_VERIFY_DEPTH],
				     gnutls_x509_crt_t * clist,
				     unsigned int *clist_size,
				     gnutls_cert_vfunc func);

int _gnutls_check_if_sorted(gnutls_x509_crt_t * crt, int nr);

inline static int _asn1_strict_der_decode (asn1_node * element, const void *ider,
		       int len, char *errorDescription)
{
#ifdef ASN1_DECODE_FLAG_ALLOW_INCORRECT_TIME
# define _ASN1_DER_FLAGS ASN1_DECODE_FLAG_ALLOW_INCORRECT_TIME|ASN1_DECODE_FLAG_STRICT_DER
#else
# define _ASN1_DER_FLAGS ASN1_DECODE_FLAG_STRICT_DER
#endif
	return asn1_der_decoding2(element, ider, &len, _ASN1_DER_FLAGS, errorDescription);
}

#endif
