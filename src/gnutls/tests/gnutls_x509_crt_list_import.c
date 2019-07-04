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

/* This tests key import for gnutls_x509_privkey_t APIs */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>
#include <assert.h>
#include "cert-common.h"
#include "utils.h"

#define testfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

#define MAX_CERTS 8

static void load_list(const char *name, const gnutls_datum_t *txt,
		      unsigned int ncerts,
		      unsigned int max1,
		      unsigned int max2,
		      unsigned flags, int exp_err)
{
	gnutls_x509_crt_t certs[MAX_CERTS];
	unsigned int max, i;
	unsigned retried = 0;
	int ret;

	assert(max1<=MAX_CERTS);
	assert(max2<=MAX_CERTS);

	if (max1)
		max = max1;
	else
		max = MAX_CERTS;

 retry:
	ret = gnutls_x509_crt_list_import(certs, &max, txt, GNUTLS_X509_FMT_PEM, flags);
	if (ret < 0) {
		if (retried == 0 && ret == GNUTLS_E_SHORT_MEMORY_BUFFER && max2 && max2 != max) {
			max = max2;
			retried = 1;
			goto retry;
		}
		if (exp_err == ret)
			return;
		testfail("gnutls_x509_crt_list_import: %s\n", gnutls_strerror(ret));
	}

	for (i=0;i<max;i++)
		gnutls_x509_crt_deinit(certs[i]);

	if (max != ncerts)
		testfail("imported number (%d) doesn't match expected (%d)\n", max, ncerts);

	return;
}

typedef struct test_st {
	const char *name;
	const gnutls_datum_t *certs;
	unsigned max1;
	unsigned max2;
	unsigned ncerts;
	unsigned flags;
	int exp_err;
} test_st;

static const char long_chain_pem[] = {
"-----BEGIN CERTIFICATE-----\n"
"MIIDQDCCAiigAwIBAgIMU/xyoxPcYVSaqH7/MA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
"BgNVBAMTBENBLTMwIhgPMjAxNDA4MjYxMTQyMjdaGA85OTk5MTIzMTIzNTk1OVow\n"
"EzERMA8GA1UEChMIc2VydmVyLTQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDkemVOFdbhBX1qwjxQHr3LmPktNEVBmXjrIvyp++dN7gCYzubnpiLcBE+B\n"
"S2b+ppxBYm9ynKijhGrO+lZPCQRXWmqUg4YDfvnEqM4n04dCE98jN4IhwvWZyP3p\n"
"+U8Ra9mVIBAY2MReo1dcJQHNmo560xzxioHsGNQHAfYgVRHiE5hIXchYbWCkBrKt\n"
"XOoSSTmfgCF3L22p6S1q143VoKUr/C9zqinZo6feGAiTprj6YH0tHswjGBbxTFLb\n"
"q3ThbGDR5FNYL5q0FvQRNbjoF4oFitZ3P1Qkrzq7VIJd9k8J1C3g/16U2dDTKqRX\n"
"ejX7maFZ6oRZJASsRSowEs4wTfRpAgMBAAGjgZMwgZAwDAYDVR0TAQH/BAIwADAa\n"
"BgNVHREEEzARgg93d3cuZXhhbXBsZS5jb20wEwYDVR0lBAwwCgYIKwYBBQUHAwEw\n"
"DwYDVR0PAQH/BAUDAwegADAdBgNVHQ4EFgQUAEYPmcA7S/KChiet+Z6+RRmogiww\n"
"HwYDVR0jBBgwFoAUjxZogHO3y4VdOLuibQHsQYdsGgwwDQYJKoZIhvcNAQELBQAD\n"
"ggEBABlA3npOWwl3eBycaLVOsmdPS+fUwhLnF8hxoyKpHe/33k1nIxd7iiqNZ3iw\n"
"6pAjnuRUCjajU+mlx6ekrmga8mpmeD6JH0I3lq+mrPeCeFXm8gc1yJpcFJ/C2l4o\n"
"+3HNY7RJKcfoQxIbiKOtZ6x9E0aYuk3s1Um3Pf8GLwENoou7Stg5qHsLbkN/GBuP\n"
"n3p/4iqik2k7VblldDe3oCob5vMp0qrAEhlNl2Fn65rcB4+bp1EiC1Z+y6X8DpRb\n"
"NomKUsOiGcbFjQ4ptT6fePmPHX1mgDCx+5/22cyBUYElefYP7Xzr+C8tqqO3JFKe\n"
"hqEmQRsll9bkqpu2dh83c3i9u4g=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBAzANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0y\n"
"MCIYDzIwMTQwODI2MTE0MjI3WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC/4ofaL+ilmmM+\n"
"bGaFRy5GYQXtkD8sA3+/GWsunR928fQS68Zh6iWU+gPm52i7Gfbh7piKWA5Tb63w\n"
"unbS6dPsfPSvgRMZGKJpzxqVcBQAnTS4MuDPlXNg3K3HMyVtbxekII8jFeGEJuCL\n"
"mBMT4dI48IZRzj+2mir38w2cQPfomaKtjg2jMokG8Z9/4+SU9VJCcY1/yZk8fCbS\n"
"dBbwhnDq10yvhPCHgX6KMYmoJr28CYgH29Q9sDP1XN3VvAx5X+PtW/6pyF0U5E2e\n"
"gRzVv7Hr3FJKvytbNxRMCoy2YOyvsTP0fIhiXdtkulTKXyiq4cxA+aYByOu1FjU4\n"
"NicWbiZ/AgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQUjxZogHO3y4VdOLuibQHsQYdsGgwwHwYDVR0jBBgwFoAUwAx0\n"
"aL2SrsoSZcZUuFlq0O17BSgwDQYJKoZIhvcNAQELBQADggEBAGQvj8SquT31w8JK\n"
"tHDL4hWOU0EwVwWl4aYsvP17WspiFIIHKApPFfQOD0/Wg9zB48ble5ZSwKA3Vc3B\n"
"DJgd77HgVAd/Nu1TS5TFDKhpuvFPJVpJ3cqt3pTsVGMzf6GRz5kG3Ly/pBgkqiMG\n"
"gv6vTlEvzNe4FcnhNBEaRKpK5Hc5+GnxtfVoki3tjG5u+oa9/OwzAT+7IOyiIKHw\n"
"7F4Cm56QAWMJgVNm329AjZrJLeNuKoQWGueNew4dOe/zlYEaVMG4So74twXQwIAB\n"
"Zko7+wk6eI4CkI4Zair36s1jLkCF8xnL8FExTT3sg6B6KBHaNUuwc67WPILVuFuc\n"
"VfVBOd8=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDMzCCAhugAwIBAgIBAjANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0x\n"
"MCIYDzIwMTQwODI2MTE0MjI3WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDIf3as4EONSgWu\n"
"Mbm9w3DbKd/su1UWlrYrcpVqmU3MKD5jXBxyoThSBWxmq1+wcNDmE1on6pHY1aad\n"
"k3188JKMC83wEcyQXaiH3DlTYFXXkkI+JJNUGlfAMSoXG248SpoCIOhCETUG03iP\n"
"Z3AZludaHYsv4akAh1Kl6qn66+bKM53l/YhoQDxhoGaYvO8ZSwKnx5DEiq447jpW\n"
"M+sUFe38RPaMjHpyc1GRctvQDzJGm+8ZRujYDH+fGNzVDDlRyRnsVanFGNdyfhmy\n"
"BN2D2+2VEvzAWlaGg2wQN8gF3+luavIVEgETXODZPa5FF7ulmQmhqGrZcw6WtDmY\n"
"hUbNmbL7AgMBAAGjgZUwgZIwDwYDVR0TAQH/BAUwAwEB/zAuBgNVHR4BAf8EJDAi\n"
"oA8wDYILZXhhbXBsZS5jb22hDzANggtleGFtcGxlLm9yZzAPBgNVHQ8BAf8EBQMD\n"
"BwQAMB0GA1UdDgQWBBTADHRovZKuyhJlxlS4WWrQ7XsFKDAfBgNVHSMEGDAWgBTg\n"
"+khaP8UOjcwSKVxgT+zhh0aWPDANBgkqhkiG9w0BAQsFAAOCAQEASq5yBiib8FPk\n"
"oRONZ4COgGqjXvigeOBRgbHf9AfagpoYDbOKDQS8Iwt9VHZfJxdcJ1OuM1aQqXlN\n"
"dUyf+JdR/24Nv1yrhL+dEfRGka6Db96YuPsbetVhNIiMm2teXDIPgGzAKuTm4xPA\n"
"6zyNVy5AwfDQ5hIZ+EUsfOoerIElNyAbh66law4MWuiv4oyX4u49m5lxLuL6mFpR\n"
"CIZYWjZMa0MJvWMKGm/AhpfEOkbT58Fg5YmxhnKMk6ps1eR6mh3NgH1IbUqvEYNC\n"
"eS42X3kAMxEDseBOMths0gxeLL+IHdQpXnAjZppW8zEIcN3yfknul35r8R6Qt9aK\n"
"q5+/m1ADBw==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwODI2MTE0MjI2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDIe0eOnLaV750K\n"
"4+mVaAftRrJp8t68KJivcRFpkl0ucQs6gwNf9EsVkHineOR3RXypjJ7Hsv+4PIKp\n"
"BhEOTprYUKcBaxHK/NIezV6NrO1AwuD6MtJDQF9jGpSy0F3eRUoBCjVYhTl+JxcZ\n"
"hGHPJd8WMeanQWY4xG4gTwtpjF3tPU5+JGQwLk5SbcLicM2QMG3CapZinOGK3/XC\n"
"Fjsvf5ZhxnixayhfiX/n9BmeP1yxz7YORNYPlL8z1CcLZqJsyjZnNkVwNvl4ft9I\n"
"FOKBLoOTSGocHFIFXh5b50GG6QHgvN+TiAwdpfRTUskWVg8VVIh7ymgDoI2jQhk4\n"
"EeMaZHd/AgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU4PpIWj/FDo3MEilcYE/s4YdGljwwHwYDVR0jBBgwFoAU6XJK\n"
"EOUYTuioWHG+1YBuz0yPFmowDQYJKoZIhvcNAQELBQADggEBAJOCrGvbeRqPj+uL\n"
"2FIfbkYZAx2nGl3RVv5ZK2YeDpU1udxLihc6Sr67OZbiA4QMKxwgI7pupuwXmyql\n"
"vs9dWnNpjzgfc0OqqzVdOFlfw8ew2DQb2sUXCcIkwqXb/pBQ9BvcgdDASu+rm74j\n"
"JWDZlhcqeVhZROKfpsjsl+lHgZ7kANwHtUJg/WvK8J971hgElqeBO1O97cGkw/in\n"
"e8ooK9Lxk3Td+WdI8C7juCYiwsGqFEKuj7b6937uzvpFmm1fYDdOHhTMcHTHIVTr\n"
"uxSSurQ4XSDF6Iuel3+IdpLL79UYJ7Cf4IhBWj0EloF6xWTA6nUYl3gzKpx1Tg1U\n"
"x2+26YY=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwODI2MTE0MjI2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCqLuVrTyiqz+Zs\n"
"9Qw5V2Z1y1YSWU6aRDMs+34rP2gwT41C69HBh2LLRS04iJUVQydwnEJukwKlTNRn\n"
"1lEpvWgtYmySWA2SyI4xkVzCXgwv0k7WyLwa39hfNY1rXAqhDTL8VO0nXxi8hCMW\n"
"ohaXcvsieglhN5uwu6voEdY3Gwtx4V8ysDJ2P9EBo49ZHdpBOv+3YLDxbWZuL/tI\n"
"nYkBUHHfWGhUHsRsu0EGob3SFnfiooCbE/vtmn9rUuBEQDqOjOg3el/aTPJzcMi/\n"
"RTz+8ho17ZrQRKHZGKWq9Skank+2X9FZoYKFCUlBm6RVud1R54QYZEIj7W9ujQLN\n"
"LJrcIwBDAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU6XJKEOUYTuioWHG+1YBuz0yPFmowDQYJKoZIhvcNAQELBQAD\n"
"ggEBAEeXYGhZ8fWDpCGfSGEDX8FTqLwfDXxw18ZJjQJwus7bsJ9K/hAXnasXrn0f\n"
"TJ+uJi8muqzP1V376mSUzlwXIzLZCtbwRdDhJJYRrLvf5zfHxHeDgvDALn+1AduF\n"
"G/GzCVIFsYNSMdKGwNRp6Ucgl43BPZs6Swn2DXrxxW7Gng+8dvUS2XGLLdH6q1O3\n"
"U1EgJilng+VXx9Rg3yCs5xDiehASySsM6MN/+v+Ouf9lkoQCEgrtlW5Lb/neOBlA\n"
"aS8PPQuKkIEggNd8hW88YWQOJXMiCAgFppVp5B1Vbghn9IDJQISx/AXAoDXQvQfE\n"
"bdOzcKFyDuklHl2IQPnYTFxm/G8=\n"
"-----END CERTIFICATE-----\n"
};

static const char bad_long_chain_pem[] = { /* 3rd-cert is broken */
"-----BEGIN CERTIFICATE-----\n"
"MIIDQDCCAiigAwIBAgIMU/xyoxPcYVSaqH7/MA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
"BgNVBAMTBENBLTMwIhgPMjAxNDA4MjYxMTQyMjdaGA85OTk5MTIzMTIzNTk1OVow\n"
"EzERMA8GA1UEChMIc2VydmVyLTQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDkemVOFdbhBX1qwjxQHr3LmPktNEVBmXjrIvyp++dN7gCYzubnpiLcBE+B\n"
"S2b+ppxBYm9ynKijhGrO+lZPCQRXWmqUg4YDfvnEqM4n04dCE98jN4IhwvWZyP3p\n"
"+U8Ra9mVIBAY2MReo1dcJQHNmo560xzxioHsGNQHAfYgVRHiE5hIXchYbWCkBrKt\n"
"XOoSSTmfgCF3L22p6S1q143VoKUr/C9zqinZo6feGAiTprj6YH0tHswjGBbxTFLb\n"
"q3ThbGDR5FNYL5q0FvQRNbjoF4oFitZ3P1Qkrzq7VIJd9k8J1C3g/16U2dDTKqRX\n"
"ejX7maFZ6oRZJASsRSowEs4wTfRpAgMBAAGjgZMwgZAwDAYDVR0TAQH/BAIwADAa\n"
"BgNVHREEEzARgg93d3cuZXhhbXBsZS5jb20wEwYDVR0lBAwwCgYIKwYBBQUHAwEw\n"
"DwYDVR0PAQH/BAUDAwegADAdBgNVHQ4EFgQUAEYPmcA7S/KChiet+Z6+RRmogiww\n"
"HwYDVR0jBBgwFoAUjxZogHO3y4VdOLuibQHsQYdsGgwwDQYJKoZIhvcNAQELBQAD\n"
"ggEBABlA3npOWwl3eBycaLVOsmdPS+fUwhLnF8hxoyKpHe/33k1nIxd7iiqNZ3iw\n"
"6pAjnuRUCjajU+mlx6ekrmga8mpmeD6JH0I3lq+mrPeCeFXm8gc1yJpcFJ/C2l4o\n"
"+3HNY7RJKcfoQxIbiKOtZ6x9E0aYuk3s1Um3Pf8GLwENoou7Stg5qHsLbkN/GBuP\n"
"n3p/4iqik2k7VblldDe3oCob5vMp0qrAEhlNl2Fn65rcB4+bp1EiC1Z+y6X8DpRb\n"
"NomKUsOiGcbFjQ4ptT6fePmPHX1mgDCx+5/22cyBUYElefYP7Xzr+C8tqqO3JFKe\n"
"hqEmQRsll9bkqpu2dh83c3i9u4g=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBAzANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0y\n"
"MCIYDzIwMTQwODI2MTE0MjI3WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC/4ofaL+ilmmM+\n"
"bGaFRy5GYQXtkD8sA3+/GWsunR928fQS68Zh6iWU+gPm52i7Gfbh7piKWA5Tb63w\n"
"unbS6dPsfPSvgRMZGKJpzxqVcBQAnTS4MuDPlXNg3K3HMyVtbxekII8jFeGEJuCL\n"
"mBMT4dI48IZRzj+2mir38w2cQPfomaKtjg2jMokG8Z9/4+SU9VJCcY1/yZk8fCbS\n"
"dBbwhnDq10yvhPCHgX6KMYmoJr28CYgH29Q9sDP1XN3VvAx5X+PtW/6pyF0U5E2e\n"
"gRzVv7Hr3FJKvytbNxRMCoy2YOyvsTP0fIhiXdtkulTKXyiq4cxA+aYByOu1FjU4\n"
"NicWbiZ/AgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQUjxZogHO3y4VdOLuibQHsQYdsGgwwHwYDVR0jBBgwFoAUwAx0\n"
"aL2SrsoSZcZUuFlq0O17BSgwDQYJKoZIhvcNAQELBQADggEBAGQvj8SquT31w8JK\n"
"tHDL4hWOU0EwVwWl4aYsvP17WspiFIIHKApPFfQOD0/Wg9zB48ble5ZSwKA3Vc3B\n"
"DJgd77HgVAd/Nu1TS5TFDKhpuvFPJVpJ3cqt3pTsVGMzf6GRz5kG3Ly/pBgkqiMG\n"
"gv6vTlEvzNe4FcnhNBEaRKpK5Hc5+GnxtfVoki3tjG5u+oa9/OwzAT+7IOyiIKHw\n"
"7F4Cm56QAWMJgVNm329AjZrJLeNuKoQWGueNew4dOe/zlYEaVMG4So74twXQwIAB\n"
"Zko7+wk6eI4CkI4Zair36s1jLkCF8xnL8FExTT3sg6B6KBHaNUuwc67WPILVuFuc\n"
"VfVBOd8=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"XXIDMzCCAhugAwIBAgIBAjANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0x\n"
"MCIYDzIwMTQwODI2MTE0MjI3WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDIf3as4EONSgWu\n"
"Mbm9w3DbKd/su1UWlrYrcpVqmU3MKD5jXBxyoThSBWxmq1+wcNDmE1on6pHY1aad\n"
"k3188JKMC83wEcyQXaiH3DlTYFXXkkI+JJNUGlfAMSoXG248SpoCIOhCETUG03iP\n"
"Z3AZludaHYsvQQXXh1Kl6qn66+bKM53l/YhoQDxhoGaYvO8ZSwKnx5DEiq447jpW\n"
"M+sUFe38RPaMQQXXc1GRctvQDzJGm+8ZRujYDH+fGNzVDDlRyRnsVanFGNdyfhmy\n"
"BN2D2+2VEvzAQQXXg2wQN8gF3+luavIVEgETXODZPa5FF7ulmQmhqGrZcw6WtDmY\n"
"hUbNmbL7AgMBQQXXgZUwgZIwDwYDVR0TAQH/BAUwAwEB/zAuBgNVHR4BAf8EJDAi\n"
"oA8wDYILZXhhQQBsZS5jb22hDzANggtleGFtcGxlLm9yZzAPBgNVHQ8BAf8EBQMD\n"
"BwQAMB0GA1UdQQQWBBTADHRovZKuyhJlxlS4WWrQ7XsFKDAfBgNVHSMEGDAWgBTg\n"
"+khaP8UOjcwSQQxgT+zhh0aWPDANBgkqhkiG9w0BAQsFAAOCAQEASq5yBiib8FPk\n"
"oRONZ4COgGqjQQigeOBRgbHf9AfagpoYDbOKDQS8Iwt9VHZfJxdcJ1OuM1aQqXlN\n"
"dUyf+JdR/24NQQyrhL+dEfRGka6Db96YuPsbetVhNIiMm2teXDIPgGzAKuTm4xPA\n"
"6zyNVy5AwfDQQQIZ+EUsfOoerIElNyAbh66law4MWuiv4oyX4u49m5lxLuL6mFpR\n"
"CIZYWjZMa0MJQQMKGm/AhpfEOkbT58Fg5YmxhnKMk6ps1eR6mh3NgH1IbUqvEYNC\n"
"eS42X3kAMxEDQQBOMths0gxeLL+IHdQpXnAjZppW8zEIcN3yfknul35r8R6Qt9aK\n"
"q5+/m1ADBw==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwODI2MTE0MjI2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDIe0eOnLaV750K\n"
"4+mVaAftRrJp8t68KJivcRFpkl0ucQs6gwNf9EsVkHineOR3RXypjJ7Hsv+4PIKp\n"
"BhEOTprYUKcBaxHK/NIezV6NrO1AwuD6MtJDQF9jGpSy0F3eRUoBCjVYhTl+JxcZ\n"
"hGHPJd8WMeanQWY4xG4gTwtpjF3tPU5+JGQwLk5SbcLicM2QMG3CapZinOGK3/XC\n"
"Fjsvf5ZhxnixayhfiX/n9BmeP1yxz7YORNYPlL8z1CcLZqJsyjZnNkVwNvl4ft9I\n"
"FOKBLoOTSGocHFIFXh5b50GG6QHgvN+TiAwdpfRTUskWVg8VVIh7ymgDoI2jQhk4\n"
"EeMaZHd/AgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU4PpIWj/FDo3MEilcYE/s4YdGljwwHwYDVR0jBBgwFoAU6XJK\n"
"EOUYTuioWHG+1YBuz0yPFmowDQYJKoZIhvcNAQELBQADggEBAJOCrGvbeRqPj+uL\n"
"2FIfbkYZAx2nGl3RVv5ZK2YeDpU1udxLihc6Sr67OZbiA4QMKxwgI7pupuwXmyql\n"
"vs9dWnNpjzgfc0OqqzVdOFlfw8ew2DQb2sUXCcIkwqXb/pBQ9BvcgdDASu+rm74j\n"
"JWDZlhcqeVhZROKfpsjsl+lHgZ7kANwHtUJg/WvK8J971hgElqeBO1O97cGkw/in\n"
"e8ooK9Lxk3Td+WdI8C7juCYiwsGqFEKuj7b6937uzvpFmm1fYDdOHhTMcHTHIVTr\n"
"uxSSurQ4XSDF6Iuel3+IdpLL79UYJ7Cf4IhBWj0EloF6xWTA6nUYl3gzKpx1Tg1U\n"
"x2+26YY=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwODI2MTE0MjI2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCqLuVrTyiqz+Zs\n"
"9Qw5V2Z1y1YSWU6aRDMs+34rP2gwT41C69HBh2LLRS04iJUVQydwnEJukwKlTNRn\n"
"1lEpvWgtYmySWA2SyI4xkVzCXgwv0k7WyLwa39hfNY1rXAqhDTL8VO0nXxi8hCMW\n"
"ohaXcvsieglhN5uwu6voEdY3Gwtx4V8ysDJ2P9EBo49ZHdpBOv+3YLDxbWZuL/tI\n"
"nYkBUHHfWGhUHsRsu0EGob3SFnfiooCbE/vtmn9rUuBEQDqOjOg3el/aTPJzcMi/\n"
"RTz+8ho17ZrQRKHZGKWq9Skank+2X9FZoYKFCUlBm6RVud1R54QYZEIj7W9ujQLN\n"
"LJrcIwBDAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU6XJKEOUYTuioWHG+1YBuz0yPFmowDQYJKoZIhvcNAQELBQAD\n"
"ggEBAEeXYGhZ8fWDpCGfSGEDX8FTqLwfDXxw18ZJjQJwus7bsJ9K/hAXnasXrn0f\n"
"TJ+uJi8muqzP1V376mSUzlwXIzLZCtbwRdDhJJYRrLvf5zfHxHeDgvDALn+1AduF\n"
"G/GzCVIFsYNSMdKGwNRp6Ucgl43BPZs6Swn2DXrxxW7Gng+8dvUS2XGLLdH6q1O3\n"
"U1EgJilng+VXx9Rg3yCs5xDiehASySsM6MN/+v+Ouf9lkoQCEgrtlW5Lb/neOBlA\n"
"aS8PPQuKkIEggNd8hW88YWQOJXMiCAgFppVp5B1Vbghn9IDJQISx/AXAoDXQvQfE\n"
"bdOzcKFyDuklHl2IQPnYTFxm/G8=\n"
"-----END CERTIFICATE-----\n"
};

static const gnutls_datum_t long_chain = {
	(void*)long_chain_pem, sizeof(long_chain_pem)-1
};

static const gnutls_datum_t bad_long_chain = {
	(void*)bad_long_chain_pem, sizeof(bad_long_chain_pem)-1
};

static const test_st tests[] = {
	{.name = "load 5 certs",
	 .certs = &long_chain,
	 .ncerts = 5,
	 .flags = 0
	},
	{.name = "partial load of 5 certs, with expected failure",
	 .certs = &bad_long_chain,
	 .ncerts = 5,
	 .flags = 0,
	 .exp_err = GNUTLS_E_ASN1_TAG_ERROR
	},
	{.name = "load 2 certs out of 5",
	 .certs = &long_chain,
	 .max1 = 2,
	 .ncerts = 2,
	 .flags = 0
	},
	{.name = "load 1 cert out of 5",
	 .certs = &long_chain,
	 .ncerts = 1,
	 .max1 = 1,
	 .max2 = 0,
	 .flags = 0
	},
	{.name = "load 2 certs with fail if exceed",
	 .certs = &long_chain,
	 .ncerts = 2,
	 .max1 = 2,
	 .flags = GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED,
	 .exp_err = GNUTLS_E_SHORT_MEMORY_BUFFER
	},
	{.name = "load 2 certs with fail if exceed and retry",
	 .certs = &long_chain,
	 .ncerts = 5,
	 .max1 = 1,
	 .max2 = 6,
	 .exp_err = 0,
	 .flags = GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED
	},
	{.name = "load certs, fail due to size, and retry, and fail again",
	 .certs = &long_chain,
	 .max1 = 1,
	 .max2 = 3,
	 .ncerts = 5,
	 .flags = GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED,
	 .exp_err = GNUTLS_E_SHORT_MEMORY_BUFFER
	}
};

void doit(void)
{
	unsigned int i;

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		success("checking: %s\n", tests[i].name);

		load_list(tests[i].name, tests[i].certs, tests[i].ncerts,
			  tests[i].max1, tests[i].max2, tests[i].flags,
			  tests[i].exp_err);
	}

	gnutls_global_deinit();
}
