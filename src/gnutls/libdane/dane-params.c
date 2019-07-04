/*
 * Copyright (C) 2012 KU Leuven
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of libdane.
 *
 * The libdane library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unbound.h>
#include <gnutls/dane.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

typedef struct cert_type_entry {
	const char *name;
	dane_cert_type_t type;
} cert_type_entry;

static const cert_type_entry dane_cert_types[] = {
	{"X.509", DANE_CERT_X509},
	{"SubjectPublicKeyInfo", DANE_CERT_PK},
	{NULL, 0}
};

typedef struct match_type_entry {
	const char *name;
	dane_match_type_t type;
} match_type_entry;

static const match_type_entry dane_match_types[] = {
	{"Exact match", DANE_MATCH_EXACT},
	{"SHA2-256 hash", DANE_MATCH_SHA2_256},
	{"SHA2-512 hash", DANE_MATCH_SHA2_512},
	{NULL, 0}
};

typedef struct cert_usage_entry {
	const char *name;
	dane_cert_usage_t usage;
} cert_usage_entry;

static const cert_usage_entry dane_cert_usages[] = {
	{"CA", DANE_CERT_USAGE_CA},
	{"End-entity", DANE_CERT_USAGE_EE},
	{"Local CA", DANE_CERT_USAGE_LOCAL_CA},
	{"Local end-entity", DANE_CERT_USAGE_LOCAL_EE},
	{NULL, 0}
};



/**
 * dane_cert_type_name:
 * @type: is a DANE match type
 *
 * Convert a #dane_cert_type_t value to a string.
 *
 * Returns: a string that contains the name of the specified
 *   type, or %NULL.
 **/
const char *dane_cert_type_name(dane_cert_type_t type)
{
	const cert_type_entry *e = dane_cert_types;

	while (e->name != NULL) {
		if (e->type == type)
			return e->name;
		e++;
	}

	return NULL;
}

/**
 * dane_match_type_name:
 * @type: is a DANE match type
 *
 * Convert a #dane_match_type_t value to a string.
 *
 * Returns: a string that contains the name of the specified
 *   type, or %NULL.
 **/
const char *dane_match_type_name(dane_match_type_t type)
{
	const match_type_entry *e = dane_match_types;

	while (e->name != NULL) {
		if (e->type == type)
			return e->name;
		e++;
	}

	return NULL;
}

/**
 * dane_cert_usage_name:
 * @usage: is a DANE certificate usage
 *
 * Convert a #dane_cert_usage_t value to a string.
 *
 * Returns: a string that contains the name of the specified
 *   type, or %NULL.
 **/
const char *dane_cert_usage_name(dane_cert_usage_t usage)
{
	const cert_usage_entry *e = dane_cert_usages;

	while (e->name != NULL) {
		if (e->usage == usage)
			return e->name;
		e++;
	}

	return NULL;

}
