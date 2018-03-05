/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#include <arpa/inet.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <common.h>
#include "udp-serv.h"
#include "list.h"

typedef struct {
	gnutls_session_t session;
	int fd;
	struct sockaddr_storage *cli_addr;
	socklen_t cli_addr_size;
} priv_data_st;

static int pull_timeout_func(gnutls_transport_ptr_t ptr, unsigned int ms);
static ssize_t push_func(gnutls_transport_ptr_t p, const void *data,
			 size_t size);
static ssize_t pull_func(gnutls_transport_ptr_t p, void *data,
			 size_t size);

#define MAX_BUFFER 255		/* Longest string to echo */

void udp_server(const char *name, int port, int mtu)
{
	int sock, ret;
	struct sockaddr_storage cli_addr;
	socklen_t cli_addr_size;
	char buffer[MAX_BUFFER];
	priv_data_st priv;
	gnutls_session_t session;
	gnutls_datum_t cookie_key;
	gnutls_dtls_prestate_st prestate;
	unsigned char sequence[8];

	ret = gnutls_key_generate(&cookie_key, GNUTLS_COOKIE_KEY_SIZE);
	if (ret < 0) {
		fprintf(stderr, "Cannot generate key\n");
		exit(1);
	}

	ret = listen_socket(name, port, SOCK_DGRAM);
	if (ret < 0) {
		fprintf(stderr, "Cannot listen\n");
		exit(1);
	}

	for (;;) {
		printf("Waiting for connection...\n");
		sock = wait_for_connection();
		if (sock < 0)
			continue;

		cli_addr_size = sizeof(cli_addr);
		ret =
		    recvfrom(sock, buffer, sizeof(buffer)-1, MSG_PEEK,
			     (struct sockaddr *) &cli_addr,
			     &cli_addr_size);
		if (ret > 0) {
			memset(&prestate, 0, sizeof(prestate));
			ret =
			    gnutls_dtls_cookie_verify(&cookie_key,
						      &cli_addr,
						      cli_addr_size,
						      buffer, ret,
						      &prestate);
			if (ret < 0) {	/* cookie not valid */
				priv_data_st s;

				memset(&s, 0, sizeof(s));
				s.fd = sock;
				s.cli_addr = (void *) &cli_addr;
				s.cli_addr_size = cli_addr_size;

				printf
				    ("Sending hello verify request to %s\n",
				     human_addr((struct sockaddr *)
						&cli_addr,
						cli_addr_size, buffer,
						sizeof(buffer)-1));
				gnutls_dtls_cookie_send(&cookie_key,
							&cli_addr,
							cli_addr_size,
							&prestate,
							(gnutls_transport_ptr_t)
							&s, push_func);

				/* discard peeked data */
				recvfrom(sock, buffer, sizeof(buffer)-1, 0,
					 (struct sockaddr *) &cli_addr,
					 &cli_addr_size);
				continue;
			}
			printf("Accepted connection from %s\n",
			       human_addr((struct sockaddr *)
					  &cli_addr, sizeof(cli_addr),
					  buffer, sizeof(buffer)-1));
		} else
			continue;

		session = initialize_session(1);
		gnutls_dtls_prestate_set(session, &prestate);
		if (mtu)
			gnutls_dtls_set_mtu(session, mtu);

		priv.session = session;
		priv.fd = sock;
		priv.cli_addr = &cli_addr;
		priv.cli_addr_size = cli_addr_size;

		gnutls_transport_set_ptr(session, &priv);
		gnutls_transport_set_push_function(session, push_func);
		gnutls_transport_set_pull_function(session, pull_func);
		gnutls_transport_set_pull_timeout_function(session,
							   pull_timeout_func);

		do {
			ret = gnutls_handshake(session);
		}
		while (ret == GNUTLS_E_AGAIN
		       || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fprintf(stderr, "Error in handshake(): %s\n",
				gnutls_strerror(ret));
			gnutls_deinit(session);
			continue;
		}

		for (;;) {
			do {
				ret =
				    gnutls_record_recv_seq(session, buffer,
							   sizeof(buffer)-1,
							   sequence);
				if (ret ==
				    GNUTLS_E_HEARTBEAT_PING_RECEIVED)
					gnutls_heartbeat_pong(session, 0);
			}
			while (ret == GNUTLS_E_INTERRUPTED
			       || ret == GNUTLS_E_AGAIN
			       || ret == GNUTLS_E_HEARTBEAT_PING_RECEIVED);

			if (ret == GNUTLS_E_REHANDSHAKE) {
				fprintf(stderr,
					"*** Received hello message\n");
				do {
					ret = gnutls_handshake(session);
				}
				while (ret == GNUTLS_E_INTERRUPTED ||
				       ret == GNUTLS_E_AGAIN);

				if (ret == 0)
					continue;
			}
			if (ret < 0) {
				fprintf(stderr, "Error in recv(): %s\n",
					gnutls_strerror(ret));
				break;
			}
			if (ret == 0) {
				printf("EOF\n\n");
				break;
			}

			buffer[ret] = 0;
			printf
			    ("received[%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x]: %s\n",
			     sequence[0], sequence[1], sequence[2],
			     sequence[3], sequence[4], sequence[5],
			     sequence[6], sequence[7], buffer);

			if (check_command(session, buffer) == 0) {
				/* reply back */
				ret =
				    gnutls_record_send(session, buffer,
						       ret);
				if (ret < 0) {
					fprintf(stderr,
						"Error in send(): %s\n",
						gnutls_strerror(ret));
					break;
				}
			}
		}
	}
	gnutls_deinit(session);
}

/* Wait for data to be received within a timeout period in milliseconds
 */
static int pull_timeout_func(gnutls_transport_ptr_t ptr, unsigned int ms)
{
	fd_set rfds;
	struct timeval tv;
	priv_data_st *priv = ptr;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_size;
	int ret;
	char c;

	FD_ZERO(&rfds);
	FD_SET(priv->fd, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = ms * 1000;

	while (tv.tv_usec >= 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec++;
	}

	ret = select(priv->fd + 1, &rfds, NULL, NULL, &tv);

	if (ret <= 0)
		return ret;

	/* only report ok if the next message is from the peer we expect
	 * from 
	 */
	cli_addr_size = sizeof(cli_addr);
	ret =
	    recvfrom(priv->fd, &c, 1, MSG_PEEK,
		     (struct sockaddr *) &cli_addr, &cli_addr_size);
	if (ret > 0) {
		if (cli_addr_size == priv->cli_addr_size
		    && memcmp(&cli_addr, priv->cli_addr,
			      sizeof(cli_addr)) == 0)
			return 1;
	}

	return 0;
}

static ssize_t push_func(gnutls_transport_ptr_t p, const void *data,
			 size_t size)
{
	priv_data_st *priv = p;

	return sendto(priv->fd, data, size, 0, (struct sockaddr*)priv->cli_addr,
		      priv->cli_addr_size);
}

static ssize_t pull_func(gnutls_transport_ptr_t p, void *data, size_t size)
{
	priv_data_st *priv = p;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_size;
	char buffer[64];
	int ret;

	cli_addr_size = sizeof(cli_addr);
	ret =
	    recvfrom(priv->fd, data, size, 0,
		     (struct sockaddr *) &cli_addr, &cli_addr_size);
	if (ret == -1)
		return ret;

	if (cli_addr_size == priv->cli_addr_size
	    && memcmp(&cli_addr, priv->cli_addr, sizeof(cli_addr)) == 0)
		return ret;

	printf("Denied connection from %s\n",
	       human_addr((struct sockaddr *)
			  &cli_addr, sizeof(cli_addr), buffer,
			  sizeof(buffer)));

	gnutls_transport_set_errno(priv->session, EAGAIN);
	return -1;
}
