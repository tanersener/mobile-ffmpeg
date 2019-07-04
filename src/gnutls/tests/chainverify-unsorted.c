/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* gnutls_trust_list_*().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}


const char ca_str[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICPDCCAaUCEHC65B0Q2Sk0tjjKewPMur8wDQYJKoZIhvcNAQECBQAwXzELMAkG\n"
    "A1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFz\n"
    "cyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTk2\n"
    "MDEyOTAwMDAwMFoXDTI4MDgwMTIzNTk1OVowXzELMAkGA1UEBhMCVVMxFzAVBgNV\n"
    "BAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFzcyAzIFB1YmxpYyBQcmlt\n"
    "YXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGfMA0GCSqGSIb3DQEBAQUAA4GN\n"
    "ADCBiQKBgQDJXFme8huKARS0EN8EQNvjV69qRUCPhAwL0TPZ2RHP7gJYHyX3KqhE\n"
    "BarsAx94f56TuZoAqiN91qyFomNFx3InzPRMxnVx0jnvT0Lwdd8KkMaOIG+YD/is\n"
    "I19wKTakyYbnsZogy1Olhec9vn2a/iRFM9x2Fe0PonFkTGUugWhFpwIDAQABMA0G\n"
    "CSqGSIb3DQEBAgUAA4GBALtMEivPLCYATxQT3ab7/AoRhIzzKBxnki98tsX63/Do\n"
    "lbwdj2wsqFHMc9ikwFPwTtYmwHYBV4GSXiHx0bH/59AhWM1pF+NEHJwZRDmJXNyc\n"
    "AA9WjQKZ7aKQRUzkuxCkPfAyAw7xzvjoyVGM5mKf5p/AfbdynMk2OmufTqj/ZA1k\n"
    "-----END CERTIFICATE-----\n";
const gnutls_datum_t ca = { (void *) ca_str, sizeof(ca_str) };


/* Chain1 is sorted */
static const char chain1[] = {
	/* chain[0] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCDCCBPCgAwIBAgIQakrDGzEQ5utI8PxRo5oXHzANBgkqhkiG9w0BAQUFADCB\n"
	    "vjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
	    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMv\n"
	    "VmVyaVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0Ew\n"
	    "HhcNMDcwNTA5MDAwMDAwWhcNMDkwNTA4MjM1OTU5WjCCAUAxEDAOBgNVBAUTBzI0\n"
	    "OTc4ODYxEzARBgsrBgEEAYI3PAIBAxMCVVMxGTAXBgsrBgEEAYI3PAIBAhMIRGVs\n"
	    "YXdhcmUxCzAJBgNVBAYTAlVTMQ4wDAYDVQQRFAU5NDA0MzETMBEGA1UECBMKQ2Fs\n"
	    "aWZvcm5pYTEWMBQGA1UEBxQNTW91bnRhaW4gVmlldzEiMCAGA1UECRQZNDg3IEVh\n"
	    "c3QgTWlkZGxlZmllbGQgUm9hZDEXMBUGA1UEChQOVmVyaVNpZ24sIEluYy4xJTAj\n"
	    "BgNVBAsUHFByb2R1Y3Rpb24gU2VjdXJpdHkgU2VydmljZXMxMzAxBgNVBAsUKlRl\n"
	    "cm1zIG9mIHVzZSBhdCB3d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjEZMBcGA1UE\n"
	    "AxQQd3d3LnZlcmlzaWduLmNvbTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n"
	    "xxA35ev879drgQCpENGRQ3ARaCPz/WneT9dtMe3qGNvzXQJs6cjm1Bx8XegyW1gB\n"
	    "jJX5Zl4WWbr9wpAWZ1YyJ0bEyShIGmkU8fPfbcXYwSyWoWwvE5NRaUB2ztmfAVdv\n"
	    "OaGMUKxny2Dnj3tAdaQ+FOeRDJJYg6K1hzczq/otOfsCAwEAAaOCAf8wggH7MAkG\n"
	    "A1UdEwQCMAAwHQYDVR0OBBYEFPFaiZNVR0u6UfVO4MsWVfTXzDhnMAsGA1UdDwQE\n"
	    "AwIFoDA+BgNVHR8ENzA1MDOgMaAvhi1odHRwOi8vRVZJbnRsLWNybC52ZXJpc2ln\n"
	    "bi5jb20vRVZJbnRsMjAwNi5jcmwwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXBjAq\n"
	    "MCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQGA1Ud\n"
	    "JQQtMCsGCCsGAQUFBwMBBggrBgEFBQcDAgYJYIZIAYb4QgQBBgorBgEEAYI3CgMD\n"
	    "MB8GA1UdIwQYMBaAFE5DyB127zdTek/yWG+U8zji1b3fMHYGCCsGAQUFBwEBBGow\n"
	    "aDArBggrBgEFBQcwAYYfaHR0cDovL0VWSW50bC1vY3NwLnZlcmlzaWduLmNvbTA5\n"
	    "BggrBgEFBQcwAoYtaHR0cDovL0VWSW50bC1haWEudmVyaXNpZ24uY29tL0VWSW50\n"
	    "bDIwMDYuY2VyMG0GCCsGAQUFBwEMBGEwX6FdoFswWTBXMFUWCWltYWdlL2dpZjAh\n"
	    "MB8wBwYFKw4DAhoEFI/l0xqGrI2Oa8PPgGrUSBgsexkuMCUWI2h0dHA6Ly9sb2dv\n"
	    "LnZlcmlzaWduLmNvbS92c2xvZ28uZ2lmMA0GCSqGSIb3DQEBBQUAA4IBAQBEueAg\n"
	    "xZJrjGPKAZk1NT8VtTn0yi87i9XUnSOnkFkAuI3THDd+cWbNSUzc5uFJg42GhMK7\n"
	    "S1Rojm8FHxESovLvimH/w111BKF9wNU2XSOb9KohfYq3GRiQG8O7v9JwIjjLepkc\n"
	    "iyITx7sYiJ+kwZlrNBwN6TwVHrONg6NzyzSnxCg+XgKRbJu2PqEQb6uQVkYhb+Oq\n"
	    "Vi9d4by9YqpnuXImSffQ0OZ/6s3Rl6vY08zIPqa6OVfjGs/H45ETblzezcUKpX0L\n"
	    "cqnOwUB9dVuPhtlX3X/hgz/ROxz96NBwwzha58HUgfEfkVtm+piI6TTI7XxS/7Av\n"
	    "nKMfhbyFQYPQ6J9g\n" "-----END CERTIFICATE-----\n"
	    /* chain[1] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIGCjCCBPKgAwIBAgIQESoAbTflEG/WynzD77rMGDANBgkqhkiG9w0BAQUFADCB\n"
	    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
	    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
	    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
	    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMTYxMTA3MjM1OTU5WjCBvjEL\n"
	    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
	    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMvVmVy\n"
	    "aVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0EwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC9Voi6iDRkZM/NyrDu5xlzxXLZ\n"
	    "u0W8taj/g74cA9vtibcuEBolvFXKQaGfC88ZXnC5XjlLnjEcX4euKqqoK6IbOxAj\n"
	    "XxOx3QiMThTag4HjtYzjaO0kZ85Wtqybc5ZE24qMs9bwcZOO23FUSutzWWqPcFEs\n"
	    "A5+X0cwRerxiDZUqyRx1V+n1x+q6hDXLx4VafuRN4RGXfQ4gNEXb8aIJ6+s9nriW\n"
	    "Q140SwglHkMaotm3igE0PcP45a9PjP/NZfAjTsWXs1zakByChQ0GDcEitnsopAPD\n"
	    "TFPRWLxyvAg5/KB2qKjpS26IPeOzMSWMcylIDjJ5Bu09Q/T25On8fb6OCNUfAgMB\n"
	    "AAGjggH0MIIB8DAdBgNVHQ4EFgQUTkPIHXbvN1N6T/JYb5TzOOLVvd8wEgYDVR0T\n"
	    "AQH/BAgwBgEB/wIBADA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczA9BgNVHR8ENjA0MDKgMKAuhixo\n"
	    "dHRwOi8vRVZTZWN1cmUtY3JsLnZlcmlzaWduLmNvbS9wY2EzLWc1LmNybDAgBgNV\n"
	    "HSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDgYDVR0PAQH/BAQDAgEGMBEG\n"
	    "CWCGSAGG+EIBAQQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFn\n"
	    "ZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRw\n"
	    "Oi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjApBgNVHREEIjAgpB4wHDEa\n"
	    "MBgGA1UEAxMRQ2xhc3MzQ0EyMDQ4LTEtNDgwPQYIKwYBBQUHAQEEMTAvMC0GCCsG\n"
	    "AQUFBzABhiFodHRwOi8vRVZTZWN1cmUtb2NzcC52ZXJpc2lnbi5jb20wHwYDVR0j\n"
	    "BBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJKoZIhvcNAQEFBQADggEBAFqi\n"
	    "sb/rjdQ4qIBywtw4Lqyncfkro7tHu21pbxA2mIzHVi67vKtKm3rW8oKT4BT+is6D\n"
	    "t4Pbk4errGV5Sf1XqbHOCR+6EBXECQ5i4/kKJdVkmPDyqA92Mn6R5hjuvOfa0E6N\n"
	    "eLvincBZK8DOlQ0kDHLKNF5wIokrSrDxaIfz7kSNKEB3OW5IckUxXWs5DoYC6maZ\n"
	    "kzEP32fepp+MnUzOcW86Ifa5ND/5btia9z7a84Ffelxtj3z2mXS3/+QXXe1hXqtI\n"
	    "u5aNZkU5tBIK9nDpnHYiS2DpKhs0Sfei1GfAsSatE7rZhAHBq+GObXAWO3eskZq7\n"
	    "Gh/aWKfkT8Fhrryi/ks=\n" "-----END CERTIFICATE-----\n"
	    /* chain[2] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIE/zCCBGigAwIBAgIQY5Jrio9Agv2swDvTeCmmwDANBgkqhkiG9w0BAQUFADBf\n"
	    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xNzA1BgNVBAsT\n"
	    "LkNsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkw\n"
	    "HhcNMDYxMTA4MDAwMDAwWhcNMjExMTA3MjM1OTU5WjCByjELMAkGA1UEBhMCVVMx\n"
	    "FzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVz\n"
	    "dCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2lnbiwgSW5jLiAtIEZv\n"
	    "ciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJpU2lnbiBDbGFzcyAz\n"
	    "IFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IC0gRzUwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1nmAMqudLO07cfLw8\n"
	    "RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbext0uz/o9+B1fs70Pb\n"
	    "ZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIzSdhDY2pSS9KP6HBR\n"
	    "TdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQGBO+QueQA5N06tRn/\n"
	    "Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+rCpSx4/VBEnkjWNH\n"
	    "iDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/NIeWiu5T6CUVAgMB\n"
	    "AAGjggHKMIIBxjAPBgNVHRMBAf8EBTADAQH/MDEGA1UdHwQqMCgwJqAkoCKGIGh0\n"
	    "dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMuY3JsMA4GA1UdDwEB/wQEAwIBBjBt\n"
	    "BggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIa\n"
	    "BBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5j\n"
	    "b20vdnNsb2dvLmdpZjA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczAdBgNVHQ4EFgQUf9Nlp8Ld7Lvw\n"
	    "MAnzQzn6Aq8zMTMwgYAGA1UdIwR5MHehY6RhMF8xCzAJBgNVBAYTAlVTMRcwFQYD\n"
	    "VQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMgUHJp\n"
	    "bWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eYIQcLrkHRDZKTS2OMp7A8y6vzAg\n"
	    "BgNVHSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDQYJKoZIhvcNAQEFBQAD\n"
	    "gYEAUNfnArcMK6xK11/59ADJdeNqKOck4skH3qw6WCAYQxfrcn4eobTInOn5G3Gu\n"
	    "39g6DapSHmBex2UtZSxvKnJVlWYQgE4P4wGoXdzV69YdCNssXNVVc59DYhDH05dZ\n"
	    "P4sJH99fucYDkJjUgRUYw35ww0OFwKgUp3CxiizbXxCqEQc=\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[3] (CA) */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIICPDCCAaUCEHC65B0Q2Sk0tjjKewPMur8wDQYJKoZIhvcNAQECBQAwXzELMAkG\n"
	    "A1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFz\n"
	    "cyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTk2\n"
	    "MDEyOTAwMDAwMFoXDTI4MDgwMTIzNTk1OVowXzELMAkGA1UEBhMCVVMxFzAVBgNV\n"
	    "BAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFzcyAzIFB1YmxpYyBQcmlt\n"
	    "YXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGfMA0GCSqGSIb3DQEBAQUAA4GN\n"
	    "ADCBiQKBgQDJXFme8huKARS0EN8EQNvjV69qRUCPhAwL0TPZ2RHP7gJYHyX3KqhE\n"
	    "BarsAx94f56TuZoAqiN91qyFomNFx3InzPRMxnVx0jnvT0Lwdd8KkMaOIG+YD/is\n"
	    "I19wKTakyYbnsZogy1Olhec9vn2a/iRFM9x2Fe0PonFkTGUugWhFpwIDAQABMA0G\n"
	    "CSqGSIb3DQEBAgUAA4GBALtMEivPLCYATxQT3ab7/AoRhIzzKBxnki98tsX63/Do\n"
	    "lbwdj2wsqFHMc9ikwFPwTtYmwHYBV4GSXiHx0bH/59AhWM1pF+NEHJwZRDmJXNyc\n"
	    "AA9WjQKZ7aKQRUzkuxCkPfAyAw7xzvjoyVGM5mKf5p/AfbdynMk2OmufTqj/ZA1k\n"
	    "-----END CERTIFICATE-----\n"
};

/* Chain2 is unsorted - reverse order */
static const char chain2[] = {
	/* chain[0] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCDCCBPCgAwIBAgIQakrDGzEQ5utI8PxRo5oXHzANBgkqhkiG9w0BAQUFADCB\n"
	    "vjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
	    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMv\n"
	    "VmVyaVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0Ew\n"
	    "HhcNMDcwNTA5MDAwMDAwWhcNMDkwNTA4MjM1OTU5WjCCAUAxEDAOBgNVBAUTBzI0\n"
	    "OTc4ODYxEzARBgsrBgEEAYI3PAIBAxMCVVMxGTAXBgsrBgEEAYI3PAIBAhMIRGVs\n"
	    "YXdhcmUxCzAJBgNVBAYTAlVTMQ4wDAYDVQQRFAU5NDA0MzETMBEGA1UECBMKQ2Fs\n"
	    "aWZvcm5pYTEWMBQGA1UEBxQNTW91bnRhaW4gVmlldzEiMCAGA1UECRQZNDg3IEVh\n"
	    "c3QgTWlkZGxlZmllbGQgUm9hZDEXMBUGA1UEChQOVmVyaVNpZ24sIEluYy4xJTAj\n"
	    "BgNVBAsUHFByb2R1Y3Rpb24gU2VjdXJpdHkgU2VydmljZXMxMzAxBgNVBAsUKlRl\n"
	    "cm1zIG9mIHVzZSBhdCB3d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjEZMBcGA1UE\n"
	    "AxQQd3d3LnZlcmlzaWduLmNvbTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n"
	    "xxA35ev879drgQCpENGRQ3ARaCPz/WneT9dtMe3qGNvzXQJs6cjm1Bx8XegyW1gB\n"
	    "jJX5Zl4WWbr9wpAWZ1YyJ0bEyShIGmkU8fPfbcXYwSyWoWwvE5NRaUB2ztmfAVdv\n"
	    "OaGMUKxny2Dnj3tAdaQ+FOeRDJJYg6K1hzczq/otOfsCAwEAAaOCAf8wggH7MAkG\n"
	    "A1UdEwQCMAAwHQYDVR0OBBYEFPFaiZNVR0u6UfVO4MsWVfTXzDhnMAsGA1UdDwQE\n"
	    "AwIFoDA+BgNVHR8ENzA1MDOgMaAvhi1odHRwOi8vRVZJbnRsLWNybC52ZXJpc2ln\n"
	    "bi5jb20vRVZJbnRsMjAwNi5jcmwwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXBjAq\n"
	    "MCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQGA1Ud\n"
	    "JQQtMCsGCCsGAQUFBwMBBggrBgEFBQcDAgYJYIZIAYb4QgQBBgorBgEEAYI3CgMD\n"
	    "MB8GA1UdIwQYMBaAFE5DyB127zdTek/yWG+U8zji1b3fMHYGCCsGAQUFBwEBBGow\n"
	    "aDArBggrBgEFBQcwAYYfaHR0cDovL0VWSW50bC1vY3NwLnZlcmlzaWduLmNvbTA5\n"
	    "BggrBgEFBQcwAoYtaHR0cDovL0VWSW50bC1haWEudmVyaXNpZ24uY29tL0VWSW50\n"
	    "bDIwMDYuY2VyMG0GCCsGAQUFBwEMBGEwX6FdoFswWTBXMFUWCWltYWdlL2dpZjAh\n"
	    "MB8wBwYFKw4DAhoEFI/l0xqGrI2Oa8PPgGrUSBgsexkuMCUWI2h0dHA6Ly9sb2dv\n"
	    "LnZlcmlzaWduLmNvbS92c2xvZ28uZ2lmMA0GCSqGSIb3DQEBBQUAA4IBAQBEueAg\n"
	    "xZJrjGPKAZk1NT8VtTn0yi87i9XUnSOnkFkAuI3THDd+cWbNSUzc5uFJg42GhMK7\n"
	    "S1Rojm8FHxESovLvimH/w111BKF9wNU2XSOb9KohfYq3GRiQG8O7v9JwIjjLepkc\n"
	    "iyITx7sYiJ+kwZlrNBwN6TwVHrONg6NzyzSnxCg+XgKRbJu2PqEQb6uQVkYhb+Oq\n"
	    "Vi9d4by9YqpnuXImSffQ0OZ/6s3Rl6vY08zIPqa6OVfjGs/H45ETblzezcUKpX0L\n"
	    "cqnOwUB9dVuPhtlX3X/hgz/ROxz96NBwwzha58HUgfEfkVtm+piI6TTI7XxS/7Av\n"
	    "nKMfhbyFQYPQ6J9g\n" "-----END CERTIFICATE-----\n"
	    /* chain[3] (CA) */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIICPDCCAaUCEHC65B0Q2Sk0tjjKewPMur8wDQYJKoZIhvcNAQECBQAwXzELMAkG\n"
	    "A1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFz\n"
	    "cyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTk2\n"
	    "MDEyOTAwMDAwMFoXDTI4MDgwMTIzNTk1OVowXzELMAkGA1UEBhMCVVMxFzAVBgNV\n"
	    "BAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFzcyAzIFB1YmxpYyBQcmlt\n"
	    "YXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGfMA0GCSqGSIb3DQEBAQUAA4GN\n"
	    "ADCBiQKBgQDJXFme8huKARS0EN8EQNvjV69qRUCPhAwL0TPZ2RHP7gJYHyX3KqhE\n"
	    "BarsAx94f56TuZoAqiN91qyFomNFx3InzPRMxnVx0jnvT0Lwdd8KkMaOIG+YD/is\n"
	    "I19wKTakyYbnsZogy1Olhec9vn2a/iRFM9x2Fe0PonFkTGUugWhFpwIDAQABMA0G\n"
	    "CSqGSIb3DQEBAgUAA4GBALtMEivPLCYATxQT3ab7/AoRhIzzKBxnki98tsX63/Do\n"
	    "lbwdj2wsqFHMc9ikwFPwTtYmwHYBV4GSXiHx0bH/59AhWM1pF+NEHJwZRDmJXNyc\n"
	    "AA9WjQKZ7aKQRUzkuxCkPfAyAw7xzvjoyVGM5mKf5p/AfbdynMk2OmufTqj/ZA1k\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[2] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIE/zCCBGigAwIBAgIQY5Jrio9Agv2swDvTeCmmwDANBgkqhkiG9w0BAQUFADBf\n"
	    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xNzA1BgNVBAsT\n"
	    "LkNsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkw\n"
	    "HhcNMDYxMTA4MDAwMDAwWhcNMjExMTA3MjM1OTU5WjCByjELMAkGA1UEBhMCVVMx\n"
	    "FzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVz\n"
	    "dCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2lnbiwgSW5jLiAtIEZv\n"
	    "ciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJpU2lnbiBDbGFzcyAz\n"
	    "IFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IC0gRzUwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1nmAMqudLO07cfLw8\n"
	    "RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbext0uz/o9+B1fs70Pb\n"
	    "ZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIzSdhDY2pSS9KP6HBR\n"
	    "TdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQGBO+QueQA5N06tRn/\n"
	    "Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+rCpSx4/VBEnkjWNH\n"
	    "iDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/NIeWiu5T6CUVAgMB\n"
	    "AAGjggHKMIIBxjAPBgNVHRMBAf8EBTADAQH/MDEGA1UdHwQqMCgwJqAkoCKGIGh0\n"
	    "dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMuY3JsMA4GA1UdDwEB/wQEAwIBBjBt\n"
	    "BggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIa\n"
	    "BBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5j\n"
	    "b20vdnNsb2dvLmdpZjA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczAdBgNVHQ4EFgQUf9Nlp8Ld7Lvw\n"
	    "MAnzQzn6Aq8zMTMwgYAGA1UdIwR5MHehY6RhMF8xCzAJBgNVBAYTAlVTMRcwFQYD\n"
	    "VQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMgUHJp\n"
	    "bWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eYIQcLrkHRDZKTS2OMp7A8y6vzAg\n"
	    "BgNVHSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDQYJKoZIhvcNAQEFBQAD\n"
	    "gYEAUNfnArcMK6xK11/59ADJdeNqKOck4skH3qw6WCAYQxfrcn4eobTInOn5G3Gu\n"
	    "39g6DapSHmBex2UtZSxvKnJVlWYQgE4P4wGoXdzV69YdCNssXNVVc59DYhDH05dZ\n"
	    "P4sJH99fucYDkJjUgRUYw35ww0OFwKgUp3CxiizbXxCqEQc=\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[1] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCjCCBPKgAwIBAgIQESoAbTflEG/WynzD77rMGDANBgkqhkiG9w0BAQUFADCB\n"
	    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
	    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
	    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
	    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMTYxMTA3MjM1OTU5WjCBvjEL\n"
	    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
	    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMvVmVy\n"
	    "aVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0EwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC9Voi6iDRkZM/NyrDu5xlzxXLZ\n"
	    "u0W8taj/g74cA9vtibcuEBolvFXKQaGfC88ZXnC5XjlLnjEcX4euKqqoK6IbOxAj\n"
	    "XxOx3QiMThTag4HjtYzjaO0kZ85Wtqybc5ZE24qMs9bwcZOO23FUSutzWWqPcFEs\n"
	    "A5+X0cwRerxiDZUqyRx1V+n1x+q6hDXLx4VafuRN4RGXfQ4gNEXb8aIJ6+s9nriW\n"
	    "Q140SwglHkMaotm3igE0PcP45a9PjP/NZfAjTsWXs1zakByChQ0GDcEitnsopAPD\n"
	    "TFPRWLxyvAg5/KB2qKjpS26IPeOzMSWMcylIDjJ5Bu09Q/T25On8fb6OCNUfAgMB\n"
	    "AAGjggH0MIIB8DAdBgNVHQ4EFgQUTkPIHXbvN1N6T/JYb5TzOOLVvd8wEgYDVR0T\n"
	    "AQH/BAgwBgEB/wIBADA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczA9BgNVHR8ENjA0MDKgMKAuhixo\n"
	    "dHRwOi8vRVZTZWN1cmUtY3JsLnZlcmlzaWduLmNvbS9wY2EzLWc1LmNybDAgBgNV\n"
	    "HSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDgYDVR0PAQH/BAQDAgEGMBEG\n"
	    "CWCGSAGG+EIBAQQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFn\n"
	    "ZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRw\n"
	    "Oi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjApBgNVHREEIjAgpB4wHDEa\n"
	    "MBgGA1UEAxMRQ2xhc3MzQ0EyMDQ4LTEtNDgwPQYIKwYBBQUHAQEEMTAvMC0GCCsG\n"
	    "AQUFBzABhiFodHRwOi8vRVZTZWN1cmUtb2NzcC52ZXJpc2lnbi5jb20wHwYDVR0j\n"
	    "BBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJKoZIhvcNAQEFBQADggEBAFqi\n"
	    "sb/rjdQ4qIBywtw4Lqyncfkro7tHu21pbxA2mIzHVi67vKtKm3rW8oKT4BT+is6D\n"
	    "t4Pbk4errGV5Sf1XqbHOCR+6EBXECQ5i4/kKJdVkmPDyqA92Mn6R5hjuvOfa0E6N\n"
	    "eLvincBZK8DOlQ0kDHLKNF5wIokrSrDxaIfz7kSNKEB3OW5IckUxXWs5DoYC6maZ\n"
	    "kzEP32fepp+MnUzOcW86Ifa5ND/5btia9z7a84Ffelxtj3z2mXS3/+QXXe1hXqtI\n"
	    "u5aNZkU5tBIK9nDpnHYiS2DpKhs0Sfei1GfAsSatE7rZhAHBq+GObXAWO3eskZq7\n"
	    "Gh/aWKfkT8Fhrryi/ks=\n" "-----END CERTIFICATE-----\n"
};

/* Chain3 is unsorted - random order */
static const char chain3[] = {
	/* chain[0] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCDCCBPCgAwIBAgIQakrDGzEQ5utI8PxRo5oXHzANBgkqhkiG9w0BAQUFADCB\n"
	    "vjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
	    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMv\n"
	    "VmVyaVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0Ew\n"
	    "HhcNMDcwNTA5MDAwMDAwWhcNMDkwNTA4MjM1OTU5WjCCAUAxEDAOBgNVBAUTBzI0\n"
	    "OTc4ODYxEzARBgsrBgEEAYI3PAIBAxMCVVMxGTAXBgsrBgEEAYI3PAIBAhMIRGVs\n"
	    "YXdhcmUxCzAJBgNVBAYTAlVTMQ4wDAYDVQQRFAU5NDA0MzETMBEGA1UECBMKQ2Fs\n"
	    "aWZvcm5pYTEWMBQGA1UEBxQNTW91bnRhaW4gVmlldzEiMCAGA1UECRQZNDg3IEVh\n"
	    "c3QgTWlkZGxlZmllbGQgUm9hZDEXMBUGA1UEChQOVmVyaVNpZ24sIEluYy4xJTAj\n"
	    "BgNVBAsUHFByb2R1Y3Rpb24gU2VjdXJpdHkgU2VydmljZXMxMzAxBgNVBAsUKlRl\n"
	    "cm1zIG9mIHVzZSBhdCB3d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjEZMBcGA1UE\n"
	    "AxQQd3d3LnZlcmlzaWduLmNvbTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n"
	    "xxA35ev879drgQCpENGRQ3ARaCPz/WneT9dtMe3qGNvzXQJs6cjm1Bx8XegyW1gB\n"
	    "jJX5Zl4WWbr9wpAWZ1YyJ0bEyShIGmkU8fPfbcXYwSyWoWwvE5NRaUB2ztmfAVdv\n"
	    "OaGMUKxny2Dnj3tAdaQ+FOeRDJJYg6K1hzczq/otOfsCAwEAAaOCAf8wggH7MAkG\n"
	    "A1UdEwQCMAAwHQYDVR0OBBYEFPFaiZNVR0u6UfVO4MsWVfTXzDhnMAsGA1UdDwQE\n"
	    "AwIFoDA+BgNVHR8ENzA1MDOgMaAvhi1odHRwOi8vRVZJbnRsLWNybC52ZXJpc2ln\n"
	    "bi5jb20vRVZJbnRsMjAwNi5jcmwwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXBjAq\n"
	    "MCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQGA1Ud\n"
	    "JQQtMCsGCCsGAQUFBwMBBggrBgEFBQcDAgYJYIZIAYb4QgQBBgorBgEEAYI3CgMD\n"
	    "MB8GA1UdIwQYMBaAFE5DyB127zdTek/yWG+U8zji1b3fMHYGCCsGAQUFBwEBBGow\n"
	    "aDArBggrBgEFBQcwAYYfaHR0cDovL0VWSW50bC1vY3NwLnZlcmlzaWduLmNvbTA5\n"
	    "BggrBgEFBQcwAoYtaHR0cDovL0VWSW50bC1haWEudmVyaXNpZ24uY29tL0VWSW50\n"
	    "bDIwMDYuY2VyMG0GCCsGAQUFBwEMBGEwX6FdoFswWTBXMFUWCWltYWdlL2dpZjAh\n"
	    "MB8wBwYFKw4DAhoEFI/l0xqGrI2Oa8PPgGrUSBgsexkuMCUWI2h0dHA6Ly9sb2dv\n"
	    "LnZlcmlzaWduLmNvbS92c2xvZ28uZ2lmMA0GCSqGSIb3DQEBBQUAA4IBAQBEueAg\n"
	    "xZJrjGPKAZk1NT8VtTn0yi87i9XUnSOnkFkAuI3THDd+cWbNSUzc5uFJg42GhMK7\n"
	    "S1Rojm8FHxESovLvimH/w111BKF9wNU2XSOb9KohfYq3GRiQG8O7v9JwIjjLepkc\n"
	    "iyITx7sYiJ+kwZlrNBwN6TwVHrONg6NzyzSnxCg+XgKRbJu2PqEQb6uQVkYhb+Oq\n"
	    "Vi9d4by9YqpnuXImSffQ0OZ/6s3Rl6vY08zIPqa6OVfjGs/H45ETblzezcUKpX0L\n"
	    "cqnOwUB9dVuPhtlX3X/hgz/ROxz96NBwwzha58HUgfEfkVtm+piI6TTI7XxS/7Av\n"
	    "nKMfhbyFQYPQ6J9g\n" "-----END CERTIFICATE-----\n"
	    /* chain[2] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIE/zCCBGigAwIBAgIQY5Jrio9Agv2swDvTeCmmwDANBgkqhkiG9w0BAQUFADBf\n"
	    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xNzA1BgNVBAsT\n"
	    "LkNsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkw\n"
	    "HhcNMDYxMTA4MDAwMDAwWhcNMjExMTA3MjM1OTU5WjCByjELMAkGA1UEBhMCVVMx\n"
	    "FzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVz\n"
	    "dCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2lnbiwgSW5jLiAtIEZv\n"
	    "ciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJpU2lnbiBDbGFzcyAz\n"
	    "IFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IC0gRzUwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1nmAMqudLO07cfLw8\n"
	    "RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbext0uz/o9+B1fs70Pb\n"
	    "ZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIzSdhDY2pSS9KP6HBR\n"
	    "TdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQGBO+QueQA5N06tRn/\n"
	    "Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+rCpSx4/VBEnkjWNH\n"
	    "iDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/NIeWiu5T6CUVAgMB\n"
	    "AAGjggHKMIIBxjAPBgNVHRMBAf8EBTADAQH/MDEGA1UdHwQqMCgwJqAkoCKGIGh0\n"
	    "dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMuY3JsMA4GA1UdDwEB/wQEAwIBBjBt\n"
	    "BggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIa\n"
	    "BBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5j\n"
	    "b20vdnNsb2dvLmdpZjA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczAdBgNVHQ4EFgQUf9Nlp8Ld7Lvw\n"
	    "MAnzQzn6Aq8zMTMwgYAGA1UdIwR5MHehY6RhMF8xCzAJBgNVBAYTAlVTMRcwFQYD\n"
	    "VQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMgUHJp\n"
	    "bWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eYIQcLrkHRDZKTS2OMp7A8y6vzAg\n"
	    "BgNVHSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDQYJKoZIhvcNAQEFBQAD\n"
	    "gYEAUNfnArcMK6xK11/59ADJdeNqKOck4skH3qw6WCAYQxfrcn4eobTInOn5G3Gu\n"
	    "39g6DapSHmBex2UtZSxvKnJVlWYQgE4P4wGoXdzV69YdCNssXNVVc59DYhDH05dZ\n"
	    "P4sJH99fucYDkJjUgRUYw35ww0OFwKgUp3CxiizbXxCqEQc=\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[3] (CA) */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIICPDCCAaUCEHC65B0Q2Sk0tjjKewPMur8wDQYJKoZIhvcNAQECBQAwXzELMAkG\n"
	    "A1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFz\n"
	    "cyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTk2\n"
	    "MDEyOTAwMDAwMFoXDTI4MDgwMTIzNTk1OVowXzELMAkGA1UEBhMCVVMxFzAVBgNV\n"
	    "BAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFzcyAzIFB1YmxpYyBQcmlt\n"
	    "YXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGfMA0GCSqGSIb3DQEBAQUAA4GN\n"
	    "ADCBiQKBgQDJXFme8huKARS0EN8EQNvjV69qRUCPhAwL0TPZ2RHP7gJYHyX3KqhE\n"
	    "BarsAx94f56TuZoAqiN91qyFomNFx3InzPRMxnVx0jnvT0Lwdd8KkMaOIG+YD/is\n"
	    "I19wKTakyYbnsZogy1Olhec9vn2a/iRFM9x2Fe0PonFkTGUugWhFpwIDAQABMA0G\n"
	    "CSqGSIb3DQEBAgUAA4GBALtMEivPLCYATxQT3ab7/AoRhIzzKBxnki98tsX63/Do\n"
	    "lbwdj2wsqFHMc9ikwFPwTtYmwHYBV4GSXiHx0bH/59AhWM1pF+NEHJwZRDmJXNyc\n"
	    "AA9WjQKZ7aKQRUzkuxCkPfAyAw7xzvjoyVGM5mKf5p/AfbdynMk2OmufTqj/ZA1k\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[1] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCjCCBPKgAwIBAgIQESoAbTflEG/WynzD77rMGDANBgkqhkiG9w0BAQUFADCB\n"
	    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
	    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
	    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
	    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMTYxMTA3MjM1OTU5WjCBvjEL\n"
	    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
	    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMvVmVy\n"
	    "aVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0EwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC9Voi6iDRkZM/NyrDu5xlzxXLZ\n"
	    "u0W8taj/g74cA9vtibcuEBolvFXKQaGfC88ZXnC5XjlLnjEcX4euKqqoK6IbOxAj\n"
	    "XxOx3QiMThTag4HjtYzjaO0kZ85Wtqybc5ZE24qMs9bwcZOO23FUSutzWWqPcFEs\n"
	    "A5+X0cwRerxiDZUqyRx1V+n1x+q6hDXLx4VafuRN4RGXfQ4gNEXb8aIJ6+s9nriW\n"
	    "Q140SwglHkMaotm3igE0PcP45a9PjP/NZfAjTsWXs1zakByChQ0GDcEitnsopAPD\n"
	    "TFPRWLxyvAg5/KB2qKjpS26IPeOzMSWMcylIDjJ5Bu09Q/T25On8fb6OCNUfAgMB\n"
	    "AAGjggH0MIIB8DAdBgNVHQ4EFgQUTkPIHXbvN1N6T/JYb5TzOOLVvd8wEgYDVR0T\n"
	    "AQH/BAgwBgEB/wIBADA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczA9BgNVHR8ENjA0MDKgMKAuhixo\n"
	    "dHRwOi8vRVZTZWN1cmUtY3JsLnZlcmlzaWduLmNvbS9wY2EzLWc1LmNybDAgBgNV\n"
	    "HSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDgYDVR0PAQH/BAQDAgEGMBEG\n"
	    "CWCGSAGG+EIBAQQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFn\n"
	    "ZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRw\n"
	    "Oi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjApBgNVHREEIjAgpB4wHDEa\n"
	    "MBgGA1UEAxMRQ2xhc3MzQ0EyMDQ4LTEtNDgwPQYIKwYBBQUHAQEEMTAvMC0GCCsG\n"
	    "AQUFBzABhiFodHRwOi8vRVZTZWN1cmUtb2NzcC52ZXJpc2lnbi5jb20wHwYDVR0j\n"
	    "BBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJKoZIhvcNAQEFBQADggEBAFqi\n"
	    "sb/rjdQ4qIBywtw4Lqyncfkro7tHu21pbxA2mIzHVi67vKtKm3rW8oKT4BT+is6D\n"
	    "t4Pbk4errGV5Sf1XqbHOCR+6EBXECQ5i4/kKJdVkmPDyqA92Mn6R5hjuvOfa0E6N\n"
	    "eLvincBZK8DOlQ0kDHLKNF5wIokrSrDxaIfz7kSNKEB3OW5IckUxXWs5DoYC6maZ\n"
	    "kzEP32fepp+MnUzOcW86Ifa5ND/5btia9z7a84Ffelxtj3z2mXS3/+QXXe1hXqtI\n"
	    "u5aNZkU5tBIK9nDpnHYiS2DpKhs0Sfei1GfAsSatE7rZhAHBq+GObXAWO3eskZq7\n"
	    "Gh/aWKfkT8Fhrryi/ks=\n" "-----END CERTIFICATE-----\n"
};

/* Chain4 is unsorted - random order and includes random certs */
static const char chain4[] = {
	/* chain[0] */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIIGCDCCBPCgAwIBAgIQakrDGzEQ5utI8PxRo5oXHzANBgkqhkiG9w0BAQUFADCB\n"
	    "vjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
	    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMv\n"
	    "VmVyaVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0Ew\n"
	    "HhcNMDcwNTA5MDAwMDAwWhcNMDkwNTA4MjM1OTU5WjCCAUAxEDAOBgNVBAUTBzI0\n"
	    "OTc4ODYxEzARBgsrBgEEAYI3PAIBAxMCVVMxGTAXBgsrBgEEAYI3PAIBAhMIRGVs\n"
	    "YXdhcmUxCzAJBgNVBAYTAlVTMQ4wDAYDVQQRFAU5NDA0MzETMBEGA1UECBMKQ2Fs\n"
	    "aWZvcm5pYTEWMBQGA1UEBxQNTW91bnRhaW4gVmlldzEiMCAGA1UECRQZNDg3IEVh\n"
	    "c3QgTWlkZGxlZmllbGQgUm9hZDEXMBUGA1UEChQOVmVyaVNpZ24sIEluYy4xJTAj\n"
	    "BgNVBAsUHFByb2R1Y3Rpb24gU2VjdXJpdHkgU2VydmljZXMxMzAxBgNVBAsUKlRl\n"
	    "cm1zIG9mIHVzZSBhdCB3d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjEZMBcGA1UE\n"
	    "AxQQd3d3LnZlcmlzaWduLmNvbTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n"
	    "xxA35ev879drgQCpENGRQ3ARaCPz/WneT9dtMe3qGNvzXQJs6cjm1Bx8XegyW1gB\n"
	    "jJX5Zl4WWbr9wpAWZ1YyJ0bEyShIGmkU8fPfbcXYwSyWoWwvE5NRaUB2ztmfAVdv\n"
	    "OaGMUKxny2Dnj3tAdaQ+FOeRDJJYg6K1hzczq/otOfsCAwEAAaOCAf8wggH7MAkG\n"
	    "A1UdEwQCMAAwHQYDVR0OBBYEFPFaiZNVR0u6UfVO4MsWVfTXzDhnMAsGA1UdDwQE\n"
	    "AwIFoDA+BgNVHR8ENzA1MDOgMaAvhi1odHRwOi8vRVZJbnRsLWNybC52ZXJpc2ln\n"
	    "bi5jb20vRVZJbnRsMjAwNi5jcmwwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXBjAq\n"
	    "MCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQGA1Ud\n"
	    "JQQtMCsGCCsGAQUFBwMBBggrBgEFBQcDAgYJYIZIAYb4QgQBBgorBgEEAYI3CgMD\n"
	    "MB8GA1UdIwQYMBaAFE5DyB127zdTek/yWG+U8zji1b3fMHYGCCsGAQUFBwEBBGow\n"
	    "aDArBggrBgEFBQcwAYYfaHR0cDovL0VWSW50bC1vY3NwLnZlcmlzaWduLmNvbTA5\n"
	    "BggrBgEFBQcwAoYtaHR0cDovL0VWSW50bC1haWEudmVyaXNpZ24uY29tL0VWSW50\n"
	    "bDIwMDYuY2VyMG0GCCsGAQUFBwEMBGEwX6FdoFswWTBXMFUWCWltYWdlL2dpZjAh\n"
	    "MB8wBwYFKw4DAhoEFI/l0xqGrI2Oa8PPgGrUSBgsexkuMCUWI2h0dHA6Ly9sb2dv\n"
	    "LnZlcmlzaWduLmNvbS92c2xvZ28uZ2lmMA0GCSqGSIb3DQEBBQUAA4IBAQBEueAg\n"
	    "xZJrjGPKAZk1NT8VtTn0yi87i9XUnSOnkFkAuI3THDd+cWbNSUzc5uFJg42GhMK7\n"
	    "S1Rojm8FHxESovLvimH/w111BKF9wNU2XSOb9KohfYq3GRiQG8O7v9JwIjjLepkc\n"
	    "iyITx7sYiJ+kwZlrNBwN6TwVHrONg6NzyzSnxCg+XgKRbJu2PqEQb6uQVkYhb+Oq\n"
	    "Vi9d4by9YqpnuXImSffQ0OZ/6s3Rl6vY08zIPqa6OVfjGs/H45ETblzezcUKpX0L\n"
	    "cqnOwUB9dVuPhtlX3X/hgz/ROxz96NBwwzha58HUgfEfkVtm+piI6TTI7XxS/7Av\n"
	    "nKMfhbyFQYPQ6J9g\n" "-----END CERTIFICATE-----\n"
	    /* chain[2] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIE/zCCBGigAwIBAgIQY5Jrio9Agv2swDvTeCmmwDANBgkqhkiG9w0BAQUFADBf\n"
	    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xNzA1BgNVBAsT\n"
	    "LkNsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkw\n"
	    "HhcNMDYxMTA4MDAwMDAwWhcNMjExMTA3MjM1OTU5WjCByjELMAkGA1UEBhMCVVMx\n"
	    "FzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVz\n"
	    "dCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2lnbiwgSW5jLiAtIEZv\n"
	    "ciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJpU2lnbiBDbGFzcyAz\n"
	    "IFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IC0gRzUwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1nmAMqudLO07cfLw8\n"
	    "RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbext0uz/o9+B1fs70Pb\n"
	    "ZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIzSdhDY2pSS9KP6HBR\n"
	    "TdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQGBO+QueQA5N06tRn/\n"
	    "Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+rCpSx4/VBEnkjWNH\n"
	    "iDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/NIeWiu5T6CUVAgMB\n"
	    "AAGjggHKMIIBxjAPBgNVHRMBAf8EBTADAQH/MDEGA1UdHwQqMCgwJqAkoCKGIGh0\n"
	    "dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMuY3JsMA4GA1UdDwEB/wQEAwIBBjBt\n"
	    "BggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIa\n"
	    "BBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5j\n"
	    "b20vdnNsb2dvLmdpZjA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczAdBgNVHQ4EFgQUf9Nlp8Ld7Lvw\n"
	    "MAnzQzn6Aq8zMTMwgYAGA1UdIwR5MHehY6RhMF8xCzAJBgNVBAYTAlVTMRcwFQYD\n"
	    "VQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMgUHJp\n"
	    "bWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eYIQcLrkHRDZKTS2OMp7A8y6vzAg\n"
	    "BgNVHSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDQYJKoZIhvcNAQEFBQAD\n"
	    "gYEAUNfnArcMK6xK11/59ADJdeNqKOck4skH3qw6WCAYQxfrcn4eobTInOn5G3Gu\n"
	    "39g6DapSHmBex2UtZSxvKnJVlWYQgE4P4wGoXdzV69YdCNssXNVVc59DYhDH05dZ\n"
	    "P4sJH99fucYDkJjUgRUYw35ww0OFwKgUp3CxiizbXxCqEQc=\n"
	    "-----END CERTIFICATE-----\n"
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIEczCCA9ygAwIBAgIQeODCPg2RbK2r7/1KoWjWZzANBgkqhkiG9w0BAQUFADCB\n"
	    "ujEfMB0GA1UEChMWVmVyaVNpZ24gVHJ1c3QgTmV0d29yazEXMBUGA1UECxMOVmVy\n"
	    "aVNpZ24sIEluYy4xMzAxBgNVBAsTKlZlcmlTaWduIEludGVybmF0aW9uYWwgU2Vy\n"
	    "dmVyIENBIC0gQ2xhc3MgMzFJMEcGA1UECxNAd3d3LnZlcmlzaWduLmNvbS9DUFMg\n"
	    "SW5jb3JwLmJ5IFJlZi4gTElBQklMSVRZIExURC4oYyk5NyBWZXJpU2lnbjAeFw0w\n"
	    "ODA2MTAwMDAwMDBaFw0wOTA3MzAyMzU5NTlaMIG2MQswCQYDVQQGEwJERTEPMA0G\n"
	    "A1UECBMGSGVzc2VuMRowGAYDVQQHFBFGcmFua2Z1cnQgYW0gTWFpbjEsMCoGA1UE\n"
	    "ChQjU3Bhcmthc3NlbiBJbmZvcm1hdGlrIEdtYkggJiBDby4gS0cxKTAnBgNVBAsU\n"
	    "IFRlcm1zIG9mIHVzZSBhdCB3d3cudmVyaXNpZ24uY29tMSEwHwYDVQQDFBhoYmNp\n"
	    "LXBpbnRhbi1ycC5zLWhiY2kuZGUwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGB\n"
	    "AK1CdQ9lqmChZWaRAInimuK7I36VImTuAVU0N6BIS4a2BbblkiekbVf15GVHGb6e\n"
	    "QV06ANN6Nd8XIdfoxi3LoAs8sa+Ku7eoEsRFi/XIU96GgtFlxf3EsVA9RbGdtfer\n"
	    "9iJGIBae2mJTlk+5LVg2EQr50PJlBuTgiYFc41xs9O2RAgMBAAGjggF6MIIBdjAJ\n"
	    "BgNVHRMEAjAAMAsGA1UdDwQEAwIFoDBGBgNVHR8EPzA9MDugOaA3hjVodHRwOi8v\n"
	    "Y3JsLnZlcmlzaWduLmNvbS9DbGFzczNJbnRlcm5hdGlvbmFsU2VydmVyLmNybDBE\n"
	    "BgNVHSAEPTA7MDkGC2CGSAGG+EUBBxcDMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v\n"
	    "d3d3LnZlcmlzaWduLmNvbS9ycGEwKAYDVR0lBCEwHwYJYIZIAYb4QgQBBggrBgEF\n"
	    "BQcDAQYIKwYBBQUHAwIwNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRw\n"
	    "Oi8vb2NzcC52ZXJpc2lnbi5jb20wbgYIKwYBBQUHAQwEYjBgoV6gXDBaMFgwVhYJ\n"
	    "aW1hZ2UvZ2lmMCEwHzAHBgUrDgMCGgQUS2u5KJYGDLvQUjibKaxLB4shBRgwJhYk\n"
	    "aHR0cDovL2xvZ28udmVyaXNpZ24uY29tL3ZzbG9nbzEuZ2lmMA0GCSqGSIb3DQEB\n"
	    "BQUAA4GBAJ03R0YAjYzlWm54gMSn6MqJi0mHdLCO2lk3CARwjbg7TEYAZvDsKqTd\n"
	    "cRuhNk079BqrQ3QapffeN55SAVrc3mzHO54Nla4n5y6x3XIQXVvRjbJGwmWXsdvr\n"
	    "W899F/pBEN30Tgdbmn7JR/iZlGhIJpY9Us1i7rwQhKYir9ZQBdj3\n"
	    "-----END CERTIFICATE-----\n"
	    /* chain[1] */
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIGCjCCBPKgAwIBAgIQESoAbTflEG/WynzD77rMGDANBgkqhkiG9w0BAQUFADCB\n"
	    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
	    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
	    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
	    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
	    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMTYxMTA3MjM1OTU5WjCBvjEL\n"
	    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
	    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMvVmVy\n"
	    "aVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0EwggEi\n"
	    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC9Voi6iDRkZM/NyrDu5xlzxXLZ\n"
	    "u0W8taj/g74cA9vtibcuEBolvFXKQaGfC88ZXnC5XjlLnjEcX4euKqqoK6IbOxAj\n"
	    "XxOx3QiMThTag4HjtYzjaO0kZ85Wtqybc5ZE24qMs9bwcZOO23FUSutzWWqPcFEs\n"
	    "A5+X0cwRerxiDZUqyRx1V+n1x+q6hDXLx4VafuRN4RGXfQ4gNEXb8aIJ6+s9nriW\n"
	    "Q140SwglHkMaotm3igE0PcP45a9PjP/NZfAjTsWXs1zakByChQ0GDcEitnsopAPD\n"
	    "TFPRWLxyvAg5/KB2qKjpS26IPeOzMSWMcylIDjJ5Bu09Q/T25On8fb6OCNUfAgMB\n"
	    "AAGjggH0MIIB8DAdBgNVHQ4EFgQUTkPIHXbvN1N6T/JYb5TzOOLVvd8wEgYDVR0T\n"
	    "AQH/BAgwBgEB/wIBADA9BgNVHSAENjA0MDIGBFUdIAAwKjAoBggrBgEFBQcCARYc\n"
	    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2NwczA9BgNVHR8ENjA0MDKgMKAuhixo\n"
	    "dHRwOi8vRVZTZWN1cmUtY3JsLnZlcmlzaWduLmNvbS9wY2EzLWc1LmNybDAgBgNV\n"
	    "HSUEGTAXBglghkgBhvhCBAEGCmCGSAGG+EUBCAEwDgYDVR0PAQH/BAQDAgEGMBEG\n"
	    "CWCGSAGG+EIBAQQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglpbWFn\n"
	    "ZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNodHRw\n"
	    "Oi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjApBgNVHREEIjAgpB4wHDEa\n"
	    "MBgGA1UEAxMRQ2xhc3MzQ0EyMDQ4LTEtNDgwPQYIKwYBBQUHAQEEMTAvMC0GCCsG\n"
	    "AQUFBzABhiFodHRwOi8vRVZTZWN1cmUtb2NzcC52ZXJpc2lnbi5jb20wHwYDVR0j\n"
	    "BBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJKoZIhvcNAQEFBQADggEBAFqi\n"
	    "sb/rjdQ4qIBywtw4Lqyncfkro7tHu21pbxA2mIzHVi67vKtKm3rW8oKT4BT+is6D\n"
	    "t4Pbk4errGV5Sf1XqbHOCR+6EBXECQ5i4/kKJdVkmPDyqA92Mn6R5hjuvOfa0E6N\n"
	    "eLvincBZK8DOlQ0kDHLKNF5wIokrSrDxaIfz7kSNKEB3OW5IckUxXWs5DoYC6maZ\n"
	    "kzEP32fepp+MnUzOcW86Ifa5ND/5btia9z7a84Ffelxtj3z2mXS3/+QXXe1hXqtI\n"
	    "u5aNZkU5tBIK9nDpnHYiS2DpKhs0Sfei1GfAsSatE7rZhAHBq+GObXAWO3eskZq7\n"
	    "Gh/aWKfkT8Fhrryi/ks=\n" "-----END CERTIFICATE-----\n"
	    /* chain[3] (CA) */
	"-----BEGIN CERTIFICATE-----\n"
	    "MIICPDCCAaUCEHC65B0Q2Sk0tjjKewPMur8wDQYJKoZIhvcNAQECBQAwXzELMAkG\n"
	    "A1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFz\n"
	    "cyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTk2\n"
	    "MDEyOTAwMDAwMFoXDTI4MDgwMTIzNTk1OVowXzELMAkGA1UEBhMCVVMxFzAVBgNV\n"
	    "BAoTDlZlcmlTaWduLCBJbmMuMTcwNQYDVQQLEy5DbGFzcyAzIFB1YmxpYyBQcmlt\n"
	    "YXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGfMA0GCSqGSIb3DQEBAQUAA4GN\n"
	    "ADCBiQKBgQDJXFme8huKARS0EN8EQNvjV69qRUCPhAwL0TPZ2RHP7gJYHyX3KqhE\n"
	    "BarsAx94f56TuZoAqiN91qyFomNFx3InzPRMxnVx0jnvT0Lwdd8KkMaOIG+YD/is\n"
	    "I19wKTakyYbnsZogy1Olhec9vn2a/iRFM9x2Fe0PonFkTGUugWhFpwIDAQABMA0G\n"
	    "CSqGSIb3DQEBAgUAA4GBALtMEivPLCYATxQT3ab7/AoRhIzzKBxnki98tsX63/Do\n"
	    "lbwdj2wsqFHMc9ikwFPwTtYmwHYBV4GSXiHx0bH/59AhWM1pF+NEHJwZRDmJXNyc\n"
	    "AA9WjQKZ7aKQRUzkuxCkPfAyAw7xzvjoyVGM5mKf5p/AfbdynMk2OmufTqj/ZA1k\n"
	    "-----END CERTIFICATE-----\n"
	    "-----BEGIN CERTIFICATE-----\n"
	    "MIIDgzCCAuygAwIBAgIQJUuKhThCzONY+MXdriJupDANBgkqhkiG9w0BAQUFADBf\n"
	    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xNzA1BgNVBAsT\n"
	    "LkNsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkw\n"
	    "HhcNOTcwNDE3MDAwMDAwWhcNMTExMDI0MjM1OTU5WjCBujEfMB0GA1UEChMWVmVy\n"
	    "aVNpZ24gVHJ1c3QgTmV0d29yazEXMBUGA1UECxMOVmVyaVNpZ24sIEluYy4xMzAx\n"
	    "BgNVBAsTKlZlcmlTaWduIEludGVybmF0aW9uYWwgU2VydmVyIENBIC0gQ2xhc3Mg\n"
	    "MzFJMEcGA1UECxNAd3d3LnZlcmlzaWduLmNvbS9DUFMgSW5jb3JwLmJ5IFJlZi4g\n"
	    "TElBQklMSVRZIExURC4oYyk5NyBWZXJpU2lnbjCBnzANBgkqhkiG9w0BAQEFAAOB\n"
	    "jQAwgYkCgYEA2IKA6NYZAn0fhRg5JaJlK+G/1AXTvOY2O6rwTGxbtueqPHNFVbLx\n"
	    "veqXQu2aNAoV1Klc9UAl3dkHwTKydWzEyruj/lYncUOqY/UwPpMo5frxCTvzt01O\n"
	    "OfdcSVq4wR3Tsor+cDCVQsv+K1GLWjw6+SJPkLICp1OcTzTnqwSye28CAwEAAaOB\n"
	    "4zCB4DAPBgNVHRMECDAGAQH/AgEAMEQGA1UdIAQ9MDswOQYLYIZIAYb4RQEHAQEw\n"
	    "KjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL0NQUzA0BgNV\n"
	    "HSUELTArBggrBgEFBQcDAQYIKwYBBQUHAwIGCWCGSAGG+EIEAQYKYIZIAYb4RQEI\n"
	    "ATALBgNVHQ8EBAMCAQYwEQYJYIZIAYb4QgEBBAQDAgEGMDEGA1UdHwQqMCgwJqAk\n"
	    "oCKGIGh0dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMuY3JsMA0GCSqGSIb3DQEB\n"
	    "BQUAA4GBAAgB7ORolANC8XPxI6I63unx2sZUxCM+hurPajozq+qcBBQHNgYL+Yhv\n"
	    "1RPuKSvD5HKNRO3RrCAJLeH24RkFOLA9D59/+J4C3IYChmFOJl9en5IeDCSk9dBw\n"
	    "E88mw0M9SR2egi5SX7w+xmYpAY5Okiy8RnUDgqxz6dl+C2fvVFIa\n"
	    "-----END CERTIFICATE-----\n"
};

static time_t mytime(time_t * t)
{
	time_t then = 1207000800;

	if (t)
		*t = then;

	return then;
}

void doit(void)
{
	int ret;
	gnutls_datum_t data;
	gnutls_x509_crt_t *crts;
	unsigned int crts_size, i;
	gnutls_x509_trust_list_t tl;
	unsigned int status, flags = GNUTLS_VERIFY_ALLOW_UNSORTED_CHAIN|GNUTLS_VERIFY_ALLOW_BROKEN;
	unsigned int not_flags = GNUTLS_VERIFY_DO_NOT_ALLOW_UNSORTED_CHAIN;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* test for gnutls_certificate_get_issuer() */
	gnutls_x509_trust_list_init(&tl, 0);

	ret =
	    gnutls_x509_trust_list_add_trust_mem(tl, &ca, NULL,
						 GNUTLS_X509_FMT_PEM, 0,
						 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_trust_mem\n");
		exit(1);
	}

	/* Chain 1 */
	data.data = (void *) chain1;
	data.size = sizeof(chain1);
	ret =
	    gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("gnutls_x509_crt_list_import2: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, crts, crts_size, flags,
					      &status, NULL);
	if (ret < 0 || status != 0) {
		fail("gnutls_x509_trust_list_verify_crt - 1\n");
		exit(1);
	}

	for (i = 0; i < crts_size; i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	/* Chain 2 */
	data.data = (void *) chain2;
	data.size = sizeof(chain2);

	/* verify whether the GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED flag is
	 * considered by gnutls_x509_crt_list_import2() */
	ret =
		gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret != GNUTLS_E_CERTIFICATE_LIST_UNSORTED) {
		fail("gnutls_x509_crt_list_import2 with flag GNUTLS_E_CERTIFICATE_LIST_UNSORTED on unsorted chain didn't fail: %s\n",  gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("gnutls_x509_crt_list_import2: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, crts, crts_size, flags,
					      &status, NULL);
	if (ret < 0 || status != 0) {
		fail("gnutls_x509_trust_list_verify_crt - 2\n");
		exit(1);
	}

	for (i = 0; i < crts_size; i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	/* Chain 3 */
	data.data = (void *) chain3;
	data.size = sizeof(chain3);
	ret =
	    gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("gnutls_x509_crt_list_import2: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, crts, crts_size, flags,
					      &status, NULL);
	if (ret < 0 || status != 0) {
		fail("gnutls_x509_trust_list_verify_crt - 3\n");
		exit(1);
	}

	for (i = 0; i < crts_size; i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	/* Chain 4 */
	data.data = (void *) chain4;
	data.size = sizeof(chain4);
	ret =
	    gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("gnutls_x509_crt_list_import2: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, crts, crts_size, flags,
					      &status, NULL);
	if (ret < 0 || status != 0) {
		fail("gnutls_x509_trust_list_verify_crt - 4\n");
		exit(1);
	}

	for (i = 0; i < crts_size; i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	/* Check if an unsorted list would fail if the unsorted flag is not given */
	data.data = (void *) chain2;
	data.size = sizeof(chain2);
	ret =
	    gnutls_x509_crt_list_import2(&crts, &crts_size, &data,
					 GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("gnutls_x509_crt_list_import2: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, crts, crts_size,
					      not_flags, &status, NULL);
	if (ret < 0 || status == 0) {
		fail("gnutls_x509_trust_list_verify_crt - 5\n");
		exit(1);
	}

	for (i = 0; i < crts_size; i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	gnutls_x509_trust_list_deinit(tl, 1);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
