/*
 * Copyright (C) 2017 Free Software Foundation, Inc.
 *
 * Author: Ander Juaristi
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

#include "gnutls_int.h"
#include "ext/psk_ke_modes.h"
#include "ext/pre_shared_key.h"
#include <assert.h>

#define PSK_KE 0
#define PSK_DHE_KE 1

static int
psk_ke_modes_send_params(gnutls_session_t session,
			 gnutls_buffer_t extdata)
{
	int ret;
	const version_entry_st *vers;
	uint8_t data[2];
	unsigned pos, i;
	unsigned have_dhpsk = 0;
	unsigned have_psk = 0;

	/* Server doesn't send psk_key_exchange_modes */
	if (session->security_parameters.entity == GNUTLS_SERVER)
		return 0;

	/* If session ticket is disabled and no PSK key exchange is
	 * enabled, don't send the extension */
	if ((session->internals.flags & GNUTLS_NO_TICKETS) &&
	    !session->internals.priorities->have_psk)
		return 0;

	vers = _gnutls_version_max(session);
	if (!vers || !vers->tls13_sem)
		return 0;

	/* We send the list prioritized according to our preferences as a convention
	 * (used throughout the protocol), even if the protocol doesn't mandate that
	 * for this particular message. That way we can keep the TLS 1.2 semantics/
	 * prioritization when negotiating PSK or DHE-PSK. Receiving servers would
	 * very likely respect our prioritization if they parse the message serially. */
	pos = 0;
	for (i=0;i<session->internals.priorities->_kx.num_priorities;i++) {
		if (session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_PSK && !have_psk) {
			assert(pos <= 1);
			data[pos++] = PSK_KE;
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_PSK;
			have_psk = 1;
		} else if ((session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_DHE_PSK ||
			    session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_ECDHE_PSK) && !have_dhpsk) {
			assert(pos <= 1);
			data[pos++] = PSK_DHE_KE;
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_DHE_PSK;
			have_dhpsk = 1;
		}

		if (have_psk && have_dhpsk)
			break;
	}

	/* For session resumption we need to send at least one */
	if (pos == 0) {
		if (session->internals.flags & GNUTLS_NO_TICKETS)
			return 0;

		data[pos++] = PSK_DHE_KE;
		data[pos++] = PSK_KE;
		session->internals.hsk_flags |= HSK_PSK_KE_MODE_DHE_PSK;
		session->internals.hsk_flags |= HSK_PSK_KE_MODE_PSK;
	}

	ret = _gnutls_buffer_append_data_prefix(extdata, 8, data, pos);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.hsk_flags |= HSK_PSK_KE_MODES_SENT;

	return 0;
}

#define MAX_POS INT_MAX

/*
 * Since we only support ECDHE-authenticated PSKs, the server
 * just verifies that a "psk_key_exchange_modes" extension was received,
 * and that it contains the value one.
 */
static int
psk_ke_modes_recv_params(gnutls_session_t session,
			 const unsigned char *data, size_t len)
{
	uint8_t ke_modes_len;
	const version_entry_st *vers = get_version(session);
	gnutls_psk_server_credentials_t cred;
	int dhpsk_pos = MAX_POS;
	int psk_pos = MAX_POS;
	int cli_psk_pos = MAX_POS;
	int cli_dhpsk_pos = MAX_POS;
	unsigned i;

	/* Client doesn't receive psk_key_exchange_modes */
	if (session->security_parameters.entity == GNUTLS_CLIENT)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

	/* we set hsk_flags to HSK_PSK_KE_MODE_INVALID on failure to ensure that
	 * when we parse the pre-shared key extension we detect PSK_KE_MODES as
	 * received. */
	if (!vers || !vers->tls13_sem) {
		session->internals.hsk_flags |= HSK_PSK_KE_MODE_INVALID;
		return gnutls_assert_val(0);
	}

	cred = (gnutls_psk_server_credentials_t)_gnutls_get_cred(session, GNUTLS_CRD_PSK);
	if (cred == NULL && (session->internals.flags & GNUTLS_NO_TICKETS)) {
		session->internals.hsk_flags |= HSK_PSK_KE_MODE_INVALID;
		return gnutls_assert_val(0);
	}

	DECR_LEN(len, 1);
	ke_modes_len = *(data++);

	for (i=0;i<session->internals.priorities->_kx.num_priorities;i++) {
		if (session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_PSK && psk_pos == MAX_POS) {
			psk_pos = i;
		} else if ((session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_DHE_PSK ||
			    session->internals.priorities->_kx.priorities[i] == GNUTLS_KX_ECDHE_PSK) &&
			    dhpsk_pos == MAX_POS) {
			dhpsk_pos = i;
		}

		if (dhpsk_pos != MAX_POS && psk_pos != MAX_POS)
			break;
	}

	if (psk_pos == MAX_POS && dhpsk_pos == MAX_POS) {
		if (!(session->internals.flags & GNUTLS_NO_TICKETS))
			dhpsk_pos = 0;
		else if (session->internals.priorities->groups.size == 0)
			return gnutls_assert_val(0);
	}

	for (i=0;i<ke_modes_len;i++) {
		DECR_LEN(len, 1);
		if (data[i] == PSK_DHE_KE)
			cli_dhpsk_pos = i;
		else if (data[i] == PSK_KE)
			cli_psk_pos = i;

		_gnutls_handshake_log("EXT[%p]: PSK KE mode %.2x received\n",
				      session, (unsigned)data[i]);
		if (cli_psk_pos != MAX_POS && cli_dhpsk_pos != MAX_POS)
			break;
	}

	if (session->internals.priorities->server_precedence) {
		if (dhpsk_pos != MAX_POS && cli_dhpsk_pos != MAX_POS && dhpsk_pos < psk_pos)
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_DHE_PSK;
		else if (psk_pos != MAX_POS && cli_psk_pos != MAX_POS && psk_pos < dhpsk_pos)
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_PSK;
	} else {
		if (dhpsk_pos != MAX_POS && cli_dhpsk_pos != MAX_POS && cli_dhpsk_pos < cli_psk_pos)
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_DHE_PSK;
		else if (psk_pos != MAX_POS && cli_psk_pos != MAX_POS && cli_psk_pos < cli_dhpsk_pos)
			session->internals.hsk_flags |= HSK_PSK_KE_MODE_PSK;
	}

	if ((session->internals.hsk_flags & HSK_PSK_KE_MODE_PSK) ||
	    (session->internals.hsk_flags & HSK_PSK_KE_MODE_DHE_PSK)) {

		return 0;
	} else {
		session->internals.hsk_flags |= HSK_PSK_KE_MODE_INVALID;
		return gnutls_assert_val(0);
	}
}

const hello_ext_entry_st ext_mod_psk_ke_modes = {
	.name = "PSK Key Exchange Modes",
	.tls_id = 45,
	.gid = GNUTLS_EXTENSION_PSK_KE_MODES,
	.client_parse_point = GNUTLS_EXT_TLS,
	.server_parse_point = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO,
	.send_func = psk_ke_modes_send_params,
	.recv_func = psk_ke_modes_recv_params
};
