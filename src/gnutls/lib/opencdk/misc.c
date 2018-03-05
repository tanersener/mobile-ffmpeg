/* misc.c
 * Copyright (C) 1998-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <c-ctype.h>
#include <opencdk.h>
#include <main.h>
#include <random.h>
#include "gnutls_int.h"
#include <str.h>


u32 _cdk_buftou32(const byte * buf)
{
	u32 u;

	if (!buf)
		return 0;
	u = ((u32)buf[0]) << 24;
	u |= buf[1] << 16;
	u |= buf[2] << 8;
	u |= buf[3];
	return u;
}


void _cdk_u32tobuf(u32 u, byte * buf)
{
	if (!buf)
		return;
	buf[0] = u >> 24;
	buf[1] = u >> 16;
	buf[2] = u >> 8;
	buf[3] = u;
}

/**
 * cdk_strlist_free:
 * @sl: the string list
 * 
 * Release the string list object.
 **/
void cdk_strlist_free(cdk_strlist_t sl)
{
	cdk_strlist_t sl2;

	for (; sl; sl = sl2) {
		sl2 = sl->next;
		cdk_free(sl);
	}
}


/**
 * cdk_strlist_add:
 * @list: destination string list
 * @string: the string to add
 * 
 * Add the given list to the string list.
 **/
cdk_strlist_t cdk_strlist_add(cdk_strlist_t * list, const char *string)
{
	if (!string)
		return NULL;
	
	cdk_strlist_t sl;
	int string_size = strlen(string);

	sl = cdk_calloc(1, sizeof *sl + string_size + 2);
	if (!sl)
		return NULL;
	sl->d = (char *) sl + sizeof(*sl);
	memcpy(sl->d, string, string_size + 1);
	sl->next = *list;
	*list = sl;
	return sl;
}

const char *_cdk_memistr(const char *buf, size_t buflen, const char *sub)
{
	const byte *t, *s;
	size_t n;

	for (t = (byte *) buf, n = buflen, s = (byte *) sub; n; t++, n--) {
		if (c_toupper(*t) == c_toupper(*s)) {
			for (buf = (char *) t++, buflen = n--, s++;
			     n && c_toupper(*t) == c_toupper((byte) * s);
			     t++, s++, n--);
			if (!*s)
				return buf;
			t = (byte *) buf;
			n = buflen;
			s = (byte *) sub;
		}
	}

	return NULL;
}

cdk_error_t _cdk_map_gnutls_error(int err)
{
	switch (err) {
	case 0:
		return CDK_Success;
	case GNUTLS_E_INVALID_REQUEST:
		return CDK_Inv_Value;
	default:
		return CDK_General_Error;
	}
}


int _cdk_check_args(int overwrite, const char *in, const char *out)
{
	struct stat stbuf;

	if (!in || !out)
		return CDK_Inv_Value;
	if (strlen(in) == strlen(out) && strcmp(in, out) == 0)
		return CDK_Inv_Mode;
	if (!overwrite && !stat(out, &stbuf))
		return CDK_Inv_Mode;
	return 0;
}

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

FILE *_cdk_tmpfile(void)
{
	/* Because the tmpfile() version of wine is not really useful,
	   we implement our own version to avoid problems with 'make check'. */
	static const char *letters = "abcdefghijklmnopqrstuvwxyz";
	unsigned char buf[512], rnd[24];
	FILE *fp;
	int fd, i;

	gnutls_rnd(GNUTLS_RND_NONCE, rnd, DIM(rnd));
	for (i = 0; i < DIM(rnd) - 1; i++) {
		char c = letters[(unsigned char) rnd[i] % 26];
		rnd[i] = c;
	}
	rnd[DIM(rnd) - 1] = 0;
	if (!GetTempPath(464, buf))
		return NULL;
	_gnutls_str_cat(buf, sizeof(buf), "_cdk_");
	_gnutls_str_cat(buf, sizeof(buf), rnd);

	/* We need to make sure the file will be deleted when it is closed. */
	fd = _open(buf, _O_CREAT | _O_EXCL | _O_TEMPORARY |
		   _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
	if (fd == -1)
		return NULL;
	fp = fdopen(fd, "w+b");
	if (fp != NULL)
		return fp;
	_close(fd);
	return NULL;
}
#else
FILE *_cdk_tmpfile(void)
{
	return tmpfile();
}
#endif

int _gnutls_hash_algo_to_pgp(int algo)
{
	switch (algo) {
	case GNUTLS_DIG_MD5:
		return 0x01;
	case GNUTLS_DIG_MD2:
		return 0x05;
	case GNUTLS_DIG_SHA1:
		return 0x02;
	case GNUTLS_DIG_RMD160:
		return 0x03;
	case GNUTLS_DIG_SHA256:
		return 0x08;
	case GNUTLS_DIG_SHA384:
		return 0x09;
	case GNUTLS_DIG_SHA512:
		return 0x0A;
	case GNUTLS_DIG_SHA224:
		return 0x0B;
	default:
		gnutls_assert();
		return 0x00;
	}
}

int _pgp_hash_algo_to_gnutls(int algo)
{
	switch (algo) {
	case 0x01:
		return GNUTLS_DIG_MD5;
	case 0x02:
		return GNUTLS_DIG_SHA1;
	case 0x03:
		return GNUTLS_DIG_RMD160;
	case 0x05:
		return GNUTLS_DIG_MD2;
	case 0x08:
		return GNUTLS_DIG_SHA256;
	case 0x09:
		return GNUTLS_DIG_SHA384;
	case 0x0A:
		return GNUTLS_DIG_SHA512;
	case 0x0B:
		return GNUTLS_DIG_SHA224;
	default:
		gnutls_assert();
		return GNUTLS_DIG_NULL;
	}
}

int _pgp_cipher_to_gnutls(int cipher)
{
	switch (cipher) {
	case 0:
		return GNUTLS_CIPHER_NULL;
	case 1:
		return GNUTLS_CIPHER_IDEA_PGP_CFB;
	case 2:
		return GNUTLS_CIPHER_3DES_PGP_CFB;
	case 3:
		return GNUTLS_CIPHER_CAST5_PGP_CFB;
	case 4:
		return GNUTLS_CIPHER_BLOWFISH_PGP_CFB;
	case 5:
		return GNUTLS_CIPHER_SAFER_SK128_PGP_CFB;
	case 7:
		return GNUTLS_CIPHER_AES128_PGP_CFB;
	case 8:
		return GNUTLS_CIPHER_AES192_PGP_CFB;
	case 9:
		return GNUTLS_CIPHER_AES256_PGP_CFB;
	case 10:
		return GNUTLS_CIPHER_TWOFISH_PGP_CFB;

	default:
		gnutls_assert();
		_gnutls_debug_log("Unknown openpgp cipher %u\n", cipher);
		return GNUTLS_CIPHER_UNKNOWN;
	}
}

int _gnutls_cipher_to_pgp(int cipher)
{
	switch (cipher) {
	case GNUTLS_CIPHER_NULL:
		return 0;
	case GNUTLS_CIPHER_IDEA_PGP_CFB:
		return 1;
	case GNUTLS_CIPHER_3DES_PGP_CFB:
		return 2;
	case GNUTLS_CIPHER_CAST5_PGP_CFB:
		return 3;
	case GNUTLS_CIPHER_BLOWFISH_PGP_CFB:
		return 4;
	case GNUTLS_CIPHER_SAFER_SK128_PGP_CFB:
		return 5;
	case GNUTLS_CIPHER_AES128_PGP_CFB:
		return 7;
	case GNUTLS_CIPHER_AES192_PGP_CFB:
		return 8;
	case GNUTLS_CIPHER_AES256_PGP_CFB:
		return 9;
	case GNUTLS_CIPHER_TWOFISH_PGP_CFB:
		return 10;
	default:
		gnutls_assert();
		return 0;
	}
}
