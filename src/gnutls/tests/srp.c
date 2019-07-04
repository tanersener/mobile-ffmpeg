/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || !defined(ENABLE_SRP)

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

#include "utils.h"

static void terminate(void);

/* This program tests the SRP and SRP-RSA ciphersuites.
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
    "MIICVjCCAcGgAwIBAgIERiYdMTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTIxWhcNMDgwNDE3MTMyOTIxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA17pcr6MM8C6pJ1aqU46o63+B\n"
    "dUxrmL5K6rce+EvDasTaDQC46kwTHzYWk95y78akXrJutsoKiFV1kJbtple8DDt2\n"
    "DZcevensf9Op7PuFZKBroEjOd35znDET/z3IrqVgbtm2jFqab7a+n2q9p/CgMyf1\n"
    "tx2S5Zacc1LWn9bIjrECAwEAAaOBkzCBkDAMBgNVHRMBAf8EAjAAMBoGA1UdEQQT\n"
    "MBGCD3Rlc3QuZ251dGxzLm9yZzATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8B\n"
    "Af8EBQMDB6AAMB0GA1UdDgQWBBTrx0Vu5fglyoyNgw106YbU3VW0dTAfBgNVHSME\n"
    "GDAWgBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAaFEPTt+7\n"
    "bzvBuOf7+QmeQcn29kT6Bsyh1RHJXf8KTk5QRfwp6ogbp94JQWcNQ/S7YDFHglD1\n"
    "AwUNBRXwd3riUsMnsxgeSDxYBfJYbDLeohNBsqaPDJb7XailWbMQKfAbFQ8cnOxg\n"
    "rOKLUQRWJ0K3HyXRMhbqjdLIaQiCvQLuizo=\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQDXulyvowzwLqknVqpTjqjrf4F1TGuYvkrqtx74S8NqxNoNALjq\n"
    "TBMfNhaT3nLvxqResm62ygqIVXWQlu2mV7wMO3YNlx696ex/06ns+4VkoGugSM53\n"
    "fnOcMRP/PciupWBu2baMWppvtr6far2n8KAzJ/W3HZLllpxzUtaf1siOsQIDAQAB\n"
    "AoGAYAFyKkAYC/PYF8e7+X+tsVCHXppp8AoP8TEZuUqOZz/AArVlle/ROrypg5kl\n"
    "8YunrvUdzH9R/KZ7saNZlAPLjZyFG9beL/am6Ai7q7Ma5HMqjGU8kTEGwD7K+lbG\n"
    "iomokKMOl+kkbY/2sI5Czmbm+/PqLXOjtVc5RAsdbgvtmvkCQQDdV5QuU8jap8Hs\n"
    "Eodv/tLJ2z4+SKCV2k/7FXSKWe0vlrq0cl2qZfoTUYRnKRBcWxc9o92DxK44wgPi\n"
    "oMQS+O7fAkEA+YG+K9e60sj1K4NYbMPAbYILbZxORDecvP8lcphvwkOVUqbmxOGh\n"
    "XRmTZUuhBrJhJKKf6u7gf3KWlPl6ShKEbwJASC118cF6nurTjuLf7YKARDjNTEws\n"
    "qZEeQbdWYINAmCMj0RH2P0mvybrsXSOD5UoDAyO7aWuqkHGcCLv6FGG+qwJAOVqq\n"
    "tXdUucl6GjOKKw5geIvRRrQMhb/m5scb+5iw8A4LEEHPgGiBaF5NtJZLALgWfo5n\n"
    "hmC8+G8F0F78znQtPwJBANexu+Tg5KfOnzSILJMo3oXiXhf5PqXIDmbN0BKyCKAQ\n"
    "LfkcEcUbVfmDaHpvzwY9VEaoMOKVLitETXdNSxVpvWM=\n"
    "-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};


static void client(int fd, const char *prio, const char *user, const char *pass, int exp_err)
{
	int ret;
	gnutls_session_t session;
	gnutls_srp_client_credentials_t srp_cred;
	gnutls_certificate_credentials_t x509_cred;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_srp_allocate_client_credentials(&srp_cred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	gnutls_srp_set_client_credentials(srp_cred, user, pass);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);
	gnutls_handshake_set_timeout(session, 40 * 1000);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0 && ret == exp_err) {
		if (debug)
			success("client: handshake failed as expected\n");
		goto end;
	}

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_bye(session, GNUTLS_SHUT_WR);
 end:
	close(fd);

	gnutls_deinit(session);

	gnutls_srp_free_client_credentials(srp_cred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

static void terminate(void)
{
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd, const char *prio)
{
	int ret, kx;
	gnutls_session_t session;
	gnutls_srp_server_credentials_t s_srp_cred;
	gnutls_certificate_credentials_t s_x509_cred;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_srp_allocate_server_credentials(&s_srp_cred);
	gnutls_srp_set_server_credentials_file(s_srp_cred, "tpasswd",
						"tpasswd.conf");

	gnutls_certificate_allocate_credentials(&s_x509_cred);
	gnutls_certificate_set_x509_key_mem(s_x509_cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_SRP, s_srp_cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				s_x509_cred);

	gnutls_transport_set_int(session, fd);
	gnutls_handshake_set_timeout(session, 40 * 1000);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	kx = gnutls_kx_get(session);
	if (kx != GNUTLS_KX_SRP && kx != GNUTLS_KX_SRP_RSA &&
	    kx != GNUTLS_KX_SRP_DSS)
		fail("server: unexpected key exchange: %s\n", gnutls_kx_get_name(kx));

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_srp_free_server_credentials(s_srp_cred);
	gnutls_certificate_free_credentials(s_x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *name, const char *prio, const char *user, const char *pass, int exp_err)
{
	int fd[2];
	int ret;

	success("testing: %s\n", name);
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
		int status;
		/* parent */
		client(fd[1], prio, user, pass, exp_err);
		if (exp_err < 0) {
			kill(child, SIGTERM);
			wait(&status);
		} else {
			wait(&status);
			check_wait_status(status);
		}
	} else {
		server(fd[0], prio);
		exit(0);
	}
}

/* test1-7 are valid users, test9 uses parameter 9 which is disallowed by the RFC5054 spec */
const char *tpasswd_file =
    "test:CsrY0PxYlYCAa8UuWUrcjpqBvG6ImlAdGwEUh3tN2DSDBbMWTvnUl7A8Hw7l0zFHwyLH5rh0llrmu/v.Df2FjDEGy0s0rYR5ARE2XlXPl66xhevHj5vitD0Qvq/J0x1v0zMWJSgq/Ah2MoOrw9aBEsQUgf9MddiHQKjE3Vetoq3:3h3cfS0WrBgPUsldDASSK0:1\n"
    "test2:1J14yVX4iBa97cySs2/SduwnSbHxiz7WieE761psJQDxkc5flpumEwXbAgK5PrSZ0aZ6q7zyrAN1apJR1QQPAdyScJ6Jw4zjDP7AnezUVGbUNMJXhsI0NPwSc0c/415XfrnM1139yjWCr1qkcYMoN4bALppMMLB8glJkxy7t.3cmH9MkRRAjXXdUgAvHw2ZFLmB/8TlZDhnDS78xCSgLQs.oubZEEIgOWl7BT2.aW76fW3yKWdVrrHQDYPtR4hKx:11rUG9wSMLHe2Cu2p7dmFY:2\n"
    "test3:LVJZDDuElMHuRt5/fcx64AhJ4erhFvbIhv/XCtD0tJI3OC6yEBzthZ1FSqblri9qtsvboPApbFHwP9WEluGtCOuzOON4LS8sSeQDBO.PaqjTnsmXKPYMKa.SuLXFuRTtdiFRwX2ZRy3GIWoCvxJtPDWCEYGBWfnjjGEYmQWvo534JVtVDyMaFItYlMTOtBSgsg488oJ5hIAU6jVyIQZGPVv8OHsPCpEt2UlTixzI9nAgQ0WL5ShKaAq0dksF/AY7UMKm0oHbtZeqAx6YcBzLbBhNvcEqYzH95ONpr.cUh91iRhVzdVscsFweSCtWsQrVT4zmSRwdsljeFQPqFbdeK:iWkELSVg3JxmyEq.XbjAW:3\n"
    "test4:YziHBXMYwzekToUa6xL1Iq/4AXwpJWO9.Z6.Y6HHGt4eUcZEvVEw4eKEzPmj.K7US59u.X29F9D7xU62yiomPk5t8/3MzDCywlrAvcCVDhXwC3YpZEFl8OgAlp9izNrDErYY33cReBwH8ILHgFBJ2zo3xZqlWjWMrR50fW2J.MMitnx5GoR9dotZWLj9Zti0kODt5bUUeMcJmK/CJorwEtXz6OvuqGIdjrAZDp.5379KFO2smEVb7Qx6JiIDhEqODMMgXJZMSYSbKMgxXC9D.xN/IOn1/TnD.rNHN6LTrGChmbtKpCpuSJb2Bq2dwLFVxE/2UH4/ubYs/s5w9OcN60ypogvmtIWBqW2GyyfbLzHXiNDpJwV.tOktYEvRUG/fF59GF9hISbOIZ.7BhOK.z.iX4T7dEnHhDW16V.QScdOofcrhihb3Fi3Ym8ZpAqBlmlgLGsaGX23idkdF8xHZxJF1cuPosQ1jHjlrhcZotvChJdV3DZdl3m3isKK.bwF8:3QcZGdH5RkBBbB9R/cyNL4:4\n"
    "test5:Ei8lv19Vi3.zrgd2DT6hHNVdd1FS2rTOg5N.ZHfIe6tsquMOQdP5JNVDqHaZL/Hr.ecaH5Y0fYCrdRby4iYWOvXvRadXBGP7noJl5II9qF84J8KUWGpdWOkKyIqXmRsdvafX2wB90JfMar7SyV0whR7taEV3fAWzQXVS7sHBA6Iyuj6qW1AIg/ObwYBR94xJuI8uKX4vGB0ptl90IS1nkI68OK.1PIt47IDdmJRxDxtbq3smZevPH9HxGCbWyPa7wp9GXmt7jjY/KhRnehLCaCWR3qpfTzqqYaohLFQ5KpkhCv0V5hFASioi1d5iVUsmJCCwWvHWf8fLKLSQ16D.yUNp6jH9AnrzBzybT5jdK647RxKpvogU67rDo4GQCMEjqoHfxExHz/LTN1mtDbX.MkphO71zGpE.bBMopQZvOzUJfpOjJwWADenalLvD6MXu4/.Hwf.cNH/cv/ueTRkXD8Fmsj0cmNkdLCel2qi3COWJxNP/B5ICQ5MnHg.S7qDSloYRcTvbU/FKGyoar8nhUdrl6w5sBwn1DKg3yijBucqEnOAPyLOmpAku8kTsbgoGVdQbEXdb7sUliLv9OnAARddRjIvAbO1mnWxHxFekBCmD5EtMVfGUUGM/ubQzjvH7PjsCCgBjo3nTPoCNGxzREich8/ChRdUvkzEuBvZXIc2:1nhQGuJI7yz9b0xtvpI87B:5\n"
    "test7:6aCMlT0VUuuEnX.pn/K7cfQN1.EefEE9UiwzkBT2a4gdT4OY04pcl7kuKLEwvbb9bSfJWjAF8i8vMT.gg1ZSQTBAcWiBzAwHnnnKv4IgtsT0RAoAjYNjVxe26IMeE/XEdcS9OnOzSdEh2uy6.c9wqgzk0pph.KsQaMV4ivjeoUTdY8ccIiGGrLZcLaScCDeLMH.Ow7HFqMCIa07erJ7W2Xe/i7.0lm1p.oiTFbjNLv.6KXXihivldmz.ca9Dg2mqtp2SMCHul4wMDS3UFXka5/H/BwDFgT72OZpyy.wv9yL2ThHHiQtmc4.jkVutZUFH4gMxdln/3UyZDaXyj.UELFbRsA5VTrOcyqpg3nMqRLnBESC//fQjQPDzsIUG4TYeufCxfX6OK3BQq/KSYCIq08lIRRa1qoLE9FAcsnRO6PQXNtjatPJzgwW9mHZy32Bcy0dAu0mlR.35VGt9B72uAo3H8C6fzgLZHAQmAYvcz2b/LV4bT.FUeZz.D5XDIhxHDzLZOFgpZuYXivgf6B.1MgDd227L6AzVl.tfLF6Tr03Sfa5.FNoZLO.WHyHCje1GWGphLjg/C22QjBvV7NBwW50BkJBDO6HARaR/eZCE5qzmwAqrLbhd3DXYBD/0JSWysm3MO8u2Yhq47Vs8ZbcD835lIObjGOfzQL8iFQerO0H.pPQbwVewUg7fyP/TzSXsSQf0.7Otx6fUObWGEAJyY4Zk3YjBj0lwfQGDYuXjKnHxLgpWzWPtRvUbUxrPJMSFyJwGo8lJC5jZdfk/g/zShzgbib0LrYxwYoD1GvEcrLg/ylqEwDQh4/q7brzkpKUu.i4815rvCbPsqe7qFb6t4keDcNboSsFpRAiDttj8b8mcs/aq1YmPv/RKDO1DEu.QIabsJvdw7hw7sKz4m3OGdQEiFtktvihG0HDhY9UyfVTYm4WysZTx4Lf6WdwIFdkGLZJmhk8KdGPsHfSIo4fyIZieLkWa40e0ez5VevkcPN4C2AjXhVKUM5/9Cx09T38I8ZGIxGC.gF8JnXFarLcFjytuaNA7AlzuiEKlYKNf5AGNBXPoeScMJ.AghZLA0ZbsfbDbHUCSljnIuBhAFs8fL6ML/IqX59sORDYEiGKZnybedKYPgdZSRyy1T/qCDcDy6K/9sA4/gDzJ9ZdhUeasmn4GyXgJoHZ5VvT.ctilLkA36cAD8mHI1f8rcKAcsc5XtdQ5Mqqq6VkeXFAD37lnIc3/oVzBUKpHkyO.k0ibhKHkkmldQVpn1d/qUfhQxKq2S5FaOvqDUohERPoKLfEpsO8cd6NOUnwpGAx8wonNlNNIPaW2rJnRJc67zpznrzyXtTbbURl6eJJ/1nLtQy3xw:2Wva3rbYQapchVRUFxMTxT:7\n"
    "test9:1UVtxG4aVjfnc6dPKMq6Cqin3rfrSoqOsGuD0Y6m4CnKqk190gb60JggCPwYbTgISssluub1TjmKlJeEfO18rXxyZgdn3KGJ3mBFLJ5x2t.kOyNRRpMGTK//7FMGiVQeJ12Mlh5p0faixLlHggR3P5e6LjpEZxsTTmU5d8pmACijdkOkuI8uDWKa4Aw.djIoAfUBhmgYGXCzx8axafeRJlZ/QYlx7tAAqdbIVrW2ES3cYTPCT/Yo8Le3IvjPH7Emw5TpIiQa/mcbEO043ewsUCEU9pSwQEyPj0ieXC5fGnTEk2KQ4ZzStgyUBDT4LgB8XGWT/DIQu13pIhwHy6yCuQ:3QFKSzbKxgN9qsll55ZlDu:9";

/* 1-7 are from SRP RFC5054 spec, and 9 is the FFDHE 2048-bit prime */
const char *tpasswd_conf_file =
    "1:Ewl2hcjiutMd3Fu2lgFnUXWSc67TVyy2vwYCKoS9MLsrdJVT9RgWTCuEqWJrfB6uE3LsE9GkOlaZabS7M29sj5TnzUqOLJMjiwEzArfiLr9WbMRANlF68N5AVLcPWvNx6Zjl3m5Scp0BzJBz9TkgfhzKJZ.WtP3Mv/67I/0wmRZ:2\n"
    "2:dUyyhxav9tgnyIg65wHxkzkb7VIPh4o0lkwfOKiPp4rVJrzLRYVBtb76gKlaO7ef5LYGEw3G.4E0jbMxcYBetDy2YdpiP/3GWJInoBbvYHIRO9uBuxgsFKTKWu7RnR7yTau/IrFTdQ4LY/q.AvoCzMxV0PKvD9Odso/LFIItn8PbTov3VMn/ZEH2SqhtpBUkWtmcIkEflhX/YY/fkBKfBbe27/zUaKUUZEUYZ2H2nlCL60.JIPeZJSzsu/xHDVcx:2\n"
    "3:2iQzj1CagQc/5ctbuJYLWlhtAsPHc7xWVyCPAKFRLWKADpASkqe9djWPFWTNTdeJtL8nAhImCn3Sr/IAdQ1FrGw0WvQUstPx3FO9KNcXOwisOQ1VlL.gheAHYfbYyBaxXL.NcJx9TUwgWDT0hRzFzqSrdGGTN3FgSTA1v4QnHtEygNj3eZ.u0MThqWUaDiP87nqha7XnT66bkTCkQ8.7T8L4KZjIImrNrUftedTTBi.WCi.zlrBxDuOM0da0JbUkQlXqvp0yvJAPpC11nxmmZOAbQOywZGmu9nhZNuwTlxjfIro0FOdthaDTuZRL9VL7MRPUDo/DQEyW.d4H.UIlzp:2\n"
    "4:///////////93zgY8MZ2DCJ6Oek0t1pHAG9E28fdp7G22xwcEnER8b5A27cED0JTxvKPiyqwGnimAmfjybyKDq/XDMrjKS95v8MrTc9UViRqJ4BffZVjQml/NBRq1hVjxZXh.rg9dwMkdoGHV4iVvaaePb7iv5izmW1ykA5ZlmMOsaWs75NJccaMFwZz9CzVWsLT8zoZhPOSOlDM88LIkvxLAGTmbfPjPmmrJagyc0JnT6m8oXWXV3AGNaOkDiuxuvvtB1WEXWER9uEYx0UYZxN5NV1lJ5B9tYlBzfLO5nWvbKbywfLgvHNI9XYO.WKG5NAEMeggn2sjCnSD151wCwXL8QlV7BfaxFk515ZRxmgAwd5NNGOCVREN3uMcuUJ7g/MkZDi9CzSUZ9JWIYLXdSxZqYOQqkvhyI/w1jcA26JOTW9pFiXgP58VAnWNUo0Ck.4NLtfXNMnt2OZ0kjb6uWZYJw1qvQinGzjR/E3z48vBWj4WgJhIol//////////:5\n"
    "5:F//////////oG/QeY5emZJ4ncABWDmSqIa2JWYAPynq0Wk.fZiJco9HIWXvZZG4tU.L6RFDEaCRC2iARV9V53TFuJLjRL72HUI5jNPYNdx6z4n2wQOtxMiB/rosz0QtxUuuQ/jQYP.bhfya4NnB7.P9A6PHxEHRFS80VBYXOxy5cDf8DXnLqvff5Z.e/IJFNuDbNIFSewsM76BpLY25KhkUrIa7S9QMRMSCDKvAl9W4yNHi2CeO8Nmoa5v6BZREE.EUTomO3eO3coU3ekm7ee.rnLtmRqnIoTuho/QLM1SOEPL9VEgLQkKLqYOOcFe541LoZbgAgiGjhJCN3GHGUZEeLI6htnowPEpxXGHOs.yAYkfnLrq637spbm.5fk7anwlrhepR2JFN7eoKu4ebOPtEuz8c6jBkQ/4l.WRPYWXas7O2Spx8QcHI7oiO5tiW3BlX5rTwOLriTmc8mBhPHk88ua.WTEMhCKFRM/pW/H2EIuBH8AaX204QSZmIfuVcruXncX2zkbiccSCd66hquZmQb6WqjXKBsYM3wSegr4pesxl2smJUZlakZlmK7xxAfYXyMKTEQy1TcRAMJw2Gmw8ZEw66KLldxHzXAN3EujUlk1lTTY5mI1pG1f4drR1QgPEqwfYDZzt1Xl.tt92cm8zDz3N9D0OncV//////////:5\n"
    "7:3//////////yaFsg8XQC8qnCPYYu3S7D4f0au8YcVCT08BlgOx4viYKKe8UOuq1DtlbHcppJf36p0h2ctoNnGtJ.4rRMrHmaNaXRLsObv.nlHCGkccD.rh2/zSjlG6j.tkE6lxMecVfQwV915yIn/cIIXcKUpaMpt207oueME/1PZQI3OSLTEQQHO/gFqapr.3PLqZtAEjbXnYyrOWXLAxdjKf1t2Mbcrd33LEIhoO1F5qR0ZA625yCf1UHYuspZlZddSi60w60vidWwBi1wAFjSLTy6zCKidUAylsbLWN63cLINpgbMhb5T8c69Zw1H0LSevQYgh4BQqp5mq4K7epg5KXgzySkcJi.uK4MDll2ehgSLTT1WnzivSFXQRXvCUhzQwCsmaprnwCbE1A9M6TpkFI9XhIxclnB/e6sOe8PDXs0dC.o6faKXyh61Tx80oxuHTNUc5TR7S9YC2wsKRY2E9Fe7Jbgp53srlyuFqGZak2qI2f8GW16d8y4gU7vjU8SPeGlRfR9fd39nXgzE8y6fHeDBOL2zebW.dAAjHCwDkxmji4texvBexy51..ogOeV5b7Jcl0NPcoba.WaCEY8pkXXb5Rv.qVOIbmpkBNhxWRtNOXS4WSq0QH9zMmMgcJjEgOZO/TmOR/jzoGfi2FJVGroJG2X98sm/gqqdnm9i7KtB9W9aRUoNKUTZswDxtu/vG6hPvJ3kNRE2z1C06ki6fJxP0ds34NboUmXbg96De.s.lFcnJjHCvikixKknlRVnH7vimbIpCWKL4hrwz2RxZq0JUCqhzPWye1nakIxF0owXNHSXq3z8BNpcvq/lRLNd0lHfWCWhMeG36G2noUMUV9Vxx7wFCZgNf.Dio8lWyTHRV/M5h5IzG7iYj1LAhCZsr.lqZXs1JCNj8FW3VWfvSLxlARuoW6eTMBjyNQTlLGgZsA7x/mwndCiQCJrLpQLidiBlAMCZX/wDTkF0He13wFPZz8OEuIlorR2tHqrkQK.HvjlX5PTAEIRnB.vUGuTtosgJBVZDY.nD1pkJ6wEyWojesTqm1q7wU/Yln7xILszfDhf2HcEgjZd5hazMWq8xHqA/79U2EF5ilZdMKju/sullo4YjaY8Yu4f0Dy1nFhLwWQ8/37D7FyP6pgC6jBoyY6BuE5tVgTIt.Ym8VeUMWp0.rRtJe6Appriw9ufcqg4/W/HFWjtp4Eu7IhQZP5b.YPe2LTmMJp7CK8HeKT.Qj86LtjVg6nrH2zVkTDS/hpQyCUpw9eDP16zEk7dv902KEBI1niruYQ02xLxZWhoHaDflm2RaULMEH7LdVfgfumKE9sLfJVo1zMw82vRd5WoO3TcEtJt///////////:J\n"
    "9:3//////////wtuL5YYkqgQhznM82SzFF7OkSM3pYqsbQdXDa4KP3Fxp9ETpYIRFlbzB.DZOmnrsFQ1iWAkn65wqzyUrTNzPM4aC/KVNmPkq8LZPLKzxHhpjLSJNdzNoJMOJmnmuEQBT.AcYThpx.Xo7V5OeJQjvpKmhCfFI3fvUhmAiOAp9FjXqGYfIxB8u/kvQjgtODVqQ1rFGgFUEKtqhbRjvsDoknaB1wV8xWfjS9u2/E7Dz.Bim3G4pIWqBs6HSlwSwOM3/uvF4ZBkye63m/ux6qnlhNCxjVoyBi8W1SMEyODz5eEonlDA9i6ox/g8Qq8uOIXSb///////////:2\n";

void doit(void)
{
	FILE *fd;

	fd = fopen("tpasswd.conf", "w");
	if (fd == NULL)
		exit(1);

	fwrite(tpasswd_conf_file, 1, strlen(tpasswd_conf_file), fd);
	fclose(fd);

	fd = fopen("tpasswd", "w");
	if (fd == NULL)
		exit(1);

	fwrite(tpasswd_file, 1, strlen(tpasswd_file), fd);
	fclose(fd);

	start("tls1.2 srp-1024", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test", "test", 0);
	start("tls1.2 srp-1536", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test2", "test2", 0);
	start("tls1.2 srp-2048", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test3", "test3", 0);
	start("tls1.2 srp-3072", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test4", "test4", 0);
	start("tls1.2 srp-4096", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test5", "test5", 0);
	start("tls1.2 srp-8192", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test7", "test7", 0);
	start("tls1.2 srp-other", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP", "test9", "test9", GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	start("tls1.2 srp-rsa", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+SRP-RSA", "test", "test", 0);

	/* check whether SRP works with TLS1.3 being prioritized */
	start("tls1.3 and srp-1024", "NORMAL:-KX-ALL:+SRP:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.1", "test", "test", 0);

	/* check whether SRP works with the default protocol set */
	start("default srp-1024", "NORMAL:-KX-ALL:+SRP", "test", "test", 0);

	remove("tpasswd");
	remove("tpasswd.conf");
}

#endif				/* _WIN32 */
