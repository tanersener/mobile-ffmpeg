/*
 * Copyright (C) 2003-2014 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include <libtasn1.h>
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <str.h>
#include <common.h>
#include <num.h>

/* This file includes all the required to parse an X.509 Distriguished
 * Name (you need a parser just to read a name in the X.509 protocols!!!)
 */

static int append_elements(ASN1_TYPE asn1_struct, const char *asn1_rdn_name, gnutls_buffer_st *str, int k1, unsigned last)
{
	int k2, result, max_k2;
	int len;
	uint8_t value[MAX_STRING_LEN];
	char tmpbuffer1[MAX_NAME_SIZE];
	char tmpbuffer2[MAX_NAME_SIZE];
	char tmpbuffer3[MAX_NAME_SIZE];
	const char *ldap_desc;
	char oid[MAX_OID_SIZE];
	gnutls_datum_t td = { NULL, 0 };
	gnutls_datum_t tvd = { NULL, 0 };

	/* create a string like "tbsCertList.issuer.rdnSequence.?1"
	 */
	if (asn1_rdn_name[0] != 0)
		snprintf(tmpbuffer1, sizeof(tmpbuffer1), "%s.?%u",
			 asn1_rdn_name, k1);
	else
		snprintf(tmpbuffer1, sizeof(tmpbuffer1), "?%u",
			 k1);

	len = sizeof(value) - 1;
	result =
	    asn1_read_value(asn1_struct, tmpbuffer1, value, &len);

	if (result != ASN1_VALUE_NOT_FOUND && result != ASN1_SUCCESS) { /* expected */
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	k2 = 0;

	result = asn1_number_of_elements(asn1_struct, tmpbuffer1, &max_k2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	do {		/* Move to the attibute type and values
				 */
		k2++;

		if (tmpbuffer1[0] != 0)
			snprintf(tmpbuffer2, sizeof(tmpbuffer2),
				 "%s.?%u", tmpbuffer1, k2);
		else
			snprintf(tmpbuffer2, sizeof(tmpbuffer2),
				 "?%u", k2);

		/* Try to read the RelativeDistinguishedName attributes.
		 */

		len = sizeof(value) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer2, value,
				    &len);

		if (result == ASN1_ELEMENT_NOT_FOUND)
			break;
		if (result != ASN1_VALUE_NOT_FOUND && result != ASN1_SUCCESS) { /* expected */
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Read the OID 
		 */
		_gnutls_str_cpy(tmpbuffer3, sizeof(tmpbuffer3),
				tmpbuffer2);
		_gnutls_str_cat(tmpbuffer3, sizeof(tmpbuffer3),
				".type");

		len = sizeof(oid) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer3, oid,
				    &len);

		if (result == ASN1_ELEMENT_NOT_FOUND)
			break;
		else if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Read the Value 
		 */
		_gnutls_str_cpy(tmpbuffer3, sizeof(tmpbuffer3),
				tmpbuffer2);
		_gnutls_str_cat(tmpbuffer3, sizeof(tmpbuffer3),
				".value");

		len = 0;

		result =
		    _gnutls_x509_read_value(asn1_struct,
					    tmpbuffer3, &tvd);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
#define STR_APPEND(y) if ((result=_gnutls_buffer_append_str( str, y)) < 0) { \
	gnutls_assert(); \
	goto cleanup; \
}
#define DATA_APPEND(x,y) if ((result=_gnutls_buffer_append_data( str, x,y)) < 0) { \
	gnutls_assert(); \
	goto cleanup; \
}
		/*   The encodings of adjoining RelativeDistinguishedNames are separated
		 *   by a comma character (',' ASCII 44).
		 */

		ldap_desc =
		    gnutls_x509_dn_oid_name(oid,
					    GNUTLS_X509_DN_OID_RETURN_OID);

		STR_APPEND(ldap_desc);
		STR_APPEND("=");

		result =
		    _gnutls_x509_dn_to_string(oid, tvd.data,
					      tvd.size, &td);
		if (result < 0) {
			gnutls_assert();
			_gnutls_debug_log
			    ("Cannot parse OID: '%s' with value '%s'\n",
			     oid, _gnutls_bin2hex(tvd.data,
						  tvd.size,
						  tmpbuffer3,
						  sizeof
						  (tmpbuffer3),
						  NULL));
			goto cleanup;
		}

		DATA_APPEND(td.data, td.size);
		_gnutls_free_datum(&td);
		_gnutls_free_datum(&tvd);

		/*   Where there is a multi-valued RDN, the outputs from adjoining
		 *   AttributeTypeAndValues are separated by a plus ('+' ASCII 43)
		 *   character.
		 */
		if (k2 < max_k2) {
			STR_APPEND("+");
		} else if (!last) {
			STR_APPEND(",");
		}
	}
	while (1);

	result = 0;

 cleanup:
	_gnutls_free_datum(&td);
	_gnutls_free_datum(&tvd);
	return result;
}

int
_gnutls_x509_get_dn(ASN1_TYPE asn1_struct,
		    const char *asn1_rdn_name, gnutls_datum_t * dn,
		    unsigned flags)
{
	gnutls_buffer_st out_str;
	int i, k1, result;

	_gnutls_buffer_init(&out_str);

	result = asn1_number_of_elements(asn1_struct, asn1_rdn_name, &k1);
	if (result != ASN1_SUCCESS) {
		if (result == ASN1_ELEMENT_NOT_FOUND || result == ASN1_VALUE_NOT_FOUND) {
			result = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		} else {
			gnutls_assert();
			result = _gnutls_asn2err(result);
		}
		goto cleanup;
	}

	if (k1 == 0) {
		gnutls_assert();
		result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	}

	if (flags & GNUTLS_X509_DN_FLAG_COMPAT) {
		for (i=0;i<k1;i++) {
			result = append_elements(asn1_struct, asn1_rdn_name, &out_str, i+1, (i==(k1-1))?1:0);
			if (result < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
	} else {
		while (k1 > 0) {
			result = append_elements(asn1_struct, asn1_rdn_name, &out_str, k1, k1==1?1:0);
			if (result < 0) {
				gnutls_assert();
				goto cleanup;
			}
			k1--;
		}
	}

	return _gnutls_buffer_to_datum(&out_str, dn, 1);

 cleanup:
	_gnutls_buffer_clear(&out_str);
	return result;

}


/* Parses an X509 DN in the asn1_struct, and puts the output into
 * the string buf. The output is an LDAP encoded DN.
 *
 * asn1_rdn_name must be a string in the form "tbsCertificate.issuer.rdnSequence".
 * That is to point in the rndSequence.
 */
int
_gnutls_x509_parse_dn(ASN1_TYPE asn1_struct,
		      const char *asn1_rdn_name, char *buf,
		      size_t * buf_size, unsigned flags)
{
	int ret;
	gnutls_datum_t dn = {NULL, 0};

	if (buf_size == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (*buf_size > 0 && buf)
		buf[0] = 0;
	else
		*buf_size = 0;

	ret = _gnutls_x509_get_dn(asn1_struct, asn1_rdn_name, &dn, flags);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (dn.size >= (unsigned int) *buf_size) {
		gnutls_assert();
		*buf_size = dn.size + 1;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto cleanup;
	}

	if (buf) {
		memcpy(buf, dn.data, dn.size);
		buf[dn.size] = 0;
		*buf_size = dn.size;
	} else
		*buf_size = dn.size + 1;

	ret = 0;
      cleanup:
	_gnutls_free_datum(&dn);
	return ret;
}

/* Parses an X509 DN in the asn1_struct, and searches for the
 * given OID in the DN.
 *
 * If raw_flag == 0, the output will be encoded in the LDAP way. (#hex for non printable)
 * Otherwise the raw DER data are returned.
 *
 * asn1_rdn_name must be a string in the form "tbsCertificate.issuer.rdnSequence".
 * That is to point in the rndSequence.
 *
 * indx specifies which OID to return. Ie 0 means return the first specified
 * OID found, 1 the second etc.
 */
int
_gnutls_x509_parse_dn_oid(ASN1_TYPE asn1_struct,
			  const char *asn1_rdn_name,
			  const char *given_oid, int indx,
			  unsigned int raw_flag, gnutls_datum_t * out)
{
	int k2, k1, result;
	char tmpbuffer1[MAX_NAME_SIZE];
	char tmpbuffer2[MAX_NAME_SIZE];
	char tmpbuffer3[MAX_NAME_SIZE];
	gnutls_datum_t td;
	uint8_t value[256];
	char oid[MAX_OID_SIZE];
	int len;
	int i = 0;

	k1 = 0;
	do {

		k1++;
		/* create a string like "tbsCertList.issuer.rdnSequence.?1"
		 */
		if (asn1_rdn_name[0] != 0)
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "%s.?%u",
				 asn1_rdn_name, k1);
		else
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "?%u",
				 k1);

		len = sizeof(value) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer1, value, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			break;
		}

		if (result != ASN1_VALUE_NOT_FOUND) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		k2 = 0;

		do {		/* Move to the attibute type and values
				 */
			k2++;

			if (tmpbuffer1[0] != 0)
				snprintf(tmpbuffer2, sizeof(tmpbuffer2),
					 "%s.?%u", tmpbuffer1, k2);
			else
				snprintf(tmpbuffer2, sizeof(tmpbuffer2),
					 "?%u", k2);

			/* Try to read the RelativeDistinguishedName attributes.
			 */

			len = sizeof(value) - 1;
			result =
			    asn1_read_value(asn1_struct, tmpbuffer2, value,
					    &len);

			if (result == ASN1_ELEMENT_NOT_FOUND) {
				break;
			}
			if (result != ASN1_VALUE_NOT_FOUND) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			/* Read the OID 
			 */
			_gnutls_str_cpy(tmpbuffer3, sizeof(tmpbuffer3),
					tmpbuffer2);
			_gnutls_str_cat(tmpbuffer3, sizeof(tmpbuffer3),
					".type");

			len = sizeof(oid) - 1;
			result =
			    asn1_read_value(asn1_struct, tmpbuffer3, oid,
					    &len);

			if (result == ASN1_ELEMENT_NOT_FOUND)
				break;
			else if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (strcmp(oid, given_oid) == 0 && indx == i++) {	/* Found the OID */

				/* Read the Value 
				 */
				_gnutls_str_cpy(tmpbuffer3,
						sizeof(tmpbuffer3),
						tmpbuffer2);
				_gnutls_str_cat(tmpbuffer3,
						sizeof(tmpbuffer3),
						".value");

				result =
				    _gnutls_x509_read_value(asn1_struct,
							    tmpbuffer3,
							    &td);
				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}

				if (raw_flag != 0) {
					out->data = td.data;
					out->size = td.size;
					return 0;

				} else {	/* parse data. raw_flag == 0 */
					result =
					    _gnutls_x509_dn_to_string(oid,
								      td.
								      data,
								      td.
								      size,
								      out);

					_gnutls_free_datum(&td);
					if (result < 0) {
						gnutls_assert();
						goto cleanup;
					}

					return 0;

				}	/* raw_flag == 0 */
			}
		}
		while (1);

	}
	while (1);

	gnutls_assert();

	result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

      cleanup:
	return result;
}


/* Parses an X509 DN in the asn1_struct, and returns the requested
 * DN OID.
 *
 * asn1_rdn_name must be a string in the form "tbsCertificate.issuer.rdnSequence".
 * That is to point in the rndSequence.
 *
 * indx specifies which OID to return. Ie 0 means return the first specified
 * OID found, 1 the second etc.
 */
int
_gnutls_x509_get_dn_oid(ASN1_TYPE asn1_struct,
			const char *asn1_rdn_name,
			int indx, void *_oid, size_t * sizeof_oid)
{
	int k2, k1, result;
	char tmpbuffer1[MAX_NAME_SIZE];
	char tmpbuffer2[MAX_NAME_SIZE];
	char tmpbuffer3[MAX_NAME_SIZE];
	char value[256];
	char oid[MAX_OID_SIZE];
	int len;
	int i = 0;

	k1 = 0;
	do {

		k1++;
		/* create a string like "tbsCertList.issuer.rdnSequence.?1"
		 */
		if (asn1_rdn_name[0] != 0)
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "%s.?%u",
				 asn1_rdn_name, k1);
		else
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "?%u",
				 k1);

		len = sizeof(value) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer1, value, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			break;
		}

		if (result != ASN1_VALUE_NOT_FOUND) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		k2 = 0;

		do {		/* Move to the attibute type and values
				 */
			k2++;

			if (tmpbuffer1[0] != 0)
				snprintf(tmpbuffer2, sizeof(tmpbuffer2),
					 "%s.?%u", tmpbuffer1, k2);
			else
				snprintf(tmpbuffer2, sizeof(tmpbuffer2),
					 "?%u", k2);

			/* Try to read the RelativeDistinguishedName attributes.
			 */

			len = sizeof(value) - 1;
			result =
			    asn1_read_value(asn1_struct, tmpbuffer2, value,
					    &len);

			if (result == ASN1_ELEMENT_NOT_FOUND) {
				break;
			}
			if (result != ASN1_VALUE_NOT_FOUND) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			/* Read the OID 
			 */
			_gnutls_str_cpy(tmpbuffer3, sizeof(tmpbuffer3),
					tmpbuffer2);
			_gnutls_str_cat(tmpbuffer3, sizeof(tmpbuffer3),
					".type");

			len = sizeof(oid) - 1;
			result =
			    asn1_read_value(asn1_struct, tmpbuffer3, oid,
					    &len);

			if (result == ASN1_ELEMENT_NOT_FOUND)
				break;
			else if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (indx == i++) {	/* Found the OID */

				len = strlen(oid) + 1;

				if (*sizeof_oid < (unsigned) len) {
					*sizeof_oid = len;
					gnutls_assert();
					return
					    GNUTLS_E_SHORT_MEMORY_BUFFER;
				}

				memcpy(_oid, oid, len);
				*sizeof_oid = len - 1;

				return 0;
			}
		}
		while (1);

	}
	while (1);

	gnutls_assert();

	result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

      cleanup:
	return result;
}

/* This will write the AttributeTypeAndValue field. The data must be already DER encoded.
 * 'multi' must be (0) if writing an AttributeTypeAndValue, and 1 if Attribute.
 * In all cases only one value is written.
 */
static int
_gnutls_x509_write_attribute(const char *given_oid,
			     ASN1_TYPE asn1_struct, const char *where,
			     const void *_data, int sizeof_data)
{
	char tmp[128];
	int result;

	/* write the data (value)
	 */

	_gnutls_str_cpy(tmp, sizeof(tmp), where);
	_gnutls_str_cat(tmp, sizeof(tmp), ".value");

	result = asn1_write_value(asn1_struct, tmp, _data, sizeof_data);
	if (result < 0) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* write the type
	 */
	_gnutls_str_cpy(tmp, sizeof(tmp), where);
	_gnutls_str_cat(tmp, sizeof(tmp), ".type");

	result = asn1_write_value(asn1_struct, tmp, given_oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}


/* Decodes an X.509 Attribute (if multi==1) or an AttributeTypeAndValue
 * otherwise.
 *
 * octet_string should be non-zero if we are to decode octet strings after
 * decoding.
 *
 * The output is allocated and stored in value.
 */
int
_gnutls_x509_decode_and_read_attribute(ASN1_TYPE asn1_struct,
				       const char *where, char *oid,
				       int oid_size,
				       gnutls_datum_t * value, int multi,
				       int octet_string)
{
	char tmpbuffer[128];
	int len, result;

	/* Read the OID 
	 */
	_gnutls_str_cpy(tmpbuffer, sizeof(tmpbuffer), where);
	_gnutls_str_cat(tmpbuffer, sizeof(tmpbuffer), ".type");

	len = oid_size - 1;
	result = asn1_read_value(asn1_struct, tmpbuffer, oid, &len);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		return result;
	}

	/* Read the Value 
	 */

	_gnutls_str_cpy(tmpbuffer, sizeof(tmpbuffer), where);
	_gnutls_str_cat(tmpbuffer, sizeof(tmpbuffer), ".value");

	if (multi)
		_gnutls_str_cat(tmpbuffer, sizeof(tmpbuffer), "s.?1");	/* .values.?1 */

	if (octet_string)
		result =
		    _gnutls_x509_read_string(asn1_struct, tmpbuffer, value,
					     ASN1_ETYPE_OCTET_STRING, 0);
	else
		result =
		    _gnutls_x509_read_value(asn1_struct, tmpbuffer, value);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;

}

/* Sets an X509 DN in the asn1_struct, and puts the given OID in the DN.
 * The input is assumed to be raw data.
 *
 * asn1_rdn_name must be a string in the form "tbsCertificate.issuer".
 * That is to point before the rndSequence.
 *
 */
int
_gnutls_x509_set_dn_oid(ASN1_TYPE asn1_struct,
			const char *asn1_name, const char *given_oid,
			int raw_flag, const char *name, int sizeof_name)
{
	int result;
	char tmp[MAX_NAME_SIZE], asn1_rdn_name[MAX_NAME_SIZE];

	if (sizeof_name == 0 || name == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* create the rdnSequence
	 */
	result =
	    asn1_write_value(asn1_struct, asn1_name, "rdnSequence", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (asn1_name[0] != 0) {
		_gnutls_str_cpy(asn1_rdn_name, sizeof(asn1_rdn_name), asn1_name);
		_gnutls_str_cat(asn1_rdn_name, sizeof(asn1_rdn_name),
			".rdnSequence");
	} else {
		_gnutls_str_cpy(asn1_rdn_name, sizeof(asn1_rdn_name), "rdnSequence");
	}

	/* create a new element 
	 */
	result = asn1_write_value(asn1_struct, asn1_rdn_name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	_gnutls_str_cpy(tmp, sizeof(tmp), asn1_rdn_name);
	_gnutls_str_cat(tmp, sizeof(tmp), ".?LAST");

	/* create the set with only one element
	 */
	result = asn1_write_value(asn1_struct, tmp, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}


	/* Encode and write the data
	 */
	_gnutls_str_cpy(tmp, sizeof(tmp), asn1_rdn_name);
	_gnutls_str_cat(tmp, sizeof(tmp), ".?LAST.?LAST");

	if (!raw_flag) {
		result =
		    _gnutls_x509_encode_and_write_attribute(given_oid,
							    asn1_struct,
							    tmp, name,
							    sizeof_name,
							    0);
	} else {
		result =
		    _gnutls_x509_write_attribute(given_oid, asn1_struct,
						 tmp, name, sizeof_name);
	}

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}


/**
 * gnutls_x509_rdn_get:
 * @idn: should contain a DER encoded RDN sequence
 * @buf: a pointer to a structure to hold the peer's name
 * @buf_size: holds the size of @buf
 *
 * This function will return the name of the given RDN sequence.  The
 * name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as described in
 * RFC4514.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_rdn_get2().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or
 * %GNUTLS_E_SHORT_MEMORY_BUFFER is returned and *@buf_size is
 * updated if the provided buffer is not long enough, otherwise a
 * negative error value.
 **/
int
gnutls_x509_rdn_get(const gnutls_datum_t * idn,
		    char *buf, size_t * buf_size)
{
	int result;
	ASN1_TYPE dn = ASN1_TYPE_EMPTY;

	if (buf_size == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (buf)
		buf[0] = 0;


	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Name", &dn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&dn, idn->data, idn->size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_parse_dn(dn, "rdnSequence", buf, buf_size, GNUTLS_X509_DN_FLAG_COMPAT);

	asn1_delete_structure(&dn);
	return result;

}

/**
 * gnutls_x509_rdn_get2:
 * @idn: should contain a DER encoded RDN sequence
 * @str: a datum that will hold the name
 * @flags: zero of %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will return the name of the given RDN sequence.  The
 * name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as described in
 * RFC4514.
 *
 * When the flag %GNUTLS_X509_DN_FLAG_COMPAT is specified, the output
 * format will match the format output by previous to 3.5.6 versions of GnuTLS
 * which was not not fully RFC4514-compliant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or
 * %GNUTLS_E_SHORT_MEMORY_BUFFER is returned and *@buf_size is
 * updated if the provided buffer is not long enough, otherwise a
 * negative error value.
 **/
int
gnutls_x509_rdn_get2(const gnutls_datum_t * idn,
		     gnutls_datum_t *str, unsigned flags)
{
	int result;
	ASN1_TYPE dn = ASN1_TYPE_EMPTY;

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Name", &dn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&dn, idn->data, idn->size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_get_dn(dn, "rdnSequence", str, flags);

	asn1_delete_structure(&dn);
	return result;

}

/**
 * gnutls_x509_rdn_get_by_oid:
 * @idn: should contain a DER encoded RDN sequence
 * @oid: an Object Identifier
 * @indx: In case multiple same OIDs exist in the RDN indicates which
 *   to send. Use 0 for the first one.
 * @raw_flag: If non-zero then the raw DER data are returned.
 * @buf: a pointer to a structure to hold the peer's name
 * @buf_size: holds the size of @buf
 *
 * This function will return the name of the given Object identifier,
 * of the RDN sequence.  The name will be encoded using the rules
 * from RFC4514.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or
 * %GNUTLS_E_SHORT_MEMORY_BUFFER is returned and *@buf_size is
 * updated if the provided buffer is not long enough, otherwise a
 * negative error value.
 **/
int
gnutls_x509_rdn_get_by_oid(const gnutls_datum_t * idn, const char *oid,
			   unsigned indx, unsigned int raw_flag,
			   void *buf, size_t * buf_size)
{
	int result;
	ASN1_TYPE dn = ASN1_TYPE_EMPTY;
	gnutls_datum_t td;

	if (buf_size == 0) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Name", &dn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&dn, idn->data, idn->size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}

	result =
	    _gnutls_x509_parse_dn_oid(dn, "rdnSequence", oid, indx,
				      raw_flag, &td);

	asn1_delete_structure(&dn);
	if (result < 0)
		return gnutls_assert_val(result);

	return _gnutls_strdatum_to_buf(&td, buf, buf_size);
}

/**
 * gnutls_x509_rdn_get_oid:
 * @idn: should contain a DER encoded RDN sequence
 * @indx: Indicates which OID to return. Use 0 for the first one.
 * @buf: a pointer to a structure to hold the peer's name OID
 * @buf_size: holds the size of @buf
 *
 * This function will return the specified Object identifier, of the
 * RDN sequence.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or
 * %GNUTLS_E_SHORT_MEMORY_BUFFER is returned and *@buf_size is
 * updated if the provided buffer is not long enough, otherwise a
 * negative error value.
 *
 * Since: 2.4.0
 **/
int
gnutls_x509_rdn_get_oid(const gnutls_datum_t * idn,
			unsigned indx, void *buf, size_t * buf_size)
{
	int result;
	ASN1_TYPE dn = ASN1_TYPE_EMPTY;

	if (buf_size == 0) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Name", &dn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&dn, idn->data, idn->size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}

	result =
	    _gnutls_x509_get_dn_oid(dn, "rdnSequence", indx, buf,
				    buf_size);

	asn1_delete_structure(&dn);
	return result;
}

/*
 * Compares the DER encoded part of a DN.
 *
 * Returns 1 if the DN's match and (0) if they don't match. Otherwise
 * a negative error code is returned to indicate error.
 */
int
_gnutls_x509_compare_raw_dn(const gnutls_datum_t * dn1,
			    const gnutls_datum_t * dn2)
{

	if (dn1->size != dn2->size) {
		gnutls_assert();
		return 0;
	}
	if (memcmp(dn1->data, dn2->data, dn2->size) != 0) {
		gnutls_assert();
		return 0;
	}
	return 1;		/* they match */
}
