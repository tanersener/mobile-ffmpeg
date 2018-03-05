/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: David Marín Carreño
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", "crq_key_id", level, str);
}

static char plain_key[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEA2d3Qs4fYDkzojm9nbaz7vdX2GIAv3stAAmI0MWq7uxWcJ7lz\n"
"4UCvSh7CFWESMqOPcIymXXZB3rdrG6RQAHhE2YfFFFi0W/5rqcmkiTMnGmAjYy2Q\n"
"lCU87b18SQgjX27YLygvlbtZrveCgT2rbJYL57Gph+wIi+cJsEUanR/2fd12hL7k\n"
"RumZNEofamTHQm6Bn47ZhhCdVfs+QkfiMh+LHJM3fFiVusAUhMIJYhml53O+cSrR\n"
"VgWVdp4za/FdUzmDnSxG4heMHblRMS+UyqtssDZEaVHytiNQXFkuQ/U3v0E+2knR\n"
"NPKy+s7ZmSISC0qzDn6C5yeJc17Kc91y5RLoZwIDAQABAoIBAQCRXAu5HPOsZufq\n"
"0K2DYZz9BdqSckR+M8HbVUZZiksDAeIUJwoHyi6qF2eK+B86JiK4Bz+gsBw2ys3t\n"
"vW2bQqM9N/boIl8D2fZfbCgZWkXGtUonC+mgzk+el4Rq/cEMFVqr6/YDwuKNeJpc\n"
"PJc5dcsvpTvlcjgpj9bJAvJEz2SYiIUpvtG4WNMGGapVZZPDvWn4/isY+75T5oDf\n"
"1X5jG0lN9uoUjcuGuThN7gxjwlRkcvEOPHjXc6rxfrWIDdiz/91V46PwpqVDpRrg\n"
"ig6U7+ckS0Oy2v32x0DaDhwAfDJ2RNc9az6Z+11lmY3LPkjG/p8Klcmgvt4/lwkD\n"
"OYRC5QGRAoGBAPFdud6nmVt9h1DL0o4R6snm6P3K81Ds765VWVmpzJkK3+bwe4PQ\n"
"GQQ0I0zN4hXkDMwHETS+EVWllqkK/d4dsE3volYtyTti8zthIATlgSEJ81x/ChAQ\n"
"vvXxgx+zPUnb1mUwy+X+6urTHe4bxN2ypg6ROIUmT+Hx1ITG40LRRiPTAoGBAOcT\n"
"WR8DTrj42xbxAUpz9vxJ15ZMwuIpk3ShE6+CWqvaXHF22Ju4WFwRNlW2zVLH6UMt\n"
"nNfOzyDoryoiu0+0mg0wSmgdJbtCSHoI2GeiAnjGn5i8flQlPQ8bdwwmU6g6I/EU\n"
"QRbGK/2XLmlrGN52gVy9UX0NsAA5fEOsAJiFj1CdAoGBAN9i3nbq6O2bNVSa/8mL\n"
"XaD1vGe/oQgh8gaIaYSpuXlfbjCAG+C4BZ81XgJkfj3CbfGbDNqimsqI0fKsAJ/F\n"
"HHpVMgrOn3L+Np2bW5YMj0Fzwy+1SCvsQ8C+gJwjOLMV6syGp/+6udMSB55rRv3k\n"
"rPnIf+YDumUke4tTw9wAcgkPAoGASHMkiji7QfuklbjSsslRMyDj21gN8mMevH6U\n"
"cX7pduBsA5dDqu9NpPAwnQdHsSDE3i868d8BykuqQAfLut3hPylY6vPYlLHfj4Oe\n"
"dj+xjrSX7YeMBE34qvfth32s1R4FjtzO25keyc/Q2XSew4FcZftlxVO5Txi3AXC4\n"
"bxnRKXECgYEAva+og7/rK+ZjboJVNxhFrwHp9bXhz4tzrUaWNvJD2vKJ5ZcThHcX\n"
"zCig8W7eXHLPLDhi9aWZ3kUZ1RLhrFc/6dujtVtU9z2w1tmn1I+4Zi6D6L4DzKdg\n"
"nMRLFoXufs/qoaJTqa8sQvKa+ceJAF04+gGtw617cuaZdZ3SYRLR2dk=\n"
"-----END RSA PRIVATE KEY-----\n";

const char key1[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,82B2F7684A1713F8\n"
    "\n"
    "1zzOuu89dfFc2UkFCtSJBsBeEFxV8wE84OSxoWu4aYkPhl1LR08BchaTbjeLTP0b\n"
    "t961vVpva0ekJkwGDEgmqlGjmhJq9y2sJfq7IeYa8OdTilfGrG1xeJ1QGBi6SCfR\n"
    "s/PhkMxwGBtrZ2Z7bEcLT5dQKmKRqsthnClQggmngvk7zX7bPk0hKQKvf+FDxt6x\n"
    "hzEaF3k9juU6vAVVSakrZ4QDqk9MUuTGHx0ksTDcC4EESS0l3Ybuum/rAzR4lQKR\n"
    "4OLmAeYBDl+l/PSMllfd5x/z1YXYoiAbkpT4ix0lyZJgHrvrYIeUtJk2ODiMHezL\n"
    "9BbK7EobtOGmrDLUNVX5BpdaExkWMGkioqzs2QqD/VkKu8RcNSsHVGqkdWKuhzXo\n"
    "wcczQ+RiHckN2uy/zApubEWZNLPeDQ499kaF+QdZ+h4RM6E1r1Gu+A==\n"
    "-----END RSA PRIVATE KEY-----\n";

const char key2[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,2A57FF97B701B3F760145D7446929481\n"
    "\n"
    "mGAPhSw48wZBnkHOhfMDg8yL2IBgMuTmeKE4xoHi7T6isHBNfkqMd0iJ+DJP/OKb\n"
    "t+7lkKjj/xQ7w/bOBvBxlfRe4MW6+ejCdAFD9XSolW6WN6CEJPMI4UtmOK5inqcC\n"
    "8l2l54f/VGrVN9uavU3KlXCjrd3Jp9B0Mu4Zh/UU4+EWs9rJAZfLIn+vHZ3OHetx\n"
    "g74LdV7nC7lt/fjxc1caNIfgHs40dUt9FVrnJvAtkcNMtcjX/D+L8ZrLgQzIWFcs\n"
    "WAbUZj7Me22mCli3RPET7Je37K59IzfWgbWFCGaNu3X02g5xtCfdcn/Uqy9eofH0\n"
    "YjKRhpgXPeGJCkoRqDeUHQNPpVP5HrzDZMVK3E4DC03C8qvgsYvuwYt3KkbG2fuA\n"
    "F3bDyqlxSOm7uxF/K3YzI44v8/D8GGnLBTpN+ANBdiY=\n"
    "-----END RSA PRIVATE KEY-----\n";

void doit(void)
{
	gnutls_x509_privkey_t pkey;
	int ret;
	gnutls_datum_t key;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0)
		fail("gnutls_x509_privkey_init: %d\n", ret);

	key.data = (void *) key1;
	key.size = sizeof(key1);
	ret = gnutls_x509_privkey_import_openssl(pkey, &key, "123456");
	if (ret < 0) {
		fail("gnutls_x509_privkey_import_openssl (key1): %s\n",
		     gnutls_strerror(ret));
	}
	gnutls_x509_privkey_deinit(pkey);

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0)
		fail("gnutls_x509_privkey_init: %d\n", ret);

	key.data = (void *) key2;
	key.size = sizeof(key2);
	ret = gnutls_x509_privkey_import_openssl(pkey, &key, "a123456");
	if (ret < 0) {
		fail("gnutls_x509_privkey_import_openssl (key2): %s\n",
		     gnutls_strerror(ret));
	}

	gnutls_x509_privkey_deinit(pkey);

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0)
		fail("gnutls_x509_privkey_init: %d\n", ret);

	key.data = (void *) key1;
	key.size = sizeof(key1);
	ret =
	    gnutls_x509_privkey_import2(pkey, &key, GNUTLS_X509_FMT_PEM,
					"123456", 0);
	if (ret < 0) {
		fail("gnutls_x509_privkey_import2: %s\n",
		     gnutls_strerror(ret));
	}
	gnutls_x509_privkey_deinit(pkey);

	/* import a plain key */
	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0)
		fail("gnutls_x509_privkey_init: %d\n", ret);

	key.data = (void *) plain_key;
	key.size = sizeof(plain_key);
	ret =
	    gnutls_x509_privkey_import2(pkey, &key, GNUTLS_X509_FMT_PEM,
					"123456", 0);
	if (ret < 0) {
		fail("gnutls_x509_privkey_import2: %s\n",
		     gnutls_strerror(ret));
	}
	gnutls_x509_privkey_deinit(pkey);

	gnutls_global_deinit();
}
