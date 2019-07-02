/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

const char *side;
extern const char *_gnutls_default_priority_string;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

struct test_st {
	const char *name;
	const char *add_prio;
	const char *def_prio;
	int exp_err;
	unsigned err_pos;
	unsigned exp_vers;
	int exp_etm;
};

static void start(struct test_st *test)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	const char *ep;
	int cret = GNUTLS_E_AGAIN;

	if (test == NULL)
		success("running gnutls_set_default_priority test\n");
	else
		success("running %s\n", test->name);

	if (test && test->def_prio)
		_gnutls_default_priority_string = test->def_prio;
	else
		_gnutls_default_priority_string = "NORMAL";

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	if (test == NULL) {
		ret = gnutls_set_default_priority(server);
		if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));
	} else {
		ret = gnutls_set_default_priority_append(server, test->add_prio, &ep, 0);
		if (ret < 0) {
			if (test->exp_err == ret) {
			/* the &ep value is only accurate when the default priorities are not overridden;
			 * otherwise it should be a pointer to the start of the string */
				if (strchr(_gnutls_default_priority_string, '@') != 0) {
					if (ep != test->add_prio) {
						fail("error expected error on start of string[%d]: %s\n",
							test->err_pos, test->add_prio);
					}
				} else {
					if (ep-test->add_prio != test->err_pos) {
						fprintf(stderr, "diff: %d\n", (int)(ep-test->add_prio));
						fail("error expected error on different position[%d]: %s\n",
						     test->err_pos, test->add_prio);
					}
				}
				goto cleanup;
			}
			fail("error: %s\n", gnutls_strerror(ret));
		}
	}

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_set_default_priority(client);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	/* check gnutls_certificate_get_ours() - client side */
	{
		const gnutls_datum_t *mcert;

		mcert = gnutls_certificate_get_ours(client);
		if (mcert != NULL) {
			fail("gnutls_certificate_get_ours(): failed\n");
			exit(1);
		}
	}

	if (test && test->exp_vers != 0) {
		if (test->exp_vers != gnutls_protocol_get_version(server)) {
			fail("expected version %s, got %s\n",
			     gnutls_protocol_get_name(test->exp_vers),
			     gnutls_protocol_get_name(gnutls_protocol_get_version(server)));
		}
	}

	/* check the number of certificates received */
	{
		unsigned cert_list_size = 0;
		gnutls_typed_vdata_st data[2];
		unsigned status;

		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size < 2) {
			fprintf(stderr, "received a certificate list of %d!\n", cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fprintf(stderr, "should not have accepted!\n");
			exit(1);
		}

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost";

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fprintf(stderr, "could not verify certificate: %.4x\n", status);
			exit(1);
		}
	}

	if (test && test->exp_etm) {
		ret = gnutls_session_ext_master_secret_status(client);
		if (ret != 1) {
			fprintf(stderr, "Extended master secret wasn't negotiated by default (client ret: %d)\n", ret);
			exit(1);
		}

		ret = gnutls_session_ext_master_secret_status(server);
		if (ret != 1) {
			fprintf(stderr, "Extended master secret wasn't negotiated by default (server ret: %d)\n", ret);
			exit(1);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_certificate_free_credentials(clientx509cred);
 cleanup:
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();
	reset_buffers();
}

struct test_st tests[] = {
	{
		.name = "additional flag",
		.def_prio = "NORMAL",
		.add_prio = "%FORCE_ETM",
		.exp_err = 0
	},
	{
		.name = "additional flag typo1",
		.def_prio = "NORMAL",
		.add_prio = ":%FORCE_ETM",
		.exp_err = GNUTLS_E_INVALID_REQUEST,
		.err_pos = 0
	},
	{
		.name = "additional flag typo2",
		.def_prio = "NORMAL",
		.add_prio = "%FORCE_ETM::%NO_TICKETS",
		.exp_err = GNUTLS_E_INVALID_REQUEST,
		.err_pos = 11
	},
	{
		.name = "additional flag typo3",
		.def_prio = "NORMAL",
		.add_prio = "%FORCE_ETM:%%NO_TICKETS",
		.exp_err = GNUTLS_E_INVALID_REQUEST,
		.err_pos = 11
	},
	{
		.name = "additional flag typo3 (with resolved def prio)",
		.def_prio = "@HELLO",
		.add_prio = "%FORCE_ETM:%%NO_TICKETS",
		.exp_err = GNUTLS_E_INVALID_REQUEST,
		.err_pos = 0
	},
	{
		.name = "additional flag for version (functional)",
		.def_prio = "NORMAL",
		.add_prio = "-VERS-ALL:+VERS-TLS1.1",
		.exp_err = 0,
		.exp_etm = 1,
		.exp_vers = GNUTLS_TLS1_1
	}
};


void doit(void)
{
	start(NULL);
	for (unsigned i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		start(&tests[i]);
	}
}
