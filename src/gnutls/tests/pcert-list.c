/*
 * Copyright (C) 2015 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>
#include "utils.h"

/* This tests functions related to pcert-lists
 */

#define CERT0 \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEITCCAomgAwIBAgIMVsXM+TCHHodT4TxYMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n" \
"BgNVBAMTBENBLTIwIBcNMTYwMjE4MTM1NDAxWhgPOTk5OTEyMzEyMzU5NTlaMBMx\n" \
"ETAPBgNVBAMTCHNlcnZlci0zMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKC\n" \
"AYEAs6z83Jg9XjIuBb87zm6uuBjGG+45IpSw6gRgU/1izgBUofefrkdvjhpneBYU\n" \
"7PySNxTcyKUe1ZijKAYwck5jE76Y/xKNdMffgYqXOusCij7xutssdtvYw7yJjUHv\n" \
"43+zqbydONRNebO8qw1/BGXzKCsAE83iYumxJxSkwTsq04Kp9vrfW6zaTpa3VGq5\n" \
"wYPBT+neszrT9/E/Bn+QJh66US+EYnl+TlI5XTp4J0XqGP8PB1OYG/WPPjdRgv7j\n" \
"C/dSsEaLmV2YdQWjPRqZ+hxQbRJbLaJ9b7czBSdK1lhefAKshUEV+SGQI2MzEVGW\n" \
"lP4tLpIhiy33fNWpnkhbxxsa/NnIS2Vb8JvQidKdgQLsJL8hRJ/it41B4JGiaBnM\n" \
"uQmIwr+DFbVs2ibm2VlV1oNB1DrFOAYNURSIUJM0th+Wj4vI9hnwIVeUY/u3Dk5V\n" \
"bhks+JfbPLmbJ7Tx9JiBCes7isuxNCtWrWRDUQj71IqCc2+iV86Q+gw3rcpLeLYN\n" \
"yv3PAgMBAAGjdzB1MAwGA1UdEwEB/wQCMAAwFAYDVR0RBA0wC4IJbG9jYWxob3N0\n" \
"MA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0OBBYEFEnsKuMM/IbHLD1TnAK78YwFx0VF\n" \
"MB8GA1UdIwQYMBaAFFGRr1BCIq0AHmB59tUBsghMjvz/MA0GCSqGSIb3DQEBCwUA\n" \
"A4IBgQB0i38qq4/os7MIhUFnBFD/eduk5B+jaGvPTM8lsZJ/17BbiMBc5dyxjMVY\n" \
"WYsm+KI5XEddBEqMYYwjdO/aoJzFLkkDu7E+UnygVZmMdQONuoyeQ/IrLk3l3zGi\n" \
"JJlylxFBNkns+a4AnXwSAv/ZiZapjQQUX378IxOpZuqzELAPqCkqp/6LyJApDiVV\n" \
"9av7WWySG5Wtp8lNs8o8l8ZxU14++fwo1euH0mQ4AM2DGLAhQSdOqChmROWt4MPd\n" \
"7raaO8dl6wMI83OgOHIhZlvlmmZTYqbpPXYm/2lM9ePBU/bkA7Y/X7HFDbTIBH9Y\n" \
"rkVZyq3FYPUtYRyqQXa8s730MQBxGmVZkKptCZjLDziZF4sAZGX78EyDeSl3Z3Jg\n" \
"I5JGLsdznHlhqEx8hNJnYtINVv1arn2UHO7p3/cB8VXt2UdQP+YJYdVzCvT4WW1E\n" \
"PvzTI6JbcwDpOs0MxRIrXrhgEZWylk0W93FO1WErd1+Sn3LZqvrtyXLzYB9wCl1I\n" \
"A34kGlE=\n" \
"-----END CERTIFICATE-----\n"

static unsigned char server_cert_pem[] =
CERT0
"-----BEGIN CERTIFICATE-----\n"
"MIIEFDCCAnygAwIBAgIBAjANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0x\n"
"MCAXDTE2MDIxODEzNTQwMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0yMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEAu1/IharA+97QfzDj\n"
"UXEBl9TAFqHkN9B5erj1yhMlwPAakreStR8VvuCx46TA3gP7sbUYU811T+2D5/GU\n"
"u7YuMWsFeSmGWvbxa/tKTpXoEM0bNV+rIbxAcfgtxbARZDocv8gxfG/70vc2dSDh\n"
"KgZCoMQyO6qGLRdsoPAf+De7YD8sKS7Q3d3Xnfyv4AVnDkbAVFsZhu4lQFuWXyfG\n"
"Sl95TT94wLDLdf/Gf/F0nNsv6+D6yb15afhJKdqo6PH19gsyE0U3zj6c/7abha2W\n"
"fvVe6hVbaW1HLDZdHZnjlJHamNFdrOHI5Xi+SJO7/3MWvdTzdMVFBDfS5o7TvYyS\n"
"pu6iTmVeJvJ1OpXV7Lw1M2dSTW9RJLzUF3fXYOsuh32qMel9IzhnVh8Veyl0I0WL\n"
"hThmkF73mGWcVq4lMPXwEnwYJtRLeH5HWvG3rgmb7m827XMNnqKE0NOkPH63OUqJ\n"
"0h4b6PBb6wiOgnsC3yZIf0KgB0gToySvmD6MyJsmbN9rQit1AgMBAAGjeTB3MA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcEADAdBgNVHQ4EFgQUUZGvUEIirQAeYHn21QGyCEyO/P8wHwYDVR0jBBgwFoAU\n"
"v9x1k1GrVS0yXKvMzD7k/zInm9gwDQYJKoZIhvcNAQELBQADggGBAIwUNzAo7Efm\n"
"X8dVGz6OEsfZ/RPIeYxZ5cmqWwcZ4oLBv55xGJNG+nIcgLMA2L6ybtFiK2nKtqy4\n"
"hMv/P6qvjX5vVQGVgLclvMkDkmXWVdqkTYDX7cSe/Bv6qIS2KBaVo87r2DIWN8Zu\n"
"J3w0U3RcD6ilXVmqvARLeKWBPrCep0DJvg/BEAFSjCgHImrpZdzm6NuUr1eYCfgN\n"
"HPwUj5Ivyy9ioPRXGzzHQH6T1p/xIRbuhqTGRUin3MqGQlFseBJ2qXPf6uQmCaWZ\n"
"tFp4oWLJThqVmlvHViPDy235roYSKkJXH4qxjbhuv0pgUZOzmSsG7YA/oYNGDm6I\n"
"bEvids1r45PjYDHctB4QLhXNY3SJVgMog1KuVCK6JQL8F8XP5Sup1qW4ed/WvXwe\n"
"PBTOWbE/ENnxF2/nQLwnr80cgVx8rAE5sxubNNQVHu/6NonPzGUhTHXmGleuXPbb\n"
"Mjv4x9s3QftWUVJb7b8GUt5bMAthqo7Y47Jed1kKIt2PAm0SNBMYrw==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIEFDCCAnygAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE2MDIxODEzNTQwMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0xMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA0QG/LnrMV5vsE3G7\n"
"mGVYr33PFQ/WojwKg1C8GGH9aaIn+VMuBS2d1/mwtM9axoje1uQzwKp/hPT+N3Z3\n"
"qmFWeX8somDwowNDWN3skST4ob/M4aSlfP6OhNeIfBNvPTz3GtsbOtN5TkOX7Nf2\n"
"+mfFm09xMHQ3z4yyGNmKG/oxGKY2WDe93hp0rlIZ6ihMPcsCHwWLfja3SAT4AcXs\n"
"TFrTxEnaTYuVxcRcoW7lEDtcCyGbPfszo/rEQfJxwxRF46Yoz6rrCSkXOzLhQa4v\n"
"PPsZJ6ltNqkCtSrnhcCl3SC9JqdI4e7lGsnDylq4evi8RtOYknVOqDwv0q/9DI2+\n"
"rhFUy4I0Ah9H2T7dC01KIOjGiHyThCgkt2Nee/AXFflpN2Ws7/SGALdx6Vy3OkVo\n"
"NkHYxlKKn/06Yp8XlNPR64EqxeJqPW9Pf742EJUCOeavu5wPWJtLQr03JyKWoeZf\n"
"IYT/HwZUJveqEBU1EKeZRSvrRwHnmzQJuxyUhj/2C92QF5edAgMBAAGjeTB3MA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcEADAdBgNVHQ4EFgQUv9x1k1GrVS0yXKvMzD7k/zInm9gwHwYDVR0jBBgwFoAU\n"
"2iUEUyXy7fPzZtc8ktanTiDzjuUwDQYJKoZIhvcNAQELBQADggGBAC9X5og786Il\n"
"CUKj4FpZrqgfN+Cwf1EebW1tX1iKYASGo8t7JS0Btt3ycVpx04JSJy5WM9cQNFU0\n"
"5vimaG0qAsWhHXljhmM0mr4ruW1Jw6KAuqw0V/JJ0oYRZaYnvi6UsoJJjq8YcatW\n"
"5ixtKr928933kYD71sMZBN7Um7ictDq0M2oaW4k0/Yt4Uqb9fv20E4EHKEpETMUR\n"
"FviTIjONdVsAVj4lxuS3u1Nt7B5ayYCkgFabME28ud6EazelwZWZwBRGiuPr6634\n"
"f8lZtnscRVU5oQb6DjkyD/SM+1ue6/wpNapoH7BimnvCcRmLvsG34vlyt7QC0BRO\n"
"cRmEPZCq8hIUIuD0x836FRNUSjjMVi2Dj+QjeNolpKgUjRF/h2yKmDRB2A7WAV5g\n"
"It7RRjMnkm3pvKj2d7/qb5OaccO4uoAq333PRAX0RLYT5yosFGq+RN8+WCnzuGsB\n"
"hCe33/7HCC6mO0/vsrQuRvECvAasznN9mF3t+ZXMvcsqTcOq4Iag1A==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIID8zCCAlugAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE2MDIxODEzNTQwMFoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0wMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA0MYBHVUjwlQH7Mvn\n"
"4viHyEONr+7M0+fLntPITQHihE8gxU3LpqAUpl7felAA4k0sJTaXvnZA+E1DCcIq\n"
"zksAhK3Qr8zZeCKNM41U1klcCh2+3IoGjg+CcQisb8gtiiXybH3qXYFgi3ww2YFG\n"
"cIjJAciZj8qLfwMhMcBPMx4IDHR7gdWH9V0xUZZiBkk7x3PBIWCr2FKD0877yR9t\n"
"wjlQ4Fbw5NW9j7WaUgeY2LV7iTtBH0bZ7D/04KsYdct6lKhUkzSUBg/bAUWCFp1j\n"
"ouFhzyqMf3jFDrcejxPKlRk15e9SkQYD/7dTpudXwbL9ugZfoP1xDRgslEyfyU/Q\n"
"DEyG5mlXjVBRiGvL+dfxRNw2E5xLpESt2rlMiBhe1cv8+XL5D6z/WBwDfBNUzoQR\n"
"X15YHK2NgNNHQ8u8GLtUbp3ZXaeKgj8fdR3UoRTqWgpy2vjVM3vN1xXFVTo13MJ8\n"
"isLXH/QNUR4tnOytDp1HyK2ybHkfXB1a0RMBwM5XDVD2LhPFAgMBAAGjWDBWMA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcGADAdBgNVHQ4EFgQU2iUEUyXy7fPzZtc8ktanTiDzjuUwDQYJKoZIhvcNAQEL\n"
"BQADggGBAJrJujtXifCeySYWbnJlraec63Zgqfv4SZIEdLt5GFLdpjk2WCxhFrN3\n"
"n6JZgI2aUWin2OL1VA1hfddAPUSHOCV8nP/Vu1f/BEaeQjEVS2AOF7T+eQSTNQtN\n"
"MkTTi0UKKXZjIIXiW4YXDs2b22JIOXkL9rFyrvN4vvbIp/jwLWx5UTHFtsktMkai\n"
"MteJBobd69ra7kdX43EkUKrgSDNpMQn10y3w4ziPDsLZ9sWaRxESbXWqDn4A7J9t\n"
"prfxut+s/3rsZgpt4s2FsswymfuW8DhzH1EjfV1Tb32blpgz/40sIRbU158Wh1UH\n"
"/DGQ6RVX0RcRt7ce7QCYTROD/yHYPVucqLfRpVNJ3oujGYaMgnSSuxEOsfwx5u+P\n"
"8USIxyQNR9cX/gQswzs3Ouj1rXBnjiSS1YXWZXvqHsUamJ8O7qpnqkL2Ti64O0HA\n"
"wdTtAcDO0BTHvanKZojLZm8nStvTvFpSVh7z+8Fu0A5zAcHsDj4vLABsdPDsXUTr\n"
"kb2G3Yy/UA==\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char unsorted_server_cert_pem[] =
CERT0
"-----BEGIN CERTIFICATE-----\n"
"MIIEFDCCAnygAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE2MDIxODEzNTQwMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0xMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA0QG/LnrMV5vsE3G7\n"
"mGVYr33PFQ/WojwKg1C8GGH9aaIn+VMuBS2d1/mwtM9axoje1uQzwKp/hPT+N3Z3\n"
"qmFWeX8somDwowNDWN3skST4ob/M4aSlfP6OhNeIfBNvPTz3GtsbOtN5TkOX7Nf2\n"
"+mfFm09xMHQ3z4yyGNmKG/oxGKY2WDe93hp0rlIZ6ihMPcsCHwWLfja3SAT4AcXs\n"
"TFrTxEnaTYuVxcRcoW7lEDtcCyGbPfszo/rEQfJxwxRF46Yoz6rrCSkXOzLhQa4v\n"
"PPsZJ6ltNqkCtSrnhcCl3SC9JqdI4e7lGsnDylq4evi8RtOYknVOqDwv0q/9DI2+\n"
"rhFUy4I0Ah9H2T7dC01KIOjGiHyThCgkt2Nee/AXFflpN2Ws7/SGALdx6Vy3OkVo\n"
"NkHYxlKKn/06Yp8XlNPR64EqxeJqPW9Pf742EJUCOeavu5wPWJtLQr03JyKWoeZf\n"
"IYT/HwZUJveqEBU1EKeZRSvrRwHnmzQJuxyUhj/2C92QF5edAgMBAAGjeTB3MA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcEADAdBgNVHQ4EFgQUv9x1k1GrVS0yXKvMzD7k/zInm9gwHwYDVR0jBBgwFoAU\n"
"2iUEUyXy7fPzZtc8ktanTiDzjuUwDQYJKoZIhvcNAQELBQADggGBAC9X5og786Il\n"
"CUKj4FpZrqgfN+Cwf1EebW1tX1iKYASGo8t7JS0Btt3ycVpx04JSJy5WM9cQNFU0\n"
"5vimaG0qAsWhHXljhmM0mr4ruW1Jw6KAuqw0V/JJ0oYRZaYnvi6UsoJJjq8YcatW\n"
"5ixtKr928933kYD71sMZBN7Um7ictDq0M2oaW4k0/Yt4Uqb9fv20E4EHKEpETMUR\n"
"FviTIjONdVsAVj4lxuS3u1Nt7B5ayYCkgFabME28ud6EazelwZWZwBRGiuPr6634\n"
"f8lZtnscRVU5oQb6DjkyD/SM+1ue6/wpNapoH7BimnvCcRmLvsG34vlyt7QC0BRO\n"
"cRmEPZCq8hIUIuD0x836FRNUSjjMVi2Dj+QjeNolpKgUjRF/h2yKmDRB2A7WAV5g\n"
"It7RRjMnkm3pvKj2d7/qb5OaccO4uoAq333PRAX0RLYT5yosFGq+RN8+WCnzuGsB\n"
"hCe33/7HCC6mO0/vsrQuRvECvAasznN9mF3t+ZXMvcsqTcOq4Iag1A==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIID8zCCAlugAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE2MDIxODEzNTQwMFoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0wMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA0MYBHVUjwlQH7Mvn\n"
"4viHyEONr+7M0+fLntPITQHihE8gxU3LpqAUpl7felAA4k0sJTaXvnZA+E1DCcIq\n"
"zksAhK3Qr8zZeCKNM41U1klcCh2+3IoGjg+CcQisb8gtiiXybH3qXYFgi3ww2YFG\n"
"cIjJAciZj8qLfwMhMcBPMx4IDHR7gdWH9V0xUZZiBkk7x3PBIWCr2FKD0877yR9t\n"
"wjlQ4Fbw5NW9j7WaUgeY2LV7iTtBH0bZ7D/04KsYdct6lKhUkzSUBg/bAUWCFp1j\n"
"ouFhzyqMf3jFDrcejxPKlRk15e9SkQYD/7dTpudXwbL9ugZfoP1xDRgslEyfyU/Q\n"
"DEyG5mlXjVBRiGvL+dfxRNw2E5xLpESt2rlMiBhe1cv8+XL5D6z/WBwDfBNUzoQR\n"
"X15YHK2NgNNHQ8u8GLtUbp3ZXaeKgj8fdR3UoRTqWgpy2vjVM3vN1xXFVTo13MJ8\n"
"isLXH/QNUR4tnOytDp1HyK2ybHkfXB1a0RMBwM5XDVD2LhPFAgMBAAGjWDBWMA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcGADAdBgNVHQ4EFgQU2iUEUyXy7fPzZtc8ktanTiDzjuUwDQYJKoZIhvcNAQEL\n"
"BQADggGBAJrJujtXifCeySYWbnJlraec63Zgqfv4SZIEdLt5GFLdpjk2WCxhFrN3\n"
"n6JZgI2aUWin2OL1VA1hfddAPUSHOCV8nP/Vu1f/BEaeQjEVS2AOF7T+eQSTNQtN\n"
"MkTTi0UKKXZjIIXiW4YXDs2b22JIOXkL9rFyrvN4vvbIp/jwLWx5UTHFtsktMkai\n"
"MteJBobd69ra7kdX43EkUKrgSDNpMQn10y3w4ziPDsLZ9sWaRxESbXWqDn4A7J9t\n"
"prfxut+s/3rsZgpt4s2FsswymfuW8DhzH1EjfV1Tb32blpgz/40sIRbU158Wh1UH\n"
"/DGQ6RVX0RcRt7ce7QCYTROD/yHYPVucqLfRpVNJ3oujGYaMgnSSuxEOsfwx5u+P\n"
"8USIxyQNR9cX/gQswzs3Ouj1rXBnjiSS1YXWZXvqHsUamJ8O7qpnqkL2Ti64O0HA\n"
"wdTtAcDO0BTHvanKZojLZm8nStvTvFpSVh7z+8Fu0A5zAcHsDj4vLABsdPDsXUTr\n"
"kb2G3Yy/UA==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIEFDCCAnygAwIBAgIBAjANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0x\n"
"MCAXDTE2MDIxODEzNTQwMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0yMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEAu1/IharA+97QfzDj\n"
"UXEBl9TAFqHkN9B5erj1yhMlwPAakreStR8VvuCx46TA3gP7sbUYU811T+2D5/GU\n"
"u7YuMWsFeSmGWvbxa/tKTpXoEM0bNV+rIbxAcfgtxbARZDocv8gxfG/70vc2dSDh\n"
"KgZCoMQyO6qGLRdsoPAf+De7YD8sKS7Q3d3Xnfyv4AVnDkbAVFsZhu4lQFuWXyfG\n"
"Sl95TT94wLDLdf/Gf/F0nNsv6+D6yb15afhJKdqo6PH19gsyE0U3zj6c/7abha2W\n"
"fvVe6hVbaW1HLDZdHZnjlJHamNFdrOHI5Xi+SJO7/3MWvdTzdMVFBDfS5o7TvYyS\n"
"pu6iTmVeJvJ1OpXV7Lw1M2dSTW9RJLzUF3fXYOsuh32qMel9IzhnVh8Veyl0I0WL\n"
"hThmkF73mGWcVq4lMPXwEnwYJtRLeH5HWvG3rgmb7m827XMNnqKE0NOkPH63OUqJ\n"
"0h4b6PBb6wiOgnsC3yZIf0KgB0gToySvmD6MyJsmbN9rQit1AgMBAAGjeTB3MA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcEADAdBgNVHQ4EFgQUUZGvUEIirQAeYHn21QGyCEyO/P8wHwYDVR0jBBgwFoAU\n"
"v9x1k1GrVS0yXKvMzD7k/zInm9gwDQYJKoZIhvcNAQELBQADggGBAIwUNzAo7Efm\n"
"X8dVGz6OEsfZ/RPIeYxZ5cmqWwcZ4oLBv55xGJNG+nIcgLMA2L6ybtFiK2nKtqy4\n"
"hMv/P6qvjX5vVQGVgLclvMkDkmXWVdqkTYDX7cSe/Bv6qIS2KBaVo87r2DIWN8Zu\n"
"J3w0U3RcD6ilXVmqvARLeKWBPrCep0DJvg/BEAFSjCgHImrpZdzm6NuUr1eYCfgN\n"
"HPwUj5Ivyy9ioPRXGzzHQH6T1p/xIRbuhqTGRUin3MqGQlFseBJ2qXPf6uQmCaWZ\n"
"tFp4oWLJThqVmlvHViPDy235roYSKkJXH4qxjbhuv0pgUZOzmSsG7YA/oYNGDm6I\n"
"bEvids1r45PjYDHctB4QLhXNY3SJVgMog1KuVCK6JQL8F8XP5Sup1qW4ed/WvXwe\n"
"PBTOWbE/ENnxF2/nQLwnr80cgVx8rAE5sxubNNQVHu/6NonPzGUhTHXmGleuXPbb\n"
"Mjv4x9s3QftWUVJb7b8GUt5bMAthqo7Y47Jed1kKIt2PAm0SNBMYrw==\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t unsorted_server_cert = { unsorted_server_cert_pem,
	sizeof(unsorted_server_cert_pem)
};

const gnutls_datum_t single_server_cert = { server_cert_pem,
	sizeof(CERT0)-1
};

static unsigned char isolated_server_cert_pem[] =
CERT0
"-----BEGIN CERTIFICATE-----\n"
"MIID8zCCAlugAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE2MDIxODEzNTQwMFoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0wMIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA0MYBHVUjwlQH7Mvn\n"
"4viHyEONr+7M0+fLntPITQHihE8gxU3LpqAUpl7felAA4k0sJTaXvnZA+E1DCcIq\n"
"zksAhK3Qr8zZeCKNM41U1klcCh2+3IoGjg+CcQisb8gtiiXybH3qXYFgi3ww2YFG\n"
"cIjJAciZj8qLfwMhMcBPMx4IDHR7gdWH9V0xUZZiBkk7x3PBIWCr2FKD0877yR9t\n"
"wjlQ4Fbw5NW9j7WaUgeY2LV7iTtBH0bZ7D/04KsYdct6lKhUkzSUBg/bAUWCFp1j\n"
"ouFhzyqMf3jFDrcejxPKlRk15e9SkQYD/7dTpudXwbL9ugZfoP1xDRgslEyfyU/Q\n"
"DEyG5mlXjVBRiGvL+dfxRNw2E5xLpESt2rlMiBhe1cv8+XL5D6z/WBwDfBNUzoQR\n"
"X15YHK2NgNNHQ8u8GLtUbp3ZXaeKgj8fdR3UoRTqWgpy2vjVM3vN1xXFVTo13MJ8\n"
"isLXH/QNUR4tnOytDp1HyK2ybHkfXB1a0RMBwM5XDVD2LhPFAgMBAAGjWDBWMA8G\n"
"A1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwkwDwYDVR0PAQH/BAUD\n"
"AwcGADAdBgNVHQ4EFgQU2iUEUyXy7fPzZtc8ktanTiDzjuUwDQYJKoZIhvcNAQEL\n"
"BQADggGBAJrJujtXifCeySYWbnJlraec63Zgqfv4SZIEdLt5GFLdpjk2WCxhFrN3\n"
"n6JZgI2aUWin2OL1VA1hfddAPUSHOCV8nP/Vu1f/BEaeQjEVS2AOF7T+eQSTNQtN\n"
"MkTTi0UKKXZjIIXiW4YXDs2b22JIOXkL9rFyrvN4vvbIp/jwLWx5UTHFtsktMkai\n"
"MteJBobd69ra7kdX43EkUKrgSDNpMQn10y3w4ziPDsLZ9sWaRxESbXWqDn4A7J9t\n"
"prfxut+s/3rsZgpt4s2FsswymfuW8DhzH1EjfV1Tb32blpgz/40sIRbU158Wh1UH\n"
"/DGQ6RVX0RcRt7ce7QCYTROD/yHYPVucqLfRpVNJ3oujGYaMgnSSuxEOsfwx5u+P\n"
"8USIxyQNR9cX/gQswzs3Ouj1rXBnjiSS1YXWZXvqHsUamJ8O7qpnqkL2Ti64O0HA\n"
"wdTtAcDO0BTHvanKZojLZm8nStvTvFpSVh7z+8Fu0A5zAcHsDj4vLABsdPDsXUTr\n"
"kb2G3Yy/UA==\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t isolated_server_cert = { isolated_server_cert_pem,
	sizeof(isolated_server_cert_pem)
};

void doit(void)
{
	gnutls_pcert_st pcert_list[16];
	unsigned pcert_list_size, flags, i;
	int ret;

	flags = GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED;

	pcert_list_size = 3;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		fail("the GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED was not considered\n");
	}

	pcert_list_size = 3;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("the normal/smaller import has failed\n");
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);


	pcert_list_size = 16;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, 0);
	if (ret != 0 || pcert_list_size != 4) {
		fail("the normal import failed\n");
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);


	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret != 0 || pcert_list_size != 4) {
		fail("the fail if unsorted import failed\n");
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);


	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_SORT;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret != 0 || pcert_list_size != 4) {
		fail("the list sort import failed\n");
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);


	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_SORT|GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret != 0 || pcert_list_size != 4) {
		fail("the combined import failed\n");
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);

	/* try the unsorted list */
	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_SORT|GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&unsorted_server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret < 0 || pcert_list_size != 4) {
		fail("the combined import failed for the unsorted list (%d): %s\n", pcert_list_size, gnutls_strerror(ret));
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);

	/* try the single cert list */
	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_SORT|GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&single_server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret < 0 || pcert_list_size != 1) {
		fail("the combined import failed for the single cert (%d): %s\n", pcert_list_size, gnutls_strerror(ret));
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);

	/* try the single final cert list */
	pcert_list_size = 16;
	flags = GNUTLS_X509_CRT_LIST_SORT|GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED;
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
			&isolated_server_cert, GNUTLS_X509_FMT_PEM, flags);
	if (ret < 0 || pcert_list_size != 1) {
		fail("the combined import failed for the isolated cert (%d): %s\n", pcert_list_size, gnutls_strerror(ret));
	}

	for (i=0;i<pcert_list_size;i++)
		gnutls_pcert_deinit(&pcert_list[i]);

	success("all ok\n");
}
