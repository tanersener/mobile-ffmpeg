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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_CONSTATE_H
#define GNUTLS_LIB_CONSTATE_H

int _gnutls_set_cipher_suite2(gnutls_session_t session,
			     const gnutls_cipher_suite_entry_st *cs);

int _gnutls_epoch_set_keys(gnutls_session_t session, uint16_t epoch, hs_stage_t stage);
int _gnutls_connection_state_init(gnutls_session_t session);
int _gnutls_read_connection_state_init(gnutls_session_t session);
int _gnutls_write_connection_state_init(gnutls_session_t session);

#define _gnutls_epoch_bump(session) \
	(session)->security_parameters.epoch_next++

int _gnutls_epoch_dup(gnutls_session_t session, unsigned int epoch_rel);

int _gnutls_epoch_get(gnutls_session_t session, unsigned int epoch_rel,
		      record_parameters_st ** params_out);
int _gnutls_epoch_setup_next(gnutls_session_t session, unsigned null_epoch, record_parameters_st **newp);
void _gnutls_epoch_gc(gnutls_session_t session);
void _gnutls_epoch_free(gnutls_session_t session,
			record_parameters_st * state);

void _gnutls_set_resumed_parameters(gnutls_session_t session);

int _tls13_connection_state_init(gnutls_session_t session, hs_stage_t stage);
int _tls13_read_connection_state_init(gnutls_session_t session, hs_stage_t stage);
int _tls13_write_connection_state_init(gnutls_session_t session, hs_stage_t stage);

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

#endif /* GNUTLS_LIB_CONSTATE_H */
