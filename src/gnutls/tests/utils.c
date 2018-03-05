/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <windows.h>		/* for Sleep */
#include <winbase.h>
#endif

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include <sys/types.h>

#include "utils.h"

int debug = 0;
int error_count = 0;
int break_on_error = 0;

const char *pkcs3 =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIGGAoGAtkxw2jlsVCsrfLqxrN+IrF/3W8vVFvDzYbLmxi2GQv9s/PQGWP1d9i22\n"
    "P2DprfcJknWt7KhCI1SaYseOQIIIAYP78CfyIpGScW/vS8khrw0rlQiyeCvQgF3O\n"
    "GeGOEywcw+oQT4SmFOD7H0smJe2CNyjYpexBXQ/A0mbTF9QKm1cCAQU=\n"
    "-----END DH PARAMETERS-----\n";

const char *pkcs3_2048 =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIICDgKCAQEAvVNCqM8M9ZoVYBKEkV2KN8ELHHJ75aTZiK9z6170iKSgbITkOxsd\n"
    "aBCLzHZd7d6/2aNofUeuWdDGHm73d8v53ma2HRVCNESeC2LKsEDFG9FjjUeugvfl\n"
    "zb85TLZwWT9Lb35Ddhdk7CtxoukjS0/JkCE+8RGzmk5+57N8tNffs4aSSHSe4+cw\n"
    "i4wULDxiG2p052czAMP3YR5egWvMuiByhy0vKShiZmOy1/Os5r6E/GUF+298gDjG\n"
    "OeaEUF9snrTcoBwB4yNjVSEbuAh5fMd5zFtz2+dzrk9TYZ44u4DQYkgToW05WcmC\n"
    "+LG0bLAH6lrJR5OMgyheZEo6F20z/d2yyQKCAQEAtzcuTHW61SFQiDRouk6eD0Yx\n"
    "0k1RJdaQdlRf6/Dcc6lEqnbezL90THzvxkBwfJ5jG1VZE7JlVCvLRkBtgb0/6SCf\n"
    "MATfEKG2JMOnKsJxvidmKEp4uN32LketXRrrEBl7rS+HABEfKAzqx+J6trBaq25E\n"
    "7FVJFsyoa8IL8N8YUWwhE2UuEfmiqQQaeoIUYC/xD2arMXn9N0W84Nyy2S9IL4ct\n"
    "e3Azi1Wc8MMfpbxxDRxXCnM2uMkLYWs1lQmcUUX+Uygv3P8lgS+RJ1Pi3+BWMx0S\n"
    "ocsZXqOr6dbEF1WOLObQRK7h/MZp80iVUyrBgX0MbVFN9M5i2u4KKTG95VKRtgIC\n"
    "AQA=\n" "-----END DH PARAMETERS-----\n";

const char *pkcs3_3072 =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIDDgKCAYEAtRUay8nDgwE5dSVzW525wEu/d0vrFolvYJSevxg2myj5S+gr3Fgq\n"
    "OGaZc4zrBxkxsELc7GuCqaXSOWL4yobT8N05yGbYWkWRPf4crRMx3P7/Gba9WsmH\n"
    "BlL71uPf1IN9CanAlabkhV89RKiYaCpUI19+/sq+N2dO874ToBZCNhxZnTgRZ+po\n"
    "Gdr6XWM0lQ8imIKSer0px3ZHI+/5gmyPry35tGpwlbyclJAg3wlTSdnqDcLxq7AF\n"
    "OZ23PzC3ij7SFErOX9EFBdS2bjtU47O3OkPc9EIYMEv5nwnXICLHslwVifmURAjV\n"
    "LfpObL8LYGN4Gac4tFxuDa0PMg0ES5ADugYBwdRFTAtCy5WOYXINzAAOrH9MommT\n"
    "rMkELf7JOCaV2ktBsvTlrgMAXeyqbf2YSG6CGjj4QnUuqPybSgwPru7VlahsS2lo\n"
    "qjutBPpgIxS53o97Wi3V5kQedKJiNuIDNnJMFNuTADAM+OYwClTH7ZSwTsxEgVpr\n"
    "tMH+WnTI7KTJAoIBgQCrELwIUB4oNbf0x+fIpVndhDpl/WcFc/lDtmiRuym5gWbb\n"
    "NPeI+1rdhnS2R3+nCJODFQTcPNMgIJuSu2EnDCSs5xJ2k08SAgSzyxEdjBpY7qJe\n"
    "+lJPJ12zhcl0vgcvMhb/YgqVe2MKz0RvnYZPwHM/aJbjYjq/6OpK3fVw4M1ZccBK\n"
    "QD4OHK8HOvGU7Wf6kRIcxUlfn15spMCIsrAZQBddWLmQgktsxJNUS+AnaPwTBoOv\n"
    "nGCr1vzw8OS1DtS03VCmtqt3otXhJ3D2oCIG6ogxVAKfHR30KIfzZLBfmCjdzHmH\n"
    "x4OwYTN1wy5juA438QtiDtcgK60ZqSzQO08ZklRncA/TkkyEH6kPn5KSh/hW9O3D\n"
    "KZeAY/KF0/Bc1XNtqPEYFb7Vo3rbTsyjXkICN1Hk9S0OIKL42K7rWBepO9KuddSd\n"
    "aXgH9staP0HXCyyW1VAyqo0TwcWDhE/R7IQQGGwGyd4rD0T+ySW/t09ox23O6X8J\n"
    "FSp6mOVNcuvhB5U2gW8CAgEA\n" "-----END DH PARAMETERS-----\n";

void _fail(const char *format, ...)
{
	va_list arg_ptr;

	va_start(arg_ptr, format);
#ifdef HAVE_VASPRINTF
	char *str = NULL;
	vasprintf(&str, format, arg_ptr);

	if (str)
		fputs(str, stderr);
#else
	{
		char str[1024];

		vsnprintf(str, sizeof(str), format, arg_ptr);
		fputs(str, stderr);
	}
#endif
	va_end(arg_ptr);
	error_count++;
	exit(1);
}

void fail_ignore(const char *format, ...)
{
	char str[1024];
	va_list arg_ptr;

	va_start(arg_ptr, format);
	vsnprintf(str, sizeof(str), format, arg_ptr);
	va_end(arg_ptr);
	fputs(str, stderr);
	error_count++;
	exit(77);
}

void sec_sleep(int sec)
{
	int ret;
#ifdef HAVE_NANOSLEEP
	struct timespec ts;

	ts.tv_sec = sec;
	ts.tv_nsec = 0;
	do {
		ret = nanosleep(&ts, NULL);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1)
		abort();
#else
	do {
		ret = sleep(sec);
	} while (ret == -1 && errno == EINTR);
#endif
}

void success(const char *format, ...)
{
	char str[1024];
	va_list arg_ptr;

	va_start(arg_ptr, format);
	vsnprintf(str, sizeof(str), format, arg_ptr);
	va_end(arg_ptr);
	fputs(str, stderr);
}

void escapeprint(const char *str, size_t len)
{
	size_t i;

	printf(" (length %d bytes):\n\t'", (int)len);
	for (i = 0; i < len; i++) {
		if (((str[i] & 0xFF) >= 'A' && (str[i] & 0xFF) <= 'Z') ||
		    ((str[i] & 0xFF) >= 'a' && (str[i] & 0xFF) <= 'z') ||
		    ((str[i] & 0xFF) >= '0' && (str[i] & 0xFF) <= '9')
		    || (str[i] & 0xFF) == ' ' || (str[i] & 0xFF) == '.')
			printf("%c", (str[i] & 0xFF));
		else
			printf("\\x%02X", (str[i] & 0xFF));
		if ((i + 1) % 16 == 0 && (i + 1) < len)
			printf("'\n\t'");
	}
	printf("\n");
}

void c_print(const unsigned char *str, size_t len)
{
	size_t i;

	printf(" (length %d bytes):\n\t\"", (int)len);
	for (i = 0; i < len; i++) {
		printf("\\x%02X", (str[i] & 0xFF));
		if ((i + 1) % 16 == 0 && (i + 1) < len)
			printf("\"\n\t\"");
	}
	printf("\"\n");
}

void hexprint(const void *_str, size_t len)
{
	size_t i;
	const char *str = _str;

	printf("\t;; ");
	for (i = 0; i < len; i++) {
		printf("%02x ", (str[i] & 0xFF));
		if ((i + 1) % 8 == 0)
			printf(" ");
		if ((i + 1) % 16 == 0 && i + 1 < len)
			printf("\n\t;; ");
	}
	printf("\n");
}

void binprint(const void *_str, size_t len)
{
	size_t i;
	const char *str = _str;

	printf("\t;; ");
	for (i = 0; i < len; i++) {
		printf("%d%d%d%d%d%d%d%d ",
			(str[i] & 0xFF) & 0x80 ? 1 : 0,
			(str[i] & 0xFF) & 0x40 ? 1 : 0,
			(str[i] & 0xFF) & 0x20 ? 1 : 0,
			(str[i] & 0xFF) & 0x10 ? 1 : 0,
			(str[i] & 0xFF) & 0x08 ? 1 : 0,
			(str[i] & 0xFF) & 0x04 ? 1 : 0,
			(str[i] & 0xFF) & 0x02 ? 1 : 0,
			(str[i] & 0xFF) & 0x01 ? 1 : 0);
		if ((i + 1) % 3 == 0)
			printf(" ");
		if ((i + 1) % 6 == 0 && i + 1 < len)
			printf("\n\t;; ");
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	do
		if (strcmp(argv[argc - 1], "-v") == 0 ||
		    strcmp(argv[argc - 1], "--verbose") == 0)
			debug = 1;
		else if (strcmp(argv[argc - 1], "-b") == 0 ||
			 strcmp(argv[argc - 1], "--break-on-error") == 0)
			break_on_error = 1;
		else if (strcmp(argv[argc - 1], "-h") == 0 ||
			 strcmp(argv[argc - 1], "-?") == 0 ||
			 strcmp(argv[argc - 1], "--help") == 0) {
			printf
			    ("Usage: %s [-vbh?] [--verbose] [--break-on-error] [--help]\n",
			     argv[0]);
			return 1;
		}
	while (argc-- > 1) ;

	doit();

	if (debug || error_count > 0)
		printf("Self test `%s' finished with %d errors\n", argv[0],
			error_count);

	return error_count ? 1 : 0;
}

struct tmp_file_st {
	char file[TMPNAME_SIZE];
	struct tmp_file_st *next;
};

static struct tmp_file_st *temp_files = (void*)-1;

static void append(const char *file)
{
	struct tmp_file_st *p;

	if (temp_files == (void*)-1)
		return;

	p = calloc(1, sizeof(*p));

	assert(p != NULL);
	snprintf(p->file, sizeof(p->file), "%s", file);
	p->next = temp_files;
	temp_files = p;
}

char *get_tmpname(char s[TMPNAME_SIZE])
{
	unsigned char rnd[6];
	static char _s[TMPNAME_SIZE];
	int ret;
	char *p;
	const char *path;

	ret = gnutls_rnd(GNUTLS_RND_NONCE, rnd, sizeof(rnd));
	if (ret < 0)
		return NULL;

	path = getenv("builddir");
	if (path == NULL)
		path = ".";

	if (s == NULL)
		p = _s;
	else
		p = s;

	snprintf(p, TMPNAME_SIZE, "%s/tmpfile-%02x%02x%02x%02x%02x%02x.tmp", path, (unsigned)rnd[0], (unsigned)rnd[1],
		(unsigned)rnd[2], (unsigned)rnd[3], (unsigned)rnd[4], (unsigned)rnd[5]);

	append(p);

	return p;
}

void track_temp_files(void)
{
	temp_files = NULL;
}

void delete_temp_files(void)
{
	struct tmp_file_st *p = temp_files;
	struct tmp_file_st *next;

	if (p == (void*)-1)
		return;

	while(p != NULL) {
		next = p->next;
		free(p);
		p = next;
	}
}
