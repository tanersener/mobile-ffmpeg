/*
 * Copyright (C) 2011-2019 Free Software Foundation, Inc.
 * Copyright (C) 2019 Red Hat, Inc.
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

#ifndef GNUTLS_NAME_VAL_ARRAY_H
#define GNUTLS_NAME_VAL_ARRAY_H

#include "gnutls_int.h"
#include "errors.h"

/* Functionality to allow an array of strings. Strings
 * are allowed to be added to the list and matched against it.
 */

typedef struct name_val_array_st {
	char *name;
	unsigned name_size;
	char *val;
	struct name_val_array_st *next;
} *name_val_array_t;

inline static void _name_val_array_init(name_val_array_t * head)
{
	*head = NULL;
}

inline static void _name_val_array_clear(name_val_array_t * head)
{
	name_val_array_t prev, array = *head;

	while (array != NULL) {
		prev = array;
		array = prev->next;
		gnutls_free(prev);
	}
	*head = NULL;
}

inline static const char *_name_val_array_value(name_val_array_t head,
					        const char *name, unsigned name_size)
{
	name_val_array_t array = head;

	while (array != NULL) {
		if (array->name_size == name_size &&
		    memcmp(array->name, name, name_size) == 0) {
			return array->val;
		}
		array = array->next;
	}

	return NULL;
}

inline static void append(name_val_array_t array, const char *name,
			  unsigned name_len, const char *val,
			  unsigned val_len)
{
	array->name = ((char *) array) + sizeof(struct name_val_array_st);
	memcpy(array->name, name, name_len);
	array->name[name_len] = 0;
	array->name_size = name_len;

	array->val = ((char *) array) + name_len + 1 + sizeof(struct name_val_array_st);
	if (val)
		memcpy(array->val, val, val_len);
	array->val[val_len] = 0;

	array->next = NULL;
}

inline static int _name_val_array_append(name_val_array_t * head,
					 const char *name,
					 const char *val)
{
	name_val_array_t prev, array;
	unsigned name_len = strlen(name);
	unsigned val_len = (val==NULL)?0:strlen(val);

	if (*head == NULL) {
		*head =
		    gnutls_malloc(val_len + name_len + 2 +
				  sizeof(struct name_val_array_st));
		if (*head == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		array = *head;
		append(array, name, name_len, val, val_len);
	} else {
		array = *head;
		prev = array;

		while (array != NULL) {
			prev = array;
			array = prev->next;
		}
		prev->next =
		    gnutls_malloc(name_len + val_len + 2 +
				  sizeof(struct name_val_array_st));
		array = prev->next;

		if (array == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		append(array, name, name_len, val, val_len);
	}

	return 0;
}

#endif
