/*
 * Copyright (C) 2012 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of libdane.
 *
 * libdane is free software; you can redistribute it and/or
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

#include <config.h>
#include <gnutls/dane.h>

/* I18n of error codes. */
#include "gettext.h"
#define _(String) dgettext (PACKAGE, String)
#define N_(String) gettext_noop (String)

#define ERROR_ENTRY(desc, name) \
	{ desc, #name, name}

struct error_entry {
	const char *desc;
	const char *_name;
	int number;
};
typedef struct error_entry error_entry;

static const error_entry error_algorithms[] = {
	ERROR_ENTRY(N_("Success."), DANE_E_SUCCESS),
	ERROR_ENTRY(N_("There was error initializing the DNS query."),
		    DANE_E_INITIALIZATION_ERROR),
	ERROR_ENTRY(N_("There was an error while resolving."),
		    DANE_E_RESOLVING_ERROR),
	ERROR_ENTRY(N_("No DANE data were found."),
		    DANE_E_NO_DANE_DATA),
	ERROR_ENTRY(N_("Unknown DANE data were found."),
		    DANE_E_UNKNOWN_DANE_DATA),
	ERROR_ENTRY(N_("No DNSSEC signature was found."),
		    DANE_E_NO_DNSSEC_SIG),
	ERROR_ENTRY(N_("Received corrupt data."),
		    DANE_E_RECEIVED_CORRUPT_DATA),
	ERROR_ENTRY(N_("The DNSSEC signature is invalid."),
		    DANE_E_INVALID_DNSSEC_SIG),
	ERROR_ENTRY(N_("There was a memory error."),
		    DANE_E_MEMORY_ERROR),
	ERROR_ENTRY(N_("The requested data are not available."),
		    DANE_E_REQUESTED_DATA_NOT_AVAILABLE),
	ERROR_ENTRY(N_("The request is invalid."),
		    DANE_E_INVALID_REQUEST),
	ERROR_ENTRY(N_("There was an error in the certificate."),
		    DANE_E_CERT_ERROR),
	ERROR_ENTRY(N_("There was an error in the public key."),
		    DANE_E_PUBKEY_ERROR),
	ERROR_ENTRY(N_("No certificate was found."),
		    DANE_E_NO_CERT),
	ERROR_ENTRY(N_("Error in file."),
		    DANE_E_FILE_ERROR),
	{NULL, NULL, 0}
};

/**
 * dane_strerror:
 * @error: is a DANE error code, a negative error code
 *
 * This function is similar to strerror.  The difference is that it
 * accepts an error number returned by a gnutls function; In case of
 * an unknown error a descriptive string is sent instead of %NULL.
 *
 * Error codes are always a negative error code.
 *
 * Returns: A string explaining the DANE error message.
 **/
const char *dane_strerror(int error)
{
	const char *ret = NULL;
	const error_entry *p;

	for (p = error_algorithms; p->desc != NULL; p++) {
		if (p->number == error) {
			ret = p->desc;
			break;
		}
	}

	/* avoid prefix */
	if (ret == NULL)
		return _("(unknown error code)");

	return _(ret);
}
