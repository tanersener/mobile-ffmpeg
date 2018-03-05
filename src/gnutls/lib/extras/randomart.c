/* $OpenBSD: key.c,v 1.98 2011/10/18 04:58:26 djm Exp $ */
/*
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2008 Alexander von Gernler.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <randomart.h>

/*
 * Draw an ASCII-Art representing the fingerprint so human brain can
 * profit from its built-in pattern recognition ability.
 * This technique is called "random art" and can be found in some
 * scientific publications like this original paper:
 *
 * "Hash Visualization: a New Technique to improve Real-World Security",
 * Perrig A. and Song D., 1999, International Workshop on Cryptographic
 * Techniques and E-Commerce (CrypTEC '99)
 * sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf
 *
 * The subject came up in a talk by Dan Kaminsky, too.
 *
 * If you see the picture is different, the key is different.
 * If the picture looks the same, you still know nothing.
 *
 * The algorithm used here is a worm crawling over a discrete plane,
 * leaving a trace (augmenting the field) everywhere it goes.
 * Movement is taken from dgst_raw 2bit-wise.  Bumping into walls
 * makes the respective movement vector be ignored for this turn.
 * Graphs are not unambiguous, because circles in graphs can be
 * walked in either direction.
 */

/*
 * Field sizes for the random art.  Have to be odd, so the starting point
 * can be in the exact middle of the picture, and FLDBASE should be >=8 .
 * Else pictures would be too dense, and drawing the frame would
 * fail, too, because the key type would not fit in anymore.
 */
#define	FLDBASE		8
#define	FLDSIZE_Y	(FLDBASE + 1)
#define	FLDSIZE_X	(FLDBASE * 2 + 1)
char *_gnutls_key_fingerprint_randomart(uint8_t * dgst_raw,
					u_int dgst_raw_len,
					const char *key_type,
					unsigned int key_size,
					const char *prefix)
{
	/*
	 * Chars to be used after each other every time the worm
	 * intersects with itself.  Matter of taste.
	 */
	const char augmentation_string[] = " .o+=*BOX@%&#/^SE";
	char *retval, *p;
	uint8_t field[FLDSIZE_X][FLDSIZE_Y];
	char size_txt[16];
	unsigned int i, b;
	int x, y;
	const size_t len = sizeof(augmentation_string) - 2;
	unsigned int prefix_len = 0;

	if (prefix)
		prefix_len = strlen(prefix);

	retval =
	    gnutls_calloc(1,
			  (FLDSIZE_X + 3 + prefix_len) * (FLDSIZE_Y + 2));
	if (retval == NULL) {
		gnutls_assert();
		return NULL;
	}

	/* initialize field */
	memset(field, 0, FLDSIZE_X * FLDSIZE_Y * sizeof(char));
	x = FLDSIZE_X / 2;
	y = FLDSIZE_Y / 2;

	/* process raw key */
	for (i = 0; i < dgst_raw_len; i++) {
		int input;
		/* each byte conveys four 2-bit move commands */
		input = dgst_raw[i];
		for (b = 0; b < 4; b++) {
			/* evaluate 2 bit, rest is shifted later */
			x += (input & 0x1) ? 1 : -1;
			y += (input & 0x2) ? 1 : -1;

			/* assure we are still in bounds */
			x = MAX(x, 0);
			y = MAX(y, 0);
			x = MIN(x, FLDSIZE_X - 1);
			y = MIN(y, FLDSIZE_Y - 1);

			/* augment the field */
			if (field[x][y] < len - 2)
				field[x][y]++;
			input = input >> 2;
		}
	}

	/* mark starting point and end point */
	field[FLDSIZE_X / 2][FLDSIZE_Y / 2] = len - 1;
	field[x][y] = len;

	if (key_size > 0)
		snprintf(size_txt, sizeof(size_txt), " %4u", key_size);
	else
		size_txt[0] = 0;

	/* fill in retval */
	if (prefix_len)
		snprintf(retval, FLDSIZE_X + prefix_len, "%s+--[%4s%s]",
			 prefix, key_type, size_txt);
	else
		snprintf(retval, FLDSIZE_X, "+--[%4s%s]", key_type,
			 size_txt);
	p = strchr(retval, '\0');

	/* output upper border */
	for (i = p - retval - 1; i < FLDSIZE_X + prefix_len; i++)
		*p++ = '-';
	*p++ = '+';
	*p++ = '\n';

	if (prefix_len) {
		memcpy(p, prefix, prefix_len);
		p += prefix_len;
	}

	/* output content */
	for (y = 0; y < FLDSIZE_Y; y++) {
		*p++ = '|';
		for (x = 0; x < FLDSIZE_X; x++)
			*p++ = augmentation_string[MIN(field[x][y], len)];
		*p++ = '|';
		*p++ = '\n';

		if (prefix_len) {
			memcpy(p, prefix, prefix_len);
			p += prefix_len;
		}
	}

	/* output lower border */
	*p++ = '+';
	for (i = 0; i < FLDSIZE_X; i++)
		*p++ = '-';
	*p++ = '+';

	return retval;
}
