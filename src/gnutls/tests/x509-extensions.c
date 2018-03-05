/*
 * Copyright (C) 2006-2012 Free Software Foundation, Inc.
 * Author: Simon Josefsson, Howard Chu
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
#include "config.h"
#endif

#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>
#include "utils.h"

static char invalid_cert[] = /* v1 certificate with extensions */
"-----BEGIN CERTIFICATE-----\n"
"MIIDHjCCAgYCDFQ7zlUDsihSxVF4mDANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQD\n"
"EwRDQS0wMCIYDzIwMTQxMDEzMTMwNjI5WhgPOTk5OTEyMzEyMzU5NTlaMBMxETAP\n"
"BgNVBAMTCHNlcnZlci0xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"zoG3/1YtwGHh/5u3ex6xAmwO0/H4gdIy/yiYLxqWcy+HzyMBBZHNXuV7W0z7x+Qo\n"
"qCGtenWkzIQSgeYKyzdcpPDscZIYOgwHWUFczxgVGdLsBKPSczgqMHpSCLgMgnDM\n"
"RaN6SNQeTQdftkLt5wdBSzNaxhhPYsCEbopSeZ8250FCLS3gRpoMtYCBiy7cjSJB\n"
"zv6zmZStXNgTYr8pLwI0nyxPyRdB+TZyqAC6r9W154y51vsqUCGmC0I9hn1A5kkD\n"
"5057x+Ho1kDwPxOfObdOR+AJSAw/FeGuStzViJY0I68B90sEo/HD+h7mB+CwJ2Yf\n"
"64/xVdh+D8L65eYkM9z88wIDAQABo3cwdTAMBgNVHRMBAf8EAjAAMBQGA1UdEQQN\n"
"MAuCCWxvY2FsaG9zdDAPBgNVHQ8BAf8EBQMDB6AAMB0GA1UdDgQWBBT7Gk/u95zI\n"
"JTM89CXJ70IxxqhegDAfBgNVHSMEGDAWgBQ9X77/zddjG9ob2zrR/WuGmxwFGDAN\n"
"BgkqhkiG9w0BAQsFAAOCAQEAaTrAcTkQ7yqf6afoTkFXZuZ+jJXYNGkubxs8Jo/z\n"
"srJk/WWVGAKuxiBDumk88Gjm+WXGyIDA7Hq9fhGaklJV2PGRfNVx9No51HXeAToT\n"
"sHs2XKhk9SdKKR4UJkuX3U2malMlCpmFMtm3EieDVZLxeukhODJQtRa3vGg8QWoz\n"
"ODlewHSmQiXhnqq52fLCbdVUaBnaRGOIwNZ0FcBWv9n0ZCuhjg9908rUVH9/OjI3\n"
"AGVZcbN9Jac2ZO8NTxP5vS1hrG2wT9+sVRh1sD5ISZSM4gWdq9sK8d7j+SwOPBWY\n"
"3dcxQlfvWw2Dt876XYoyUZuKirmASVlMw+hkm1WXM7Svsw==\n"
"-----END CERTIFICATE-----\n";

static char pem[] =
    "-----BEGIN CERTIFICATE-----"
    "MIIFdDCCBN2gAwIBAgIBBzANBgkqhkiG9w0BAQsFADCBkzEVMBMGA1UEAxMMQ2lu"
    "ZHkgTGF1cGVyMRcwFQYKCZImiZPyLGQBARMHY2xhdXBlcjERMA8GA1UECxMIQ0Eg"
    "ZGVwdC4xEjAQBgNVBAoTCUtva28gaW5jLjEPMA0GA1UECBMGQXR0aWtpMQswCQYD"
    "VQQGEwJHUjEcMBoGCSqGSIb3DQEJARYNbm9uZUBub25lLm9yZzAiGA8yMDA3MDQy"
    "MTIyMDAwMFoYDzk5OTkxMjMxMjM1OTU5WjCBkzEVMBMGA1UEAxMMQ2luZHkgTGF1"
    "cGVyMRcwFQYKCZImiZPyLGQBARMHY2xhdXBlcjERMA8GA1UECxMIQ0EgZGVwdC4x"
    "EjAQBgNVBAoTCUtva28gaW5jLjEPMA0GA1UECBMGQXR0aWtpMQswCQYDVQQGEwJH"
    "UjEcMBoGCSqGSIb3DQEJARYNbm9uZUBub25lLm9yZzCBnzANBgkqhkiG9w0BAQEF"
    "AAOBjQAwgYkCgYEApcbOdUOEv2SeAicT8QNZ93ktku18L1CkA/EtebmGiwV+OrtE"
    "qq+EzxOYHhxKOPczLXqfctRrbSawMTdwEPtC6didGGV+GUn8BZYEaIMed4a/7fXl"
    "EjsT/jMYnBp6HWmvRwJgeh+56M/byDQwUZY9jJZcALxh3ggPsTYhf6kA4wUCAwEA"
    "AaOCAtAwggLMMBIGA1UdEwEB/wQIMAYBAf8CAQQwagYDVR0RBGMwYYIMd3d3Lm5v"
    "bmUub3JnghN3d3cubW9yZXRoYW5vbmUub3Jnghd3d3cuZXZlbm1vcmV0aGFub25l"
    "Lm9yZ4cEwKgBAYENbm9uZUBub25lLm9yZ4EOd2hlcmVAbm9uZS5vcmcwgfcGA1Ud"
    "IASB7zCB7DB3BgwrBgEEAapsAQpjAQAwZzAwBggrBgEFBQcCAjAkDCJUaGlzIGlz"
    "IGEgbG9uZyBwb2xpY3kgdG8gc3VtbWFyaXplMDMGCCsGAQUFBwIBFidodHRwOi8v"
    "d3d3LmV4YW1wbGUuY29tL2EtcG9saWN5LXRvLXJlYWQwcQYMKwYBBAGqbAEKYwEB"
    "MGEwJAYIKwYBBQUHAgIwGAwWVGhpcyBpcyBhIHNob3J0IHBvbGljeTA5BggrBgEF"
    "BQcCARYtaHR0cDovL3d3dy5leGFtcGxlLmNvbS9hbm90aGVyLXBvbGljeS10by1y"
    "ZWFkMB0GA1UdJQQWMBQGCCsGAQUFBwMDBggrBgEFBQcDCTBYBgNVHR4BAf8ETjBM"
    "oCQwDYILZXhhbXBsZS5jb20wE4ERbm1hdkBAZXhhbXBsZS5uZXShJDASghB0ZXN0"
    "LmV4YW1wbGUuY29tMA6BDC5leGFtcGxlLmNvbTA2BggrBgEFBQcBAQQqMCgwJgYI"
    "KwYBBQUHMAGGGmh0dHA6Ly9teS5vY3NwLnNlcnZlci9vY3NwMA8GA1UdDwEB/wQF"
    "AwMHBgAwHQYDVR0OBBYEFF1ArfDOlECVi36ZlB2SVCLKcjZfMG8GA1UdHwRoMGYw"
    "ZKBioGCGHmh0dHA6Ly93d3cuZ2V0Y3JsLmNybC9nZXRjcmwxL4YeaHR0cDovL3d3"
    "dy5nZXRjcmwuY3JsL2dldGNybDIvhh5odHRwOi8vd3d3LmdldGNybC5jcmwvZ2V0"
    "Y3JsMy8wDQYJKoZIhvcNAQELBQADgYEAdacOt4/Vgc9Y3pSkik3HBifDeK2OtiW0"
    "BZ7xOXqXtL8Uwx6wx/DybZsUbzuR55GLUROYAc3cio5M/0pTwjqmmQ8vuHIt2p8A"
    "2fegFcBbNLX38XxACQh4TDAT/4ftPwOtEol4UR4ItZ1d7faDzDXNpmGE+sp5s6ii"
    "3cIIpInMKE8=" "-----END CERTIFICATE-----";

#define MAX_DATA_SIZE 1024

typedef int (*ext_parse_func) (const gnutls_datum_t * der);

struct ext_handler_st {
	const char *oid;
	ext_parse_func handler;
	unsigned critical;
};

static int basic_constraints(const gnutls_datum_t * der)
{
	int ret, pathlen;
	unsigned ca;

/*
		Basic Constraints (critical):
			Certificate Authority (CA): TRUE
			Path Length Constraint: 4
*/
	ret = gnutls_x509_ext_import_basic_constraints(der, &ca, &pathlen);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (ca != 1) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (pathlen != 4) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	return 0;
}

static int cmp_name(unsigned type, gnutls_datum_t * name,
		    unsigned expected_type, const char *expected_name)
{
	if (type != expected_type) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (name->size != strlen(expected_name)) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (strcmp((char *)name->data, expected_name) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}
	return 0;
}

static int subject_alt_name(const gnutls_datum_t * der)
{
	int ret;
	gnutls_subject_alt_names_t san;
	gnutls_datum_t name;
	unsigned type;
	unsigned i = 0;

	ret = gnutls_subject_alt_names_init(&san);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_subject_alt_names(der, san, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

/*
		Subject Alternative Name (not critical):
			DNSname: www.none.org
			DNSname: www.morethanone.org
			DNSname: www.evenmorethanone.org
			IPAddress: 192.168.1.1
			tRFC822Name: none@none.org
			tRFC822Name: where@none.org
*/
	ret = cmp_name(type, &name, GNUTLS_SAN_DNSNAME, "www.none.org");
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	ret = cmp_name(type, &name, GNUTLS_SAN_DNSNAME, "www.morethanone.org");
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	ret =
	    cmp_name(type, &name, GNUTLS_SAN_DNSNAME,
		     "www.evenmorethanone.org");
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	if (type != GNUTLS_SAN_IPADDRESS) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	ret = cmp_name(type, &name, GNUTLS_SAN_RFC822NAME, "none@none.org");
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	ret = cmp_name(type, &name, GNUTLS_SAN_RFC822NAME, "where@none.org");
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_subject_alt_names_get(san, i++, &type, &name, NULL);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_subject_alt_names_deinit(san);

	return 0;
}

static int ext_key_usage(const gnutls_datum_t * der)
{
/*
		Key Purpose (not critical):
			OCSP signing.
*/
	int ret;
	gnutls_x509_key_purposes_t p;
	unsigned i = 0;
	gnutls_datum_t oid;

	ret = gnutls_x509_key_purpose_init(&p);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_key_purposes(der, p, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_key_purpose_get(p, i++, &oid);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (strcmp((char *)oid.data, "1.3.6.1.5.5.7.3.3") != 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__,
			(char *)oid.data);
		return -1;
	}

	ret = gnutls_x509_key_purpose_get(p, i++, &oid);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (strcmp((char *)oid.data, "1.3.6.1.5.5.7.3.9") != 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__,
			(char *)oid.data);
		return -1;
	}

	ret = gnutls_x509_key_purpose_get(p, i++, &oid);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_x509_key_purpose_deinit(p);

	return 0;
}

static int crt_policies(const gnutls_datum_t * der)
{
	int ret;
	gnutls_x509_policies_t policies;
	struct gnutls_x509_policy_st policy;
	unsigned i = 0;

	ret = gnutls_x509_policies_init(&policies);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_policies(der, policies, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_policies_get(policies, i++, &policy);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
/*
		Certificate Policies (not critical):
			1.3.6.1.4.1.5484.1.10.99.1.0
				Note: This is a long policy to summarize
				URI: http://www.example.com/a-policy-to-read
			1.3.6.1.4.1.5484.1.10.99.1.1
				Note: This is a short policy
				URI: http://www.example.com/another-policy-to-read
*/
	if (strcmp(policy.oid, "1.3.6.1.4.1.5484.1.10.99.1.0") != 0
	    || policy.qualifiers != 2) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (policy.qualifier[0].type != GNUTLS_X509_QUALIFIER_NOTICE ||
	    policy.qualifier[0].size != 34) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (policy.qualifier[1].type != GNUTLS_X509_QUALIFIER_URI ||
	    policy.qualifier[1].size !=
	    strlen("http://www.example.com/a-policy-to-read")
	    || strcmp("http://www.example.com/a-policy-to-read",
		      policy.qualifier[1].data) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	/* second policy */
	ret = gnutls_x509_policies_get(policies, i++, &policy);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}
	if (strcmp(policy.oid, "1.3.6.1.4.1.5484.1.10.99.1.1") != 0
	    || policy.qualifiers != 2) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (policy.qualifier[0].type != GNUTLS_X509_QUALIFIER_NOTICE ||
	    policy.qualifier[0].size != 22) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (policy.qualifier[1].type != GNUTLS_X509_QUALIFIER_URI ||
	    policy.qualifier[1].size !=
	    strlen("http://www.example.com/another-policy-to-read")
	    || strcmp("http://www.example.com/another-policy-to-read",
		      policy.qualifier[1].data) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_policies_get(policies, i++, &policy);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_x509_policies_deinit(policies);

	return 0;
}

static int key_usage(const gnutls_datum_t * der)
{
/*
		Key Usage (critical):
			Certificate signing.
*/
	int ret;
	unsigned int usage = 0;

	ret = gnutls_x509_ext_import_key_usage(der, &usage);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (usage != (GNUTLS_KEY_KEY_CERT_SIGN | GNUTLS_KEY_CRL_SIGN)) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	return 0;
}

static int subject_key_id(const gnutls_datum_t * der)
{
/*
		Subject Key Identifier (not critical):
			5d40adf0ce9440958b7e99941d925422ca72365f
*/
	int ret;
	gnutls_datum_t id;

	ret = gnutls_x509_ext_import_subject_key_id(der, &id);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (id.size != 20 ||
	    memcmp(id.data,
		   "\x5d\x40\xad\xf0\xce\x94\x40\x95\x8b\x7e\x99\x94\x1d\x92\x54\x22\xca\x72\x36\x5f",
		   20) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}
	gnutls_free(id.data);

	return 0;
}

static int crl_dist_points(const gnutls_datum_t * der)
{
	int ret;
	gnutls_x509_crl_dist_points_t dp = NULL;
	unsigned i = 0;
	unsigned flags;
	gnutls_datum_t url;
	unsigned type;

/*
		CRL Distribution points (not critical):
			URI: http://www.getcrl.crl/getcrl1/
			URI: http://www.getcrl.crl/getcrl2/
			URI: http://www.getcrl.crl/getcrl3/
*/

	ret = gnutls_x509_crl_dist_points_init(&dp);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_crl_dist_points(der, dp, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_crl_dist_points_get(dp, i++, &type, &url, &flags);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_URI || flags != 0 ||
	    strcmp((char *)url.data, "http://www.getcrl.crl/getcrl1/") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_crl_dist_points_get(dp, i++, &type, &url, &flags);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_URI || flags != 0 ||
	    strcmp((char *)url.data, "http://www.getcrl.crl/getcrl2/") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_crl_dist_points_get(dp, i++, &type, &url, &flags);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_URI || flags != 0 ||
	    strcmp((char *)url.data, "http://www.getcrl.crl/getcrl3/") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_crl_dist_points_get(dp, i++, &type, &url, &flags);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_x509_crl_dist_points_deinit(dp);

	return 0;
}

static int name_constraints(const gnutls_datum_t * der)
{
	int ret;
	gnutls_x509_name_constraints_t nc = NULL;
	unsigned i = 0;
	gnutls_datum_t name;
	unsigned type;

/*
		Name Constraints (critical):
			Permitted:
				DNSname: example.com
				tRFC822Name: nmav@@example.net
			Excluded:
				DNSname: test.example.com
				tRFC822Name: .example.com
*/

	ret = gnutls_x509_name_constraints_init(&nc);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_name_constraints(der, nc, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_DNSNAME || name.size != 11 ||
	    strcmp((char *)name.data, "example.com") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_RFC822NAME || name.size != 17 ||
	    strcmp((char *)name.data, "nmav@@example.net") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	i = 0;
	ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &type, &name);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_DNSNAME || name.size != 16 ||
	    strcmp((char *)name.data, "test.example.com") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &type, &name);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (type != GNUTLS_SAN_RFC822NAME || name.size != 12 ||
	    strcmp((char *)name.data, ".example.com") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &type, &name);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_x509_name_constraints_deinit(nc);

	return 0;
}

static int ext_aia(const gnutls_datum_t * der)
{
	int ret;
	gnutls_x509_aia_t aia = NULL;
	unsigned i = 0;
	gnutls_datum_t oid;
	gnutls_datum_t name;
	unsigned type;

/*		Authority Information Access (not critical):
			Access Method: 1.3.6.1.5.5.7.48.1 (id-ad-ocsp)
			Access Location URI: http://my.ocsp.server/ocsp
*/
	ret = gnutls_x509_aia_init(&aia);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_ext_import_aia(der, aia, 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	ret = gnutls_x509_aia_get(aia, i++, &oid, &type, &name);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return ret;
	}

	if (strcmp((char *)oid.data, "1.3.6.1.5.5.7.48.1") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	if (type != GNUTLS_SAN_URI || name.size != 26 ||
	    strcmp((char *)name.data, "http://my.ocsp.server/ocsp") != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	ret = gnutls_x509_aia_get(aia, i++, &oid, &type, &name);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "error in %d\n", __LINE__);
		return -1;
	}

	gnutls_x509_aia_deinit(aia);

	return 0;
}

struct ext_handler_st handlers[] = {
	{GNUTLS_X509EXT_OID_BASIC_CONSTRAINTS, basic_constraints, 1},
	{GNUTLS_X509EXT_OID_SAN, subject_alt_name, 0},
	{GNUTLS_X509EXT_OID_CRT_POLICY, crt_policies, 0},
	{GNUTLS_X509EXT_OID_EXTENDED_KEY_USAGE, ext_key_usage, 0},
	{GNUTLS_X509EXT_OID_KEY_USAGE, key_usage, 1},
	{GNUTLS_X509EXT_OID_SUBJECT_KEY_ID, subject_key_id, 0},
	{GNUTLS_X509EXT_OID_CRL_DIST_POINTS, crl_dist_points, 0},
	{GNUTLS_X509EXT_OID_NAME_CONSTRAINTS, name_constraints, 1},
	{GNUTLS_X509EXT_OID_AUTHORITY_INFO_ACCESS, ext_aia, 0},
	{NULL, NULL}
};

void doit(void)
{
	int ret;
	gnutls_datum_t derCert = { (void *)pem, sizeof(pem)-1 };
	gnutls_datum_t v1Cert = { (void *)invalid_cert, sizeof(invalid_cert)-1 };
	gnutls_x509_crt_t cert;
	size_t oid_len = MAX_DATA_SIZE;
	gnutls_datum_t ext;
	char oid[MAX_DATA_SIZE];
	unsigned int critical = 0;
	unsigned i, j;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0)
		fail("crt_init %d\n", ret);

	ret = gnutls_x509_crt_import(cert, &v1Cert, GNUTLS_X509_FMT_PEM);
	if (ret >= 0)
		fail("crt_import of v1 cert with extensions should have failed: %d\n", ret);
	gnutls_x509_crt_deinit(cert);

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0)
		fail("crt_init %d\n", ret);

	ret = gnutls_x509_crt_import(cert, &derCert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("crt_import %d\n", ret);

	for (i = 0;; i++) {
		oid_len = sizeof(oid);
		ret =
		    gnutls_x509_crt_get_extension_info(cert, i, oid, &oid_len,
							&critical);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			if (i != 9) {
				fail("unexpected number of extensions: %d\n",
				     i);
			}
			break;
		}

		if (ret < 0) {
			fail("error in %d: %s\n", __LINE__,
			     gnutls_strerror(ret));
		}

		ret = gnutls_x509_crt_get_extension_data2(cert, i, &ext);
		if (ret < 0) {
			fail("error in %d: %s\n", __LINE__,
			     gnutls_strerror(ret));
		}

		/* find the handler for this extension and run it */
		for (j = 0;; j++) {
			if (handlers[j].oid == NULL) {
				fail("could not find handler for extension %s\n", oid);
				break;
			}

			if (strcmp(handlers[j].oid, oid) == 0) {
				if (critical != handlers[j].critical) {
					fail("error in %d (%s)\n", __LINE__,
					     oid);
				}

				ret = handlers[j].handler(&ext);
				if (ret < 0) {
					fail("error in %d (%s): %s\n", __LINE__,
					     oid, gnutls_strerror(ret));
				}
				break;
			}
		}
		gnutls_free(ext.data);
		ext.data = NULL;
	}

	if (debug)
		success("done\n");

	gnutls_x509_crt_deinit(cert);
	gnutls_global_deinit();
}

/* The template used to generate the certificate */

/*
# X.509 Certificate options
#
# DN options

# The organization of the subject.
organization = "Koko inc."

# The organizational unit of the subject.
unit = "CA dept."

# The locality of the subject.
# locality =

# The state of the certificate owner.
state = "Attiki"

# The country of the subject. Two letter code.
country = GR

# The common name of the certificate owner.
cn = "Cindy Lauper"

# A user id of the certificate owner.
uid = "clauper"

# This is deprecated and should not be used in new
# certificates.
pkcs9_email = "none@none.org"

# The serial number of the certificate
serial = 7

# In how many days, counting from today, this certificate will expire.
expiration_days = -1

# X.509 v3 extensions

# A dnsname in case of a WWW server.
dns_name = "www.none.org"
dns_name = "www.morethanone.org"

# An IP address in case of a server.
ip_address = "192.168.1.1"

dns_name = "www.evenmorethanone.org"

# An email in case of a person
email = "none@none.org"

# An URL that has CRLs (certificate revocation lists)
# available. Needed in CA certificates.
crl_dist_points = "http://www.getcrl.crl/getcrl1/"
crl_dist_points = "http://www.getcrl.crl/getcrl2/"
crl_dist_points = "http://www.getcrl.crl/getcrl3/"

email = "where@none.org"

# Whether this is a CA certificate or not
ca
path_len = 4

nc_permit_dns = example.com
nc_exclude_dns = test.example.com
nc_permit_email = nmav@@example.net
nc_exclude_email = .example.com

proxy_policy_language = 1.3.6.1.5.5.7.21.1

policy1 = 1.3.6.1.4.1.5484.1.10.99.1.0
policy1_txt = "This is a long policy to summarize"
policy1_url = http://www.example.com/a-policy-to-read
  
policy2 = 1.3.6.1.4.1.5484.1.10.99.1.1
policy2_txt = "This is a short policy"
policy2_url = http://www.example.com/another-policy-to-read

ocsp_uri = http://my.ocsp.server/ocsp

# Whether this certificate will be used for a TLS client
#tls_www_client

# Whether this certificate will be used for a TLS server
#tls_www_server

# Whether this certificate will be used to sign data (needed
# in TLS DHE ciphersuites).
signing_key

# Whether this certificate will be used to encrypt data (needed
# in TLS RSA ciphersuites). Note that it is preferred to use different
# keys for encryption and signing.
#encryption_key

# Whether this key will be used to sign other certificates.
cert_signing_key

# Whether this key will be used to sign CRLs.
crl_signing_key

# Whether this key will be used to sign code.
code_signing_key

# Whether this key will be used to sign OCSP data.
ocsp_signing_key

# Whether this key will be used for time stamping.
#time_stamping_key

# Whether this key will be used for IPsec IKE operations.
#ipsec_ike_key

*/
