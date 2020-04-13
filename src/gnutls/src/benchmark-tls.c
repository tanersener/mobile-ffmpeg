/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <math.h>

#define fail(...) \
	{ \
		fprintf(stderr, __VA_ARGS__); \
		exit(1); \
	}

#include "../tests/eagain-common.h"
#include "benchmark.h"

const char *side = "";

#define PRIO_DHE_RSA "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+DHE-RSA:+GROUP-FFDHE3072"
#define PRIO_ECDH "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-SECP256R1"
#define PRIO_ECDH_X25519 "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-X25519"
#define PRIO_ECDHE_ECDSA "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-ECDSA:+CURVE-SECP256R1"
#define PRIO_ECDH_X25519_ECDSA "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-ECDSA:+CURVE-X25519"
#define PRIO_ECDH_X25519_EDDSA "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-EDDSA-ED25519:+COMP-NULL:+ECDHE-ECDSA:+CURVE-X25519"
#define PRIO_RSA "NONE:+VERS-TLS1.2:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_ECDH_RSA_PSS "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-RSA-PSS-SHA256:+COMP-NULL:+ECDHE-RSA:+CURVE-SECP256R1"


#define PRIO_AES_CBC_SHA1 "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_TLS12_AES_GCM "NONE:+VERS-TLS1.2:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_AES_GCM "NONE:+VERS-TLS1.3:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define PRIO_TLS12_AES_CCM "NONE:+VERS-TLS1.2:+AES-128-CCM:+AEAD:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_AES_CCM "NONE:+VERS-TLS1.3:+AES-128-CCM:+AEAD:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define PRIO_TLS12_CHACHA_POLY1305 "NONE:+VERS-TLS1.2:+CHACHA20-POLY1305:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-ALL"
#define PRIO_CHACHA_POLY1305 "NONE:+VERS-TLS1.3:+CHACHA20-POLY1305:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-ALL"
#define PRIO_CAMELLIA_CBC_SHA1 "NONE:+VERS-TLS1.0:+CAMELLIA-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_GOST_CNT "NONE:+VERS-TLS1.2:+GOST28147-TC26Z-CNT:+GOST28147-TC26Z-IMIT:+SIGN-ALL:+SIGN-GOSTR341012-256:+COMP-NULL:+VKO-GOST-12:+GROUP-GOST-ALL"

static const int rsa_bits = 3072, ec_bits = 256;

static unsigned char server_rsa_pss_cert_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIErTCCAuWgAwIBAgIIWTZrqjOeCfIwPQYJKoZIhvcNAQEKMDCgDTALBglghkgB\n"
	"ZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMCASAwFzEVMBMGA1UE\n"
	"AxMMcnNhLXBzcyBjZXJ0MCAXDTE3MDYwNjA4NDUzMVoYDzk5OTkxMjMxMjM1OTU5\n"
	"WjAXMRUwEwYDVQQDEwxyc2EtcHNzIGNlcnQwggHSMD0GCSqGSIb3DQEBCjAwoA0w\n"
	"CwYJYIZIAWUDBAIBoRowGAYJKoZIhvcNAQEIMAsGCWCGSAFlAwQCAaIDAgEgA4IB\n"
	"jwAwggGKAoIBgQDswF+JIWGcyu+JfjTcM8UDRKaxOuLVY0SODV1uaXPB5ZW9nEX/\n"
	"FFYIG+ldSKCyz5JF5ThrdvwqO+GVByuvETJdM7N4i8fzGHU8WIsj/CABAV+SaDT/\n"
	"xb+h1ar9dIehKelBmXQADVFX+xvu9OM5Ft3P/wyO9gWWrR7e/MU/SVzWzMT69+5Y\n"
	"oE4QkrYYCuEBtlVHDo2mmNWGSQ5tUVIWARgXbqsmj4voWkutE/CiT0+g6GQilMAR\n"
	"kROElIhO5NH+u3/Lt2wRQO5tEP1JmSoqvrMOmF16txze8qMzvKg1Eafijv9DR4Nc\n"
	"Cc6s8+g+CZbyODSdAybiyKsC7JCIrQjsnAjgPKKBLuZ1NTmu5liuXO05XsdcBoKD\n"
	"bKNAQdJCz4uxfqTr4CGFgHQk48Nhmq01EGmpwAeA/BOCB5qsWzqURtMX8EVB1Zdo\n"
	"3LD5Vwz18mm+ZdeLPlYy3L/FBpVPDbYoZlFgINUNCQvGgvzqGJAQrKR4w8X/Y6HH\n"
	"9R8sv+U8kNtQI90CAwEAAaNrMGkwDAYDVR0TAQH/BAIwADAUBgNVHREEDTALggls\n"
	"b2NhbGhvc3QwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYDVR0PAQH/BAUDAweAADAd\n"
	"BgNVHQ4EFgQU1TmyUrkZZn4yMf4asV5OKq8bZ1gwPQYJKoZIhvcNAQEKMDCgDTAL\n"
	"BglghkgBZQMEAgGhGjAYBgkqhkiG9w0BAQgwCwYJYIZIAWUDBAIBogMCASADggGB\n"
	"AGxMPB+Z6pgmWNRw5NjIJgnvJfdMWmQib0II5kdU9I1UybrVRUGpI6tFjIB/pRWU\n"
	"SiD8wTZpxfTHkRHUn+Wyhh14XOg2Pdad5Ek2XU/QblL2k4kh1sHdOcCRFbDzP5k8\n"
	"LKIzFcndgnKTRun5368H+NLcXRx/KAi7s9zi4swp9dPxRvNvp8HjQyVhdFi5pK6n\n"
	"pN1Sw/QD22CE1fRVJ3OYxq4sqCEZANhRv6h/M3AcetGt4LR8ErwuzP1fdtuXeumw\n"
	"T0deQ2hhSYZmbkk/S+qHA8as6J224ry7Zr5bhB9hr52yum9yC9SjFy0XEV/895jJ\n"
	"0MDIM33DmPUdnn90Btt+Oq+bgZqTIolifSmcs0sPH10SuxDOnXwkbR44Wu9NbCzx\n"
	"h3VzhlxAdgcnOYSmJnXKWXog4N1BPFrB4rFqXWFF0Avqs4euK81W4IQ4Sk7fYT7C\n"
	"tyrDILPqBhN80Q9Me70y7KRsek6yFn4Jd0Lok6vetaeWtSW0929bhU49b1hkdSzt\n"
	"kw==\n"
	"-----END CERTIFICATE-----\n";

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEOjCCAqKgAwIBAgIMU+I+KjQZpH+ZdjOlMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
    "BgNVBAMTBENBLTAwIhgPMjAxNDA4MDYxNDM5MzhaGA85OTk5MTIzMTIzNTk1OVow\n"
    "EzERMA8GA1UEAxMIc2VydmVyLTEwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGK\n"
    "AoIBgQDswF+JIWGcyu+JfjTcM8UDRKaxOuLVY0SODV1uaXPB5ZW9nEX/FFYIG+ld\n"
    "SKCyz5JF5ThrdvwqO+GVByuvETJdM7N4i8fzGHU8WIsj/CABAV+SaDT/xb+h1ar9\n"
    "dIehKelBmXQADVFX+xvu9OM5Ft3P/wyO9gWWrR7e/MU/SVzWzMT69+5YoE4QkrYY\n"
    "CuEBtlVHDo2mmNWGSQ5tUVIWARgXbqsmj4voWkutE/CiT0+g6GQilMARkROElIhO\n"
    "5NH+u3/Lt2wRQO5tEP1JmSoqvrMOmF16txze8qMzvKg1Eafijv9DR4NcCc6s8+g+\n"
    "CZbyODSdAybiyKsC7JCIrQjsnAjgPKKBLuZ1NTmu5liuXO05XsdcBoKDbKNAQdJC\n"
    "z4uxfqTr4CGFgHQk48Nhmq01EGmpwAeA/BOCB5qsWzqURtMX8EVB1Zdo3LD5Vwz1\n"
    "8mm+ZdeLPlYy3L/FBpVPDbYoZlFgINUNCQvGgvzqGJAQrKR4w8X/Y6HH9R8sv+U8\n"
    "kNtQI90CAwEAAaOBjTCBijAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuCCWxvY2Fs\n"
    "aG9zdDATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8BAf8EBQMDB6AAMB0GA1Ud\n"
    "DgQWBBTVObJSuRlmfjIx/hqxXk4qrxtnWDAfBgNVHSMEGDAWgBQ5vvRl/1WhIqpf\n"
    "ZFiHs89kf3N3OTANBgkqhkiG9w0BAQsFAAOCAYEAC0KQNPASZ7adSMMM3qx0Ny8Z\n"
    "AkcVAtohkjlwCwhoutcavZVyTjdpGydte6nfyTWOjs6ATBV2GhpyH+nvRJaYQFAh\n"
    "7uksjJxptSlaQuJqUI12urzx6BX0kenwh7nNwnLOngSBRqYwQqQdbnZf0w1DAdac\n"
    "vSa/Y1PrDpcXyPHpk7pDrtI9Mj24rIbvjeWM1RfgkNQYLPkZBDQqKkc5UrCA5y3v\n"
    "3motWyTdfvVYL7KWcEmGeKsWaTDkahd8Xhx29WvE4P740AOvXm/nkrE+PkHODbXi\n"
    "iD0a4cO2FPjjVt5ji+iaJTaXBEd9GHklKE6ZTZhj5az9ygQj1m6HZ2i3shWtG2ks\n"
    "AjgnGzsA8Wm/5X6YyR8UND41rS/lAc9yx8Az9Hqzfg8aOyvixYVPNKoTEPAMmypA\n"
    "oQT6g4b989lZFcjrwnLCrwz83jPD683p5IenCnRI5yhuFoQauy2tgHIbC1FRgs0C\n"
    "dyiOeDh80u1fekMVjRztIAwavuwxI6XgRzPSHhWR\n"
    "-----END CERTIFICATE-----\n";

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "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\n"
    "-----END RSA PRIVATE KEY-----\n";

static unsigned char server_ecc_key_pem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHgCAQEEIQDrAKCAbdMKPngHu4zdSQ2Pghob8PhyrbUpWAR8V07E+qAKBggqhkjO\n"
    "PQMBB6FEA0IABDfo4YLPkO4pBpQamtObIV3J6l92vI+RkyNtaQ9gtSWDj20w/aBC\n"
    "WlbcTsRZ2itEpJ6GdLsGOW4RRfmiubzC9JU=\n"
    "-----END EC PRIVATE KEY-----\n";

static unsigned char server_ecc_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBrjCCAVSgAwIBAgIMU+I+axGZmBD/YL96MAoGCCqGSM49BAMCMA8xDTALBgNV\n"
    "BAMTBENBLTAwIhgPMjAxNDA4MDYxNDQwNDNaGA85OTk5MTIzMTIzNTk1OVowEzER\n"
    "MA8GA1UEAxMIc2VydmVyLTEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQ36OGC\n"
    "z5DuKQaUGprTmyFdyepfdryPkZMjbWkPYLUlg49tMP2gQlpW3E7EWdorRKSehnS7\n"
    "BjluEUX5orm8wvSVo4GNMIGKMAwGA1UdEwEB/wQCMAAwFAYDVR0RBA0wC4IJbG9j\n"
    "YWxob3N0MBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMHgAAwHQYD\n"
    "VR0OBBYEFOuSntH2To0gJLH79Ow4wNpBuhmEMB8GA1UdIwQYMBaAFMZ1miRvZAYr\n"
    "nBEymOtPjbfTrnblMAoGCCqGSM49BAMCA0gAMEUCIQCMP3aBcCxSPbCUhihOsUmH\n"
    "G04AgT1PKw8z4LgZ4VGTVAIgYw3IFwS5sSYEAHRZAH8eaTXTz7XFmWmnkve9EBkN\n"
    "cBE=\n"
    "-----END CERTIFICATE-----\n";

static unsigned char server_ed25519_key_pem[] =
	"-----BEGIN PRIVATE KEY-----\n"
	"MC4CAQAwBQYDK2VwBCIEIOXDJXOU6J6XdXx4WfcyPILPYJDH5bRfm9em+DYMkllw\n"
	"-----END PRIVATE KEY-----\n";

static unsigned char server_ed25519_cert_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIBwTCCAWagAwIBAgIIWTZasQWGNVEwCgYIKoZIzj0EAwIwfTELMAkGA1UEBhMC\n"
	"QkUxDzANBgNVBAoTBkdudVRMUzElMCMGA1UECxMcR251VExTIGNlcnRpZmljYXRl\n"
	"IGF1dGhvcml0eTEPMA0GA1UECBMGTGV1dmVuMSUwIwYDVQQDExxHbnVUTFMgY2Vy\n"
	"dGlmaWNhdGUgYXV0aG9yaXR5MCAXDTE3MDYwNjA3MzMwNVoYDzk5OTkxMjMxMjM1\n"
	"OTU5WjAZMRcwFQYDVQQDEw5FZDI1NTE5IHNpZ25lcjAqMAUGAytlcAMhAPMF++lz\n"
	"LIzfyCX0v0B7LIabZWZ/dePW9HexIbW3tYmHo2EwXzAMBgNVHRMBAf8EAjAAMA8G\n"
	"A1UdDwEB/wQFAwMHgAAwHQYDVR0OBBYEFONSSnOdGLzpv3xNcci8ZiKKqzyqMB8G\n"
	"A1UdIwQYMBaAFPC0gf6YEr+1KLlkQAPLzB9mTigDMAoGCCqGSM49BAMCA0kAMEYC\n"
	"IQDHGfSgM44DVZfrP5CF8LSNlFN55ti3Z69YJ0SK8Fy9eQIhAN2UKeX3l8A9Ckcm\n"
	"7barRoh+qx7ZVYpe+5w3JYuxy16w\n"
	"-----END CERTIFICATE-----\n";

#ifdef ENABLE_GOST
static unsigned char server_gost12_256_key_pem[] =
	"-----BEGIN PRIVATE KEY-----\n"
	"MEgCAQAwHwYIKoUDBwEBAQEwEwYHKoUDAgIkAAYIKoUDBwEBAgIEIgQg0+JttJEV\n"
	"Ud+XBzX9q13ByKK+j2b+mEmNIo1yB0wGleo=\n"
	"-----END PRIVATE KEY-----\n";

static unsigned char server_gost12_256_cert_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIC8DCCAVigAwIBAgIIWcZKgxkCMvcwDQYJKoZIhvcNAQELBQAwDzENMAsGA1UE\n"
	"AxMEQ0EtMzAgFw0xOTEwMDgxMDQ4MTZaGA85OTk5MTIzMTIzNTk1OVowDTELMAkG\n"
	"A1UEAxMCR1IwZjAfBggqhQMHAQEBATATBgcqhQMCAiQABggqhQMHAQECAgNDAARA\n"
	"J9sMEEx0JW9QsT5bDqyc0TNcjVg9ZSdp4GkMtShM+OOgyBGrWK3zLP5IzHYSXja8\n"
	"373QrJOUvdX7T7TUk5yU5aOBjTCBijAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuC\n"
	"CWxvY2FsaG9zdDATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8BAf8EBQMDB4AA\n"
	"MB0GA1UdDgQWBBQYSEtdwsYrtnOq6Ya3nt8DgFPCQjAfBgNVHSMEGDAWgBT5qIYZ\n"
	"Y7akFBNgdg8BmjU27/G0rzANBgkqhkiG9w0BAQsFAAOCAYEAR0xtx7MWEP1KyIzM\n"
	"4lXKdTyU4Nve5RcgqF82yR/0odqT5MPoaZDvLuRWEcQryztZD3kmRUmPmn1ujSfc\n"
	"BbPfRnSutDXcf6imq0/U1/TV/BF3vpS1plltzetvibf8MYetHVFQHUBJDZJHh9h7\n"
	"PGwA9SnmnGKFIxFdV6bVOLkPR54Gob9zN3E17KslL19lNtht1pxk9pshwTn35oRY\n"
	"uOdxof9F4XjpI/4WbC8kp15QeG8XyZd5JWSl+niNOqYK31+ilQdVBr4RiZSDIcAg\n"
	"twS5yV9Ap+R8rM8TLbeT2io4rhdUgmDllUf49zV3t6AbVvbsQfkqXmHXW8uW2WBu\n"
	"A8FiXEbIIOb+QIW0ZGwk3BVQ7wdiw1M5w6kYtz5kBtNPxBmc+eu1+e6EAfYbFNr3\n"
	"pkxtMk3veYWHb5s3dHZ4/t2Rn85hWqh03CWwCkKTN3qmEs4/XpybbXE/UE49e7u1\n"
	"FkpM1bT/0gUNsNt5h3pyUzQZdiB0XbdGGFta3tB3+inIO45h\n"
	"-----END CERTIFICATE-----\n";

static const gnutls_datum_t server_gost12_256_key = { server_gost12_256_key_pem,
	sizeof(server_gost12_256_key_pem)-1
};

static const gnutls_datum_t server_gost12_256_cert = { server_gost12_256_cert_pem,
	sizeof(server_gost12_256_cert_pem)-1
};
#endif

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

const gnutls_datum_t server_rsa_pss_cert = { server_rsa_pss_cert_pem,
	sizeof(server_rsa_pss_cert_pem)
};

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

const gnutls_datum_t server_ecc_cert = { server_ecc_cert_pem,
	sizeof(server_ecc_cert_pem)
};

const gnutls_datum_t server_ecc_key = { server_ecc_key_pem,
	sizeof(server_ecc_key_pem)
};

const gnutls_datum_t server_ed25519_cert = { server_ed25519_cert_pem,
	sizeof(server_ed25519_cert_pem)
};

const gnutls_datum_t server_ed25519_key = { server_ed25519_key_pem,
	sizeof(server_ed25519_key_pem)
};

char buffer[64 * 1024];

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static void test_ciphersuite(const char *cipher_prio, int size)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	gnutls_certificate_credentials_t c_certcred, s_certcred;
	gnutls_session_t server;
	int sret, cret;
	const char *str;
	/* Client stuff. */
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	/* Need to enable anonymous KX specifically. */
	int ret;
	struct benchmark_st st;
	gnutls_packet_t packet;
	const char *name;

	/* Init server */
#ifdef ENABLE_ANON
	gnutls_anon_allocate_server_credentials(&s_anoncred);
#endif
	gnutls_certificate_allocate_credentials(&s_certcred);

	gnutls_certificate_set_x509_key_mem(s_certcred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_key_mem(s_certcred, &server_ecc_cert,
					    &server_ecc_key,
					    GNUTLS_X509_FMT_PEM);
#ifdef ENABLE_GOST
	gnutls_certificate_set_x509_key_mem(s_certcred, &server_gost12_256_cert,
					    &server_gost12_256_key,
					    GNUTLS_X509_FMT_PEM);
#endif

	gnutls_init(&server, GNUTLS_SERVER);
	ret = gnutls_priority_set_direct(server, cipher_prio, &str);
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}
#ifdef ENABLE_ANON
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);
#endif
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, s_certcred);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, (gnutls_transport_ptr_t) server);
	reset_buffers();

	/* Init client */
#ifdef ENABLE_ANON
	gnutls_anon_allocate_client_credentials(&c_anoncred);
#endif
	gnutls_certificate_allocate_credentials(&c_certcred);
	gnutls_init(&client, GNUTLS_CLIENT);

	ret = gnutls_priority_set_direct(client, cipher_prio, &str);
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}
#ifdef ENABLE_ANON
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
#endif
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, c_certcred);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, (gnutls_transport_ptr_t) client);

	HANDSHAKE(client, server);

	name = gnutls_cipher_get_name(gnutls_cipher_get(server));
	fprintf(stdout, "%30s - %s  ", name, gnutls_protocol_get_name(
		gnutls_protocol_get_version(server)));
	fflush(stdout);

	ret = gnutls_rnd(GNUTLS_RND_NONCE, buffer, sizeof(buffer));
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}

	start_benchmark(&st);

	do {
		do {
			ret = gnutls_record_send(client, buffer, size);
		}
		while (ret == GNUTLS_E_AGAIN);

		if (ret < 0) {
			fprintf(stderr, "Failed sending to server\n");
			exit(1);
		}

		do {
			ret =
			    gnutls_record_recv_packet(server, &packet);
		}
		while (ret == GNUTLS_E_AGAIN);

		if (ret < 0) {
			fprintf(stderr, "Failed receiving from client: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		st.size += size;
		gnutls_packet_deinit(packet);
	}
	while (benchmark_must_finish == 0);

	stop_benchmark(&st, NULL, 1);

	gnutls_bye(client, GNUTLS_SHUT_WR);
	gnutls_bye(server, GNUTLS_SHUT_WR);

	gnutls_deinit(client);
	gnutls_deinit(server);

#ifdef ENABLE_ANON
	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);
#endif
}

static
double calc_avg(uint64_t *diffs, unsigned int diffs_size)
{
	double avg = 0;
	unsigned int i;

	for (i = 0; i < diffs_size; i++)
		avg += diffs[i];

	avg /= diffs_size;

	return avg;
}

static
double calc_svar(uint64_t *diffs, unsigned int diffs_size,
		   double avg)
{
	double sum = 0, d;
	unsigned int i;

	for (i = 0; i < diffs_size; i++) {
		d = ((double) diffs[i] - avg);
		d *= d;

		sum += d;
	}
	sum /= diffs_size - 1;

	return sum;
}


uint64_t total_diffs[32 * 1024];
unsigned int total_diffs_size = 0;

static void test_ciphersuite_kx(const char *cipher_prio, unsigned pk)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	gnutls_session_t server;
	int sret, cret, ret;
	const char *str;
	char *suite = NULL;
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_certificate_credentials_t c_certcred, s_certcred;
	gnutls_session_t client;
	unsigned i;
	struct benchmark_st st;
	struct timespec tr_start, tr_stop;
	double avg, svar;
	gnutls_priority_t priority_cache;
	const char *scale;

	total_diffs_size = 0;

	/* Init server */
	gnutls_certificate_allocate_credentials(&s_certcred);
#ifdef ENABLE_ANON
	gnutls_anon_allocate_server_credentials(&s_anoncred);
#endif

	ret = 0;
	if (pk == GNUTLS_PK_RSA_PSS)
		ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_rsa_pss_cert,
						    &server_key,
						    GNUTLS_X509_FMT_PEM);
	else if (pk == GNUTLS_PK_RSA)
		ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_cert,
						    &server_key,
						    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "Error in %d: %s\n", __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = 0;
	if (pk == GNUTLS_PK_ECDSA)
		ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_ecc_cert,
						    &server_ecc_key,
						    GNUTLS_X509_FMT_PEM);
	else if (pk == GNUTLS_PK_EDDSA_ED25519)
		ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_ed25519_cert,
						    &server_ed25519_key,
						    GNUTLS_X509_FMT_PEM);
#ifdef ENABLE_GOST
	else if (pk == GNUTLS_PK_GOST_12_256)
		ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_gost12_256_cert,
						    &server_gost12_256_key,
						    GNUTLS_X509_FMT_PEM);
#endif
	if (ret < 0) {
		fprintf(stderr, "Error in %d: %s\n", __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	/* Init client */
#ifdef ENABLE_ANON
	gnutls_anon_allocate_client_credentials(&c_anoncred);
#endif
	gnutls_certificate_allocate_credentials(&c_certcred);

	start_benchmark(&st);

	ret = gnutls_priority_init(&priority_cache, cipher_prio, &str);
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}

	do {

		gnutls_init(&server, GNUTLS_SERVER);
		ret =
		    gnutls_priority_set(server, priority_cache);
		if (ret < 0) {
			fprintf(stderr, "Error in setting priority: %s\n", gnutls_strerror(ret));
			exit(1);
		}
#ifdef ENABLE_ANON
		gnutls_credentials_set(server, GNUTLS_CRD_ANON,
				       s_anoncred);
#endif
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				       s_certcred);
		gnutls_transport_set_push_function(server, server_push);
		gnutls_transport_set_pull_function(server, server_pull);
		gnutls_transport_set_ptr(server,
					 (gnutls_transport_ptr_t) server);
		reset_buffers();

		gnutls_init(&client, GNUTLS_CLIENT);

		ret =
		    gnutls_priority_set(client, priority_cache);
		if (ret < 0) {
			fprintf(stderr, "Error in setting priority: %s\n", gnutls_strerror(ret));
			exit(1);
		}
#ifdef ENABLE_ANON
		gnutls_credentials_set(client, GNUTLS_CRD_ANON,
				       c_anoncred);
#endif
		gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				       c_certcred);

		gnutls_transport_set_push_function(client, client_push);
		gnutls_transport_set_pull_function(client, client_pull);
		gnutls_transport_set_ptr(client,
					 (gnutls_transport_ptr_t) client);

		gettime(&tr_start);

		HANDSHAKE(client, server);

		gettime(&tr_stop);

		if (suite == NULL)
			suite =
			    gnutls_session_get_desc(server);

		gnutls_deinit(client);
		gnutls_deinit(server);

		total_diffs[total_diffs_size++] = timespec_sub_ns(&tr_stop, &tr_start);
		if (total_diffs_size > sizeof(total_diffs)/sizeof(total_diffs[0]))
			abort();

		st.size += 1;
	}
	while (benchmark_must_finish == 0);

	fprintf(stdout, "%s\n - ", suite);
	gnutls_free(suite);
	stop_benchmark(&st, "transactions", 1);
	gnutls_priority_deinit(priority_cache);

	avg = calc_avg(total_diffs, total_diffs_size);

	if (avg < 1000) {
		scale = "ns";
	} else if (avg < 1000000) {
		scale = "\u00B5s";
		avg /= 1000;
		for (i=0;i<total_diffs_size;i++)
			total_diffs[i] /= 1000;
	} else {
		scale = "ms";
		avg /= 1000*1000;
		for (i=0;i<total_diffs_size;i++)
			total_diffs[i] /= 1000*1000;
	}

	svar = calc_svar(total_diffs, total_diffs_size, avg);

	printf(" - avg. handshake time: %.2f %s\n - standard deviation: %.2f %s\n\n",
	       avg, scale, sqrt(svar), scale);

#ifdef ENABLE_ANON
	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);
#endif
}

void benchmark_tls(int debug_level, int ciphers)
{
	int size;

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(debug_level);
	gnutls_global_init();

	if (ciphers != 0) {
		size = 1400;
		printf
		    ("Testing throughput in cipher/MAC combinations (payload: %d bytes)\n",
		     size);

		test_ciphersuite(PRIO_TLS12_AES_GCM, size);
		test_ciphersuite(PRIO_AES_GCM, size);
		test_ciphersuite(PRIO_TLS12_AES_CCM, size);
		test_ciphersuite(PRIO_AES_CCM, size);
		test_ciphersuite(PRIO_TLS12_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_AES_CBC_SHA1, size);
		test_ciphersuite(PRIO_CAMELLIA_CBC_SHA1, size);
#ifdef ENABLE_GOST
		test_ciphersuite(PRIO_GOST_CNT, size);
#endif

		size = 16 * 1024;
		printf
		    ("\nTesting throughput in cipher/MAC combinations (payload: %d bytes)\n",
		     size);
		test_ciphersuite(PRIO_TLS12_AES_GCM, size);
		test_ciphersuite(PRIO_AES_GCM, size);
		test_ciphersuite(PRIO_TLS12_AES_CCM, size);
		test_ciphersuite(PRIO_AES_CCM, size);
		test_ciphersuite(PRIO_TLS12_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_AES_CBC_SHA1, size);
		test_ciphersuite(PRIO_CAMELLIA_CBC_SHA1, size);
#ifdef ENABLE_GOST
		test_ciphersuite(PRIO_GOST_CNT, size);
#endif
	} else {
		printf
		    ("Testing key exchanges (RSA/DH bits: %d, EC bits: %d)\n\n",
		     rsa_bits, ec_bits);
		test_ciphersuite_kx(PRIO_DHE_RSA, GNUTLS_PK_RSA);
		test_ciphersuite_kx(PRIO_ECDH_RSA_PSS, GNUTLS_PK_RSA_PSS);
		test_ciphersuite_kx(PRIO_ECDH, GNUTLS_PK_RSA);
		test_ciphersuite_kx(PRIO_ECDH_X25519, GNUTLS_PK_RSA);
		test_ciphersuite_kx(PRIO_ECDHE_ECDSA, GNUTLS_PK_ECC);
		test_ciphersuite_kx(PRIO_ECDH_X25519_ECDSA, GNUTLS_PK_ECC);
		test_ciphersuite_kx(PRIO_ECDH_X25519_EDDSA, GNUTLS_PK_EDDSA_ED25519);
		test_ciphersuite_kx(PRIO_RSA, GNUTLS_PK_RSA);
#ifdef ENABLE_GOST
		test_ciphersuite_kx(PRIO_GOST_CNT, GNUTLS_PK_GOST_12_256);
#endif
	}

	gnutls_global_deinit();

}
