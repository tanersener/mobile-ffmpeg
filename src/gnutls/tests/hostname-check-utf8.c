/*
 * Copyright (C) 2016 Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#ifdef ENABLE_OPENPGP
#include <gnutls/openpgp.h>
#endif

#include "utils.h"

/*
  A self-test of the RFC 2818 hostname matching algorithm for UTF-8
  certificates.
*/

char pem_inv_utf8_dns[] = "\n"
	"	Subject Alternative Name (not critical):\n"
	"			DNSname: γγγ.τόστ.gr\n"
	"			DNSname: τέστ.gr\n"
	"			DNSname: *.teχ.gr\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDWzCCAkOgAwIBAgIMU/SjEDp2nsS3kX9vMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
	"BgNVBAMTBENBLTAwIhgPMjAxNDA4MjAxMzMwNTZaGA85OTk5MTIzMTIzNTk1OVow\n"
	"EzERMA8GA1UEAxMIc2VydmVyLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
	"AoIBAQDggz41h9PcOjL7UOqx0FfZNtqoRhYQn6bVQqCehwERMDlR4QOqK3LRqE2B\n"
	"cYyVlcdS63tnNFjYCLCz3/CV4rcJBNI3hfFZHUza70iFQ72xMvcgFPyl7UmXqIne\n"
	"8swJ9jLMKou350ztPhshhXORqKxaDHBMcgD/Ade3Yxo2N1smsyINK+riged7A4QD\n"
	"O9IgR9eERQbFrHGz+WgUUgoLFLF4DN1ANpWuZcOV1f9bRB8ADPyKo1yZY1sJj1gE\n"
	"JRRsiOZLSLZ9D/1MLM7BXPuxWmWlJAGfNvrcXX/7FHe6QxC5gi1C6ZUEIZCne+Is\n"
	"HpDNoz/A9vDn6iXZJBFXKyijNpVfAgMBAAGjga4wgaswDAYDVR0TAQH/BAIwADA1\n"
	"BgNVHREELjAsghLOs86zzrMuz4TPjM+Dz4QuZ3KCC8+Ezq3Pg8+ELmdyggkqLnRl\n"
	"z4cuZ3IwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYDVR0PAQH/BAUDAwegADAdBgNV\n"
	"HQ4EFgQUvjD8gT+By/Xj/n+SGCVvL/KVElMwHwYDVR0jBBgwFoAUhU7w94kERpAh\n"
	"6DEIh3nEVJnwSaUwDQYJKoZIhvcNAQELBQADggEBAIKuSREAd6ZdcS+slbx+hvew\n"
	"IRBz5QGlCCjR4Oj5arIwFGnh0GdvAgzPa3qn6ReG1gvpe8k3X6Z2Yevw+DubLZNG\n"
	"9CsfLfDIg2wUm05cuQdQG+gTSBVqw56jWf/JFXXwzhnbjX3c2QtepFsvkOnlWGFE\n"
	"uVX6AiPfiNChVxnb4e1xpxOt6W/su19ar5J7rdDrdyVVm/ioSKvXhbBXI4f8NF2x\n"
	"wTEzbtl99HyjbLIRRCWpUU277khHLr8SSFqdSr100zIkdiB72LfPXAHVld1onV2z\n"
	"PPFYVMsnY+fuxIsTVErX3bLj6v67Bs3BNzagFUlyJl5rBGwn73UafNWz3BYDyxY=\n"
	"-----END CERTIFICATE-----\n";

char pem_utf8_dns[] = 
	"Subject Alternative Name (not critical):\n"
	"		DNSname: xn--oxaaa.xn--4xabb4a.gr (γγγ.τόστ.gr)\n"
	"		DNSname: xn--ixa8bbc.gr (τέστ.gr)\n"
	"		DNSname: *.xn--te-8bc.gr (*.teχ.gr)\n"
	"\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIEFTCCAn2gAwIBAgIMWElZgiWN43F5pluiMA0GCSqGSIb3DQEBCwUAMA0xCzAJ\n"
	"BgNVBAYTAkdSMB4XDTA0MDIyOTE1MjE0MloXDTI0MDIyOTE1MjE0MVowDTELMAkG\n"
	"A1UEBhMCR1IwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQC23cZ4hvts\n"
	"D/zjXmX70ewCWpFaOXXhSiB1U4ogVsIYPh0o3eJ3w2vr8k7f8CHZXT9T64g9UYoH\n"
	"PM+vPkcT6RnwHNfe6SpSqTtPCNC9UQyp4wVq+HxnQsxOrmf2bClYn6CGaXQvDNiG\n"
	"KQCDGoxLZx+d12dYUxL4l07J3rogk7Wqe9znkpC+9UqyDJIAZgF9e4H190sRY0FM\n"
	"zrOkDDDmt/vBlu0SPhP0sktUJDjvOtHY/V2IDp0y9tImxnFhdl5k4kAEiPiph72C\n"
	"QjSRf/Kb5siUcgRxmTvN9GgWNPg3EtmyynMjIlnzicO1p6Wju80hAuVhYKOI3aq6\n"
	"FAUHY0DQkkna7dcmKwJdUo9jzMWBV+B+eOT69rDKcAvQJz5PfrrnE9SJ4/eteam7\n"
	"l4BcIZIKSuaZz48ymh6exEpSY+P3SD05oZbeQVfgi4e7Ui81S63XRlPqLPCYp0+N\n"
	"q2nSeVedR59AtQhyGhQLgQneV0R17aym+1nJ8AjsZXL7sfYef/OOxeMCAwEAAaN1\n"
	"MHMwDAYDVR0TAQH/BAIwADBEBgNVHREEPTA7ghh4bi0tb3hhYWEueG4tLTR4YWJi\n"
	"NGEuZ3KCDnhuLS1peGE4YmJjLmdygg8qLnhuLS10ZS04YmMuZ3IwHQYDVR0OBBYE\n"
	"FPmohhljtqQUE2B2DwGaNTbv8bSvMA0GCSqGSIb3DQEBCwUAA4IBgQAOAECgc096\n"
	"3WH7G83bRmVDooGATNP0v3cmYebVu3RL77/vlCO3UOS9lVxEwlF/6V1u3OqEqwUy\n"
	"EzGInEAmqR/VIoubIVrFqzaMMjfCHdKPuyWeCb3ylp0o2lxRKbC9m/Bu8Iv5rZdN\n"
	"fTZVyJbp1Ddw4GhM0UZ/IK3h8J8UtarSijhha0UX9EwQo4wi1NRpc2nxRGy7xUHG\n"
	"GqUCFBe6cgKBEBRWh3Gha5UgwqkapA9eGGmb7CRzOHZA0raIcxwb2w2Htf7ziE1G\n"
	"UBdo0ZtpVYq/EDggP4XIvqHb8bJVFuOiu2xf71JoPgjg4+1CEj+vgkI4j/RGDjZ/\n"
	"bQ66XHY2EbCjhSLoCGpY924frilrFL3cMofdMguxtsONwUotYmCF6VI/EtELvIdf\n"
	"NbdaPqI2524oBDlD98DTJa5mGoaFUyJGotcK3e9fniIxbVW8/Ykwhqbj+9wKjYEP\n"
	"ywY/9UOj+wjwULkIxK9g91yGLRDAO/6xzCF5ly5i4oXBqKLAKZ7vBTU=\n"
	"-----END CERTIFICATE-----\n";

void doit(void)
{
	gnutls_x509_crt_t x509;
	gnutls_datum_t data;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	ret = gnutls_x509_crt_init(&x509);
	if (ret < 0)
		fail("gnutls_x509_crt_init: %d\n", ret);

	if (debug)
		success("Testing pem_invalid_utf8_dns...\n");
	data.data = (unsigned char *) pem_inv_utf8_dns;
	data.size = strlen(pem_inv_utf8_dns);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τεστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τoστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "γαβ.τόστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.in.teχ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.teχ.gr");
	if (ret)
		fail("%d: Invalid hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "γγγ.τόστ.gr");
	if (ret)
		fail("%d: Invalid hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "γΓγ.τόΣτ.gr");
	if (ret)
		fail("%d: Invalid hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τέστ.gr");
	if (ret)
		fail("%d: Invalid hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "ΤΈΣΤ.gr");
	if (ret)
		fail("%d: Invalid hostname incorrectly matches (%d)\n", __LINE__, ret);


	if (debug)
		success("Testing pem_utf8_dns...\n");
	data.data = (unsigned char *) pem_utf8_dns;
	data.size = strlen(pem_utf8_dns);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τεστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τoστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "γαβ.τόστ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.in.teχ.gr");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

#if defined(HAVE_LIBIDN) || defined(HAVE_LIBIDN2)
	ret = gnutls_x509_crt_check_hostname(x509, "www.teχ.gr");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "γγγ.τόστ.gr");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "τέστ.gr");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

#if defined(HAVE_LIBIDN) /* There are IDNA2003 */
	ret = gnutls_x509_crt_check_hostname(x509, "γΓγ.τόΣτ.gr");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "ΤΈΣΤ.gr");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);
#endif
#endif

	gnutls_x509_crt_deinit(x509);

	gnutls_global_deinit();
}
