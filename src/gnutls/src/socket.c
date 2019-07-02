/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <socket.h>
#include <c-ctype.h>
#include "sockets.h"
#include "common.h"

#ifdef _WIN32
# undef endservent
# define endservent()
#endif

#define MAX_BUF 4096

/* Functions to manipulate sockets
 */

ssize_t
socket_recv(const socket_st * socket, void *buffer, int buffer_size)
{
	int ret;

	if (socket->secure) {
		do {
			ret =
			    gnutls_record_recv(socket->session, buffer,
					       buffer_size);
			if (ret == GNUTLS_E_HEARTBEAT_PING_RECEIVED)
				gnutls_heartbeat_pong(socket->session, 0);
		}
		while (ret == GNUTLS_E_INTERRUPTED
		       || ret == GNUTLS_E_HEARTBEAT_PING_RECEIVED);

	} else
		do {
			ret = recv(socket->fd, buffer, buffer_size, 0);
		}
		while (ret == -1 && errno == EINTR);

	return ret;
}

ssize_t
socket_recv_timeout(const socket_st * socket, void *buffer, int buffer_size, unsigned ms)
{
	int ret;

	if (socket->secure)
		gnutls_record_set_timeout(socket->session, ms);
	ret = socket_recv(socket, buffer, buffer_size);

	if (socket->secure)
		gnutls_record_set_timeout(socket->session, 0);

	return ret;
}

ssize_t
socket_send(const socket_st * socket, const void *buffer, int buffer_size)
{
	return socket_send_range(socket, buffer, buffer_size, NULL);
}


ssize_t
socket_send_range(const socket_st * socket, const void *buffer,
		  int buffer_size, gnutls_range_st * range)
{
	int ret;

	if (socket->secure)
		do {
			if (range == NULL)
				ret =
				    gnutls_record_send(socket->session,
						       buffer,
						       buffer_size);
			else
				ret =
				    gnutls_record_send_range(socket->
							     session,
							     buffer,
							     buffer_size,
							     range);
		}
		while (ret == GNUTLS_E_AGAIN
		       || ret == GNUTLS_E_INTERRUPTED);
	else
		do {
			ret = send(socket->fd, buffer, buffer_size, 0);
		}
		while (ret == -1 && errno == EINTR);

	if (ret > 0 && ret != buffer_size && socket->verbose)
		fprintf(stderr,
			"*** Only sent %d bytes instead of %d.\n", ret,
			buffer_size);

	return ret;
}

static
ssize_t send_line(socket_st * socket, const char *txt)
{
	int len = strlen(txt);
	int ret;

	if (socket->verbose)
		fprintf(stderr, "starttls: sending: %s\n", txt);

	ret = send(socket->fd, txt, len, 0);

	if (ret == -1) {
		fprintf(stderr, "error sending \"%s\"\n", txt);
		exit(2);
	}

	return ret;
}

static
ssize_t wait_for_text(socket_st * socket, const char *txt, unsigned txt_size)
{
	char buf[1024];
	char *pbuf, *p;
	int ret;
	fd_set read_fds;
	struct timeval tv;
	size_t left, got;

	if (txt_size > sizeof(buf))
		abort();

	if (socket->verbose && txt != NULL)
		fprintf(stderr, "starttls: waiting for: \"%.*s\"\n", txt_size, txt);

	pbuf = buf;
	left = sizeof(buf)-1;
	got = 0;

	do {
		FD_ZERO(&read_fds);
		FD_SET(socket->fd, &read_fds);
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		ret = select(socket->fd + 1, &read_fds, NULL, NULL, &tv);
		if (ret > 0)
			ret = recv(socket->fd, pbuf, left, 0);
		if (ret == -1) {
			fprintf(stderr, "error receiving '%s': %s\n", txt, strerror(errno));
			exit(2);
		} else if (ret == 0) {
			fprintf(stderr, "error receiving '%s': Timeout\n", txt);
			exit(2);
		}
		pbuf[ret] = 0;

		if (txt == NULL)
			break;

		if (socket->verbose)
			fprintf(stderr, "starttls: received: %s\n", pbuf);

		pbuf += ret;
		left -= ret;
		got += ret;


		/* check for text after a newline in buffer */
		if (got > txt_size) {
			p = memmem(buf, got, txt, txt_size);
			if (p != NULL && p != buf) {
				p--;
				if (*p == '\n' || *p == '\r' || (*txt == '<' && *p == '>')) // XMPP is not line oriented, uses XML format
					break;
			}
		}
	} while(got < txt_size || strncmp(buf, txt, txt_size) != 0);

	return got;
}

static void
socket_starttls(socket_st * socket)
{
	char buf[512];

	if (socket->secure)
		return;

	if (socket->app_proto == NULL || strcasecmp(socket->app_proto, "https") == 0)
		return;

	if (strcasecmp(socket->app_proto, "smtp") == 0 || strcasecmp(socket->app_proto, "submission") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating SMTP STARTTLS\n");

		wait_for_text(socket, "220 ", 4);
		snprintf(buf, sizeof(buf), "EHLO %s\r\n", socket->hostname);
		send_line(socket, buf);
		wait_for_text(socket, "250 ", 4);
		send_line(socket, "STARTTLS\r\n");
		wait_for_text(socket, "220 ", 4);
	} else if (strcasecmp(socket->app_proto, "imap") == 0 || strcasecmp(socket->app_proto, "imap2") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating IMAP STARTTLS\n");

		send_line(socket, "a CAPABILITY\r\n");
		wait_for_text(socket, "a OK", 4);
		send_line(socket, "a STARTTLS\r\n");
		wait_for_text(socket, "a OK", 4);
	} else if (strcasecmp(socket->app_proto, "xmpp") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating XMPP STARTTLS\n");

		snprintf(buf, sizeof(buf), "<stream:stream xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' to='%s' version='1.0'>\n", socket->hostname);
		send_line(socket, buf);
		wait_for_text(socket, "<?", 2);
		send_line(socket, "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>");
		wait_for_text(socket, "<proceed", 8);
	} else if (strcasecmp(socket->app_proto, "ldap") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating LDAP STARTTLS\n");
#define LDAP_STR "\x30\x1d\x02\x01\x01\x77\x18\x80\x16\x31\x2e\x33\x2e\x36\x2e\x31\x2e\x34\x2e\x31\x2e\x31\x34\x36\x36\x2e\x32\x30\x30\x33\x37"
		send(socket->fd, LDAP_STR, sizeof(LDAP_STR)-1, 0);
		wait_for_text(socket, NULL, 0);
	} else if (strcasecmp(socket->app_proto, "ftp") == 0 || strcasecmp(socket->app_proto, "ftps") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating FTP STARTTLS\n");

		send_line(socket, "FEAT\r\n");
		wait_for_text(socket, "211 ", 4);
		send_line(socket, "AUTH TLS\r\n");
		wait_for_text(socket, "234", 3);
	} else if (strcasecmp(socket->app_proto, "lmtp") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating LMTP STARTTLS\n");

		wait_for_text(socket, "220 ", 4);
		snprintf(buf, sizeof(buf), "LHLO %s\r\n", socket->hostname);
		send_line(socket, buf);
		wait_for_text(socket, "250 ", 4);
		send_line(socket, "STARTTLS\r\n");
		wait_for_text(socket, "220 ", 4);
	} else if (strcasecmp(socket->app_proto, "pop3") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating POP3 STARTTLS\n");

		wait_for_text(socket, "+OK", 3);
		send_line(socket, "STLS\r\n");
		wait_for_text(socket, "+OK", 3);
	} else if (strcasecmp(socket->app_proto, "nntp") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating NNTP STARTTLS\n");

		wait_for_text(socket, "200 ", 4);
		send_line(socket, "STARTTLS\r\n");
		wait_for_text(socket, "382 ", 4);
	} else if (strcasecmp(socket->app_proto, "sieve") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating Sieve STARTTLS\n");

		wait_for_text(socket, "OK ", 3);
		send_line(socket, "STARTTLS\r\n");
		wait_for_text(socket, "OK ", 3);
	} else if (strcasecmp(socket->app_proto, "postgres") == 0 || strcasecmp(socket->app_proto, "postgresql") == 0) {
		if (socket->verbose)
			log_msg(stdout, "Negotiating PostgreSQL STARTTLS\n");

#define POSTGRES_STR "\x00\x00\x00\x08\x04\xD2\x16\x2F"
		send(socket->fd, POSTGRES_STR, sizeof(POSTGRES_STR)-1, 0);
		wait_for_text(socket, NULL, 0);
	} else {
		if (!c_isdigit(socket->app_proto[0])) {
			static int warned = 0;
			if (warned == 0) {
				fprintf(stderr, "unknown protocol '%s'\n", socket->app_proto);
				warned = 1;
			}
		}
	}

	return;
}

#define CANON_SERVICE(app_proto) \
	if (strcasecmp(app_proto, "xmpp") == 0) \
		app_proto = "xmpp-server"; \

int
starttls_proto_to_port(const char *app_proto)
{
	struct servent *s;

	CANON_SERVICE(app_proto);

	s = getservbyname(app_proto, NULL);
	if (s != NULL) {
		return ntohs(s->s_port);
	}

	endservent();

	return 443;
}

const char *starttls_proto_to_service(const char *app_proto)
{
	struct servent *s;

	CANON_SERVICE(app_proto);

	s = getservbyname(app_proto, NULL);
	if (s != NULL) {
		return s->s_name;
	}
	endservent();

	return "443";
}

void socket_bye(socket_st * socket, unsigned polite)
{
	int ret;

	if (socket->secure && socket->session) {
		if (polite) {
			do
				ret = gnutls_bye(socket->session, GNUTLS_SHUT_WR);
			while (ret == GNUTLS_E_INTERRUPTED
			       || ret == GNUTLS_E_AGAIN);
			if (socket->verbose && ret < 0)
				fprintf(stderr, "*** gnutls_bye() error: %s\n",
					gnutls_strerror(ret));
		}
	}

	if (socket->session) {
		gnutls_deinit(socket->session);
		socket->session = NULL;
	}

	freeaddrinfo(socket->addr_info);
	socket->addr_info = socket->ptr = NULL;
	socket->connect_addrlen = 0;

	free(socket->ip);
	free(socket->hostname);
	free(socket->service);

	shutdown(socket->fd, SHUT_RDWR);	/* no more receptions */
	close(socket->fd);

	gnutls_free(socket->rdata.data);
	socket->rdata.data = NULL;

	if (socket->server_trace)
		fclose(socket->server_trace);
	if (socket->client_trace)
		fclose(socket->client_trace);

	socket->fd = -1;
	socket->secure = 0;
}

/* Handle host:port format.
 */
void canonicalize_host(char *hostname, char *service, unsigned service_size)
{
	char *p;

	if ((p = strchr(hostname, ':'))) {
		unsigned char buf[64];

		if (inet_pton(AF_INET6, hostname, buf) == 1)
			return;

		*p = 0;

		if (service && service_size)
			snprintf(service, service_size, "%s", p+1);
	} else
		p = hostname + strlen(hostname);

	if (p > hostname && p[-1] == '.')
		p[-1] = 0; // remove trailing dot on FQDN
}

static ssize_t
wrap_pull(gnutls_transport_ptr_t ptr, void *data, size_t len)
{
	socket_st *hd = ptr;
	ssize_t r;

	r = recv(hd->fd, data, len, 0);
	if (r > 0 && hd->server_trace) {
		fwrite(data, 1, r, hd->server_trace);
	}
	return r;
}

static ssize_t
wrap_push(gnutls_transport_ptr_t ptr, const void *data, size_t len)
{
	socket_st *hd = ptr;

	if (hd->client_trace) {
		fwrite(data, 1, len, hd->client_trace);
	}

	return send(hd->fd, data, len, 0);
}

/* inline is used to avoid a gcc warning if used in mini-eagain */
inline static int wrap_pull_timeout_func(gnutls_transport_ptr_t ptr,
					   unsigned int ms)
{
	socket_st *hd = ptr;

	return gnutls_system_recv_timeout((gnutls_transport_ptr_t)(long)hd->fd, ms);
}


void
socket_open2(socket_st * hd, const char *hostname, const char *service,
	    const char *app_proto, int flags, const char *msg, gnutls_datum_t *rdata, gnutls_datum_t *edata,
	    FILE *server_trace, FILE *client_trace)
{
	struct addrinfo hints, *res, *ptr;
	int sd, err = 0;
	int udp = flags & SOCKET_FLAG_UDP;
	int ret;
	int fastopen = flags & SOCKET_FLAG_FASTOPEN;
	char buffer[MAX_BUF + 1];
	char portname[16] = { 0 };
	gnutls_datum_t idna;
	char *a_hostname;

	memset(hd, 0, sizeof(*hd));

	if (flags & SOCKET_FLAG_VERBOSE)
		hd->verbose = 1;

	if (rdata) {
		hd->rdata.data = rdata->data;
		hd->rdata.size = rdata->size;
	}

	if (edata) {
		hd->edata.data = edata->data;
		hd->edata.size = edata->size;
	}

	ret = gnutls_idna_map(hostname, strlen(hostname), &idna, 0);
	if (ret < 0) {
		fprintf(stderr, "Cannot convert %s to IDNA: %s\n", hostname, gnutls_strerror(ret));
		exit(1);
	}

	hd->hostname = strdup(hostname);
	a_hostname = (char*)idna.data;

	if (msg != NULL)
		log_msg(stdout, "Resolving '%s:%s'...\n", a_hostname, service);

	/* get server name */
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
	if ((err = getaddrinfo(a_hostname, service, &hints, &res))) {
		fprintf(stderr, "Cannot resolve %s:%s: %s\n", hostname,
			service, gai_strerror(err));
		exit(1);
	}

	sd = -1;
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		sd = socket(ptr->ai_family, ptr->ai_socktype,
			    ptr->ai_protocol);
		if (sd == -1)
			continue;

		if ((err =
		     getnameinfo(ptr->ai_addr, ptr->ai_addrlen, buffer,
				 MAX_BUF, portname, sizeof(portname),
				 NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
			fprintf(stderr, "getnameinfo(): %s\n",
				gai_strerror(err));
			continue;
		}

		if (hints.ai_socktype == SOCK_DGRAM) {
#if defined(IP_DONTFRAG)
			int yes = 1;
			if (setsockopt(sd, IPPROTO_IP, IP_DONTFRAG,
				       (const void *) &yes,
				       sizeof(yes)) < 0)
				perror("setsockopt(IP_DF) failed");
#elif defined(IP_MTU_DISCOVER)
			int yes = IP_PMTUDISC_DO;
			if (setsockopt(sd, IPPROTO_IP, IP_MTU_DISCOVER,
				       (const void *) &yes,
				       sizeof(yes)) < 0)
				perror("setsockopt(IP_DF) failed");
#endif
		}

		if (fastopen && ptr->ai_socktype == SOCK_STREAM
		    && (ptr->ai_family == AF_INET || ptr->ai_family == AF_INET6)) {
			memcpy(&hd->connect_addr, ptr->ai_addr, ptr->ai_addrlen);
			hd->connect_addrlen = ptr->ai_addrlen;

			if (msg)
				log_msg(stdout, "%s '%s:%s' (TFO)...\n", msg, buffer, portname);

		} else {
			if (msg)
				log_msg(stdout, "%s '%s:%s'...\n", msg, buffer, portname);

			if ((err = connect(sd, ptr->ai_addr, ptr->ai_addrlen)) < 0)
				continue;
		}

		hd->fd = sd;
		if (flags & SOCKET_FLAG_STARTTLS) {
			hd->app_proto = app_proto;
			socket_starttls(hd);
			hd->app_proto = NULL;
		}

		if (!(flags & SOCKET_FLAG_SKIP_INIT)) {
			hd->session = init_tls_session(hostname);
			if (hd->session == NULL) {
				fprintf(stderr, "error initializing session\n");
				exit(1);
			}
		}

		if (hd->session) {
			if (hd->edata.data) {
				ret = gnutls_record_send_early_data(hd->session, hd->edata.data, hd->edata.size);
				if (ret < 0) {
					fprintf(stderr, "error sending early data\n");
					exit(1);
				}
			}
			if (hd->rdata.data) {
				gnutls_session_set_data(hd->session, hd->rdata.data, hd->rdata.size);
			}

			if (server_trace)
				hd->server_trace = server_trace;

			if (client_trace)
				hd->client_trace = client_trace;

			gnutls_transport_set_push_function(hd->session, wrap_push);
			gnutls_transport_set_pull_function(hd->session, wrap_pull);
			gnutls_transport_set_pull_timeout_function(hd->session, wrap_pull_timeout_func);
			gnutls_transport_set_ptr(hd->session, hd);
		}

		if (!(flags & SOCKET_FLAG_RAW) && !(flags & SOCKET_FLAG_SKIP_INIT)) {
			err = do_handshake(hd);
			if (err == GNUTLS_E_PUSH_ERROR) { /* failed connecting */
				gnutls_deinit(hd->session);
				hd->session = NULL;
				continue;
			}
			else if (err < 0) {
				if (!(flags & SOCKET_FLAG_DONT_PRINT_ERRORS))
					fprintf(stderr, "*** handshake has failed: %s\n", gnutls_strerror(err));
				exit(1);
			}
		}

		break;
	}

	if (err != 0) {
		int e = errno;
		fprintf(stderr, "Could not connect to %s:%s: %s\n",
				buffer, portname, strerror(e));
		exit(1);
	}

	if (sd == -1) {
		fprintf(stderr, "Could not find a supported socket\n");
		exit(1);
	}

	if ((flags & SOCKET_FLAG_RAW) || (flags & SOCKET_FLAG_SKIP_INIT))
		hd->secure = 0;
	else
		hd->secure = 1;

	hd->fd = sd;
	hd->ip = strdup(buffer);
	hd->service = strdup(portname);
	hd->ptr = ptr;
	hd->addr_info = res;
	gnutls_free(hd->rdata.data);
	hd->rdata.data = NULL;
	gnutls_free(hd->edata.data);
	hd->edata.data = NULL;
	gnutls_free(idna.data);
	return;
}

/* converts a textual service or port to
 * a service.
 */
const char *port_to_service(const char *sport, const char *proto)
{
	unsigned int port;
	struct servent *sr;

	if (!c_isdigit(sport[0]))
		return sport;

	port = atoi(sport);
	if (port == 0)
		return sport;

	port = htons(port);

	sr = getservbyport(port, proto);
	if (sr == NULL) {
		fprintf(stderr,
			"Warning: getservbyport(%s) failed. Using port number as service.\n", sport);
		return sport;
	}

	return sr->s_name;
}

int service_to_port(const char *service, const char *proto)
{
	unsigned int port;
	struct servent *sr;

	port = atoi(service);
	if (port != 0)
		return port;

	sr = getservbyname(service, proto);
	if (sr == NULL) {
		fprintf(stderr, "Warning: getservbyname() failed for '%s/%s'.\n", service, proto);
		exit(1);
	}

	return ntohs(sr->s_port);
}
