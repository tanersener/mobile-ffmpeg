/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
 *
 * Author: Nikos Mavrogiannopoulos
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

/* This tests key import for gnutls_ocsp_resp_t APIs */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/ocsp.h>
#include "ocsp-common.h"
#include "utils.h"

#define testfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

static void load_list(const char *name, const gnutls_datum_t *txt,
		      unsigned int nocsps,
		      int format,
		      unsigned flags, int exp_err)
{
	gnutls_ocsp_resp_t *ocsps;
	unsigned int i, size;
	int ret;

	ret = gnutls_ocsp_resp_list_import2(&ocsps, &size, txt, format, flags);
	if (ret < 0) {
		if (exp_err == ret)
			return;
		testfail("gnutls_x509_crt_list_import: %s\n", gnutls_strerror(ret));
	}

	for (i=0;i<size;i++)
		gnutls_ocsp_resp_deinit(ocsps[i]);
	gnutls_free(ocsps);

	if (size != nocsps)
		testfail("imported number (%d) doesn't match expected (%d)\n", size, nocsps);

	return;
}

typedef struct test_st {
	const char *name;
	const gnutls_datum_t *ocsps;
	unsigned nocsps;
	unsigned flags;
	int format;
	int exp_err;
} test_st;

static const char long_chain_pem[] =
	"-----BEGIN OCSP RESPONSE-----\n"
	"MIICOgoBAKCCAjMwggIvBgkrBgEFBQcwAQEEggIgMIICHDCBhaEUMBIxEDAOBgNV\n"
	"BAMTB3N1YkNBLTMYDzIwMTcxMDE4MTIyMDQ5WjBcMFowRTAJBgUrDgMCGgUABBSy\n"
	"5lyOboNLvRHZl/o2k1merVwVxAQUnpHsjKokWyLg6BHo6aSRtZEmAPECDFejHTI3\n"
	"ZFj6e1Jv14IAGA8yMDE3MTAxODEyMjA0OVowDQYJKoZIhvcNAQELBQADggGBAJg8\n"
	"/9F2k7DdQsqMfU+f53gUHZAlZzRRPPYQfrmMGfSaMmr9W3fpCrLNMV4PWxGndTh7\n"
	"AforaCwUb6+QyWlnE3B4UUQLphaEawnDJ/8GJZAnCIcjyxpWYZ4onEIZ6pN8BRQE\n"
	"f8ccQN01xlB5RtdqsVmvxtoM0husO0YJDnsCwwFVXulPEFgWuFSoVLsx65lkc+4/\n"
	"RM67+QrbNpBRgKrhb8MAE2WANjpjSAVSf5GWsH9T/F2HDG5crApFIoNywK9e21zk\n"
	"qYAWQ6tVcps3QbvvIEXVy/jOqVASeaxuwHmkdBz4SNT83LvaNnJGBTKXTGukPKAO\n"
	"t6xJpFLwrNWNhgfbw2fklWJSMzMtAEkjzBJi+4kn1SfLdcTLYBf9Tnoq1wsJhAMg\n"
	"OFNzcWb8ZJxuGh7FXgytneM38sL8oTEmLKHfBRnWGOglfCMj3olvXpjotrIlKDAS\n"
	"GbGElY+PZXUtkKiN2cNAecjIodzQFgL+YL6jWKLEuuWGT/MvRrliL83kGmKDdg==\n"
	"-----END OCSP RESPONSE-----\n"
	"-----BEGIN OCSP RESPONSE-----\n"
	"MIICNwoBAKCCAjAwggIsBgkrBgEFBQcwAQEEggIdMIICGTCBgqERMA8xDTALBgNV\n"
	"BAMTBENBLTMYDzIwMTcxMDE4MTIzODUyWjBcMFowRTAJBgUrDgMCGgUABBS3yg+r\n"
	"3G+4sJZ6FayYCg8Z/qQS3gQUHoXtf55x+gidN0hDoBLv5arh44oCDFejHTI1s0/Q\n"
	"ufXnPIIAGA8yMDE3MTAxODEyMzg1MlowDQYJKoZIhvcNAQELBQADggGBALMParB9\n"
	"K97DlT4FmMdPScoT7oAAsar4XxKLU9+oraht7H+WTAYSpnCxh/ugR17G0jtzTzIw\n"
	"nLQFAyR9MDYKp4Om4YqQ7r+43DiIqKVU25WcrVifUbtkR+LbjH+Bk1UHvFE8mCOX\n"
	"ZB+cmQyjGap1RX0dnj2Wm48vUwqp71nA8AYcXL575xZ4rb9DDhaoV2h3S0Zlu4IN\n"
	"btuDIVsxJ53kqkGjjVB4/R0RtqCXOI2ThMK3SfDWqwzF9tYA763VVXi+g+w3oyv4\n"
	"ZtP8QUWOVUY4azpElX1wqoO8znUjxs1AzROLUeLPK8GMLVIZLP361J2kLgcj0Gdq\n"
	"GIVH5N54p6bl5OgSUP3EdKbFRZyCVZ2n8Der3Cf9PtfvGV7Ze4Cv/CCN6rJkk54P\n"
	"6auP6pEJg0ESGC5fop5HFCyVM+W/ot0A1cxN0+cHYlqB1NQholLqe3psDjJ2EoIK\n"
	"LtN5dRLO6z5L74CwwiJ1SeLh8XyJtr/ee9RnFB56XCzO7lyhbHPx/VT6Qw==\n"
	"-----END OCSP RESPONSE-----\n"
	"-----BEGIN OCSP RESPONSE-----\n"
	"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\n"
	"-----END OCSP RESPONSE-----\n";

static const char bad_long_chain_pem[] = /* second response is broken */
	"-----BEGIN OCSP RESPONSE-----\n"
	"MIICOgoBAKCCAjMwggIvBgkrBgEFBQcwAQEEggIgMIICHDCBhaEUMBIxEDAOBgNV\n"
	"BAMTB3N1YkNBLTMYDzIwMTcxMDE4MTIyMDQ5WjBcMFowRTAJBgUrDgMCGgUABBSy\n"
	"5lyOboNLvRHZl/o2k1merVwVxAQUnpHsjKokWyLg6BHo6aSRtZEmAPECDFejHTI3\n"
	"ZFj6e1Jv14IAGA8yMDE3MTAxODEyMjA0OVowDQYJKoZIhvcNAQELBQADggGBAJg8\n"
	"/9F2k7DdQsqMfU+f53gUHZAlZzRRPPYQfrmMGfSaMmr9W3fpCrLNMV4PWxGndTh7\n"
	"AforaCwUb6+QyWlnE3B4UUQLphaEawnDJ/8GJZAnCIcjyxpWYZ4onEIZ6pN8BRQE\n"
	"f8ccQN01xlB5RtdqsVmvxtoM0husO0YJDnsCwwFVXulPEFgWuFSoVLsx65lkc+4/\n"
	"RM67+QrbNpBRgKrhb8MAE2WANjpjSAVSf5GWsH9T/F2HDG5crApFIoNywK9e21zk\n"
	"qYAWQ6tVcps3QbvvIEXVy/jOqVASeaxuwHmkdBz4SNT83LvaNnJGBTKXTGukPKAO\n"
	"t6xJpFLwrNWNhgfbw2fklWJSMzMtAEkjzBJi+4kn1SfLdcTLYBf9Tnoq1wsJhAMg\n"
	"OFNzcWb8ZJxuGh7FXgytneM38sL8oTEmLKHfBRnWGOglfCMj3olvXpjotrIlKDAS\n"
	"GbGElY+PZXUtkKiN2cNAecjIodzQFgL+YL6jWKLEuuWGT/MvRrliL83kGmKDdg==\n"
	"-----END OCSP RESPONSE-----\n"
	"-----BEGIN OCSP RESPONSE-----\n"
	"MIICNwoBAKCCAjAwggIsBgkrBgEFBQcwAQEEggIdMIICGTCBgqERMA8xDTALBgNV\n"
	"BAMTBENBLTMYDzIwMTcxMDE4MTIzODUyWjBcMFowRTAJBgUrDgMCGgUABBS3yg+r\n"
	"3G+4sJZ6FayYCg8Z/qQS3gQUHoXtf55x+gidN0hDoBLv5arh44oCDFejHTI1s0/Q\n"
	"ufXnPIIAGA8yMDE3MTAxODEyMzg1MlowDQYJKoZIhvcNAQELBQADggGBALMParB9\n"
	"K97DlT4FmMdPScoT7oAAsar4XxKLU9+oraht7H+WTAYSpnCxh/ugR1fG0jtzTzIw\n"
	"nLQFAyR9MDYKp4Om4YqQ7r+43DiIqKVU25WcrVifUbtkR+LbjH+Bk1UHvFE8mCOX\n"
	"ZB+cmQyjGap1RX0dnj2Wm48vwwqp71nA8AYcXL575xZ4rb9DDhaoV2h3S0Zlu4IN\n"
	"btuDIVsxJ53kqkGjjVB4/R0RtqCXOI2ThMK3SfDWqwzF9tYA763VVXi+g+w3oyv4\n"
	"ZtP8QUWOVUY4azpzlX1wqoO8znUjxs1AzROLUeLPK8GMLVIZLP361J2kLgcj0Gdq\n"
	"GIVH5N54p6bl5OgSUP3EdKbFRZyCVZ2n8Der3Cf9PtfvGV7Ze4Cv/CCN6rJkk54P\n"
	"6auP6pEJg0ESGC5fop5HFCyVM+W/ot0A1cxN0+cHYlqB1NQholLqe3psDjJ2EoIK\n"
	"LtN5dRLO6z5L74CwwiJ1SeLh8XyJtr/ee9RnFB56XCzO7lyhbHPx/VT6Qw==\n"
	"-----END OCSP RESPONSE-----\n"
	"-----BEGIN OCSP RESPONSE-----\n"
	"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\n"
	"-----END OCSP RESPONSE-----\n";

static const gnutls_datum_t long_chain = {
	(void*)long_chain_pem, sizeof(long_chain_pem)-1
};

static const gnutls_datum_t bad_long_chain = {
	(void*)bad_long_chain_pem, sizeof(bad_long_chain_pem)-1
};

static const gnutls_datum_t no_chain = {
	(void*)" ", 1
};

static const test_st tests[] = {
	{.name = "load no ocsps",
	 .ocsps = &no_chain,
	 .nocsps = 0,
	 .flags = 0,
	 .format = GNUTLS_X509_FMT_PEM,
	 .exp_err = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
	},
	{.name = "load of 3 ocsps, with expected failure",
	 .ocsps = &bad_long_chain,
	 .nocsps = 3,
	 .flags = 0,
	 .format = GNUTLS_X509_FMT_PEM,
	 .exp_err = GNUTLS_E_ASN1_TAG_ERROR
	},
	{.name = "load 3 ocsps",
	 .ocsps = &long_chain,
	 .nocsps = 3,
	 .format = GNUTLS_X509_FMT_PEM,
	 .flags = 0
	},
	{.name = "load 1 DER ocsp",
	 .ocsps = &ocsp_subca3_unknown,
	 .nocsps = 1,
	 .format = GNUTLS_X509_FMT_DER,
	 .flags = 0
	}
};

void doit(void)
{
	unsigned int i;

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		success("checking: %s\n", tests[i].name);

		load_list(tests[i].name, tests[i].ocsps, tests[i].nocsps,
			  tests[i].format, tests[i].flags, tests[i].exp_err);
	}

	gnutls_global_deinit();
}
