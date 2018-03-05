/*
 * Copyright (C) 2015 Red Hat, Inc.
 *
 * Author: Daniel Berrange
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

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

#define PRIVATE_KEY \
	"-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
	"MIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAiebBrnqPv4owICCAAw\n" \
	"HQYJYIZIAWUDBAEqBBBykFR6i1My/DYFBYrz1lmABIGQ3XGpp3+v/ENC1S+X7Ay6\n" \
	"JoquYKuMw6yUmWoGFvPIPA9UWqMve2Uj4l2l96Sywd6iNFP63ow6pIq4wUP6REuY\n" \
	"ZhCgoAOQomeFqhAhkw6QJCygp5vw2rh9OZ5tiP/Ko6IDTA2rSas91nepHpQOb247\n" \
	"zta5XzXb5TRkBsVU8tAPADP+wS/vBCS05ne1wmhdD6c6\n" \
	"-----END ENCRYPTED PRIVATE KEY-----\n"


static int test_decode(void)
{
  gnutls_x509_privkey_t key;
  const gnutls_datum_t data = {
    (unsigned char *)PRIVATE_KEY,
    strlen(PRIVATE_KEY)
  };
  int err;

  if ((err = gnutls_x509_privkey_init(&key)) < 0) {
    fail("Failed to init key %s\n", gnutls_strerror(err));
  }

  err = gnutls_x509_privkey_import_pkcs8(key, &data,
					GNUTLS_X509_FMT_PEM, "", 0);
  if (err != GNUTLS_E_DECRYPTION_FAILED) {
    fail("Unexpected error code: %s/%d\n", gnutls_strerror(err), err);
  }

  err = gnutls_x509_privkey_import_pkcs8(key, &data,
					GNUTLS_X509_FMT_PEM, "password", 0);
  if (err != 0) {
    fail("Unexpected error code: %s\n", gnutls_strerror(err));
  }

  success("Loaded key\n%s", PRIVATE_KEY);

  gnutls_x509_privkey_deinit(key);
  return 0;
}

void doit(void)
{
	test_decode();
}
