/*
 * Copyright (C) 2008-2014 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
#include <string.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

static const char *simple1[] = {
/* CRL */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA5MDUyM1oYDzIwMTUwOTEzMDkwNTIzWjAAoDowODAfBgNVHSMEGDAWgBTx\n"
"Fcb1FYLbhH9yuqg4wlVJUZnYKTAVBgNVHRQEDgIMVBQI0zeJoFHkLaWNMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQB6SGYePy1MBmtELyWdnlJHlQ4bBgb4vjuLDSfH0X6b4dAS\n"
"MEZws8iA5SaJFIioIP41s3lfQ1Am7GjSoNccHdrLmEcUSTQLwLYaDL8SgxekP5Au\n"
"w8HTu1cz/mnjBBDURq1RvyGNFm6MXf1Rg/bHSea/EpDkn8KY152BT1/46iQ+Uho6\n"
"hz6UUWsTB4Lj25X8F2hlKwQcb3E63Or2XEPBw4rhaCDFAtSZeBaGUUSJ8CLUKXZf\n"
"5b45MjiZ/osgd81tfn3wdQVjDnaQwNtjeRbK+qU0Z4pIKBvHzRS/fZKwTnrK1DLI\n"
"yY/nqBJT/+Q5zdUx5FXp0bwyZuarJ1GHqcES3Rz1\n"
"-----END X509 CRL-----\n",
/* CA - cert_signing_key only */
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwOTEzMDkwNTIzWhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCybbzvQTOmfwlA\n"
"+q8F/4ms42nhl5lo1lK6JCvE7jZdhqZNXE8e1eNACrU6rCxRQynDhOyAOCLQAAul\n"
"ivNMCW+SFN0IkSYXSRM8aWIDOZT8FyWB3yJSyvi3+SMgm7OYHFW8htH8qaIv0xJf\n"
"1h/ADBE62j9uaQIg7qSn6pVHMDHaITAbPg3y6II1iP3W28Vj/rtvK9yoZu4AThSD\n"
"Vdjl8WT4b4VOBbmioSNCDjx2C73+HLM2eUsdumCVcjWD9gkvCKkqTbOVplGRvCzO\n"
"sKNVGJamH9eGOjF2Az9XuYR+m7jWdIyTitLtbliyFiWwFguQ7BAPVnUS3TSKoLKL\n"
"X9WRGDIVAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcG\n"
"ADAdBgNVHQ4EFgQU8RXG9RWC24R/crqoOMJVSVGZ2CkwDQYJKoZIhvcNAQELBQAD\n"
"ggEBAASDvSD6Gt9E/IANgJ2lq7cvqKHhK/S0crpBHmzouLU1YANAbva8vZ2iVsgP\n"
"ojj5+QKosXgZM67g1u4Vr/Kt7APwYDVV9NlfE7BLSaksaQbh6J464rJ8pXONW6xP\n"
"z6tl/Pm1RqXuxzgnUv700OFuxBnnbglz9aQk5eS7kag8bfUx8MfN5gbW34nB79fn\n"
"5943Z8DmcDfUQZRY66v4S/NAYs7s96ABMB18u9Ct6KqGP/LKfDt2bgeTE/1b68T+\n"
"xmYF8N+JsJ3qP4lqBHgHLUL945nEoG8yDPIiZw3pmw1SyS0ktoVASynAh3W5j//r\n"
"d9Uk2Ojqo2tp/lJ0LCuQ3nWeM2Y=\n"
"-----END CERTIFICATE-----\n"
};

static const char *simple1_broken[] = {
/* CRL with some bits flipped */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA5MDUyM1oYDzIwMTUwOTEzMDkwNTIzWjAAoDowODAfBgNVHSMEGDAWgBTx\n"
"Fcb1FYLbhH9yuqg4wlVJUZnYKTAVBgNVHRQEDgIMVBQI0zeJoFHkLaWNMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQB6SGYePy1MBmtELyWdnlJHlQ4bBgb4vjuLDSfH0X6b4dAS\n"
"MEZws8iA5SaJFIioIP41s3lfQ1Am7GjSoNccHdrLmEcUSTQLwLYaDL8SgxekP5Au\n"
"w8HTu1cz/mnjBBDURq1RvyGNFm6MXf1Rg/bHSea/EpDkn8KY152BT1/46iQ+Uho6\n"
"hz6UUWsTB4Lj25X8F3hlKwQcb3E63Or2XEPBw4rhaCDFAtSZeBaGUUSJ8CLUKXZf\n"
"5b45MjiZ/osgd81tfn3wdQVjDnaQwNtjeRbK+qU0Z4pIKBvHzRS/fZKwTnrK1DLI\n"
"yY/nqBJT/+Q5zdUx5FXp0bwyZuarJ1GHqcES3Rz1\n"
"-----END X509 CRL-----\n",
/* CA - cert_signing_key only */
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwOTEzMDkwNTIzWhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCybbzvQTOmfwlA\n"
"+q8F/4ms42nhl5lo1lK6JCvE7jZdhqZNXE8e1eNACrU6rCxRQynDhOyAOCLQAAul\n"
"ivNMCW+SFN0IkSYXSRM8aWIDOZT8FyWB3yJSyvi3+SMgm7OYHFW8htH8qaIv0xJf\n"
"1h/ADBE62j9uaQIg7qSn6pVHMDHaITAbPg3y6II1iP3W28Vj/rtvK9yoZu4AThSD\n"
"Vdjl8WT4b4VOBbmioSNCDjx2C73+HLM2eUsdumCVcjWD9gkvCKkqTbOVplGRvCzO\n"
"sKNVGJamH9eGOjF2Az9XuYR+m7jWdIyTitLtbliyFiWwFguQ7BAPVnUS3TSKoLKL\n"
"X9WRGDIVAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcG\n"
"ADAdBgNVHQ4EFgQU8RXG9RWC24R/crqoOMJVSVGZ2CkwDQYJKoZIhvcNAQELBQAD\n"
"ggEBAASDvSD6Gt9E/IANgJ2lq7cvqKHhK/S0crpBHmzouLU1YANAbva8vZ2iVsgP\n"
"ojj5+QKosXgZM67g1u4Vr/Kt7APwYDVV9NlfE7BLSaksaQbh6J464rJ8pXONW6xP\n"
"z6tl/Pm1RqXuxzgnUv700OFuxBnnbglz9aQk5eS7kag8bfUx8MfN5gbW34nB79fn\n"
"5943Z8DmcDfUQZRY66v4S/NAYs7s96ABMB18u9Ct6KqGP/LKfDt2bgeTE/1b68T+\n"
"xmYF8N+JsJ3qP4lqBHgHLUL945nEoG8yDPIiZw3pmw1SyS0ktoVASynAh3W5j//r\n"
"d9Uk2Ojqo2tp/lJ0LCuQ3nWeM2Y=\n"
"-----END CERTIFICATE-----\n"
};

static const char *simple1_constraints[] = {
/* CRL */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA4NTkxNloYDzIwMTUwOTEzMDg1OTE2WjAAoDowODAfBgNVHSMEGDAWgBTn\n"
"sISO6h9leKTKtOh/HG0jV03AMzAVBgNVHRQEDgIMVBQHZC2mj6EAgMPSMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQBHUgtxpOn8EHwlajVYoOh6DFCwIoxBIeUA4518W1cHoV7J\n"
"KMif6lmJRodrcbienDX781QcOaQcNnuu/oBEcoBdbZa0VICzXekIteSwEgGsbRve\n"
"QQFPnZn83I4btse1ly5fdxMsliSM+qRwIyNR18VHXZz9GWYrr4tYWnI2b9XrDnaC\n"
"1b3Ywt7I9pNi0/O0C0rE/37/VvPx6HghnC+un7LtT0Y0n+FQP7dhlMvzHaR8wVxs\n"
"WAzaNvSiJ1rVPzL21iCmQJsRQeDTSJBlzm0lWiU8Nys3ugM2KlERezfp8DkFGA3y\n"
"9Yzpq6gAi39ZK+LjopgGDkrQjxzBIaoe2bcDqB7X\n"
"-----END X509 CRL-----\n",
/* CA - cert_signing_key only */
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwOTEzMDg1OTE2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC7AVMcGmvenCAt\n"
"14Yi2zi6In2vjNakbzDfUa5xaG8oD73h4P8zP2TQqDmUBAAi5EdXoF5/crpgnGY3\n"
"oyUEFYnT7GTI/FO+RxZz9jCLvY3hpeuJcofsFny8n0ARL9WiFKuAEvrZkg+6V3Fh\n"
"TC9bCOFsGVTaLiUoi/nkD9IUgCkybFTqZM+8tLT4/gCMFNs9e0ANa5F+wtvS0bjy\n"
"LLozq6+XpzEXlL3UNKJq9cf02zHjb9ftlMDykRRkGPzppBSfOCJAMOX/BBNpWznJ\n"
"I1bg0m/6X3+SDO3j0PKLVc7BWWTnXXHb4rznwcRZm8zJiKKFE0GDOijzpT6Dl/gX\n"
"JI0lroeJAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU57CEjuofZXikyrTofxxtI1dNwDMwDQYJKoZIhvcNAQELBQAD\n"
"ggEBALPFKXFauyO0R7Y+zhpiqYe1ms4qU9aprr/x4GMG4ByZ0i0FK8Kh+L5BsNQA\n"
"FsEMeEEmKTHKzkMHfvTJ6y/K6P9rTVY7W2MqlX8IXM02L3fg0zn7Xd9CtCG1nnzh\n"
"fQMf/K/9Xqiotjlrgo8noEZksGPIvDPXXY98dd0clGnBvw2HwiG4h+csr4i9y7CH\n"
"tpnTRJnfzdqDYIh8vnM0tIJbXbe5DBLHnmnx15FQB1apFNa87gdBHAnkHCXrV1vC\n"
"oZXEeUL/zW2ax+ALOglM82dwex2qV9jgcsWfq1Y2JBlVT1QPpbAooCnjvBhmPCjX\n"
"qYkVfApeRr4QAwwkLnyfSKNLHco=\n"
"-----END CERTIFICATE-----\n"
};

static const char *simple1_fail[] = {
/* CRL */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA4NTkxNloYDzIwMTUwOTEzMDg1OTE2WjAAoDowODAfBgNVHSMEGDAWgBTn\n"
"sISO6h9leKTKtOh/HG0jV03AMzAVBgNVHRQEDgIMVBQHZC2mj6EAgMPSMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQBHUgtxpOn8EHwlajVYoOh6DFCwIoxBIeUA4518W1cHoV7J\n"
"KMif6lmJRodrcbienDX781QcOaQcNnuu/oBEcoBdbZa0VICzXekIteSwEgGsbRve\n"
"QQFPnZn83I4btse1ly5fdxMsliSM+qRwIyNR18VHXZz9GWYrr4tYWnI2b9XrDnaC\n"
"1b3Ywt7I9pNi0/O0C0rE/37/VvPx6HghnC+un7LtT0Y0n+FQP7dhlMvzHaR8wVxs\n"
"WAzaNvSiJ1rVPzL21iCmQJsRQeDTSJBlzm0lWiU8Nys3ugM2KlERezfp8DkFGA3y\n"
"9Yzpq6gAi39ZK+LjopgGDkrQjxzBIaoe2bcDqB7X\n"
"-----END X509 CRL-----\n",
/* CA (unrelated to CRL) */
"-----BEGIN CERTIFICATE-----\n"
"MIIDFTCCAf2gAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwODI2MTEwODUyWhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC+iPUnEs+qmj2U\n"
"Rz8plNAE/CpeUxUfNNVonluu4DzulsxAJMN78g+Oqx+ggdkECZxHLISkzErMgiuv\n"
"bG+nr9yxjyHH2YoOAgzgknar5JkOBkKp1bIvyA950ZSygMFEHX1qoaM+F/1/DKjG\n"
"NmMCNUpR0c4m+K22s72LnrpMLMmCZU0fnqngb1+F+iZE6emhcX5Z5D0QTJTAeiYK\n"
"ArnO0rpVEvU0o3nwe3dDrT0YyoCYrzCsCOKUa2wFtkOzLZKJbMBRMflL+fBmtj/Q\n"
"7xUe7ox62ZEqSD7W+Po48/mIuSOhx7u+yToBZ60wKGz9OkQ/JwykkK5ZgI+nPWGT\n"
"1au1K4V7AgMBAAGjeDB2MA8GA1UdEwEB/wQFMAMBAf8wEgYDVR0eAQH/BAgwBqEE\n"
"MAKCADAPBgNVHQ8BAf8EBQMDBwQAMB0GA1UdDgQWBBSgAJcc9Q5KDpAhkrMORPJS\n"
"boq3vzAfBgNVHSMEGDAWgBQ/lKQpHoyEFz7J+Wn6eT5qxgYQpjANBgkqhkiG9w0B\n"
"AQsFAAOCAQEAoMeZ0cnHes8bWRHLvrGc6wpwVnxYx2CBF9Xd3k4YMNunwBF9oM+T\n"
"ZYSMo4k7C1XZ154avBIyiCne3eU7/oHG1nkqY9ndN5LMyL8KFOniETBY3BdKtlGA\n"
"N+pDiQsrWG6mtqQ+kHFJICnGEDDByGB2eH+oAS+8gNtSfamLuTWYMI6ANjA9OWan\n"
"rkIA7ta97UiH2flvKRctqvZ0n6Vp3n3aUc53FkAbTnxOCBNCBx/veCgD/r74WbcY\n"
"jiwh2RE//3D3Oo7zhUlwQEWQSa/7poG5e6bl7oj4JYjpwSmESCYokT83Iqeb9lwO\n"
"D+dr9zs1tCudW9xz3sUg6IBXhZ4UvegTNg==\n"
"-----END CERTIFICATE-----\n"
};

static struct
{
  const char *name;
  const char **crl;
  const char **ca;
  unsigned int verify_flags;
  unsigned int expected_verify_result;
} crl_list[] =
{
  { "simple-success", &simple1[0], &simple1[1],
    0, 0 },
  { "simple-constraints", &simple1_constraints[0], &simple1_constraints[1],
    0, GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE|GNUTLS_CERT_INVALID },
  { "simple-broken", &simple1_broken[0], &simple1_broken[1],
    0, GNUTLS_CERT_INVALID | GNUTLS_CERT_SIGNATURE_FAILURE },
  { "simple-fail", &simple1_fail[0], &simple1_fail[1],
    0, GNUTLS_CERT_INVALID | GNUTLS_CERT_SIGNER_NOT_FOUND},
  { NULL, NULL, NULL, 0, 0}
};

/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	time_t then = 1410599367;

	if (t)
		*t = then;

	return then;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	int exit_val = 0;
	size_t i;
	int ret;
	gnutls_x509_trust_list_t tl;
	unsigned int verify_status;
	gnutls_x509_crl_t crl;
	gnutls_x509_crt_t ca;
	gnutls_datum_t tmp;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	for (i = 0; crl_list[i].name; i++) {

		if (debug)
			printf("Chain '%s' (%d)...\n", crl_list[i].name,
				(int) i);

		if (debug > 2)
			printf("\tAdding CRL...");

		ret = gnutls_x509_crl_init(&crl);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crl_init[%d]: %s\n",
				(int) i,
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char *) *crl_list[i].crl;
		tmp.size = strlen(*crl_list[i].crl);

		ret =
		    gnutls_x509_crl_import(crl, &tmp,
					   GNUTLS_X509_FMT_PEM);
		if (debug > 2)
		printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crl_import[%s]: %s\n",
				crl_list[i].name,
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crl_print(crl,
				      GNUTLS_CRT_PRINT_ONELINE,
				      &tmp);
		if (debug)
			printf("\tCRL: %.*s\n", 
				tmp.size, tmp.data);
		gnutls_free(tmp.data);

		if (debug > 2)
			printf("\tAdding CA certificate...");

		ret = gnutls_x509_crt_init(&ca);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_crt_init: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char *) *crl_list[i].ca;
		tmp.size = strlen(*crl_list[i].ca);

		ret =
		    gnutls_x509_crt_import(ca, &tmp, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_crt_import: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		if (debug > 2)
			printf("done\n");

		gnutls_x509_crt_print(ca, GNUTLS_CRT_PRINT_ONELINE, &tmp);
		if (debug)
			printf("\tCA Certificate: %.*s\n", tmp.size,
				tmp.data);
		gnutls_free(tmp.data);

		if (debug)
			printf("\tVerifying...");

		ret = gnutls_x509_crl_verify(crl, &ca, 1, crl_list[i].verify_flags,
						  &verify_status);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_list_verify[%d]: %s\n",
				(int) i, gnutls_strerror(ret));
			exit(1);
		}

		if (verify_status != crl_list[i].expected_verify_result) {
			gnutls_datum_t out1, out2;
			gnutls_certificate_verification_status_print
			    (verify_status, GNUTLS_CRT_X509, &out1, 0);
			gnutls_certificate_verification_status_print(crl_list
								     [i].
								     expected_verify_result,
								     GNUTLS_CRT_X509,
								     &out2,
								     0);
			fail("chain[%s]:\nverify_status: %d: %s\nexpected: %d: %s\n", crl_list[i].name, verify_status, out1.data, crl_list[i].expected_verify_result, out2.data);
			gnutls_free(out1.data);
			gnutls_free(out2.data);

			if (!debug)
				exit(1);
		} else if (debug)
			printf("done\n");

		gnutls_x509_trust_list_init(&tl, 0);

		ret =
		    gnutls_x509_trust_list_add_cas(tl, &ca, 1, 0);
		if (ret != 1) {
			fail("gnutls_x509_trust_list_add_trust_mem\n");
			exit(1);
		}

		/* make sure that the two functions don't diverge */
		ret = gnutls_x509_trust_list_add_crls(tl, &crl, 1, GNUTLS_TL_VERIFY_CRL, crl_list[i].verify_flags);
		if (crl_list[i].expected_verify_result == 0 && ret < 0) {
			fprintf(stderr,
				"gnutls_x509_trust_list_add_crls[%d]: %s\n",
				(int) i, gnutls_strerror(ret));
			exit(1);
		}
		if (crl_list[i].expected_verify_result != 0 && ret > 0) {
			fprintf(stderr,
				"gnutls_x509_trust_list_add_crls[%d]: succeeded when it shouldn't\n",
				(int) i);
			exit(1);
		}

		if (debug)
			printf("\tCleanup...");

		gnutls_x509_trust_list_deinit(tl, 0);
		gnutls_x509_crt_deinit(ca);
		gnutls_x509_crl_deinit(crl);

		if (debug)
			printf("done\n\n\n");
	}

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);

	exit(exit_val);
}
