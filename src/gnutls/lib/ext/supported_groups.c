/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* This file contains the code for the Supported Groups extension (rfc7919).
 * This extension was previously named Supported Elliptic Curves under TLS 1.2.
 */

#include "ext/supported_groups.h"
#include "str.h"
#include "num.h"
#include "auth/psk.h"
#include "auth/cert.h"
#include "auth/anon.h"
#include "algorithms.h"
#include <gnutls/gnutls.h>


static int _gnutls_supported_groups_recv_params(gnutls_session_t session,
					     const uint8_t * data,
					     size_t data_size);
static int _gnutls_supported_groups_send_params(gnutls_session_t session,
					     gnutls_buffer_st * extdata);


const hello_ext_entry_st ext_mod_supported_groups = {
	.name = "Supported Groups",
	.tls_id = 10,
	.gid = GNUTLS_EXTENSION_SUPPORTED_GROUPS,
	.client_parse_point = GNUTLS_EXT_TLS,
	.server_parse_point = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_supported_groups_recv_params,
	.send_func = _gnutls_supported_groups_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL,
	.cannot_be_overriden = 1
};


static unsigned get_min_dh(gnutls_session_t session)
{
	gnutls_certificate_credentials_t cert_cred;
	gnutls_psk_server_credentials_t psk_cred;
	gnutls_anon_server_credentials_t anon_cred;
	unsigned level = 0;

	cert_cred = (gnutls_certificate_credentials_t)_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	psk_cred = (gnutls_psk_server_credentials_t)_gnutls_get_cred(session, GNUTLS_CRD_PSK);
	anon_cred = (gnutls_anon_server_credentials_t)_gnutls_get_cred(session, GNUTLS_CRD_ANON);

	if (cert_cred) {
		level = cert_cred->dh_sec_param;
	} else if (psk_cred) {
		level = psk_cred->dh_sec_param;
	} else if (anon_cred) {
		level = anon_cred->dh_sec_param;
	}

	if (level)
		return gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, level);

	return 0;
}

/*
 * In case of a server: if a SUPPORTED_GROUPS extension type is received then it stores
 * into the session security parameters the new value. The server may use gnutls_session_certificate_type_get(),
 * to access it.
 *
 * In case of a client: If supported_eccs have been specified then we send the extension.
 *
 */
static int
_gnutls_supported_groups_recv_params(gnutls_session_t session,
				  const uint8_t * data, size_t data_size)
{
	int i;
	uint16_t len;
	const uint8_t *p = data;
	const gnutls_group_entry_st *group = NULL;
	unsigned have_ffdhe = 0;
	unsigned tls_id;
	unsigned min_dh;
	unsigned j;
	int serv_ec_idx, serv_dh_idx; /* index in server's priority listing */
	int cli_ec_pos, cli_dh_pos; /* position in listing sent by client */

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		/* A client shouldn't receive this extension in TLS1.2. It is
		 * possible to read that message under TLS1.3 as an encrypted
		 * extension. */
		return 0;
	} else {		/* SERVER SIDE - we must check if the sent supported ecc type is the right one
				 */
		if (data_size < 2)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		DECR_LEN(data_size, 2);
		len = _gnutls_read_uint16(p);
		p += 2;

		if (len % 2 != 0)
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		DECR_LEN(data_size, len);

		/* we figure what is the minimum DH allowed for this session, if any */
		min_dh = get_min_dh(session);

		serv_ec_idx = serv_dh_idx = -1;
		cli_ec_pos = cli_dh_pos = -1;

		/* This extension is being processed prior to a ciphersuite being selected,
		 * so we cannot rely on ciphersuite information. */
		for (i = 0; i < len; i += 2) {
			if (have_ffdhe == 0 && p[i] == 0x01)
				have_ffdhe = 1;

			tls_id = _gnutls_read_uint16(&p[i]);
			group = _gnutls_tls_id_to_group(tls_id);

			_gnutls_handshake_log("EXT[%p]: Received group %s (0x%x)\n", session, group?group->name:"unknown", tls_id);
			if (group == NULL)
				continue;

			if (min_dh > 0 && group->prime && group->prime->size*8 < min_dh)
				continue;

			/* we simulate _gnutls_session_supports_group, but we prioritize if
			 * %SERVER_PRECEDENCE is given */
			for (j = 0; j < session->internals.priorities->groups.size; j++) {
				if (session->internals.priorities->groups.entry[j]->id == group->id) {
					if (session->internals.priorities->server_precedence) {
						if (group->pk == GNUTLS_PK_DH) {
							if (serv_dh_idx != -1 && (int)j > serv_dh_idx)
								break;
							serv_dh_idx = j;
							cli_dh_pos = i;
						} else if (IS_EC(group->pk)) {
							if (serv_ec_idx != -1 && (int)j > serv_ec_idx)
								break;
							serv_ec_idx = j;
							cli_ec_pos = i;
						}
					} else {
						if (group->pk == GNUTLS_PK_DH) {
							if (cli_dh_pos != -1)
								break;
							cli_dh_pos = i;
							serv_dh_idx = j;
						} else if (IS_EC(group->pk)) {
							if (cli_ec_pos != -1)
								break;
							cli_ec_pos = i;
							serv_ec_idx = j;
						}
					}
					break;
				}
			}
		}

		/* serv_dh/ec_pos contain the index of the groups we want to use.
		 */
		if (serv_dh_idx != -1) {
			session->internals.cand_dh_group = session->internals.priorities->groups.entry[serv_dh_idx];
			session->internals.cand_group = session->internals.cand_dh_group;
		}

		if (serv_ec_idx != -1) {
			session->internals.cand_ec_group = session->internals.priorities->groups.entry[serv_ec_idx];
			if (session->internals.cand_group == NULL ||
			    (session->internals.priorities->server_precedence && serv_ec_idx < serv_dh_idx) ||
			    (!session->internals.priorities->server_precedence && cli_ec_pos < cli_dh_pos)) {
				session->internals.cand_group = session->internals.cand_ec_group;
			}
		}

		if (session->internals.cand_group)
			_gnutls_handshake_log("EXT[%p]: Selected group %s\n", session, session->internals.cand_group->name);

		if (have_ffdhe)
			session->internals.hsk_flags |= HSK_HAVE_FFDHE;
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_supported_groups_send_params(gnutls_session_t session,
				  gnutls_buffer_st * extdata)
{
	unsigned len, i;
	int ret;
	uint16_t p;

	/* this extension is only being sent on client side */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {

		len = session->internals.priorities->groups.size;
		if (len > 0) {
			ret =
			    _gnutls_buffer_append_prefix(extdata, 16,
							 len * 2);
			if (ret < 0)
				return gnutls_assert_val(ret);

			for (i = 0; i < len; i++) {
				p = session->internals.priorities->groups.entry[i]->tls_id;

				_gnutls_handshake_log("EXT[%p]: Sent group %s (0x%x)\n", session,
					session->internals.priorities->groups.entry[i]->name, (unsigned)p);

				ret =
				    _gnutls_buffer_append_prefix(extdata,
								 16, p);
				if (ret < 0)
					return gnutls_assert_val(ret);
			}
			return (len + 1) * 2;
		}

	}

	return 0;
}

/* Returns 0 if the given ECC curve is allowed in the current
 * session. A negative error value is returned otherwise.
 */
int
_gnutls_session_supports_group(gnutls_session_t session,
				unsigned int group)
{
	unsigned i;

	for (i = 0; i < session->internals.priorities->groups.size; i++) {
		if (session->internals.priorities->groups.entry[i]->id == group)
			return 0;
	}

	return GNUTLS_E_ECC_UNSUPPORTED_CURVE;
}
