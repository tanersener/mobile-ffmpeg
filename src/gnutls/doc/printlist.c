/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/openpgp.h>
#include "common.h"

static void main_texinfo(void);
static void main_latex(void);

char buffer[1024];

int main(int argc, char *argv[])
{
	if (argc > 1)
		main_latex();
	else
		main_texinfo();

	return 0;
}

static void main_texinfo(void)
{
	{
		size_t i;
		const char *name;
		char id[2];
		gnutls_kx_algorithm_t kx;
		gnutls_cipher_algorithm_t cipher;
		gnutls_mac_algorithm_t mac;
		gnutls_protocol_t version;

		printf("@heading Ciphersuites\n");
		printf("@multitable @columnfractions .60 .20 .20\n");
		printf
		    ("@headitem Ciphersuite name @tab TLS ID @tab Since\n");
		for (i = 0;
		     (name =
		      gnutls_cipher_suite_info(i, id, &kx, &cipher, &mac,
					       &version)); i++) {
			printf("@item %s\n@tab 0x%02X 0x%02X\n@tab %s\n",
			       escape_texi_string(name, buffer,
						  sizeof(buffer)),
			       (unsigned char) id[0],
			       (unsigned char) id[1],
			       gnutls_protocol_get_name(version));
		}
		printf("@end multitable\n");

	}

	{
		const gnutls_certificate_type_t *p =
		    gnutls_certificate_type_list();

		printf("\n\n@heading Certificate types\n");
		printf("@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n",
			       gnutls_certificate_type_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_protocol_t *p = gnutls_protocol_list();

		printf("\n@heading Protocols\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_protocol_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_cipher_algorithm_t *p = gnutls_cipher_list();

		printf("\n@heading Ciphers\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_cipher_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_mac_algorithm_t *p = gnutls_mac_list();

		printf("\n@heading MAC algorithms\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_mac_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_kx_algorithm_t *p = gnutls_kx_list();

		printf("\n@heading Key exchange methods\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_kx_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_pk_algorithm_t *p = gnutls_pk_list();

		printf("\n@heading Public key algorithms\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_pk_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_sign_algorithm_t *p = gnutls_sign_list();

		printf
		    ("\n@heading Public key signature algorithms\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n", gnutls_sign_get_name(*p));
		}
		printf("@end table\n");
	}

	{
		const gnutls_group_t *p = gnutls_group_list();

		printf("\n@heading Groups\n@table @code\n");
		for (; *p; p++) {
			printf("@item %s\n",
			       gnutls_group_get_name(*p));
		}
		printf("@end table\n");
	}
}

static const char headers[] = "\\tablefirsthead{%\n"
    "\\hline\n" "Ciphersuite name & TLS ID & Since\\\\\n" "\\hline}\n"
#if 0
    "\\tablehead{%\n"
    "\\hline\n"
    "\\multicolumn{3}{|l|}{\\small\\sl continued from previous page}\\\\\n"
    "\\hline}\n"
    "\\tabletail{%\n"
    "\\hline\n"
    "\\multicolumn{3}{|r|}{\\small\\sl continued on next page}\\\\\n"
    "\\hline}\n"
#endif
    "\\tablelasttail{\\hline}\n"
    "\\bottomcaption{The ciphersuites table}\n\n";

static void main_latex(void)
{
	int i, j;
	const char *desc;
	const char *_name;

	puts(headers);

	printf
	    ("\\begin{supertabular}{|p{.64\\linewidth}|p{.12\\linewidth}|p{.09\\linewidth}|}\n");

	{
		size_t i;
		const char *name;
		char id[2];
		gnutls_kx_algorithm_t kx;
		gnutls_cipher_algorithm_t cipher;
		gnutls_mac_algorithm_t mac;
		gnutls_protocol_t version;

		for (i = 0; (name = gnutls_cipher_suite_info
			     (i, id, &kx, &cipher, &mac, &version)); i++) {
			printf
			    ("{\\small{%s}} & \\code{0x%02X 0x%02X} & %s",
			     escape_string(name, buffer, sizeof(buffer)),
			     (unsigned char) id[0], (unsigned char) id[1],
			     gnutls_protocol_get_name(version));
			printf("\\\\\n");
		}
		printf("\\end{supertabular}\n\n");

	}

	return;

}
