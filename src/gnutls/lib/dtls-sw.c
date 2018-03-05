/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Authors: Fridolin Pokorny
 *	  Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* Functions that relate to DTLS sliding window handling.
 */

#ifndef DTLS_SW_NO_INCLUDES
#include "gnutls_int.h"
#include "errors.h"
#include "debug.h"
#include "dtls.h"
#include "record.h"
#endif

/*
 * DTLS sliding window handling
 */
#define DTLS_EPOCH_SHIFT		(6*CHAR_BIT)
#define DTLS_SEQ_NUM_MASK		0x0000FFFFFFFFFFFF

#define DTLS_EMPTY_BITMAP		(0xFFFFFFFFFFFFFFFFULL)

/* We expect the compiler to be able to spot that this is a byteswapping
 * load, and emit instructions like 'movbe' on x86_64 where appropriate.
*/
#define LOAD_UINT64(out, ubytes)  \
	out = (((uint64_t)ubytes[0] << 56) |	\
	       ((uint64_t)ubytes[1] << 48) |	\
	       ((uint64_t)ubytes[2] << 40) |	\
	       ((uint64_t)ubytes[3] << 32) |	\
	       ((uint64_t)ubytes[4] << 24) |	\
	       ((uint64_t)ubytes[5] << 16) |	\
	       ((uint64_t)ubytes[6] << 8) |	\
	       ((uint64_t)ubytes[7] << 0) )


void _dtls_reset_window(struct record_parameters_st *rp)
{
	rp->dtls_sw_have_recv = 0;
}

/* Checks if a sequence number is not replayed. If a replayed
 * packet is detected it returns a negative value (but no sensible error code).
 * Otherwise zero.
 */
int _dtls_record_check(struct record_parameters_st *rp, gnutls_uint64 * _seq)
{
	uint64_t seq_num = 0;

	LOAD_UINT64(seq_num, _seq->i);

	if ((seq_num >> DTLS_EPOCH_SHIFT) != rp->epoch) {
		return gnutls_assert_val(-1);
	}

	seq_num &= DTLS_SEQ_NUM_MASK;

	/*
	 * rp->dtls_sw_next is the next *expected* packet (N), being
	 * the sequence number *after* the latest we have received.
	 *
	 * By definition, therefore, packet N-1 *has* been received.
	 * And thus there's no point wasting a bit in the bitmap for it.
	 *
	 * So the backlog bitmap covers the 64 packets prior to that,
	 * with the LSB representing packet (N - 2), and the MSB
	 * representing (N - 65). A received packet is represented
	 * by a zero bit, and a missing packet is represented by a one.
	 *
	 * Thus we can allow out-of-order reception of packets that are
	 * within a reasonable interval of the latest packet received.
	 */
	if (!rp->dtls_sw_have_recv) {
		rp->dtls_sw_next = seq_num + 1;
		rp->dtls_sw_bits = DTLS_EMPTY_BITMAP;
		rp->dtls_sw_have_recv = 1;
		return 0;
	} else if (seq_num == rp->dtls_sw_next) {
		/* The common case. This is the packet we expected next. */

		rp->dtls_sw_bits <<= 1;

		/* This might reach a value higher than 48-bit DTLS sequence
		 * numbers can actually reach. Which is fine. When that
		 * happens, we'll do the right thing and just not accept
		 * any newer packets. Someone needs to start a new epoch. */
		rp->dtls_sw_next++;
		return 0;
	} else if (seq_num > rp->dtls_sw_next) {
		/* The packet we were expecting has gone missing; this one is newer.
		 * We always advance the window to accommodate it. */
		uint64_t delta = seq_num - rp->dtls_sw_next;

		if (delta >= 64) {
			/* We jumped a long way into the future. We have not seen
			 * any of the previous 32 packets so set the backlog bitmap
			 * to all ones. */
			rp->dtls_sw_bits = DTLS_EMPTY_BITMAP;
		} else if (delta == 63) {
			/* Avoid undefined behaviour that shifting by 64 would incur.
			 * The (clear) top bit represents the packet which is currently
			 * rp->dtls_sw_next, which we know was already received. */
			rp->dtls_sw_bits = DTLS_EMPTY_BITMAP >> 1;
		} else {
			/* We have missed (delta) packets. Shift the backlog by that
			 * amount *plus* the one we would have shifted it anyway if
			 * we'd received the packet we were expecting. The zero bit
			 * representing the packet which is currently rp->dtls_sw_next-1,
			 * which we know has been received, ends up at bit position
			 * (1<<delta). Then we set all the bits lower than that, which
			 * represent the missing packets. */
			rp->dtls_sw_bits <<= delta + 1;
			rp->dtls_sw_bits |= (1ULL << delta) - 1;
		}
		rp->dtls_sw_next = seq_num + 1;
		return 0;
	} else {
		/* This packet is older than the one we were expecting. By how much...? */
		uint64_t delta = rp->dtls_sw_next - seq_num;

		if (delta > 65) {
			/* Too old. We can't know if it's a replay */
			return gnutls_assert_val(-2);
		} else if (delta == 1) {
			/* Not in the bitmask since it is by definition already received. */
			return gnutls_assert_val(-3);
		} else {
			/* Within the sliding window, so we remember whether we've seen it or not */
			uint64_t mask = 1ULL << (rp->dtls_sw_next - seq_num - 2);

			if (!(rp->dtls_sw_bits & mask))
				return gnutls_assert_val(-3);

			rp->dtls_sw_bits &= ~mask;
			return 0;
		}
	}
}
