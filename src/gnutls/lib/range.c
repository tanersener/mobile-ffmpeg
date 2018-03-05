/*
 * Copyright (C) 2012 INRIA Paris-Rocquencourt
 *
 * Author: Alfredo Pironti
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
#include "errors.h"
#include "algorithms.h"
#include "constate.h"
#include "record.h"

static void
_gnutls_set_range(gnutls_range_st * dst, const size_t low,
		  const size_t high)
{
	dst->low = low;
	dst->high = high;
	return;
}

/*
 * Returns how much LH pad we can put in this fragment, given we'll
 * put at least data_length bytes of user data.
 */
static ssize_t
_gnutls_range_max_lh_pad(gnutls_session_t session, ssize_t data_length,
			 ssize_t max_frag)
{
	int ret;
	ssize_t max_pad;
	unsigned int fixed_pad;
	record_parameters_st *record_params;
	ssize_t this_pad;
	ssize_t block_size;
	ssize_t tag_size, overflow;

	ret =
	    _gnutls_epoch_get(session, EPOCH_WRITE_CURRENT,
			      &record_params);
	if (ret < 0) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	max_pad = MAX_PAD_SIZE;
	fixed_pad = 1;

	this_pad = MIN(max_pad, max_frag - data_length);

	block_size = _gnutls_cipher_get_block_size(record_params->cipher);
	tag_size =
	    _gnutls_auth_cipher_tag_len(&record_params->write.
					cipher_state);
	switch (_gnutls_cipher_type(record_params->cipher)) {
	case CIPHER_AEAD:
	case CIPHER_STREAM:
		return this_pad;

	case CIPHER_BLOCK:
		overflow =
		    (data_length + this_pad + tag_size +
		     fixed_pad) % block_size;
		if (overflow > this_pad) {
			return this_pad;
		} else {
			return this_pad - overflow;
		}
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

/**
 * gnutls_record_can_use_length_hiding:
 * @session: is a #gnutls_session_t type.
 *
 * If the session supports length-hiding padding, you can
 * invoke gnutls_range_send_message() to send a message whose
 * length is hidden in the given range. If the session does not
 * support length hiding padding, you can use the standard
 * gnutls_record_send() function, or gnutls_range_send_message()
 * making sure that the range is the same as the length of the
 * message you are trying to send.
 *
 * Returns: true (1) if the current session supports length-hiding
 * padding, false (0) if the current session does not.
 **/
int gnutls_record_can_use_length_hiding(gnutls_session_t session)
{
	int ret;
	record_parameters_st *record_params;

#ifdef ENABLE_SSL3
	if (get_num_version(session) == GNUTLS_SSL3)
		return 0;
#endif

	ret =
	    _gnutls_epoch_get(session, EPOCH_WRITE_CURRENT,
			      &record_params);
	if (ret < 0) {
		return 0;
	}

	switch (_gnutls_cipher_type(record_params->cipher)) {
	case CIPHER_BLOCK:
		return 1;
	case CIPHER_STREAM:
	case CIPHER_AEAD:
	default:
		return 0;
	}
}

/**
 * gnutls_range_split:
 * @session: is a #gnutls_session_t type
 * @orig: is the original range provided by the user
 * @next: is the returned range that can be conveyed in a TLS record
 * @remainder: is the returned remaining range
 *
 * This function should be used when it is required to hide the length
 * of very long data that cannot be directly provided to gnutls_record_send_range().
 * In that case this function should be called with the desired length
 * hiding range in @orig. The returned @next value should then be used in
 * the next call to gnutls_record_send_range() with the partial data.
 * That process should be repeated until @remainder is (0,0).
 *
 * Returns: 0 in case splitting succeeds, non zero in case of error.
 * Note that @orig is not changed, while the values of @next
 * and @remainder are modified to store the resulting values.
 */
int
gnutls_range_split(gnutls_session_t session,
		   const gnutls_range_st * orig,
		   gnutls_range_st * next, gnutls_range_st * remainder)
{
	int ret;
	ssize_t max_frag;
	ssize_t orig_low = (ssize_t) orig->low;
	ssize_t orig_high = (ssize_t) orig->high;
	record_parameters_st *record_params;

	ret =
	    _gnutls_epoch_get(session, EPOCH_WRITE_CURRENT,
			      &record_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	max_frag = max_user_send_size(session, record_params);

	if (orig_high == orig_low) {
		int length = MIN(orig_high, max_frag);
		int rem = orig_high - length;
		_gnutls_set_range(next, length, length);
		_gnutls_set_range(remainder, rem, rem);

		return 0;
	} else {
		if (orig_low >= max_frag) {
			_gnutls_set_range(next, max_frag, max_frag);
			_gnutls_set_range(remainder, orig_low - max_frag,
					  orig_high - max_frag);
		} else {
			ret =
			    _gnutls_range_max_lh_pad(session, orig_low,
						     max_frag);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ssize_t this_pad = MIN(ret, orig_high - orig_low);

			_gnutls_set_range(next, orig_low,
					  orig_low + this_pad);
			_gnutls_set_range(remainder, 0,
					  orig_high - (orig_low +
						       this_pad));
		}

		return 0;
	}
}

static size_t
_gnutls_range_fragment(size_t data_size, gnutls_range_st cur,
		       gnutls_range_st next)
{
	return MIN(cur.high, data_size - next.low);
}

/**
 * gnutls_record_send_range:
 * @session: is a #gnutls_session_t type.
 * @data: contains the data to send.
 * @data_size: is the length of the data.
 * @range: is the range of lengths in which the real data length must be hidden.
 *
 * This function operates like gnutls_record_send() but, while
 * gnutls_record_send() adds minimal padding to each TLS record,
 * this function uses the TLS extra-padding feature to conceal the real
 * data size within the range of lengths provided.
 * Some TLS sessions do not support extra padding (e.g. stream ciphers in standard
 * TLS or SSL3 sessions). To know whether the current session supports extra
 * padding, and hence length hiding, use the gnutls_record_can_use_length_hiding()
 * function.
 *
 * Note: This function currently is only limited to blocking sockets.
 *
 * Returns: The number of bytes sent (that is data_size in a successful invocation),
 * or a negative error code.
 **/
ssize_t
gnutls_record_send_range(gnutls_session_t session, const void *data,
			 size_t data_size, const gnutls_range_st * range)
{
	size_t sent = 0;
	size_t next_fragment_length;
	ssize_t ret;
	gnutls_range_st cur_range, next_range;

	/* sanity check on range and data size */
	if (range->low > range->high ||
	    data_size < range->low || data_size > range->high) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	ret = gnutls_record_can_use_length_hiding(session);
	if (ret == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	_gnutls_set_range(&cur_range, range->low, range->high);

	_gnutls_record_log
	    ("RANGE: Preparing message with size %d, range (%d,%d)\n",
	     (int) data_size, (int) range->low, (int) range->high);

	while (cur_range.high != 0) {
		ret =
		    gnutls_range_split(session, &cur_range, &cur_range,
				       &next_range);
		if (ret < 0) {
			return ret;	/* already gnutls_assert_val'd */
		}

		next_fragment_length =
		    _gnutls_range_fragment(data_size, cur_range,
					   next_range);

		_gnutls_record_log
		    ("RANGE: Next fragment size: %d (%d,%d); remaining range: (%d,%d)\n",
		     (int) next_fragment_length, (int) cur_range.low,
		     (int) cur_range.high, (int) next_range.low,
		     (int) next_range.high);

		ret =
		    _gnutls_send_tlen_int(session, GNUTLS_APPLICATION_DATA,
					  -1, EPOCH_WRITE_CURRENT,
					  &(((char *) data)[sent]),
					  next_fragment_length,
					  cur_range.high -
					  next_fragment_length,
					  MBUFFER_FLUSH);

		while (ret == GNUTLS_E_AGAIN
		       || ret == GNUTLS_E_INTERRUPTED) {
			ret =
			    _gnutls_send_tlen_int(session,
						  GNUTLS_APPLICATION_DATA,
						  -1, EPOCH_WRITE_CURRENT,
						  NULL, 0, 0,
						  MBUFFER_FLUSH);
		}

		if (ret < 0) {
			return gnutls_assert_val(ret);
		}
		if (ret != (ssize_t) next_fragment_length) {
			_gnutls_record_log
			    ("RANGE: ERROR: ret = %d; next_fragment_length = %d\n",
			     (int) ret, (int) next_fragment_length);
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		}
		sent += next_fragment_length;
		data_size -= next_fragment_length;
		_gnutls_set_range(&cur_range, next_range.low,
				  next_range.high);
	}

	return sent;
}
