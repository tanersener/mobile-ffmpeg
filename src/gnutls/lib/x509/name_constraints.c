/*
 * Copyright (C) 2014-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos, Daiki Ueno, Martin Ukrop
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

/* Functions on X.509 Certificate parsing
 */

#include "gnutls_int.h"
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <gnutls/x509-ext.h>
#include <x509_b64.h>
#include <x509_int.h>
#include <libtasn1.h>

#include "ip.h"
#include "ip-in-cidr.h"

// for documentation see the implementation
static int name_constraints_intersect_nodes(name_constraints_node_st * nc1,
					    name_constraints_node_st * nc2,
					    name_constraints_node_st ** intersection);

/*-
 * is_nc_empty:
 * @nc: name constraints structure
 * @type: type (gnutls_x509_subject_alt_name_t)
 *
 * Test whether given name constraints structure has any constraints (permitted
 * or excluded) of a given type. @nc must be allocated (not NULL) before the call.
 *
 * Returns: 0 if @nc contains constraints of type @type, 1 otherwise
 -*/
static unsigned is_nc_empty(struct gnutls_name_constraints_st* nc, unsigned type)
{
	name_constraints_node_st *t;

	if (nc->permitted == NULL && nc->excluded == NULL)
		return 1;

	t = nc->permitted;
	while (t != NULL) {
		if (t->type == type)
			return 0;
		t = t->next;
	}

	t = nc->excluded;
	while (t != NULL) {
		if (t->type == type)
			return 0;
		t = t->next;
	}

	/* no constraint for that type exists */
	return 1;
}

/*-
 * validate_name_constraints_node:
 * @type: type of name constraints
 * @name: datum of name constraint
 *
 * Check the validity of given name constraints node (@type and @name).
 * The supported types are GNUTLS_SAN_DNSNAME, GNUTLS_SAN_RFC822NAME,
 * GNUTLS_SAN_DN, GNUTLS_SAN_URI and GNUTLS_SAN_IPADDRESS.
 *
 * CIDR ranges are checked for correct length (IPv4/IPv6) and correct mask format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 -*/
static int validate_name_constraints_node(gnutls_x509_subject_alt_name_t type,
					  const gnutls_datum_t* name)
{
	if (type != GNUTLS_SAN_DNSNAME && type != GNUTLS_SAN_RFC822NAME &&
		type != GNUTLS_SAN_DN && type != GNUTLS_SAN_URI &&
		type != GNUTLS_SAN_IPADDRESS) {
		return gnutls_assert_val(GNUTLS_E_X509_UNKNOWN_SAN);
	}

	if (type == GNUTLS_SAN_IPADDRESS) {
		if (name->size != 8 && name->size != 32)
			return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		int prefix = _gnutls_mask_to_prefix(name->data + name->size/2, name->size/2);
		if (prefix < 0)
			return gnutls_assert_val(GNUTLS_E_MALFORMED_CIDR);
	}

	return GNUTLS_E_SUCCESS;
}

int _gnutls_extract_name_constraints(ASN1_TYPE c2, const char *vstr,
									 name_constraints_node_st ** _nc)
{
	int ret;
	char tmpstr[128];
	unsigned indx = 0;
	gnutls_datum_t tmp = { NULL, 0 };
	unsigned int type;
	struct name_constraints_node_st *nc, *prev;

	prev = *_nc;
	if (prev != NULL) {
		while(prev->next != NULL)
			prev = prev->next;
	}

	do {
		indx++;
		snprintf(tmpstr, sizeof(tmpstr), "%s.?%u.base", vstr, indx);

		ret =
		    _gnutls_parse_general_name2(c2, tmpstr, -1, &tmp, &type, 0);

		if (ret < 0) {
			gnutls_assert();
			break;
		}

		ret = validate_name_constraints_node(type, &tmp);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		nc = gnutls_malloc(sizeof(struct name_constraints_node_st));
		if (nc == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto cleanup;
		}

		memcpy(&nc->name, &tmp, sizeof(gnutls_datum_t));
		nc->type = type;
		nc->next = NULL;

		if (prev == NULL) {
			*_nc = prev = nc;
		} else {
			prev->next = nc;
			prev = nc;
		}

		tmp.data = NULL;
	} while (ret >= 0);

	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(tmp.data);
	return ret;
}

/*-
 * _gnutls_name_constraints_node_free:
 * @node: name constriants node
 *
 * Deallocate a list of name constraints nodes starting at the given node.
 -*/
void _gnutls_name_constraints_node_free(name_constraints_node_st *node)
{
	name_constraints_node_st *next, *t;

	t = node;
	while (t != NULL) {
		next = t->next;
		gnutls_free(t->name.data);
		gnutls_free(t);
		t = next;
	}
}

/*-
 * name_constraints_node_new:
 * @type: name constraints type to set (gnutls_x509_subject_alt_name_t)
 * @data: name.data to set or NULL
 * @size: name.size to set
 *
 * Allocate a new name constraints node and set its type, name size and name data.
 * If @data is set to NULL, name data will be an array of \x00 (the length of @size).
 * The .next pointer is set to NULL.
 *
 * Returns: Pointer to newly allocated node or NULL in case of memory error.
 -*/
static name_constraints_node_st* name_constraints_node_new(unsigned type,
							   unsigned char *data,
							   unsigned int size)
{
	name_constraints_node_st *tmp = gnutls_malloc(sizeof(struct name_constraints_node_st));
	if (tmp == NULL)
		return NULL;
	tmp->type = type;
	tmp->next = NULL;
	tmp->name.size = size;
	tmp->name.data = NULL;
	if (tmp->name.size > 0) {

		tmp->name.data = gnutls_malloc(tmp->name.size);
		if (tmp->name.data == NULL) {
			gnutls_free(tmp);
			return NULL;
		}
		if (data != NULL) {
			memcpy(tmp->name.data, data, size);
		} else {
			memset(tmp->name.data, 0, size);
		}
	}
	return tmp;
}

/*-
 * @brief _gnutls_name_constraints_intersect:
 * @_nc: first name constraints list (permitted)
 * @_nc2: name constraints list to merge with (permitted)
 * @_nc_excluded: Corresponding excluded name constraints list
 *
 * This function finds the intersection of @_nc and @_nc2. The result is placed in @_nc,
 * the original @_nc is deallocated. @_nc2 is not changed. If necessary, a universal
 * excluded name constraint node of the right type is added to the list provided
 * in @_nc_excluded.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 -*/
static
int _gnutls_name_constraints_intersect(name_constraints_node_st ** _nc,
				       name_constraints_node_st * _nc2,
				       name_constraints_node_st ** _nc_excluded)
{
	name_constraints_node_st *nc, *nc2, *t, *tmp, *dest = NULL, *prev = NULL;
	int ret, type, used;

	/* temporary array to see, if we need to add universal excluded constraints
	 * (see phase 3 for details)
	 * indexed directly by (gnutls_x509_subject_alt_name_t enum - 1) */
	unsigned char types_with_empty_intersection[GNUTLS_SAN_MAX];
	memset(types_with_empty_intersection, 0, sizeof(types_with_empty_intersection));

	if (*_nc == NULL || _nc2 == NULL)
		return 0;

	/* Phase 1
	 * For each name in _NC, if a _NC2 does not contain a name
	 * with the same type, preserve the original name.
	 * Do this also for node of unknown type (not DNS, email, IP */
	t = nc = *_nc;
	while (t != NULL) {
		name_constraints_node_st *next = t->next;
		nc2 = _nc2;
		while (nc2 != NULL) {
			if (t->type == nc2->type) {
				// check bounds (we will use 't->type' as index)
				if (t->type > GNUTLS_SAN_MAX || t->type == 0)
					return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
				// note the possibility of empty intersection for this type
				// if we add something to the intersection in phase 2,
				// we will reset this flag back to 0 then
				types_with_empty_intersection[t->type - 1] = 1;
				break;
			}
			nc2 = nc2->next;
		}
		if (nc2 == NULL ||
			(t->type != GNUTLS_SAN_DNSNAME &&
			 t->type != GNUTLS_SAN_RFC822NAME &&
			 t->type != GNUTLS_SAN_IPADDRESS)
		   ) {
			/* move node from NC to DEST */
			if (prev != NULL)
				prev->next = next;
			else
				prev = nc = next;
			t->next = dest;
			dest = t;
		} else {
			prev = t;
		}
		t = next;
	}

	/* Phase 2
	 * iterate through all combinations from nc2 and nc1
	 * and create intersections of nodes with same type */
	nc2 = _nc2;
	while (nc2 != NULL) {
		// current nc2 node has not yet been used for any intersection
		// (and is not in DEST either)
		used = 0;
		t = nc;
		while (t != NULL) {
			// save intersection of name constraints into tmp
			ret = name_constraints_intersect_nodes(t, nc2, &tmp);
			if (ret < 0) return gnutls_assert_val(ret);
			used = 1;
			// if intersection is not empty
			if (tmp != NULL) { // intersection for this type is not empty
				// check bounds
				if (tmp->type > GNUTLS_SAN_MAX || tmp->type == 0) {
					gnutls_free(tmp);
					return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
				}
				// we will not add universal excluded constraint for this type
				types_with_empty_intersection[tmp->type - 1] = 0;
				// add intersection node to DEST
				tmp->next = dest;
				dest = tmp;
			}
			t = t->next;
		}
		// if the node from nc2 was not used for intersection, copy it to DEST
		// Beware: also copies nodes other than DNS, email, IP,
		//	 since their counterpart may have been moved in phase 1.
		if (!used) {
			tmp = name_constraints_node_new(nc2->type, nc2->name.data, nc2->name.size);
			if (tmp == NULL) {
				_gnutls_name_constraints_node_free(dest);
				return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
			}
			tmp->next = dest;
			dest = tmp;
		}
		nc2 = nc2->next;
	}

	/* replace the original with the new */
	_gnutls_name_constraints_node_free(nc);
	*_nc = dest;

	/* Phase 3
	 * For each type: If we have empty permitted name constraints now
	 * and we didn't have at the beginning, we have to add a new
	 * excluded constraint with universal wildcard
	 * (since the intersection of permitted is now empty). */
	for (type = 1; type <= GNUTLS_SAN_MAX; type++) {
		if (types_with_empty_intersection[type-1] == 0)
			continue;
		_gnutls_hard_log("Adding universal excluded name constraint for type %d.\n", type);
		switch (type) {
			case GNUTLS_SAN_IPADDRESS:
				// add universal restricted range for IPv4
				tmp = name_constraints_node_new(GNUTLS_SAN_IPADDRESS, NULL, 8);
				if (tmp == NULL) {
					_gnutls_name_constraints_node_free(dest);
					return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				}
				tmp->next = *_nc_excluded;
				*_nc_excluded = tmp;
				// add universal restricted range for IPv6
				tmp = name_constraints_node_new(GNUTLS_SAN_IPADDRESS, NULL, 32);
				if (tmp == NULL) {
					_gnutls_name_constraints_node_free(dest);
					return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				}
				tmp->next = *_nc_excluded;
				*_nc_excluded = tmp;
				break;
			case GNUTLS_SAN_DNSNAME:
			case GNUTLS_SAN_RFC822NAME:
				tmp = name_constraints_node_new(type, NULL, 0);
				if (tmp == NULL) {
					_gnutls_name_constraints_node_free(dest);
					return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				}
				tmp->next = *_nc_excluded;
				*_nc_excluded = tmp;
				break;
			default: // do nothing, at least one node was already moved in phase 1
				break;
		}
	}
	return GNUTLS_E_SUCCESS;
}

static int _gnutls_name_constraints_append(name_constraints_node_st **_nc,
										   name_constraints_node_st *_nc2)
{
	name_constraints_node_st *nc, *nc2;
	struct name_constraints_node_st *tmp;

	if (_nc2 == NULL)
		return 0;

	nc2 = _nc2;
	while (nc2) {
		nc = *_nc;

		tmp = name_constraints_node_new(nc2->type, nc2->name.data, nc2->name.size);
		if (tmp == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		tmp->next = nc;
		*_nc = tmp;

		nc2 = nc2->next;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_name_constraints:
 * @crt: should contain a #gnutls_x509_crt_t type
 * @nc: The nameconstraints intermediate type
 * @flags: zero or %GNUTLS_EXT_FLAG_APPEND
 * @critical: the extension status
 *
 * This function will return an intermediate type containing
 * the name constraints of the provided CA certificate. That
 * structure can be used in combination with gnutls_x509_name_constraints_check()
 * to verify whether a server's name is in accordance with the constraints.
 *
 * When the @flags is set to %GNUTLS_EXT_FLAG_APPEND,
 * then if the @nc structure is empty this function will behave
 * identically as if the flag was not set.
 * Otherwise if there are elements in the @nc structure then the
 * constraints will be merged with the existing constraints following
 * RFC5280 p6.1.4 (excluded constraints will be appended, permitted
 * will be intersected).
 *
 * Note that @nc must be initialized prior to calling this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_crt_get_name_constraints(gnutls_x509_crt_t crt,
					 gnutls_x509_name_constraints_t nc,
					 unsigned int flags,
					 unsigned int *critical)
{
	int ret;
	gnutls_datum_t der = { NULL, 0 };

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_x509_crt_get_extension(crt, "2.5.29.30", 0, &der,
					   critical);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (der.size == 0 || der.data == NULL)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	ret = gnutls_x509_ext_import_name_constraints(&der, nc, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	_gnutls_free_datum(&der);

	return ret;

}

/**
 * gnutls_x509_name_constraints_deinit:
 * @nc: The nameconstraints
 *
 * This function will deinitialize a name constraints type.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_name_constraints_deinit(gnutls_x509_name_constraints_t nc)
{
	_gnutls_name_constraints_node_free(nc->permitted);
	_gnutls_name_constraints_node_free(nc->excluded);

	gnutls_free(nc);
}

/**
 * gnutls_x509_name_constraints_init:
 * @nc: The nameconstraints
 *
 * This function will initialize a name constraints type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_name_constraints_init(gnutls_x509_name_constraints_t *nc)
{
	*nc = gnutls_calloc(1, sizeof(struct gnutls_name_constraints_st));
	if (*nc == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static
int name_constraints_add(gnutls_x509_name_constraints_t nc,
			 gnutls_x509_subject_alt_name_t type,
			 const gnutls_datum_t * name,
			 unsigned permitted)
{
	struct name_constraints_node_st * tmp, *prev = NULL;
	int ret;

	ret = validate_name_constraints_node(type, name);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (permitted != 0)
		prev = tmp = nc->permitted;
	else
		prev = tmp = nc->excluded;

	while(tmp != NULL) {
		tmp = tmp->next;
		if (tmp != NULL)
			prev = tmp;
	}

	tmp = name_constraints_node_new(type, name->data, name->size);
	if (tmp == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	tmp->next = NULL;

	if (prev == NULL) {
		if (permitted != 0)
			nc->permitted = tmp;
		else
			nc->excluded = tmp;
	} else
		prev->next = tmp;

	return 0;
}

/*-
 * _gnutls_x509_name_constraints_merge:
 * @nc: The nameconstraints
 * @nc2: The name constraints to be merged with
 *
 * This function will merge the provided name constraints structures
 * as per RFC5280 p6.1.4. That is, the excluded constraints will be appended,
 * and permitted will be intersected. The intersection assumes that @nc
 * is the root CA constraints.
 *
 * The merged constraints will be placed in @nc.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.5.0
 -*/
int _gnutls_x509_name_constraints_merge(gnutls_x509_name_constraints_t nc,
					gnutls_x509_name_constraints_t nc2)
{
	int ret;

	ret =
	    _gnutls_name_constraints_intersect(&nc->permitted,
						   nc2->permitted, &nc->excluded);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_name_constraints_append(&nc->excluded,
					    nc2->excluded);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_name_constraints_add_permitted:
 * @nc: The nameconstraints
 * @type: The type of the constraints
 * @name: The data of the constraints
 *
 * This function will add a name constraint to the list of permitted
 * constraints. The constraints @type can be any of the following types:
 * %GNUTLS_SAN_DNSNAME, %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DN,
 * %GNUTLS_SAN_URI, %GNUTLS_SAN_IPADDRESS. For the latter, an IP address
 * in network byte order is expected, followed by its network mask.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_name_constraints_add_permitted(gnutls_x509_name_constraints_t nc,
					       gnutls_x509_subject_alt_name_t type,
					       const gnutls_datum_t * name)
{
	return name_constraints_add(nc, type, name, 1);
}

/**
 * gnutls_x509_name_constraints_add_excluded:
 * @nc: The nameconstraints
 * @type: The type of the constraints
 * @name: The data of the constraints
 *
 * This function will add a name constraint to the list of excluded
 * constraints. The constraints @type can be any of the following types:
 * %GNUTLS_SAN_DNSNAME, %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DN,
 * %GNUTLS_SAN_URI, %GNUTLS_SAN_IPADDRESS. For the latter, an IP address
 * in network byte order is expected, followed by its network mask (which is
 * 4 bytes in IPv4 or 16-bytes in IPv6).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_name_constraints_add_excluded(gnutls_x509_name_constraints_t nc,
					      gnutls_x509_subject_alt_name_t type,
					      const gnutls_datum_t * name)
{
	return name_constraints_add(nc, type, name, 0);
}

/**
 * gnutls_x509_crt_set_name_constraints:
 * @crt: The certificate
 * @nc: The nameconstraints structure
 * @critical: whether this extension will be critical
 *
 * This function will set the provided name constraints to
 * the certificate extension list. This extension is always
 * marked as critical.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_crt_set_name_constraints(gnutls_x509_crt_t crt, 
					 gnutls_x509_name_constraints_t nc,
					 unsigned int critical)
{
int ret;
gnutls_datum_t der;

	ret = gnutls_x509_ext_export_name_constraints(nc, &der);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.30", &der, critical);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
	crt->use_extensions = 1;

cleanup:
	_gnutls_free_datum(&der);
	return ret;
}

static
unsigned ends_with(const gnutls_datum_t * str, const gnutls_datum_t * suffix)
{
	unsigned char *tree;
	unsigned int treelen;

	if (suffix->size >= str->size)
		return 0;

	tree = suffix->data;
	treelen = suffix->size;
	if((treelen > 0) && (tree[0] == '.')) {
		tree++;
		treelen--;
	}

	if (memcmp(str->data + str->size - treelen, tree, treelen) == 0 &&
		str->data[str->size - treelen -1] == '.')
		return 1; /* match */

	return 0;
}

static
unsigned email_ends_with(const gnutls_datum_t * str, const gnutls_datum_t * suffix)
{
	if (suffix->size >= str->size)
		return 0;

	if (suffix->size > 1 && suffix->data[0] == '.') {
		/* .domain.com */
		if (memcmp(str->data + str->size - suffix->size, suffix->data, suffix->size) == 0)
			return 1; /* match */
	} else {
		if (memcmp(str->data + str->size - suffix->size, suffix->data, suffix->size) == 0 &&
			str->data[str->size - suffix->size -1] == '@')
			return 1; /* match */
	}

	return 0;
}

static unsigned dnsname_matches(const gnutls_datum_t *name, const gnutls_datum_t *suffix)
{
	_gnutls_hard_log("matching %.*s with DNS constraint %.*s\n", name->size, name->data,
		suffix->size, suffix->data);

	if (suffix->size == name->size && memcmp(suffix->data, name->data, suffix->size) == 0)
		return 1; /* match */

	return ends_with(name, suffix);
}

static unsigned email_matches(const gnutls_datum_t *name, const gnutls_datum_t *suffix)
{
	_gnutls_hard_log("matching %.*s with e-mail constraint %.*s\n", name->size, name->data,
		suffix->size, suffix->data);

	if (suffix->size == name->size && memcmp(suffix->data, name->data, suffix->size) == 0)
		return 1; /* match */

	return email_ends_with(name, suffix);
}

/*-
 * name_constraints_intersect_nodes:
 * @nc1: name constraints node 1
 * @nc2: name constraints node 2
 * @_intersection: newly allocated node with intersected constraints,
 *		 NULL if the intersection is empty
 *
 * Inspect 2 name constraints nodes (of possibly different types) and allocate
 * a new node with intersection of given constraints.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 -*/
static int
name_constraints_intersect_nodes(name_constraints_node_st * nc1,
				 name_constraints_node_st * nc2,
				 name_constraints_node_st ** _intersection)
{
	// presume empty intersection
	name_constraints_node_st *intersection = NULL;
	name_constraints_node_st *to_copy = NULL;
	unsigned iplength = 0;
	unsigned byte;

	*_intersection = NULL;

	if (nc1->type != nc2->type) {
		return GNUTLS_E_SUCCESS;
	}
	switch (nc1->type) {
	case GNUTLS_SAN_DNSNAME:
		if (!dnsname_matches(&nc2->name, &nc1->name))
			return GNUTLS_E_SUCCESS;
		to_copy = nc2;
		break;
	case GNUTLS_SAN_RFC822NAME:
		if (!email_matches(&nc2->name, &nc1->name))
			return GNUTLS_E_SUCCESS;
		to_copy = nc2;
		break;
	case GNUTLS_SAN_IPADDRESS:
		if (nc1->name.size != nc2->name.size)
			return GNUTLS_E_SUCCESS;
		iplength = nc1->name.size/2;
		for (byte = 0; byte < iplength; byte++) {
			if (((nc1->name.data[byte]^nc2->name.data[byte]) // XOR of addresses
				 & nc1->name.data[byte+iplength]  // AND mask from nc1
				 & nc2->name.data[byte+iplength]) // AND mask from nc2
				 != 0) {
				// CIDRS do not intersect
				return GNUTLS_E_SUCCESS;
			}
		}
		to_copy = nc2;
		break;
	default:
		// for other types, we don't know how to do the intersection, assume empty
		return GNUTLS_E_SUCCESS;
	}

	// copy existing node if applicable
	if (to_copy != NULL) {
		*_intersection = name_constraints_node_new(to_copy->type, to_copy->name.data, to_copy->name.size);
		if (*_intersection == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		intersection = *_intersection;

		assert(intersection->name.data != NULL);

		if (intersection->type == GNUTLS_SAN_IPADDRESS) {
			// make sure both IP addresses are correctly masked
			_gnutls_mask_ip(intersection->name.data, intersection->name.data+iplength, iplength);
			_gnutls_mask_ip(nc1->name.data, nc1->name.data+iplength, iplength);
			// update intersection, if necessary (we already know one is subset of other)
			for (byte = 0; byte < 2 * iplength; byte++) {
				intersection->name.data[byte] |= nc1->name.data[byte];
			}
		}
	}

	return GNUTLS_E_SUCCESS;
}

/*
 * Returns: true if the certification is acceptable, and false otherwise.
 */
static
unsigned check_unsupported_constraint(gnutls_x509_name_constraints_t nc,
									  gnutls_x509_subject_alt_name_t type)
{
unsigned i;
int ret;
unsigned rtype;
gnutls_datum_t rname;

	/* check if there is a restrictions with that type, if
	 * yes, then reject the name.
	 */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != type)
				continue;
			else
				return gnutls_assert_val(0);
		}

	} while(ret == 0);

	return 1;
}

static
unsigned check_dns_constraints(gnutls_x509_name_constraints_t nc,
							   const gnutls_datum_t * name)
{
unsigned i;
int ret;
unsigned rtype;
unsigned allowed_found = 0;
gnutls_datum_t rname;

	/* check restrictions */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_DNSNAME)
				continue;

			/* a name of value 0 means that the CA shouldn't have issued
			 * a certificate with a DNSNAME. */
			if (rname.size == 0)
				return gnutls_assert_val(0);

			if (dnsname_matches(name, &rname) != 0)
				return gnutls_assert_val(0); /* rejected */
		}
	} while(ret == 0);

	/* check allowed */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_DNSNAME)
				continue;

			if (rname.size == 0)
				continue;

			allowed_found = 1;

			if (dnsname_matches(name, &rname) != 0)
				return 1; /* accepted */
		}
	} while(ret == 0);

	if (allowed_found != 0) /* there are allowed directives but this host wasn't found */
		return gnutls_assert_val(0);

	return 1;
}

static
unsigned check_email_constraints(gnutls_x509_name_constraints_t nc,
								 const gnutls_datum_t * name)
{
unsigned i;
int ret;
unsigned rtype;
unsigned allowed_found = 0;
gnutls_datum_t rname;

	/* check restrictions */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_RFC822NAME)
				continue;

			/* a name of value 0 means that the CA shouldn't have issued
			 * a certificate with an e-mail. */
			if (rname.size == 0)
				return gnutls_assert_val(0);

			if (email_matches(name, &rname) != 0)
				return gnutls_assert_val(0); /* rejected */
		}
	} while(ret == 0);

	/* check allowed */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_RFC822NAME)
				continue;

			if (rname.size == 0)
				continue;

			allowed_found = 1;

			if (email_matches(name, &rname) != 0)
				return 1; /* accepted */
		}
	} while(ret == 0);

	if (allowed_found != 0) /* there are allowed directives but this host wasn't found */
		return gnutls_assert_val(0);

	return 1;
}

static
unsigned check_ip_constraints(gnutls_x509_name_constraints_t nc,
							  const gnutls_datum_t * name)
{
	unsigned i;
	int ret;
	unsigned rtype;
	unsigned allowed_found = 0;
	gnutls_datum_t rname;

	/* check restrictions */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_IPADDRESS)
				continue;

			/* do not check IPv4 against IPv6 constraints and vice versa */
			if (name->size != rname.size / 2)
				continue;

			if (ip_in_cidr(name, &rname) != 0)
				return gnutls_assert_val(0); /* rejected */
		}
	} while(ret == 0);

	/* check allowed */
	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &rtype, &rname);
		if (ret >= 0) {
			if (rtype != GNUTLS_SAN_IPADDRESS)
				continue;

			/* do not check IPv4 against IPv6 constraints and vice versa */
			if (name->size != rname.size / 2)
				continue;

			allowed_found = 1;

			if (ip_in_cidr(name, &rname) != 0)
				return 1; /* accepted */
		}
	} while(ret == 0);

	if (allowed_found != 0) /* there are allowed directives but this host wasn't found */
		return gnutls_assert_val(0);

	return 1;
}

/**
 * gnutls_x509_name_constraints_check:
 * @nc: the extracted name constraints
 * @type: the type of the constraint to check (of type gnutls_x509_subject_alt_name_t)
 * @name: the name to be checked
 *
 * This function will check the provided name against the constraints in
 * @nc using the RFC5280 rules. Currently this function is limited to DNS
 * names, emails and IP addresses (of type %GNUTLS_SAN_DNSNAME,
 * %GNUTLS_SAN_RFC822NAME and %GNUTLS_SAN_IPADDRESS).
 *
 * Returns: zero if the provided name is not acceptable, and non-zero otherwise.
 *
 * Since: 3.3.0
 **/
unsigned gnutls_x509_name_constraints_check(gnutls_x509_name_constraints_t nc,
					    gnutls_x509_subject_alt_name_t type,
					    const gnutls_datum_t * name)
{
	if (type == GNUTLS_SAN_DNSNAME)
		return check_dns_constraints(nc, name);

	if (type == GNUTLS_SAN_RFC822NAME)
		return check_email_constraints(nc, name);

	if (type == GNUTLS_SAN_IPADDRESS)
		return check_ip_constraints(nc, name);

	return check_unsupported_constraint(nc, type);
}

/* This function checks for unsupported constraints, that we also
 * know their structure. That is it will fail only if the constraint
 * is present in the CA, _and_ the name in the end certificate contains
 * the constrained element.
 *
 * Returns: true if the certification is acceptable, and false otherwise
 */
static unsigned check_unsupported_constraint2(gnutls_x509_crt_t cert, 
					      gnutls_x509_name_constraints_t nc,
					      gnutls_x509_subject_alt_name_t type)
{
	unsigned idx, found_one;
	char name[MAX_CN];
	size_t name_size;
	unsigned san_type;
	int ret;

	idx = 0;
	found_one = 0;

	do {
		name_size = sizeof(name);
		ret = gnutls_x509_crt_get_subject_alt_name2(cert,
			idx++, name, &name_size, &san_type, NULL);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		else if (ret < 0)
			return gnutls_assert_val(0);

		if (san_type != GNUTLS_SAN_URI)
			continue;

		found_one = 1;
		break;
	} while(ret >= 0);

	if (found_one != 0)
		return check_unsupported_constraint(nc, type);

	/* no name was found in the certificate, so accept */
	return 1;
}

/**
 * gnutls_x509_name_constraints_check_crt:
 * @nc: the extracted name constraints
 * @type: the type of the constraint to check (of type gnutls_x509_subject_alt_name_t)
 * @cert: the certificate to be checked
 *
 * This function will check the provided certificate names against the constraints in
 * @nc using the RFC5280 rules. It will traverse all the certificate's names and
 * alternative names.
 *
 * Currently this function is limited to DNS
 * names and emails (of type %GNUTLS_SAN_DNSNAME and %GNUTLS_SAN_RFC822NAME).
 *
 * Returns: zero if the provided name is not acceptable, and non-zero otherwise.
 *
 * Since: 3.3.0
 **/
unsigned gnutls_x509_name_constraints_check_crt(gnutls_x509_name_constraints_t nc,
						gnutls_x509_subject_alt_name_t type,
						gnutls_x509_crt_t cert)
{
char name[MAX_CN];
size_t name_size;
int ret;
unsigned idx, t, san_type;
gnutls_datum_t n;
unsigned found_one;

	if (is_nc_empty(nc, type) != 0)
		return 1; /* shortcut; no constraints to check */

	if (type == GNUTLS_SAN_RFC822NAME) {
		idx = found_one = 0;
		do {
			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_subject_alt_name2(cert,
				idx++, name, &name_size, &san_type, NULL);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			else if (ret < 0)
				return gnutls_assert_val(0);

			if (san_type != GNUTLS_SAN_RFC822NAME)
				continue;

			found_one = 1;
			n.data = (void*)name;
			n.size = name_size;
			t = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME,
				&n);
			if (t == 0)
				return gnutls_assert_val(t);
		} while(ret >= 0);

		/* there is at least a single e-mail. That means that the EMAIL field will
		 * not be used for verifying the identity of the holder. */
		if (found_one != 0)
			return 1;

		do {
			/* ensure there is only a single EMAIL, similarly to CN handling (rfc6125) */
			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_PKCS9_EMAIL,
							    1, 0, name, &name_size);
			if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				return gnutls_assert_val(0);

			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_PKCS9_EMAIL,
							    0, 0, name, &name_size);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			else if (ret < 0)
				return gnutls_assert_val(0);

			found_one = 1;
			n.data = (void*)name;
			n.size = name_size;
			t = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &n);
			if (t == 0)
				return gnutls_assert_val(t);
		} while(0);

		/* passed */
		if (found_one != 0)
			return 1;
		else {
			/* no name was found. According to RFC5280: 
			 * If no name of the type is in the certificate, the certificate is acceptable.
			 */
			return gnutls_assert_val(1);
		}
	} else if (type == GNUTLS_SAN_DNSNAME) {
		idx = found_one = 0;
		do {
			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_subject_alt_name2(cert,
				idx++, name, &name_size, &san_type, NULL);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			else if (ret < 0)
				return gnutls_assert_val(0);

			if (san_type != GNUTLS_SAN_DNSNAME)
				continue;

			found_one = 1;
			n.data = (void*)name;
			n.size = name_size;
			t = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME,
				&n);
			if (t == 0)
				return gnutls_assert_val(t);
		} while(ret >= 0);

		/* there is at least a single DNS name. That means that the CN will
		 * not be used for verifying the identity of the holder. */
		if (found_one != 0)
			return 1;

		/* verify the name constraints against the CN, if the certificate is
		 * not a CA. We do this check only on certificates marked as WWW server,
		 * because that's where the CN check is only performed. */
		if (_gnutls_check_key_purpose(cert, GNUTLS_KP_TLS_WWW_SERVER, 0) != 0)
		do {
			/* ensure there is only a single CN, according to rfc6125 */
			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_X520_COMMON_NAME,
							    1, 0, name, &name_size);
			if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				return gnutls_assert_val(0);

			name_size = sizeof(name);
			ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_X520_COMMON_NAME,
							    0, 0, name, &name_size);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			else if (ret < 0)
				return gnutls_assert_val(0);

			found_one = 1;
			n.data = (void*)name;
			n.size = name_size;
			t = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME,
				&n);
			if (t == 0)
				return gnutls_assert_val(t);
		} while(0);

		/* passed */
		if (found_one != 0)
			return 1;
		else {
			/* no name was found. According to RFC5280: 
			 * If no name of the type is in the certificate, the certificate is acceptable.
			 */
			return gnutls_assert_val(1);
		}
	} else if (type == GNUTLS_SAN_IPADDRESS) {
			idx = found_one = 0;
			do {
				name_size = sizeof(name);
				ret = gnutls_x509_crt_get_subject_alt_name2(cert,
					idx++, name, &name_size, &san_type, NULL);
				if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
					break;
				else if (ret < 0)
					return gnutls_assert_val(0);

				if (san_type != GNUTLS_SAN_IPADDRESS)
					continue;

				found_one = 1;
				n.data = (void*)name;
				n.size = name_size;
				t = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, &n);
				if (t == 0)
					return gnutls_assert_val(t);
			} while(ret >= 0);

			/* there is at least a single IP address. */

			if (found_one != 0) {
				return 1;
			} else {
				/* no name was found. According to RFC5280:
				 * If no name of the type is in the certificate, the certificate is acceptable.
				 */
				return gnutls_assert_val(1);
			}
	} else if (type == GNUTLS_SAN_URI) {
		return check_unsupported_constraint2(cert, nc, type);
	} else
		return check_unsupported_constraint(nc, type);
}

/**
 * gnutls_x509_name_constraints_get_permitted:
 * @nc: the extracted name constraints
 * @idx: the index of the constraint
 * @type: the type of the constraint (of type gnutls_x509_subject_alt_name_t)
 * @name: the name in the constraint (of the specific type)
 *
 * This function will return an intermediate type containing
 * the name constraints of the provided CA certificate. That
 * structure can be used in combination with gnutls_x509_name_constraints_check()
 * to verify whether a server's name is in accordance with the constraints.
 *
 * The name should be treated as constant and valid for the lifetime of @nc.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_name_constraints_get_permitted(gnutls_x509_name_constraints_t nc,
					       unsigned idx,
					       unsigned *type, gnutls_datum_t * name)
{
	unsigned int i;
	struct name_constraints_node_st * tmp = nc->permitted;

	for (i = 0; i < idx; i++) {
		if (tmp == NULL)
			return
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

		tmp = tmp->next;
	}

	if (tmp == NULL)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	*type = tmp->type;
	*name = tmp->name;

	return 0;
}

/**
 * gnutls_x509_name_constraints_get_excluded:
 * @nc: the extracted name constraints
 * @idx: the index of the constraint
 * @type: the type of the constraint (of type gnutls_x509_subject_alt_name_t)
 * @name: the name in the constraint (of the specific type)
 *
 * This function will return an intermediate type containing
 * the name constraints of the provided CA certificate. That
 * structure can be used in combination with gnutls_x509_name_constraints_check()
 * to verify whether a server's name is in accordance with the constraints.
 *
 * The name should be treated as constant and valid for the lifetime of @nc.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_name_constraints_get_excluded(gnutls_x509_name_constraints_t nc,
					      unsigned idx,
					      unsigned *type, gnutls_datum_t * name)
{
	unsigned int i;
	struct name_constraints_node_st * tmp = nc->excluded;

	for (i = 0; i < idx; i++) {
		if (tmp == NULL)
			return
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

		tmp = tmp->next;
	}

	if (tmp == NULL)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	*type = tmp->type;
	*name = tmp->name;

	return 0;
}
