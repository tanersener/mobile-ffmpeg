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

/* This program tests the MTU calculation in various cipher/mac algorithm combinations
 * in gnutls */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include "eagain-common.h"
#include "cert-common.h"
#include "utils.h"
#include <assert.h>

#define myfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static void dtls_mtu_try(const char *name, const char *client_prio,
		unsigned link_mtu, unsigned tunnel_mtu)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	unsigned dmtu;
	unsigned i;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);

	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER|GNUTLS_DATAGRAM|GNUTLS_NONBLOCK) >= 0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	assert(gnutls_priority_set_direct(server,
				   "NORMAL:+ANON-ECDH:+ANON-DH:+3DES-CBC:+ECDHE-RSA:+DHE-RSA:+RSA:+ECDHE-ECDSA:+SHA256:+CURVE-X25519",
				   NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server, server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_init(&client, GNUTLS_CLIENT|GNUTLS_DATAGRAM|GNUTLS_NONBLOCK);
	if (ret < 0)
		exit(1);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred) >= 0);
	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred) >= 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client, client_pull_timeout_func);
	
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, NULL);
	if (ret < 0) {
		fail("%s: error in priority setting\n", name);
		exit(1);
	}
	success("negotiating %s\n", name);
	HANDSHAKE_DTLS(client, server);

	gnutls_dtls_set_mtu(client, link_mtu);
	dmtu = gnutls_dtls_get_data_mtu(client);
	if (dmtu != tunnel_mtu) {
		fail("%s: Calculated MTU (%d) does not match expected (%d)\n", name, dmtu, tunnel_mtu);
	}

	{
		char *msg = gnutls_malloc(dmtu+1);
		assert(msg);
		memset(msg, 1, dmtu+1);
		ret = gnutls_record_send(client, msg, dmtu+1);
		if (ret != (int)GNUTLS_E_LARGE_PACKET) {
			myfail("could send larger packet than MTU (%d), ret: %d\n", dmtu, ret);
		}

		ret = gnutls_record_send(client, msg, dmtu);
		if (ret != (int)dmtu) {
			myfail("could not send %d bytes (sent %d)\n", dmtu, ret);
		}

		memset(msg, 2, dmtu);
		ret = gnutls_record_recv(server, msg, dmtu);
		if (ret != (int)dmtu) {
			myfail("could not receive %d bytes (received %d)\n", dmtu, ret);
		}

		for (i=0;i<dmtu;i++)
			assert(msg[i]==1);

		gnutls_free(msg);
	}

	gnutls_dtls_set_data_mtu(client, link_mtu);
	dmtu = gnutls_dtls_get_data_mtu(client);
	if (dmtu != link_mtu) {
		if (gnutls_mac_get(client) == GNUTLS_MAC_AEAD)
			fail("%s: got MTU (%d) which does not match expected (%d)\n", name, dmtu, link_mtu);
		else if (dmtu < link_mtu)
			fail("%s: got MTU (%d) smaller than expected (%d)\n", name, dmtu, link_mtu);
	}

	gnutls_dtls_set_mtu(client, link_mtu);
	dmtu = gnutls_dtls_get_mtu(client);
	if (dmtu != link_mtu) {
		fail("%s: got MTU (%d) which does not match expected (%d)\n", name, dmtu, link_mtu);
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
}


void doit(void)
{
	global_init();

	/* check padding in CBC */
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1500, 1435);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1501", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1501, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1502", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1502, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1503", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1503, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1504", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1504, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1505", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1505, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1506", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1506, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1507", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1507, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1508", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1508, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1509", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1509, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1510", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1510, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1511", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1511, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1512", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1512, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1513", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1513, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1514", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1514, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1515", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1515, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1516", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1516, 1451);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1517", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1517, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1518", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1518, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1519", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1519, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1520", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1520, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1521", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1521, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1522", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1522, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1523", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1523, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1524", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1524, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1525", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1525, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1526", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1526, 1467);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1536", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1536, 1483);

	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA256", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA256", 1500, 1423);
	dtls_mtu_try("DTLS 1.2 with 3DES-CBC-HMAC-SHA1", "NORMAL:%NO_ETM:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+3DES-CBC:-MAC-ALL:+SHA1", 1500, 1451);

	/* check non-CBC ciphers */
	dtls_mtu_try("DTLS 1.2 with AES-128-GCM", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-GCM", 1500, 1463);
	if (!gnutls_fips140_mode_enabled())
		dtls_mtu_try("DTLS 1.2 with CHACHA20-POLY1305", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+CHACHA20-POLY1305", 1500, 1471);

	/* check EtM CBC */
	dtls_mtu_try("DTLS 1.2/EtM with AES-128-CBC-HMAC-SHA1", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1500, 1439);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1501", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1501, 1439);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1502", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1502, 1439);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1503", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1503, 1439);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1504", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1504, 1439);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1505", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1505, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1506", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1506, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1507", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1507, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1508", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1508, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1509", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1509, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1510", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1510, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1511", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1511, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1512", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1512, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1513", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1513, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1514", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1514, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1515", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1515, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1516", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1516, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1517", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1517, 1455);
	dtls_mtu_try("DTLS 1.2 with AES-128-CBC-HMAC-SHA1 - mtu:1518", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA1", 1518, 1455);

	dtls_mtu_try("DTLS 1.2/EtM with AES-128-CBC-HMAC-SHA256", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+AES-128-CBC:-MAC-ALL:+SHA256", 1500, 1423);
	dtls_mtu_try("DTLS 1.2/EtM with 3DES-CBC-HMAC-SHA1", "NORMAL:-VERS-ALL:+VERS-DTLS1.2:-CIPHER-ALL:+3DES-CBC:-MAC-ALL:+SHA1", 1500, 1455);

	gnutls_global_deinit();
}
