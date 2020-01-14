/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <config.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Unit test for DTLS window handling */

#define LARGE_INT 4194304
#define INT_OVER_32_BITS 281474976708836LL

struct record_parameters_st {
	uint64_t dtls_sw_bits;
	uint64_t dtls_sw_next;
	unsigned dtls_sw_have_recv;
	unsigned epoch;
};

#define gnutls_assert_val(x) x

void _dtls_reset_window(struct record_parameters_st *rp);
int _dtls_record_check(struct record_parameters_st *rp, uint64_t _seq);

#define DTLS_SW_NO_INCLUDES
#include "../lib/dtls-sw.c"

#define RESET_WINDOW \
	memset(&state, 0, sizeof(state))

#define SET_WINDOW_NEXT(x) \
	state.dtls_sw_next = (((x)&DTLS_SEQ_NUM_MASK))

#define SET_WINDOW_LAST_RECV(x) \
	t = x; \
	state.dtls_sw_have_recv = 1

static void check_dtls_window_uninit_0(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);

	t = 0;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_uninit_large(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;

	t = LARGE_INT+1+64;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_uninit_very_large(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;

	t = INT_OVER_32_BITS;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_12(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(1);

	t = 2;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_19(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(1);

	t = 9;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_skip1(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;
	unsigned i;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(1);

	for (i=2;i<256;i+=2) {
		t = i;
		assert_int_equal(_dtls_record_check(&state, t), 0);
	}
}

static void check_dtls_window_skip3(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;
	unsigned i;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(1);

	for (i=5;i<256;i+=2) {
		t = i;
		assert_int_equal(_dtls_record_check(&state, t), 0);
	}
}

static void check_dtls_window_21(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(2);

	t = 1;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_91(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(9);

	t = 1;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_large_21(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT);
	SET_WINDOW_LAST_RECV(LARGE_INT+2);

	t = LARGE_INT+1;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_large_12(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT);
	SET_WINDOW_LAST_RECV(LARGE_INT+1);

	t = LARGE_INT+2;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_large_91(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT);
	SET_WINDOW_LAST_RECV(LARGE_INT+9);

	t = LARGE_INT+1;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_large_19(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT);
	SET_WINDOW_LAST_RECV(LARGE_INT+1);

	t = LARGE_INT+9;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_very_large_12(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(INT_OVER_32_BITS);
	SET_WINDOW_LAST_RECV(INT_OVER_32_BITS+1);

	t = INT_OVER_32_BITS+2;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_very_large_91(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(INT_OVER_32_BITS);
	SET_WINDOW_LAST_RECV(INT_OVER_32_BITS+9);

	t = INT_OVER_32_BITS+1;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_very_large_19(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(INT_OVER_32_BITS);
	SET_WINDOW_LAST_RECV(INT_OVER_32_BITS+1);

	t = INT_OVER_32_BITS+9;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_outside(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(0);
	SET_WINDOW_LAST_RECV(1);

	t = 1+64;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_large_outside(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT);
	SET_WINDOW_LAST_RECV(LARGE_INT+1);

	t = LARGE_INT+1+64;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_very_large_outside(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(INT_OVER_32_BITS);
	SET_WINDOW_LAST_RECV(INT_OVER_32_BITS+1);

	t = INT_OVER_32_BITS+1+64;

	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_dup1(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT-1);
	SET_WINDOW_LAST_RECV(LARGE_INT);

	t = LARGE_INT;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+1;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+16;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+1;
	assert_int_equal(_dtls_record_check(&state, t), -3);
}

static void check_dtls_window_dup2(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT-1);
	SET_WINDOW_LAST_RECV(LARGE_INT);

	t = LARGE_INT;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+16;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+1;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+16;
	assert_int_equal(_dtls_record_check(&state, t), -3);
}

static void check_dtls_window_dup3(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT-1);
	SET_WINDOW_LAST_RECV(LARGE_INT);

	t = LARGE_INT;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+16;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+15;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+14;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+5;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+5;
	assert_int_equal(_dtls_record_check(&state, t), -3);
}

static void check_dtls_window_out_of_order(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT-1);
	SET_WINDOW_LAST_RECV(LARGE_INT);

	t = LARGE_INT;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+8;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+7;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+6;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+5;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+4;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+3;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+2;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+1;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = LARGE_INT+9;
	assert_int_equal(_dtls_record_check(&state, t), 0);
}

static void check_dtls_window_epoch_higher(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	SET_WINDOW_NEXT(LARGE_INT-1);
	SET_WINDOW_LAST_RECV(LARGE_INT);

	t = LARGE_INT;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = (LARGE_INT+8)|0x1000000000000LL;
	assert_int_equal(_dtls_record_check(&state, t), -1);
}

static void check_dtls_window_epoch_lower(void **glob_state)
{
	struct record_parameters_st state;
	uint64_t t;

	RESET_WINDOW;
	t = 0x1000000000000LL;

	state.epoch = 1;
	SET_WINDOW_NEXT(0x1000000000000LL);
	SET_WINDOW_LAST_RECV((0x1000000000000LL) + 1);

	t = 2 | 0x1000000000000LL;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = 3 | 0x1000000000000LL;
	assert_int_equal(_dtls_record_check(&state, t), 0);

	t = 5;
	assert_int_equal(_dtls_record_check(&state, t), -1);
}


int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(check_dtls_window_uninit_0),
		cmocka_unit_test(check_dtls_window_uninit_large),
		cmocka_unit_test(check_dtls_window_uninit_very_large),
		cmocka_unit_test(check_dtls_window_12),
		cmocka_unit_test(check_dtls_window_21),
		cmocka_unit_test(check_dtls_window_19),
		cmocka_unit_test(check_dtls_window_91),
		cmocka_unit_test(check_dtls_window_large_21),
		cmocka_unit_test(check_dtls_window_large_12),
		cmocka_unit_test(check_dtls_window_large_19),
		cmocka_unit_test(check_dtls_window_large_91),
		cmocka_unit_test(check_dtls_window_dup1),
		cmocka_unit_test(check_dtls_window_dup2),
		cmocka_unit_test(check_dtls_window_dup3),
		cmocka_unit_test(check_dtls_window_outside),
		cmocka_unit_test(check_dtls_window_large_outside),
		cmocka_unit_test(check_dtls_window_out_of_order),
		cmocka_unit_test(check_dtls_window_epoch_lower),
		cmocka_unit_test(check_dtls_window_epoch_higher),
		cmocka_unit_test(check_dtls_window_very_large_12),
		cmocka_unit_test(check_dtls_window_very_large_19),
		cmocka_unit_test(check_dtls_window_very_large_91),
		cmocka_unit_test(check_dtls_window_very_large_outside),
		cmocka_unit_test(check_dtls_window_skip3),
		cmocka_unit_test(check_dtls_window_skip1)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
