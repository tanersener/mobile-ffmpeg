/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_CONSTATE_H
#define GNUTLS_CONSTATE_H

int _gnutls_epoch_set_cipher_suite(gnutls_session_t session, int epoch_rel,
				   const uint8_t suite[2]);
int _gnutls_epoch_set_compression(gnutls_session_t session, int epoch_rel,
				  gnutls_compression_method_t comp_algo);
int _gnutls_epoch_get_compression(gnutls_session_t session, int epoch_rel);
void _gnutls_epoch_set_null_algos(gnutls_session_t session,
				  record_parameters_st * params);
int _gnutls_epoch_set_keys(gnutls_session_t session, uint16_t epoch);
int _gnutls_connection_state_init(gnutls_session_t session);
int _gnutls_read_connection_state_init(gnutls_session_t session);
int _gnutls_write_connection_state_init(gnutls_session_t session);

int _gnutls_epoch_get(gnutls_session_t session, unsigned int epoch_rel,
		      record_parameters_st ** params_out);
int _gnutls_epoch_alloc(gnutls_session_t session, uint16_t epoch,
			record_parameters_st ** out);
void _gnutls_epoch_gc(gnutls_session_t session);
void _gnutls_epoch_free(gnutls_session_t session,
			record_parameters_st * state);

static inline int _gnutls_epoch_is_valid(gnutls_session_t session,
					 int epoch)
{
	record_parameters_st *params;
	int ret;

	ret = _gnutls_epoch_get(session, epoch, &params);
	if (ret < 0)
		return 0;

	return 1;
}


static inline int _gnutls_epoch_refcount_inc(gnutls_session_t session,
					     int epoch)
{
	record_parameters_st *params;
	int ret;

	ret = _gnutls_epoch_get(session, epoch, &params);
	if (ret < 0)
		return ret;

	params->usage_cnt++;

	return params->epoch;
}

static inline int _gnutls_epoch_refcount_dec(gnutls_session_t session,
					     uint16_t epoch)
{
	record_parameters_st *params;
	int ret;

	ret = _gnutls_epoch_get(session, epoch, &params);
	if (ret < 0)
		return ret;

	params->usage_cnt--;
	if (params->usage_cnt < 0)
		return GNUTLS_E_INTERNAL_ERROR;

	return 0;
}

#endif
