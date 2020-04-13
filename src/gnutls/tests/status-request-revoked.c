/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Daiki Ueno
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

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>

#include "utils.h"

/* This program tests that the client does not send the
 * status request extension if GNUTLS_NO_EXTENSIONS is set.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned char server_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIEKjCCAhKgAwIBAgIIRiBQA6KFBj0wDQYJKoZIhvcNAQELBQAwDzENMAsGA1UE\n"
"AxMEaWNhMTAeFw0xOTEwMjQxNDA1MDBaFw0yMDEwMjQxNDAzMDBaMBoxGDAWBgNV\n"
"BAMTD3Rlc3Quc2VydmVyLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\n"
"ggEBAKgBiCBLx9eqe2tcCdkyDvQb3UZMR/Gs1mHaiW9zUbqnHkMD/N+0B+JRcfW2\n"
"P5WnQRTlSrWM/gFJh+va0Wtnu0VZWdBHhyR8Vq62DskNRSXUSTsQVqktaMmA/yPY\n"
"iYtY5069WUBoa1GD23BRaeoinLtmBEaUIvsAdCPQ5bCdaVSFOLlnuDxF6/bOAQAC\n"
"5EJ3UDAdqqGmHCQAJcKiCim2ttCIquLqAsgalHMKKBAdEm01o+LO6FOHK1OkwA1W\n"
"GiDNaojEojMS87x9VjmdiamvPuAALLAMMQ3fh8DxqAWA4pfkYWJKehnlPHdjPfkO\n"
"GjUvpezsWev5PBJKp5x6ce9vlgMCAwEAAaN/MH0wCQYDVR0TBAIwADALBgNVHQ8E\n"
"BAMCA4gwEwYDVR0lBAwwCgYIKwYBBQUHAwEwGgYDVR0RBBMwEYIPdGVzdC5zZXJ2\n"
"ZXIuY29tMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcwAYYWaHR0cDovLzEwLjEu\n"
"Mi4yNDI6ODg4ODANBgkqhkiG9w0BAQsFAAOCAgEAK9Bo7i8Mj7t8l+nZqQ6ezG6d\n"
"sq5FYkr2h+T5C0Pt2RscMYAKRdBjAXmCTy1jhaojUVIupm/pK1YQAOgSQF5PMhLl\n"
"3W1SiLl2aU1A0HjHpHvN81YP5VeceHgoJrA5VYGYQohIyH9zfSJNb5TyhQcIHiqZ\n"
"aSC64c7sSywHC4vEHYyYu0LVMic4y7EWM2Y5Vh3xhB28jq5ixChCxG/i6rHt1fC+\n"
"1YsKQaE+sAY4QVjMYE8g4SldqMDpnSCiHDFBfWMGD5hGvp4WMfNXpuiDG9M8wAcT\n"
"A93NxnZqmUdksK/waGS7/uj/eY1hMU2Z/TVhaCDk146hH+lOUf6CnwM3MXLOALaz\n"
"eHyfbm/P8XniWhzBQIiY+5wYVath9YlOkRZhAMKRglRNpwXoTKZiJNkqrwaz6RnB\n"
"S19QByi+L6tFP7AxLFd7DKv4FbI2FWh5GyCrqa8rNc3Bh/oxDR0iAUetEFQUjkxN\n"
"x5A0mOnKds0UoTq3nI5t6obgzAjFkiMgVMXyXo4HqfzpAtqIgZd+PJn5snFoJ6Xh\n"
"NPjCYbfBb9LQFlfodWVg0W4mjfp0HypFaBIudgw0ANdQUUOosWFi0H2msj7CJf4G\n"
"crMZmsCvD+xKfwKqph+tH0/e/xFeFmVOSVI78ESJhpRcQ9ODiOA98FTR4W1Nv9gd\n"
"2GOAQzJDUd051fcRBXQ=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIEvzCCAqegAwIBAgIIV9UmBssMHTUwDQYJKoZIhvcNAQELBQAwETEPMA0GA1UE\n"
"AxMGcm9vdGNhMB4XDTE5MTAyNDE0MDQwMFoXDTIwMTAyNDE0MDMwMFowDzENMAsG\n"
"A1UEAxMEaWNhMTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALY5o80n\n"
"QOQWNJnWOEL6Vg/UH84r9TP4ZWKlWmC4K+pi0S+8x7uFBUyxFffS/SaWeoxI2wBm\n"
"ezMjAl1gFQTCxsojxfgS9Ky9fbxdaADeLKW7B8UHRzrKO8I9Khhe82oU87vAYUFX\n"
"cC0ALIE4zpcdezmr53ACloYfTwDy4onl6VKVjwhfZ6PglwZRkOLjSRMbmSScJPGF\n"
"pMx29dhXEFeCyAdqU+H9Bhu0cIwHUeFp4BM1j8NsW6GHLIzioc5f70EX76M9FyRA\n"
"EB/csmEGX37vNjPmmyki6SJ2nFoa7C9o3ty7IzoUdrU8Cfj9o6WdEfCRlIuxLWra\n"
"LFQrduuhk3sZYU2adZa3hJJ3Y+jx0lUBO4TxtO1Maf39Rfp4BzK1WybjCpDnO/Xg\n"
"kU5hDjQX9zRWYPcwPMGEv3wJAezTsp/mx8UGtIlWVpd0z/oVoKvOrGaxx0RYq5SP\n"
"mutaKDCvQ0j6t9wc69fyG5d9iNA3INXLhkFiZqEKpwZsi7RaSjD615EX2GqdIoZ0\n"
"Ib4NEtpMv12/7ti1VFVxNNMaDNiOKRg7/Ha5SrRnCabyEuykSUnxjttNjBEazn1M\n"
"VWIel0scvDtRFDFGklFuABOJmoYGkAPnIpN+H/l17/VtSWBsPy1rnlfgK2ftm773\n"
"5kiEQJn42uh+jbgWaBdt9Q1B+VeV51pnjjc7AgMBAAGjHTAbMAwGA1UdEwQFMAMB\n"
"Af8wCwYDVR0PBAQDAgEGMA0GCSqGSIb3DQEBCwUAA4ICAQBgm8eEsLeGo5eI4plQ\n"
"WtqvyKrrvpa9YachqmQfoARbMBJg4J0Eq3u3yjL1kUHZ5f0IkXeiaw/w5u2oxZTe\n"
"SHHJCCd54NhLzBeTV/GQuDnWqU+GZP3ay+SkzjAMbfkHibVlRZLkeVnDZLRGd5jb\n"
"RMXRj1LMVagI1xVM3Y4PEcDw+Bhp4XFHBUcxtcqFrjJQBbJJYE9A2QiPwoDQlYoy\n"
"gSvVffbb04bDM01pbYOfPL9t1IIiq7KHHOq0vWzvoU+hnAx+U30wNSaeshKuixNa\n"
"PpWKZ1hoejkhddeiypFqhS54oOxaCxArXPFIl/mLPJlztf/s1Xumi0W4fkIACtoY\n"
"SFilawtsf/vC/WesFsQ502IkFpjYCeUavk8nAfZPZg1BwQ+ZLcwZXBuCtqyn9Mk9\n"
"3UgHAiwMLDqeSQShjHWkBeLr5IOYMubT6SuLpd13rz2WOj6ETq7zizUanV8yAeaT\n"
"x/pn1/rVpbzrbEAL5RkYlUK0ZbwpKjTLygiHUXFqpiID4L1OXoJbtbgSQlXFTEXV\n"
"AnG40QNerXjQ8b+BlmFmCY2nxtNFgtVLUHb4vyG1mUcNIYafH+9TD5tUdzDVZP90\n"
"NnU+8i3Ah3qk9B4Cv9wdHY4Mq/m2jTZd060oGb5l5381Ju5tr2BE+xPXCzSk7TsL\n"
"tdq43/hGqq4D2YGCc9E0WnOVxw==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIEwTCCAqmgAwIBAgIIFv+PS+AkgjowDQYJKoZIhvcNAQELBQAwETEPMA0GA1UE\n"
"AxMGcm9vdGNhMB4XDTE5MTAyNDE0MDMwMFoXDTIwMTAyNDE0MDMwMFowETEPMA0G\n"
"A1UEAxMGcm9vdGNhMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAtjmj\n"
"zSdA5BY0mdY4QvpWD9Qfziv1M/hlYqVaYLgr6mLRL7zHu4UFTLEV99L9JpZ6jEjb\n"
"AGZ7MyMCXWAVBMLGyiPF+BL0rL19vF1oAN4spbsHxQdHOso7wj0qGF7zahTzu8Bh\n"
"QVdwLQAsgTjOlx17OavncAKWhh9PAPLiieXpUpWPCF9no+CXBlGQ4uNJExuZJJwk\n"
"8YWkzHb12FcQV4LIB2pT4f0GG7RwjAdR4WngEzWPw2xboYcsjOKhzl/vQRfvoz0X\n"
"JEAQH9yyYQZffu82M+abKSLpInacWhrsL2je3LsjOhR2tTwJ+P2jpZ0R8JGUi7Et\n"
"atosVCt266GTexlhTZp1lreEkndj6PHSVQE7hPG07Uxp/f1F+ngHMrVbJuMKkOc7\n"
"9eCRTmEONBf3NFZg9zA8wYS/fAkB7NOyn+bHxQa0iVZWl3TP+hWgq86sZrHHRFir\n"
"lI+a61ooMK9DSPq33Bzr1/Ibl32I0Dcg1cuGQWJmoQqnBmyLtFpKMPrXkRfYap0i\n"
"hnQhvg0S2ky/Xb/u2LVUVXE00xoM2I4pGDv8drlKtGcJpvIS7KRJSfGO202MERrO\n"
"fUxVYh6XSxy8O1EUMUaSUW4AE4mahgaQA+cik34f+XXv9W1JYGw/LWueV+ArZ+2b\n"
"vvfmSIRAmfja6H6NuBZoF231DUH5V5XnWmeONzsCAwEAAaMdMBswDAYDVR0TBAUw\n"
"AwEB/zALBgNVHQ8EBAMCAQYwDQYJKoZIhvcNAQELBQADggIBAAk7UHoAWMRlcCtH\n"
"qPH4YOuqqhMEqrJ3nRrRffDmRNCy/R2OpYRmI37HItaWmAB/aGK6H3nQG5fHY1/e\n"
"Ypn/uwpYyvpMtYZgeNNFHckXcWQo3C7wOlCwQzWzI9po0zRp3EqTNBneKa4cZoe0\n"
"FxcMfLbHL4SRKE08PZ3NBRW4n01fjjSs3o4cXvhD6puMTwjL581tWhgmfTrYGMvH\n"
"i7/XSUuFKzj74dA1LioEvbi5qy4kCvy1zxLMySXRd8ZtdnlS/tP3dTx+f1qCZaH6\n"
"E3jE7pi24yRmQaiNaO8Ap4uKcPaMXCsqg+TNTID3QJx6hDgQYsD7P64cUJXXhT/S\n"
"bmdawUaWhwZXVCm2VIpYI3GYhnEVpovyqHOsopNfrabCzvuVB/d4wJBO9MJUk/0l\n"
"BBCTJx3DluvkjKlDWxVDgpofElbU+77mEKLLki4G0f12biJLXOoS+jYayHSKbNlT\n"
"5qzXO3swPMNyS1iBdJtmsh3d5JxHa96UlBgKa5pZY2vk+rHUP0j5aLPMqqCixOpE\n"
"rYX6hvg898wlR2enXY//dgnvgprDW9Fs1x/PdaFx6p1EFpGuJX/td7CK633MsRbu\n"
"dirhB+L70skZjiGGR/kY0i6edHFiMoqmyXm3ML9ID3ZWfQV9gDCCIKvz9SqpW08q\n"
"dZHbP85IPw8a3Lzour7HV3acvaKA\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)-1
};

static unsigned char ca_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIEwTCCAqmgAwIBAgIIFv+PS+AkgjowDQYJKoZIhvcNAQELBQAwETEPMA0GA1UE\n"
"AxMGcm9vdGNhMB4XDTE5MTAyNDE0MDMwMFoXDTIwMTAyNDE0MDMwMFowETEPMA0G\n"
"A1UEAxMGcm9vdGNhMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAtjmj\n"
"zSdA5BY0mdY4QvpWD9Qfziv1M/hlYqVaYLgr6mLRL7zHu4UFTLEV99L9JpZ6jEjb\n"
"AGZ7MyMCXWAVBMLGyiPF+BL0rL19vF1oAN4spbsHxQdHOso7wj0qGF7zahTzu8Bh\n"
"QVdwLQAsgTjOlx17OavncAKWhh9PAPLiieXpUpWPCF9no+CXBlGQ4uNJExuZJJwk\n"
"8YWkzHb12FcQV4LIB2pT4f0GG7RwjAdR4WngEzWPw2xboYcsjOKhzl/vQRfvoz0X\n"
"JEAQH9yyYQZffu82M+abKSLpInacWhrsL2je3LsjOhR2tTwJ+P2jpZ0R8JGUi7Et\n"
"atosVCt266GTexlhTZp1lreEkndj6PHSVQE7hPG07Uxp/f1F+ngHMrVbJuMKkOc7\n"
"9eCRTmEONBf3NFZg9zA8wYS/fAkB7NOyn+bHxQa0iVZWl3TP+hWgq86sZrHHRFir\n"
"lI+a61ooMK9DSPq33Bzr1/Ibl32I0Dcg1cuGQWJmoQqnBmyLtFpKMPrXkRfYap0i\n"
"hnQhvg0S2ky/Xb/u2LVUVXE00xoM2I4pGDv8drlKtGcJpvIS7KRJSfGO202MERrO\n"
"fUxVYh6XSxy8O1EUMUaSUW4AE4mahgaQA+cik34f+XXv9W1JYGw/LWueV+ArZ+2b\n"
"vvfmSIRAmfja6H6NuBZoF231DUH5V5XnWmeONzsCAwEAAaMdMBswDAYDVR0TBAUw\n"
"AwEB/zALBgNVHQ8EBAMCAQYwDQYJKoZIhvcNAQELBQADggIBAAk7UHoAWMRlcCtH\n"
"qPH4YOuqqhMEqrJ3nRrRffDmRNCy/R2OpYRmI37HItaWmAB/aGK6H3nQG5fHY1/e\n"
"Ypn/uwpYyvpMtYZgeNNFHckXcWQo3C7wOlCwQzWzI9po0zRp3EqTNBneKa4cZoe0\n"
"FxcMfLbHL4SRKE08PZ3NBRW4n01fjjSs3o4cXvhD6puMTwjL581tWhgmfTrYGMvH\n"
"i7/XSUuFKzj74dA1LioEvbi5qy4kCvy1zxLMySXRd8ZtdnlS/tP3dTx+f1qCZaH6\n"
"E3jE7pi24yRmQaiNaO8Ap4uKcPaMXCsqg+TNTID3QJx6hDgQYsD7P64cUJXXhT/S\n"
"bmdawUaWhwZXVCm2VIpYI3GYhnEVpovyqHOsopNfrabCzvuVB/d4wJBO9MJUk/0l\n"
"BBCTJx3DluvkjKlDWxVDgpofElbU+77mEKLLki4G0f12biJLXOoS+jYayHSKbNlT\n"
"5qzXO3swPMNyS1iBdJtmsh3d5JxHa96UlBgKa5pZY2vk+rHUP0j5aLPMqqCixOpE\n"
"rYX6hvg898wlR2enXY//dgnvgprDW9Fs1x/PdaFx6p1EFpGuJX/td7CK633MsRbu\n"
"dirhB+L70skZjiGGR/kY0i6edHFiMoqmyXm3ML9ID3ZWfQV9gDCCIKvz9SqpW08q\n"
"dZHbP85IPw8a3Lzour7HV3acvaKA\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t ca_cert = { ca_cert_pem,
	sizeof(ca_cert_pem)
};

static unsigned char server_key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEogIBAAKCAQEAqAGIIEvH16p7a1wJ2TIO9BvdRkxH8azWYdqJb3NRuqceQwP8\n"
"37QH4lFx9bY/ladBFOVKtYz+AUmH69rRa2e7RVlZ0EeHJHxWrrYOyQ1FJdRJOxBW\n"
"qS1oyYD/I9iJi1jnTr1ZQGhrUYPbcFFp6iKcu2YERpQi+wB0I9DlsJ1pVIU4uWe4\n"
"PEXr9s4BAALkQndQMB2qoaYcJAAlwqIKKba20Iiq4uoCyBqUcwooEB0SbTWj4s7o\n"
"U4crU6TADVYaIM1qiMSiMxLzvH1WOZ2Jqa8+4AAssAwxDd+HwPGoBYDil+RhYkp6\n"
"GeU8d2M9+Q4aNS+l7OxZ6/k8EkqnnHpx72+WAwIDAQABAoIBAEw4Ba3BM3SgH0Xh\n"
"h4ZFs4sDaSuPR8RYiRnzrw4k3xsy3gPBN2O1pS4DjRPQDqCyNFBqha4/vKyQ010o\n"
"9IEpmkgn9RsMmD7xOdIhPivwHULAQEjPbMFrnHJuV1HH1v6k4qtSM7+In8dnbpJS\n"
"HR7ffQN3kNEEO6pr1kS5bLrnbvWsjpKcqELOMJfJY+uMS/GhITfrhWtm0PagZ1ze\n"
"w/WYHTkgzGOgBeJuOjb6jCfLOuNDsP/RKALnq7eGeHce8w1tRFkpnxHrCquisgaQ\n"
"ZpFIwG8r8VgwRgd/ydjD1+jMgIwx1NW2GBnX91uLCe6/hTMDAub6TPYZkqiHIlDZ\n"
"UcMg5eECgYEA0kdGFO4XjlpLg5y5FmZSNH0UhEYbn67PTb9DMi3ecDPH5zSjbiYa\n"
"0JCFYQqRTPBl6D/cRIIIaWJOBIg7DgzELFTqnZlpWuenSef9sHNIQp/6XhT75aLD\n"
"EUJgcP3oyV0MOhGp48ZnYgmbnBZuoQKpuWV39IqqgSlAUKjTcFQAV9UCgYEAzIk/\n"
"eTkd1tOh+K22pGBaz8hHi5+YJ/YpHKSjryI2I0PgtmrWL8S284pySkGU+oFV4N8Z\n"
"Ajh5+26DVWxQK8LAqRYlgrooF/+85FJuGx47BhnTCVxmypOZILJ0jeJk8StBrAUk\n"
"TvcEVcQr8kFSvUPyz1codEFTECsMYbKP37aeuncCgYBnOa3hoG/X5eOkHE+P+3Ln\n"
"aW+k73WoEfyaQgYOoA3OLt03VtPTwsjvEcMoPDPP/UNJm+/Zgav3b9a0ytuSrhmv\n"
"WZBDBYh+o7Gvyj7zW+RhMH+Lp+lwdVIlKtyFG2AnWZIi/4DS3BbsPaMyIKD2UYRY\n"
"CsO0PE4vUbzM29PQFKyGcQKBgG3hrf/p92XZ/EIk0OIuAZtu9UDFVHDjheKlcGo9\n"
"7uezJ53Yd4jiHYdo8U2DPg32PbS5Ji5TOPUiwdu6fLeFwQsVosFAURnTgh8HSa+3\n"
"5e25Ie79fRuHf9RZCtTOs3v8ySMpAACMJAAPi6xx+4lCX8eUA1+xWHZvKg+yZijB\n"
"azSxAoGABIvXwUi1NaRDF/fMDiwiwnlJf8FdfY3RBbM1X3ZJbhzqxGL3Hfc4vRcB\n"
"zl7xUnP5Ot9trof6AjHsYCRW+FFjrbUs0x56KoIDCTsd8uArmquyKnSrQb+Zu4FK\n"
"b9M8/NMq3h3Ub+yO/YBm1HOSWeJs8pNSMU72j3QhorNIjAsLGyE=\n"
	"-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)-1
};

static int sent = 0;
static int received = 0;

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	success("received status request\n");
	received = 1;
	return 0;
}

#define RESP "\x30\x82\x05\xa3\x0a\x01\x00\xa0\x82\x05\x9c\x30\x82\x05\x98\x06\x09\x2b\x06\x01\x05\x05\x07\x30\x01\x01\x04\x82\x05\x89\x30\x82\x05\x85\x30\x81\x80\xa1\x02\x30\x00\x18\x0f\x32\x30\x31\x39\x31\x32\x30\x35\x31\x38\x30\x31\x30\x34\x5a\x30\x69\x30\x67\x30\x41\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\x46\x2c\x91\xc8\xd2\x57\xe2\xb8\xb1\xd3\xd0\x99\xc1\xfe\x38\x51\x0e\x17\xa9\x50\x04\x14\x11\x92\x6c\xe3\xa7\x50\x77\x21\xfe\x95\xfa\xca\x6d\x3f\xc7\xa9\xaf\xa4\x9e\x82\x02\x08\x46\x20\x50\x03\xa2\x85\x06\x3d\xa1\x11\x18\x0f\x32\x30\x31\x39\x31\x32\x30\x35\x31\x35\x32\x37\x35\x35\x5a\x18\x0f\x32\x30\x31\x39\x31\x32\x30\x35\x31\x38\x30\x31\x30\x34\x5a\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x03\x82\x01\x01\x00\x05\x88\x2c\x3d\x57\xf4\x75\xbf\x7f\xbe\x9e\x0f\xdf\x8f\x6c\x5f\x08\x56\xc4\x04\xc6\xd6\x3c\xfa\x33\x54\x3e\x42\x1c\x77\xda\x3a\x2a\x48\xcf\xfd\xf1\x6e\xb5\x1d\x94\x06\xfa\xfd\xf8\xba\xec\x66\xc3\x22\x7c\x43\xaa\x48\xaa\x58\x3a\xdc\x2a\x55\x44\x78\xc5\x6e\x0d\x1e\x66\xff\x79\x33\xb3\x26\x22\x86\xa0\x0a\xc0\x59\xb1\xdf\x6d\x07\x2d\x86\x2d\x5b\x0b\x29\x0f\xf3\xc1\x39\x21\x05\xf9\xdb\xdd\x47\x11\x6b\x83\xa0\xc7\x24\xbc\xaa\x42\x43\x9e\x20\x1f\x63\x10\x6c\xeb\x94\x7a\x9c\x44\xaa\x24\xfb\xde\x8f\x49\x92\x1c\xc7\x45\x21\xca\xf9\x1a\x11\x54\x4f\x68\xab\xf0\xce\xd3\x0a\xdc\x9f\xc3\x5d\x8d\x7e\xd4\x96\x30\x74\x31\x95\x04\x55\x8d\xf5\xdf\x3f\x34\x8b\x32\xfc\xf0\x4d\x10\xc6\xc4\x46\xfc\x6a\xb1\xa3\x5c\x9a\xde\xf2\x22\xc3\x5f\x08\x8a\x70\x65\xff\xaa\xf5\xc0\x14\x8b\x13\x47\xff\x0c\x72\x6a\x09\x51\xeb\xec\x92\xc5\xfd\x41\x37\x11\x12\x57\x7b\x47\x9e\x25\xd5\xf2\x10\xc2\xf7\xae\x0e\x72\xfb\x2d\x4f\x8d\x54\xe6\x5a\x71\x2b\xfa\x2b\x9c\xd7\x59\xe5\x31\x30\x21\x3f\x7f\xa7\x85\x07\x31\x93\x9d\x6d\x54\xb2\x40\xa9\x78\xef\x24\xc7\xa0\x82\x03\xea\x30\x82\x03\xe6\x30\x82\x03\xe2\x30\x82\x01\xca\xa0\x03\x02\x01\x02\x02\x08\x68\xfe\x28\x8e\x92\xfb\xa8\x37\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x30\x11\x31\x0f\x30\x0d\x06\x03\x55\x04\x03\x13\x06\x72\x6f\x6f\x74\x63\x61\x30\x1e\x17\x0d\x31\x39\x31\x31\x32\x30\x31\x36\x32\x34\x30\x30\x5a\x17\x0d\x32\x30\x31\x30\x32\x34\x31\x34\x30\x33\x30\x30\x5a\x30\x00\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xa8\x01\x88\x20\x4b\xc7\xd7\xaa\x7b\x6b\x5c\x09\xd9\x32\x0e\xf4\x1b\xdd\x46\x4c\x47\xf1\xac\xd6\x61\xda\x89\x6f\x73\x51\xba\xa7\x1e\x43\x03\xfc\xdf\xb4\x07\xe2\x51\x71\xf5\xb6\x3f\x95\xa7\x41\x14\xe5\x4a\xb5\x8c\xfe\x01\x49\x87\xeb\xda\xd1\x6b\x67\xbb\x45\x59\x59\xd0\x47\x87\x24\x7c\x56\xae\xb6\x0e\xc9\x0d\x45\x25\xd4\x49\x3b\x10\x56\xa9\x2d\x68\xc9\x80\xff\x23\xd8\x89\x8b\x58\xe7\x4e\xbd\x59\x40\x68\x6b\x51\x83\xdb\x70\x51\x69\xea\x22\x9c\xbb\x66\x04\x46\x94\x22\xfb\x00\x74\x23\xd0\xe5\xb0\x9d\x69\x54\x85\x38\xb9\x67\xb8\x3c\x45\xeb\xf6\xce\x01\x00\x02\xe4\x42\x77\x50\x30\x1d\xaa\xa1\xa6\x1c\x24\x00\x25\xc2\xa2\x0a\x29\xb6\xb6\xd0\x88\xaa\xe2\xea\x02\xc8\x1a\x94\x73\x0a\x28\x10\x1d\x12\x6d\x35\xa3\xe2\xce\xe8\x53\x87\x2b\x53\xa4\xc0\x0d\x56\x1a\x20\xcd\x6a\x88\xc4\xa2\x33\x12\xf3\xbc\x7d\x56\x39\x9d\x89\xa9\xaf\x3e\xe0\x00\x2c\xb0\x0c\x31\x0d\xdf\x87\xc0\xf1\xa8\x05\x80\xe2\x97\xe4\x61\x62\x4a\x7a\x19\xe5\x3c\x77\x63\x3d\xf9\x0e\x1a\x35\x2f\xa5\xec\xec\x59\xeb\xf9\x3c\x12\x4a\xa7\x9c\x7a\x71\xef\x6f\x96\x03\x02\x03\x01\x00\x01\xa3\x4f\x30\x4d\x30\x09\x06\x03\x55\x1d\x13\x04\x02\x30\x00\x30\x0b\x06\x03\x55\x1d\x0f\x04\x04\x03\x02\x07\x80\x30\x13\x06\x03\x55\x1d\x25\x04\x0c\x30\x0a\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x09\x30\x1e\x06\x09\x60\x86\x48\x01\x86\xf8\x42\x01\x0d\x04\x11\x16\x0f\x78\x63\x61\x20\x63\x65\x72\x74\x69\x66\x69\x63\x61\x74\x65\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x03\x82\x02\x01\x00\x82\x9d\x8f\xa1\x17\x9b\x3b\xee\x86\x1c\xee\x33\xeb\x80\x71\xb5\x7e\x6b\xd7\xcf\x7d\x9a\x8b\x80\x2b\x3c\x65\xde\xe1\x65\x00\x3b\x4a\x27\x7a\x5d\x63\x19\x4e\x59\xde\xfa\x38\x01\x2b\x09\x91\xc1\x70\x81\x8c\x87\x9b\x17\x68\x22\x88\xf2\x57\x8f\x15\x52\x12\x0f\x1d\x43\x2b\xff\x83\x00\x2f\xd0\xf5\xc7\x93\xd4\xf2\x14\xfd\x94\xcc\x9f\x72\x75\x99\x44\x54\xdc\x6a\x39\x75\x80\xd7\x07\x9c\xb9\x67\xe3\xac\x4b\x72\x9f\xe0\x5d\x00\x6e\x60\xc5\x26\xaf\x9f\xf7\x94\xaa\xb1\xa2\x6f\xa0\xe4\xe8\x0d\x1c\x4e\x34\xe8\xa5\x06\x5c\x31\x64\x09\xf3\x67\xea\xe8\x45\x68\xc1\x13\x21\x41\x38\x9c\x2c\xf9\x6c\xb8\x79\xf4\xae\x8c\x27\x12\xa3\x0a\x0f\x12\x56\xbc\xda\x77\x23\xf0\xe2\xa2\x81\xf9\xdd\x0d\x69\x77\xc3\x3d\x08\x9d\xfe\xac\x18\x14\x83\x49\x67\xde\x85\x3a\x09\xd4\x4f\xec\x85\x85\xbc\xab\xd1\xc8\x01\x83\x74\x34\xc0\x03\x4e\x52\x3c\xb2\xed\x3b\xc0\x66\xa7\x41\xbf\x77\x3b\xcc\x12\xee\xf9\x2f\xd8\x50\x6d\x54\xc5\xf8\x5e\x14\x61\x81\x24\xdb\xcb\xf3\xb4\x25\x84\xc6\x3b\x99\x35\x07\x2e\xd0\xb3\x05\x38\xdf\x64\x21\x71\x9e\xe2\xf2\xce\xbc\x27\x80\x4e\x53\x97\xd3\xe1\xc1\x15\x46\x24\x0c\xc5\x86\x0e\x5b\xdf\x22\xb9\xfe\x35\xd6\xf0\x53\xda\x8f\x9b\x9c\x77\x7e\x9e\x41\x6f\x8e\xbc\x7b\xd4\xf0\x6c\x4f\xac\xe2\x91\x69\x8f\x67\x48\xb0\xc8\x80\x06\x10\xb1\x33\xf9\x8b\xf0\x01\x5d\x49\x9a\x5a\x59\xec\xc6\xb4\xad\x79\x9a\x32\x87\x81\x18\xce\x77\xf6\xc6\xa5\xce\x8b\x36\xee\xc6\xcc\x6b\xd7\x76\xbb\x99\xc1\x34\x2c\xda\x6a\x5f\x1d\x47\xc6\x9e\x98\xa0\x1d\xf0\xd4\x8b\x27\x8a\xa4\x7b\x56\xd8\x7c\x12\xa2\x51\x6e\xd1\x52\xa9\xa5\x31\x77\x9f\xf5\x06\xb4\xba\xb4\x60\x24\x55\xa2\x9d\x4b\x02\xcb\xa7\x62\xa5\x3d\x74\x9e\x47\x9e\x14\x84\x0b\x24\xe0\x01\x13\x9c\xf1\x62\xbd\x78\x18\x9b\xa5\xdf\xd8\x77\x7c\xa9\xc7\x09\x94\x61\x79\x41\x60\x2f\xcc\xe1\x15\x28\x3c\x17\x1d\xb6\x95\x78\x28\x91\x9e\xd1\xbc\xd6\x71\xff\x29\x2f\x22\xed\x24\x26\x81\xb8\xb6\x14\x80\x04\x00\x95\xdf\x50\x46\xe6\xa1\xff\x56\x94\xbc\x11\x48\x5c\xbf\xca\xb7\x4f\xac\xa1\x34\x40\x80\x0d\x88\x27\x73\x76\x24\x1a\xa9\x86\x36\x56\x3c\x84\xb8\x97\x38\xa8\x0e\x14\xab\x83\xca\x6b\x64\x7f\xa7\xfb\x86\x63\xc2\x40\xfc"
#define RESP_SIZE (sizeof(RESP)-1)

static int status_func(gnutls_session_t session, void *ptr, gnutls_datum_t *resp)
{
	resp->data = gnutls_malloc(RESP_SIZE);
	if (resp->data == NULL)
		return -1;

	memcpy(resp->data, RESP, RESP_SIZE);
	resp->size = RESP_SIZE;
	sent = 1;
	return 0;
}

#define MAX_BUF 1024

static int cert_verify_callback(gnutls_session_t session)
{
	unsigned int status;
	int ret;

	ret = gnutls_certificate_verify_peers2(session, &status);
	if (ret < 0)
		return -1;
	if (status == (GNUTLS_CERT_INVALID | GNUTLS_CERT_REVOKED)) {
		if (debug)
			success("certificate verify status %x\n", status);
	} else
		fail("certificate verify status doesn't match: %x != %x",
		     status, GNUTLS_CERT_INVALID | GNUTLS_CERT_REVOKED);
	return 0;
}

static void client(int fd, const char *prio)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CERTIFICATE_STATUS,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_certificate_set_verify_function(x509_cred,
					       cert_verify_callback);

	gnutls_certificate_set_x509_trust_mem(x509_cred, &ca_cert, GNUTLS_X509_FMT_PEM);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (received == 0) {
		fail("client: didn't receive status request\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_certificate_set_ocsp_status_request_function(x509_cred, status_func, NULL);

	gnutls_init(&session, GNUTLS_SERVER);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CERTIFICATE_STATUS,
					   GNUTLS_HOOK_PRE,
					   handshake_callback);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		/* failure is expected here */
		goto end;
	}

	if (debug) {
		success("server: Handshake was completed\n");
	}

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (sent == 0) {
		fail("status request was sent\n");
		exit(1);
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	return;
}

static
void start(const char *prio)
{
	pid_t child;
	int fd[2];
	int ret, status = 0;

	success("trying %s\n", prio);

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		/* parent */
		close(fd[1]);
		client(fd[0], prio);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], prio);
		exit(0);
	}
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}
#endif				/* _WIN32 */
