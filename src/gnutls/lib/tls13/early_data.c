/*
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

#include "gnutls_int.h"
#include "handshake.h"
#include "tls13/early_data.h"

int _gnutls13_send_early_data(gnutls_session_t session)
{
	int ret;

	if (!(session->security_parameters.entity == GNUTLS_CLIENT &&
	      session->internals.hsk_flags & HSK_EARLY_DATA_IN_FLIGHT))
		return 0;

	while (session->internals.early_data_presend_buffer.length > 0) {
		ret =
			gnutls_record_send(session,
					   session->internals.
					   early_data_presend_buffer.data,
					   session->internals.
					   early_data_presend_buffer.
					   length);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.early_data_presend_buffer.data += ret;
		session->internals.early_data_presend_buffer.length -= ret;
	}


	return 0;
}

int _gnutls13_send_end_of_early_data(gnutls_session_t session, unsigned again)
{
	int ret;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;

	if (!(session->security_parameters.entity == GNUTLS_CLIENT &&
	      session->internals.hsk_flags & HSK_EARLY_DATA_ACCEPTED))
		return 0;

	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_END_OF_EARLY_DATA);
}

int _gnutls13_recv_end_of_early_data(gnutls_session_t session)
{
	int ret;
	gnutls_buffer_st buf;

	if (!(session->security_parameters.entity == GNUTLS_SERVER &&
	      session->internals.hsk_flags & HSK_EARLY_DATA_ACCEPTED))
		return 0;

	ret = _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_END_OF_EARLY_DATA, 0, &buf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (buf.length != 0) {
		gnutls_assert();
		ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
		goto cleanup;
	}

	session->internals.hsk_flags &= ~HSK_EARLY_DATA_IN_FLIGHT;

	ret = 0;
cleanup:

	_gnutls_buffer_clear(&buf);
	return ret;
}
