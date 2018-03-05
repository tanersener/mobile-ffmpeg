/* CC0 license (public domain) - see LICENSE file for details */
#include <config.h>
#include <hex.h>
#include <stdio.h>
#include <stdlib.h>

static bool char_to_hex(unsigned char *val, char c)
{
	if (c >= '0' && c <= '9') {
		*val = c - '0';
		return true;
	}
	if (c >= 'a' && c <= 'f') {
		*val = c - 'a' + 10;
		return true;
	}
	if (c >= 'A' && c <= 'F') {
		*val = c - 'A' + 10;
		return true;
	}
	return false;
}

bool hex_decode(const char *str, size_t slen, void *buf, size_t bufsize)
{
	unsigned char v1, v2;
	unsigned char *p = buf;

	while (slen > 1) {
		if (!char_to_hex(&v1, str[0]) || !char_to_hex(&v2, str[1]))
			return false;
		if (!bufsize)
			return false;
		*(p++) = (v1 << 4) | v2;
		str += 2;
		slen -= 2;
		bufsize--;
	}
	return slen == 0 && bufsize == 0;
}

static char hexchar(unsigned int val)
{
	if (val < 10)
		return '0' + val;
	if (val < 16)
		return 'a' + val - 10;
	abort();
}

bool hex_encode(const void *buf, size_t bufsize, char *dest, size_t destsize)
{
	size_t used = 0;

	if (destsize < 1)
		return false;

	while (used < bufsize) {
		unsigned int c = ((const unsigned char *)buf)[used];
		if (destsize < 3)
			return false;
		*(dest++) = hexchar(c >> 4);
		*(dest++) = hexchar(c & 0xF);
		used++;
		destsize -= 2;
	}
	*dest = '\0';

	return used + 1;
}
