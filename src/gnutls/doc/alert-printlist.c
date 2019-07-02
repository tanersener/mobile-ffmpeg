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
		gnutls_kx_algorithm_t kx;
		gnutls_cipher_algorithm_t cipher;
		gnutls_mac_algorithm_t mac;
		gnutls_protocol_t version;

		printf
		    ("@multitable @columnfractions .55 .10 .30\n@anchor{tab:alerts}\n");
		printf("@headitem Alert @tab ID @tab Description\n");
		for (i = 0; i < 256; i++) {
			if (gnutls_alert_get_strname(i) == NULL)
				continue;
			printf("@item %s\n@tab %d\n@tab %s\n",
			       escape_texi_string(gnutls_alert_get_strname
						  (i), buffer,
						  sizeof(buffer)),
			       (unsigned int) i, gnutls_alert_get_name(i));
		}
		printf("@end multitable\n");

	}
}

static const char headers[] = "\\tablefirsthead{%\n"
    "\\hline\n" "Alert & ID & Description\\\\\n" "\\hline}\n"
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
    "\\bottomcaption{The TLS alert table}\n\n";

static void main_latex(void)
{
	int i, j;
	const char *desc;
	const char *_name;

	puts(headers);

	printf
	    ("\\begin{supertabular}{|p{.50\\linewidth}|p{.07\\linewidth}|p{.34\\linewidth}|}\n\\label{tab:alerts}\n");

	{
		size_t i;
		const char *name;
		gnutls_kx_algorithm_t kx;
		gnutls_cipher_algorithm_t cipher;
		gnutls_mac_algorithm_t mac;
		gnutls_protocol_t version;

		for (i = 0; i < 256; i++) {
			if (gnutls_alert_get_strname(i) == NULL)
				continue;
			printf("{\\small{%s}} & \\code{%d} & %s",
			       escape_string(gnutls_alert_get_strname(i),
					     buffer, sizeof(buffer)),
			       (unsigned int) i, gnutls_alert_get_name(i));
			printf("\\\\\n");
		}

		printf("\\end{supertabular}\n\n");

	}

	return;

}
