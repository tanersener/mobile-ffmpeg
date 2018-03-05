/*
 * Copyright (C) 2000-2014 Free Software Foundation, Inc.
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>		/* for strverscmp */

#include "libtasn1.h"

/**
 * asn1_check_version:
 * @req_version: Required version number, or %NULL.
 *
 * Check that the version of the library is at minimum the
 * requested one and return the version string; return %NULL if the
 * condition is not satisfied.  If a %NULL is passed to this function,
 * no check is done, but the version string is simply returned.
 *
 * See %ASN1_VERSION for a suitable @req_version string.
 *
 * Returns: Version string of run-time library, or %NULL if the
 *   run-time library does not meet the required version number.
 */
const char *
asn1_check_version (const char *req_version)
{
  if (!req_version || strverscmp (req_version, ASN1_VERSION) <= 0)
    return ASN1_VERSION;

  return NULL;
}
