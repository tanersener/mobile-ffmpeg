/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* Contains functions that are supposed to pack and unpack session data,
 * before and after they are sent to the database backend.
 */

#include "gnutls_int.h"
#ifdef ENABLE_SRP
#include <auth/srp_kx.h>
#endif
#ifdef ENABLE_PSK
#include <auth/psk.h>
#endif
#include <auth/anon.h>
#include <auth/cert.h>
#include "errors.h"
#include <auth.h>
#include <session_pack.h>
#include <datum.h>
#include <num.h>
#include <hello_ext.h>
#include <constate.h>
#include <algorithms.h>
#include <state.h>
#include <db.h>
#include "tls13/session_ticket.h"

static int pack_certificate_auth_info(gnutls_session_t,
				      gnutls_buffer_st * packed_session);
static int unpack_certificate_auth_info(gnutls_session_t,
					gnutls_buffer_st * packed_session);

static int unpack_srp_auth_info(gnutls_session_t session,
				gnutls_buffer_st * packed_session);
static int pack_srp_auth_info(gnutls_session_t session,
			      gnutls_buffer_st * packed_session);

static int unpack_psk_auth_info(gnutls_session_t session,
				gnutls_buffer_st * packed_session);
static int pack_psk_auth_info(gnutls_session_t session,
			      gnutls_buffer_st * packed_session);

static int unpack_anon_auth_info(gnutls_session_t session,
				 gnutls_buffer_st * packed_session);
static int pack_anon_auth_info(gnutls_session_t session,
			       gnutls_buffer_st * packed_session);

static int unpack_security_parameters(gnutls_session_t session,
				      gnutls_buffer_st * packed_session);
static int pack_security_parameters(gnutls_session_t session,
				    gnutls_buffer_st * packed_session);
static int tls13_unpack_security_parameters(gnutls_session_t session,
					    gnutls_buffer_st * packed_session);
static int tls13_pack_security_parameters(gnutls_session_t session,
					  gnutls_buffer_st * packed_session);


/* Since auth_info structures contain malloced data, this function
 * is required in order to pack these structures in a vector in
 * order to store them to the DB.
 *
 * packed_session will contain the session data.
 *
 * The data will be in a platform independent format.
 */
int
_gnutls_session_pack(gnutls_session_t session,
		     gnutls_datum_t * packed_session)
{
	int ret;
	gnutls_buffer_st sb;
	uint8_t id;

	if (packed_session == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	_gnutls_buffer_init(&sb);


	id = gnutls_auth_get_type(session);

	BUFFER_APPEND_NUM(&sb, PACKED_SESSION_MAGIC);
	BUFFER_APPEND_NUM(&sb, session->security_parameters.timestamp);
	BUFFER_APPEND_NUM(&sb, session->internals.expire_time);
	BUFFER_APPEND(&sb, &id, 1);

	switch (id) {
#ifdef ENABLE_SRP
	case GNUTLS_CRD_SRP:
		ret = pack_srp_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
		break;
#endif
#ifdef ENABLE_PSK
	case GNUTLS_CRD_PSK:
		ret = pack_psk_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
		break;
#endif
#ifdef ENABLE_ANON
	case GNUTLS_CRD_ANON:
		ret = pack_anon_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
		break;
#endif
	case GNUTLS_CRD_CERTIFICATE:
		ret = pack_certificate_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	}

	/* Auth_info structures copied. Now copy security_parameters_st. 
	 * packed_session must have allocated space for the security parameters.
	 */
	ret = pack_security_parameters(session, &sb);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}


	if (session->security_parameters.pversion->tls13_sem) {
		ret = tls13_pack_security_parameters(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
	}

	/* Extensions are re-negotiated in a resumed session under TLS 1.3 */
	if (!session->security_parameters.pversion->tls13_sem) {
		ret = _gnutls_hello_ext_pack(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
	}

	return _gnutls_buffer_to_datum(&sb, packed_session, 0);

      fail:
	_gnutls_buffer_clear(&sb);
	return ret;
}


/* Load session data from a buffer.
 */
int
_gnutls_session_unpack(gnutls_session_t session,
		       const gnutls_datum_t * packed_session)
{
	int ret;
	gnutls_buffer_st sb;
	uint32_t magic;
	uint32_t expire_time;
	uint8_t id;

	_gnutls_buffer_init(&sb);

	if (packed_session == NULL || packed_session->size == 0) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret =
	    _gnutls_buffer_append_data(&sb, packed_session->data,
				       packed_session->size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (session->key.auth_info != NULL) {
		_gnutls_free_auth_info(session);
	}

	BUFFER_POP_NUM(&sb, magic);
	if (magic != PACKED_SESSION_MAGIC) {
		ret = gnutls_assert_val(GNUTLS_E_DB_ERROR);
		goto error;
	}

	BUFFER_POP_NUM(&sb,
		       session->internals.resumed_security_parameters.
		       timestamp);
	BUFFER_POP_NUM(&sb, expire_time);
	(void) expire_time;
	BUFFER_POP(&sb, &id, 1);

	switch (id) {
#ifdef ENABLE_SRP
	case GNUTLS_CRD_SRP:
		ret = unpack_srp_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
		break;
#endif
#ifdef ENABLE_PSK
	case GNUTLS_CRD_PSK:
		ret = unpack_psk_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
		break;
#endif
#ifdef ENABLE_ANON
	case GNUTLS_CRD_ANON:
		ret = unpack_anon_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		break;
#endif
	case GNUTLS_CRD_CERTIFICATE:
		ret = unpack_certificate_auth_info(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
		break;
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto error;

	}

	/* Auth_info structures copied. Now copy security_parameters_st. 
	 * packed_session must have allocated space for the security parameters.
	 */
	ret = unpack_security_parameters(session, &sb);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (session->internals.resumed_security_parameters.pversion->tls13_sem) {
		/* 'prf' will not be NULL at this point, else unpack_security_parameters() would have failed */
		ret = tls13_unpack_security_parameters(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	}

	if (!session->internals.resumed_security_parameters.pversion->tls13_sem) {
		ret = _gnutls_hello_ext_unpack(session, &sb);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	}

	ret = 0;

      error:
	_gnutls_buffer_clear(&sb);

	return ret;
}

/*
 * If we're using TLS 1.3 semantics, we might have TLS 1.3-specific data.
 * Format:
 *      4 bytes the total length
 *      4 bytes the ticket lifetime
 *      4 bytes the ticket age add value
 *      1 byte the ticket nonce length
 *      x bytes the ticket nonce
 *      4 bytes the ticket length
 *      x bytes the ticket
 *      1 bytes the resumption master secret length
 *      x bytes the resumption master secret
 *     12 bytes the ticket arrival time
 *      4 bytes the max early data size
 *
 * We only store that info if we received a TLS 1.3 NewSessionTicket at some point.
 * If we didn't receive any NST then we cannot resume a TLS 1.3 session and hence
 * its nonsense to store all that info.
 */
static int
tls13_pack_security_parameters(gnutls_session_t session, gnutls_buffer_st *ps)
{
	int ret = 0;
	uint32_t length = 0;
	size_t length_pos;
	tls13_ticket_st *ticket = &session->internals.tls13_ticket;

	length_pos = ps->length;
	BUFFER_APPEND_NUM(ps, 0);

	if (ticket->ticket.data != NULL) {
		BUFFER_APPEND_NUM(ps, ticket->lifetime);
		length += 4;
		BUFFER_APPEND_NUM(ps, ticket->age_add);
		length += 4;
		BUFFER_APPEND_PFX1(ps,
				   ticket->nonce,
				   ticket->nonce_size);
		length += (1 + ticket->nonce_size);
		BUFFER_APPEND_PFX4(ps,
				   ticket->ticket.data,
				   ticket->ticket.size);
		length += (4 + ticket->ticket.size);
		BUFFER_APPEND_PFX1(ps,
				   ticket->resumption_master_secret,
				   ticket->prf->output_size);
		length += (1 + ticket->prf->output_size);
		BUFFER_APPEND_TS(ps, ticket->arrival_time);
		length += 12;
		BUFFER_APPEND_NUM(ps,
				  session->security_parameters.
				  max_early_data_size);
		length += 4;

		/* Overwrite the length field */
		_gnutls_write_uint32(length, ps->data + length_pos);
	}

	return ret;
}

static int
tls13_unpack_security_parameters(gnutls_session_t session, gnutls_buffer_st *ps)
{
	uint32_t ttl_len;
	tls13_ticket_st *ticket = &session->internals.tls13_ticket;
	gnutls_datum_t t;
	int ret = 0;

	BUFFER_POP_NUM(ps, ttl_len);

	if (ttl_len > 0) {
		BUFFER_POP_NUM(ps, ticket->lifetime);
		BUFFER_POP_NUM(ps, ticket->age_add);

		ret = _gnutls_buffer_pop_datum_prefix8(ps, &t);
		if (ret < 0 || t.size > sizeof(ticket->nonce)) {
			    ret = GNUTLS_E_PARSING_ERROR;
			    gnutls_assert();
			    goto error;
		}
		ticket->nonce_size = t.size;
		memcpy(ticket->nonce, t.data, t.size);

		BUFFER_POP_DATUM(ps, &ticket->ticket);

		ret = _gnutls_buffer_pop_datum_prefix8(ps, &t);
		if (ret < 0 || t.size > sizeof(ticket->resumption_master_secret)) {
			    ret = GNUTLS_E_PARSING_ERROR;
			    gnutls_assert();
			    goto error;
		}
		memcpy(ticket->resumption_master_secret, t.data, t.size);

		if (unlikely(session->internals.resumed_security_parameters.prf == NULL ||
		    session->internals.resumed_security_parameters.prf->output_size != t.size))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		ticket->prf = session->internals.resumed_security_parameters.prf;

		BUFFER_POP_TS(ps, ticket->arrival_time);
		BUFFER_POP_NUM(ps,
			       session->security_parameters.
			       max_early_data_size);
	}

error:
	return ret;
}

/* Format: 
 *      1 byte the credentials type
 *      4 bytes the size of the whole structure
 *	DH stuff
 *      2 bytes the size of secret key in bits
 *      4 bytes the size of the prime
 *      x bytes the prime
 *      4 bytes the size of the generator
 *      x bytes the generator
 *      4 bytes the size of the public key
 *      x bytes the public key
 *	RSA stuff
 *      4 bytes the size of the modulus
 *      x bytes the modulus
 *      4 bytes the size of the exponent
 *      x bytes the exponent
 *	CERTIFICATES
 *      4 bytes the length of the certificate list
 *      4 bytes the size of first certificate
 *      x bytes the certificate
 *       and so on...
 */
static int
pack_certificate_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	unsigned int i;
	int cur_size, ret;
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	int size_offset;

	size_offset = ps->length;
	BUFFER_APPEND_NUM(ps, 0);
	cur_size = ps->length;

	if (info) {

		BUFFER_APPEND_NUM(ps, info->dh.secret_bits);
		BUFFER_APPEND_PFX4(ps, info->dh.prime.data,
				   info->dh.prime.size);
		BUFFER_APPEND_PFX4(ps, info->dh.generator.data,
				   info->dh.generator.size);
		BUFFER_APPEND_PFX4(ps, info->dh.public_key.data,
				   info->dh.public_key.size);

		BUFFER_APPEND_NUM(ps, info->ncerts);

		for (i = 0; i < info->ncerts; i++) {
			BUFFER_APPEND_PFX4(ps,
					   info->raw_certificate_list[i].
					   data,
					   info->raw_certificate_list[i].
					   size);
		}

		BUFFER_APPEND_NUM(ps, info->nocsp);

		for (i = 0; i < info->nocsp; i++) {
			BUFFER_APPEND_PFX4(ps,
					   info->raw_ocsp_list[i].
					   data,
					   info->raw_ocsp_list[i].
					   size);
		}
	}

	/* write the real size */
	_gnutls_write_uint32(ps->length - cur_size,
			     ps->data + size_offset);

	return 0;
}


/* Upack certificate info.
 */
static int
unpack_certificate_auth_info(gnutls_session_t session,
			     gnutls_buffer_st * ps)
{
	int ret;
	unsigned int i = 0, j = 0;
	size_t pack_size;
	cert_auth_info_t info = NULL;
	unsigned cur_ncerts = 0;
	unsigned cur_nocsp = 0;

	BUFFER_POP_NUM(ps, pack_size);

	if (pack_size == 0)
		return 0;	/* nothing to be done */

	/* client and server have the same auth_info here
	 */
	ret =
	    _gnutls_auth_info_init(session, GNUTLS_CRD_CERTIFICATE,
				  sizeof(cert_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	BUFFER_POP_NUM(ps, info->dh.secret_bits);

	BUFFER_POP_DATUM(ps, &info->dh.prime);
	BUFFER_POP_DATUM(ps, &info->dh.generator);
	BUFFER_POP_DATUM(ps, &info->dh.public_key);

	BUFFER_POP_NUM(ps, info->ncerts);

	if (info->ncerts > 0) {
		info->raw_certificate_list =
		    gnutls_calloc(info->ncerts, sizeof(gnutls_datum_t));
		if (info->raw_certificate_list == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}
	}

	for (i = 0; i < info->ncerts; i++) {
		BUFFER_POP_DATUM(ps, &info->raw_certificate_list[i]);
		cur_ncerts++;
	}

	/* read OCSP responses */
	BUFFER_POP_NUM(ps, info->nocsp);

	if (info->nocsp > 0) {
		info->raw_ocsp_list =
		    gnutls_calloc(info->nocsp, sizeof(gnutls_datum_t));
		if (info->raw_ocsp_list == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}
	}

	for (i = 0; i < info->nocsp; i++) {
		BUFFER_POP_DATUM(ps, &info->raw_ocsp_list[i]);
		cur_nocsp++;
	}

	return 0;

      error:
	if (info) {
		_gnutls_free_datum(&info->dh.prime);
		_gnutls_free_datum(&info->dh.generator);
		_gnutls_free_datum(&info->dh.public_key);

		for (j = 0; j < cur_ncerts; j++)
			_gnutls_free_datum(&info->raw_certificate_list[j]);

		for (j = 0; j < cur_nocsp; j++)
			_gnutls_free_datum(&info->raw_ocsp_list[j]);

		gnutls_free(info->raw_certificate_list);
		gnutls_free(info->raw_ocsp_list);
	}

	return ret;

}

#ifdef ENABLE_SRP
/* Packs the SRP session authentication data.
 */

/* Format: 
 *      1 byte the credentials type
 *      4 bytes the size of the SRP username (x)
 *      x bytes the SRP username
 */
static int
pack_srp_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	srp_server_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_SRP);
	int len, ret;
	int size_offset;
	size_t cur_size;
	const char *username = NULL;

	if (info) {
		username = info->username;
		len = strlen(info->username) + 1;	/* include the terminating null */
	} else
		len = 0;

	size_offset = ps->length;
	BUFFER_APPEND_NUM(ps, 0);
	cur_size = ps->length;

	BUFFER_APPEND_PFX4(ps, username, len);

	/* write the real size */
	_gnutls_write_uint32(ps->length - cur_size,
			     ps->data + size_offset);

	return 0;
}


static int
unpack_srp_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	size_t username_size;
	int ret;
	srp_server_auth_info_t info;

	BUFFER_POP_NUM(ps, username_size);
	if (username_size > sizeof(info->username)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret =
	    _gnutls_auth_info_init(session, GNUTLS_CRD_SRP,
				  sizeof(srp_server_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_SRP);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	BUFFER_POP(ps, info->username, username_size);
	if (username_size == 0)
		info->username[0] = 0;

	ret = 0;

      error:
	return ret;
}
#endif


#ifdef ENABLE_ANON
/* Packs the ANON session authentication data.
 */

/* Format: 
 *      1 byte the credentials type
 *      4 bytes the size of the whole structure
 *      2 bytes the size of secret key in bits
 *      4 bytes the size of the prime
 *      x bytes the prime
 *      4 bytes the size of the generator
 *      x bytes the generator
 *      4 bytes the size of the public key
 *      x bytes the public key
 */
static int
pack_anon_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	int cur_size, ret;
	anon_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
	int size_offset;

	size_offset = ps->length;
	BUFFER_APPEND_NUM(ps, 0);
	cur_size = ps->length;

	if (info) {
		BUFFER_APPEND_NUM(ps, info->dh.secret_bits);
		BUFFER_APPEND_PFX4(ps, info->dh.prime.data,
				   info->dh.prime.size);
		BUFFER_APPEND_PFX4(ps, info->dh.generator.data,
				   info->dh.generator.size);
		BUFFER_APPEND_PFX4(ps, info->dh.public_key.data,
				   info->dh.public_key.size);
	}

	/* write the real size */
	_gnutls_write_uint32(ps->length - cur_size,
			     ps->data + size_offset);

	return 0;
}


static int
unpack_anon_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	int ret;
	size_t pack_size;
	anon_auth_info_t info = NULL;

	BUFFER_POP_NUM(ps, pack_size);

	if (pack_size == 0)
		return 0;	/* nothing to be done */

	/* client and server have the same auth_info here
	 */
	ret =
	    _gnutls_auth_info_init(session, GNUTLS_CRD_ANON,
				  sizeof(anon_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	BUFFER_POP_NUM(ps, info->dh.secret_bits);

	BUFFER_POP_DATUM(ps, &info->dh.prime);
	BUFFER_POP_DATUM(ps, &info->dh.generator);
	BUFFER_POP_DATUM(ps, &info->dh.public_key);

	return 0;

      error:
	if (info) {
		_gnutls_free_datum(&info->dh.prime);
		_gnutls_free_datum(&info->dh.generator);
		_gnutls_free_datum(&info->dh.public_key);
	}

	return ret;
}
#endif				/* ANON */

#ifdef ENABLE_PSK
/* Packs the PSK session authentication data.
 */

/* Format: 
 *      1 byte the credentials type
 *      4 bytes the size of the whole structure
 *
 *      4 bytes the size of the PSK username (x)
 *      x bytes the PSK username
 *      2 bytes the size of secret key in bits
 *      4 bytes the size of the prime
 *      x bytes the prime
 *      4 bytes the size of the generator
 *      x bytes the generator
 *      4 bytes the size of the public key
 *      x bytes the public key
 */
static int
pack_psk_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	psk_auth_info_t info;
	int username_len;
	int hint_len, ret;
	int size_offset;
	size_t cur_size;

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	username_len = strlen(info->username) + 1;	/* include the terminating null */
	hint_len = strlen(info->hint) + 1;	/* include the terminating null */

	size_offset = ps->length;
	BUFFER_APPEND_NUM(ps, 0);
	cur_size = ps->length;

	BUFFER_APPEND_PFX4(ps, info->username, username_len);
	BUFFER_APPEND_PFX4(ps, info->hint, hint_len);

	BUFFER_APPEND_NUM(ps, info->dh.secret_bits);
	BUFFER_APPEND_PFX4(ps, info->dh.prime.data, info->dh.prime.size);
	BUFFER_APPEND_PFX4(ps, info->dh.generator.data,
			   info->dh.generator.size);
	BUFFER_APPEND_PFX4(ps, info->dh.public_key.data,
			   info->dh.public_key.size);

	/* write the real size */
	_gnutls_write_uint32(ps->length - cur_size,
			     ps->data + size_offset);
	return 0;
}

static int
unpack_psk_auth_info(gnutls_session_t session, gnutls_buffer_st * ps)
{
	size_t username_size, hint_size;
	int ret;
	psk_auth_info_t info;
	unsigned pack_size;

	ret =
	    _gnutls_auth_info_init(session, GNUTLS_CRD_PSK,
				  sizeof(psk_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	BUFFER_POP_NUM(ps, pack_size);
	if (pack_size == 0)
		return GNUTLS_E_INVALID_REQUEST;

	BUFFER_POP_NUM(ps, username_size);
	if (username_size > sizeof(info->username)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	BUFFER_POP(ps, info->username, username_size);
	if (username_size == 0)
		info->username[0] = 0;

	BUFFER_POP_NUM(ps, hint_size);
	if (hint_size > sizeof(info->hint)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}
	BUFFER_POP(ps, info->hint, hint_size);
	if (hint_size == 0)
		info->hint[0] = 0;

	BUFFER_POP_NUM(ps, info->dh.secret_bits);

	BUFFER_POP_DATUM(ps, &info->dh.prime);
	BUFFER_POP_DATUM(ps, &info->dh.generator);
	BUFFER_POP_DATUM(ps, &info->dh.public_key);

	ret = 0;

      error:
	_gnutls_free_datum(&info->dh.prime);
	_gnutls_free_datum(&info->dh.generator);
	_gnutls_free_datum(&info->dh.public_key);

	return ret;
}
#endif


/* Packs the security parameters.
 */
static int
pack_security_parameters(gnutls_session_t session, gnutls_buffer_st * ps)
{

	int ret;
	int size_offset;
	size_t cur_size;

	if (session->security_parameters.epoch_read
	    != session->security_parameters.epoch_write &&
	    !(session->internals.hsk_flags & HSK_EARLY_START_USED)) {
		gnutls_assert();
		return GNUTLS_E_UNAVAILABLE_DURING_HANDSHAKE;
	}

	ret = _gnutls_epoch_get(session, EPOCH_READ_CURRENT, NULL);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* move after the auth info stuff.
	 */
	size_offset = ps->length;
	BUFFER_APPEND_NUM(ps, 0);
	cur_size = ps->length;

	BUFFER_APPEND_NUM(ps, session->security_parameters.entity);
	BUFFER_APPEND_NUM(ps, session->security_parameters.prf->id);

	BUFFER_APPEND_NUM(ps,
			  session->security_parameters.client_auth_type);
	BUFFER_APPEND_NUM(ps,
			  session->security_parameters.server_auth_type);

	BUFFER_APPEND(ps, &session->security_parameters.session_id_size,
		      1);
	BUFFER_APPEND(ps, session->security_parameters.session_id,
		      session->security_parameters.session_id_size);

	BUFFER_APPEND_NUM(ps, session->security_parameters.pversion->id);

	BUFFER_APPEND_NUM(ps, session->security_parameters.client_ctype);
	BUFFER_APPEND_NUM(ps, session->security_parameters.server_ctype);

	/* if we are under TLS 1.3 do not pack keys or params negotiated using an extension
	 * they are not necessary */
	if (!session->security_parameters.pversion->tls13_sem) {
		BUFFER_APPEND(ps, session->security_parameters.cs->id, 2);

		BUFFER_APPEND_PFX1(ps, session->security_parameters.master_secret,
			      GNUTLS_MASTER_SIZE);
		BUFFER_APPEND_PFX1(ps, session->security_parameters.client_random,
			      GNUTLS_RANDOM_SIZE);
		BUFFER_APPEND_PFX1(ps, session->security_parameters.server_random,
			      GNUTLS_RANDOM_SIZE);

		/* reset max_record_recv_size if it was negotiated
		 * using the record_size_limit extension */
		if (session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_NEGOTIATED) {
			BUFFER_APPEND_NUM(ps,
					  session->security_parameters.
					  max_user_record_send_size);
			BUFFER_APPEND_NUM(ps,
					  session->security_parameters.
					  max_user_record_recv_size);
		} else {
			BUFFER_APPEND_NUM(ps,
					  session->security_parameters.
					  max_record_recv_size);
			BUFFER_APPEND_NUM(ps,
					  session->security_parameters.
					  max_record_send_size);
		}

		if (session->security_parameters.grp) {
			BUFFER_APPEND_NUM(ps, session->security_parameters.grp->id);
		} else {
			BUFFER_APPEND_NUM(ps, 0);
		}

		BUFFER_APPEND_NUM(ps,
				  session->security_parameters.server_sign_algo);
		BUFFER_APPEND_NUM(ps,
				  session->security_parameters.client_sign_algo);
		BUFFER_APPEND_NUM(ps,
				  session->security_parameters.ext_master_secret);
		BUFFER_APPEND_NUM(ps,
				  session->security_parameters.etm);
	}


	_gnutls_write_uint32(ps->length - cur_size,
			     ps->data + size_offset);

	return 0;
}

static int
unpack_security_parameters(gnutls_session_t session, gnutls_buffer_st * ps)
{
	size_t pack_size;
	int ret;
	unsigned version;
	gnutls_datum_t t;
	time_t timestamp;
	uint8_t cs[2];

	BUFFER_POP_NUM(ps, pack_size);

	if (pack_size == 0)
		return GNUTLS_E_INVALID_REQUEST;

	timestamp =
	    session->internals.resumed_security_parameters.timestamp;
	memset(&session->internals.resumed_security_parameters, 0,
	       sizeof(session->internals.resumed_security_parameters));
	session->internals.resumed_security_parameters.timestamp =
	    timestamp;

	BUFFER_POP_NUM(ps,
		       session->internals.resumed_security_parameters.
		       entity);

	BUFFER_POP_NUM(ps, version);
	session->internals.resumed_security_parameters.prf = mac_to_entry(version);
	if (session->internals.resumed_security_parameters.prf == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	BUFFER_POP_NUM(ps,
		       session->internals.resumed_security_parameters.
		       client_auth_type);
	BUFFER_POP_NUM(ps,
		       session->internals.resumed_security_parameters.
		       server_auth_type);

	BUFFER_POP(ps,
		   &session->internals.resumed_security_parameters.
		   session_id_size, 1);

	BUFFER_POP(ps,
		   session->internals.resumed_security_parameters.
		   session_id,
		   session->internals.resumed_security_parameters.
		   session_id_size);

	BUFFER_POP_NUM(ps, version);
	session->internals.resumed_security_parameters.pversion =
	    version_to_entry(version);
	if (session->internals.resumed_security_parameters.pversion ==
	    NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	BUFFER_POP_NUM(ps,
		       session->internals.resumed_security_parameters.
		       client_ctype);
	BUFFER_POP_NUM(ps,
		       session->internals.resumed_security_parameters.
		       server_ctype);

	if (!session->internals.resumed_security_parameters.pversion->tls13_sem) {
		BUFFER_POP(ps, cs, 2);
		session->internals.resumed_security_parameters.cs = ciphersuite_to_entry(cs);
		if (session->internals.resumed_security_parameters.cs == NULL)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		/* master secret */
		ret = _gnutls_buffer_pop_datum_prefix8(ps, &t);
		if (ret < 0) {
			ret = GNUTLS_E_PARSING_ERROR;
			gnutls_assert();
			goto error;
		}
		if (t.size == GNUTLS_MASTER_SIZE)
			memcpy(session->internals.resumed_security_parameters.master_secret, t.data, t.size);

		/* client random */
		ret = _gnutls_buffer_pop_datum_prefix8(ps, &t);
		if (ret < 0) {
			ret = GNUTLS_E_PARSING_ERROR;
			gnutls_assert();
			goto error;
		}
		if (t.size == GNUTLS_RANDOM_SIZE)
			memcpy(session->internals.resumed_security_parameters.client_random, t.data, t.size);

		/* server random */
		ret = _gnutls_buffer_pop_datum_prefix8(ps, &t);
		if (ret < 0) {
			ret = GNUTLS_E_PARSING_ERROR;
			gnutls_assert();
			goto error;
		}
		if (t.size == GNUTLS_RANDOM_SIZE)
			memcpy(session->internals.resumed_security_parameters.server_random, t.data, t.size);


		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       max_record_send_size);
		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       max_record_recv_size);

		BUFFER_POP_NUM(ps, ret);
		session->internals.resumed_security_parameters.grp = _gnutls_id_to_group(ret);
		/* it can be null */

		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       server_sign_algo);
		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       client_sign_algo);
		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       ext_master_secret);
		BUFFER_POP_NUM(ps,
			       session->internals.resumed_security_parameters.
			       etm);

		if (session->internals.resumed_security_parameters.
		    max_record_recv_size == 0
		    || session->internals.resumed_security_parameters.
		    max_record_send_size == 0) {
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		}
	}

	ret = 0;

      error:
	return ret;
}

/**
 * gnutls_session_set_premaster:
 * @session: is a #gnutls_session_t type.
 * @entity: GNUTLS_SERVER or GNUTLS_CLIENT
 * @version: the TLS protocol version
 * @kx: the key exchange method
 * @cipher: the cipher
 * @mac: the MAC algorithm
 * @comp: the compression method (ignored)
 * @master: the master key to use
 * @session_id: the session identifier
 *
 * This function sets the premaster secret in a session. This is
 * a function intended for exceptional uses. Do not use this
 * function unless you are implementing a legacy protocol.
 * Use gnutls_session_set_data() instead.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_set_premaster(gnutls_session_t session, unsigned int entity,
			     gnutls_protocol_t version,
			     gnutls_kx_algorithm_t kx,
			     gnutls_cipher_algorithm_t cipher,
			     gnutls_mac_algorithm_t mac,
			     gnutls_compression_method_t comp,
			     const gnutls_datum_t * master,
			     const gnutls_datum_t * session_id)
{
	int ret;
	uint8_t cs[2];

	memset(&session->internals.resumed_security_parameters, 0,
	       sizeof(session->internals.resumed_security_parameters));

	session->internals.resumed_security_parameters.entity = entity;

	ret =
	    _gnutls_cipher_suite_get_id(kx, cipher, mac, cs);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.resumed_security_parameters.cs = ciphersuite_to_entry(cs);
	if (session->internals.resumed_security_parameters.cs == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	session->internals.resumed_security_parameters.client_ctype =
			DEFAULT_CERT_TYPE;
	session->internals.resumed_security_parameters.server_ctype =
			DEFAULT_CERT_TYPE;
	session->internals.resumed_security_parameters.pversion =
	    version_to_entry(version);
	if (session->internals.resumed_security_parameters.pversion ==
	    NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (session->internals.resumed_security_parameters.pversion->selectable_prf)
		session->internals.resumed_security_parameters.prf = mac_to_entry(session->internals.resumed_security_parameters.cs->prf);
	else
		session->internals.resumed_security_parameters.prf = mac_to_entry(GNUTLS_MAC_MD5_SHA1);
	if (session->internals.resumed_security_parameters.prf == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (master->size != GNUTLS_MASTER_SIZE)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(session->internals.resumed_security_parameters.
	       master_secret, master->data, master->size);

	if (session_id->size > GNUTLS_MAX_SESSION_ID)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	session->internals.resumed_security_parameters.session_id_size =
	    session_id->size;
	memcpy(session->internals.resumed_security_parameters.session_id,
	       session_id->data, session_id->size);

	session->internals.resumed_security_parameters.
	    max_record_send_size =
	    session->internals.resumed_security_parameters.
	    max_record_recv_size = DEFAULT_MAX_RECORD_SIZE;

	session->internals.resumed_security_parameters.timestamp =
	    gnutls_time(0);

	session->internals.resumed_security_parameters.grp = 0;

	session->internals.resumed_security_parameters.post_handshake_auth = 0;

	session->internals.premaster_set = 1;

	return 0;
}
