/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_STR_ARRAY_H
#define GNUTLS_STR_ARRAY_H

#include "gnutls_int.h"
#include "errors.h"

/* Functionality to allow an array of strings. Strings
 * are allowed to be added to the list and matched against it.
 */

typedef struct gnutls_str_array_st {
	char *str;
	unsigned int len;
	struct gnutls_str_array_st *next;
} *gnutls_str_array_t;

inline static void _gnutls_str_array_init(gnutls_str_array_t * head)
{
	*head = NULL;
}

inline static void _gnutls_str_array_clear(gnutls_str_array_t * head)
{
	gnutls_str_array_t prev, array = *head;

	while (array != NULL) {
		prev = array;
		array = prev->next;
		gnutls_free(prev);
	}
	*head = NULL;
}

inline static int _gnutls_str_array_match(gnutls_str_array_t head,
					  const char *str)
{
	gnutls_str_array_t array = head;

	while (array != NULL) {
		if (strcmp(array->str, str) == 0)
			return 1;
		array = array->next;
	}

	return 0;
}

inline static void append(gnutls_str_array_t array, const char *str,
			  int len)
{
	array->str = ((char *) array) + sizeof(struct gnutls_str_array_st);
	memcpy(array->str, str, len);
	array->str[len] = 0;
	array->len = len;
	array->next = NULL;
}

inline static int _gnutls_str_array_append(gnutls_str_array_t * head,
					   const char *str, int len)
{
	gnutls_str_array_t prev, array;
	if (*head == NULL) {
		*head =
		    gnutls_malloc(len + 1 +
				  sizeof(struct gnutls_str_array_st));
		if (*head == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		array = *head;
		append(array, str, len);
	} else {
		array = *head;
		prev = array;
		while (array != NULL) {
			prev = array;
			array = prev->next;
		}
		prev->next =
		    gnutls_malloc(len + 1 +
				  sizeof(struct gnutls_str_array_st));

		array = prev->next;

		if (array == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		append(array, str, len);
	}

	return 0;
}

#endif
